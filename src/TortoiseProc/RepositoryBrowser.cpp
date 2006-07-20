// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "BrowseFolder.h"
#include "SVNDiff.h"

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
#define ID_BREAKLOCK		21

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
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REPOS_TREE, m_treeRepository);
}


BEGIN_MESSAGE_MAP(CRepositoryBrowser, CResizableStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_FILESDROPPED, OnFilesDropped)
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

	bool bSortNumerical = !!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\SortNumerical"), TRUE);
	m_treeRepository.SortNumerical(bSortNumerical);
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
	m_bThreadRunning = true;
	if (AfxBeginThread(InitThreadEntry, this)==NULL)
	{
		m_bThreadRunning = false;
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
	// force the cursor to change
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	DialogEnableWindow(IDOK, FALSE);
	DialogEnableWindow(IDCANCEL, FALSE);
	m_treeRepository.m_strReposRoot = m_treeRepository.m_svn.GetRepositoryRoot(CTSVNPath(m_InitialSvnUrl.GetPath()));
	m_treeRepository.m_strReposRoot = SVNUrl::Unescape(m_treeRepository.m_strReposRoot);
	PostMessage(WM_AFTERINIT);
	DialogEnableWindow(IDOK, TRUE);
	DialogEnableWindow(IDCANCEL, TRUE);
	
	m_bThreadRunning = false;

	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return 0;
}

