// SetMainPage.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseMerge.h"
#include "SetMainPage.h"
#include ".\setmainpage.h"


// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_bBackup(FALSE)
	, m_bFirstDiffOnLoad(FALSE)
	, m_nTabSize(0)
	, m_bIgnoreEOL(FALSE)
	, m_bOnePane(FALSE)
{
	m_regBackup = CRegDWORD(_T("Software\\TortoiseMerge\\Backup"));
	m_regFirstDiffOnLoad = CRegDWORD(_T("Software\\TortoiseMerge\\FirstDiffOnLoad"));
	m_regTabSize = CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	m_regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreEOL"), TRUE);	
	m_regOnePane = CRegDWORD(_T("Software\\TortoiseMerge\\OnePane"));
	m_regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	
	m_bBackup = m_regBackup;
	m_bFirstDiffOnLoad = m_regFirstDiffOnLoad;
	m_nTabSize = m_regTabSize;
	m_bIgnoreEOL = m_regIgnoreEOL;
	m_bOnePane = m_regOnePane;
	m_nIgnoreWS = m_regIgnoreWS;
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_BACKUP, m_bBackup);
	DDX_Check(pDX, IDC_FIRSTDIFFONLOAD, m_bFirstDiffOnLoad);
	DDX_Text(pDX, IDC_TABSIZE, m_nTabSize);
	DDV_MinMaxInt(pDX, m_nTabSize, 1, 1000);
	DDX_Check(pDX, IDC_IGNORELF, m_bIgnoreEOL);
	DDX_Check(pDX, IDC_ONEPANE, m_bOnePane);
}

void CSetMainPage::SaveData()
{
	m_regBackup = m_bBackup;
	m_regFirstDiffOnLoad = m_bFirstDiffOnLoad;
	m_regTabSize = m_nTabSize;
	m_regIgnoreEOL = m_bIgnoreEOL;
	m_regOnePane = m_bOnePane;
	m_regIgnoreWS = m_nIgnoreWS;
}

BOOL CSetMainPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

BOOL CSetMainPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UINT uRadio = IDC_WSIGNORELEADING;
	switch (m_nIgnoreWS)
	{
	case 0:
		uRadio = IDC_WSCOMPARE;
		break;
	case 1:
		uRadio = IDC_WSIGNOREALL;
		break;
	case 2:
		uRadio = IDC_WSIGNORELEADING;
		break;	
	default:
		break;	
	}

	CheckRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL, uRadio);

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_BN_CLICKED(IDC_BACKUP, OnBnClickedBackup)
	ON_BN_CLICKED(IDC_IGNORELF, OnBnClickedIgnorelf)
	ON_BN_CLICKED(IDC_ONEPANE, OnBnClickedOnepane)
	ON_BN_CLICKED(IDC_FIRSTDIFFONLOAD, OnBnClickedFirstdiffonload)
	ON_BN_CLICKED(IDC_WSCOMPARE, OnBnClickedWscompare)
	ON_BN_CLICKED(IDC_WSIGNORELEADING, OnBnClickedWsignoreleading)
	ON_BN_CLICKED(IDC_WSIGNOREALL, OnBnClickedWsignoreall)
	ON_EN_CHANGE(IDC_TABSIZE, OnEnChangeTabsize)
END_MESSAGE_MAP()


// CSetMainPage message handlers

void CSetMainPage::OnBnClickedBackup()
{
	SetModified();
}

void CSetMainPage::OnBnClickedIgnorelf()
{
	SetModified();
}

void CSetMainPage::OnBnClickedOnepane()
{
	SetModified();
}

void CSetMainPage::OnBnClickedFirstdiffonload()
{
	SetModified();
}

void CSetMainPage::OnBnClickedWscompare()
{
	SetModified();
	UINT uRadio = GetCheckedRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL);
	switch (uRadio)
	{
	case IDC_WSCOMPARE:
		m_nIgnoreWS = 0;
		break;
	case IDC_WSIGNOREALL:
		m_nIgnoreWS = 1;
		break;
	case IDC_WSIGNORELEADING:
		m_nIgnoreWS = 2;
		break;	
	default:
		break;	
	}
}

void CSetMainPage::OnBnClickedWsignoreleading()
{
	SetModified();
	UINT uRadio = GetCheckedRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL);
	switch (uRadio)
	{
	case IDC_WSCOMPARE:
		m_nIgnoreWS = 0;
		break;
	case IDC_WSIGNOREALL:
		m_nIgnoreWS = 1;
		break;
	case IDC_WSIGNORELEADING:
		m_nIgnoreWS = 2;
		break;	
	default:
		break;	
	}
}

void CSetMainPage::OnBnClickedWsignoreall()
{
	SetModified();
	UINT uRadio = GetCheckedRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL);
	switch (uRadio)
	{
	case IDC_WSCOMPARE:
		m_nIgnoreWS = 0;
		break;
	case IDC_WSIGNOREALL:
		m_nIgnoreWS = 1;
		break;
	case IDC_WSIGNORELEADING:
		m_nIgnoreWS = 2;
		break;	
	default:
		break;	
	}
}

void CSetMainPage::OnEnChangeTabsize()
{
	SetModified();
}
