// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2013 - TortoiseSVN

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
#include "SettingsTBlame.h"

// CSettingsTBlame dialog

IMPLEMENT_DYNAMIC(CSettingsTBlame, ISettingsPropPage)

CSettingsTBlame::CSettingsTBlame()
    : ISettingsPropPage(CSettingsTBlame::IDD)
    , m_dwFontSize(0)
    , m_sFontName(_T(""))
    , m_dwTabSize(4)
{
    m_regNewLinesColor = CRegDWORD(_T("Software\\TortoiseSVN\\BlameNewColor"), BLAMENEWCOLOR);
    m_regOldLinesColor = CRegDWORD(_T("Software\\TortoiseSVN\\BlameOldColor"), BLAMEOLDCOLOR);
    m_regNewLinesColorBar = CRegDWORD(_T("Software\\TortoiseSVN\\BlameLocatorNewColor"), BLAMENEWCOLORBAR);
    m_regOldLinesColorBar = CRegDWORD(_T("Software\\TortoiseSVN\\BlameLocatorOldColor"), BLAMEOLDCOLORBAR);
    m_regFontName = CRegString(_T("Software\\TortoiseSVN\\BlameFontName"), _T("Courier New"));
    m_regFontSize = CRegDWORD(_T("Software\\TortoiseSVN\\BlameFontSize"), 10);
    m_regTabSize = CRegDWORD(_T("Software\\TortoiseSVN\\BlameTabSize"), 4);

    m_regIndexColors[0] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor1", BLAMEINDEXCOLOR1);
    m_regIndexColors[1] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor2", BLAMEINDEXCOLOR2);
    m_regIndexColors[2] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor3", BLAMEINDEXCOLOR3);
    m_regIndexColors[3] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor4", BLAMEINDEXCOLOR4);
    m_regIndexColors[4] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor5", BLAMEINDEXCOLOR5);
    m_regIndexColors[5] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor6", BLAMEINDEXCOLOR6);
    m_regIndexColors[6] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor7", BLAMEINDEXCOLOR7);
    m_regIndexColors[7] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor8", BLAMEINDEXCOLOR8);
    m_regIndexColors[8] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor9", BLAMEINDEXCOLOR9);
    m_regIndexColors[9] =  CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor10", BLAMEINDEXCOLOR10);
    m_regIndexColors[10] = CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor11", BLAMEINDEXCOLOR11);
    m_regIndexColors[11] = CRegDWORD(L"Software\\TortoiseSVN\\BlameIndexColor12", BLAMEINDEXCOLOR12);
}

CSettingsTBlame::~CSettingsTBlame()
{
}

void CSettingsTBlame::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_NEWLINESCOLOR, m_cNewLinesColor);
    DDX_Control(pDX, IDC_OLDLINESCOLOR, m_cOldLinesColor);
    DDX_Control(pDX, IDC_NEWLINESCOLORBAR, m_cNewLinesColorBar);
    DDX_Control(pDX, IDC_OLDLINESCOLORBAR, m_cOldLinesColorBar);
    DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
    m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
    if ((m_dwFontSize==0)||(m_dwFontSize == -1))
    {
        CString t;
        m_cFontSizes.GetWindowText(t);
        m_dwFontSize = _ttoi(t);
    }
    DDX_Control(pDX, IDC_FONTNAMES, m_cFontNames);
    DDX_Text(pDX, IDC_TABSIZE, m_dwTabSize);

    DDX_Control(pDX, IDC_INDEXCOLOR1,  m_indexColors[0]);
    DDX_Control(pDX, IDC_INDEXCOLOR2,  m_indexColors[1]);
    DDX_Control(pDX, IDC_INDEXCOLOR3,  m_indexColors[2]);
    DDX_Control(pDX, IDC_INDEXCOLOR4,  m_indexColors[3]);
    DDX_Control(pDX, IDC_INDEXCOLOR5,  m_indexColors[4]);
    DDX_Control(pDX, IDC_INDEXCOLOR6,  m_indexColors[5]);
    DDX_Control(pDX, IDC_INDEXCOLOR7,  m_indexColors[6]);
    DDX_Control(pDX, IDC_INDEXCOLOR8,  m_indexColors[7]);
    DDX_Control(pDX, IDC_INDEXCOLOR9,  m_indexColors[8]);
    DDX_Control(pDX, IDC_INDEXCOLOR10, m_indexColors[9]);
    DDX_Control(pDX, IDC_INDEXCOLOR11, m_indexColors[10]);
    DDX_Control(pDX, IDC_INDEXCOLOR12, m_indexColors[11]);
}


