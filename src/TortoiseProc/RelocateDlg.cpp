// RelocateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RelocateDlg.h"
#include ".\relocatedlg.h"


// CRelocateDlg dialog

IMPLEMENT_DYNAMIC(CRelocateDlg, CDialog)
CRelocateDlg::CRelocateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRelocateDlg::IDD, pParent)
	, m_sToUrl(_T(""))
	, m_sFromUrl(_T(""))
{
}

CRelocateDlg::~CRelocateDlg()
{
}

void CRelocateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TOURL, m_sToUrl);
	DDX_Text(pDX, IDC_FROMURL, m_sFromUrl);
}


BEGIN_MESSAGE_MAP(CRelocateDlg, CDialog)
END_MESSAGE_MAP()


// CRelocateDlg message handlers

BOOL CRelocateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
