// SetMainPage.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseMerge.h"
#include "DirFileList.h"
#include "version.h"
#include "Utils.h"
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
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	m_regBackup = CRegDWORD(_T("Software\\TortoiseMerge\\Backup"));
	m_regFirstDiffOnLoad = CRegDWORD(_T("Software\\TortoiseMerge\\FirstDiffOnLoad"));
	m_regTabSize = CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	m_regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreEOL"), TRUE);	
	m_regOnePane = CRegDWORD(_T("Software\\TortoiseMerge\\OnePane"));
	m_regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	
	m_dwLanguage = m_regLanguage;
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
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
}

void CSetMainPage::SaveData()
{
	m_regLanguage = m_dwLanguage;
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

	//set up the language selecting combobox
	m_LanguageCombo.AddString(_T("English"));
	m_LanguageCombo.SetItemData(0, 1033);
	CRegString str(_T("Software\\TortoiseSVN\\Directory"),_T(""), FALSE, HKEY_LOCAL_MACHINE);
	CString path = str;
	path = path + _T("\\Languages\\");
	CDirFileList list;
	list.BuildList(path, FALSE, FALSE);
	int langcount = 1;
	for (int i=0; i<list.GetCount(); i++)
	{
		CString file = list.GetAt(i);
		if (file.Right(3).CompareNoCase(_T("dll"))==0)
		{
			CString filename = file.Mid(file.ReverseFind('\\')+1);
			if (filename.Left(12).CompareNoCase(_T("TortoiseMerge"))==0)
			{
				if (CUtils::GetVersionFromFile(file).Compare(_T(STRPRODUCTVER_INCVERSION))!=0)
					continue;
				DWORD loc = _tstoi(filename.Mid(12));
				TCHAR buf[MAX_PATH];
				GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, sizeof(buf)/sizeof(TCHAR));
				m_LanguageCombo.AddString(buf);
				m_LanguageCombo.SetItemData(langcount++, loc);
			} // if (filename.Left(12).CompareNoCase(_T("TortoiseProc"))==0) 
		} // if (file.Right(3).CompareNoCase(_T("dll"))==0) 
	} // for (int i=0; i<list.GetCount(); i++) 
	
	for (int i=0; i<m_LanguageCombo.GetCount(); i++)
	{
		if (m_LanguageCombo.GetItemData(i) == m_dwLanguage)
			m_LanguageCombo.SetCurSel(i);
	} // for (int i=0; i<m_LanguageCombo.GetCount(); i++) 

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
