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
#include "SVNProgressDlg.h"
#include "UnicodeUtils.h"
#include ".\svnprogressdlg.h"


// CSVNProgressDlg dialog

IMPLEMENT_DYNAMIC(CSVNProgressDlg, CResizableDialog)
CSVNProgressDlg::CSVNProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSVNProgressDlg::IDD, pParent)
{
	m_bCancelled = FALSE;
	m_bThreadRunning = FALSE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_nRevisionEnd = 0;
}

CSVNProgressDlg::~CSVNProgressDlg()
{
}

void CSVNProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SVNPROGRESS, m_ProgList);
}


BEGIN_MESSAGE_MAP(CSVNProgressDlg, CResizableDialog)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOGBUTTON, OnBnClickedLogbutton)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SVNPROGRESS, OnNMCustomdrawSvnprogress)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSVNProgressDlg::OnPaint() 
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
HCURSOR CSVNProgressDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CSVNProgressDlg::Cancel()
{
	return m_bCancelled;
}

BOOL CSVNProgressDlg::Notify(CString path, svn_wc_notify_action_t action, svn_node_kind_t kind, CString myme_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, LONG rev)
{
	CString temp;

	int count = m_ProgList.GetItemCount();

	int iInsertedAt = m_ProgList.InsertItem(count, GetActionText(action, content_state, prop_state));
	if (iInsertedAt != -1)
	{
		if (action != svn_wc_notify_update_completed)
		{
			m_ProgList.SetItemText(iInsertedAt, 1, path);
			m_arActions.Add(action);
			m_arActionCStates.Add(content_state);
			m_arActionPStates.Add(prop_state);
		}
		else
		{
			temp.Format(IDS_PROGRS_ATREV, rev);
			m_ProgList.SetItemText(count, 1, temp);
			m_arActions.Add(action);
			m_arActionCStates.Add(content_state);
			m_arActionPStates.Add(prop_state);
			if (m_nRevisionEnd == 0)
				m_nRevisionEnd = rev;
		}

		//make colums width fit
		ResizeColumns();

		// Make sure the item is *entirely* visible even if the horizontal
		// scroll bar is visible.
		m_ProgList.EnsureVisible(iInsertedAt, false);

	}
	return TRUE;
}

void CSVNProgressDlg::SetParams(Command cmd, BOOL isTempFile, CString path, CString url /* = "" */, CString message /* = "" */, LONG revision /* = -1 */, CString modName /* = "" */)
{
	m_Command = cmd;
	m_IsTempFile = isTempFile;
	m_sPath = path;
	m_sUrl = url;
	m_sMessage = message;
	m_nRevision = revision;
	m_sModName = modName;
}

void CSVNProgressDlg::ResizeColumns()
{
	m_ProgList.SetRedraw(false);

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;

	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_ProgList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	
	m_ProgList.SetRedraw(true);
	
	if (IsWindowVisible())
	{
		Invalidate(TRUE);
	}
}


// CSVNProgressDlg message handlers

BOOL CSVNProgressDlg::OnInitDialog()
{
	__super::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_ProgList.SetExtendedStyle (LVS_EX_FULLROWSELECT);

	m_ProgList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ProgList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROGRS_ACTION);
	m_ProgList.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	m_ProgList.InsertColumn(1, temp);

	//first start a thread to obtain the log messages without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &ProgressThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_ICONERROR);
	}
	UpdateData(FALSE);

	// Call this early so that the column headings aren't hidden before any
	// text gets added.
	ResizeColumns();

	m_arActions.RemoveAll();
	m_arActionCStates.RemoveAll();
	m_arActionPStates.RemoveAll();

	AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


