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
#include "SVNProgressDlg.h"
#include "UnicodeUtils.h"
#include <map>
#include ".\svnprogressdlg.h"


// CSVNProgressDlg dialog

BOOL	CSVNProgressDlg::m_bAscending = FALSE;
int		CSVNProgressDlg::m_nSortedColumn = -1;

IMPLEMENT_DYNAMIC(CSVNProgressDlg, CResizableDialog)
CSVNProgressDlg::CSVNProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSVNProgressDlg::IDD, pParent)
	, m_Revision(_T("HEAD"))
	, m_RevisionEnd(0)
{
	m_bCancelled = FALSE;
	m_bThreadRunning = FALSE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CSVNProgressDlg::~CSVNProgressDlg()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
	for (int i=0; i<m_arData.GetCount(); i++)
	{
		Data * data = m_arData.GetAt(i);
		delete data;
	} // for (int i=0; i<m_arData.GetCount(); i++)
	m_arData.RemoveAll();
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
	ON_NOTIFY(NM_DBLCLK, IDC_SVNPROGRESS, OnNMDblclkSvnprogress)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickSvnprogress)
	ON_WM_SETCURSOR()
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

BOOL CSVNProgressDlg::Notify(CString path, svn_wc_notify_action_t action, svn_node_kind_t kind, CString mime_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, LONG rev)
{
	CString temp;

	int count = m_ProgList.GetItemCount();

	int iInsertedAt = m_ProgList.InsertItem(count, GetActionText(action, content_state, prop_state));
	if (iInsertedAt != -1)
	{
		if (action != svn_wc_notify_update_completed)
		{
			m_ProgList.SetItemText(iInsertedAt, 1, path);
		} // if (action != svn_wc_notify_update_completed)
		else
		{
			temp.Format(IDS_PROGRS_ATREV, rev);
			m_ProgList.SetItemText(count, 1, temp);
			m_RevisionEnd = rev;
		}
		//m_ProgList.SetItemText(iInsertedAt, 2, mime_type);
		Data * data = new Data();
		data->path = path;
		data->action = action;
		data->kind = kind;
		data->mime_type = mime_type;
		data->content_state = content_state;
		data->prop_state = prop_state;
		data->rev = rev;
		m_arData.Add(data);
		//make colums width fit
		ResizeColumns();

		// Make sure the item is *entirely* visible even if the horizontal
		// scroll bar is visible.
		m_ProgList.EnsureVisible(iInsertedAt, false);

	}
	return TRUE;
}

