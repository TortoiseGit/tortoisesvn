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
#include "LogPromptDlg.h"
#include "UnicodeUtils.h"
#include ".\logpromptdlg.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"

// CLogPromptDlg dialog

IMPLEMENT_DYNAMIC(CLogPromptDlg, CResizableDialog)
CLogPromptDlg::CLogPromptDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CLogPromptDlg::IDD, pParent)
	, m_sLogMessage(_T(""))
	, m_bRecursive(FALSE)
	, m_bShowUnversioned(FALSE)
	, m_bBlock(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CLogPromptDlg::~CLogPromptDlg()
{
}

void CLogPromptDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOGMESSAGE, m_sLogMessage);
	DDX_Control(pDX, IDC_FILELIST, m_ListCtrl);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_LogMessage);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CLogPromptDlg, CResizableDialog)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
END_MESSAGE_MAP()


// CLogPromptDlg message handlers
// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.

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
// the minimized window.
HCURSOR CLogPromptDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CLogPromptDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	LOGFONT LogFont;
	LogFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
	LogFont.lfWidth          = 0;
	LogFont.lfEscapement     = 0;
	LogFont.lfOrientation    = 0;
	LogFont.lfWeight         = 400;
	LogFont.lfItalic         = 0;
	LogFont.lfUnderline      = 0;
	LogFont.lfStrikeOut      = 0;
	LogFont.lfCharSet        = DEFAULT_CHARSET;
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy(LogFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	m_logFont.CreateFontIndirect(&LogFont);
	GetDlgItem(IDC_LOGMESSAGE)->SetFont(&m_logFont);

	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	UpdateData(FALSE);

	OnEnChangeLogmessage();

	CString temp = m_sPath;

	m_ListCtrl.Init(SVNSLC_COLSTATUS);
	m_ListCtrl.SetSelectButton(&m_SelectAll);
	m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &StatusThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bBlock = TRUE;

	m_tooltips.Create(this);
	m_SelectAll.SetCheck(BST_INDETERMINATE);
	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();

	if (CRegDWORD(_T("Software\\TortoiseSVN\\MinLogSize"), 0) > 0)
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HINTLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(_T("LogPromptDlg"));
	CenterWindow(CWnd::FromHandle(hWndExplorer));

	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CLogPromptDlg::OnOK()
{
	if (m_bBlock)
		return;
	m_bBlock = TRUE;
	CDWordArray arDeleted;
	//first add all the unversioned files the user selected
	//and check if all versioned files are selected
	int nUnchecked = 0;
	for (int j=0; j<m_ListCtrl.GetItemCount(); j++)
	{
		CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
		if (m_ListCtrl.GetCheck(j))
		{
			if (entry->status == svn_wc_status_unversioned)
			{
				SVN svn;
				svn.Add(entry->path, FALSE);
			} 
			if (entry->status == svn_wc_status_missing)
			{
				SVN svn;
				svn.Remove(entry->path, TRUE);
			}
			if (entry->status == svn_wc_status_deleted)
			{
				arDeleted.Add(j);
			}
		}
		else
		{
			if ((entry->status != svn_wc_status_unversioned)	&&
				(entry->status != svn_wc_status_ignored))
				nUnchecked++;
		}
	} // for (int j=0; j<m_ListCtrl.GetItemCount(); j++)

	if ((nUnchecked == 0)&&(m_ListCtrl.m_nTargetCount == 1))
	{
		m_bRecursive = TRUE;
	}
	else
	{
		m_bRecursive = FALSE;

		//the next step: find all deleted files and check if they're 
		//inside a deleted folder. If that's the case, then remove those
		//files from the list since they'll get deleted by the parent
		//folder automatically.
		for (int i=0; i<arDeleted.GetCount(); i++)
		{
			if (m_ListCtrl.GetCheck(arDeleted.GetAt(i)))
			{
				CString path = ((CSVNStatusListCtrl::FileEntry *)(m_ListCtrl.GetListEntry(arDeleted.GetAt(i))))->path;
				path += _T("/");
				if (PathIsDirectory(path))
				{
					//now find all children of this directory
					for (int j=0; j<arDeleted.GetCount(); j++)
					{
						if (i!=j)
						{
							if (m_ListCtrl.GetCheck(arDeleted.GetAt(j)))
							{
								if (path.CompareNoCase(((CSVNStatusListCtrl::FileEntry *)(m_ListCtrl.GetListEntry(arDeleted.GetAt(j))))->path.Left(path.GetLength()))==0)
								{
									m_ListCtrl.SetCheck(arDeleted.GetAt(j), FALSE);
								}
							}
						}
					}
				}
			}
		} // for (int i=0; i<arDeleted.GetCount(); i++) 

		//save only the files the user has selected into the temporary file
		try
		{
			CStdioFile file(m_sPath, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
			for (int i=0; i<m_ListCtrl.GetItemCount(); i++)
			{
				if (m_ListCtrl.GetCheck(i))
				{
					file.WriteString(m_ListCtrl.GetListEntry(i)->path + _T("\n"));
				}
			} // for (int i=0; i<m_ListCtrl.GetItemCount(); i++) 
			file.Close();
		}
		catch (CFileException* pE)
		{
			TRACE(_T("CFileException in Commit!\n"));
			pE->Delete();
		}
	}
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	m_bBlock = FALSE;
	CResizableDialog::OnOK();
}

DWORD WINAPI StatusThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CLogPromptDlg*	pDlg;
	pDlg = (CLogPromptDlg*)pVoid;
	pDlg->m_bBlock = TRUE;
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(false);

	pDlg->m_ListCtrl.GetStatus(pDlg->m_sPath);
	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL;
	dwShow |= DWORD(pDlg->m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
	pDlg->m_ListCtrl.Show(dwShow);
	pDlg->m_ListCtrl.CheckAll(SVNSLC_SHOWMODIFIED|SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWMISSING|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED);

	if (pDlg->m_ListCtrl.GetItemCount()==0)
	{
		CMessageBox::Show(pDlg->m_hWnd, IDS_LOGPROMPT_NOTHINGTOCOMMIT, IDS_APPNAME, MB_ICONINFORMATION);
		pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
		pDlg->m_bBlock = FALSE;
		pDlg->EndDialog(0);
		return -1;
	} // if (pDlg->m_ListCtrl.GetItemCount()==0) 
	if (pDlg->m_ListCtrl.HasExternalsFromDifferentRepos())
	{
		CMessageBox::Show(pDlg->m_hWnd, IDS_LOGPROMPT_EXTERNALS, IDS_APPNAME, MB_ICONINFORMATION);
	} // if (bHasExternalsFromDifferentRepos) 
	pDlg->GetDlgItem(IDC_COMMIT_TO)->SetWindowText(pDlg->m_ListCtrl.m_sURL);
	pDlg->m_tooltips.AddTool(pDlg->GetDlgItem(IDC_STATISTICS), pDlg->m_ListCtrl.GetStatistics());
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
	pDlg->m_bBlock = FALSE;
	return 0;
}

void CLogPromptDlg::OnCancel()
{
	if (m_bBlock)
		return;
	DeleteFile(m_sPath);
	UpdateData(TRUE);
	CResizableDialog::OnCancel();
}

void CLogPromptDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == 2)
		return;
	m_ListCtrl.SelectAll(state == 1);
}

BOOL CLogPromptDlg::PreTranslateMessage(MSG* pMsg)
{
	if (!m_bBlock)
		m_tooltips.RelayEvent(pMsg);
	if ((pMsg->message == WM_KEYDOWN)&&(pMsg->wParam == VK_F5))//(nChar == VK_F5)
	{
		if (m_bBlock)
			return CResizableDialog::PreTranslateMessage(pMsg);
		Refresh();
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CLogPromptDlg::Refresh()
{
	m_bBlock = TRUE;
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &StatusThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CLogPromptDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CLogPromptDlg::OnBnClickedShowunversioned()
{
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if (!m_bBlock)
	{
		DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL;
		dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
		m_ListCtrl.Show(dwShow);
	}
}

void CLogPromptDlg::OnEnChangeLogmessage()
{
	CString sTemp;
	GetDlgItem(IDC_LOGMESSAGE)->GetWindowText(sTemp);
	if (DWORD(sTemp.GetLength()) > CRegDWORD(_T("Software\\TortoiseSVN\\MinLogSize"), 0))
	{
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
}









