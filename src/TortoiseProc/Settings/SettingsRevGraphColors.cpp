// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "SettingsRevGraphColors.h"
#include ".\settingscolors.h"

IMPLEMENT_DYNAMIC(CSettingsRevisionGraphColors, ISettingsPropPage)
CSettingsRevisionGraphColors::CSettingsRevisionGraphColors()
	: ISettingsPropPage(CSettingsRevisionGraphColors::IDD)
{
}

CSettingsRevisionGraphColors::~CSettingsRevisionGraphColors()
{
}

void CSettingsRevisionGraphColors::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DELETEDNODECOLOR, m_cDeletedNode);
	DDX_Control(pDX, IDC_ADDEDNODECOLOR, m_cAddedNode);
	DDX_Control(pDX, IDC_RENAMEDNODECOLOR, m_cRenamedNode);
	DDX_Control(pDX, IDC_MODIFIEDNODECOLOR, m_cModifiedNode);
    DDX_Control(pDX, IDC_UNCHANGEDNODECOLOR, m_cUnchangedNode);
    DDX_Control(pDX, IDC_LASTCOMMITNODECOLOR, m_cLastCommitNode);
    DDX_Control(pDX, IDC_WCNODECOLOR, m_cWCNode);
    DDX_Control(pDX, IDC_WCNODEBORDERCOLOR, m_cWCNodeBorder);

    DDX_Control(pDX, IDC_TAGOVERLAYCOLOR, m_cTagOverlay);
    DDX_Control(pDX, IDC_TRUNKOVERLAYCOLOR, m_cTrunkOverlay);

    DDX_Control(pDX, IDC_TAGMARKERCOLOR, m_cTagMarker);
    DDX_Control(pDX, IDC_TRUNKMARKERCOLOR, m_cTrunkMarker);

    DDX_Control(pDX, IDC_STRIPECOLOR1, m_cStripeColor1);
    DDX_Control(pDX, IDC_STRIPECOLOR2, m_cStripeColor2);
	DDX_Text(pDX, IDC_STRIPEALPHA1, m_sStripeAlpha1);
	DDX_Text(pDX, IDC_STRIPEALPHA2, m_sStripeAlpha2);
}


