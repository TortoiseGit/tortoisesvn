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
#include "UnicodeUtils.h"
#include "LogDlg.h"
#include ".\logdlg.h"

// CLogDlg dialog

IMPLEMENT_DYNAMIC(CLogDlg, CResizableDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CLogDlg::IDD, pParent)
{
	m_bCancelled = FALSE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CLogDlg::~CLogDlg()
{
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGLIST, m_LogList);
	DDX_Control(pDX, IDC_LOGMSG, m_LogMsgCtrl);
}


BEGIN_MESSAGE_MAP(CLogDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_LOGLIST, OnNMClickLoglist)
	ON_NOTIFY(NM_RCLICK, IDC_LOGLIST, OnNMRclickLoglist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, OnLvnKeydownLoglist)
	ON_NOTIFY(LVN_ITEMCHANGING, IDC_LOGMSG, OnLvnItemchangingLogmsg)
	ON_NOTIFY(NM_RCLICK, IDC_LOGMSG, OnNMRclickLogmsg)
END_MESSAGE_MAP()



void CLogDlg::SetParams(CString path, long startrev /* = 0 */, long endrev /* = -1 */, BOOL hasWC)
{
	m_path = path;
	m_startrev = startrev;
	m_endrev = endrev;
	m_hasWC = hasWC;
}

void CLogDlg::OnPaint() 
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
HCURSOR CLogDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CLogDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
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
	m_LogMsgCtrl.InsertColumn(0, _T("Message"));
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
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;

	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMSG, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CLogDlg::FillLogMessageCtrl(CString msg)
{
	m_LogMsgCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT );
	m_LogMsgCtrl.DeleteAllItems();
	m_LogMsgCtrl.SetRedraw(FALSE);
	int line = 0;
	CString temp = _T("");
	int curpos = 0;
	CString resToken = msg.Tokenize(_T("\n\r"), curpos);
	while (resToken != _T(""))
	{
		m_LogMsgCtrl.InsertItem(line++, resToken);
		resToken = msg.Tokenize(_T("\n\r"), curpos);
	} // while (resToken != _T(""))
	m_LogMsgCtrl.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
	m_LogMsgCtrl.SetRedraw();
}

BOOL CLogDlg::Cancel()
{
	return m_bCancelled;
}

BOOL CLogDlg::Log(LONG rev, CString author, CString date, CString message, CString& cpaths)
{
	int line = 0;
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
	try
	{
		temp = "";
		if (message.GetLength()>0)
		{
			message.Replace(_T("\r\n"), _T("\n"));
			message.Replace(_T("\r"), _T("\n"));
			message.Replace(_T("\n"), _T("\n\r"));
		} // if (message.GetLength()>0)
		message += _T("\r\n---------------------------------\r\n");
		line++;
		m_arFileListStarts.Add(line);
		message += cpaths;
		m_arLogMessages.Add(message);
		m_arRevs.Add(rev);
	}
	catch (CException * e)
	{
		::MessageBox(NULL, _T("not enough memory!"), _T("TortoiseSVN"), MB_ICONERROR);
		e->Delete();
		m_bCancelled = TRUE;
	}
	m_LogList.SetRedraw();
	return TRUE;
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
		//m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
		FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex));
		UpdateData(FALSE);
	}
	else
	{
		//m_sLogMsgCtrl = _T("");
		FillLogMessageCtrl(_T(""));
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
		//m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
		FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex));
		UpdateData(FALSE);
	}
	else
	{
		//m_sLogMsgCtrl = "";
		FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex));
		UpdateData(FALSE);
	}
	*pResult = 0;
}


