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
#include "messagebox.h"
#include "cursor.h"
#include "LogDlg.h"

// CLogDlg dialog

IMPLEMENT_DYNAMIC(CLogDlg, CDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLogDlg::IDD, pParent)
	, m_sLogMsgCtrl(_T(""))
{
	m_bCancelled = FALSE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CLogDlg::~CLogDlg()
{
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGLIST, m_LogList);
	DDX_Text(pDX, IDC_LOGMSG, m_sLogMsgCtrl);
}


BEGIN_MESSAGE_MAP(CLogDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_LOGLIST, OnNMClickLoglist)
	ON_NOTIFY(NM_RCLICK, IDC_LOGLIST, OnNMRclickLoglist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, OnLvnKeydownLoglist)
END_MESSAGE_MAP()


BEGIN_RESIZER_MAP(CLogDlg)
    RESIZER(IDC_LOGLIST,RS_BORDER,RS_BORDER,RS_BORDER,IDC_LOGMSG,0)
	RESIZER(IDC_LOGMSG, RS_BORDER, IDC_LOGLIST, RS_BORDER, IDOK, 0)
	RESIZER(IDOK, RS_KEEPSIZE, RS_KEEPSIZE, RS_BORDER, RS_BORDER, 0)
END_RESIZER_MAP


void CLogDlg::SetParams(CString path, long startrev /* = 0 */, long endrev /* = -1 */)
{
	m_path = path;
	m_startrev = startrev;
	m_endrev = endrev;
}

void CLogDlg::OnPaint() 
{
	RESIZER_GRIP;
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
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLogDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CLogDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_LogList.SetExtendedStyle ( LVS_EX_FULLROWSELECT );

	m_LogList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_LogList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOG_REVISION);
	m_LogList.InsertColumn(0, temp);
	temp.LoadString(IDS_LOG_AUTHOR);
	m_LogList.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_DATE);
	m_LogList.InsertColumn(2, temp);
	m_arLogMessages.SetSize(0,100);
	m_arRevs.SetSize(0,100);

	m_LogList.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	//first start a thread to obtain the log messages without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &LogThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK);
	}
	//UpdateData(FALSE);
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;

	INIT_RESIZER;
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CLogDlg::Cancel()
{
	return m_bCancelled;
}

BOOL CLogDlg::Log(LONG rev, CString author, CString date, CString message, CString cpaths)
{
	CString temp;
	m_logcounter += 1;
	int count = m_LogList.GetItemCount();
	temp.Format(_T("%d"),rev);
	m_LogList.InsertItem(count, temp);
	m_LogList.SetItemText(count, 1, author);
	m_LogList.SetItemText(count, 2, date);

	//split multiline logentries and concatenate them
	//again but this time with \r\n as line separators
	//so that the edit control recognizes them
	temp = "";
	if (message.GetLength()>0)
	{
		int curPos= 0;
		CString resToken= message.Tokenize(_T("\n\r"),curPos);
		temp += resToken+_T("\r\n");
		resToken= message.Tokenize(_T("\n\r"),curPos);
		while (resToken != _T(""))
		{
			temp += resToken+_T("\r\n");
			resToken= message.Tokenize(_T("\n\r"),curPos);
		};
	} // if (message.GetLength()>0)
	temp += _T("\r\n---------------------------------\r\n");
	temp += cpaths;
	m_arLogMessages.Add(temp);
	m_arRevs.Add(rev);
	m_LogList.SetRedraw();
	return TRUE;

}

void CLogDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	UPDATE_RESIZER;
}

//this is the thread function which calls the subversion function
DWORD WINAPI LogThread(LPVOID pVoid)
{
	CLogDlg*	pDlg;
	pDlg = (CLogDlg*)pVoid;
	CString temp;
	temp.LoadString(IDS_MSGBOX_CANCEL);
	pDlg->GetDlgItem(IDOK)->SetWindowText(temp);

	if (!pDlg->ReceiveLog(pDlg->m_path, pDlg->m_startrev, pDlg->m_endrev, true))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}  
		
	pDlg->m_LogList.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(pDlg->m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		pDlg->m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	pDlg->m_LogList.SetRedraw(true);

	temp.LoadString(IDS_MSGBOX_OK);
	pDlg->GetDlgItem(IDOK)->SetWindowText(temp);
	pDlg->m_bCancelled = TRUE;
	return 0;
}

void CLogDlg::OnNMClickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
		UpdateData(FALSE);
	}
	else
	{
		m_sLogMsgCtrl = _T("");
		UpdateData(FALSE);
	}
	*pResult = 0;
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		//check which key was pressed
		//this is necessary because we get here BEFORE
		//the selection actually changes, so we have to
		//adjust the selected index
		if (pLVKeyDow->wVKey == VK_UP)
		{
			if (selIndex > 0)
				selIndex--;
		}
		if (pLVKeyDow->wVKey == VK_DOWN)
		{
			selIndex++;		
			if (selIndex >= m_LogList.GetItemCount())
				selIndex = m_LogList.GetItemCount()-1;
		}
		m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
		UpdateData(FALSE);
	}
	else
	{
		m_sLogMsgCtrl = "";
		UpdateData(FALSE);
	}
	*pResult = 0;
}


