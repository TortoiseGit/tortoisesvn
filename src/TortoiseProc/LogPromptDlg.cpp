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
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "CheckTempFiles.h"
#include "LogPromptDlg.h"
#include "UnicodeUtils.h"
#include ".\logpromptdlg.h"


// CLogPromptDlg dialog

IMPLEMENT_DYNAMIC(CLogPromptDlg, CResizableDialog)
CLogPromptDlg::CLogPromptDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CLogPromptDlg::IDD, pParent)
	, m_sLogMessage(_T(""))
	, m_bSelectAll(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_templist.RemoveAll();
}

CLogPromptDlg::~CLogPromptDlg()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
}

void CLogPromptDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOGMESSAGE, m_sLogMessage);
	DDX_Control(pDX, IDC_FILELIST, m_ListCtrl);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
}


BEGIN_MESSAGE_MAP(CLogPromptDlg, CResizableDialog)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SETCURSOR()
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(NM_RCLICK, IDC_FILELIST, OnNMRclickFilelist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_FILELIST, OnLvnItemchangedFilelist)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
END_MESSAGE_MAP()


// CLogPromptDlg message handlers
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLogPromptDlg::OnPaint() 
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
HCURSOR CLogPromptDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CLogPromptDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CString temp = m_sPath;

	//set the listcontrol to support checkboxes
	m_ListCtrl.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	m_ListCtrl.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ListCtrl.DeleteColumn(c--);
	temp.LoadString(IDS_LOGPROMPT_FILE);
	m_ListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_LOGPROMPT_STATUS);
	m_ListCtrl.InsertColumn(1, temp);

	m_ListCtrl.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_ListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_ListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &StatusThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_ListCtrl.UpdateData(FALSE);

	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();

	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HINTLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CLogPromptDlg::OnLvnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	//if the OK button is not active means the thread is still running - so let the
	//thread change the items without prompting the user
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
		return;
	//was the item checked?
	if (m_ListCtrl.GetCheck(pNMLV->iItem))
	{
		//yes, is it an unversioned item?
		if (m_arFileStatus.GetAt(pNMLV->iItem) == svn_wc_status_unversioned)
		{
			CMessageBox::ShowCheck(this->m_hWnd, 
									IDS_LOGPROMPT_ASKADD, 
									IDS_APPNAME, 
									MB_OK | MB_ICONQUESTION, 
									_T("AddUnversionedFilesOnCommitMsgBox"), 
									IDS_MSGBOX_DONOTSHOW);
			if (!PathIsDirectory(m_arFileList.GetAt(pNMLV->iItem)))
			{
				//user selected a file, so we need to check the parent folder too
				CString folderpath = m_arFileList.GetAt(pNMLV->iItem);
				folderpath = folderpath.Left(folderpath.ReverseFind('/'));
				for (int i=0; i<m_ListCtrl.GetItemCount(); i++)
				{
					CString t = m_arFileList.GetAt(i);
					if (folderpath.CompareNoCase(m_arFileList.GetAt(i))==0)
					{
						m_ListCtrl.SetCheck(i, TRUE);
						return;
					}
				} // for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
			}
		}
	}
	else
	{
		if (PathIsDirectory(m_arFileList.GetAt(pNMLV->iItem)))
		{
			//disable all files within that folder
			CString folderpath = m_arFileList.GetAt(pNMLV->iItem);
			for (int i=0; i<m_ListCtrl.GetItemCount(); i++)
			{
				CString t = m_arFileList.GetAt(i);
				if (folderpath.CompareNoCase(m_arFileList.GetAt(i).Left(folderpath.GetLength()))==0)
				{
					m_ListCtrl.SetCheck(i, FALSE);
				}
			} // for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
		} // if (PathIsDirectory(m_arFileList.GetAt(index)))
	}
}