void CLogDlg::OnNMRclickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	//check if an entry is selected
	*pResult = 0;
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
				if (!PathIsDirectory(m_path))
				{
					temp.LoadString(IDS_LOG_POPUP_COMPARE);
					if (m_hasWC)
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
					temp.LoadString(IDS_LOG_POPUP_SAVE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
				} // if (!PathIsDirectory(m_path))
				else
				{
					temp.LoadString(IDS_LOG_POPUP_COPY);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPY, temp);
				}
				temp.LoadString(IDS_LOG_POPUP_UPDATE);
				if (m_hasWC)
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);
				temp.LoadString(IDS_LOG_POPUP_REVERTREV);
				if (m_hasWC)
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
			}
			else if (m_LogList.GetSelectedCount() == 2)
			{
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF2, temp);
				if (!PathIsDirectory(m_path))
				{
					temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARETWO, temp);
				}
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			this->m_app = &theApp;
			theApp.DoWaitCursor(1);
			switch (cmd)
			{
			case ID_GNUDIFF1:
				{
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					this->m_bCancelled = FALSE;
					CString tempfile = CUtils::GetTempFile();
					tempfile += _T(".diff");
					if (!Diff(m_path, rev-1, m_path, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
					{
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						DeleteFile(tempfile);
						break;		//exit
					}
					CUtils::StartDiffViewer(tempfile);
				}
				break;
			case ID_GNUDIFF2:
				{
					POSITION pos = m_LogList.GetFirstSelectedItemPosition();
					long rev1 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
					long rev2 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
					this->m_bCancelled = FALSE;
					CString tempfile = CUtils::GetTempFile();
					tempfile += _T(".diff");
					if (!Diff(m_path, rev2, m_path, rev1, TRUE, FALSE, TRUE, _T(""), tempfile))
					{
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						DeleteFile(tempfile);
						break;		//exit
					}
					CUtils::StartDiffViewer(tempfile);
				}
				break;
			case ID_REVERTREV:
				{
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					if (CMessageBox::Show(this->m_hWnd, IDS_LOG_REVERT_CONFIRM, IDS_APPNAME, MB_ICONQUESTION) == IDOK)
					{
						SVNStatus status;
						status.GetStatus(m_path);
						if (status.status->entry == NULL)
						{
							CMessageBox::Show(NULL, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
							TRACE(_T("could not retrieve the URL of the folder!\n"));
							break;		//exit
						} // if ((rev == (-2))||(status.status->entry == NULL))
						CString url = CUnicodeUtils::GetUnicode(status.status->entry->url);
						CSVNProgressDlg dlg;
						dlg.SetParams(Enum_Merge, false, m_path, url, url, rev);		//use the message as the second url
						dlg.m_nRevisionEnd = rev-1;
						dlg.DoModal();
					}
				}
				break;
			case ID_COPY:
				{
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);
					CCopyDlg dlg;
					SVNStatus status;
					status.GetStatus(m_path);
					if (status.status->entry == NULL)
					{
						CMessageBox::Show(NULL, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the folder!\n"));
						break;		//exit
					} // if (status.status->entry == NULL) 
					CString url = CUnicodeUtils::GetUnicode(status.status->entry->url);
					dlg.m_URL = url;
					if (dlg.DoModal() == IDOK)
					{
						SVN svn;
						if (!svn.Copy(url, dlg.m_URL, rev))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
					} // if (dlg.DoModal() == IDOK) 
				} 
				break;
			case ID_COMPARE:
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
						GetDlgItem(IDOK)->EnableWindow(TRUE);
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
				}
				break;
			case ID_COMPARETWO:
				{
					//user clicked on the menu item "compare revisions"
					//now first get the revisions which are selected
					POSITION pos = m_LogList.GetFirstSelectedItemPosition();
					long rev1 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
					long rev2 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));

					StartDiff(m_path, rev1, m_path, rev2);
				}
				break;
			case ID_SAVEAS:
				{
					//now first get the revision which is selected
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);

					OPENFILENAME ofn;		// common dialog box structure
					TCHAR szFile[MAX_PATH];  // buffer for file name
					ZeroMemory(szFile, sizeof(szFile));
					if (m_hasWC)
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
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							return;
						}
					} // if (GetSaveFileName(&ofn)==TRUE)
				}
				break;
			case ID_UPDATE:
				{
					//now first get the revision which is selected
					int selIndex = m_LogList.GetSelectionMark();
					long rev = m_arRevs.GetAt(selIndex);

					SVN svn;
					if (!svn.Update(m_path, rev, TRUE))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						GetDlgItem(IDOK)->EnableWindow(TRUE);
						return;
					}
				}
				break;
			default:
				break;
			} // switch (cmd)
			theApp.DoWaitCursor(-1);
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

void CLogDlg::OnLvnItemchangingLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->iItem < m_arFileListStarts.GetAt(m_LogList.GetSelectionMark()))
		*pResult = 1;
}

