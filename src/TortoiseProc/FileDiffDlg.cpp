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
#include "UnicodeUtils.h"
#include "MessageBox.h"
#include "Utils.h"
#include "TempFile.h"
#include "ProgressDlg.h"
#include ".\filediffdlg.h"

#define ID_COMPARE 1
#define ID_BLAME 2

// CFileDiffDlg dialog

IMPLEMENT_DYNAMIC(CFileDiffDlg, CResizableStandAloneDialog)
CFileDiffDlg::CFileDiffDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CFileDiffDlg::IDD, pParent),
	m_bBlame(false)
{
}

CFileDiffDlg::~CFileDiffDlg()
{
}

void CFileDiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_cFileList);
}


BEGIN_MESSAGE_MAP(CFileDiffDlg, CResizableStandAloneDialog)
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILELIST, OnNMCustomdrawFilelist)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


// CFileDiffDlg message handlers

BOOL CFileDiffDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_cFileList.DeleteAllItems();
	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
	m_cFileList.SetExtendedStyle(exStyle);

	int c = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cFileList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_FILEDIFF_FILE);
	m_cFileList.InsertColumn(0, temp);
	temp.LoadString(IDS_FILEDIFF_COMMENT);
	m_cFileList.InsertColumn(1, temp);

	m_cFileList.SetRedraw(false);
	for (INT_PTR i=0; i<m_arFileList.GetCount(); ++i)
	{
		FileDiff diff = m_arFileList.GetAt(i);
		if (diff.relative1.IsEmpty())
		{
			m_cFileList.InsertItem(i, diff.url1);
		}
		else
		{
			m_cFileList.InsertItem(i, diff.relative1);
		}
		if (!diff.rev1.IsValid())
		{
			if (diff.middle2.IsEmpty())
				temp.Format(IDS_FILEDIFF_COMMENTCOPY, diff.url2);
			else
				temp.Format(IDS_FILEDIFF_COMMENTCOPY, diff.middle2);
			m_cFileList.SetItemText(i, 1, temp);
		}
		if (!diff.rev2.IsValid())
		{
			if (diff.middle1.IsEmpty())
				temp.Format(IDS_FILEDIFF_COMMENTCOPY, diff.url1);
			else
				temp.Format(IDS_FILEDIFF_COMMENTCOPY, diff.middle1);
			m_cFileList.SetItemText(i, 1, temp);
		}
	}

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cFileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	
	m_cFileList.SetRedraw(true);
	
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

bool CFileDiffDlg::SetUnifiedDiff(const CTSVNPath& diffFile, const CString& sRepoRoot)
{
	bool bRet = true;
	m_sRepoRoot = sRepoRoot;
	try
	{
		CString strLine;
		CStdioFile file(diffFile.GetWinPath(), CFile::typeText | CFile::modeRead);

		while (file.ReadString(strLine))
		{
			if (strLine.Left(3).Compare(_T("---"))==0)
			{
				strLine = strLine.Mid(4);
				SVNRev rev1, rev2;
				CString sAddedLine;
				file.ReadString(sAddedLine);
				if (sAddedLine.Left(3).Compare(_T("+++"))!=0)
				{
					bRet = false;
					break;
				}
				sAddedLine = sAddedLine.Mid(4);
				CString revone, revtwo;
				revone = strLine.Mid(strLine.ReverseFind('('));
				strLine = strLine.Left(strLine.ReverseFind('('));
				revtwo = sAddedLine.Mid(sAddedLine.ReverseFind('('));
				sAddedLine = sAddedLine.Left(sAddedLine.ReverseFind('('));
				strLine.Trim();
				sAddedLine.Trim();
				revone.Trim(_T(" ()"));
				revtwo.Trim(_T(" ()"));
				// now remove the 'revision'
				revone = revone.Mid(9);
				revtwo = revtwo.Mid(9);
				LONG r1 = _ttol(revone);
				if (r1 == 0)
					rev1 = SVNRev::REV_WC;
				else
					rev1 = r1;
				LONG r2 = _ttol(revtwo);
				if (r2 == 0)
					rev2 = SVNRev::REV_WC;
				else
					rev2 = r2;

				// sometimes the paths here still have a '(.../path/path)
				// appended. This is when the diff isn't done from the
				// repository root.
				CString middle1, middle2, relative1, relative2;
				if (strLine.Find(_T("(...")) >= 0)
				{
					middle1 = strLine.Mid(strLine.Find(_T("(..."))+4).TrimRight(_T(" )"));
					relative1 = strLine.Left(strLine.Find(_T("(..."))-1);
					strLine = middle1 + _T("/") + relative1;
				}
				if (sAddedLine.Find(_T("(...")) >= 0)
				{
					middle2 = sAddedLine.Mid(sAddedLine.Find(_T("(..."))+4).TrimRight(_T(" )"));
					relative2 = sAddedLine.Left(sAddedLine.Find(_T("(..."))-1);
					sAddedLine = middle2 + _T("/") + relative2;
				}
				if (!m_sRepoRoot.IsEmpty())
				{
					if (!SVN::PathIsURL(strLine))
					{
						if (strLine[0] == '/')
							strLine = m_sRepoRoot + strLine;
						else
							strLine = m_sRepoRoot + _T("/") + strLine;
					}
					if (!SVN::PathIsURL(sAddedLine))
					{
						if (sAddedLine[0] == '/')
							sAddedLine = m_sRepoRoot + sAddedLine;
						else
							sAddedLine = m_sRepoRoot + _T("/") + sAddedLine;
					}
				}
				if ((rev1.IsWorking())&&(SVN::PathIsURL(strLine)))
					rev1 = SVNRev();
				if ((rev2.IsWorking())&&(SVN::PathIsURL(sAddedLine)))
					rev2 = SVNRev();
				CFileDiffDlg::FileDiff fd;
				fd.rev1 = rev1;
				fd.rev2 = rev2;
				fd.url1 = strLine;
				fd.middle1 = middle1;
				fd.relative1 = relative1;
				fd.url2 = sAddedLine;
				fd.middle2 = middle2;
				fd.relative2 = relative2;
				m_arFileList.Add(fd);
			}
		}
		file.Close();
	}
	catch (CFileException* /*pE*/)
	{
		bRet = false;
	}
	return bRet;
}

