// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "AboutDlg.h"


// CAboutDlg dialog

IMPLEMENT_DYNAMIC(CAboutDlg, CDialog)
CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAboutDlg::OnPaint() 
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
HCURSOR CAboutDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CPictureHolder tmpPic;
	tmpPic.CreateFromBitmap(IDB_LOGOFLIPPED);
	m_renderSrc.Create32BitFromPicture(&tmpPic,301,167);
	m_renderDest.Create32BitFromPicture(&tmpPic,301,167);

	m_waterEffect.Create(301,167);
	SetTimer(ID_EFFECTTIMER, 40, NULL);
	SetTimer(ID_DROPTIMER, 300, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == ID_EFFECTTIMER)
	{
		m_waterEffect.Render((DWORD*)m_renderSrc.GetDIBits(), (DWORD*)m_renderDest.GetDIBits());
		CClientDC dc(this);
		CPoint ptOrigin(15,20);
		m_renderDest.Draw(&dc,ptOrigin);
	}
	if (nIDEvent == ID_DROPTIMER)
	{
		CRect r;
		r.left = 15;
		r.top = 20;
		r.right = r.left + m_renderSrc.GetWidth();
		r.bottom = r.top + m_renderSrc.GetHeight();
		m_waterEffect.Blob(random(r.left,r.right), random(r.top, r.bottom), 2, 400, m_waterEffect.m_iHpage);
	}
	CDialog::OnTimer(nIDEvent);
}

void CAboutDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect r;
	r.left = 15;
	r.top = 20;
	r.right = r.left + m_renderSrc.GetWidth();
	r.bottom = r.top + m_renderSrc.GetHeight();

	if(r.PtInRect(point) == TRUE)
	{
		// dibs are drawn upside down...
		point.y -= 20;
		point.y = 167-point.y;

		if (nFlags & MK_LBUTTON)
			m_waterEffect.Blob(point.x -15,point.y,5,1600,m_waterEffect.m_iHpage);
		else
			m_waterEffect.Blob(point.x -15,point.y,2,50,m_waterEffect.m_iHpage);

	}


	CDialog::OnMouseMove(nFlags, point);
}
