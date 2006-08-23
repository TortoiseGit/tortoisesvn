// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
	m_arVertPositions.RemoveAll();
	m_targetsbottom.clear();
	m_targetsright.clear();
	m_GraphRect.SetRectEmpty();
	m_ViewRect.SetRectEmpty();
	GetViewSize();
	BuildConnections();
	SetScrollbars(0,0,m_ViewRect.Width(),m_ViewRect.Height());
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

INT_PTR CRevisionGraphWnd::GetIndexOfRevision(source_entry * sentry)
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		if (((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->revision == sentry->revisionto)
			if (IsParentOrItself(sentry->pathto, ((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->url))
				return i;
	}
	return -1;
}

CRevisionEntry * CRevisionGraphWnd::GetRevisionEntry(source_entry * sentry) const
{
	std::multimap<svn_revnum_t, CRevisionEntry*>::const_iterator it = m_mapEntryPtrs.lower_bound(sentry->revisionto);
	if (it != m_mapEntryPtrs.end())
	{
		while ((it != m_mapEntryPtrs.end())&&(it->first == sentry->revisionto))
		{
			if (IsParentOrItself(sentry->pathto, it->second->url))
				return it->second;
			++it;
		}
	}
	return NULL;
}
CRevisionEntry * CRevisionGraphWnd::GetRevisionEntry(LONG rev) const
{
	std::multimap<svn_revnum_t, CRevisionEntry*>::const_iterator it = m_mapEntryPtrs.find(rev);
	if (it != m_mapEntryPtrs.end())
	{
		return it->second;
	}
	return NULL;
}

void CRevisionGraphWnd::MarkSpaceLines(source_entry * entry, int level, svn_revnum_t startrev, svn_revnum_t endrev)
{
	int maxright = 0;
	int maxbottom = 0;
	std::set<CRevisionEntry*> rightset;
	std::set<CRevisionEntry*> bottomset;
	
	for (INT_PTR i=0; i < m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		bool incremented = false;
		if (reventry->level == level)
		{
			if ((reventry->revision >= startrev)&&(reventry->revision <= endrev))
			{
				reventry->rightlines++;
				incremented = true;
				maxright = max(reventry->rightlines, maxright);
				rightset.insert(reventry);
			}
			else if (reventry->revision < startrev)
			{
				for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
				{
					source_entry * sentry = (source_entry*)reventry->sourcearray[j];
					for (std::set<CRevisionEntry*>::iterator it = rightset.begin(); it!= rightset.end(); ++it)
					{
						if ((sentry->revisionto >= (*it)->revision)&&(!incremented))
						{
							// before we add this, we have to check if it's a top->bottom line and not one
							// which goes to the right around another node
							bool nodesinbetween = true;
							if ((*it)->level == level)
							{
								nodesinbetween = false;

								std::multimap<svn_revnum_t, CRevisionEntry*>::const_iterator it2 = m_mapEntryPtrs.lower_bound(reventry->revision);
								if (it2 != m_mapEntryPtrs.end())
									++it2;
								while ((it2 != m_mapEntryPtrs.end())&&(it2->second->revision < sentry->revisionto))
								{
									if (it2->second->level == reventry->level)
									{
										nodesinbetween = true;
										break;
									}
									++it2;
								}
							}
							if (nodesinbetween)
							{
								reventry->rightlines++;
								incremented = true;
								maxright = max(reventry->rightlines, maxright);
								rightset.insert(reventry);
							}
						}
					}
				}
			}
		}
		if (reventry->revision == endrev)
		{
			if (reventry->level > level)
			{
				reventry->bottomlines++;
				maxbottom = max(reventry->bottomlines, maxbottom);
				bottomset.insert(reventry);
			}
		}
	}
	
	for (std::set<CRevisionEntry*>::iterator it=rightset.begin(); it!=rightset.end(); ++it)
	{
		m_targetsright.insert(std::pair<source_entry*, CRevisionEntry*>(entry, (*it)));
	}
	for (std::set<CRevisionEntry*>::iterator it=bottomset.begin(); it!=bottomset.end(); ++it)
	{
		m_targetsbottom.insert(std::pair<source_entry*, CRevisionEntry*>(entry, (*it)));
	}

	std::multimap<source_entry*, CRevisionEntry*>::iterator beginright = m_targetsright.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endright = m_targetsright.upper_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator beginbottom = m_targetsbottom.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endbottom = m_targetsbottom.upper_bound(entry);

	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginright; it!=endright; ++it)
	{
		(it->second)->rightlines = maxright;
	}

	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginbottom; it!=endbottom; ++it)
	{
		(it->second)->bottomlines = maxbottom;
	}
}