void CLogPromptDlg::OnNMRclickFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int selIndex = m_ListCtrl.GetSelectionMark();
	if (selIndex >= 0)
	{
		CString filepath = m_arFileList.GetAt(selIndex);
		svn_wc_status_kind wcStatus = (svn_wc_status_kind)m_arFileStatus.GetAt(selIndex);
		//entry is selected, now show the popup menu
		CMenu popup;
		POINT point;
		GetCursorPos(&point);
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if ((wcStatus > svn_wc_status_normal)&&(wcStatus != svn_wc_status_added))
			{
				temp.LoadString(IDS_LOG_COMPAREWITHBASE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
				temp.LoadString(IDS_MENUREVERT);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERT, temp);
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			theApp.DoWaitCursor(1);
			switch (cmd)
			{
			case ID_REVERT:
				{
					if (CMessageBox::Show(this->m_hWnd, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO)==IDYES)
					{
						SVN svn;
						if (!svn.Revert(filepath, FALSE))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
						else
						{
							//since the entry got reverted we need to remove
							//it from the list too
							m_ListCtrl.DeleteItem(selIndex);
							m_arFileStatus.RemoveAt(selIndex);
							m_arFileList.RemoveAt(selIndex);
						}
					} // if (CMessageBox::Show(this->m_hWnd, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO)==IDYES) 
				} 
				break;
			case ID_COMPARE:
				{
					StartDiff(selIndex);
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

void CLogPromptDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	//if the OK button is not active means the thread is still running
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
		return;

	StartDiff(pNMLV->iItem);
}

void CLogPromptDlg::StartDiff(int fileindex)
{
	if (fileindex < 0)
		return;
	if (m_arFileStatus.GetAt(fileindex) == svn_wc_status_added)
		return;		//we don't compare an added file to itself
	if (m_arFileStatus.GetAt(fileindex) == svn_wc_status_deleted)
		return;		//we don't compare a deleted file (nothing) with something
	CString path1;
	CString path2 = SVN::GetPristinePath(m_arFileList.GetAt(fileindex));

	if (SVN::GetTranslatedFile(path1, m_arFileList.GetAt(fileindex)))
	{
		m_templist.Add(path1);
	}
	else
	{
		path1 = m_arFileList.GetAt(fileindex);
	}

	CUtils::StartDiffViewer(path2, path1);
}

void CLogPromptDlg::OnOK()
{
	//first add all the unversioned files the user selected
	for (int j=0; j<m_ListCtrl.GetItemCount(); j++)
	{
		if (m_ListCtrl.GetCheck(j))
		{
			if (m_arFileStatus.GetAt(j) == svn_wc_status_unversioned)
			{
				SVN svn;
				svn.Add(m_arFileList.GetAt(j), FALSE);
			} // if (m_arFileStatus.GetAt(j) == svn_wc_status_unversioned)
			if (m_arFileStatus.GetAt(j) == svn_wc_status_missing)
			{
				SVN svn;
				svn.Remove(m_arFileList.GetAt(j), TRUE);
			}
		}
	}
	//save only the files the user has selected into the temporary file
	try
	{
		CStdioFile file(m_sPath, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
		for (int i=0; i<m_ListCtrl.GetItemCount(); i++)
		{
			if (m_ListCtrl.GetCheck(i))
			{
				file.WriteString(m_arFileList.GetAt(i)+_T("\n"));
			}
		} // for (int i=0; i<m_ListCtrl.GetItemCount(); i++) 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE(_T("CFileException in Commit!\n"));
		pE->Delete();
	}

	CResizableDialog::OnOK();
}

DWORD WINAPI StatusThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CLogPromptDlg*	pDlg;
	pDlg = (CLogPromptDlg*)pVoid;
	pDlg->GetDlgItem(IDOK)->EnableWindow(false);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(false);

	pDlg->m_ListCtrl.SetRedraw(false);

	try
	{
		CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
		CString strLine = _T("");
		TCHAR buf[MAX_PATH];
		const TCHAR * strbuf = NULL;;
		while (file.ReadString(strLine))
		{
			strLine.Replace('\\', '/');
			BOOL bIsFolder = PathIsDirectory(strLine);
			SVNStatus status;
			svn_wc_status_t *s;
			s = status.GetFirstFileStatus(strLine, &strbuf);
			if (s!=0)
			{
				CString temp;
				svn_wc_status_kind stat;
				stat = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
				if (s->entry)
				{
					CUtils::Unescape((char *)s->entry->url);
					CString url = CUnicodeUtils::GetUnicode(s->entry->url);
					pDlg->GetDlgItem(IDC_COMMIT_TO)->SetWindowText(url);
				}
				temp = strbuf;
				if (SVNStatus::IsImportant(stat))
				{
					pDlg->m_arFileList.Add(strLine);
					pDlg->m_arFileStatus.Add(stat);
					int count = pDlg->m_ListCtrl.GetItemCount();
					pDlg->m_ListCtrl.InsertItem(count, strLine.Right(strLine.GetLength() - strLine.ReverseFind('/') - 1));
					//SVNStatus::GetStatusString(stat, buf);
					SVNStatus::GetStatusString(AfxGetResourceHandle(), stat, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
					if ((stat == s->prop_status)&&(!SVNStatus::IsImportant(s->text_status)))
						_tcscat(buf, _T("(P only)"));
					pDlg->m_ListCtrl.SetItemText(count, 1, buf);
					pDlg->m_ListCtrl.SetCheck(count);
				} // if (SVNStatus::IsImportant(stat)) 
				if ((stat == svn_wc_status_unversioned)&&(CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"))))
				{
					if (!CCheckTempFiles::IsTemp(strLine))
					{
						pDlg->m_arFileList.Add(strLine);
						pDlg->m_arFileStatus.Add(stat);
						int count = pDlg->m_ListCtrl.GetItemCount();
						pDlg->m_ListCtrl.InsertItem(count, strLine.Right(strLine.GetLength() - strLine.ReverseFind('/') - 1));
						//SVNStatus::GetStatusString(stat, buf);
						SVNStatus::GetStatusString(AfxGetResourceHandle(), stat, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
						pDlg->m_ListCtrl.SetItemText(count, 1, buf);
					} // if (!CCheckTempFiles::IsTemp(strLine)) 
				} // if ((stat == svn_wc_status_unversioned)&&(CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"))))  
				while ((s = status.GetNextFileStatus(&strbuf)) != NULL)
				{
					temp = strbuf;
					stat = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
					if (SVNStatus::IsImportant(stat))
					{
						pDlg->m_arFileList.Add(temp);
						pDlg->m_arFileStatus.Add(stat);
						int count = pDlg->m_ListCtrl.GetItemCount();
						if (bIsFolder)
							pDlg->m_ListCtrl.InsertItem(count, temp.Right(temp.GetLength() - strLine.GetLength() - 1));
						else
							pDlg->m_ListCtrl.InsertItem(count, temp.Right(temp.GetLength() - temp.ReverseFind('/') - 1));
						//SVNStatus::GetStatusString(stat, buf);
						SVNStatus::GetStatusString(AfxGetResourceHandle(), stat, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
						if ((stat == s->prop_status)&&(!SVNStatus::IsImportant(s->text_status)))
							_tcscat(buf, _T("(P only)"));
						pDlg->m_ListCtrl.SetItemText(count, 1, buf);
						pDlg->m_ListCtrl.SetCheck(count);
					} // if (SVNStatus::IsImportant(stat)) 
					if ((stat == svn_wc_status_unversioned)&&(CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"))))
					{
						if (!CCheckTempFiles::IsTemp(temp))
						{
							pDlg->m_arFileList.Add(temp);
							pDlg->m_arFileStatus.Add(stat);
							int count = pDlg->m_ListCtrl.GetItemCount();
							if (bIsFolder)
							{
								pDlg->m_ListCtrl.InsertItem(count, temp.Right(temp.GetLength() - strLine.GetLength() - 1));
								//we have an unversioned folder -> get all files in it recursively!
								CDirFileList	filelist;
								filelist.BuildList(temp, TRUE, TRUE);
								int count = pDlg->m_ListCtrl.GetItemCount();
								for (int i=0; i<filelist.GetSize(); i++)
								{
									CString filename = filelist.GetAt(i);
									filename.Replace('\\', '/');
									if (!CCheckTempFiles::IsTemp(filename))
									{
										pDlg->m_arFileList.Add(filename);
										pDlg->m_arFileStatus.Add(stat);
										pDlg->m_ListCtrl.InsertItem(count, filename.Right(filename.GetLength() - strLine.GetLength() - 1));
										SVNStatus::GetStatusString(AfxGetResourceHandle(), stat, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
										pDlg->m_ListCtrl.SetItemText(count++, 1, buf);
									} // if (!CCheckTempFiles::IsTemp(filename))
								} // for (int i=0; i<filelist.GetSize(); i++) 
							} // if (bIsFolder) 
							else
								pDlg->m_ListCtrl.InsertItem(count, temp.Right(temp.GetLength() - temp.ReverseFind('/') - 1));
							//SVNStatus::GetStatusString(stat, buf);
							SVNStatus::GetStatusString(AfxGetResourceHandle(), stat, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
							pDlg->m_ListCtrl.SetItemText(count, 1, buf);
						} // if (!CCheckTempFiles::IsTemp(temp))
					} // if ((stat == svn_wc_status_unversioned)&&(CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"))))  
				} // while ((s = status.GetNextFileStatus(&strbuf)) != NULL) 
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
	int maxcol = ((CHeaderCtrl*)(pDlg->m_ListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		pDlg->m_ListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	pDlg->m_ListCtrl.SetRedraw(true);

	pDlg->GetDlgItem(IDOK)->EnableWindow(true);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
	if (pDlg->m_ListCtrl.GetItemCount()==0)
	{
		CMessageBox::Show(pDlg->m_hWnd, IDS_LOGPROMPT_NOTHINGTOCOMMIT, IDS_APPNAME, MB_ICONINFORMATION);
		pDlg->EndDialog(0);
	}
	return 0;
}

void CLogPromptDlg::OnCancel()
{
	DeleteFile(m_sPath);
	UpdateData(TRUE);
	CResizableDialog::OnCancel();
}

BOOL CLogPromptDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (GetDlgItem(IDOK)->IsWindowEnabled())
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
		SetCursor(hCur);
		return CResizableDialog::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	SetCursor(hCur);
	return TRUE;
}

void CLogPromptDlg::OnBnClickedSelectall()
{
	UpdateData();
	theApp.DoWaitCursor(1);
	for (int i=0; i<m_ListCtrl.GetItemCount(); i++)
	{
		m_ListCtrl.SetCheck(i, m_bSelectAll);
	}
	theApp.DoWaitCursor(-1);
}