BEGIN_MESSAGE_MAP(CSettingsTBlame, ISettingsPropPage)
    ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
    ON_CBN_SELCHANGE(IDC_FONTSIZES, OnChange)
    ON_CBN_SELCHANGE(IDC_FONTNAMES, OnChange)
    ON_EN_CHANGE(IDC_TABSIZE, OnChange)
    ON_BN_CLICKED(IDC_NEWLINESCOLOR, &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_OLDLINESCOLOR, &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_NEWLINESCOLORBAR, &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_OLDLINESCOLORBAR, &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR1,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR2,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR3,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR4,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR5,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR6,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR7,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR8,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR9,  &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR10, &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR11, &CSettingsTBlame::OnBnClickedColor)
    ON_BN_CLICKED(IDC_INDEXCOLOR12, &CSettingsTBlame::OnBnClickedColor)
END_MESSAGE_MAP()


// CSettingsTBlame message handlers

BOOL CSettingsTBlame::OnInitDialog()
{
    CMFCFontComboBox::m_bDrawUsingFont = true;

    ISettingsPropPage::OnInitDialog();

    m_cNewLinesColor.SetColor((DWORD)m_regNewLinesColor);
    m_cOldLinesColor.SetColor((DWORD)m_regOldLinesColor);
    m_cNewLinesColorBar.SetColor((DWORD)m_regNewLinesColorBar);
    m_cOldLinesColorBar.SetColor((DWORD)m_regOldLinesColorBar);

    CString sDefaultText, sCustomText;
    sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
    sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);
    m_cNewLinesColor.EnableAutomaticButton(sDefaultText, BLAMENEWCOLOR);
    m_cNewLinesColor.EnableOtherButton(sCustomText);
    m_cOldLinesColor.EnableAutomaticButton(sDefaultText, BLAMEOLDCOLOR);
    m_cOldLinesColor.EnableOtherButton(sCustomText);
    m_cNewLinesColorBar.EnableAutomaticButton(sDefaultText, BLAMENEWCOLORBAR);
    m_cNewLinesColorBar.EnableOtherButton(sCustomText);
    m_cOldLinesColorBar.EnableAutomaticButton(sDefaultText, BLAMEOLDCOLORBAR);
    m_cOldLinesColorBar.EnableOtherButton(sCustomText);

    for (int i = 0; i < MAX_BLAMECOLORS; ++i)
    {
        m_indexColors[i].EnableOtherButton(sCustomText);
    }
    m_indexColors[0].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR1);
    m_indexColors[1].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR2);
    m_indexColors[2].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR3);
    m_indexColors[3].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR4);
    m_indexColors[4].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR5);
    m_indexColors[5].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR6);
    m_indexColors[6].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR7);
    m_indexColors[7].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR8);
    m_indexColors[8].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR9);
    m_indexColors[9].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR10);
    m_indexColors[10].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR11);
    m_indexColors[11].EnableAutomaticButton(sDefaultText, BLAMEINDEXCOLOR12);
    m_indexColors[0].SetColor(BLAMEINDEXCOLOR1);
    m_indexColors[1].SetColor(BLAMEINDEXCOLOR2);
    m_indexColors[2].SetColor(BLAMEINDEXCOLOR3);
    m_indexColors[3].SetColor(BLAMEINDEXCOLOR4);
    m_indexColors[4].SetColor(BLAMEINDEXCOLOR5);
    m_indexColors[5].SetColor(BLAMEINDEXCOLOR6);
    m_indexColors[6].SetColor(BLAMEINDEXCOLOR7);
    m_indexColors[7].SetColor(BLAMEINDEXCOLOR8);
    m_indexColors[8].SetColor(BLAMEINDEXCOLOR9);
    m_indexColors[9].SetColor(BLAMEINDEXCOLOR10);
    m_indexColors[10].SetColor(BLAMEINDEXCOLOR11);
    m_indexColors[11].SetColor(BLAMEINDEXCOLOR12);


    m_dwTabSize = m_regTabSize;
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
        temp.Format(_T("%lu"), m_dwFontSize);
        m_cFontSizes.SetWindowText(temp);
    }
    m_cFontNames.Setup(DEVICE_FONTTYPE|RASTER_FONTTYPE|TRUETYPE_FONTTYPE, 1, FIXED_PITCH);
    m_cFontNames.SelectFont(m_sFontName);

    UpdateData(FALSE);
    return TRUE;
}

