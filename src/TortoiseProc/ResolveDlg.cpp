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
#include "messagebox.h"
#include "ResolveDlg.h"


// CResolveDlg dialog

IMPLEMENT_DYNAMIC(CResolveDlg, CResizableStandAloneDialog)
CResolveDlg::CResolveDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CResolveDlg::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{
}

CResolveDlg::~CResolveDlg()
{
}

void CResolveDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RESOLVELIST, m_resolveListCtrl);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CResolveDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
END_MESSAGE_MAP()


// CResolveDlg message handlers
BOOL CResolveDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	//set the listcontrol to support checkboxes
	m_resolveListCtrl.Init(0, _T("ResolveDlg"), SVNSLC_POPALL ^ (SVNSLC_POPIGNORE|SVNSLC_POPADD|SVNSLC_POPCOMMIT));
	m_resolveListCtrl.SetConfirmButton((CButton*)GetDlgItem(IDOK));
	m_resolveListCtrl.SetSelectButton(&m_SelectAll);
	m_resolveListCtrl.SetCancelBool(&m_bCancelled);

	AddAnchor(IDC_RESOLVELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("ResolveDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	if(AfxBeginThread(ResolveThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CResolveDlg::OnOK()
{
	if (m_bThreadRunning)
		return;

	//save only the files the user has selected into the pathlist
	m_resolveListCtrl.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

void CResolveDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CResolveDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	theApp.DoWaitCursor(1);
	m_resolveListCtrl.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

UINT CResolveDlg::ResolveThreadEntry(LPVOID pVoid)
{
	return ((CResolveDlg*)pVoid)->ResolveThread();
}
UINT CResolveDlg::ResolveThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	DialogEnableWindow(IDOK, false);

	m_bCancelled = false;

	if (!m_resolveListCtrl.GetStatus(m_pathList))
	{
		CMessageBox::Show(m_hWnd, m_resolveListCtrl.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
	}
	m_resolveListCtrl.Show(SVNSLC_SHOWCONFLICTED, SVNSLC_SHOWCONFLICTED);

	DialogEnableWindow(IDOK, true);
	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

void CResolveDlg::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CResolveDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
					return TRUE;
				}
			}
			break;
		case VK_F5:
			{
				if (!m_bThreadRunning)
				{
					if(AfxBeginThread(ResolveThreadEntry, this) == NULL)
					{
						CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
					}
					else
						InterlockedExchange(&m_bThreadRunning, TRUE);
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CResolveDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if(AfxBeginThread(ResolveThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}