BEGIN_MESSAGE_MAP(CSettingsRevisionGraphColors, ISettingsPropPage)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)

	ON_BN_CLICKED(IDC_DELETEDNODECOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
	ON_BN_CLICKED(IDC_ADDEDNODECOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
	ON_BN_CLICKED(IDC_RENAMEDNODECOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
	ON_BN_CLICKED(IDC_MODIFIEDNODECOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_UNCHANGEDNODECOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_LASTCOMMITNODECOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_WCNODECOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_WCNODEBORDERCOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)

    ON_BN_CLICKED(IDC_TAGOVERLAYCOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_TRUNKOVERLAYCOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)

    ON_BN_CLICKED(IDC_TAGMARKERCOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_TRUNKMARKERCOLOR, &CSettingsRevisionGraphColors::OnBnClickedColor)

    ON_BN_CLICKED(IDC_STRIPECOLOR1, &CSettingsRevisionGraphColors::OnBnClickedColor)
    ON_BN_CLICKED(IDC_STRIPECOLOR2, &CSettingsRevisionGraphColors::OnBnClickedColor)
	ON_EN_CHANGE(IDC_STRIPEALPHA1, &CSettingsRevisionGraphColors::OnBnClickedColor)
	ON_EN_CHANGE(IDC_STRIPEALPHA2, &CSettingsRevisionGraphColors::OnBnClickedColor)
END_MESSAGE_MAP()

void CSettingsRevisionGraphColors::InitColorPicker 
    ( CMFCColorButton& button
    , CColors::GDIPlusColor color)
{
	CString sDefaultText;
    CString sCustomText;
	sDefaultText.LoadString (IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString (IDS_COLOURPICKER_CUSTOMTEXT);

	button.SetColor (m_Colors.GetColor (color).ToCOLORREF());
	button.EnableAutomaticButton (sDefaultText, m_Colors.GetColor (color, true).ToCOLORREF());
	button.EnableOtherButton (sCustomText);
}

void CSettingsRevisionGraphColors::InitColorPicker 
    ( CMFCColorButton& button
    , CColors::GDIPlusColorTable table
    , int index)
{
	CString sDefaultText;
    CString sCustomText;
	sDefaultText.LoadString (IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString (IDS_COLOURPICKER_CUSTOMTEXT);

	button.SetColor (m_Colors.GetColor (table, index).ToCOLORREF());
	button.EnableAutomaticButton (sDefaultText, m_Colors.GetColor (table, index, true).ToCOLORREF());
	button.EnableOtherButton (sCustomText);
}

BOOL CSettingsRevisionGraphColors::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

    InitColorPicker (m_cAddedNode, CColors::gdpAddedNode);
    InitColorPicker (m_cDeletedNode, CColors::gdpDeletedNode);
    InitColorPicker (m_cRenamedNode, CColors::gdpRenamedNode);
    InitColorPicker (m_cModifiedNode, CColors::gdpModifiedNode);
    InitColorPicker (m_cUnchangedNode, CColors::gdpUnchangedNode);
    InitColorPicker (m_cLastCommitNode, CColors::gdpLastCommitNode);
    InitColorPicker (m_cWCNode, CColors::gdpWCNode);
    InitColorPicker (m_cWCNodeBorder, CColors::gdpWCNodeBorder);

    InitColorPicker (m_cTagOverlay, CColors::gdpTagOverlay);
    InitColorPicker (m_cTrunkOverlay, CColors::gdpTrunkOverlay);

    InitColorPicker (m_cTagMarker, CColors::ctMarkers, 0);
    InitColorPicker (m_cTrunkMarker, CColors::ctMarkers, 1);

    InitColorPicker (m_cStripeColor1, CColors::gdpStripeColor1);
    InitColorPicker (m_cStripeColor2, CColors::gdpStripeColor2);
    m_sStripeAlpha1 = m_Colors.GetColor (CColors::gdpStripeColor1).GetA();
    m_sStripeAlpha2 = m_Colors.GetColor (CColors::gdpStripeColor2).GetA();

    UpdateData(FALSE);

	return TRUE;
}

void CSettingsRevisionGraphColors::ResetColor 
    ( CMFCColorButton& button
    , CColors::GDIPlusColor color)
{
	button.SetColor (m_Colors.GetColor (color, true).ToCOLORREF());
}

void CSettingsRevisionGraphColors::ResetColor 
    ( CMFCColorButton& button
    , CColors::GDIPlusColorTable table
    , int index)
{
	button.SetColor (m_Colors.GetColor (table, index, true).ToCOLORREF());
}

void CSettingsRevisionGraphColors::OnBnClickedRestore()
{
    ResetColor (m_cAddedNode, CColors::gdpAddedNode);
    ResetColor (m_cDeletedNode, CColors::gdpDeletedNode);
    ResetColor (m_cRenamedNode, CColors::gdpRenamedNode);
    ResetColor (m_cModifiedNode, CColors::gdpModifiedNode);
    ResetColor (m_cUnchangedNode, CColors::gdpUnchangedNode);
    ResetColor (m_cLastCommitNode, CColors::gdpLastCommitNode);
    ResetColor (m_cWCNode, CColors::gdpWCNode);
    ResetColor (m_cWCNodeBorder, CColors::gdpWCNodeBorder);

    ResetColor (m_cTagOverlay, CColors::gdpTagOverlay);
    ResetColor (m_cTrunkOverlay, CColors::gdpTrunkOverlay);

    ResetColor (m_cTagMarker, CColors::ctMarkers, 0);
    ResetColor (m_cTrunkMarker, CColors::ctMarkers, 1);

    ResetColor (m_cStripeColor1, CColors::gdpStripeColor1);
    ResetColor (m_cStripeColor2, CColors::gdpStripeColor2);

    m_sStripeAlpha1 = m_Colors.GetColor (CColors::gdpStripeColor1, true).GetA();
    m_sStripeAlpha2 = m_Colors.GetColor (CColors::gdpStripeColor2, true).GetA();

    UpdateData (FALSE);

	SetModified(TRUE);
}

void CSettingsRevisionGraphColors::ApplyColor 
    ( CMFCColorButton& button
    , CColors::GDIPlusColor color
    , DWORD alpha)
{
    COLORREF value = button.GetColor() == -1 
                   ? button.GetAutomaticColor() 
                   : button.GetColor();

    Gdiplus::Color temp;
    temp.SetFromCOLORREF (value);
	m_Colors.SetColor (color, (temp.GetValue() & 0xffffff) + (alpha << 24));
}

void CSettingsRevisionGraphColors::ApplyColor 
    ( CMFCColorButton& button
    , CColors::GDIPlusColorTable table
    , int index)
{
    COLORREF value = button.GetColor() == -1 
                   ? button.GetAutomaticColor() 
                   : button.GetColor();

    Gdiplus::Color temp;
    temp.SetFromCOLORREF (value);
	m_Colors.SetColor (table, index, (temp.GetValue() & 0xffffff) + 0xff000000);
}

BOOL CSettingsRevisionGraphColors::OnApply()
{
    UpdateData();

    ApplyColor (m_cAddedNode, CColors::gdpAddedNode);
    ApplyColor (m_cDeletedNode, CColors::gdpDeletedNode);
    ApplyColor (m_cRenamedNode, CColors::gdpRenamedNode);
    ApplyColor (m_cModifiedNode, CColors::gdpModifiedNode);
    ApplyColor (m_cUnchangedNode, CColors::gdpUnchangedNode);
    ApplyColor (m_cLastCommitNode, CColors::gdpLastCommitNode);
    ApplyColor (m_cWCNode, CColors::gdpWCNode);
    ApplyColor (m_cWCNodeBorder, CColors::gdpWCNodeBorder);

    ApplyColor (m_cTagOverlay, CColors::gdpTagOverlay, 0x80);
    ApplyColor (m_cTrunkOverlay, CColors::gdpTrunkOverlay, 0x40);

    ApplyColor (m_cTagMarker, CColors::ctMarkers, 0);
    ApplyColor (m_cTrunkMarker, CColors::ctMarkers, 1);

    ApplyColor (m_cStripeColor1, CColors::gdpStripeColor1, m_sStripeAlpha1);
    ApplyColor (m_cStripeColor2, CColors::gdpStripeColor2, m_sStripeAlpha2);

    return ISettingsPropPage::OnApply();
}

void CSettingsRevisionGraphColors::OnBnClickedColor()
{
	SetModified();
}
