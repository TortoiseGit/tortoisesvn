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
#include "messagebox.h"
#include "DirFileEnum.h"
#include "AddDlg.h"
#include "SVNConfig.h"
#include ".\adddlg.h"


// CAddDlg dialog

IMPLEMENT_DYNAMIC(CAddDlg, CResizableDialog)
CAddDlg::CAddDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CAddDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CAddDlg::~CAddDlg()
{
}

void CAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ADDLIST, m_addListCtrl);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CAddDlg, CResizableDialog)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


// CAddDlg message handlers
void CAddDlg::OnPaint() 
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
	} // if (IsIconic()) 
	else
	{
		CResizableDialog::OnPaint();
	}
}
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAddDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CAddDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//set the listcontrol to support checkboxes
	m_addListCtrl.Init(0);
	m_addListCtrl.SetSelectButton(&m_SelectAll);

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &AddThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bThreadRunning = TRUE;

	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(_T("AddDlg"));
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAddDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	//save only the files the user has selected into the temporary file
	try
	{
		CStdioFile file(m_sPath, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
		for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
		{
			if (m_addListCtrl.GetCheck(i))
			{
				file.WriteString(m_addListCtrl.GetListEntry(i)->path+_T("\n"));
			}
		} 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Add!\n");
		pE->Delete();
	}

	CResizableDialog::OnOK();
}

void CAddDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;

	CResizableDialog::OnCancel();
}

void CAddDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == 2)
		return;
	theApp.DoWaitCursor(1);
	m_addListCtrl.SelectAll(state == 1);
	theApp.DoWaitCursor(-1);
}

DWORD WINAPI AddThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CAddDlg*	pDlg;
	pDlg = (CAddDlg*)pVoid;
	pDlg->GetDlgItem(IDOK)->EnableWindow(false);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(false);

	pDlg->m_addListCtrl.GetStatus(pDlg->m_sPath);
	pDlg->m_addListCtrl.Show(SVNSLC_SHOWUNVERSIONED);
	pDlg->m_addListCtrl.CheckAll(SVNSLC_SHOWUNVERSIONED);

	pDlg->GetDlgItem(IDOK)->EnableWindow(true);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
	pDlg->m_bThreadRunning = FALSE;
	return 0;
}

void CAddDlg::OnBnClickedHelp()
{
	OnHelp();
}






