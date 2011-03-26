// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2011 - TortoiseSVN

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
#include "SettingsAdvanced.h"
#include "registry.h"


CSettingsAdvanced::AdvancedSetting CSettingsAdvanced::settings[] =
{
    { _T("AllowAuthSave"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("AllowUnversionedObstruction"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("AlwaysExtendedMenu"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("AutocompleteRemovesExtensions"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("BlockStatus"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("CacheTrayIcon"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("ColumnsEveryWhere"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("ConfigDir"), CSettingsAdvanced::SettingTypeString, _T("") },
    { _T("CtrlEnter"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("Debug"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("DebugOutputString"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("DiffBlamesWithTortoiseMerge"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("FullRowSelect"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("IncludeExternals"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("LogStatusCheck"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("MergeLogSeparator"), CSettingsAdvanced::SettingTypeString, _T("........") },
    { _T("OldVersionCheck"), CSettingsAdvanced::SettingTypeBoolean, false },
    { _T("ShellMenuAccelerators"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("ShowContextMenuIcons"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("ShowAppContextMenuIcons"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("StyleCommitMessages"), CSettingsAdvanced::SettingTypeBoolean, true },
    { _T("UpdateCheckURL"), CSettingsAdvanced::SettingTypeString, _T("") },
    { _T("VersionCheck"), CSettingsAdvanced::SettingTypeBoolean, true },

    { _T(""), CSettingsAdvanced::SettingTypeNone, false }
};



IMPLEMENT_DYNAMIC(CSettingsAdvanced, ISettingsPropPage)

CSettingsAdvanced::CSettingsAdvanced()
    : ISettingsPropPage(CSettingsAdvanced::IDD)
{

}

CSettingsAdvanced::~CSettingsAdvanced()
{
}

void CSettingsAdvanced::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CONFIG, m_ListCtrl);
}


BEGIN_MESSAGE_MAP(CSettingsAdvanced, ISettingsPropPage)
    ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_CONFIG, &CSettingsAdvanced::OnLvnBeginlabeledit)
    ON_NOTIFY(LVN_ENDLABELEDIT, IDC_CONFIG, &CSettingsAdvanced::OnLvnEndlabeledit)
    ON_NOTIFY(NM_DBLCLK, IDC_CONFIG, &CSettingsAdvanced::OnNMDblclkConfig)
END_MESSAGE_MAP()


BOOL CSettingsAdvanced::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_ListCtrl.DeleteAllItems();
    int c = ((CHeaderCtrl*)(m_ListCtrl.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_ListCtrl.DeleteColumn(c--);

    SetWindowTheme(m_ListCtrl.GetSafeHwnd(), L"Explorer", NULL);

    CString temp;
    temp.LoadString(IDS_SETTINGS_CONF_VALUECOL);
    m_ListCtrl.InsertColumn(0, temp);
    temp.LoadString(IDS_SETTINGS_CONF_NAMECOL);
    m_ListCtrl.InsertColumn(1, temp);

    m_ListCtrl.SetRedraw(FALSE);

    int i = 0;
    while (settings[i].type != SettingTypeNone)
    {
        m_ListCtrl.InsertItem(i, settings[i].sName);
        m_ListCtrl.SetItemText(i, 1, settings[i].sName);
        switch (settings[i].type)
        {
        case SettingTypeBoolean:
            {
                CRegDWORD s(_T("Software\\TortoiseSVN\\")+settings[i].sName, settings[i].def.b);
                m_ListCtrl.SetItemText(i, 0, DWORD(s) ? _T("true") : _T("false"));
            }
            break;
        case SettingTypeNumber:
            {
                CRegDWORD s(_T("Software\\TortoiseSVN\\")+settings[i].sName, settings[i].def.l);
                CString temp;
                temp.Format(_T("%ld"), (DWORD)s);
                m_ListCtrl.SetItemText(i, 0, temp);
            }
            break;
        case SettingTypeString:
            {
                CRegString s(_T("Software\\TortoiseSVN\\")+settings[i].sName, settings[i].def.s);
                m_ListCtrl.SetItemText(i, 0, CString(s));
            }
        }

        i++;
    }

    int mincol = 0;
    int maxcol = ((CHeaderCtrl*)(m_ListCtrl.GetDlgItem(0)))->GetItemCount()-1;
    int col;
    for (col = mincol; col <= maxcol; col++)
    {
        m_ListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
    }
    int arr[2] = {1,0};
    m_ListCtrl.SetColumnOrderArray(2, arr);
    m_ListCtrl.SetRedraw(TRUE);

    return TRUE;
}

BOOL CSettingsAdvanced::OnApply()
{
    int i = 0;
    while (settings[i].type != SettingTypeNone)
    {
        CString sValue = m_ListCtrl.GetItemText(i, 0);
        switch (settings[i].type)
        {
        case SettingTypeBoolean:
            {
                CRegDWORD s(_T("Software\\TortoiseSVN\\")+settings[i].sName, settings[i].def.b);
                if (sValue.IsEmpty())
                    s.removeValue();
                else
                {
                    DWORD newValue = sValue.Compare(_T("true")) == 0;
                    if (DWORD(s) != newValue)
                    {
                        s = newValue;
                    }
                }
            }
            break;
        case SettingTypeNumber:
            {
                CRegDWORD s(_T("Software\\TortoiseSVN\\")+settings[i].sName, settings[i].def.l);
                if (DWORD(_tstol(sValue)) != DWORD(s))
                {
                    s = _tstol(sValue);
                }
            }
            break;
        case SettingTypeString:
            {
                CRegString s(_T("Software\\TortoiseSVN\\")+settings[i].sName, settings[i].def.s);
                if (sValue.Compare(CString(s)))
                {
                    s = sValue;
                }
            }
        }

        i++;
    }

    return ISettingsPropPage::OnApply();
}

void CSettingsAdvanced::OnLvnBeginlabeledit(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    *pResult = FALSE;
}

void CSettingsAdvanced::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    *pResult = 0;
    if (pDispInfo->item.pszText == NULL)
        return;

    bool allowEdit = false;
    switch (settings[pDispInfo->item.iItem].type)
    {
    case SettingTypeBoolean:
        {
            if ( (_tcscmp(pDispInfo->item.pszText, _T("true")) == 0)  ||
                 (_tcscmp(pDispInfo->item.pszText, _T("false")) == 0) ||
                 (_tcslen(pDispInfo->item.pszText) == 0) )
            {
                allowEdit = true;
            }
        }
        break;
    case SettingTypeNumber:
        {
            TCHAR * pChar = pDispInfo->item.pszText;
            allowEdit = true;
            while (*pChar)
            {
                if (!_istdigit(*pChar))
                    allowEdit = false;
                pChar++;
            }
        }
        break;
    case SettingTypeString:
        allowEdit = true;
        break;
    }

    if (allowEdit)
        SetModified();

    *pResult = allowEdit ? TRUE : FALSE;
}

BOOL CSettingsAdvanced::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_F2:
            {
                m_ListCtrl.EditLabel(m_ListCtrl.GetSelectionMark());
            }
            break;
        }
    }
    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSettingsAdvanced::OnNMDblclkConfig(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    m_ListCtrl.EditLabel(pNMItemActivate->iItem);
    *pResult = 0;
}