void CLogDlg::OnNMRclickLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int selIndex = m_LogMsgCtrl.GetSelectionMark();
	long rev = m_arRevs.GetAt(m_LogList.GetSelectionMark());
	if (selIndex >= m_arFileListStarts.GetAt(m_LogList.GetSelectionMark()))
	{
		//entry is selected, now show the popup menu
		CMenu popup;
		POINT point;
		GetCursorPos(&point);
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if (m_LogMsgCtrl.GetSelectedCount() == 1)
			{
				temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
				temp = temp.Left(temp.Find(' '));
				CString t, tt;
				t.LoadString(IDS_SVNACTION_ADD);
				tt.LoadString(IDS_SVNACTION_DELETE);
				if ((rev > 1)&&(temp.Compare(t)!=0)&&(temp.Compare(tt)!=0))
				{
					temp.LoadString(IDS_LOG_POPUP_DIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DIFF, temp);
				}
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				GetDlgItem(IDOK)->EnableWindow(FALSE);
				this->m_app = &theApp;
				theApp.DoWaitCursor(1);
				//get the filename
				SVNStatus status;
				if ((status.GetStatus(m_path) == (-2))||(status.status->entry == NULL))
				{
					theApp.DoWaitCursor(-1);
					CMessageBox::Show(NULL, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the file!\n"));
					return;		//exit
				} // if ((rev == (-2))||(status.status->entry == NULL))
				temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
				CString filepath = CString(status.status->entry->url);
				m_bCancelled = FALSE;
				filepath = GetRepositoryRoot(filepath);
				temp = temp.Mid(temp.Find(' '));
				temp = temp.Trim();
				filepath += temp;
				switch (cmd)
				{
				case ID_DIFF:
					{
						StartDiff(filepath, rev, filepath, rev-1);
					}
					break;
				default:
					break;
				} // switch (cmd)
				theApp.DoWaitCursor(-1);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			} // if (m_LogMsgCtrl.GetSelectedCount() == 1)
		} // if (popup.CreatePopupMenu())
	} // if (selIndex >= m_arFileListStarts.GetAt(m_LogList.GetSelectionMark()))
}

BOOL CLogDlg::StartDiff(CString path1, LONG rev1, CString path2, LONG rev2)
{
	TCHAR path[MAX_PATH];
	TCHAR tempF[MAX_PATH];
	DWORD len = ::GetTempPath (MAX_PATH, path);
	UINT unique = ::GetTempFileName (path, _T("svn"), 0, tempF);
	CString tempfile1 = CString(tempF);
	len = ::GetTempPath (MAX_PATH, path);
	unique = ::GetTempFileName (path, _T("svn"), 0, tempF);
	CString tempfile2 = CString(tempF);

	CProgressDlg progDlg;
	if (progDlg.IsValid())
	{
		CString temp;
		temp.Format(IDS_PROGRESSGETFILE, path1);
		progDlg.SetLine(1, temp, true);
		temp.Format(IDS_PROGRESSREVISION, rev1);
		progDlg.SetLine(2, temp, false);
		temp.LoadString(IDS_PROGRESSWAIT);
		progDlg.SetTitle(temp);
		progDlg.SetLine(3, _T(""));
		progDlg.SetCancelMsg(_T(""));
		progDlg.ShowModeless(this);
	}
	m_bCancelled = FALSE;
	this->m_app = &theApp;
	theApp.DoWaitCursor(1);
	if (!Cat(path1, rev1, tempfile1))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	} // if (!Cat(path1, rev1, tempfile1))
	if (progDlg.IsValid())
	{
		progDlg.SetProgress((DWORD)1,(DWORD)2);
		CString temp;
		temp.Format(IDS_PROGRESSGETFILE, path1);
		progDlg.SetLine(1, temp, true);
		temp.Format(IDS_PROGRESSREVISION, rev1);
		progDlg.SetLine(2, temp, false);
	}
	if (!Cat(path2, rev2, tempfile2))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	}
	if (progDlg.IsValid())
	{
		progDlg.SetProgress((DWORD)2,(DWORD)2);
		progDlg.Stop();
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
			theApp.DoWaitCursor(-1);
			temp.Format(IDS_ERR_EXTDIFFSTART, lpMsgBuf);
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
			LocalFree( lpMsgBuf );
			DeleteFile(tempfile1);
			DeleteFile(tempfile2);
			return FALSE;
		} // if (CreateProcess(diffpath, cmdline, NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
		//wait for the process to end
		WaitForSingleObject(process.hProcess, INFINITE);
		//now delete the temporary files
		DeleteFile(tempfile1);
		DeleteFile(tempfile2);
		theApp.DoWaitCursor(-1);
		return TRUE;
	} // if (diffpath != _T("")) 
	theApp.DoWaitCursor(-1);
	return FALSE;
}








