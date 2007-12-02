// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MemDC.h"
#include <gdiplus.h>
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include ".\revisiongraphwnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

void CRevisionGraphWnd::InitView()
{
	m_bIsRubberBand = false;
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	m_GraphRect.SetRectEmpty();
	m_ViewRect.SetRectEmpty();
	GetViewSize();
	BuildConnections();
	SetScrollbars(0,0,m_ViewRect.Width(),m_ViewRect.Height());
}

void CRevisionGraphWnd::BuildPreview()
{
	m_Preview.DeleteObject();
	if (!m_bShowOverview)
		return;

	float origZoom = m_fZoomFactor;
	// zoom the graph so that it is completely visible in the window
	DoZoom(1.0);
	GetViewSize();
	float horzfact = float(m_GraphRect.Width())/float(REVGRAPH_PREVIEW_WIDTH);
	float vertfact = float(m_GraphRect.Height())/float(REVGRAPH_PREVIEW_HEIGHT);
	float fZoom = 1.0f/(max(horzfact, vertfact));
	if (fZoom > 1.0f)
		fZoom = 1.0f;
	int trycounter = 0;
	m_fZoomFactor = fZoom;
	while ((trycounter < 5)&&((m_GraphRect.Width()>REVGRAPH_PREVIEW_WIDTH)||(m_GraphRect.Height()>REVGRAPH_PREVIEW_HEIGHT)))
	{
		m_fZoomFactor = fZoom;
		DoZoom(m_fZoomFactor);
		GetViewSize();
		fZoom *= 0.95f;
		trycounter++;
	}
	// make sure the preview window has a minimal size
	if ((m_GraphRect.Width()>10) && (m_GraphRect.Height()>10))
	{
		m_previewWidth = max(m_GraphRect.Width(), 30);
		m_previewHeight = max(m_GraphRect.Height(), 30);
	}
	else
	{
		// if the preview window is too small (at least one side is zero)
		// then don't show the preview at all.
		m_previewHeight = 0;
		m_previewWidth = 0;
	}

	CClientDC ddc(this);
	CDC dc;
	if (!dc.CreateCompatibleDC(&ddc))
		return;
	m_Preview.CreateCompatibleBitmap(&ddc, REVGRAPH_PREVIEW_WIDTH, REVGRAPH_PREVIEW_HEIGHT);
	HBITMAP oldbm = (HBITMAP)dc.SelectObject(m_Preview);
	// paint the whole graph
	DrawGraph(&dc, m_ViewRect, 0, 0, true);
	// now we have a bitmap the size of the preview window
	dc.SelectObject(oldbm);
	dc.DeleteDC();

	DoZoom(origZoom);
}

void CRevisionGraphWnd::SetScrollbars(int nVert, int nHorz, int oldwidth, int oldheight)
{
	CRect clientrect;
	GetClientRect(&clientrect);
	CRect * pRect = GetGraphSize();
	SCROLLINFO ScrollInfo;
	ScrollInfo.cbSize = sizeof(SCROLLINFO);
	ScrollInfo.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &ScrollInfo);
	if ((nVert)||(oldheight==0))
		ScrollInfo.nPos = nVert;
	else
		ScrollInfo.nPos = ScrollInfo.nPos * pRect->Height() / oldheight;
	ScrollInfo.fMask = SIF_ALL;
	ScrollInfo.nMin = 0;
	ScrollInfo.nMax = pRect->bottom;
	ScrollInfo.nPage = clientrect.Height();
	ScrollInfo.nTrackPos = 0;
	SetScrollInfo(SB_VERT, &ScrollInfo);
	GetScrollInfo(SB_HORZ, &ScrollInfo);
	if ((nHorz)||(oldwidth==0))
		ScrollInfo.nPos = nHorz;
	else
		ScrollInfo.nPos = ScrollInfo.nPos * pRect->Width() / oldwidth;
	ScrollInfo.nMax = pRect->right;
	ScrollInfo.nPage = clientrect.Width();
	SetScrollInfo(SB_HORZ, &ScrollInfo);
}