void CSVNProgressDlg::SetParams(Command cmd, BOOL isTempFile, CString path, CString url /* = "" */, CString message /* = "" */, SVNRev revision /* = -1 */, CString modName /* = "" */)
{
	m_Command = cmd;
	m_IsTempFile = isTempFile;
	m_sPath = path;
	m_sUrl = url;
	m_sMessage = message;
	m_Revision = revision;
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

	m_ProgList.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_DOUBLEBUFFER);

	m_ProgList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ProgList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROGRS_ACTION);
	m_ProgList.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	m_ProgList.InsertColumn(1, temp);
	temp.LoadString(IDS_PROGRS_MIMETYPE);
	//m_ProgList.InsertColumn(2, temp);

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

	AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
	this->hWnd = this->m_hWnd;
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
	CString sWindowTitle;

	pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(TRUE);

	pDlg->m_bThreadRunning = TRUE;
	switch (pDlg->m_Command)
	{
		case Checkout:			//no tempfile!
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
			pDlg->SetWindowText(sWindowTitle);
			if (!pDlg->Checkout(pDlg->m_sUrl, pDlg->m_sPath, pDlg->m_Revision, pDlg->m_IsTempFile /* temfile used as recursive/nonrecursive */))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			break;
		case Import:			//no tempfile!
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_IMPORT);
			pDlg->SetWindowText(sWindowTitle);
			if (!pDlg->Import(pDlg->m_sPath, pDlg->m_sUrl, pDlg->m_sMessage, true))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
				logmessage.removeValue();
			break;
		case Update:
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_UPDATE);
			pDlg->SetWindowText(sWindowTitle);
			if (pDlg->m_IsTempFile)
			{
				try
				{
					// open the temp file
					CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead); 
					CString strLine = _T(""); // initialise the variable which holds each line's contents
					CString sfile;
					CStringA uuid;
					std::map<CStringA, LONG> uuidmap;
					SVNRev revstore = pDlg->m_Revision;
					while (file.ReadString(strLine))
					{
						SVNStatus st;
						LONG headrev = -1;
						pDlg->m_Revision = revstore;
						if (pDlg->m_Revision.IsHead())
						{
							if ((headrev = st.GetStatus(strLine, TRUE)) != (-2))
							{
								if (st.status->entry != NULL)
								{
									pDlg->m_nUpdateStartRev = st.status->entry->cmt_rev;
									if (st.status->entry->uuid)
									{
										uuid = st.status->entry->uuid;
										std::map<CStringA, LONG>::iterator iter;
										if ((iter = uuidmap.find(uuid)) == uuidmap.end())
											uuidmap[uuid] = headrev;
										else
											headrev = iter->second;
										pDlg->m_Revision = headrev;
									} // if (st.status->entry->uuid)
									else
										pDlg->m_Revision = headrev;
								} // if (st.status->entry != NULL) 
							} // if ((headrev = st.GetStatus(strLine)) != (-2)) 
							else
							{
								if ((headrev = st.GetStatus(strLine, FALSE)) != (-2))
								{
									if (st.status->entry != NULL)
										pDlg->m_nUpdateStartRev = st.status->entry->cmt_rev;
								}
							}
						} // if (pDlg->m_Revision.IsHead()) 
						TRACE(_T("update file %s\n"), strLine);
						if (!pDlg->Update(strLine, pDlg->m_Revision, (pDlg->m_sModName.Compare(_T("yes"))!=0)))
						{
							TRACE(_T("%s"), pDlg->GetLastErrorMessage());
							CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						updateFileCounter++;
						sfile = strLine;
					} // while (file.ReadString(strLine)) 
					file.Close();
					DeleteFile(pDlg->m_sPath);
					pDlg->m_sPath = sfile;		// the log would be shown for the last selected file/dir in the list
				}
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Update!\n"));
					pE->ReportError();
					pE->Delete();
					updateFileCounter = 0;
				}
				// after an update, show the user the log button, but only if only one single item was updated
				// (either a file or a directory)
				if (updateFileCounter == 1)
					pDlg->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
			} // if (pDlg->m_IsTempFile) 
			else
			{
				//check the revision of the directory before the update
				LONG rev = pDlg->m_Revision;
				SVNStatus st;
				if (st.GetStatus(pDlg->m_sPath) != (-2))
				{
					if (st.status->entry != NULL)
					{
						pDlg->m_Revision = st.status->entry->revision;
					}
				}
				if (pDlg->Update(pDlg->m_sPath, rev, true))
				{
					pDlg->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
					break;
				}
				else
				{
					TRACE(_T("%s"), pDlg->GetLastErrorMessage());
					CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
				pDlg->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
			}
			break;
		case Commit:
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_COMMIT);
			pDlg->SetWindowText(sWindowTitle);
			if (pDlg->m_IsTempFile)
			{
				try
				{
					CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
					CString strLine = _T("");
					CString commitString = _T("");
					BOOL isTag = FALSE;
					BOOL bStatusFetched = FALSE;
					CString url;
					int nTargets = 0;
					while (file.ReadString(strLine))
					{
						nTargets++;
						if (bStatusFetched == FALSE)
						{
							// getting the Status once is enough, since it's not possible to
							// select multiple files/folders from different working copies in
							// the explorer. So once we have the status for one file/folder
							// and its URL, that's enough.
							SVNStatus svnStatus;
							if (svnStatus.GetStatus(strLine) != (-2))
							{
								if ((svnStatus.status->entry)&&(svnStatus.status->entry->url))
								{
									url = svnStatus.status->entry->url;
									bStatusFetched = TRUE;
								}
								if (url.Find(_T("/tags/"))>=0)
									isTag = TRUE;
							} // if (svnStatus.GetStatus(strLine) != (-2))
						} // if (bStatusFetched == FALSE)
						if (commitString.IsEmpty())
							commitString = strLine;
						else
							commitString = commitString + _T("*") + strLine;
					} // while (file.ReadString(strLine)) 
					if (commitString.IsEmpty())
					{
						temp.LoadString(IDS_PROGRS_TITLEFIN);
						sWindowTitle = sWindowTitle + _T(" ") + temp;
						pDlg->SetWindowText(sWindowTitle);
						temp.LoadString(IDS_MSGBOX_OK);

						pDlg->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
						pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);

						pDlg->m_bThreadRunning = FALSE;
						pDlg->GetDlgItem(IDOK)->EnableWindow(true);
						break;
					}
					if (isTag)
					{
						if (CMessageBox::Show(pDlg->m_hWnd, IDS_PROGRS_COMMITT_TRUNK, IDS_APPNAME, MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)==IDCANCEL)
							break;
					} // if (isTag)
					if (!pDlg->Commit(commitString, pDlg->m_sMessage, ((nTargets == 1)&&(pDlg->m_Revision == 0))))
					{
						TRACE("%s", pDlg->GetLastErrorMessage());
						CMessageBox::Show(NULL, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					} // if (!pDlg->Commit(commitString, pDlg->m_sMessage, true))
					else
						logmessage.removeValue();
					file.Close();
					DeleteFile(pDlg->m_sPath);
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
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_ADD);
			pDlg->SetWindowText(sWindowTitle);
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
					DeleteFile(pDlg->m_sPath);
				}
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Add!\n"));
					pE->Delete();
				}
			}
			else
			{
				pDlg->Commit(pDlg->m_sPath, pDlg->m_sMessage, true);
			}
			break;
		case Revert:
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_REVERT);
			pDlg->SetWindowText(sWindowTitle);
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
					DeleteFile(pDlg->m_sPath);
				}
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Revert!\n"));
					pE->Delete();
				}
			}
			else
			{
				pDlg->Commit(pDlg->m_sPath, pDlg->m_sMessage, true);
			}
			break;
		case Resolve:
			{
				sWindowTitle.LoadString(IDS_PROGRS_TITLE_RESOLVE);
				pDlg->SetWindowText(sWindowTitle);
				//check if the file may still have conflict markers in it.
				BOOL bMarkers = FALSE;
				try
				{
					if (!PathIsDirectory(pDlg->m_sPath))
					{
						CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
						CString strLine = _T("");
						while (file.ReadString(strLine))
						{
							if (strLine.Find(_T("<<<<<<<"))==0)
							{
								bMarkers = TRUE;
								break;
							}
						}
						file.Close();
					} // if (!PathIsDirectory(pDlg->m_sPath)) 
				} 
				catch (CFileException* pE)
				{
					TRACE(_T("CFileException in Resolve!\n"));
					pE->Delete();
				}
				if (bMarkers)
				{
					if (CMessageBox::Show(pDlg->m_hWnd, IDS_PROGRS_REVERTMARKERS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)==IDYES)
						pDlg->Resolve(pDlg->m_sPath, true);
				} // if (bMarkers)
				else
					pDlg->Resolve(pDlg->m_sPath, true);
			}
			break;
		case Switch:
			{
				SVNStatus st;
				sWindowTitle.LoadString(IDS_PROGRS_TITLE_SWITCH);
				pDlg->SetWindowText(sWindowTitle);
				LONG rev = 0;
				if (st.GetStatus(pDlg->m_sPath) != (-2))
				{
					if (st.status->entry != NULL)
					{
						rev = st.status->entry->revision;
					}
				}
				if (!pDlg->Switch(pDlg->m_sPath, pDlg->m_sUrl, pDlg->m_Revision, true))
				{
					CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					break;
				} // if (!pDlg->Switch(pDlg->m_sPath, pDlg->m_sUrl, pDlg->m_nRevision, true))
				pDlg->m_nUpdateStartRev = rev;
				if (pDlg->m_RevisionEnd > pDlg->m_nUpdateStartRev)
					pDlg->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
			}
			break;
		case Export:
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_EXPORT);
			pDlg->SetWindowText(sWindowTitle);
			if (!pDlg->Export(pDlg->m_sUrl, pDlg->m_sPath, pDlg->m_Revision))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			break;
		case Merge:
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGE);
			pDlg->SetWindowText(sWindowTitle);
			if (!pDlg->Merge(pDlg->m_sUrl, pDlg->m_Revision, pDlg->m_sMessage, pDlg->m_RevisionEnd, pDlg->m_sPath, true, true))
			{
				CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			break;
		case Copy:
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_COPY);
			pDlg->SetWindowText(sWindowTitle);
			if (!pDlg->Copy(pDlg->m_sPath, pDlg->m_sUrl, pDlg->m_Revision, pDlg->m_sMessage))
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
	sWindowTitle = sWindowTitle + _T(" ") + temp;
	pDlg->SetWindowText(sWindowTitle);

	pDlg->ReleasePool();

	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);

	pDlg->m_bCancelled = TRUE;
	pDlg->m_bThreadRunning = FALSE;
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	if ((WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\AutoClose"), FALSE))
		pDlg->PostMessage(WM_QUIT);
	return 0;
}

void CSVNProgressDlg::OnBnClickedLogbutton()
{
	CLogDlg dlg;
	dlg.SetParams(m_sPath, m_RevisionEnd, m_nUpdateStartRev);
	dlg.DoModal();
}

void CSVNProgressDlg::OnClose()
{
	if (m_bCancelled)
	{
		TerminateThread(m_hThread, -1);
		m_bThreadRunning = FALSE;
	}
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

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = RGB(0,0,0);

		if (m_arData.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			Data * data = m_arData.GetAt(pLVCD->nmcd.dwItemSpec);
			if (data == NULL)
				return;
				
			switch (data->action)
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
				if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
					crText = RGB(255, 0, 0);
				else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
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
	}
}

void CSVNProgressDlg::OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iItem < 0)
		return;
	Data * data = m_arData.GetAt(pNMLV->iItem);
	if ((data->action == svn_wc_notify_update_update) &&
		((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted)))
	{
		SVNStatus status;
		CString merge = data->path;
		CString path = merge.Left(merge.ReverseFind('/'));
		path = path + _T("/");
		status.GetStatus(merge);
		CString theirs;
		CString mine;
		CString base;

		//we have the conflicted file (%merged)
		//now look for the other required files
		SVNStatus stat;
		stat.GetStatus(merge);
		if (stat.status->entry)
		{
			if (stat.status->entry->conflict_new)
			{
				theirs = stat.status->entry->conflict_new;
				theirs = path + theirs;
			}
			if (stat.status->entry->conflict_old)
			{
				base = stat.status->entry->conflict_old;
				base = path + base;
			}
			if (stat.status->entry->conflict_wrk)
			{
				mine = stat.status->entry->conflict_wrk;
				mine = path + mine;
			}
		}
		CUtils::StartExtMerge(base, theirs, mine, merge);
	} // ify_state_conflicted) || (prop_state == svn_wc_notify_state_conflicted)))
	else if ((data->action == svn_wc_notify_update_update) && (data->content_state == svn_wc_notify_state_merged))
	{
		CString sWC = data->path;
		CString sBase = SVN::GetPristinePath(sWC);

		if ((!CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase"), TRUE))&&(SVN::GetTranslatedFile(sWC, sWC)))
		{
			m_templist.Add(sWC);
		}
		CString name = CUtils::GetFileNameFromPath(data->path);
		CString ext = CUtils::GetFileExtFromPath(data->path);
		CString n1, n2;
		n1.Format(IDS_DIFF_WCNAME, name);
		n2.Format(IDS_DIFF_BASENAME, name);
		CUtils::StartDiffViewer(sBase, sWC, FALSE, n2, n1, ext);

	}
}

void CSVNProgressDlg::OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	if (m_bThreadRunning)
		return;
	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;
	Sort();

	CString temp;
	m_ProgList.SetRedraw(FALSE);
	m_ProgList.DeleteAllItems();
	for (int i=0; i<m_arData.GetCount(); i++)
	{
		Data * data = m_arData.GetAt(i);
		m_ProgList.InsertItem(i, GetActionText(data->action, data->content_state, data->prop_state));
		if (data->action != svn_wc_notify_update_completed)
		{
			m_ProgList.SetItemText(i, 1, data->path);
		} // if (action != svn_wc_notify_update_completed)
		else
		{
			temp.Format(IDS_PROGRS_ATREV, data->rev);
			m_ProgList.SetItemText(i, 1, temp);
		}
		//m_ProgList.SetItemText(i, 2, data->mime_type);
	} // for (int i=0; i<m_arData.GetCount(); i++) 
	
	m_ProgList.SetRedraw(TRUE);

	*pResult = 0;
}

