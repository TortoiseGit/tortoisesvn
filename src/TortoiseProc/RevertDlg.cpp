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
#include "messagebox.h"
#include "Revertdlg.h"
#include "SVN.h"
#include "Registry.h"
#include ".\revertdlg.h"


// CRevertDlg dialog

IMPLEMENT_DYNAMIC(CRevertDlg, CResizableStandAloneDialog)
CRevertDlg::CRevertDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRevertDlg::IDD, pParent)
	, m_bSelectAll(TRUE)
	, m_bThreadRunning(FALSE)
	
{
}

CRevertDlg::~CRevertDlg()
{
}

void CRevertDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REVERTLIST, m_RevertList);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CRevertDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()



BOOL CRevertDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_RevertList.Init(SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS);
	m_RevertList.SetSelectButton(&m_SelectAll);
	
	AddAnchor(IDC_REVERTLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RevertDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	if (AfxBeginThread(RevertThreadEntry, this)==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bThreadRunning = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

UINT CRevertDlg::RevertThreadEntry(LPVOID pVoid)
{
	return ((CRevertDlg*)pVoid)->RevertThread();
}

UINT CRevertDlg::RevertThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	GetDlgItem(IDOK)->EnableWindow(false);
	GetDlgItem(IDCANCEL)->EnableWindow(false);

	m_RevertList.GetStatus(m_pathList);
	m_RevertList.Show(SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWDIRECTFILES, 
						SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWDIRECTFILES);

	GetDlgItem(IDOK)->EnableWindow(true);
	GetDlgItem(IDCANCEL)->EnableWindow(true);

	m_bThreadRunning = FALSE;
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	return 0;
}

void CRevertDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	//save only the files the user has selected into the temporary file
	m_bRecursive = TRUE;
	for (int i=0; i<m_RevertList.GetItemCount(); ++i)
	{
		if (!m_RevertList.GetCheck(i))
		{
			m_bRecursive = FALSE;
			break;
		}
	}
	if (!m_bRecursive)
	{
		m_RevertList.WriteCheckedNamesToPathList(m_pathList);
	}

	CResizableStandAloneDialog::OnOK();
}

void CRevertDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CRevertDlg::OnBnClickedHelp()
{
	OnHelp();
}


BOOL CRevertDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bThreadRunning)
	{
		if ((pWnd)&&(pWnd == GetDlgItem(IDC_REVERTLIST)))
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

void CRevertDlg::OnBnClickedSelectall()
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
	m_RevertList.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

BOOL CRevertDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					PostMessage(WM_COMMAND, IDOK);
					return TRUE;
				}
			}
			break;
		case VK_F5:
			{
				if (!m_bThreadRunning)
				{
					if (AfxBeginThread(RevertThreadEntry, this)==0)
					{
						CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
					}
					else
						m_bThreadRunning = TRUE;
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