void CRevisionGraphWnd::BuildConnections()
{
	// delete all entries which we might have left
	// in the array and free the memory they use.
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	
	// the spacing of the row/col grid (left-top to next left-top)

	float columnSpacing = m_node_rect_width + m_node_space_left + m_node_space_right;
	float rowSpacing = m_node_rect_height + m_node_space_top + m_node_space_bottom;

	for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
	{
		CRevisionEntry * sourceEntry = m_entryPtrs[i];

		// reference coordinate for the connection source

		CPoint sourceLeftTop;
		sourceLeftTop.x = (long)((sourceEntry->column - 1) * columnSpacing + m_node_space_left);
		sourceLeftTop.y = (long)((sourceEntry->row - 1) * rowSpacing + m_node_space_top);

		for (size_t j = 0, count = sourceEntry->copyTargets.size(); j < count; ++j)
		{
			CRevisionEntry * targetEntry = sourceEntry->copyTargets[j];

            // don't draw lines for split branch sections

            if (targetEntry->action == CRevisionEntry::splitEnd)
                continue;

			// reference coordinate for the connection target

			CPoint targetLeftTop;
			targetLeftTop.x = (long)((targetEntry->column - 1) * columnSpacing + m_node_space_left);
			targetLeftTop.y = (long)((targetEntry->row - 1) * rowSpacing + m_node_space_top);

			CPoint source;
			CPoint target;

			if (sourceEntry->column == targetEntry->column)
			{
				// straight vertical line

				source.x = (long)(sourceLeftTop.x + m_node_rect_width / 2);
				source.y = sourceLeftTop.y;

				target.x = source.x;
				target.y = targetLeftTop.y;

				if (target.y < source.y)
					target.y += (long)(m_node_rect_height);
				else
					source.y += (long)(m_node_rect_height);
			}
			else if (sourceEntry->row == targetEntry->row)
			{
				// straight horizontal line

				source.x = sourceLeftTop.x;
				source.y = (long)(sourceLeftTop.y + m_node_rect_height / 2);

				target.x = targetLeftTop.x;
				target.y = source.y;

				if (target.x < source.x)
					target.x += (long)(m_node_rect_width);
				else
					source.x += (long)(m_node_rect_width);
			}
			else
			{
				// curved line: source left / right -> target top / bottom

				source.x = sourceLeftTop.x;
				source.y = (long)(sourceLeftTop.y + m_node_rect_height / 2);

				target.x = (long)(targetLeftTop.x + m_node_rect_width / 2);
				target.y = targetLeftTop.y;

				if (source.x < target.x)
					source.x += (long)(m_node_rect_width);
				if (source.y > target.y)
					target.y += (long)(m_node_rect_height);
			}

			// bezier points

			CPoint * pt = new CPoint[4];
			pt[0] = source;
			pt[1].x = (source.x + target.x) / 2;		// first control point
			pt[1].y = source.y;
			pt[2].x = target.x;							// second control point
			pt[2].y = source.y;
			pt[3] = target;

			// put it into the list

			m_arConnections.Add(pt);
		}
	}
}

CRect * CRevisionGraphWnd::GetGraphSize()
{
	if (m_GraphRect.Height() != 0)
		return &m_GraphRect;
	m_GraphRect.top = 0;
	m_GraphRect.left = 0;

	if ((m_maxColumn == 0) || (m_maxRow == 0) || (m_maxurllength == 0) || m_maxurl.IsEmpty())
	{
		for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
		{
			CRevisionEntry * reventry = m_entryPtrs[i];
			if (m_maxColumn < reventry->column)
				m_maxColumn = reventry->column;
			if (m_maxRow < reventry->row)
				m_maxRow = reventry->row;

			size_t len = reventry->path.GetPath().size();
			if (m_maxurllength < len)
			{
				m_maxurllength = len;
				m_maxurl = CUnicodeUtils::GetUnicode (reventry->path.GetPath().c_str());
			}
		}
	}

	// calculate the width of the nodes by looking
	// at the url lengths
	CRect r;
	CDC * pDC = this->GetDC();
	if (pDC)
	{
		CFont * pOldFont = pDC->SelectObject(GetFont(TRUE));
		pDC->DrawText(m_maxurl, &r, DT_CALCRECT);
		// keep the width inside reasonable values.
		m_node_rect_width = min(500.0f * m_fZoomFactor, r.Width()+40.0f);
		m_node_rect_width = max(NODE_RECT_WIDTH * m_fZoomFactor, m_node_rect_width);
		pDC->SelectObject(pOldFont);
	}
	ReleaseDC(pDC);

	m_GraphRect.right = long(float(m_maxColumn) * (m_node_rect_width + m_node_space_left + m_node_space_right));
	m_GraphRect.bottom = long(float(m_maxRow) * (m_node_rect_height + m_node_space_top + m_node_space_bottom));
	return &m_GraphRect;
}

CRect * CRevisionGraphWnd::GetViewSize()
{
	if (m_ViewRect.Height() != 0)
		return &m_ViewRect;
	m_ViewRect = GetGraphSize();
	CRect rect;
	GetClientRect(&rect);
	if (m_ViewRect.Width() < rect.Width())
	{
		m_ViewRect.left = rect.left;
		m_ViewRect.right = rect.right;
	}
	if (m_ViewRect.Height() < rect.Height())
	{
		m_ViewRect.top = rect.top;
		m_ViewRect.bottom = rect.bottom;
	}
	return &m_ViewRect;
}

int CRevisionGraphWnd::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	if (GetImageEncodersSize(&num, &size)!=Ok)
		return -1;
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	if (GetImageEncoders(num, size, pImageCodecInfo)==Ok)
	{
		for(UINT j = 0; j < num; ++j)
		{
			if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}
		}

	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