LRESULT CRepositoryBrowser::OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_InitialSvnUrl.GetPath().IsEmpty())
		m_InitialSvnUrl = m_barRepository.GetCurrentUrl();

	m_treeRepository.ChangeToUrl(m_InitialSvnUrl);
	m_barRepository.GotoUrl(m_InitialSvnUrl);
	m_treeRepository.m_pProjectProperties = &m_ProjectProperties;
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
	CRect rect;
	m_treeRepository.GetHeaderCtrl()->GetWindowRect(rect);
	if (rect.PtInRect(pt))
	{
		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			CString temp(MAKEINTRESOURCE(IDS_REPOBROWSE_SORTNUMERICAL));
			UINT flags = MF_STRING | MF_ENABLED;
			if (m_treeRepository.m_bSortNumerical)
			{
				flags |= MF_CHECKED;
			}
			popup.AppendMenu(flags, 1, temp);
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, this, 0);
			if (cmd == 1)
			{
				m_treeRepository.m_bSortNumerical = !m_treeRepository.m_bSortNumerical;
				m_treeRepository.GetHeaderCtrl()->SetSortColumn(0, TRUE);
				m_treeRepository.ResortItems();
			}
		}
		return;
	}
	HTREEITEM hSelItem = m_treeRepository.GetItemHandle(m_treeRepository.GetFirstSelectedItem());
	UINT uSelCount = m_treeRepository.GetSelectedCount();
	CString url = m_treeRepository.MakeUrl(hSelItem);
	CString itemtext = m_treeRepository.GetItemText(m_treeRepository.GetItemIndex(hSelItem), 0);
	if (itemtext.Left(8).Compare(_T("Error * "))==0)
	{
		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			CString temp(MAKEINTRESOURCE(IDS_REPOBROWSE_COPYERROR));
			popup.AppendMenu(MF_STRING | MF_ENABLED, 1, temp);
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, this, 0);
			if (cmd == 1)
				CStringUtils::WriteAsciiStringToClipboard(CStringA(itemtext));
		}
		return;
	}

	HTREEITEM hSelItem1;
	HTREEITEM hSelItem2;

	CTSVNPath url1;
	CTSVNPath url2;

	BOOL bFolder1 = FALSE;
	BOOL bFolder2 = FALSE;
	BOOL hasFolders = FALSE;
	BOOL bLocked = FALSE;

	int si = m_treeRepository.GetFirstSelectedItem();
	if (si == RVI_INVALID)
		return;
	do
	{
		if (m_treeRepository.IsFolder(m_treeRepository.GetItemHandle(si)))
			hasFolders = TRUE;
		if (!m_treeRepository.GetItemText(si, 6).IsEmpty())
			bLocked = TRUE;
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
				if (url.Compare(m_treeRepository.m_strReposRoot)!=0)
				{
					temp.LoadString(IDS_MENUREVISIONGRAPH);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVGRAPH, temp);
				}
				if (!bFolder)
				{
					temp.LoadString(IDS_MENUBLAME);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPBLAME, temp);		// "Blame..."
				}
				if (bFolder)
				{
					temp.LoadString(IDS_MENUEXPORT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPEXPORT, temp);		// "Export"
				}
			}
			if (bFolder)
			{
				temp.LoadString(IDS_MENUCHECKOUT);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCHECKOUT, temp);		// "Checkout.."
			}
			if (uSelCount == 1)
			{
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
					else if (bLocked)
					{
						temp.LoadString(IDS_MENU_UNLOCKFORCE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BREAKLOCK, temp);	// "Break Lock"
					}

					temp.LoadString(IDS_REPOBROWSE_DELETE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPDELETE, temp);		// "Remove"

					temp.LoadString(IDS_REPOBROWSE_RENAME);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPRENAME, temp);		// "Rename"

					temp.LoadString(IDS_REPOBROWSE_COPYTOWC);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCOPYTOWC, temp);		// "Copy To Working Copy..."
				} // if (GetRevision().IsHead()

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
					temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPDIFF, temp);
				}
				temp.LoadString(IDS_MENULOG);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSHOWLOG, temp);
			} // if (uSelCount == 2) 
			if (uSelCount >= 2)
			{
				if (GetRevision().IsHead())
				{
					temp.LoadString(IDS_REPOBROWSE_DELETE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPDELETE, temp);		// "Remove"
				}
				if (!hasFolders)
				{
					temp.LoadString(IDS_REPOBROWSE_SAVEAS);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSAVEAS, temp);		// "Save as..."
					temp.LoadString(IDS_REPOBROWSE_COPYTOWC);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCOPYTOWC, temp);		// "Copy To Working Copy..."
				}
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, this, 0);
			DialogEnableWindow(IDOK, FALSE);
			bool bOpenWith = false;
			switch (cmd)
			{
			case ID_POPURLTOCLIPBOARD:
				{
					CStringUtils::WriteAsciiStringToClipboard(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(url)));
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
						_tcscpy_s(szFile, MAX_PATH, filename);
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(OPENFILENAME));
						ofn.lStructSize = sizeof(OPENFILENAME);
						//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
						ofn.hwndOwner = this->m_hWnd;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
						CString temp;
						temp.LoadString(IDS_REPOBROWSE_SAVEAS);
						CStringUtils::RemoveAccelerators(temp);
						if (temp.IsEmpty())
							ofn.lpstrTitle = NULL;
						else
							ofn.lpstrTitle = temp;
						ofn.Flags = OFN_OVERWRITEPROMPT;

						CString sFilter;
						sFilter.LoadString(IDS_COMMONFILEFILTER);
						TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
						_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
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
						int counter = 0;		// the file counter
						progDlg.SetTitle(IDS_REPOBROWSE_SAVEASPROGTITLE);
						progDlg.ShowModeless(GetSafeHwnd());
						progDlg.SetProgress((DWORD)0, (DWORD)selcount);
						svn.SetAndClearProgressInfo(&progDlg);
						do
						{
							saveurl = m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(si));
							CTSVNPath savepath = tempfile;
							if (tempfile.IsDirectory())
								savepath.AppendPathString(saveurl.Mid(saveurl.ReverseFind('/')));
							CString sInfoLine;
							sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, saveurl, (LONG)GetRevision());
							progDlg.SetLine(1, sInfoLine);
							if (!svn.Cat(CTSVNPath(saveurl), GetRevision(), GetRevision(), savepath)||(progDlg.HasUserCancelled()))
							{
								wait_cursor.Hide();
								progDlg.Stop();
								svn.SetAndClearProgressInfo((HWND)NULL);
								if (!progDlg.HasUserCancelled())
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							}
							counter++;
							progDlg.SetProgress((DWORD)counter, (DWORD)selcount);
							si = m_treeRepository.GetNextSelectedItem(si);
						} while (si != RVI_INVALID);
						progDlg.Stop();
						svn.SetAndClearProgressInfo((HWND)NULL);
					} // if (GetSaveFileName(&ofn)==TRUE) 
				}
				break;
			case ID_POPSHOWLOG:
				{
					if (uSelCount == 2)
					{
						// get log of first URL
						CString sCopyFrom1, sCopyFrom2;
						LogHelper helper;
						SVNRev rev1 = helper.GetCopyFromRev(url1, sCopyFrom1, GetRevision());
						if (!rev1.IsValid())
						{
							CMessageBox::Show(this->m_hWnd, helper.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						SVNRev rev2 = helper.GetCopyFromRev(url2, sCopyFrom2, GetRevision());
						if (!rev2.IsValid())
						{
							CMessageBox::Show(this->m_hWnd, helper.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;
						}
						if ((sCopyFrom1.IsEmpty())||(sCopyFrom1.Compare(sCopyFrom2)!=0))
						{
							// no common copyfrom URL, so showing a log between
							// the two urls is not possible.
							CMessageBox::Show(m_hWnd, IDS_ERR_NOCOMMONCOPYFROM, IDS_APPNAME, MB_ICONERROR);
							break;							
						}
						if ((LONG)rev1 < (LONG)rev2)
						{
							SVNRev temp = rev1;
							rev1 = rev2;
							rev2 = temp;
						}
						CString sCmd;
						sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /revstart:%ld /revend:%ld"),
							CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), sCopyFrom1, (LONG)rev1, (LONG)rev2);

						ATLTRACE(sCmd);
						if (!m_path.IsUrl())
						{
							sCmd += _T(" /propspath:\"");
							sCmd += m_path.GetWinPathString();
							sCmd += _T("\"");
						}	

						CAppUtils::LaunchApplication(sCmd, NULL, false);
					}
					else
					{
						CString sCmd;
						sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /revstart:%ld"), 
							CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), url, (LONG)GetRevision());

						if (!m_path.IsUrl())
						{
							sCmd += _T(" /propspath:\"");
							sCmd += m_path.GetWinPathString();
							sCmd += _T("\"");
						}	

						CAppUtils::LaunchApplication(sCmd, NULL, false);
					}
				}
				break;
			case ID_POPCHECKOUT:
				{
					int selItem = m_treeRepository.GetFirstSelectedItem();
					CString itemsToCheckout;
					do
					{
						itemsToCheckout += m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(selItem)) + _T("*");
						selItem = m_treeRepository.GetNextSelectedItem(selItem);
					} while (selItem != RVI_INVALID);
					itemsToCheckout.TrimRight('*');
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:checkout /url:\"%s\" /revstart:%ld"), 
						CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), (LPCTSTR)itemsToCheckout, (LONG)GetRevision());

					CAppUtils::LaunchApplication(sCmd, NULL, false);
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
					dlg.SetPath(url);
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
							CString sBrowserUrl = CString(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(url)));
							
							ShellExecute(NULL, _T("open"), sBrowserUrl, NULL, NULL, SW_SHOWNORMAL);
							break;
						}
					}
					CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, CTSVNPath(url), GetRevision());
					CWaitCursorEx wait_cursor;
					SVN svn;
					svn.SetPromptApp(&theApp);
					CProgressDlg progDlg;
					progDlg.SetTitle(IDS_APPNAME);
					CString sInfoLine;
					sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, url, (LONG)GetRevision());
					progDlg.SetLine(1, sInfoLine);
					svn.SetAndClearProgressInfo(&progDlg);
					progDlg.ShowModeless(m_hWnd);
					if (!svn.Cat(CTSVNPath(url), GetRevision(), GetRevision(), tempfile))
					{
						progDlg.Stop();
						svn.SetAndClearProgressInfo((HWND)NULL);
						wait_cursor.Hide();
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;;
					}
					progDlg.Stop();
					svn.SetAndClearProgressInfo((HWND)NULL);
					SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
					if (!bOpenWith)
					{
						int ret = (int)ShellExecute(NULL, _T("open"), tempfile.GetWinPathString(), NULL, NULL, SW_SHOWNORMAL);
						if (ret <= HINSTANCE_ERROR)
							bOpenWith = true;
					}
					if (bOpenWith)
					{
						CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						cmd += tempfile.GetWinPathString();
						CAppUtils::LaunchApplication(cmd, NULL, false);
					}
				}
				break;
			case ID_POPDELETE:
				{
					DeleteSelectedEntries();
					*pResult = 1; // mark HTREEITEM as deleted
				}
				break;
			case ID_BREAKLOCK:
				{
					SVN svn;
					if (!svn.Unlock(CTSVNPathList(CTSVNPath(url)), TRUE))
					{
						CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return;
					}
					m_treeRepository.SetItemText(m_treeRepository.GetItemIndex(hSelItem), 6, _T(""));
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
						if (m_ProjectProperties.sLogTemplate.IsEmpty())
							input.m_sInputText.LoadString(IDS_INPUT_ADDFOLDERLOGMSG);
						if (input.DoModal() == IDOK)
						{
							CProgressDlg progDlg;
							progDlg.SetTitle(IDS_APPNAME);
							CString sInfoLine;
							sInfoLine.Format(IDS_PROGRESSIMPORT, filename);
							progDlg.SetLine(1, sInfoLine);
							svn.SetAndClearProgressInfo(&progDlg);
							progDlg.ShowModeless(m_hWnd);
							if (!svn.Import(svnPath, CTSVNPath(url+_T("/")+filename), input.m_sInputText, FALSE, TRUE))
							{
								progDlg.Stop();
								svn.SetAndClearProgressInfo((HWND)NULL);
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							}
							progDlg.Stop();
							svn.SetAndClearProgressInfo((HWND)NULL);
							m_treeRepository.AddFolder(url+_T("/")+filename);
						}
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
					_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
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
					CStringUtils::RemoveAccelerators(temp);
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
						if (m_ProjectProperties.sLogTemplate.IsEmpty())
							input.m_sInputText.LoadString(IDS_INPUT_ADDLOGMSG);
						if (input.DoModal() == IDOK)
						{
							CProgressDlg progDlg;
							progDlg.SetTitle(IDS_APPNAME);
							CString sInfoLine;
							sInfoLine.Format(IDS_PROGRESSIMPORT, filename);
							progDlg.SetLine(1, sInfoLine);
							svn.SetAndClearProgressInfo(&progDlg);
							progDlg.ShowModeless(m_hWnd);
							if (!svn.Import(path, CTSVNPath(url+_T("/")+filename), input.m_sInputText, FALSE, TRUE))
							{
								progDlg.Stop();
								svn.SetAndClearProgressInfo((HWND)NULL);
								delete [] pszFilters;
								wait_cursor.Hide();
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							}
							progDlg.Stop();
							svn.SetAndClearProgressInfo((HWND)NULL);
							m_treeRepository.AddFile(url+_T("/")+filename);
						}
					} // if (GetOpenFileName(&ofn)==TRUE) 
					delete [] pszFilters;
				}
				break;
			case ID_POPRENAME:
				{
					m_treeRepository.BeginEdit(m_treeRepository.GetItemRow(m_treeRepository.GetItemIndex(hSelItem)), 0, VK_LBUTTON);
				}
				break;
			case ID_POPCOPYTO:
				{
					CRenameDlg dlg;
					dlg.m_name = url;
					dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_COPY);
					CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
					if (dlg.DoModal() == IDOK)
					{
						SVN svn;
						svn.SetPromptApp(&theApp);
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						SetupInputDlg(&input);
						if (m_ProjectProperties.sLogTemplate.IsEmpty())
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
						_tcscpy_s(szFile, MAX_PATH, filename);
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(OPENFILENAME));
						ofn.lStructSize = sizeof(OPENFILENAME);
						//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
						ofn.hwndOwner = this->m_hWnd;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
						CString temp;
						temp.LoadString(IDS_REPOBROWSE_SAVEAS);
						CStringUtils::RemoveAccelerators(temp);
						if (temp.IsEmpty())
							ofn.lpstrTitle = NULL;
						else
							ofn.lpstrTitle = temp;
						ofn.Flags = OFN_OVERWRITEPROMPT;

						CString sFilter;
						sFilter.LoadString(IDS_COMMONFILEFILTER);
						TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
						_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
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
						progDlg.SetTitle(IDS_APPNAME);
						svn.SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);

						int counter = 0;		// the file counter
						do
						{
							saveurl = m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(si));
							CTSVNPath savepath = tempfile;
							if (tempfile.IsDirectory())
								savepath.AppendPathString(saveurl.Mid(saveurl.ReverseFind('/')));
							CString sInfoLine;
							sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, saveurl, (LONG)GetRevision());
							progDlg.SetLine(1, sInfoLine);
							if (!svn.Copy(CTSVNPath(saveurl), savepath, GetRevision())||(progDlg.HasUserCancelled()))
							{
								progDlg.Stop();
								svn.SetAndClearProgressInfo((HWND)NULL);
								wait_cursor.Hide();
								progDlg.Stop();
								if (!progDlg.HasUserCancelled())
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							}
							progDlg.Stop();
							svn.SetAndClearProgressInfo((HWND)NULL);
							counter++;
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
					CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
					if (dlg.DoModal() == IDOK)
					{
						SVN svn;
						svn.SetPromptApp(&theApp);
						CWaitCursorEx wait_cursor;
						CInputDlg input(this);
						SetupInputDlg(&input);
						if (m_ProjectProperties.sLogTemplate.IsEmpty())
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
					SVNDiff diff(NULL, this->m_hWnd, true);
					diff.ShowUnifiedDiff(url1, GetRevision(), url2, GetRevision());
				}
				break;
			case ID_POPDIFF:
				{
					SVNDiff diff(NULL, this->m_hWnd, true);
					diff.ShowCompare(url1, GetRevision(), url2, GetRevision());
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
					dlg.EndRev = GetRevision();
					if (dlg.DoModal() == IDOK)
					{
						CBlame blame;
						CString tempfile;
						CString logfile;
						tempfile = blame.BlameToTempFile(CTSVNPath(url), dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, TRUE);
						if (!tempfile.IsEmpty())
						{
							if (logfile.IsEmpty())
							{
								//open the default text editor for the result file
								CAppUtils::StartTextViewer(tempfile);
							}
							else
							{
								if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(url)))
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
			DialogEnableWindow(IDOK, TRUE);
		} // if (popup.CreatePopupMenu()) 
	} // if (hSelItem) 
}

