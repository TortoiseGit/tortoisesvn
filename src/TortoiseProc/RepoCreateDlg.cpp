// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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
#include "RepoCreateDlg.h"
#include ".\repocreatedlg.h"


// CRepoCreateDlg dialog

IMPLEMENT_DYNAMIC(CRepoCreateDlg, CDialog)
CRepoCreateDlg::CRepoCreateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRepoCreateDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CRepoCreateDlg::~CRepoCreateDlg()
{
}

void CRepoCreateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRepoCreateDlg, CDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


// CRepoCreateDlg message handlers

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRepoCreateDlg::OnPaint() 
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
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRepoCreateDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CRepoCreateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CheckRadioButton(IDC_RADIOBDB, IDC_RADIOFSFS, IDC_RADIOBDB);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRepoCreateDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	if (GetCheckedRadioButton(IDC_RADIOBDB, IDC_RADIOFSFS) == IDC_RADIOBDB)
	{
		RepoType = _T("bdb");
	}
	else
	{
		RepoType = _T("fsfs");
	}

	CDialog::OnOK();
}

void CRepoCreateDlg::OnBnClickedHelp()
{
	OnHelp();
}