DWORD WINAPI ProgressThread(LPVOID pVoid)
{
	CSVNProgressDlg*	pDlg;
	pDlg = (CSVNProgressDlg*)pVoid;

	int updateFileCounter = 0;
	CRegString logmessage = CRegString(_T("\\Software\\TortoiseSVN\\lastlogmessage"));

	CString temp;
	//temp.LoadString(IDS_MSGBOX_CANCEL);
	//pDlg->GetDlgItem(IDOK)->SetWindowText(temp);

	pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(TRUE);

	pDlg->m_bThreadRunning = TRUE;
	switch (pDlg->m_Command)
	{
		case Checkout:			//no tempfile!
			temp.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
			pDlg->SetWindowText(temp);
			if (!pDlg->Checkout(pDlg->m_sUrl, pDlg->m_sPath, pDlg->m_nRevision, true))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			break;
		case Import:			//no tempfile!
			temp.LoadString(IDS_PROGRS_TITLE_IMPORT);
			pDlg->SetWindowText(temp);
			if (!pDlg->Import(pDlg->m_sPath, pDlg->m_sUrl, pDlg->m_sMessage, true))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
				logmessage.removeValue();
			break;
		case Update:
			temp.LoadString(IDS_PROGRS_TITLE_UPDATE);
			pDlg->SetWindowText(temp);
			if (pDlg->m_IsTempFile)
			{
				LONG rev = pDlg->m_nRevision;
				try
				{
					// open the temp file
					CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead); 
					CString strLine = _T(""); // initialise the variable which holds each line's contents
					CString sfile;
					while (file.ReadString(strLine))
					{
						SVNStatus st;
						if (st.GetStatus(strLine) != (-2))
						{
							if (st.status->entry != NULL)
							{
								pDlg->m_nRevision = st.status->entry->revision;
							}
						}
						TRACE(_T("update file %s\n"), strLine);
						if (!pDlg->Update(strLine, rev, true))
						{
							TRACE(_T("%s"), pDlg->GetLastErrorMessage());
							CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						updateFileCounter++;
						sfile = strLine;
					}
					file.Close();
					CFile::Remove(pDlg->m_sPath);
					pDlg->m_sPath = sfile;		// the log would be shown for the last selected file/dir in the list
				}
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Update!\n"));
					pE->Delete();
				}
				// after an update, show the user the log button, but only if only one single item was updated
				// (either a file or a directory)
				if (updateFileCounter == 1)
					pDlg->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
			}
			else
			{
				//check the revision of the directory before the update
				LONG rev = pDlg->m_nRevision;
				SVNStatus st;
				if (st.GetStatus(pDlg->m_sPath) != (-2))
				{
					if (st.status->entry != NULL)
					{
						pDlg->m_nRevision = st.status->entry->revision;
					}
				}
				if (pDlg->Update(pDlg->m_sPath, rev, true))
				{
					pDlg->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
				}
				else
				{
					TRACE(_T("%s"), pDlg->GetLastErrorMessage());
					CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
			}
			break;
		case Commit:
			temp.LoadString(IDS_PROGRS_TITLE_COMMIT);
			pDlg->SetWindowText(temp);
			if (pDlg->m_IsTempFile)
			{
				try
				{
					CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
					CString strLine = _T("");
					CString commitString = _T("");
					BOOL isTag = FALSE;
					CString url;
					while (file.ReadString(strLine))
					{
						SVNStatus svnStatus;
						if (svnStatus.GetStatus(strLine) != (-2))
						{
							if ((svnStatus.status->entry)&&(svnStatus.status->entry->url))
								url = svnStatus.status->entry->url;
							if (url.Find(_T("/tags/"))>=0)
								isTag = TRUE;
							if (commitString.IsEmpty())
								commitString = strLine;
							else
								commitString = commitString + _T(";") + strLine;
						}
					}
					if (commitString.IsEmpty())
					{
						temp.LoadString(IDS_PROGRS_TITLEFIN);
						pDlg->SetWindowText(temp);
						temp.LoadString(IDS_MSGBOX_OK);
						//pDlg->GetDlgItem(IDOK)->SetWindowText(temp);
						//pDlg->m_bCancelled = TRUE;

						pDlg->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
						pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);

						pDlg->m_bThreadRunning = FALSE;
						pDlg->GetDlgItem(IDOK)->EnableWindow(true);
						return 0;
					}
					if (isTag)
					{
						if (CMessageBox::Show(pDlg->m_hWnd, IDS_PROGRS_COMMITT_TRUNK, IDS_APPNAME, MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)==IDCANCEL)
							return 0;
					}
					if (!pDlg->Commit(commitString, pDlg->m_sMessage, true))
					{
						TRACE("%s", pDlg->GetLastErrorMessage());
						CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					} // if (!pDlg->Commit(commitString, pDlg->m_sMessage, true))
					else
						logmessage.removeValue();
					file.Close();
					CFile::Remove(pDlg->m_sPath);
				}
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Commit!\n"));
					pE->Delete();
				}
			}
			else
			{
				pDlg->Commit(pDlg->m_sPath, pDlg->m_sMessage, true);
			}
			break;
		case Add:
			temp.LoadString(IDS_PROGRS_TITLE_ADD);
			pDlg->SetWindowText(temp);
			if (pDlg->m_IsTempFile)
			{
				try
				{
					CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
					CString strLine = _T("");
					while (file.ReadString(strLine))
					{
						TRACE(_T("add file %s\n"), strLine);
						if (!pDlg->Add(strLine, false))
						{
							TRACE(_T("%s"), pDlg->GetLastErrorMessage());
							CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
					}
					file.Close();
					CFile::Remove(pDlg->m_sPath);
				}
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Commit!\n"));
					pE->Delete();
				}
			}
			else
			{
				pDlg->Commit(pDlg->m_sPath, pDlg->m_sMessage, true);
			}
			break;
		case Revert:
			temp.LoadString(IDS_PROGRS_TITLE_REVERT);
			pDlg->SetWindowText(temp);
			if (pDlg->m_IsTempFile)
			{
				try
				{
					CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
					CString strLine = _T("");
					while (file.ReadString(strLine))
					{
						TRACE(_T("revert file %s\n"), strLine);
						if (!pDlg->Revert(strLine, true))
						{
							TRACE(_T("%s"), pDlg->GetLastErrorMessage());
							CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
					}
					file.Close();
					CFile::Remove(pDlg->m_sPath);
				}
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Commit!\n"));
					pE->Delete();
				}
			}
			else
			{
				pDlg->Commit(pDlg->m_sPath, pDlg->m_sMessage, true);
			}
			break;
		case Resolve:
			temp.LoadString(IDS_PROGRS_TITLE_RESOLVE);
			pDlg->SetWindowText(temp);
			pDlg->Resolve(pDlg->m_sPath, true);
			break;
		case Switch:
			temp.LoadString(IDS_PROGRS_TITLE_SWITCH);
			pDlg->SetWindowText(temp);
			if (!pDlg->Switch(pDlg->m_sPath, pDlg->m_sUrl, pDlg->m_nRevision, true))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			break;
		case Export:
			temp.LoadString(IDS_PROGRS_TITLE_EXPORT);
			pDlg->SetWindowText(temp);
			if (!pDlg->Export(pDlg->m_sUrl, pDlg->m_sPath, -1))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			break;
		case Merge:
			temp.LoadString(IDS_PROGRS_TITLE_MERGE);
			pDlg->SetWindowText(temp);
			if (!pDlg->Merge(pDlg->m_sUrl, pDlg->m_nRevision, pDlg->m_sMessage, pDlg->m_nRevisionEnd, pDlg->m_sPath, true, true))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			break;
		case Copy:
			temp.LoadString(IDS_PROGRS_TITLE_COPY);
			pDlg->SetWindowText(temp);
			if (!pDlg->Copy(pDlg->m_sPath, pDlg->m_sUrl, pDlg->m_nRevision, pDlg->m_sMessage))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				break;
			}
			else
				logmessage.removeValue();
			CString sTemp;
			sTemp.LoadString(IDS_PROGRS_COPY_WARNING);
			CMessageBox::Show(pDlg->m_hWnd, sTemp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
			break;
	}
	temp.LoadString(IDS_PROGRS_TITLEFIN);
	pDlg->SetWindowText(temp);
	//temp.LoadString(IDS_MSGBOX_OK);
	//pDlg->GetDlgItem(IDOK)->SetWindowText(temp);

	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);

	pDlg->m_bCancelled = TRUE;
	pDlg->m_bThreadRunning = FALSE;
	return 0;
}

