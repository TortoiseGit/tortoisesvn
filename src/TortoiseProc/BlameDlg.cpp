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
#include "BlameDlg.h"
#include "Balloon.h"

// CBlameDlg dialog

IMPLEMENT_DYNAMIC(CBlameDlg, CResizableDialog)
CBlameDlg::CBlameDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CBlameDlg::IDD, pParent)
	, StartRev(1)
	, EndRev(0)
	, m_sStartRev(_T("1"))
	, m_bTextView(FALSE)
{
	m_regTextView = CRegDWORD(_T("Software\\TortoiseSVN\\TextBlame"), FALSE);
	m_bTextView = m_regTextView;
}

CBlameDlg::~CBlameDlg()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBlameDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVISON_START, m_sStartRev);
	DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
	DDX_Check(pDX, IDC_CHECK1, m_bTextView);
}


BEGIN_MESSAGE_MAP(CBlameDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_REVISION_HEAD, OnBnClickedRevisionHead)
	ON_BN_CLICKED(IDC_REVISION_N, OnBnClickedRevisionN)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


void CBlameDlg::OnPaint() 
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
HCURSOR CBlameDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CBlameDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_bTextView = m_regTextView;
	// set head revision as default revision
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("BlameDlg"));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CBlameDlg::OnBnClickedRevisionHead()
{
	GetDlgItem(IDC_REVISION_END)->EnableWindow(FALSE);
}

void CBlameDlg::OnBnClickedRevisionN()
{
	GetDlgItem(IDC_REVISION_END)->EnableWindow(TRUE);
}

void CBlameDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	m_regTextView = m_bTextView;
	StartRev = SVNRev(m_sStartRev);
	EndRev = SVNRev(m_sEndRev);
	if (!StartRev.IsValid())
	{
		CWnd* ctrl = GetDlgItem(IDC_REVISON_START);
		CRect rt;
		ctrl->GetWindowRect(rt);
		CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
		CBalloon::ShowBalloon(this, point, IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}
	EndRev = SVNRev(m_sEndRev);
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		EndRev = SVNRev(_T("HEAD"));
	}
	if (!EndRev.IsValid())
	{
		CWnd* ctrl = GetDlgItem(IDC_REVISION_END);
		CRect rt;
		ctrl->GetWindowRect(rt);
		CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
		CBalloon::ShowBalloon(this, point, IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	UpdateData(FALSE);

	CResizableDialog::OnOK();
}

void CBlameDlg::OnBnClickedHelp()
{
	OnHelp();
}