void CRepositoryBrowser::OnRVNKeyDownReposTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMREPORTVIEW pNMTreeView = reinterpret_cast<LPNMREPORTVIEW>(pNMHDR);

	*pResult = 0;
	int hFirstSelItem = m_treeRepository.GetFirstSelectedItem();
	if (hFirstSelItem == RVI_INVALID)
		return;
	HTREEITEM hSelItem = m_treeRepository.GetItemHandle(hFirstSelItem);
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
		if (GetRevision().IsHead())
			DeleteSelectedEntries();
		*pResult = 1;
		break;	
	case VK_RETURN:
		{
			if (hSelItem && uSelCount == 1)
			{
				ShellExecute(NULL, _T("open"), m_treeRepository.MakeUrl(hSelItem), NULL, NULL, SW_SHOWNORMAL);
				*pResult = 1;
			}
		}
		break;
	case 'C':
	case 'c':
		{
			if ((hSelItem)&&(uSelCount == 1)&&(GetAsyncKeyState(VK_CONTROL)&0x8000))
			{
				CString url = m_treeRepository.GetItemText(m_treeRepository.GetItemIndex(hSelItem), 0);
				if (url.Find('*')>=0)
				{
					// an error message
					CStringUtils::WriteAsciiStringToClipboard(CStringA(url));
					*pResult = 1;
				}
				else
				{
					url = m_treeRepository.MakeUrl(hSelItem);
					CStringUtils::WriteAsciiStringToClipboard(CStringA(url));
					*pResult = 1;
				}
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
	CStringUtils::RemoveAccelerators(dlg.m_sHintText);
	dlg.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
	CStringUtils::RemoveAccelerators(dlg.m_sTitle);
	if (m_ProjectProperties.sLogTemplate.IsEmpty())
		dlg.m_sInputText.LoadString(IDS_INPUT_REMOVELOGMSG);
	CStringUtils::RemoveAccelerators(dlg.m_sInputText);
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
	CStringUtils::RemoveAccelerators(dlg->m_sHintText);
	dlg->m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
	CStringUtils::RemoveAccelerators(dlg->m_sTitle);
	dlg->m_pProjectProperties = &m_ProjectProperties;
	dlg->m_bUseLogWidth = true;
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

LRESULT CRepositoryBrowser::OnFilesDropped(WPARAM wParam, LPARAM lParam)
{
	OnFilesDropped((int)wParam, (int)lParam, m_treeRepository.m_DroppedPaths);
	return 0;
}

void CRepositoryBrowser::OnFilesDropped(int iItem, int iSubItem, const CTSVNPathList& droppedPaths)
{
	if (iSubItem != 0)
		return;		// no subitem drop!

	CString url = m_treeRepository.MakeUrl(m_treeRepository.GetItemHandle(iItem));
	if (url.IsEmpty())
		return;

	if (droppedPaths.GetCount() == 0)
		return;		
	if (droppedPaths.GetCount() > 1)
	{
		if (CMessageBox::Show(m_hWnd, IDS_REPOBROWSE_MULTIIMPORT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)!=IDYES)
			return;
	}

	CInputDlg input(this);
	SetupInputDlg(&input);
	if (m_ProjectProperties.sLogTemplate.IsEmpty())
	{
		input.m_sInputText.LoadString(IDS_INPUT_ADDFILEFOLDERMSG);
		input.m_sInputText += _T("\r\n\r\n");
		for (int i=0; i<droppedPaths.GetCount(); ++i)
		{
			input.m_sInputText += droppedPaths[i].GetWinPathString() + _T("\r\n");
		}
	}
	
	if (input.DoModal() == IDOK)
	{
		SVN svn;
		for (int importindex = 0; importindex<droppedPaths.GetCount(); ++importindex)
		{
			CString filename = droppedPaths[importindex].GetFileOrDirectoryName();
			if (!svn.Import(droppedPaths[importindex], 
				CTSVNPath(url+_T("/")+filename), 
				input.m_sInputText, TRUE, TRUE))
			{
				CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return;
			}
			if (droppedPaths[importindex].IsDirectory())
				m_treeRepository.AddFolder(url+_T("/")+filename);
			else
				m_treeRepository.AddFile(url+_T("/")+filename);				
		}
	}

	return;
}