void CLogDlg::OnNMRclickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	//check if an entry is selected
	*pResult = 0;
	if (PathIsDirectory(m_path))
		return;					//no menus for directories!
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		//entry is selected, now show the popup menu
		CMenu popup;
		POINT point;
		GetCursorPos(&point);
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if (m_LogList.GetSelectedCount() == 1)
			{
				temp.LoadString(IDS_LOG_POPUP_COMPARE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
				temp.LoadString(IDS_LOG_POPUP_SAVE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
			}
			else if (m_LogList.GetSelectedCount() == 2)
			{
				temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARETWO, temp);
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			CCursor(IDC_WAIT);
			if (cmd == ID_COMPARE)
			{
				//user clicked on the menu item "compare with working copy"
				//now first get the revision which is selected
				int selIndex = m_LogList.GetSelectionMark();
				long rev = m_arRevs.GetAt(selIndex);
				//next step is to create a temporary file to hold the required revision
				TCHAR path[MAX_PATH];
				TCHAR tempF[MAX_PATH];
				DWORD len = ::GetTempPath (MAX_PATH, path);
				UINT unique = ::GetTempFileName (path, TEXT("svn"), 0, tempF);
				CString tempfile = CString(tempF);

				SVN svn;
				if (!svn.Cat(m_path, rev, tempfile))
				{
					CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return;
				}

				CString diffpath = CUtils::GetDiffPath();
				if (diffpath != "")
				{

					CString cmdline;
					cmdline = _T("\"")+diffpath; //ensure the diff exe is prepend the commandline
					cmdline += _T("\" ");
					cmdline += _T(" \"") + tempfile;
					cmdline += _T("\" "); 
					cmdline += _T(" \"") + m_path;
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
					DeleteFile(tempfile);
				} // if (diffpath != "")
			} // if (cmd == ID_COMPARE)
			if (cmd == ID_COMPARETWO)
			{
				//user clicked on the menu item "compare revisions"
				//now first get the revisions which are selected
				POSITION pos = m_LogList.GetFirstSelectedItemPosition();
				long rev1 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
				long rev2 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
				//next step is to create a temporary file to hold the required revision
				TCHAR path[MAX_PATH];
				TCHAR tempF[MAX_PATH];
				DWORD len = ::GetTempPath (MAX_PATH, path);
				UINT unique = ::GetTempFileName (path, _T("svn"), 0, tempF);
				CString tempfile1 = CString(tempF);
				len = ::GetTempPath (MAX_PATH, path);
				unique = ::GetTempFileName (path, _T("svn"), 0, tempF);
				CString tempfile2 = CString(tempF);

				SVN svn;
				if (!svn.Cat(m_path, rev1, tempfile1))
				{
					CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return;
				}
				if (!svn.Cat(m_path, rev2, tempfile2))
				{
					CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return;
				}

				CString diffpath = CUtils::GetDiffPath();
				if (diffpath != _T(""))
				{

					CString cmdline;
					cmdline = _T("\"")+diffpath; //ensure the diff exe is prepend the commandline
					cmdline += _T("\" ");
					cmdline += _T(" \"") + tempfile2;
					cmdline += _T("\" "); 
					cmdline += _T(" \"") + tempfile1;
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
					//now delete the temporary files
					DeleteFile(tempfile1);
					DeleteFile(tempfile2);
				} // if (diffpath != "")
			} // if (cmd == ID_COMPARE)
			if (cmd == ID_SAVEAS)
			{
				//now first get the revision which is selected
				int selIndex = m_LogList.GetSelectionMark();
				long rev = m_arRevs.GetAt(selIndex);

				OPENFILENAME ofn;		// common dialog box structure
				TCHAR szFile[MAX_PATH];  // buffer for file name
				ZeroMemory(szFile, sizeof(szFile));
				_tcscpy(szFile, m_path);
				// Initialize OPENFILENAME
				ZeroMemory(&ofn, sizeof(OPENFILENAME));
				//ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
				ofn.hwndOwner = this->m_hWnd;
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
				CString temp;
				temp.LoadString(IDS_LOG_POPUP_SAVE);
				//ofn.lpstrTitle = "Save revision to...\0";
				if (temp.IsEmpty())
					ofn.lpstrTitle = NULL;
				else
					ofn.lpstrTitle = temp;
				ofn.Flags = OFN_OVERWRITEPROMPT;

				// Display the Open dialog box. 
				CString tempfile;
				if (GetSaveFileName(&ofn)==TRUE)
				{
					tempfile = CString(ofn.lpstrFile);
					SVN svn;
					if (!svn.Cat(m_path, rev, tempfile))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return;
					}
				}
			} // if (cmd == ID_SAVEAS) 
			GetDlgItem(IDOK)->EnableWindow(TRUE);
		} // if (popup.CreatePopupMenu())
	} // if (selIndex >= 0)
}

void CLogDlg::OnOK()
{
	CString temp;
	CString buttontext;
	GetDlgItem(IDOK)->GetWindowText(buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();
	m_bCancelled = TRUE;
}







