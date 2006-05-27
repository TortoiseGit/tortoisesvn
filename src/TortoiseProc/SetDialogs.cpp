// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetMainPage.h"
#include "Utils.h"
#include "DirFileEnum.h"
#include "SVNProgressDlg.h"
#include "..\version.h"
#include ".\setdialogs.h"
#include "SVN.h"
#include "MessageBox.h"


// CSetDialogs dialog

IMPLEMENT_DYNAMIC(CSetDialogs, CPropertyPage)
CSetDialogs::CSetDialogs()
	: CPropertyPage(CSetDialogs::IDD)
	, m_sDefaultLogs(_T(""))
	, m_bShortDateFormat(FALSE)
	, m_dwFontSize(0)
	, m_sFontName(_T(""))
	, m_bInitialized(FALSE)
	, m_bUseWCURL(FALSE)
{
	m_regAutoClose = CRegDWORD(_T("Software\\TortoiseSVN\\AutoClose"));
	m_regDefaultLogs = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
	m_regShortDateFormat = CRegDWORD(_T("Software\\TortoiseSVN\\LogDateFormat"), FALSE);
	m_regFontName = CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New"));
	m_regFontSize = CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8);
	m_regUseWCURL = CRegDWORD(_T("Software\\TortoiseSVN\\MergeWCURL"), FALSE);
}

CSetDialogs::~CSetDialogs()
{
}

int CSetDialogs::SaveData()
{
	if (m_bInitialized == FALSE)
		return 0;
	m_regAutoClose = m_dwAutoClose;
	if (m_regAutoClose.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regAutoClose.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regShortDateFormat = m_bShortDateFormat;
	if (m_regShortDateFormat.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regShortDateFormat.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	long val = _ttol(m_sDefaultLogs);
	if (val > 5)
	{
		m_regDefaultLogs = val;
		if (m_regDefaultLogs.LastError != ERROR_SUCCESS)
			CMessageBox::Show(m_hWnd, m_regDefaultLogs.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	}
	else
	{
		m_regDefaultLogs = 100;
		if (m_regDefaultLogs.LastError != ERROR_SUCCESS)
			CMessageBox::Show(m_hWnd, m_regDefaultLogs.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	}

	m_regFontName = m_sFontName;
	if (m_regFontName.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regFontName.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regFontSize = m_dwFontSize;
	if (m_regFontSize.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regFontSize.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regUseWCURL = m_bUseWCURL;
	if (m_regUseWCURL.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regUseWCURL.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	return 0;
}

void CSetDialogs::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _ttoi(t);
	}
	DDX_FontPreviewCombo (pDX, IDC_FONTNAMES, m_sFontName);
	DDX_Text(pDX, IDC_DEFAULTLOG, m_sDefaultLogs);
	DDX_Check(pDX, IDC_SHORTDATEFORMAT, m_bShortDateFormat);
	DDX_Control(pDX, IDC_AUTOCLOSECOMBO, m_cAutoClose);
	DDX_Check(pDX, IDC_WCURLFROM, m_bUseWCURL);
}


BEGIN_MESSAGE_MAP(CSetDialogs, CPropertyPage)
	ON_EN_CHANGE(IDC_DEFAULTLOG, OnEnChangeDefaultlog)
	ON_BN_CLICKED(IDC_SHORTDATEFORMAT, OnBnClickedShortdateformat)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnCbnSelchangeFontsizes)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnCbnSelchangeFontnames)
	ON_CBN_SELCHANGE(IDC_AUTOCLOSECOMBO, OnCbnSelchangeAutoclosecombo)
	ON_BN_CLICKED(IDC_WCURLFROM, OnBnClickedWcurlfrom)
END_MESSAGE_MAP()


// CSetDialogs message handlers
BOOL CSetDialogs::OnInitDialog()
{
	m_cFontNames.SubclassDlgItem (IDC_FONTNAMES, this);
	m_cFontNames.SetFontHeight(16, false);
	m_cFontNames.SetPreviewStyle(CFontPreviewCombo::NAME_THEN_SAMPLE, false);
	m_cFontNames.Init();

	CPropertyPage::OnInitDialog();

	EnableToolTips();

	int ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_MANUAL)));
	m_cAutoClose.SetItemData(ind, CLOSE_MANUAL);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOMERGES)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOMERGES);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOCONFLICTS)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOCONFLICTS);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOERROR)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOERRORS);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_LOCAL)));
	m_cAutoClose.SetItemData(ind, CLOSE_LOCAL);

	m_dwAutoClose = m_regAutoClose;
	m_bShortDateFormat = m_regShortDateFormat;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bUseWCURL = m_regUseWCURL;

	for (int i=0; i<m_cAutoClose.GetCount(); ++i)
		if (m_cAutoClose.GetItemData(i)==m_dwAutoClose)
			m_cAutoClose.SetCurSel(i);

	CString temp;
	temp.Format(_T("%ld"), (DWORD)m_regDefaultLogs);
	m_sDefaultLogs = temp;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_SHORTDATEFORMAT, IDS_SETTINGS_SHORTDATEFORMAT_TT);
	m_tooltips.AddTool(IDC_AUTOCLOSECOMBO, IDS_SETTINGS_AUTOCLOSE_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETION, IDS_SETTINGS_AUTOCOMPLETION_TT);
	m_tooltips.AddTool(IDC_WCURLFROM, IDS_SETTINGS_USEWCURL_TT);
	
	int count = 0;
	for (int i=6; i<32; i=i+2)
	{
		temp.Format(_T("%d"), i);
		m_cFontSizes.AddString(temp);
		m_cFontSizes.SetItemData(count++, i);
	}
	BOOL foundfont = FALSE;
	for (int i=0; i<m_cFontSizes.GetCount(); i++)
	{
		if (m_cFontSizes.GetItemData(i) == m_dwFontSize)
		{
			m_cFontSizes.SetCurSel(i);
			foundfont = TRUE;
		}
	}
	if (!foundfont)
	{
		temp.Format(_T("%d"), m_dwFontSize);
		m_cFontSizes.SetWindowText(temp);
	}
	m_cFontNames.AdjustHeight(&m_cFontSizes);

	m_bInitialized = TRUE;
	
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetDialogs::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CSetDialogs::OnBnClickedShortdateformat()
{
	SetModified();
}

void CSetDialogs::OnEnChangeDefaultlog()
{
	SetModified();
}

void CSetDialogs::OnCbnSelchangeFontsizes()
{
	SetModified();
}

void CSetDialogs::OnCbnSelchangeFontnames()
{
	SetModified();
}

void CSetDialogs::OnBnClickedWcurlfrom()
{
	SetModified();
}

BOOL CSetDialogs::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSetDialogs::OnCbnSelchangeAutoclosecombo()
{
	if (m_cAutoClose.GetCurSel() != CB_ERR)
	{
		m_dwAutoClose = m_cAutoClose.GetItemData(m_cAutoClose.GetCurSel());
	}
	SetModified();
}













