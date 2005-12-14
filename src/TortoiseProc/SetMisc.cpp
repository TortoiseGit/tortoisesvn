// SetMisc.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetMisc.h"


// CSetMisc dialog

IMPLEMENT_DYNAMIC(CSetMisc, CPropertyPage)

CSetMisc::CSetMisc()
	: CPropertyPage(CSetMisc::IDD)
	, m_bUnversionedRecurse(FALSE)
{
	m_regUnversionedRecurse = CRegDWORD(_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE);
	m_bUnversionedRecurse = (DWORD)m_regUnversionedRecurse;
}

CSetMisc::~CSetMisc()
{
}

void CSetMisc::SaveData()
{
	m_regUnversionedRecurse = m_bUnversionedRecurse;
}

void CSetMisc::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UNVERSIONEDRECURSE, m_bUnversionedRecurse);
}


BEGIN_MESSAGE_MAP(CSetMisc, CPropertyPage)
	ON_BN_CLICKED(IDC_UNVERSIONEDRECURSE, &CSetMisc::OnChanged)
END_MESSAGE_MAP()


// CSetMisc message handlers

void CSetMisc::OnChanged()
{
	SetModified();
}

BOOL CSetMisc::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_UNVERSIONEDRECURSE, IDS_SETTINGS_UNVERSIONEDRECURSE_TT);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetMisc::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}
