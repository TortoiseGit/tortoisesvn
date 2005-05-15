// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "ProgressDlg.h"
#include ".\filediffdlg.h"


// CFileDiffDlg dialog

IMPLEMENT_DYNAMIC(CFileDiffDlg, CResizableStandAloneDialog)
CFileDiffDlg::CFileDiffDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CFileDiffDlg::IDD, pParent)
{
}

CFileDiffDlg::~CFileDiffDlg()
{
	m_tempFileList.DeleteAllFiles();
}

void CFileDiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_cFileList);
}


BEGIN_MESSAGE_MAP(CFileDiffDlg, CResizableStandAloneDialog)
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
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
	temp.LoadString(IDS_LOGPROMPT_FILE);
	m_cFileList.InsertColumn(0, temp);

	m_cFileList.SetRedraw(false);
	for (INT_PTR i=0; i<m_arFileList.GetCount(); ++i)
	{
		m_cFileList.InsertItem(i, m_arFileList.GetAt(i).url1);
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

bool CFileDiffDlg::SetUnifiedDiff(const CTSVNPath& diffFile)
{
	bool bRet = true;
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
				CFileDiffDlg::FileDiff fd;
				fd.rev1 = rev1;
				fd.rev2 = rev2;
				fd.url1 = strLine;
				fd.url2 = sAddedLine;
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
void CFileDiffDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int selIndex = pNMLV->iItem;
	if (selIndex <= 0)
		return;
	if (selIndex >= m_arFileList.GetCount())
		return;	
	
	CFileDiffDlg::FileDiff fd = m_arFileList.GetAt(selIndex);
	
	CTSVNPath url1 = CTSVNPath(fd.url1);
	CTSVNPath url2 = CTSVNPath(fd.url2);
	CTSVNPath tempfile = CUtils::GetTempFilePath(url1);
	m_tempFileList.AddPath(tempfile);
	CString sTemp;
	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_PROGRESSWAIT);
	progDlg.ShowModeless(this);
	progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)url1.GetUIPathString());
	progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, (LONG)fd.rev1);
	
	if (!m_SVN.Cat(url1, fd.rev1, tempfile))
	{
		CMessageBox::Show(NULL, m_SVN.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	} 
	progDlg.SetProgress(1, 2);
	progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)url2.GetUIPathString());
	progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, (LONG)fd.rev2);
	CTSVNPath tempfile2 = CUtils::GetTempFilePath(url2);
	m_tempFileList.AddPath(tempfile2);
	if (!m_SVN.Cat(url2, fd.rev2, tempfile2))
	{
		CMessageBox::Show(NULL, m_SVN.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	} 
	progDlg.SetProgress(2,2);
	progDlg.Stop();

	CString rev1name, rev2name;
	rev1name.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFilename(), (LONG)fd.rev1);
	rev2name.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFilename(), (LONG)fd.rev2);
	CUtils::StartExtDiff(tempfile, tempfile2, rev1name, rev2name);

}

void CFileDiffDlg::OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (pGetInfoTip->iItem >= m_arFileList.GetCount())
		return;
	
		if (pGetInfoTip->cchTextMax > m_arFileList.GetAt(pGetInfoTip->iItem).url1.GetLength())
			_tcsncpy(pGetInfoTip->pszText, m_arFileList.GetAt(pGetInfoTip->iItem).url1, pGetInfoTip->cchTextMax);
	*pResult = 0;
}
