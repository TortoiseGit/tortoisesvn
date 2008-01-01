// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MessageBox.h"
#include "SettingsTBlame.h"


// CSettingsTBlame dialog

IMPLEMENT_DYNAMIC(CSettingsTBlame, ISettingsPropPage)

CSettingsTBlame::CSettingsTBlame()
	: ISettingsPropPage(CSettingsTBlame::IDD)
	, m_dwFontSize(0)
	, m_sFontName(_T(""))
	, m_dwTabSize(4)
{
	m_regNewLinesColor = CRegStdWORD(_T("Software\\TortoiseSVN\\BlameNewColor"), RGB(255, 230, 230));
	m_regOldLinesColor = CRegStdWORD(_T("Software\\TortoiseSVN\\BlameOldColor"), RGB(230, 230, 255));
	m_regFontName = CRegStdString(_T("Software\\TortoiseSVN\\BlameFontName"), _T("Courier New"));
	m_regFontSize = CRegStdWORD(_T("Software\\TortoiseSVN\\BlameFontSize"), 10);
	m_regTabSize = CRegStdWORD(_T("Software\\TortoiseSVN\\BlameTabSize"), 4);
}

CSettingsTBlame::~CSettingsTBlame()
{
}

void CSettingsTBlame::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NEWLINESCOLOR, m_cNewLinesColor);
	DDX_Control(pDX, IDC_OLDLINESCOLOR, m_cOldLinesColor);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _ttoi(t);
	}
	DDX_FontPreviewCombo (pDX, IDC_FONTNAMES, m_sFontName);
	DDX_Text(pDX, IDC_TABSIZE, m_dwTabSize);
}


BEGIN_MESSAGE_MAP(CSettingsTBlame, ISettingsPropPage)
	ON_MESSAGE(CPN_SELCHANGE, OnChanged)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnChange)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnChange)
	ON_EN_CHANGE(IDC_TABSIZE, OnChange)
END_MESSAGE_MAP()


// CSettingsTBlame message handlers

BOOL CSettingsTBlame::OnInitDialog()
{
	m_cFontNames.SubclassDlgItem (IDC_FONTNAMES, this);
	m_cFontNames.SetFontHeight(16, false);
	m_cFontNames.SetPreviewStyle(CFontPreviewCombo::NAME_THEN_SAMPLE, false);
	m_cFontNames.Init();

	ISettingsPropPage::OnInitDialog();

	m_cNewLinesColor.SetColor((DWORD)m_regNewLinesColor);
	m_cOldLinesColor.SetColor((DWORD)m_regOldLinesColor);

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);
	m_cNewLinesColor.SetDefaultText(sDefaultText);
	m_cNewLinesColor.SetCustomText(sCustomText);
	m_cOldLinesColor.SetDefaultText(sDefaultText);
	m_cOldLinesColor.SetCustomText(sCustomText);

	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	int count = 0;
	CString temp;
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

	UpdateData(FALSE);
	return TRUE;
}

LRESULT CSettingsTBlame::OnChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	SetModified(TRUE);
	return 0;
}

void CSettingsTBlame::OnChange()
{
	SetModified();
}

void CSettingsTBlame::OnBnClickedRestore()
{
	m_cOldLinesColor.SetColor(RGB(230, 230, 255));
	m_cNewLinesColor.SetColor(RGB(255, 230, 230));
	SetModified(TRUE);
}

BOOL CSettingsTBlame::OnApply()
{
	UpdateData();
	m_regNewLinesColor = m_cNewLinesColor.GetColor(TRUE);
	if (m_regNewLinesColor.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regNewLinesColor.getErrorString().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regOldLinesColor = m_cOldLinesColor.GetColor(TRUE);
	if (m_regOldLinesColor.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regOldLinesColor.getErrorString().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regFontName = (LPCTSTR)m_sFontName;
	if (m_regFontName.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regFontName.getErrorString().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regFontSize = m_dwFontSize;
	if (m_regFontSize.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regFontSize.getErrorString().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regTabSize = m_dwTabSize;
	if (m_regTabSize.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regTabSize.getErrorString().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
