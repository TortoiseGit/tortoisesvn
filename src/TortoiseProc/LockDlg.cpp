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
#include ".\lockdlg.h"


// CLockDlg dialog

IMPLEMENT_DYNAMIC(CLockDlg, CResizableStandAloneDialog)
CLockDlg::CLockDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CLockDlg::IDD, pParent)
	, m_bStealLocks(FALSE)
	, m_pThread(NULL)
{
}

CLockDlg::~CLockDlg()
{
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}

void CLockDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_STEALLOCKS, m_bStealLocks);
	DDX_Control(pDX, IDC_FILELIST, m_cFileList);
	DDX_Control(pDX, IDC_LOCKMESSAGE, m_cEdit);
}


BEGIN_MESSAGE_MAP(CLockDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_HELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_LOCKMESSAGE, OnEnChangeLockmessage)
END_MESSAGE_MAP()


// CLockDlg message handlers

BOOL CLockDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_cFileList.Init(SVNSLC_COLEXT | SVNSLC_COLLOCK);
	m_ProjectProperties.ReadPropsPathList(m_pathList);
	m_cEdit.Init(m_ProjectProperties);
	m_cEdit.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));

	if (!m_sLockMessage.IsEmpty())
		m_cEdit.SetText(m_sLockMessage);
		
	AddAnchor(IDC_LOCKTITLELABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOCKMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_STEALLOCKS, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("LogPromptDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
	m_bBlock = TRUE;
	
	GetDlgItem(IDC_LOCKMESSAGE)->SetFocus();
	return FALSE;
}

void CLockDlg::OnOK()
{
	if (m_bBlock)
		return;
	m_cFileList.WriteCheckedNamesToPathList(m_pathList);
	UpdateData();
	m_sLockMessage = m_cEdit.GetText();


	CResizableStandAloneDialog::OnOK();
}

void CLockDlg::OnCancel()
{
	if (m_bBlock)
		return;
	CResizableStandAloneDialog::OnCancel();
}

UINT CLockDlg::StatusThreadEntry(LPVOID pVoid)
{
	return ((CLockDlg*)pVoid)->StatusThread();
}

UINT CLockDlg::StatusThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	m_bBlock = TRUE;
	GetDlgItem(IDCANCEL)->EnableWindow(false);
	GetDlgItem(IDOK)->EnableWindow(false);

	// Initialise the list control with the status of the files/folders below us
	m_cFileList.GetStatus(m_pathList);

	DWORD dwShow = SVNSLC_SHOWNORMAL | SVNSLC_SHOWMODIFIED | SVNSLC_SHOWMERGED | SVNSLC_SHOWLOCKS;
	m_cFileList.Show(dwShow, dwShow, false);

	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	GetDlgItem(IDCANCEL)->EnableWindow(true);
	CString logmsg;
	GetDlgItem(IDC_LOCKMESSAGE)->GetWindowText(logmsg);
	if (m_ProjectProperties.nMinLockMsgSize > logmsg.GetLength())
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	m_bBlock = FALSE;
	return 0;
}

BOOL CLockDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bBlock)
					return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
				Refresh();
			}
			break;
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					PostMessage(WM_COMMAND, IDOK);
					return TRUE;
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CLockDlg::Refresh()
{
	m_bBlock = TRUE;
	if (AfxBeginThread(StatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CLockDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CLockDlg::OnEnChangeLockmessage()
{
	CString sTemp;
	GetDlgItem(IDC_LOCKMESSAGE)->GetWindowText(sTemp);
	if (sTemp.GetLength() >= m_ProjectProperties.nMinLockMsgSize)
	{
		if (!m_bBlock)
			GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
}

LRESULT CLockDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	Refresh();
	return 0;
}
