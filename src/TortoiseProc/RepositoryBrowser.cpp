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
#include "RevisionGraphDlg.h"
#include "CheckoutDlg.h"
#include "SVNProgressDlg.h"
#include "Utils.h"
#include "UnicodeUtils.h"
#include "BrowseFolder.h"

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
#define ID_REVGRAPH			15
#define ID_POPEXPORT		16
//#define ID_POPPROPS			17		commented out because already defined to 17 in LogDlg.h
#define ID_POPCHECKOUT		18
#define ID_POPOPENWITH		19
#define ID_POPURLTOCLIPBOARD 20
// CRepositoryBrowser dialog

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(const SVNUrl& svn_url, BOOL bFile)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, NULL)
	, m_treeRepository(svn_url.GetPath(), bFile)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_bStandAlone(true)
	, m_InitialSvnUrl(svn_url)
	, m_bInitDone(false)
{
}

CRepositoryBrowser::CRepositoryBrowser(const SVNUrl& svn_url, CWnd* pParent, BOOL bFile)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
	, m_treeRepository(svn_url.GetPath(), bFile)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_InitialSvnUrl(svn_url)
	, m_bStandAlone(false)
	, m_bInitDone(false)
{
}

CRepositoryBrowser::~CRepositoryBrowser()
{
	m_templist.DeleteAllFiles();
	for (int i=0; i<m_arLogDialogs.GetCount(); ++i)
	{
		if (!m_arLogDialogs.GetAt(i)->IsThreadRunning())
			delete m_arLogDialogs.GetAt(i);
	}
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REPOS_TREE, m_treeRepository);
}


BEGIN_MESSAGE_MAP(CRepositoryBrowser, CResizableStandAloneDialog)
	ON_NOTIFY(RVN_ITEMRCLICK, IDC_REPOS_TREE, OnRVNItemRClickReposTree)
	ON_NOTIFY(RVN_ITEMRCLICKUP, IDC_REPOS_TREE, OnRVNItemRClickUpReposTree)
	ON_NOTIFY(RVN_KEYDOWN, IDC_REPOS_TREE, OnRVNKeyDownReposTree)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_REGISTERED_MESSAGE(WM_AFTERINIT, OnAfterInitDialog) 
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

BOOL CRepositoryBrowser::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_cnrRepositoryBar.SubclassDlgItem(IDC_REPOS_BAR_CNR, this);
	m_barRepository.Create(&m_cnrRepositoryBar, 12345);
	m_barRepository.AssocTree(&m_treeRepository);
	m_treeRepository.Init(GetRevision());

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
	if (AfxBeginThread(InitThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//this is the thread function which calls the subversion function
UINT CRepositoryBrowser::InitThreadEntry(LPVOID pVoid)
{
	return ((CRepositoryBrowser*)pVoid)->InitThread();
}


//this is the thread function which calls the subversion function
UINT CRepositoryBrowser::InitThread()
{
	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));
	SVN svn;
	m_treeRepository.m_strReposRoot = svn.GetRepositoryRoot(CTSVNPath(m_InitialSvnUrl.GetPath()));
	m_treeRepository.m_strReposRoot = SVNUrl::Unescape(m_treeRepository.m_strReposRoot);
	svn.GetLocks(CTSVNPath(m_treeRepository.m_strReposRoot), &m_treeRepository.m_locks);
	PostMessage(WM_AFTERINIT);
	return 0;
}

