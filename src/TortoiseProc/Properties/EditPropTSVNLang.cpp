// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2013-2015 - TortoiseSVN

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
#include "AppUtils.h"


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
    ON_BN_CLICKED(IDHELP, &CEditPropTSVNLang::OnBnClickedHelp)
END_MESSAGE_MAP()


// CEditPropTSVNLang message handlers

BOOL CEditPropTSVNLang::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_FILELISTENGLISH);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    AdjustControlSize(IDC_FILELISTENGLISH);
    AdjustControlSize(IDC_PROPRECURSIVE);

    // fill the combo box with all available languages
    EnumSystemLocales(EnumLocalesProc, LCID_SUPPORTED);
    int index = m_langCombo.AddString(L"(none)");
    m_langCombo.SetItemData(index, 0);

    DWORD projLang = 0;
    for (auto it = m_properties.begin(); it != m_properties.end(); ++it)
    {
        if (it->second.isinherited)
            continue;
        if (it->first.compare(PROJECTPROPNAME_LOGFILELISTLANG) == 0)
        {
            m_bKeepEnglish = it->second.value.empty();
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

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || (!m_bFolder && !m_bMultiple) || m_bRemote ? SW_HIDE : SW_SHOW);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    UpdateData(false);

    return TRUE;
}

void CEditPropTSVNLang::OnOK()
{
    UpdateData();

    TProperties newProps;
    PropValue pVal;

    char numBuf[20] = { 0 };
    sprintf_s(numBuf, "%Id", m_langCombo.GetItemData(m_langCombo.GetCurSel()));
    pVal.value = numBuf;
    pVal.remove = (m_langCombo.GetItemData(m_langCombo.GetCurSel()) == 0);
    newProps.insert(std::make_pair(PROJECTPROPNAME_PROJECTLANGUAGE, pVal));

    pVal.value = m_bKeepEnglish ? "" : "false";
    pVal.remove = !!m_bKeepEnglish;
    newProps.insert(std::make_pair(PROJECTPROPNAME_LOGFILELISTLANG, pVal));

    m_bChanged = true;
    m_properties = newProps;

    CStandAloneDialog::OnOK();
}

BOOL CEditPropTSVNLang::EnumLocalesProc(LPTSTR lpLocaleString)
{
    DWORD langID = wcstol(lpLocaleString, NULL, 16);

    TCHAR buf[MAX_PATH] = {0};
    GetLocaleInfo(langID, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
    CString sLang = buf;
    GetLocaleInfo(langID, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
    if (buf[0])
    {
        sLang += L" (";
        sLang += buf;
        sLang += L")";
    }

    int index = m_langCombo.AddString(sLang);
    m_langCombo.SetItemData(index, langID);

    return TRUE;
}

void CEditPropTSVNLang::OnBnClickedHelp()
{
    OnHelp();
}
