// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include ".\revertdlg.h"


// CRevertDlg dialog

IMPLEMENT_DYNAMIC(CRevertDlg, CResizableDialog)
CRevertDlg::CRevertDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CRevertDlg::IDD, pParent)
	, m_bSelectAll(TRUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CRevertDlg::~CRevertDlg()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
}

void CRevertDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REVERTLIST, m_RevertList);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
}


BEGIN_MESSAGE_MAP(CRevertDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_NOTIFY(NM_DBLCLK, IDC_REVERTLIST, OnNMDblclkRevertlist)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_REVERTLIST, OnLvnGetInfoTipRevertlist)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CRevertDlg message handlers
void CRevertDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	} // if (IsIconic()) 
	else
	{
		CResizableDialog::OnPaint();
	}
}
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRevertDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CRevertDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	//set the listcontrol to support checkboxes
	m_RevertList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	m_RevertList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_RevertList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_RevertList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOGPROMPT_FILE);
	m_RevertList.InsertColumn(0, temp);
	temp.LoadString(IDS_LOGPROMPT_STATUS);
	m_RevertList.InsertColumn(1, temp);

	m_RevertList.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_RevertList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_RevertList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((CreateThread(NULL, 0, &RevertThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bThreadRunning = TRUE;
	m_RevertList.UpdateData(FALSE);

	AddAnchor(IDC_REVERTLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(_T("RevertDlg"));
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

DWORD WINAPI RevertThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CRevertDlg*	pDlg;
	pDlg = (CRevertDlg*)pVoid;
	pDlg->GetDlgItem(IDOK)->EnableWindow(false);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(false);

	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));

	pDlg->m_RevertList.SetRedraw(false);
	try
	{
		CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
		CString strLine = _T("");
		const TCHAR * strbuf = NULL;
		TCHAR buf[MAX_PATH];
		while (file.ReadString(strLine))
		{
			SVNStatus status;
			svn_wc_status_t *s;

			// Determine whether the item the user selected (strLine) is a directory.

			BOOL bIsDir = PathIsDirectory(strLine);

			// The directory that contains this item.
			// Even if strLine is a directory, we will hold its root. 
			// Thus, we can distinguish sub-items with the same relative 
			// path in different selected directories.

			CString root = strLine.Left(strLine.ReverseFind('\\') + 1);

			// Ask SVN for the first item found for strLine.

			s = status.GetFirstFileStatus(strLine, &strbuf);
			while (s!=0)
			{
				// the current item (path)

				CString item = strbuf;

				// Get the "combined" status of item and its properties

				svn_wc_status_kind stat = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);

				if (stat == SVNStatus::GetMoreImportant(stat, svn_wc_status_added))
				{
					CString ponly;
					ponly.LoadString(IDS_STATUSLIST_PROPONLY);
					pDlg->m_arFileList.Add(item);
					int count = pDlg->m_RevertList.GetItemCount();
					pDlg->m_RevertList.InsertItem(count, item.Mid(root.GetLength()));
					SVNStatus::GetStatusString(AfxGetResourceHandle(), stat, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
					if ((stat == s->prop_status)&&(!SVNStatus::IsImportant(s->text_status)))
						_tcscat(buf, ponly);
					pDlg->m_RevertList.SetItemText(count, 1, buf);
					pDlg->m_RevertList.SetCheck(count);
				} 
				// Ask SVN for the next item for strLine. 
				// This will usually be the case for directories and svn:externals.

				s = status.GetNextFileStatus(&strbuf);
			} // if (s!=0) 
		} // while (file.ReadString(strLine)) 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Commit!\n");
		pE->Delete();
	}


	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(pDlg->m_RevertList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		pDlg->m_RevertList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	pDlg->m_RevertList.SetRedraw(true);

	pDlg->GetDlgItem(IDOK)->EnableWindow(true);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
	pDlg->m_bThreadRunning = FALSE;
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return 0;
}

void CRevertDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	//save only the files the user has selected into the temporary file
	try
	{
		CStdioFile file(m_sPath, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
		for (int i=0; i<m_RevertList.GetItemCount(); i++)
		{
			if (m_RevertList.GetCheck(i))
			{
				file.WriteString(m_arFileList.GetAt(i)+_T("\n"));
			}
		} 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Add!\n");
		pE->Delete();
	}


	CResizableDialog::OnOK();
}

void CRevertDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;

	CResizableDialog::OnCancel();
}

void CRevertDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CRevertDlg::OnBnClickedSelectall()
{
	UpdateData();
	theApp.DoWaitCursor(1);
	m_RevertList.SetRedraw(false);
	int itemCount = m_RevertList.GetItemCount();
	for (int i=0; i<itemCount; i++)
	{
		m_RevertList.SetCheck(i, m_bSelectAll);
	}
	m_RevertList.SetRedraw(true);
	theApp.DoWaitCursor(-1);
}

void CRevertDlg::OnNMDblclkRevertlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_bThreadRunning)
		return;

	StartDiff(pNMLV->iItem);
}

void CRevertDlg::StartDiff(int fileindex)
{
	if (fileindex < 0)
		return;
	CString path = m_arFileList.GetAt(fileindex);
	if (PathIsDirectory(path))
		return;		//we don't compare folders
	CString path1;
	CString path2 = SVN::GetPristinePath(path);
	if (path2.IsEmpty())
		return;

	if ((!CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase"), TRUE))&&(SVN::GetTranslatedFile(path1, path)))
	{
		m_templist.Add(path1);
	}
	else
	{
		path1 = path;
	}

	CString name = CUtils::GetFileNameFromPath(path);
	CString ext = CUtils::GetFileExtFromPath(path);
	CString n1, n2;
	n1.Format(IDS_DIFF_WCNAME, name);
	n2.Format(IDS_DIFF_BASENAME, name);
	CUtils::StartDiffViewer(path2, path1, FALSE, n2, n1, ext);
}

void CRevertDlg::OnLvnGetInfoTipRevertlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);

	if (pGetInfoTip->cchTextMax > m_arFileList.GetAt(pGetInfoTip->iItem).GetLength())
		_tcsncpy(pGetInfoTip->pszText, m_arFileList.GetAt(pGetInfoTip->iItem), pGetInfoTip->cchTextMax);

	*pResult = 0;
}

BOOL CRevertDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bThreadRunning)
	{
		if ((pWnd)&&(pWnd == GetDlgItem(IDC_REVERTLIST)))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableDialog::OnSetCursor(pWnd, nHitTest, message);
}
