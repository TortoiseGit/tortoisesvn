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

#include "MessageBox.h"
#include "InputDlg.h"
#include "LogDlg.h"
#include "PropDlg.h"
#include "Blame.h"
#include "BlameDlg.h"
#include "WaitCursorEx.h"
#include "Repositorybrowser.h"
#include "BrowseFolder.h"
#include "RenameDlg.h"


#define ID_POPSAVEAS		1
#define ID_POPSHOWLOG		2
#define	ID_POPOPEN			3
#define ID_POPDELETE		4
#define ID_POPIMPORT		5
#define ID_POPRENAME		6
#define ID_POPCOPYTO		7
#define ID_POPMKDIR			8
#define ID_POPGNUDIFF		9
#define ID_POPDIFF			10
#define ID_POPREFRESH		11
#define ID_POPBLAME			12
#define ID_POPCOPYTOWC		13
#define ID_POPIMPORTFOLDER  14
//#define ID_POPPROPS			17		commented out because already defined to 17 in LogDlg.h

// CRepositoryBrowser dialog

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableDialog)

CRepositoryBrowser::CRepositoryBrowser(const SVNUrl& svn_url, BOOL bFile)
	: CResizableDialog(CRepositoryBrowser::IDD, NULL)
	, m_treeRepository(svn_url.GetPath(), bFile)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_bStandAlone(true)
	, m_InitialSvnUrl(svn_url)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CRepositoryBrowser::CRepositoryBrowser(const SVNUrl& svn_url, CWnd* pParent, BOOL bFile)
	: CResizableDialog(CRepositoryBrowser::IDD, pParent)
	, m_treeRepository(svn_url.GetPath(), bFile)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_InitialSvnUrl(svn_url)
	, m_bStandAlone(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CRepositoryBrowser::~CRepositoryBrowser()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REPOS_TREE, m_treeRepository);
}


BEGIN_MESSAGE_MAP(CRepositoryBrowser, CResizableDialog)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZING()
	ON_NOTIFY(RVN_ITEMRCLICK, IDC_REPOS_TREE, OnRVNItemRClickReposTree)
	ON_NOTIFY(RVN_ITEMRCLICKUP, IDC_REPOS_TREE, OnRVNItemRClickUpReposTree)
	ON_NOTIFY(RVN_KEYDOWN, IDC_REPOS_TREE, OnRVNKeyDownReposTree)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()



// CRepositoryBrowser public interface

SVNUrl CRepositoryBrowser::GetURL() const
{
	return m_barRepository.GetCurrentUrl();
}

SVNRev CRepositoryBrowser::GetRevision() const
{
	return GetURL().GetRevision();
}

CString CRepositoryBrowser::GetPath() const
{
	return GetURL().GetPath();
}



// CRepositoryBrowser message handlers

void CRepositoryBrowser::OnPaint() 
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
HCURSOR CRepositoryBrowser::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CRepositoryBrowser::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_cnrRepositoryBar.SubclassDlgItem(IDC_REPOS_BAR_CNR, this);
	m_barRepository.Create(&m_cnrRepositoryBar, 12345);
	m_barRepository.AssocTree(&m_treeRepository);
	m_treeRepository.Init(GetRevision());

	if (m_InitialSvnUrl.GetPath().IsEmpty())
		m_InitialSvnUrl = m_barRepository.GetCurrentUrl();

	m_barRepository.GotoUrl(m_InitialSvnUrl);

	if (m_bStandAlone)
	{
		GetDlgItem(IDCANCEL)->ShowWindow(FALSE);

		// reposition buttons
		CRect rect_cancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rect_cancel);
		ScreenToClient(rect_cancel);
		GetDlgItem(IDOK)->MoveWindow(rect_cancel);
	}

	AddAnchor(IDC_REPOS_TREE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REPOS_BAR_CNR, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_F5HINT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(_T("RepositoryBrowser"));
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRepositoryBrowser::OnRVNItemRClickReposTree(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	*pResult = 0;	// Force standard behaviour
}

void CRepositoryBrowser::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		m_treeRepository.GetItemRect(m_treeRepository.GetFirstSelectedItem(), 0, &rect, RVIR_TEXT);
		m_treeRepository.ClientToScreen(&rect);
		point.x = rect.left + rect.Width()/2;
		point.y = rect.top + rect.Height()/2;
	}
	LRESULT Result = 0;
	ShowContextMenu(point, &Result);
}

