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
#include "ChangedDlg.h"
#include "messagebox.h"
#include "cursor.h"
#include ".\changeddlg.h"


IMPLEMENT_DYNAMIC(CChangedDlg, CResizableDialog)
CChangedDlg::CChangedDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CChangedDlg::IDD, pParent)
	, m_bShowUnversioned(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bRemote = FALSE;
}

CChangedDlg::~CChangedDlg()
{
}

void CChangedDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHANGEDLIST, m_FileListCtrl);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
}


BEGIN_MESSAGE_MAP(CChangedDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CHECKREPO, OnBnClickedCheckrepo)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
END_MESSAGE_MAP()


void CChangedDlg::OnPaint() 
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
HCURSOR CChangedDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CChangedDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;
	UpdateData(FALSE);

	m_FileListCtrl.Init(SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS | SVNSLC_COLREMOTETEXT | SVNSLC_COLREMOTEPROP, FALSE);

	AddAnchor(IDC_CHANGEDLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SUMMARYTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CHECKREPO, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	this->hWnd = this->m_hWnd;
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("ChangedDlg"));

	//first start a thread to obtain the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &ChangedStatusThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


DWORD WINAPI ChangedStatusThread(LPVOID pVoid)
{
	CChangedDlg * pDlg;
	pDlg = (CChangedDlg *)pVoid;

	pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	pDlg->GetDlgItem(IDC_CHECKREPO)->EnableWindow(FALSE);
	pDlg->GetDlgItem(IDC_SHOWUNVERSIONED)->EnableWindow(FALSE);
	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));

	if (!pDlg->m_FileListCtrl.GetStatus(pDlg->m_path, pDlg->m_bRemote))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->m_FileListCtrl.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
	}
	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL;
	dwShow |= pDlg->m_bShowUnversioned ? SVNSLC_SHOWUNVERSIONED : 0;
	pDlg->m_FileListCtrl.Show(dwShow);

	if (LONG(pDlg->m_FileListCtrl.m_HeadRev) >= 0)
	{
		CString temp;
		temp.Format(IDS_REPOSTATUS_HEADREV, LONG(pDlg->m_FileListCtrl.m_HeadRev));
		pDlg->GetDlgItem(IDC_SUMMARYTEXT)->SetWindowText(temp);
	}

	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	pDlg->GetDlgItem(IDC_CHECKREPO)->EnableWindow(TRUE);
	pDlg->GetDlgItem(IDC_SHOWUNVERSIONED)->EnableWindow(TRUE);
	return 0;
}

void CChangedDlg::OnOK()
{
	if (GetDlgItem(IDOK)->IsWindowEnabled())
		__super::OnOK();
}

void CChangedDlg::OnCancel()
{
	if (GetDlgItem(IDOK)->IsWindowEnabled())
		__super::OnCancel();
}

void CChangedDlg::OnBnClickedCheckrepo()
{
	m_bRemote = TRUE;
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &ChangedStatusThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CChangedDlg::OnBnClickedShowunversioned()
{
	UpdateData();
	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL;
	dwShow |= m_bShowUnversioned ? SVNSLC_SHOWUNVERSIONED : 0;
	m_FileListCtrl.Show(dwShow);
	m_regAddBeforeCommit = m_bShowUnversioned;
}