void CRevisionGraphWnd::CompareRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);

	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry1->path.GetPath().c_str()));
	url2.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry2->path.GetPath().c_str()));

	SVNRev peg = (SVNRev)(bHead ? m_SelectedEntry1->revision : SVNRev());

	SVNDiff diff(&svn, this->m_hWnd);
	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	diff.ShowCompare(url1, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry1->revision),
		url2, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry2->revision),
		peg);
}

void CRevisionGraphWnd::UnifiedDiffRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);

	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry1->path.GetPath().c_str()));
	url2.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry2->path.GetPath().c_str()));

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowUnifiedDiff(url1, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry1->revision),
						 url2, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry2->revision),
						 m_SelectedEntry1->revision);
}

CTSVNPath CRevisionGraphWnd::DoUnifiedDiff(bool bHead, CString& sRoot, bool& bIsFolder)
{
	theApp.DoWaitCursor(1);
	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, CTSVNPath(_T("test.diff")));
	// find selected objects
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);
	
	// find out if m_sPath points to a file or a folder
	if (SVN::PathIsURL(m_sPath))
	{
		SVNInfo info;
		const SVNInfoData * infodata = info.GetFirstFileInfo(CTSVNPath(m_sPath), SVNRev::REV_HEAD, SVNRev::REV_HEAD);
		if (infodata)
		{
			bIsFolder = (infodata->kind == svn_node_dir);
		}
	}
	else
	{
		bIsFolder = CTSVNPath(m_sPath).IsDirectory();
	}
	
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry1->path.GetPath().c_str()));
	url2.SetFromSVN (sRepoRoot + CUnicodeUtils::GetUnicode (m_SelectedEntry2->path.GetPath().c_str()));
	CTSVNPath url1_temp = url1;
	CTSVNPath url2_temp = url2;
	INT_PTR iMax = min(url1_temp.GetSVNPathString().GetLength(), url2_temp.GetSVNPathString().GetLength());
	INT_PTR i = 0;
	for ( ; ((i<iMax) && (url1_temp.GetSVNPathString().GetAt(i)==url2_temp.GetSVNPathString().GetAt(i))); ++i)
		;
	while (url1_temp.GetSVNPathString().GetLength()>i)
		url1_temp = url1_temp.GetContainingDirectory();

	if (bIsFolder)
		sRoot = url1_temp.GetSVNPathString();
	else
		sRoot = url1_temp.GetContainingDirectory().GetSVNPathString();
	
	if (url1.IsEquivalentTo(url2))
	{
		if (!svn.PegDiff(url1, SVNRev(m_SelectedEntry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry2->revision), 
			CTSVNPath(), svn_depth_infinity, TRUE, FALSE, FALSE, CString(), tempfile))
		{
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			theApp.DoWaitCursor(-1);
			return CTSVNPath();
		}
	}
	else
	{
		if (!svn.Diff(url1, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry1->revision), 
			url2, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry2->revision), 
			CTSVNPath(), svn_depth_infinity, TRUE, FALSE, FALSE, CString(), false, tempfile))
		{
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);		
			theApp.DoWaitCursor(-1);
			return CTSVNPath();
		}
	}
	theApp.DoWaitCursor(-1);
	return tempfile;
}

void CRevisionGraphWnd::DoZoom(float fZoomFactor)
{
	float oldzoom = m_fZoomFactor;
	m_fZoomFactor = fZoomFactor;
	m_node_space_left = NODE_SPACE_LEFT * fZoomFactor;
	m_node_space_right = NODE_SPACE_RIGHT * fZoomFactor;
	m_node_space_line = NODE_SPACE_LINE * fZoomFactor;
	m_node_rect_height = NODE_RECT_HEIGHT * fZoomFactor;
	m_node_space_top = NODE_SPACE_TOP * fZoomFactor;
	m_node_space_bottom = NODE_SPACE_BOTTOM * fZoomFactor;
	m_nFontSize = max(7, int(12.0f * fZoomFactor));
	m_RoundRectPt.x = int(ROUND_RECT * fZoomFactor);
	m_RoundRectPt.y = int(ROUND_RECT * fZoomFactor);
	m_nIconSize = int(32 * fZoomFactor);
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	SCROLLINFO si1 = {0};
	si1.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_VERT, &si1);
	SCROLLINFO si2 = {0};
	si2.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_HORZ, &si2);
	InitView();
	si1.nPos = int(float(si1.nPos)*m_fZoomFactor/oldzoom);
	si2.nPos = int(float(si2.nPos)*m_fZoomFactor/oldzoom);
	SetScrollPos(SB_VERT, si1.nPos);
	SetScrollPos(SB_HORZ, si2.nPos);
	Invalidate();
}