LRESULT CRepositoryBrowser::OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_InitialSvnUrl.GetPath().IsEmpty())
		m_InitialSvnUrl = m_barRepository.GetCurrentUrl();

	m_treeRepository.ChangeToUrl(m_InitialSvnUrl);
	m_barRepository.GotoUrl(m_InitialSvnUrl);
	m_bInitDone = TRUE;
	return 0;
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
		point = rect.CenterPoint();
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

	if (url.Left(8).Compare(_T("Error * "))==0)
		return;

	HTREEITEM hSelItem1;
	HTREEITEM hSelItem2;

	CTSVNPath url1;
	CTSVNPath url2;

	BOOL bFolder1;
	BOOL bFolder2;
	BOOL hasFolders = FALSE;

	int si = m_treeRepository.GetFirstSelectedItem();
	do
	{
		if (m_treeRepository.IsFolder(m_treeRepository.GetItemHandle(si)))
			hasFolders = TRUE;
		si = m_treeRepository.GetNextSelectedItem(si);
	} while (si != RVI_INVALID);

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
				if (!bFolder)
				{
					temp.LoadString(IDS_REPOBROWSE_OPEN);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPOPEN, temp);		// "open"
					temp.LoadString(IDS_LOG_POPUP_OPENWITH);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPOPENWITH, temp);
					popup.AppendMenu(MF_SEPARATOR, NULL);
				}				

				temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSHOWLOG, temp);			// "Show Log..."
				temp.LoadString(IDS_MENUREVISIONGRAPH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVGRAPH, temp);
				if (!bFolder)
				{
					temp.LoadString(IDS_MENUBLAME);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPBLAME, temp);		// "Blame..."
				}

				if (bFolder)
				{
					temp.LoadString(IDS_MENUEXPORT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPEXPORT, temp);		// "Export"
					temp.LoadString(IDS_MENUCHECKOUT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCHECKOUT, temp);		// "Checkout.."
					temp.LoadString(IDS_REPOBROWSE_REFRESH);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPREFRESH, temp);		// "Refresh"
				}
				else
				{
					temp.LoadString(IDS_REPOBROWSE_SAVEAS);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSAVEAS, temp);		// "Save as..."
				}
				popup.AppendMenu(MF_SEPARATOR, NULL);				

				if (GetRevision().IsHead())
				{
					if (bFolder)
					{
						temp.LoadString(IDS_REPOBROWSE_MKDIR);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPMKDIR, temp);	// "create directory"

						temp.LoadString(IDS_REPOBROWSE_IMPORT);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPIMPORT, temp);	// "Add/Import File"

						temp.LoadString(IDS_REPOBROWSE_IMPORTFOLDER);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPIMPORTFOLDER, temp);	// "Add/Import Folder"

						popup.AppendMenu(MF_SEPARATOR, NULL);
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
				
				temp.LoadString(IDS_REPOBROWSE_URLTOCLIPBOARD);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPURLTOCLIPBOARD, temp);			// "Copy URL to clipboard"

				popup.AppendMenu(MF_SEPARATOR, NULL);
				
				temp.LoadString(IDS_REPOBROWSE_SHOWPROP);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPPROPS, temp);			// "Show Properties"
			} // if (uSelCount == 1)
			if (uSelCount == 2)
			{
				hSelItem1 = m_treeRepository.GetItemHandle(m_treeRepository.GetFirstSelectedItem());
				hSelItem2 = m_treeRepository.GetItemHandle(m_treeRepository.GetNextSelectedItem(m_treeRepository.GetItemIndex(hSelItem1)));

				url1.SetFromUnknown(m_treeRepository.MakeUrl(hSelItem1));
				url2.SetFromUnknown(m_treeRepository.MakeUrl(hSelItem2));

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
				if (!hasFolders)
				{
					temp.LoadString(IDS_REPOBROWSE_SAVEAS);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSAVEAS, temp);		// "Save as..."
					temp.LoadString(IDS_REPOBROWSE_COPYTOWC);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCOPYTOWC, temp);		// "Copy To Working Copy..."
				}
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, this, 0);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			bool bOpenWith = false;
			switch (cmd)
			{
			case ID_POPURLTOCLIPBOARD:
				{
					CUtils::WriteAsciiStringToClipboard(CUtils::PathEscape(CUnicodeUtils::GetUTF8(url)));
				}
				break;
			case ID_POPSAVEAS:
				{
					bool bSavePathOK = false;
					CTSVNPath tempfile;
					if (uSelCount == 1)
					{
						OPENFILENAME ofn;		// common dialog box structure
						TCHAR szFile[MAX_PATH];  // buffer for file name
						ZeroMemory(szFile, sizeof(szFile));
						CString filename = url.Mid(url.ReverseFind('/')+1);
						_tcscpy(szFile, filename);
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(OPENFILENAME));
						ofn.lStructSize = sizeof(OPENFILENAME);
						//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
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
						bSavePathOK = (GetSaveFileName(&ofn)==TRUE);
						if (bSavePathOK)
							tempfile.SetFromWin(ofn.lpstrFile);
						delete [] pszFilters;
					} // if (uSelCount == 1)
					else
					{
						CBrowseFolder browser;
						CString sTempfile;
						browser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
						browser.Show(GetSafeHwnd(), sTempfile);
						if (!sTempfile.IsEmpty())
						{
							bSavePathOK = true;
							tempfile.SetFromWin(sTempfile);
						}
					}
					if (bSavePathOK)
					{
						CWaitCursorEx wait_cursor;
						SVN svn;
						svn.SetPromptApp(&theApp);

						int si = m_treeRepository.GetFirstSelectedItem();
						CString saveurl;
						CProgressDlg progDlg;
						int selcount = m_treeRepository.GetSelectedCount();
						if (selcount > 1)
						{
							progDlg.SetTitle(IDS_REPOBROWSE_SAVEASPROGTITLE);
							progDlg.SetShowProgressBar(true);
							progDlg.ShowModeless(GetSafeHwnd());
							progDlg.SetProgress((DWORD)0, (DWORD)selcount);
						}
						int counter = 0;		// the file counter
						do
						{
							saveurl = m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(si));
							CTSVNPath savepath = tempfile;
							if (tempfile.IsDirectory())
								savepath.AppendPathString(saveurl.Mid(saveurl.ReverseFind('/')));
							progDlg.FormatPathLine(1, IDS_REPOBROWSE_SAVEAS_LINE1, saveurl);
							progDlg.FormatPathLine(2, IDS_REPOBROWSE_SAVEAS_LINE2, savepath.GetWinPathString());
							if (!svn.Cat(CTSVNPath(saveurl), GetRevision(), savepath)||(progDlg.HasUserCancelled()))
							{
								wait_cursor.Hide();
								progDlg.Stop();
								if (!progDlg.HasUserCancelled())
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							}
							counter++;
							progDlg.SetProgress((DWORD)counter, (DWORD)selcount);
							si = m_treeRepository.GetNextSelectedItem(si);
						} while (si != RVI_INVALID);
						progDlg.Stop();
					} // if (GetSaveFileName(&ofn)==TRUE) 
				}
				break;
			case ID_POPSHOWLOG:
				{
					CLogDlg * pDlg = new CLogDlg();
					m_arLogDialogs.Add(pDlg);
					int limit = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
					pDlg->SetParams(CTSVNPath(url), GetRevision(), 1, limit, FALSE);
					pDlg->m_ProjectProperties = m_ProjectProperties;
					pDlg->Create(IDD_LOGMESSAGE, NULL);
					pDlg->ShowWindow(SW_SHOW);
				}
				break;
			case ID_POPCHECKOUT:
				{
					CCheckoutDlg dlg;
					dlg.m_URL = url;
					dlg.Revision = GetRevision();
					if (dlg.DoModal()==IDOK)
					{
						CTSVNPath checkoutDirectory;
						checkoutDirectory.SetFromWin(dlg.m_strCheckoutDirectory, true);

						CSVNProgressDlg progDlg;
						int opts = dlg.m_bNonRecursive ? ProgOptNonRecursive : ProgOptRecursive;
						if (dlg.m_bNoExternals)
							opts |= ProgOptIgnoreExternals;
						progDlg.SetParams(CSVNProgressDlg::Checkout, opts, CTSVNPathList(checkoutDirectory), dlg.m_URL, _T(""), dlg.Revision);
						progDlg.DoModal();
					}
				}
				break;
			case ID_POPEXPORT:
				{
					CCheckoutDlg dlg;
					dlg.IsExport = TRUE;
					dlg.m_URL = url;
					dlg.Revision = GetRevision();
					if (dlg.DoModal()==IDOK)
					{
						CTSVNPath checkoutDirectory;
						checkoutDirectory.SetFromWin(dlg.m_strCheckoutDirectory, true);

						CSVNProgressDlg progDlg;
						int opts = dlg.m_bNonRecursive ? ProgOptNonRecursive : ProgOptRecursive;
						if (dlg.m_bNoExternals)
							opts |= ProgOptIgnoreExternals;
						progDlg.SetParams(CSVNProgressDlg::Export, opts, CTSVNPathList(checkoutDirectory), dlg.m_URL, _T(""), dlg.Revision);
						progDlg.DoModal();
					}
				}
				break;
			case ID_REVGRAPH:
				{
					CRevisionGraphDlg dlg;
					dlg.m_sPath = url;
					dlg.DoModal();
				}
				break;
			case ID_POPOPENWITH:
				bOpenWith = true;
			case ID_POPOPEN:
				{
					if (GetRevision().IsHead() && (bOpenWith==false))
					{
						if (url.Left(4).CompareNoCase(_T("http")) == 0)
						{
							CString sBrowserUrl = CString(CUtils::PathEscape(CUnicodeUtils::GetUTF8(url)));
							
							ShellExecute(NULL, _T("open"), sBrowserUrl, NULL, NULL, SW_SHOWNORMAL);
							break;
						}
					}
					CTSVNPath tempfile = CUtils::GetTempFilePath(CTSVNPath(url));
					CWaitCursorEx wait_cursor;
					SVN svn;
					svn.SetPromptApp(&theApp);
					if (!svn.Cat(CTSVNPath(url), GetRevision(), tempfile))
					{
						wait_cursor.Hide();
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;;
					}
					if (bOpenWith)
					{
						CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						cmd += tempfile.GetWinPathString();
						CUtils::LaunchApplication(cmd, NULL, false);
					}
					else
						ShellExecute(NULL, _T("open"), tempfile.GetWinPathString(), NULL, NULL, SW_SHOWNORMAL);
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
						svn.SetPromptApp(&theApp);
						CTSVNPath svnPath(path);
						CWaitCursorEx wait_cursor;
						CString filename = svnPath.GetFileOrDirectoryName();
						CInputDlg input(this);
						SetupInputDlg(&input);
						input.m_sInputText.LoadString(IDS_INPUT_ADDFOLDERLOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Import(svnPath, CTSVNPath(url+_T("/")+filename), input.m_sInputText, FALSE))
							{
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.Import(path, url, _T("adding file remotely"), FALSE)) 
							m_treeRepository.AddFolder(url+_T("/")+filename);
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
					ofn.lStructSize = sizeof(OPENFILENAME);
					//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
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
						CTSVNPath path(ofn.lpstrFile);
						SVN svn;
						svn.SetPromptApp(&theApp);
						CWaitCursorEx wait_cursor;
						CString filename = path.GetFileOrDirectoryName();
						CInputDlg input(this);
						SetupInputDlg(&input);
						input.m_sInputText.LoadString(IDS_INPUT_ADDLOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Import(path, CTSVNPath(url+_T("/")+filename), input.m_sInputText, FALSE))
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
						svn.SetPromptApp(&theApp);
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						SetupInputDlg(&input);
						input.m_sInputText.LoadString(IDS_INPUT_MOVELOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Move(CTSVNPath(url), CTSVNPath(filepath), TRUE, input.m_sInputText))
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
						svn.SetPromptApp(&theApp);
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						SetupInputDlg(&input);
						input.m_sInputText.LoadString(IDS_INPUT_COPYLOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Copy(CTSVNPath(url), CTSVNPath(dlg.m_name), GetRevision(), input.m_sInputText))
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
					bool bSavePathOK = false;
					CTSVNPath tempfile;
					if (uSelCount == 1)
					{
						OPENFILENAME ofn;		// common dialog box structure
						TCHAR szFile[MAX_PATH];  // buffer for file name
						ZeroMemory(szFile, sizeof(szFile));
						CString filename = url.Mid(url.ReverseFind('/')+1);
						_tcscpy(szFile, filename);
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(OPENFILENAME));
						ofn.lStructSize = sizeof(OPENFILENAME);
						//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
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
						bSavePathOK = (GetSaveFileName(&ofn)==TRUE);
						if (bSavePathOK)
							tempfile.SetFromWin(ofn.lpstrFile);
						delete [] pszFilters;
					} // if (uSelCount == 1)
					else
					{
						CBrowseFolder browser;
						CString sTempfile;
						browser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
						browser.Show(GetSafeHwnd(), sTempfile);
						if (!sTempfile.IsEmpty())
						{
							bSavePathOK = true;
							tempfile.SetFromWin(sTempfile);
						}
					}
					if (bSavePathOK)
					{
						CWaitCursorEx wait_cursor;
						SVN svn;
						svn.SetPromptApp(&theApp);

						int si = m_treeRepository.GetFirstSelectedItem();
						CString saveurl;
						CProgressDlg progDlg;
						int selcount = m_treeRepository.GetSelectedCount();
						if (selcount > 1)
						{
							progDlg.SetTitle(IDS_REPOBROWSE_COPYTOWCPROGTITLE);
							progDlg.SetShowProgressBar(true);
							progDlg.ShowModeless(GetSafeHwnd());
							progDlg.SetProgress((DWORD)0, (DWORD)selcount);
						}
						int counter = 0;		// the file counter
						do
						{
							saveurl = m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(si));
							CTSVNPath savepath = tempfile;
							if (tempfile.IsDirectory())
								savepath.AppendPathString(saveurl.Mid(saveurl.ReverseFind('/')));
							progDlg.FormatPathLine(1, IDS_REPOBROWSE_COPYTOWC_LINE1, saveurl);
							progDlg.FormatPathLine(2, IDS_REPOBROWSE_COPYTOWC_LINE2, savepath.GetWinPathString());
							if (!svn.Copy(CTSVNPath(saveurl), savepath, GetRevision())||(progDlg.HasUserCancelled()))
							{
								wait_cursor.Hide();
								progDlg.Stop();
								if (!progDlg.HasUserCancelled())
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							}
							counter++;
							progDlg.SetProgress((DWORD)counter, (DWORD)selcount);
							si = m_treeRepository.GetNextSelectedItem(si);
						} while (si != RVI_INVALID);
						progDlg.Stop();
					} // if (GetSaveFileName(&ofn)==TRUE) 
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
						svn.SetPromptApp(&theApp);
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						SetupInputDlg(&input);
						input.m_sInputText.LoadString(IDS_INPUT_MKDIRLOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.MakeDir(CTSVNPathList(CTSVNPath(url+_T("/")+dlg.m_name)), input.m_sInputText))
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
					CTSVNPath tempfile = CUtils::GetTempFilePath();
					tempfile.AppendRawString(_T(".diff"));
					SVN svn;
					if (!svn.Diff(url1, GetRevision(), url2, GetRevision(), TRUE, FALSE, FALSE, FALSE, _T(""), tempfile))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						::DeleteFile(tempfile.GetWinPath());
						break;		//exit
					}
					else
					{
						m_templist.AddPath(tempfile);
						CUtils::StartUnifiedDiffViewer(tempfile);
					}
				}
				break;
			case ID_POPDIFF:
				{
					CTSVNPath tempfile1 = CUtils::GetTempFilePath(url1);
					SVN svn;
					if (!svn.Cat(url1, GetRevision(), CTSVNPath(tempfile1)))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						::DeleteFile(tempfile1.GetWinPath());
						break;		//exit
					}
					m_templist.AddPath(tempfile1);
					CTSVNPath tempfile2 = CUtils::GetTempFilePath(url2);
					if (!svn.Cat(url2, GetRevision(), CTSVNPath(tempfile2)))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						::DeleteFile(tempfile2.GetWinPath());
						break;		//exit
					}
					CUtils::StartExtDiff(tempfile1, tempfile2, url1.GetUIPathString(), url2.GetUIPathString());	
				}
				break;
			case ID_POPPROPS:
				{
					CPropDlg dlg;
					dlg.m_rev = GetRevision();
					dlg.m_Path = CTSVNPath(url);
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
						tempfile = blame.BlameToTempFile(CTSVNPath(url), dlg.StartRev, dlg.EndRev, logfile, TRUE);
						if (!tempfile.IsEmpty())
						{
							if (logfile.IsEmpty())
							{
								//open the default text editor for the result file
								CUtils::StartTextViewer(tempfile);
							}
							else
							{
								if(!CUtils::LaunchTortoiseBlame(tempfile, logfile, CUtils::GetFileNameFromPath(url)))
								{
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
	CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnBnClickedHelp()
{
	OnHelp();
}

void CRepositoryBrowser::DeleteSelectedEntries()
{
	SVN svn;
	svn.SetPromptApp(&theApp);
	CWaitCursorEx wait_cursor;
	CInputDlg dlg(this);
	dlg.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
	CUtils::RemoveAccelerators(dlg.m_sHintText);
	dlg.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
	CUtils::RemoveAccelerators(dlg.m_sTitle);
	dlg.m_sInputText.LoadString(IDS_INPUT_REMOVELOGMSG);
	CUtils::RemoveAccelerators(dlg.m_sInputText);
	dlg.m_pProjectProperties = &m_ProjectProperties;
	if (dlg.DoModal()==IDOK)
	{
		int selItem = m_treeRepository.GetFirstSelectedItem();
		CTSVNPathList itemsToRemove;
		do
		{
			itemsToRemove.AddPath(CTSVNPath(m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(selItem))));
			selItem = m_treeRepository.GetNextSelectedItem(selItem);
		} while (selItem != RVI_INVALID);
		if (!svn.Remove(itemsToRemove, TRUE, dlg.m_sInputText))
		{
			wait_cursor.Hide();
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		} // if (!svn.Remove(url, TRUE)) 
		for(int nItem = 0; nItem < itemsToRemove.GetCount(); nItem++)
		{
			m_treeRepository.DeleteUrl(itemsToRemove[nItem].GetSVNPathString());
		}
	}
}

void CRepositoryBrowser::SetupInputDlg(CInputDlg * dlg)
{
	dlg->m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
	CUtils::RemoveAccelerators(dlg->m_sHintText);
	dlg->m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
	CUtils::RemoveAccelerators(dlg->m_sTitle);
	dlg->m_pProjectProperties = &m_ProjectProperties;
}

BOOL CRepositoryBrowser::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!m_bInitDone)
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&
			((pWnd == &m_treeRepository)||
			(pWnd == &m_barRepository)||
			(pWnd == &m_cnrRepositoryBar)))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}
