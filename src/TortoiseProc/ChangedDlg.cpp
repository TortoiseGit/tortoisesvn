// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "ChangedDlg.h"
#include "messagebox.h"
#include "cursor.h"
#include ".\changeddlg.h"


IMPLEMENT_DYNAMIC(CChangedDlg, CResizableDialog)
CChangedDlg::CChangedDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CChangedDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CChangedDlg::~CChangedDlg()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
}

void CChangedDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHANGEDLIST, m_FileListCtrl);
}


BEGIN_MESSAGE_MAP(CChangedDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_RCLICK, IDC_CHANGEDLIST, OnNMRclickChangedlist)
END_MESSAGE_MAP()


void CChangedDlg::OnPaint() 
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
	}
	else
	{
		CResizableDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CChangedDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CChangedDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_FileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT );

	m_FileListCtrl.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_FileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_FileListCtrl.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_CHSTAT_FILECOL);
	m_FileListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_CHSTAT_WCCOL);
	m_FileListCtrl.InsertColumn(1, temp);
	temp.LoadString(IDS_CHSTAT_REPOCOL);
	m_FileListCtrl.InsertColumn(2, temp);

	m_FileListCtrl.SetRedraw(FALSE);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_FileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_FileListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_FileListCtrl.SetRedraw(TRUE);

	//first start a thread to obtain the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &ChangedStatusThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK);
	}
	GetDlgItem(IDC_CHANGEDLIST)->UpdateData(FALSE);


	AddAnchor(IDC_CHANGEDLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	this->hWnd = this->m_hWnd;
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChangedDlg::AddEntry(CString file, svn_wc_status_t * status)
{
	int index = m_FileListCtrl.GetItemCount();

	svn_wc_status_kind text = svn_wc_status_none;
	if (status->text_status != svn_wc_status_ignored)
		text = status->text_status;
	if (status->prop_status != svn_wc_status_ignored)
		text = text > status->prop_status ? text : status->prop_status;

	svn_wc_status_kind repo = svn_wc_status_none;
	if (status->repos_text_status != svn_wc_status_ignored)
		repo = status->repos_text_status;
	if (status->repos_prop_status != svn_wc_status_ignored)
		repo = repo > status->repos_prop_status ? repo : status->repos_prop_status;

	if ((text > svn_wc_status_normal)||(repo > svn_wc_status_normal))
	{
		TCHAR buf[MAX_PATH];
		m_FileListCtrl.InsertItem(index, file);
		SVNStatus::GetStatusString(text, buf);
		//GetStatusString(theApp.m_hInstance, text, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
		m_FileListCtrl.SetItemText(index, 1, buf);
		SVNStatus::GetStatusString(repo, buf);
		//GetStatusString(theApp.m_hInstance, repo, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
		m_FileListCtrl.SetItemText(index, 2, buf);

		m_FileListCtrl.SetRedraw(FALSE);
		int mincol = 0;
		int maxcol = ((CHeaderCtrl*)(m_FileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
		int col;
		for (col = mincol; col <= maxcol; col++)
		{
			m_FileListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
		}
		m_FileListCtrl.SetRedraw(TRUE);
		m_arWCStatus.Add(text);
		m_arRepoStatus.Add(repo);
		m_arPaths.Add(file);
		if (status->entry)
			m_arRevisions.Add(status->entry->revision);
		else
			m_arRevisions.Add(0);
	}
}

DWORD WINAPI ChangedStatusThread(LPVOID pVoid)
{
	svn_wc_status_t * status;
	const TCHAR * file = NULL;
	CChangedDlg * pDlg;
	pDlg = (CChangedDlg *)pVoid;
	pDlg->m_arPaths.RemoveAll();
	pDlg->m_arRepoStatus.RemoveAll();
	pDlg->m_arRevisions.RemoveAll();
	pDlg->m_arWCStatus.RemoveAll();
	pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	
	status = pDlg->m_svnstatus.GetFirstFileStatus(pDlg->m_path, &file, true);
	if (status)
	{
		pDlg->AddEntry(file, status);
		while (status = pDlg->m_svnstatus.GetNextFileStatus(&file))
		{
			pDlg->AddEntry(CString(file), status);
		}
	} // if (status)
	else
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->m_svnstatus.GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
	}
	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	return 0;
}

void CChangedDlg::OnNMRclickChangedlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int selIndex = m_FileListCtrl.GetSelectionMark();
	if (selIndex >= 0)
	{
		CString filepath = m_arPaths.GetAt(selIndex);
		svn_wc_status_kind wcStatus = (svn_wc_status_kind)m_arWCStatus.GetAt(selIndex);
		svn_wc_status_kind repoStatus = (svn_wc_status_kind)m_arRepoStatus.GetAt(selIndex);
		svn_revnum_t revision = m_arRevisions.GetAt(selIndex);
		//entry is selected, now show the popup menu
		CMenu popup;
		POINT point;
		GetCursorPos(&point);
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if (m_FileListCtrl.GetSelectedCount() == 1)
			{
				if ((repoStatus > svn_wc_status_normal)||(wcStatus > svn_wc_status_normal))
				{
					temp.LoadString(IDS_LOG_POPUP_COMPARE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
					temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
					if (repoStatus > svn_wc_status_normal)
					{
						temp.LoadString(IDS_SVNACTION_UPDATE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);
					} // if (repoStatus > svn_wc_status_normal)
					if (wcStatus > svn_wc_status_normal)
					{
						temp.LoadString(IDS_MENUREVERT);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERT, temp);
					}
				}
			} // if (m_FileListCtrl.GetSelectedCount() == 1)
			else
			{
				if (repoStatus > svn_wc_status_normal)
				{
					temp.LoadString(IDS_LOG_POPUP_UPDATE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);
				} // if (repoStatus > svn_wc_status_normal)

			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			this->m_app = &theApp;
			theApp.DoWaitCursor(1);
			switch (cmd)
			{
			case ID_COMPARE:
				{
					//user clicked on the menu item "compare with working copy"
					CString tempfile;
					if (repoStatus <= svn_wc_status_normal)
					{
						tempfile = SVN::GetPristinePath(filepath);

						//TODO:
						//as soon as issue 1361 of subversion 
						//http://subversion.tigris.org/issues/show_bug.cgi?id=1361
						//uncomment the lines below and delete the 
						//line above. This will then allow diff-viewers which
						//don't ignore different line endings to work correctly

						//CString tempfile = CUtils::GetTempFile();
						//SVN svn;
						//if (!svn.Cat(filepath, SVN::REV_BASE, tempfile))
						//{
						//	tempfile = SVN::GetPristinePath(filepath);
						//}
						//else
						//{
						//	m_templist.Add(filepath);
						//}
					}
					else
					{
						//next step is to create a temporary file to hold the required revision
						tempfile = CUtils::GetTempFile();

						SVN svn;
						if (!svn.Cat(filepath, SVN::REV_HEAD, tempfile))
						{
							CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						} // if (!svn.Cat(filepath, SVN::REV_HEAD, tempfile))
						m_templist.Add(tempfile);
					}
					if (repoStatus <= svn_wc_status_normal)
					{
						//exchange filepath and tempfile so always the oldest version is the first param to the diff program
						CString t = filepath;
						filepath = tempfile;
						tempfile = t;
					}
					CString diffpath = CUtils::GetDiffPath();
					if (diffpath != "")
					{

						CString cmdline;
						cmdline = _T("\"")+diffpath; //ensure the diff exe is prepend the commandline
						cmdline += _T("\" ");
						cmdline += _T(" \"") + filepath;
						cmdline += _T("\" "); 
						cmdline += _T(" \"") + tempfile;
						cmdline += _T("\"");
						STARTUPINFO startup;
						PROCESS_INFORMATION process;
						memset(&startup, 0, sizeof(startup));
						startup.cb = sizeof(startup);
						memset(&process, 0, sizeof(process));
						if (CreateProcess(NULL /*(LPCTSTR)diffpath*/, const_cast<TCHAR*>((LPCTSTR)cmdline), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
						{
							LPVOID lpMsgBuf;
							FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
								FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL 
								);
							CString temp;
							//temp.Format("could not start external diff program!\n<hr=100%>\n%s", lpMsgBuf);
							temp.Format(IDS_ERR_EXTDIFFSTART, lpMsgBuf);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
							LocalFree( lpMsgBuf );
						} // if (CreateProcess(diffpath, cmdline, NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
						//wait for the process to end
						WaitForSingleObject(process.hProcess, INFINITE);
						//now delete the temporary file
						if (repoStatus > svn_wc_status_normal)
							DeleteFile(tempfile);
					} // if (diffpath != "")
					theApp.DoWaitCursor(-1);
					GetDlgItem(IDOK)->EnableWindow(TRUE);
				}
				break;
			case ID_GNUDIFF1:
				{
					CString tempfile = CUtils::GetTempFile();
					tempfile += _T(".diff");
					if (!Diff(filepath, SVN::REV_WC, filepath, SVN::REV_HEAD, TRUE, FALSE, TRUE, _T(""), tempfile))
					{
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						DeleteFile(tempfile);
						break;		//exit
					} // if (!Diff(filepath, SVN::REV_WC, filepath, SVN::REV_HEAD, TRUE, FALSE, TRUE, _T(""), tempfile))
					m_templist.Add(tempfile);
					CUtils::StartDiffViewer(tempfile);
					theApp.DoWaitCursor(-1);
					GetDlgItem(IDOK)->EnableWindow(TRUE);
				}
				break;
			case ID_UPDATE:
				{
					CString tempFile = CUtils::GetTempFile();
					m_templist.Add(tempFile);
					HANDLE file = ::CreateFile (tempFile,
												GENERIC_WRITE, 
												FILE_SHARE_READ, 
												0, 
												CREATE_ALWAYS, 
												FILE_ATTRIBUTE_TEMPORARY,
												0);
					POSITION pos = m_FileListCtrl.GetFirstSelectedItemPosition();
					int index;
					while ((index = m_FileListCtrl.GetNextSelectedItem(pos)) >= 0)
					{
						DWORD written = 0;
						::WriteFile (file, m_arPaths.GetAt(index), m_arPaths.GetAt(index).GetLength()*sizeof(TCHAR), &written, 0);
						::WriteFile (file, _T("\n"), 2, &written, 0);
					} // while ((index = m_FileListCtrl.GetNextSelectedItem(&pos)) >= 0)
					::CloseHandle(file);
					CSVNProgressDlg dlg;
					dlg.SetParams(Enum_Update, true, tempFile);
					dlg.DoModal();
					m_FileListCtrl.DeleteAllItems();
					theApp.DoWaitCursor(-1);
					GetDlgItem(IDOK)->EnableWindow(TRUE);
					DWORD dwThreadId;
					if ((m_hThread = CreateThread(NULL, 0, &ChangedStatusThread, this, 0, &dwThreadId))==0)
					{
						CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK);
					}
				}
				break;
			case ID_REVERT:
				{
					if (CMessageBox::Show(this->m_hWnd, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO)==IDYES)
					{
						SVN svn;
						if (!svn.Revert(filepath, FALSE))
						{
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
					} // if (CMessageBox::Show(this->m_hWnd, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO)==IDYES) 
				} 
				break;
			default:
				GetDlgItem(IDOK)->EnableWindow(TRUE);
				break;
			} // switch (cmd)
			GetDlgItem(IDOK)->EnableWindow(TRUE);
			theApp.DoWaitCursor(-1);
		} // if (popup.CreatePopupMenu())
	} // if (selIndex >= 0)

}
