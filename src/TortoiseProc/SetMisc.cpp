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
	, m_bAutocompletion(FALSE)
	, m_dwAutocompletionTimeout(0)
	, m_bSpell(TRUE)
	, m_bCheckRepo(FALSE)
{
	m_regUnversionedRecurse = CRegDWORD(_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE);
	m_bUnversionedRecurse = (DWORD)m_regUnversionedRecurse;
	m_regAutocompletion = CRegDWORD(_T("Software\\TortoiseSVN\\Autocompletion"), TRUE);
	m_bAutocompletion = (DWORD)m_regAutocompletion;
	m_regAutocompletionTimeout = CRegDWORD(_T("Software\\TortoiseSVN\\AutocompleteParseTimeout"), 5);
	m_dwAutocompletionTimeout = (DWORD)m_regAutocompletionTimeout;
	m_regSpell = CRegDWORD(_T("Software\\TortoiseSVN\\Spellchecker"), FALSE);
	m_bSpell = (DWORD)m_regSpell;
	m_regCheckRepo = CRegDWORD(_T("Software\\TortoiseSVN\\CheckRepo"), FALSE);
	m_bCheckRepo = (DWORD)m_regCheckRepo;
}

CSetMisc::~CSetMisc()
{
}

void CSetMisc::SaveData()
{
	m_regUnversionedRecurse = m_bUnversionedRecurse;
	m_regAutocompletion = m_bAutocompletion;
	m_regAutocompletionTimeout = m_dwAutocompletionTimeout;
	m_regSpell = m_bSpell;
	m_regCheckRepo = m_bCheckRepo;
}

void CSetMisc::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UNVERSIONEDRECURSE, m_bUnversionedRecurse);
	DDX_Check(pDX, IDC_AUTOCOMPLETION, m_bAutocompletion);
	DDX_Text(pDX, IDC_AUTOCOMPLETIONTIMEOUT, m_dwAutocompletionTimeout);
	DDV_MinMaxUInt(pDX, m_dwAutocompletionTimeout, 1, 100);
	DDX_Check(pDX, IDC_SPELL, m_bSpell);
	DDX_Check(pDX, IDC_REPOCHECK, m_bCheckRepo);
}


BEGIN_MESSAGE_MAP(CSetMisc, CPropertyPage)
	ON_BN_CLICKED(IDC_UNVERSIONEDRECURSE, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_AUTOCOMPLETION, &CSetMisc::OnChanged)
	ON_EN_CHANGE(IDC_AUTOCOMPLETIONTIMEOUT, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_SPELL, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_REPOCHECK, &CSetMisc::OnChanged)
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
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUT, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUTLABEL, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_SPELL, IDS_SETTINGS_SPELLCHECKER_TT);
	m_tooltips.AddTool(IDC_REPOCHECK, IDS_SETTINGS_REPOCHECK_TT);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetMisc::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}
