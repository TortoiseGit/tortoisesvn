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
#include "RepositoryBrowser.h"

#include "MessageBox.h"
#include "InputDlg.h"
#include "LogDlg.h"


// CRepositoryBrowser dialog

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableDialog)
CRepositoryBrowser::CRepositoryBrowser(const CString& strUrl, CWnd* pParent /*=NULL*/)
	: CResizableDialog(CRepositoryBrowser::IDD, pParent),
	m_treeRepository(strUrl)
	, m_strUrl(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bStandAlone = FALSE;
}

CRepositoryBrowser::~CRepositoryBrowser()
{
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REPOS_TREE, m_treeRepository);
	DDX_Text(pDX, IDC_URL, m_strUrl);
}


BEGIN_MESSAGE_MAP(CRepositoryBrowser, CResizableDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_REPOS_TREE, OnTvnSelchangedReposTree)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZING()
	ON_NOTIFY(NM_RCLICK, IDC_REPOS_TREE, OnNMRclickReposTree)
END_MESSAGE_MAP()


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

	m_treeRepository.Init();

	if (m_bStandAlone)
	{
		GetDlgItem(IDCANCEL)->ShowWindow(FALSE);
		GetDlgItem(IDC_URL)->ShowWindow(FALSE);
	}

	AddAnchor(IDC_REPOS_TREE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_STATICURL, BOTTOM_LEFT);
	AddAnchor(IDC_URL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRepositoryBrowser::OnTvnSelchangedReposTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	m_strUrl = m_treeRepository.GetFolderUrl(pNMTreeView->itemNew.hItem);
	UpdateData(FALSE);
	*pResult = 0;
}

