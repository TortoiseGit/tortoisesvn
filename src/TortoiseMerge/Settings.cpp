#include "stdafx.h"
#include "Settings.h"



IMPLEMENT_DYNAMIC(CSettings, CPropertySheet)
CSettings::CSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CSettings::CSettings(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddPropPages();
}

CSettings::~CSettings()
{
	RemovePropPages();
}

void CSettings::AddPropPages()
{
	m_pMainPage = new CSetMainPage();

	AddPage(m_pMainPage);
}

void CSettings::RemovePropPages()
{
	delete m_pMainPage;
}

void CSettings::SaveData()
{
	m_pMainPage->SaveData();
}

BEGIN_MESSAGE_MAP(CSettings, CPropertySheet)
END_MESSAGE_MAP()


// CSettings message handlers

BOOL CSettings::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	return bResult;
}