void CSVNProgressDlg::OnBnClickedLogbutton()
{
	CLogDlg dlg;
	dlg.SetParams(m_sPath, m_nRevisionEnd, m_nRevision);
	dlg.DoModal();
}

void CSVNProgressDlg::OnClose()
{
	if (m_bCancelled)
		TerminateThread(m_hThread, -1);
	else
	{
		m_bCancelled = TRUE;
		return;
	}
	__super::OnClose();
}

void CSVNProgressDlg::OnOK()
{
	if ((m_bCancelled)&&(!m_bThreadRunning))
		__super::OnOK();
	m_bCancelled = TRUE;
}

void CSVNProgressDlg::OnCancel()
{
	if ((m_bCancelled)&&(!m_bThreadRunning))
		__super::OnCancel();
	m_bCancelled = TRUE;
}

void CSVNProgressDlg::OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
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
		// We'll cycle the colors through red, green, and light blue.

		COLORREF crText = RGB(0,0,0);

		if (m_arActions.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			svn_wc_notify_action_t action = (svn_wc_notify_action_t)m_arActions.GetAt(pLVCD->nmcd.dwItemSpec);
			svn_wc_notify_state_t content_state = (svn_wc_notify_state_t)m_arActionCStates.GetAt(pLVCD->nmcd.dwItemSpec);
			svn_wc_notify_state_t prop_state = (svn_wc_notify_state_t)m_arActionPStates.GetAt(pLVCD->nmcd.dwItemSpec);

			switch (action)
			{
			case svn_wc_notify_add:
			case svn_wc_notify_update_add:
			case svn_wc_notify_commit_added:
				crText = GetSysColor(COLOR_HIGHLIGHT);
			case svn_wc_notify_delete:
			case svn_wc_notify_update_delete:
			case svn_wc_notify_commit_deleted:
			case svn_wc_notify_commit_replaced:
				crText = RGB(100,0,0);
				break;
			case svn_wc_notify_update_update:
				if ((content_state == svn_wc_notify_state_conflicted) || (prop_state == svn_wc_notify_state_conflicted))
					crText = RGB(255, 0, 0);
				else if ((content_state == svn_wc_notify_state_merged) || (prop_state == svn_wc_notify_state_merged))
					crText = RGB(0, 100, 0);
				break;
			case svn_wc_notify_commit_modified:
				crText = GetSysColor(COLOR_HIGHLIGHT);
				break;
			default:
				crText = GetSysColor(COLOR_WINDOWTEXT);
				break;
			} // switch (action)

			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
		}

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;
	}
}