void CRepositoryBrowser::OnRVNItemRClickUpReposTree(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	POINT point;
	GetCursorPos(&point);
	ShowContextMenu(point, pResult);
}

void CRepositoryBrowser::ShowContextMenu(CPoint pt, LRESULT *pResult)
{
	*pResult = 0;
	HTREEITEM hSelItem = m_treeRepository.GetItemHandle(m_treeRepository.GetFirstSelectedItem());
	UINT uSelCount = m_treeRepository.GetSelectedCount();
	CString url = m_treeRepository.MakeUrl(hSelItem);

	HTREEITEM hSelItem1;
	HTREEITEM hSelItem2;

	CString url1;
	CString url2;

	BOOL bFolder1;
	BOOL bFolder2;

	if (hSelItem)
	{
		BOOL bFolder = m_treeRepository.IsFolder(hSelItem);

		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if (uSelCount == 1)
			{
				// Let "Open" be the very first entry, like in Explorer
				if (GetRevision().IsHead())
				{
					if (!bFolder && (url.Left(4).CompareNoCase(_T("http")) == 0))
					{
						temp.LoadString(IDS_REPOBROWSE_OPEN);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPOPEN, temp);		// "open"
					}
				}

				temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSHOWLOG, temp);			// "Show Log..."

				if (!bFolder)
				{
					temp.LoadString(IDS_MENUBLAME);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPBLAME, temp);			// "Blame..."
				}

				if (bFolder)
				{
					temp.LoadString(IDS_REPOBROWSE_REFRESH);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPREFRESH, temp);		// "Refresh"
				}
				else
				{
					temp.LoadString(IDS_REPOBROWSE_SAVEAS);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSAVEAS, temp);		// "Save as..."
				}

				if (GetRevision().IsHead())
				{
					popup.AppendMenu(MF_SEPARATOR);

					if (bFolder)
					{
						temp.LoadString(IDS_REPOBROWSE_MKDIR);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPMKDIR, temp);	// "create directory"

						temp.LoadString(IDS_REPOBROWSE_IMPORT);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPIMPORT, temp);	// "Add/Import File"

						temp.LoadString(IDS_REPOBROWSE_IMPORTFOLDER);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPIMPORTFOLDER, temp);	// "Add/Import Folder"
					}

					temp.LoadString(IDS_REPOBROWSE_DELETE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPDELETE, temp);		// "Remove"

					temp.LoadString(IDS_REPOBROWSE_RENAME);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPRENAME, temp);		// "Rename"

				} // if (GetRevision().IsHead()
				else
				{
					temp.LoadString(IDS_REPOBROWSE_COPYTOWC);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCOPYTOWC, temp);		// "Copy To Working Copy..."
				}
				temp.LoadString(IDS_REPOBROWSE_COPY);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCOPYTO, temp);			// "Copy To..."
				temp.LoadString(IDS_REPOBROWSE_SHOWPROP);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPPROPS, temp);			// "Show Properties"
			} // if (uSelCount == 1)
			if (uSelCount == 2)
			{
				hSelItem1 = m_treeRepository.GetItemHandle(m_treeRepository.GetFirstSelectedItem());
				hSelItem2 = m_treeRepository.GetItemHandle(m_treeRepository.GetNextSelectedItem(m_treeRepository.GetItemIndex(hSelItem1)));

				url1 = m_treeRepository.MakeUrl(hSelItem1);
				url2 = m_treeRepository.MakeUrl(hSelItem2);

				bFolder1 = TRUE;
				bFolder2 = TRUE;

				RVITEM Item;
				Item.nMask = RVIM_IMAGE;
				Item.iItem = m_treeRepository.GetItemIndex(hSelItem1);
				m_treeRepository.GetItem(&Item);
				if (Item.iImage != m_treeRepository.m_nIconFolder)
					bFolder1 = FALSE;
				Item.iItem = m_treeRepository.GetItemIndex(hSelItem2);
				m_treeRepository.GetItem(&Item);
				if (Item.iImage != m_treeRepository.m_nIconFolder)
					bFolder2 = FALSE;

				if (bFolder1 == bFolder2)
				{
					temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPGNUDIFF, temp);
				} // if (bFolder1 == bFolder2)	
				if (!bFolder1 && !bFolder2)
				{
					temp.LoadString(IDS_LOG_POPUP_DIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPDIFF, temp);
				} // if (!bFolder1 && !bFolder2) 
			} // if (uSelCount == 2) 
			if (uSelCount >= 2)
			{
				temp.LoadString(IDS_REPOBROWSE_DELETE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPDELETE, temp);		// "Remove"
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, this, 0);
			GetDlgItem(IDOK)->EnableWindow(FALSE);

			switch (cmd)
			{
			case ID_POPSAVEAS:
				{
					OPENFILENAME ofn;		// common dialog box structure
					TCHAR szFile[MAX_PATH];  // buffer for file name
					ZeroMemory(szFile, sizeof(szFile));
					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					//ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
					ofn.hwndOwner = this->m_hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
					CString temp;
					temp.LoadString(IDS_REPOBROWSE_SAVEAS);
					CUtils::RemoveAccelerators(temp);
					if (temp.IsEmpty())
						ofn.lpstrTitle = NULL;
					else
						ofn.lpstrTitle = temp;
					ofn.Flags = OFN_OVERWRITEPROMPT;

					CString sFilter;
					sFilter.LoadString(IDS_COMMONFILEFILTER);
					TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
					_tcscpy (pszFilters, sFilter);
					// Replace '|' delimeters with '\0's
					TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
					while (ptr != pszFilters)
					{
						if (*ptr == '|')
							*ptr = '\0';
						ptr--;
					} // while (ptr != pszFilters) 
					ofn.lpstrFilter = pszFilters;
					ofn.nFilterIndex = 1;
					// Display the Open dialog box. 
					CString tempfile;
					if (GetSaveFileName(&ofn)==TRUE)
					{
						CWaitCursorEx wait_cursor;
						tempfile = CString(ofn.lpstrFile);
						SVN svn;
						svn.m_app = &theApp;
						if (!svn.Cat(url, GetRevision(), tempfile))
						{
							delete [] pszFilters;
							wait_cursor.Hide();
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						} // if (!svn.Cat(url, GetRevision(), tempfile)) 
					} // if (GetSaveFileName(&ofn)==TRUE) 
					delete [] pszFilters;
				}
				break;
			case ID_POPSHOWLOG:
				{
					CLogDlg dlg;
					long revend = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
					revend = -revend;
					dlg.SetParams(url, GetRevision(), revend, TRUE);
					dlg.m_ProjectProperties = m_ProjectProperties;
					dlg.DoModal();
				}
				break;
			case ID_POPOPEN:
				{
					ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
				}
				break;
			case ID_POPDELETE:
				{
					DeleteSelectedEntries();
					*pResult = 1; // mark HTREEITEM as deleted
				}
				break;
			case ID_POPIMPORTFOLDER:
				{
					CString path;
					CBrowseFolder folderBrowser;
					folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
					if (folderBrowser.Show(GetSafeHwnd(), path)==CBrowseFolder::OK)
					{
						SVN svn;
						svn.m_app = &theApp;
						CWaitCursorEx wait_cursor;
						CString filename = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						CUtils::RemoveAccelerators(input.m_sHintText);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						CUtils::RemoveAccelerators(input.m_sTitle);
						input.m_sInputText.LoadString(IDS_INPUT_ADDFOLDERLOGMSG);
						CUtils::RemoveAccelerators(input.m_sInputText);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Import(path, url+_T("/")+filename, input.m_sInputText, FALSE))
							{
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.Import(path, url, _T("adding file remotely"), FALSE)) 
							m_treeRepository.AddFile(url+_T("/")+filename);
						} // if (input.DoModal() == IDOK) 
					} // if (GetOpenFileName(&ofn)==TRUE) 
				}
				break;
			case ID_POPIMPORT:
				{
					OPENFILENAME ofn;		// common dialog box structure
					TCHAR szFile[MAX_PATH];  // buffer for file name
					ZeroMemory(szFile, sizeof(szFile));
					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					//ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
					ofn.hwndOwner = this->m_hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
					CString sFilter;
					sFilter.LoadString(IDS_COMMONFILEFILTER);
					TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
					_tcscpy (pszFilters, sFilter);
					// Replace '|' delimeters with '\0's
					TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
					while (ptr != pszFilters)
					{
						if (*ptr == '|')
							*ptr = '\0';
						ptr--;
					} // while (ptr != pszFilters) 
					ofn.lpstrFilter = pszFilters;
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					CString temp;
					temp.LoadString(IDS_REPOBROWSE_IMPORT);
					CUtils::RemoveAccelerators(temp);
					ofn.lpstrTitle = temp;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

					// Display the Open dialog box. 

					if (GetOpenFileName(&ofn)==TRUE)
					{
						CString path = ofn.lpstrFile;
						SVN svn;
						svn.m_app = &theApp;
						CWaitCursorEx wait_cursor;
						CString filename = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						CUtils::RemoveAccelerators(input.m_sHintText);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						CUtils::RemoveAccelerators(input.m_sTitle);
						input.m_sInputText.LoadString(IDS_INPUT_ADDLOGMSG);
						CUtils::RemoveAccelerators(input.m_sInputText);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Import(path, url+_T("/")+filename, input.m_sInputText, FALSE))
							{
								delete [] pszFilters;
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.Import(path, url, _T("adding file remotely"), FALSE)) 
							m_treeRepository.AddFile(url+_T("/")+filename);
						} // if (input.DoModal() == IDOK) 
					} // if (GetOpenFileName(&ofn)==TRUE) 
					delete [] pszFilters;
				}
				break;
			case ID_POPRENAME:
				{
					CRenameDlg dlg;
					CString filename = url.Right(url.GetLength() - url.ReverseFind('\\') - 1);
					CString filepath = url.Left(url.ReverseFind('\\') + 1);
					dlg.m_name = filename;
					if (dlg.DoModal() == IDOK)
					{
						filepath =  filepath + dlg.m_name;
						SVN svn;
						svn.m_app = &theApp;
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						CUtils::RemoveAccelerators(input.m_sHintText);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						CUtils::RemoveAccelerators(input.m_sTitle);
						input.m_sInputText.LoadString(IDS_INPUT_MOVELOGMSG);
						CUtils::RemoveAccelerators(input.m_sInputText);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Move(url, filepath, TRUE, input.m_sInputText))
							{
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.Move(url, filepath, TRUE)) 
							m_treeRepository.Refresh(hSelItem);
						} // if (input.DoModal() == IDOK) 
					} // if (dlg.DoModal() == IDOK) 
				}
				break;
			case ID_POPCOPYTO:
				{
					CRenameDlg dlg;
					dlg.m_name = url;
					dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_COPY);
					CUtils::RemoveAccelerators(dlg.m_windowtitle);
					if (dlg.DoModal() == IDOK)
					{
						SVN svn;
						svn.m_app = &theApp;
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						CUtils::RemoveAccelerators(input.m_sHintText);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						CUtils::RemoveAccelerators(input.m_sTitle);
						input.m_sInputText.LoadString(IDS_INPUT_COPYLOGMSG);
						CUtils::RemoveAccelerators(input.m_sInputText);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Copy(url, dlg.m_name, GetRevision(), input.m_sInputText))
							{
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							}
							if (GetRevision().IsHead())
							{
								if (bFolder)
									m_treeRepository.AddFolder(dlg.m_name);
								else
									m_treeRepository.AddFile(dlg.m_name);
							}
						} // if (input.DoModal() == IDOK) 
					} // if (dlg.DoModal() == IDOK) 
				}
				break;
			case ID_POPCOPYTOWC:
				{
					OPENFILENAME ofn;		// common dialog box structure
					TCHAR szFile[MAX_PATH];  // buffer for file name
					ZeroMemory(szFile, sizeof(szFile));
					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					//ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
					ofn.hwndOwner = this->m_hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
					CString temp;
					temp.LoadString(IDS_REPOBROWSE_SAVEAS);
					CUtils::RemoveAccelerators(temp);
					if (temp.IsEmpty())
						ofn.lpstrTitle = NULL;
					else
						ofn.lpstrTitle = temp;
					ofn.Flags = OFN_OVERWRITEPROMPT;

					CString sFilter;
					sFilter.LoadString(IDS_COMMONFILEFILTER);
					TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
					_tcscpy (pszFilters, sFilter);
					// Replace '|' delimeters with '\0's
					TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
					while (ptr != pszFilters)
					{
						if (*ptr == '|')
							*ptr = '\0';
						ptr--;
					} // while (ptr != pszFilters) 
					ofn.lpstrFilter = pszFilters;
					ofn.nFilterIndex = 1;
					// Display the Open dialog box. 
					CString tempfile;
					if (GetSaveFileName(&ofn)==TRUE)
					{
						CWaitCursorEx wait_cursor;
						tempfile = CString(ofn.lpstrFile);
						SVN svn;
						svn.m_app = &theApp;
						if (!svn.Copy(url, tempfile, GetRevision()))
						{
							delete [] pszFilters;
							wait_cursor.Hide();
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						} 
					} // if (GetSaveFileName(&ofn)==TRUE) 
					delete [] pszFilters;
				}
				break;
			case ID_POPMKDIR:
				{
					CRenameDlg dlg;
					dlg.m_name = _T("");
					dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_MKDIR);
					CUtils::RemoveAccelerators(dlg.m_windowtitle);
					if (dlg.DoModal() == IDOK)
					{
						SVN svn;
						svn.m_app = &theApp;
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						CUtils::RemoveAccelerators(input.m_sHintText);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						CUtils::RemoveAccelerators(input.m_sTitle);
						input.m_sInputText.LoadString(IDS_INPUT_MKDIRLOGMSG);
						CUtils::RemoveAccelerators(input.m_sInputText);
						if (input.DoModal() == IDOK)
						{
							if (!svn.MakeDir(url+_T("/")+dlg.m_name, input.m_sInputText))
							{
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.MakeDir(url+_T("/")+dlg.m_name, _T("created directory remotely"))) 
							m_treeRepository.AddFolder(url+_T("/")+dlg.m_name);
						} // if (input.DoModal() == IDOK) 
					} // if (dlg.DoModal() == IDOK) 
				}
				break;
			case ID_POPREFRESH:
				{
					m_treeRepository.RefreshMe(hSelItem);
				}
				break;
			case ID_POPGNUDIFF:
				{
					CString tempfile = CUtils::GetTempFile();
					tempfile += _T(".diff");
					SVN svn;
					if (!svn.Diff(url1, GetRevision(), url2, GetRevision(), TRUE, FALSE, TRUE, _T(""), tempfile))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						DeleteFile(tempfile);
						break;		//exit
					} // if (!Diff(url1, GetRevision(), url2, GetRevision(), TRUE, FALSE, TRUE, _T(""), tempfile)) 
					else
					{
						m_templist.Add(tempfile);
						CUtils::StartDiffViewer(tempfile);
					}
					theApp.DoWaitCursor(-1); //???
				}
				break;
			case ID_POPDIFF:
				{
					CString tempfile1 = CUtils::GetTempFile();
					if (url1.ReverseFind('.')>=url1.ReverseFind('/'))
						tempfile1 += url1.Mid(url1.ReverseFind('.'));
					SVN svn;
					if (!svn.Cat(url1, GetRevision(), tempfile1))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						DeleteFile(tempfile1);
						break;		//exit
					} // if (!Cat(url1, GetRevision(), tempfile1))
					m_templist.Add(tempfile1);
					CString tempfile2 = CUtils::GetTempFile();
					if (url2.ReverseFind('.')>=url2.ReverseFind('/'))
						tempfile2 += url2.Mid(url2.ReverseFind('.'));
					if (!svn.Cat(url2, GetRevision(), tempfile2))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						DeleteFile(tempfile2);
						break;		//exit
					} // if (!Cat(url2, GetRevision(), tempfile2)) 
					CString ext = CUtils::GetFileExtFromPath(url1);
					CUtils::StartDiffViewer(tempfile2, tempfile1, FALSE, url1, url2, ext);
					theApp.DoWaitCursor(-1); //???
				}
				break;
			case ID_POPPROPS:
				{
					CPropDlg dlg;
					dlg.m_rev = GetRevision();
					dlg.m_sPath = url;
					dlg.DoModal();
				}
				break;
			case ID_POPBLAME:
				{
					CBlameDlg dlg;
					if (dlg.DoModal() == IDOK)
					{
						CBlame blame;
						CString tempfile;
						CString logfile;
						tempfile = blame.BlameToTempFile(url, dlg.StartRev, dlg.EndRev, logfile, TRUE);
						if (!tempfile.IsEmpty())
						{
							if (logfile.IsEmpty())
							{
								//open the default text editor for the result file
								CUtils::StartTextViewer(tempfile);
							}
							else
							{
								TCHAR tblame[MAX_PATH];
								GetModuleFileName(NULL, tblame, MAX_PATH);
								CString viewer = tblame;
								viewer.Replace(_T("TortoiseProc.exe"), _T("TortoiseBlame.exe"));
								viewer += _T(" \"") + tempfile + _T("\"");
								viewer += _T(" \"") + logfile + _T("\"");
								viewer += _T(" \"") + CUtils::GetFileNameFromPath(url) + _T("\"");
								STARTUPINFO startup;
								PROCESS_INFORMATION process;
								memset(&startup, 0, sizeof(startup));
								startup.cb = sizeof(startup);
								memset(&process, 0, sizeof(process));

								if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)viewer), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
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
									CString strMessage;
									strMessage.Format(IDS_ERR_EXTDIFFSTART, lpMsgBuf);
									CMessageBox::Show(NULL, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
									LocalFree( lpMsgBuf );
									break;
								} 
							}
						} // if (!tempfile.IsEmpty()) 
						else
						{
							CMessageBox::Show(this->m_hWnd, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
					} // if (dlg.DoModal() == IDOK) 
					
				}
			default:
				break;
			}
			GetDlgItem(IDOK)->EnableWindow(TRUE);
		} // if (popup.CreatePopupMenu()) 
	} // if (hSelItem) 
}