void CSettingsTBlame::OnChange()
{
    SetModified();
}

void CSettingsTBlame::OnBnClickedRestore()
{
    m_cOldLinesColor.SetColor(BLAMEOLDCOLOR);
    m_cNewLinesColor.SetColor(BLAMENEWCOLOR);
    m_cOldLinesColorBar.SetColor(BLAMEOLDCOLORBAR);
    m_cNewLinesColorBar.SetColor(BLAMENEWCOLORBAR);

    m_indexColors[0].SetColor(BLAMEINDEXCOLOR1);
    m_indexColors[1].SetColor(BLAMEINDEXCOLOR2);
    m_indexColors[2].SetColor(BLAMEINDEXCOLOR3);
    m_indexColors[3].SetColor(BLAMEINDEXCOLOR4);
    m_indexColors[4].SetColor(BLAMEINDEXCOLOR5);
    m_indexColors[5].SetColor(BLAMEINDEXCOLOR6);
    m_indexColors[6].SetColor(BLAMEINDEXCOLOR7);
    m_indexColors[7].SetColor(BLAMEINDEXCOLOR8);
    m_indexColors[8].SetColor(BLAMEINDEXCOLOR9);
    m_indexColors[9].SetColor(BLAMEINDEXCOLOR10);
    m_indexColors[10].SetColor(BLAMEINDEXCOLOR11);
    m_indexColors[11].SetColor(BLAMEINDEXCOLOR12);

    SetModified(TRUE);
}

BOOL CSettingsTBlame::OnApply()
{
    UpdateData();
    if (m_cFontNames.GetSelFont())
        m_sFontName = m_cFontNames.GetSelFont()->m_strName;
    else
        m_sFontName = m_regFontName;

    Store ((m_cNewLinesColor.GetColor() == -1 ? m_cNewLinesColor.GetAutomaticColor() : m_cNewLinesColor.GetColor()), m_regNewLinesColor);
    Store ((m_cOldLinesColor.GetColor() == -1 ? m_cOldLinesColor.GetAutomaticColor() : m_cOldLinesColor.GetColor()), m_regOldLinesColor);
    Store ((m_cNewLinesColorBar.GetColor() == -1 ? m_cNewLinesColorBar.GetAutomaticColor() : m_cNewLinesColorBar.GetColor()), m_regNewLinesColorBar);
    Store ((m_cOldLinesColorBar.GetColor() == -1 ? m_cOldLinesColorBar.GetAutomaticColor() : m_cOldLinesColorBar.GetColor()), m_regOldLinesColorBar);
    Store ((LPCTSTR)m_sFontName, m_regFontName);
    Store (m_dwFontSize, m_regFontSize);
    Store (m_dwTabSize, m_regTabSize);

    for (int i = 0; i < MAX_BLAMECOLORS; ++i)
        Store ((m_indexColors[i].GetColor() == -1 ? m_indexColors[i].GetAutomaticColor() : m_indexColors[i].GetColor()), m_regIndexColors[i]);

    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSettingsTBlame::OnBnClickedColor()
{
    SetModified();
}
