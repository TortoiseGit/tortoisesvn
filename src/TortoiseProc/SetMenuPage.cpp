// SetMenuPage.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetMenuPage.h"
#include ".\setmenupage.h"
#include "Globals.h"

// CSetMenuPage dialog

IMPLEMENT_DYNAMIC(CSetMenuPage, CPropertyPage)
CSetMenuPage::CSetMenuPage()
	: CPropertyPage(CSetMenuPage::IDD)
	, m_bMenu1(FALSE)
	, m_bMenu2(FALSE)
	, m_bMenu3(FALSE)
	, m_bMenu4(FALSE)
	, m_bMenu5(FALSE)
	, m_bMenu6(FALSE)
	, m_bMenu7(FALSE)
	, m_bMenu8(FALSE)
	, m_bMenu9(FALSE)
	, m_bMenu10(FALSE)
	, m_bMenu11(FALSE)
	, m_bMenu12(FALSE)
	, m_bMenu13(FALSE)
	, m_bMenu14(FALSE)
	, m_bMenu15(FALSE)
	, m_bMenu16(FALSE)
	, m_bMenu17(FALSE)
	, m_bMenu18(FALSE)
	, m_bMenu19(FALSE)
	, m_bMenu20(FALSE)
	, m_bMenu21(FALSE)
	, m_bMenu22(FALSE)
	, m_bMenu23(FALSE)
{
	DWORD topmenu = CRegDWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);

	m_bMenu1 = (topmenu & MENUCHECKOUT) ? TRUE : FALSE;
	m_bMenu2 = (topmenu & MENUUPDATE) ? TRUE : FALSE;
	m_bMenu3 = (topmenu & MENUCOMMIT) ? TRUE : FALSE;
	m_bMenu4 = (topmenu & MENUDIFF) ? TRUE : FALSE;
	m_bMenu5 = (topmenu & MENULOG) ? TRUE : FALSE;
	m_bMenu6 = (topmenu & MENUSHOWCHANGED) ? TRUE : FALSE;
	m_bMenu7 = (topmenu & MENUCONFLICTEDITOR) ? TRUE : FALSE;
	m_bMenu8 = (topmenu & MENUUPDATEEXT) ? TRUE : FALSE;
	m_bMenu9 = (topmenu & MENURENAME) ? TRUE : FALSE;
	m_bMenu10 = (topmenu & MENUREMOVE) ? TRUE : FALSE;
	m_bMenu11 = (topmenu & MENURESOLVE) ? TRUE : FALSE;
	m_bMenu12 = (topmenu & MENUREVERT) ? TRUE : FALSE;
	m_bMenu13 = (topmenu & MENUCLEANUP) ? TRUE : FALSE;
	m_bMenu14 = (topmenu & MENUCOPY) ? TRUE : FALSE;
	m_bMenu15 = (topmenu & MENUSWITCH) ? TRUE : FALSE;
	m_bMenu16 = (topmenu & MENUMERGE) ? TRUE : FALSE;
	m_bMenu17 = (topmenu & MENUEXPORT) ? TRUE : FALSE;
	m_bMenu18 = (topmenu & MENURELOCATE) ? TRUE : FALSE;
	m_bMenu19 = (topmenu & MENUCREATEREPOS) ? TRUE : FALSE;
	m_bMenu20 = (topmenu & MENUADD) ? TRUE : FALSE;
	m_bMenu21 = (topmenu & MENUIMPORT) ? TRUE : FALSE;
	m_bMenu22 = (topmenu & MENUIGNORE) ? TRUE : FALSE;
	m_bMenu23 = (topmenu & MENUREPOBROWSE) ? TRUE : FALSE;

	this->m_pPSP->dwFlags &= ~PSP_HASHELP;
}

CSetMenuPage::~CSetMenuPage()
{
}

void CSetMenuPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MENU1, m_bMenu1);
	DDX_Check(pDX, IDC_MENU2, m_bMenu2);
	DDX_Check(pDX, IDC_MENU3, m_bMenu3);
	DDX_Check(pDX, IDC_MENU4, m_bMenu4);
	DDX_Check(pDX, IDC_MENU5, m_bMenu5);
	DDX_Check(pDX, IDC_MENU6, m_bMenu6);
	DDX_Check(pDX, IDC_MENU7, m_bMenu7);
	DDX_Check(pDX, IDC_MENU8, m_bMenu8);
	DDX_Check(pDX, IDC_MENU9, m_bMenu9);
	DDX_Check(pDX, IDC_MENU10, m_bMenu10);
	DDX_Check(pDX, IDC_MENU11, m_bMenu11);
	DDX_Check(pDX, IDC_MENU12, m_bMenu12);
	DDX_Check(pDX, IDC_MENU13, m_bMenu13);
	DDX_Check(pDX, IDC_MENU14, m_bMenu14);
	DDX_Check(pDX, IDC_MENU15, m_bMenu15);
	DDX_Check(pDX, IDC_MENU16, m_bMenu16);
	DDX_Check(pDX, IDC_MENU17, m_bMenu17);
	DDX_Check(pDX, IDC_MENU18, m_bMenu18);
	DDX_Check(pDX, IDC_MENU19, m_bMenu19);
	DDX_Check(pDX, IDC_MENU20, m_bMenu20);
	DDX_Check(pDX, IDC_MENU21, m_bMenu21);
	DDX_Check(pDX, IDC_MENU22, m_bMenu22);
	DDX_Check(pDX, IDC_MENU23, m_bMenu23);
}