void CRepositoryBrowser::OnNMRclickReposTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	POINT point;
	GetCursorPos(&point);
	::MapWindowPoints(NULL, m_treeRepository.m_hWnd, &point, 1);
	HTREEITEM hSelItem = m_treeRepository.HitTest(point);
	//HTREEITEM hSelItem = m_treeRepository.GetSelectedItem();
	CString url = m_treeRepository.MakeUrl(hSelItem);
	if (hSelItem)
	{
		BOOL bFolder = TRUE;;
		TVITEM Item;
		Item.mask = TVIF_IMAGE;
		Item.hItem = hSelItem;
		m_treeRepository.GetItem(&Item);
		if (Item.iImage != m_treeRepository.m_nIconFolder)
			bFolder = FALSE;

		CMenu popup;
		POINT point;
		GetCursorPos(&point);
		if (popup.CreatePopupMenu())
		{
			CString temp;
			UINT flags;
			temp.LoadString(IDS_REPOBROWSE_SAVEAS);
			if (bFolder)
				flags = MF_STRING | MF_GRAYED;
			else
				flags = MF_STRING | MF_ENABLED;
			popup.AppendMenu(flags, ID_POPSAVEAS, temp);						// "Save as..."

			temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPSHOWLOG, temp);		// "Show Log..."

			temp.LoadString(IDS_REPOBROWSE_OPEN);
			if ((bFolder)&&(url.Left(4).CompareNoCase(_T("http"))!=0))
				flags = MF_STRING | MF_GRAYED;
			else
				flags = MF_STRING | MF_ENABLED;
			popup.AppendMenu(flags, ID_POPOPEN, temp);							// "open"

			temp.LoadString(IDS_REPOBROWSE_MKDIR);
			if (bFolder)
				flags = MF_STRING | MF_ENABLED;
			else
				flags = MF_STRING | MF_GRAYED;
			popup.AppendMenu(flags, ID_POPMKDIR, temp);							// "create directory"

			temp.LoadString(IDS_REPOBROWSE_DELETE);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPDELETE, temp);		// "Remove"

			temp.LoadString(IDS_REPOBROWSE_IMPORT);
			if (bFolder)
				flags = MF_STRING | MF_ENABLED;
			else
				flags = MF_STRING | MF_GRAYED;
			popup.AppendMenu(flags, ID_POPIMPORT, temp);						// "Add/Import"
			
			temp.LoadString(IDS_REPOBROWSE_RENAME);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPRENAME, temp);		// "Rename"
			
			temp.LoadString(IDS_REPOBROWSE_COPY);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPCOPYTO, temp);		// "Copy To..."

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
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
						svn.m_app = &theApp;
						theApp.DoWaitCursor(1);
						if (!svn.Cat(url, -1, tempfile))
						{
							theApp.DoWaitCursor(-1);
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						} // if (!svn.Cat(url, -1, tempfile)) 
						theApp.DoWaitCursor(-1);
					} // if (GetSaveFileName(&ofn)==TRUE) 
				}
				break;
			case ID_POPSHOWLOG:
				{
					CLogDlg dlg;
					dlg.SetParams(url, SVN::REV_HEAD, 0, FALSE);
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
					SVN svn;
					svn.m_app = &theApp;
					theApp.DoWaitCursor(1);
					CInputDlg dlg(this);
					dlg.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
					dlg.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
					dlg.m_sInputText.LoadString(IDS_INPUT_REMOVELOGMSG);
					if (dlg.DoModal()==IDOK)
					{
						if (!svn.Remove(url, TRUE, dlg.m_sInputText))
						{
							theApp.DoWaitCursor(-1);
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						} // if (!svn.Remove(url, TRUE)) 
						m_treeRepository.Refresh(hSelItem);
					}
					theApp.DoWaitCursor(-1);
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
					ofn.lpstrFilter = _T("All\0*.*\0");
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					CString temp;
					temp.LoadString(IDS_REPOBROWSE_IMPORT);
					ofn.lpstrTitle = temp;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					// Display the Open dialog box. 

					if (GetOpenFileName(&ofn)==TRUE)
					{
						CString path = ofn.lpstrFile;
						SVN svn;
						svn.m_app = &theApp;
						theApp.DoWaitCursor(1);
						CString filename = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						input.m_sInputText.LoadString(IDS_INPUT_ADDLOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Import(path, url+_T("/")+filename, input.m_sInputText, FALSE))
							{
								theApp.DoWaitCursor(-1);
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.Import(path, url, _T("adding file remotely"), FALSE)) 
							m_treeRepository.Refresh(hSelItem);
						}
						theApp.DoWaitCursor(-1);
					} // if (GetOpenFileName(&ofn)==TRUE) 
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
						theApp.DoWaitCursor(1);
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						input.m_sInputText.LoadString(IDS_INPUT_MOVELOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Move(url, filepath, TRUE, input.m_sInputText))
							{
								theApp.DoWaitCursor(-1);
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.Move(url, filepath, TRUE)) 
							m_treeRepository.Refresh(hSelItem);
						}
						theApp.DoWaitCursor(-1);
					} // if (dlg.DoModal() == IDOK) 
				}
				break;
			case ID_POPCOPYTO:
				{
					CRenameDlg dlg;
					dlg.m_name = url;
					dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_COPY);
					if (dlg.DoModal() == IDOK)
					{
						SVN svn;
						svn.m_app = &theApp;
						theApp.DoWaitCursor(1);
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						input.m_sInputText.LoadString(IDS_INPUT_COPYLOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.Copy(url, dlg.m_name, SVN::REV_HEAD, input.m_sInputText))
							{
								theApp.DoWaitCursor(-1);
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.Copy(url, dlg.m_name, SVN::REV_HEAD, _T("copy remotely"))) 
							m_treeRepository.Refresh(hSelItem);
						}
						theApp.DoWaitCursor(-1);
					} // if (dlg.DoModal() == IDOK) 
				}
				break;
			case ID_POPMKDIR:
				{
					CRenameDlg dlg;
					dlg.m_name = _T("");
					dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_MKDIR);
					if (dlg.DoModal() == IDOK)
					{
						SVN svn;
						svn.m_app = &theApp;
						theApp.DoWaitCursor(1);
						CInputDlg input(this);
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						input.m_sInputText.LoadString(IDS_INPUT_MKDIRLOGMSG);
						if (input.DoModal() == IDOK)
						{
							if (!svn.MakeDir(url+_T("/")+dlg.m_name, input.m_sInputText))
							{
								theApp.DoWaitCursor(-1);
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return;
							} // if (!svn.MakeDir(url+_T("/")+dlg.m_name, _T("created directory remotely"))) 
							m_treeRepository.RefreshMe(hSelItem);
						}
						theApp.DoWaitCursor(-1);
					} // if (dlg.DoModal() == IDOK) 
				}
				break;
			}
			GetDlgItem(IDOK)->EnableWindow(TRUE);
		} // if (popup.CreatePopupMenu()) 
	} // if (hSelItem) 
}