void CFileDiffDlg::DoDiff(int selIndex, bool blame)
{
	CFileDiffDlg::FileDiff fd = m_arFileList.GetAt(selIndex);

	CTSVNPath url1 = CTSVNPath(fd.url1);
	CTSVNPath url2 = CTSVNPath(fd.url2);
	LONG rev1 = fd.rev1;
	LONG rev2 = fd.rev2;
	if (rev1 < 0)
		rev1 = 0;
	if (rev2 < 0)
		rev2 = 0;
	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, url1, rev1);
	CString sTemp;
	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_PROGRESSWAIT);
	progDlg.ShowModeless(this);
	progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)url1.GetUIPathString());
	progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, rev1);

	if ((rev1 > 0)&&(!blame)&&(!m_SVN.Cat(url1, SVNRev(rev1), rev1, tempfile)))
	{
		CMessageBox::Show(NULL, m_SVN.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	else if ((rev1 > 0)&&(blame)&&(!m_blamer.BlameToFile(url1, 1, rev1, rev1, tempfile)))
	{
		return;
	}
	SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	progDlg.SetProgress(1, 2);
	progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)url2.GetUIPathString());
	progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, rev2);
	CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(true, url2, rev2);
	if ((rev2 > 0)&&(!blame)&&(!m_SVN.Cat(url2, fd.rev2, fd.rev2, tempfile2)))
	{
		CMessageBox::Show(NULL, m_SVN.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	else if ((rev2 > 0)&&(blame)&&(!m_blamer.BlameToFile(url2, 1, fd.rev2, fd.rev2, tempfile2)))
	{
		return;
	}
	SetFileAttributes(tempfile2.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	progDlg.SetProgress(2,2);
	progDlg.Stop();

	CString rev1name, rev2name;
	if ((url1.GetFilename().CompareNoCase(url2.GetFilename())!=0)&&(rev1 == rev2))
	{
		rev1name.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFilename(), rev1);
		rev2name.Format(_T("%s Revision %ld"), (LPCTSTR)url2.GetFilename(), rev2);
	}
	else
	{
		// use the relative URL's of the files, since the filenames and revisions are the same.
		if ((fd.middle1.IsEmpty())||(fd.relative1.IsEmpty()))
		{
			rev1name.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFilename(), rev1);
			rev2name.Format(_T("%s Revision %ld"), (LPCTSTR)url2.GetFilename(), rev2);
		}
		else
		{
			rev1name.Format(_T("%s Revision %ld"), (LPCTSTR)(fd.middle1 +_T("/") + fd.relative1), rev1);
			rev2name.Format(_T("%s Revision %ld"), (LPCTSTR)(fd.middle2 + _T("/") + fd.relative2), rev2);
		}
	}
	CUtils::StartExtDiff(tempfile, tempfile2, rev1name, rev2name, FALSE, blame);
}

void CFileDiffDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int selIndex = pNMLV->iItem;
	if (selIndex < 0)
		return;
	if (selIndex >= m_arFileList.GetCount())
		return;	
	
	DoDiff(selIndex, m_bBlame);
}

void CFileDiffDlg::OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (pGetInfoTip->iItem >= m_arFileList.GetCount())
		return;
	
		if (pGetInfoTip->cchTextMax > m_arFileList.GetAt(pGetInfoTip->iItem).url1.GetLength())
			_tcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, m_arFileList.GetAt(pGetInfoTip->iItem).url1, pGetInfoTip->cchTextMax);
	*pResult = 0;
}

void CFileDiffDlg::OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

		if (m_arFileList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			FileDiff fd = m_arFileList.GetAt(pLVCD->nmcd.dwItemSpec);
			if ((fd.rev1 == 0)||(fd.rev2 == 0))
				crText = GetSysColor(COLOR_GRAYTEXT);
		}
		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
	}
}

void CFileDiffDlg::AddFile(FileDiff filediff)
{
	m_arFileList.Add(filediff);
}

void CFileDiffDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		CString temp;
		temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
		temp.LoadString(IDS_FILEDIFF_POPBLAME);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAME, temp);
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case ID_COMPARE:
			{
				POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
				while (pos)
				{
					int index = m_cFileList.GetNextSelectedItem(pos);
					DoDiff(index, false);
				}					
			}
			break;
		case ID_BLAME:
			{
				POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
				while (pos)
				{
					int index = m_cFileList.GetNextSelectedItem(pos);
					DoDiff(index, true);
				}					
			}
			break;
		}
	}
}