BEGIN_MESSAGE_MAP(CSetMenuPage, CPropertyPage)
	ON_BN_CLICKED(IDC_MENU1, OnBnClickedMenu1)
	ON_BN_CLICKED(IDC_MENU2, OnBnClickedMenu2)
	ON_BN_CLICKED(IDC_MENU3, OnBnClickedMenu3)
	ON_BN_CLICKED(IDC_MENU4, OnBnClickedMenu4)
	ON_BN_CLICKED(IDC_MENU5, OnBnClickedMenu5)
	ON_BN_CLICKED(IDC_MENU6, OnBnClickedMenu6)
	ON_BN_CLICKED(IDC_MENU7, OnBnClickedMenu7)
	ON_BN_CLICKED(IDC_MENU8, OnBnClickedMenu8)
	ON_BN_CLICKED(IDC_MENU9, OnBnClickedMenu9)
	ON_BN_CLICKED(IDC_MENU10, OnBnClickedMenu10)
	ON_BN_CLICKED(IDC_MENU11, OnBnClickedMenu11)
	ON_BN_CLICKED(IDC_MENU12, OnBnClickedMenu12)
	ON_BN_CLICKED(IDC_MENU13, OnBnClickedMenu13)
	ON_BN_CLICKED(IDC_MENU14, OnBnClickedMenu14)
	ON_BN_CLICKED(IDC_MENU15, OnBnClickedMenu15)
	ON_BN_CLICKED(IDC_MENU16, OnBnClickedMenu16)
	ON_BN_CLICKED(IDC_MENU17, OnBnClickedMenu17)
	ON_BN_CLICKED(IDC_MENU18, OnBnClickedMenu18)
	ON_BN_CLICKED(IDC_MENU19, OnBnClickedMenu19)
	ON_BN_CLICKED(IDC_MENU20, OnBnClickedMenu20)
	ON_BN_CLICKED(IDC_MENU21, OnBnClickedMenu21)
	ON_BN_CLICKED(IDC_MENU22, OnBnClickedMenu22)
	ON_BN_CLICKED(IDC_MENU23, OnBnClickedMenu23)
END_MESSAGE_MAP()

void CSetMenuPage::SaveData()
{
	DWORD topmenu = 0;
	topmenu |= (m_bMenu1 ? MENUCHECKOUT : 0);
	topmenu |= (m_bMenu2 ? MENUUPDATE : 0);
	topmenu |= (m_bMenu3 ? MENUCOMMIT : 0);
	topmenu |= (m_bMenu4 ? MENUDIFF : 0);
	topmenu |= (m_bMenu5 ? MENULOG : 0);
	topmenu |= (m_bMenu6 ? MENUSHOWCHANGED : 0);
	topmenu |= (m_bMenu7 ? MENUCONFLICTEDITOR : 0);
	topmenu |= (m_bMenu8 ? MENUUPDATEEXT : 0);
	topmenu |= (m_bMenu9 ? MENURENAME : 0);
	topmenu |= (m_bMenu10 ? MENUREMOVE : 0);
	topmenu |= (m_bMenu11 ? MENURESOLVE : 0);
	topmenu |= (m_bMenu12 ? MENUREVERT : 0);
	topmenu |= (m_bMenu13 ? MENUCLEANUP : 0);
	topmenu |= (m_bMenu14 ? MENUCOPY : 0);
	topmenu |= (m_bMenu15 ? MENUSWITCH : 0);
	topmenu |= (m_bMenu16 ? MENUMERGE : 0);
	topmenu |= (m_bMenu17 ? MENUEXPORT : 0);
	topmenu |= (m_bMenu18 ? MENURELOCATE : 0);
	topmenu |= (m_bMenu19 ? MENUCREATEREPOS : 0);
	topmenu |= (m_bMenu20 ? MENUADD : 0);
	topmenu |= (m_bMenu21 ? MENUIMPORT : 0);
	topmenu |= (m_bMenu22 ? MENUIGNORE : 0);
	topmenu |= (m_bMenu23 ? MENUREPOBROWSE : 0);
	CRegDWORD regtopmenu = CRegDWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
	regtopmenu = topmenu;
}

// CSetMenuPage message handlers

BOOL CSetMenuPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSetMenuPage::OnBnClickedMenu1()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu2()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu3()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu4()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu5()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu6()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu7()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu8()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu9()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu10()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu11()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu12()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu13()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu14()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu15()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu16()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu17()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu18()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu19()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu20()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu21()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu22()
{
	SetModified();
}

void CSetMenuPage::OnBnClickedMenu23()
{
	SetModified();
}

BOOL CSetMenuPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}
