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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Control(pDX, IDC_OLDLOGS, m_OldLogs);
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
	ON_BN_CLICKED(IDC_FILLLOG, OnBnClickedFilllog)
	ON_CBN_SELCHANGE(IDC_OLDLOGS, OnCbnSelchangeOldlogs)
	ON_CBN_CLOSEUP(IDC_OLDLOGS, OnCbnCloseupOldlogs)
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
	m_ProjectProperties.ReadPropsTempfile(m_sPath);
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
	
	if (m_ProjectProperties.sMessage.IsEmpty())
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}
	else
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
		if (!m_ProjectProperties.sLabel.IsEmpty())
			GetDlgItem(IDC_BUGIDLABEL)->SetWindowText(m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGID)->SetFocus();
	}
	if (m_ProjectProperties.nLogWidthMarker)
	{
		m_LogMessage.WordWrap(FALSE);
		m_LogMessage.SetMarginLine(m_ProjectProperties.nLogWidthMarker);
	}
	m_LogMessage.SetWindowText(m_ProjectProperties.sLogTemplate);
	m_OldLogs.LoadHistory(_T("commit"), _T("logmsgs"));
	
	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHIST, TOP_LEFT);
	AddAnchor(IDC_OLDLOGS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HINTLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILLLOG, BOTTOM_RIGHT);
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
	CString id;
	GetDlgItem(IDC_BUGID)->GetWindowText(id);
	if (m_ProjectProperties.bNumber)
	{
		TCHAR c = 0;
		BOOL bInvalid = FALSE;
		for (int i=0; i<id.GetLength(); ++i)
		{
			c = id.GetAt(i);
			if ((c < '0')&&(c != ','))
			{
				bInvalid = TRUE;
				break;
			}
			if (c > '9')
				bInvalid = TRUE;
		}
		if (bInvalid)
		{
			CWnd* ctrl = GetDlgItem(IDC_BUGID);
			CRect rt;
			ctrl->GetWindowRect(rt);
			CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
			CBalloon::ShowBalloon(ctrl, point, IDS_LOGPROMPT_ONLYNUMBERS, TRUE, IDI_EXCLAMATION);
			return;
		}
	}
	if ((m_ProjectProperties.bWarnIfNoIssue)&&(id.IsEmpty()))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_LOGPROMPT_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}
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
	m_sBugID.Trim();
	m_OldLogs.AddString(m_sLogMessage, 0);
	m_OldLogs.SaveHistory();
	if (!m_sBugID.IsEmpty())
	{
		m_sBugID.Replace(_T(", "), _T(","));
		m_sBugID.Replace(_T(" ,"), _T(","));
		CString sBugID = m_ProjectProperties.sMessage;
		sBugID.Replace(_T("%BUGID%"), m_sBugID);
		m_sLogMessage += _T("\n") + sBugID + _T("\n");
		UpdateData(FALSE);		
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
	pDlg->m_bBlock = TRUE;
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(false);
	pDlg->GetDlgItem(IDOK)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_FILLLOG)->EnableWindow(false);
	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));
	BOOL success = pDlg->m_ListCtrl.GetStatus(pDlg->m_sPath);
	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS;
	dwShow |= DWORD(pDlg->m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
	pDlg->m_ListCtrl.Show(dwShow);
	pDlg->m_ListCtrl.CheckAll(SVNSLC_SHOWMODIFIED|SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWMISSING|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED);

	if (pDlg->m_ListCtrl.HasExternalsFromDifferentRepos()||pDlg->m_ListCtrl.HasExternals())
	{
		CMessageBox::Show(pDlg->m_hWnd, IDS_LOGPROMPT_EXTERNALS, IDS_APPNAME, MB_ICONINFORMATION);
	} // if (bHasExternalsFromDifferentRepos) 
	pDlg->GetDlgItem(IDC_COMMIT_TO)->SetWindowText(pDlg->m_ListCtrl.m_sURL);
	pDlg->m_tooltips.AddTool(pDlg->GetDlgItem(IDC_STATISTICS), pDlg->m_ListCtrl.GetStatistics());
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
	pDlg->GetDlgItem(IDC_FILLLOG)->EnableWindow(true);
	CString logmsg;
	pDlg->GetDlgItem(IDC_LOGMESSAGE)->GetWindowText(logmsg);
	if (pDlg->m_ProjectProperties.nMinLogSize > (DWORD)logmsg.GetLength())
	{
		pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	if (!success)
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->m_ListCtrl.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
		pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
		pDlg->m_bBlock = FALSE;
		pDlg->EndDialog(0);
		return -1;
	}
	if (pDlg->m_ListCtrl.GetItemCount()==0)
	{
		CMessageBox::Show(pDlg->m_hWnd, IDS_LOGPROMPT_NOTHINGTOCOMMIT, IDS_APPNAME, MB_ICONINFORMATION);
		pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
		pDlg->m_bBlock = FALSE;
		pDlg->EndDialog(0);
		return -1;
	} // if (pDlg->m_ListCtrl.GetItemCount()==0) 
	pDlg->m_bBlock = FALSE;
	return 0;
}

void CLogPromptDlg::OnCancel()
{
	if (m_bBlock)
		return;
	DeleteFile(m_sPath);
	UpdateData(TRUE);
	m_OldLogs.AddString(m_sLogMessage, 0);
	m_OldLogs.SaveHistory();
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
		DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS;
		dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
		m_ListCtrl.Show(dwShow);
	}
}

void CLogPromptDlg::OnEnChangeLogmessage()
{
	CString sTemp;
	GetDlgItem(IDC_LOGMESSAGE)->GetWindowText(sTemp);
	if (DWORD(sTemp.GetLength()) >= m_ProjectProperties.nMinLogSize)
	{
		if (!m_bBlock)
			GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
}

void CLogPromptDlg::OnBnClickedFilllog()
{
	if (m_bBlock)
		return;
	CString logmsg;
	TCHAR buf[MAX_PATH];
	for (int i=0; i<m_ListCtrl.GetItemCount(); ++i)
	{
		if (m_ListCtrl.GetCheck(i))
		{
			CString line;
			CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(i);
			svn_wc_status_kind status = entry->status;
			if (status == svn_wc_status_unversioned)
				status = svn_wc_status_added;
			if (status == svn_wc_status_missing)
				status = svn_wc_status_deleted;
			WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
			if ((DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\EnglishTemplate"), FALSE)==TRUE)
				langID = 1033;
			SVNStatus::GetStatusString(AfxGetResourceHandle(), status, buf, sizeof(buf)/sizeof(TCHAR), langID);
			line.Format(_T("%-10s %s\r\n"), buf, m_ListCtrl.GetItemText(i,0));
			logmsg += line;
		}
	}
	m_LogMessage.ReplaceSel(logmsg, TRUE);
}

void CLogPromptDlg::OnCbnSelchangeOldlogs()
{
	m_sLogMessage = m_OldLogs.GetString();
	UpdateData(FALSE);
	if (m_ProjectProperties.nMinLogSize > (DWORD)m_sLogMessage.GetLength())
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
}

void CLogPromptDlg::OnCbnCloseupOldlogs()
{
	m_sLogMessage = m_OldLogs.GetString();
	UpdateData(FALSE);
	if (m_ProjectProperties.nMinLogSize > (DWORD)m_sLogMessage.GetLength())
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
}