void CSVNProgressDlg::Sort()
{
	qsort(m_arData.GetData(), m_arData.GetSize(), sizeof(Data *), (GENERICCOMPAREFN)SortCompare);
}

int CSVNProgressDlg::SortCompare(const void * pElem1, const void * pElem2)
{
	Data * pData1 = *((Data**)pElem1);
	Data * pData2 = *((Data**)pElem2);
	int result = 0;
	switch (m_nSortedColumn)
	{
	case 0:		//action column
		{
			CString t1 = GetActionText(pData1->action, pData1->content_state, pData1->prop_state);
			CString t2 = GetActionText(pData2->action, pData2->content_state, pData2->prop_state);
			result = t1.Compare(t2);
			if (result == 0)
			{
				result = pData1->path.Compare(pData2->path);
			}
		}
		break;
	case 1:		//path column
		{
			result = pData1->path.Compare(pData2->path);
		}
		break;
	case 2:		//mime-type column
		{
			result = pData1->mime_type.Compare(pData2->mime_type);
			if (result == 0)
			{
				result = pData1->path.Compare(pData2->path);
			}
		}
		break;
	default:
		break;
	} // switch (m_nSortedColumn)
	if (!m_bAscending)
		result = -result;
	return result;
}

BOOL CSVNProgressDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
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
