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
#include "SimplePrompt.h"
#include ".\simpleprompt.h"


// CSimplePrompt dialog

IMPLEMENT_DYNAMIC(CSimplePrompt, CDialog)
CSimplePrompt::CSimplePrompt(CWnd* pParent /*=NULL*/)
	: CDialog(CSimplePrompt::IDD, pParent)
	, m_sUsername(_T(""))
	, m_sPassword(_T(""))
	, m_bSaveAuthentication(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CSimplePrompt::~CSimplePrompt()
{
}

void CSimplePrompt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USEREDIT, m_sUsername);
	DDX_Text(pDX, IDC_PASSEDIT, m_sPassword);
	DDX_Check(pDX, IDC_SAVECHECK, m_bSaveAuthentication);
}


BEGIN_MESSAGE_MAP(CSimplePrompt, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CSimplePrompt message handlers

BOOL CSimplePrompt::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	GetDlgItem(IDC_USEREDIT)->SetFocus();
	CenterWindow(CWnd::FromHandle(m_hParentWnd));
	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSimplePrompt::OnPaint()
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

HCURSOR CSimplePrompt::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
