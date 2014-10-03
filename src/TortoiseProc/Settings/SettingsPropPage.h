// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011, 2014 - TortoiseSVN

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
#pragma once

/**
 * \ingroup TortoiseProc
 * Base class for all the settings property pages
 */
class ISettingsPropPage : public CPropertyPage
{
public:
    // simple construction
    ISettingsPropPage();
    explicit ISettingsPropPage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
    explicit ISettingsPropPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));

    // extended construction
    ISettingsPropPage(UINT nIDTemplate, UINT nIDCaption,
        UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
    ISettingsPropPage(LPCTSTR lpszTemplateName, UINT nIDCaption,
        UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));

    virtual ~ISettingsPropPage();

    enum SettingsRestart
    {
        Restart_None = 0,
        Restart_System = 1,
        Restart_Cache = 2
    };

    /**
     * Returns the icon ID
     */
    virtual UINT GetIconID() = 0;

    /**
     * Returns the restart code
     */
    virtual SettingsRestart GetRestart() {return m_restart;}

protected:

    SettingsRestart m_restart;

    /**
     * Utility method:
     * Store the current value of a BOOL, DWORD or CString into the
     * respective CRegDWORD etc. and check for success.
     */

    template<class T, class Reg>
    void Store (const T& value, Reg& registryKey)
    {
        registryKey = value;
        if (registryKey.GetLastError() != ERROR_SUCCESS)
            TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), registryKey.getErrorString(), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
    }

    /**
     * Wrapper around the CWnd::EnableWindow() method, but
     * makes sure that a control that has the focus is not disabled
     * before the focus is passed on to the next control.
     */
    BOOL DialogEnableWindow(UINT nID, BOOL bEnable)
    {
        CWnd * pwndDlgItem = GetDlgItem(nID);
        return DialogEnableWindow(pwndDlgItem, bEnable);
    }
    /**
     * Wrapper around the CWnd::EnableWindow() method, but
     * makes sure that a control that has the focus is not disabled
     * before the focus is passed on to the next control.
     */
    BOOL DialogEnableWindow(CWnd * pwndDlgItem, BOOL bEnable)
    {
        if (pwndDlgItem == NULL)
            return FALSE;
        if (bEnable)
            return pwndDlgItem->EnableWindow(bEnable);
        if (GetFocus() == pwndDlgItem)
        {
            SendMessage(WM_NEXTDLGCTL, 0, FALSE);
        }
        return pwndDlgItem->EnableWindow(bEnable);
    }
    /**
    * Display a balloon with close button, anchored at a given edit control on this dialog.
    */
    void ShowEditBalloon(UINT nIdControl, UINT nIdText, UINT nIdTitle, int nIcon = TTI_WARNING)
    {
        CString text(MAKEINTRESOURCE(nIdText));
        CString title(MAKEINTRESOURCE(nIdTitle));
        ShowEditBalloon(nIdControl, text, title, nIcon);
    }
    void ShowEditBalloon(UINT nIdControl, const CString& text, const CString& title, int nIcon = TTI_WARNING)
    {
        EDITBALLOONTIP bt;
        bt.cbStruct = sizeof(bt);
        bt.pszText = text;
        bt.pszTitle = title;
        bt.ttiIcon = nIcon;
        SendDlgItemMessage(nIdControl, EM_SHOWBALLOONTIP, 0, (LPARAM)&bt);
    }
};