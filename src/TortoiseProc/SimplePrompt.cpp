// SimplePrompt.cpp : implementation file
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
