// RelocateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RelocateDlg.h"


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