void CRevisionGraphWnd::DecrementSpaceLines(source_entry * entry)
{
	
	std::multimap<source_entry*, CRevisionEntry*>::iterator beginright = m_targetsright.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endright = m_targetsright.upper_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator beginbottom = m_targetsbottom.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endbottom = m_targetsbottom.upper_bound(entry);
	
	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginright; it!=endright; ++it)
	{
		(it->second)->rightlinesleft--;
	}
	
	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginbottom; it!=endbottom; ++it)
	{
		(it->second)->bottomlinesleft--;
	}
}

void CRevisionGraphWnd::ClearEntryConnections()
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		reventry->bottomconnections = 0;
		reventry->leftconnections = 0;
		reventry->rightconnections = 0;
		reventry->bottomlines = 0;
		reventry->rightlines = 0;
	}
	m_targetsright.clear();
	m_targetsbottom.clear();
}

void CRevisionGraphWnd::CountEntryConnections()
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		reventry->leftconnections += reventry->sourcearray.GetCount();
		for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
		{
			source_entry * sentry = (source_entry*)reventry->sourcearray[j];
			CRevisionEntry * reventryto = GetRevisionEntry(sentry);
			if (reventryto == NULL)
				continue;
			if (reventry->level == reventryto->level)
			{
				// if there are entries in between, then the connection
				// is right-up-left
				// without entries in between, the connection is straight up
				bool nodesinbetween = false;

				std::multimap<svn_revnum_t, CRevisionEntry*>::const_iterator it = m_mapEntryPtrs.lower_bound(reventry->revision);
				if (it != m_mapEntryPtrs.end())
					++it;
				while ((it != m_mapEntryPtrs.end())&&(it->first < reventryto->revision))
				{
					if (it->second->level == reventry->level)
					{
						nodesinbetween = true;
						break;
					}
					++it;
				}
				if (nodesinbetween)
				{
					reventryto->rightconnections++;
					reventry->rightconnections++;
					MarkSpaceLines(sentry, reventry->level, reventry->revision, reventryto->revision);
				}
				else
				{
					reventryto->bottomconnections++;
				}
			}
			else
			{
				if (reventry->level < reventryto->level)
				{
					reventry->rightconnections++;
					reventryto->bottomconnections++;
				}
				else
				{
					reventry->leftconnections++;
					reventryto->bottomconnections++;
				}
				MarkSpaceLines(sentry, reventry->level, reventry->revision, reventryto->revision);
			}
		}
	}
}

