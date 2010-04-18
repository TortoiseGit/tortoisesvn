// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "EditPropTSVNLang.h"


CComboBox    CEditPropTSVNLang::m_langCombo;

IMPLEMENT_DYNAMIC(CEditPropTSVNLang, CStandAloneDialog)

CEditPropTSVNLang::CEditPropTSVNLang(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CEditPropTSVNLang::IDD, pParent)
    , m_bKeepEnglish(TRUE)
{

}

CEditPropTSVNLang::~CEditPropTSVNLang()
{
}

void CEditPropTSVNLang::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_FILELISTENGLISH, m_bKeepEnglish);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
    DDX_Control(pDX, IDC_LANGCOMBO, m_langCombo);
}


BEGIN_MESSAGE_MAP(CEditPropTSVNLang, CStandAloneDialog)
END_MESSAGE_MAP()


// CEditPropTSVNLang message handlers

BOOL CEditPropTSVNLang::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_FILELISTENGLISH);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancel(this);

    AdjustControlSize(IDC_FILELISTENGLISH);
    AdjustControlSize(IDC_PROPRECURSIVE);

    // fill the combo box with all available languages
    EnumSystemLocales(EnumLocalesProc, LCID_SUPPORTED);
    int index = m_langCombo.AddString(_T("(none)"));
    m_langCombo.SetItemData(index, 0);

    DWORD projLang = 0;
    for (IT it = m_properties.begin(); it != m_properties.end(); ++it)
    {
        if (it->first.compare(PROJECTPROPNAME_LOGFILELISTLANG) == 0)
        {
            m_bKeepEnglish = it->second.value.size() == 0;
        }
        else if (it->first.compare(PROJECTPROPNAME_PROJECTLANGUAGE) == 0)
        {
            projLang = atoi(it->second.value.c_str());
        }
    }

    for (int i = 0; i < m_langCombo.GetCount(); ++i)
    {
        if (m_langCombo.GetItemData(i) == projLang)
        {
            m_langCombo.SetCurSel(i);
            break;
        }
    }

    UpdateData(false);

    return TRUE;
}

void CEditPropTSVNLang::OnOK()
{
    UpdateData();

    TProperties newProps;
    PropValue pVal;

    char numBuf[20];
    sprintf_s(numBuf, _countof(numBuf), "%ld", m_langCombo.GetItemData(m_langCombo.GetCurSel()));
    pVal.value = numBuf;
    pVal.remove = (m_langCombo.GetItemData(m_langCombo.GetCurSel()) == 0);
    newProps[PROJECTPROPNAME_PROJECTLANGUAGE] = pVal;

    pVal.value = m_bKeepEnglish ? "" : "false";
    pVal.remove = !!m_bKeepEnglish;
    newProps[PROJECTPROPNAME_LOGFILELISTLANG] = pVal;

    m_bChanged = true;
    m_properties = newProps;

    CStandAloneDialog::OnOK();
}

BOOL CEditPropTSVNLang::EnumLocalesProc(LPTSTR lpLocaleString)
{
    DWORD langID = _tcstol(lpLocaleString, NULL, 16);

    TCHAR buf[MAX_PATH] = {0};
    GetLocaleInfo(langID, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
    CString sLang = buf;
    GetLocaleInfo(langID, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
    if (buf[0])
    {
        sLang += _T(" (");
        sLang += buf;
        sLang += _T(")");
    }

    int index = m_langCombo.AddString(sLang);
    m_langCombo.SetItemData(index, langID);

    return TRUE;
}