void CRepositoryBrowser::OnRVNKeyDownReposTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMREPORTVIEW pNMTreeView = reinterpret_cast<LPNMREPORTVIEW>(pNMHDR);

	*pResult = 0;
	HTREEITEM hSelItem = m_treeRepository.GetItemHandle(m_treeRepository.GetFirstSelectedItem());
	UINT uSelCount = m_treeRepository.GetSelectedCount();

	switch (pNMTreeView->nKeys)
	{
	case VK_F5:
		if (hSelItem && uSelCount == 1)
		{
			m_treeRepository.RefreshMe(hSelItem);
		}
		*pResult = 1;
		break;
	case VK_DELETE:
		DeleteSelectedEntries();
		*pResult = 1;
		break;	
	case VK_RETURN:
	{
		if (hSelItem && uSelCount == 1)
		{
			ShellExecute(NULL, _T("open"), m_treeRepository.MakeUrl(hSelItem), NULL, NULL, SW_SHOWNORMAL);
		}
	}
	break;
	default:
		break;
	}
}

void CRepositoryBrowser::OnOK()
{
	m_barRepository.SaveHistory();
	CResizableDialog::OnOK();
}

void CRepositoryBrowser::OnBnClickedHelp()
{
	OnHelp();
}

void CRepositoryBrowser::DeleteSelectedEntries()
{
	SVN svn;
	svn.m_app = &theApp;
	CWaitCursorEx wait_cursor;
	CInputDlg dlg(this);
	dlg.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
	CUtils::RemoveAccelerators(dlg.m_sHintText);
	dlg.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
	CUtils::RemoveAccelerators(dlg.m_sTitle);
	dlg.m_sInputText.LoadString(IDS_INPUT_REMOVELOGMSG);
	CUtils::RemoveAccelerators(dlg.m_sInputText);
	if (dlg.DoModal()==IDOK)
	{
		int selItem = m_treeRepository.GetFirstSelectedItem();
		CString url;
		do
		{
			url += m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(selItem));
			url += _T("*");
			selItem = m_treeRepository.GetNextSelectedItem(selItem);
		} while (selItem != RVI_INVALID);
		url = url.Left(url.GetLength()-1);		//remove the trailing '*'
		if (!svn.Remove(url, TRUE, dlg.m_sInputText))
		{
			wait_cursor.Hide();
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		} // if (!svn.Remove(url, TRUE)) 
		while (url.Find('*')>=0)
		{
			m_treeRepository.DeleteUrl(url.Left(url.Find('*')));
			url = url.Mid(url.Find('*')+1);
		}
		m_treeRepository.DeleteUrl(url);
	} // if (dlg.DoModal()==IDOK) 
}
