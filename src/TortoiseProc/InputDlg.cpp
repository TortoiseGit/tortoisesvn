// InputDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "InputDlg.h"
#include ".\inputdlg.h"


// CInputDlg dialog

IMPLEMENT_DYNAMIC(CInputDlg, CResizableDialog)
CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CInputDlg::IDD, pParent)
	, m_sInputText(_T(""))
{
}

CInputDlg::~CInputDlg()
{
}

void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_INPUTTEXT, m_sInputText);
}


BEGIN_MESSAGE_MAP(CInputDlg, CResizableDialog)
END_MESSAGE_MAP()


// CInputDlg message handlers

BOOL CInputDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	if (!m_sHintText.IsEmpty())
	{
		GetDlgItem(IDC_HINTTEXT)->SetWindowText(m_sHintText);
	}
	if (!m_sTitle.IsEmpty())
	{
		this->SetWindowText(m_sTitle);
	}
	if (!m_sInputText.IsEmpty())
	{
		GetDlgItem(IDC_INPUTTEXT)->SetWindowText(m_sInputText);
	}

	AddAnchor(IDC_HINTTEXT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_INPUTTEXT, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_CENTER);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