void CRevisionGraphWnd::BuildConnections()
{
	// create an array which holds the vertical position of each
	// revision entry. Since there can be several entries in the
	// same revision, this speeds up the search for the right
	// position for drawing.
	m_arVertPositions.RemoveAll();
	svn_revnum_t vprev = 0;
	int currentvpos = 0;
	for (INT_PTR vp = 0; vp < m_arEntryPtrs.GetCount(); ++vp)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(vp);
		if (reventry->revision != vprev)
		{
			vprev = reventry->revision;
			currentvpos++;
		}
		m_arVertPositions.Add(currentvpos-1);
	}
	// delete all entries which we might have left
	// in the array and free the memory they use.
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	
	ClearEntryConnections();
	CountEntryConnections();
	
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		reventry->bottomconnectionsleft = reventry->bottomconnections;
		reventry->leftconnectionsleft = reventry->leftconnections;
		reventry->rightconnectionsleft = reventry->rightconnections;
		reventry->bottomlinesleft = reventry->bottomlines;
		reventry->rightlinesleft = reventry->rightlines;
	}

	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		int vertpos = m_arVertPositions[i];
		for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
		{
			source_entry * sentry = (source_entry*)reventry->sourcearray.GetAt(j);
			CRevisionEntry * reventry2 = GetRevisionEntry(sentry);
			INT_PTR index = GetIndexOfRevision(sentry);
			if (reventry2 == NULL)
				continue;
			
			// we always draw from bottom to top!			
			CPoint * pt = new CPoint[5];
			if (reventry->level < reventry2->level)
			{
				if (reventry->revision < reventry2->revision)
				{
					//       5
					//       |
					//    3--4
					//    |
					// 1--2
					
					// x-offset for line 2-3
					int xoffset = (reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/(reventry->rightlines+1);
					// y-offset for line 3-4
					int yoffset = (reventry2->bottomlinesleft)*(m_node_space_top+m_node_space_bottom)/(reventry2->bottomlines+1);
					
					//Starting point: 1
					pt[0].y = vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top;	// top of rect
					pt[0].y += (reventry->rightconnectionsleft)*(m_node_rect_heigth)/(reventry->rightconnections+1);
					reventry->rightconnectionsleft--;
					pt[0].x = (reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width;
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + xoffset;
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = ((m_arVertPositions[index])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom)) + m_node_rect_heigth + m_node_space_top;
					pt[2].y += yoffset;
					//line to middle of target rect: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((reventry2->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left + m_node_rect_width/2;
					//line up to target rect: 5
					pt[4].y = (m_arVertPositions[index]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth) + m_node_space_top;
					pt[4].x = pt[3].x;
				}
				else
				{
					// since we should *never* draw a connection from a higher to a lower
					// revision, assert!
					ATLASSERT(false);
				}
			}
			else if (reventry->level > reventry2->level)
			{
				if (reventry->revision < reventry2->revision)
				{
					// 5
					// |
					// 4----3
					//      |
					//      |
					//      2-----1

					// x-offset for line 2-3
					int xoffset = (reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/(reventry->rightlines+1);
					// y-offset for line 3-4
					int yoffset = (reventry2->bottomlinesleft)*(m_node_space_top+m_node_space_bottom)/(reventry2->bottomlines+1);

					//Starting point: 1
					pt[0].y = (vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += (reventry->leftconnectionsleft)*(m_node_rect_heigth)/(reventry->leftconnections+1);
					reventry->leftconnectionsleft--;
					pt[0].x = ((reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x - xoffset;
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = (m_arVertPositions[index]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth + m_node_space_top + m_node_space_bottom);
					pt[2].y += yoffset;
					//line to middle of target rect: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((reventry2->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left + m_node_rect_width/2;
					//line up to target rect: 5
					pt[4].x = pt[3].x;
					pt[4].y = (m_arVertPositions[index]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth + m_node_space_top);
				}
				else
				{
					// since we should *never* draw a connection from a higher to a lower
					// revision, assert!
					ATLASSERT(false);
				}
			}
			else
			{
				// same level!
				// check first if there are other nodes in between the two connected ones
				BOOL nodesinbetween = FALSE;
				LONG startrev = min(reventry->revision, reventry2->revision);
				LONG endrev = max(reventry->revision, reventry2->revision);
				for (LONG k=startrev+1; k<endrev; ++k)
				{
					CRevisionEntry * samereventry = GetRevisionEntry(k);
					if ((samereventry)&&(samereventry->level == reventry2->level))
					{
						nodesinbetween = TRUE;
						break;
					}
				}
				if (nodesinbetween)
				{
					// 4----3
					//      |
					//      |
					//      |
					// 1----2

					// x-offset for line 2-3
					int xoffset = (reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/(reventry->rightlines+1);
					// y-offset for line 3-4
					int yoffset = (reventry2->rightconnectionsleft)*(m_node_rect_heigth)/(reventry2->rightconnections+1);
					reventry2->rightconnectionsleft--;
										
					//Starting point: 1
					pt[0].y = (vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += (reventry->rightconnectionsleft)*(m_node_rect_heigth)/(reventry->rightconnections+1);
					reventry->rightconnectionsleft--;
					pt[0].x = ((reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width;
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + xoffset;
					//line down: 3
					pt[2].x = pt[1].x;
					pt[2].y = (m_arVertPositions[index]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += yoffset;
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width;
					pt[4].y = pt[3].y;
					pt[4].x = pt[3].x;
				}
				else
				{
					if (reventry->revision < reventry2->revision)
					{
						// 2
						// |
						// |
						// 1
						pt[0].y = (vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
						pt[0].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width/2);
						pt[1].y = pt[0].y;
						pt[1].x = pt[0].x;
						pt[2].y = (m_arVertPositions[index]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top + m_node_rect_heigth);
						pt[2].x = pt[0].x;
						pt[3].y = pt[2].y;
						pt[3].x = pt[2].x;
						pt[4].y = pt[3].y;
						pt[4].x = pt[3].x;
					}
					else
					{
						ATLASSERT(false);
					}
				}
			}
			INT_PTR conindex = m_arConnections.Add(pt);

			// we add the connection index to each revision entry which the connection
			// passes by vertically. We use this to reduce the time to draw the connections,
			// because we know which nodes are in the visible area but not which connections.
			// By doing this, we can simply get the connections we have to draw from
			// the nodes we draw.
			for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(reventry->revision); it != m_mapEntryPtrs.upper_bound(reventry2->revision); ++it)
			{
				it->second->connections.insert(conindex);
			}
			DecrementSpaceLines(sentry);
		}
	}
}

CRect * CRevisionGraphWnd::GetGraphSize()
{
	if (m_GraphRect.Height() != 0)
		return &m_GraphRect;
	m_GraphRect.top = 0;
	m_GraphRect.left = 0;
	int lastrev = -1;
	if ((m_maxlevel == 0) || (m_numRevisions == 0) || (m_maxurllength == 0) || m_maxurl.IsEmpty())
	{
		for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
		{
			CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
			if (m_maxlevel < reventry->level)
				m_maxlevel = reventry->level;
			if (lastrev != reventry->revision)
			{
				m_numRevisions++;
				lastrev = reventry->revision;
			}
			size_t len = strlen(reventry->url);
			if (m_maxurllength < len)
			{
				m_maxurllength = len;
				m_maxurl = CUnicodeUtils::GetUnicode(reventry->url);
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
		m_node_rect_width = min(int(500 * m_fZoomFactor), r.Width()+40);
		m_node_rect_width = max(int(NODE_RECT_WIDTH * m_fZoomFactor), m_node_rect_width);
		pDC->SelectObject(pOldFont);
	}
	ReleaseDC(pDC);

	m_GraphRect.right = m_maxlevel * (m_node_rect_width + m_node_space_left + m_node_space_right);
	m_GraphRect.bottom = m_numRevisions * (m_node_rect_heigth + m_node_space_top + m_node_space_bottom);
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

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry1->url));
	url2.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry2->url));

	SVNRev peg = (SVNRev)(bHead ? m_SelectedEntry1->revision : SVNRev());

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowCompare(url1, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry1->revision),
		url2, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry2->revision),
		peg);
}

void CRevisionGraphWnd::UnifiedDiffRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry1->url));
	url2.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry2->url));

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
	
	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry1->url));
	url2.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry2->url));
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
			TRUE, TRUE, FALSE, FALSE, CString(), tempfile))
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
			TRUE, TRUE, FALSE, FALSE, CString(), false, tempfile))
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
	m_fZoomFactor = fZoomFactor;
	m_node_space_left = int(NODE_SPACE_LEFT * fZoomFactor);
	m_node_space_right = max(int(NODE_SPACE_RIGHT * fZoomFactor),1);
	m_node_space_line = int(NODE_SPACE_LINE * fZoomFactor);
	m_node_rect_heigth = max(int(NODE_RECT_HEIGTH * fZoomFactor),1);
	m_node_space_top = max(int(NODE_SPACE_TOP * fZoomFactor),1);
	m_node_space_bottom = max(int(NODE_SPACE_BOTTOM * fZoomFactor),1);
	m_nFontSize = int(12 * fZoomFactor);
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
	InitView();
	Invalidate();
}












