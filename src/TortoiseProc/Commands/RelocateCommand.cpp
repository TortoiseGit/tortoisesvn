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
#include "stdafx.h"
#include "RelocateCommand.h"

#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "RelocateDlg.h"
#include "SVN.h"
#include "MessageBox.h"
#include "PathUtils.h"

bool RelocateCommand::Execute()
{
    bool bRet = false;
    SVN svn;
    CRelocateDlg dlg;
    dlg.m_path = cmdLinePath;
    dlg.m_sFromUrl = CPathUtils::PathUnescape(svn.GetRepositoryRoot(cmdLinePath));
    dlg.m_sToUrl = dlg.m_sFromUrl;

    if (dlg.DoModal() == IDOK)
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": relocate from %s to %s\n", (LPCTSTR)dlg.m_sFromUrl, (LPCTSTR)dlg.m_sToUrl);
        // crack the urls into their components
        TCHAR urlpath1[INTERNET_MAX_PATH_LENGTH+1];
        TCHAR scheme1[INTERNET_MAX_SCHEME_LENGTH+1];
        TCHAR hostname1[INTERNET_MAX_HOST_NAME_LENGTH+1];
        TCHAR username1[INTERNET_MAX_USER_NAME_LENGTH+1];
        TCHAR password1[INTERNET_MAX_PASSWORD_LENGTH+1];
        TCHAR urlpath2[INTERNET_MAX_PATH_LENGTH+1];
        TCHAR scheme2[INTERNET_MAX_SCHEME_LENGTH+1];
        TCHAR hostname2[INTERNET_MAX_HOST_NAME_LENGTH+1];
        TCHAR username2[INTERNET_MAX_USER_NAME_LENGTH+1];
        TCHAR password2[INTERNET_MAX_PASSWORD_LENGTH+1];
        URL_COMPONENTS components1 = {0};
        URL_COMPONENTS components2 = {0};
        components1.dwStructSize = sizeof(URL_COMPONENTS);
        components1.dwUrlPathLength = _countof(urlpath1) - 1;
        components1.lpszUrlPath = urlpath1;
        components1.lpszScheme = scheme1;
        components1.dwSchemeLength = _countof(scheme1) - 1;
        components1.lpszHostName = hostname1;
        components1.dwHostNameLength = _countof(hostname1) - 1;
        components1.lpszUserName = username1;
        components1.dwUserNameLength = _countof(username1) - 1;
        components1.lpszPassword = password1;
        components1.dwPasswordLength = _countof(password1) - 1;
        components2.dwStructSize = sizeof(URL_COMPONENTS);
        components2.dwUrlPathLength = _countof(urlpath2) - 1;
        components2.lpszUrlPath = urlpath2;
        components2.lpszScheme = scheme2;
        components2.dwSchemeLength = _countof(scheme2) - 1;
        components2.lpszHostName = hostname2;
        components2.dwHostNameLength = _countof(hostname2) - 1;
        components2.lpszUserName = username2;
        components2.dwUserNameLength = _countof(username2) - 1;
        components2.lpszPassword = password2;
        components2.dwPasswordLength = _countof(password2) - 1;
        CString sTempUrl = dlg.m_sFromUrl;
        if (sTempUrl.Left(8).Compare(L"file:///\\")==0)
            sTempUrl.Replace(L"file:///\\", L"file://");
        InternetCrackUrl((LPCTSTR)sTempUrl, sTempUrl.GetLength(), 0, &components1);
        sTempUrl = dlg.m_sToUrl;
        if (sTempUrl.Left(8).Compare(L"file:///\\")==0)
            sTempUrl.Replace(L"file:///\\", L"file://");
        InternetCrackUrl((LPCTSTR)sTempUrl, sTempUrl.GetLength(), 0, &components2);
        // now compare the url components.
        // If the 'main' parts differ (e.g. hostname, port, scheme, ...) then a relocate is
        // necessary and we don't show a warning. But if only the path part of the url
        // changed, we assume the user really wants to switch and show the warning.
        bool bPossibleSwitch = true;
        if (components1.dwSchemeLength != components2.dwSchemeLength)
            bPossibleSwitch = false;
        else if (_tcsncmp(components1.lpszScheme, components2.lpszScheme, components1.dwSchemeLength))
            bPossibleSwitch = false;
        if (components1.dwHostNameLength != components2.dwHostNameLength)
            bPossibleSwitch = false;
        else if (_tcsncmp(components1.lpszHostName, components2.lpszHostName, components1.dwHostNameLength))
            bPossibleSwitch = false;
        if (components1.dwUserNameLength != components2.dwUserNameLength)
            bPossibleSwitch = false;
        else if (_tcsncmp(components1.lpszUserName, components2.lpszUserName, components1.dwUserNameLength))
            bPossibleSwitch = false;
        if (components1.dwPasswordLength != components2.dwPasswordLength)
            bPossibleSwitch = false;
        else if (_tcsncmp(components1.lpszPassword, components2.lpszPassword, components1.dwPasswordLength))
            bPossibleSwitch = false;
        if (components1.nPort != components2.nPort)
            bPossibleSwitch = false;
        if (bPossibleSwitch)
        {
            if ((dlg.m_sFromUrl.Left(7).Compare(L"file://") == 0) &&
                (dlg.m_sToUrl.Left(7).Compare(L"file://") == 0))
            {
                CString s1 = dlg.m_sFromUrl.Mid(7);
                CString s2 = dlg.m_sToUrl.Mid(7);
                s1.TrimLeft('/');
                s2.TrimLeft('/');
                if (s1.GetLength() && s2.GetLength())
                {
                    if (s1.GetAt(0) != s2.GetAt(0))
                        bPossibleSwitch = false;
                }
                else
                    bPossibleSwitch = false;
            }
        }
        if (bPossibleSwitch)
        {
            if (CTaskDialog::IsSupported())
            {
                CString sInfo;
                sInfo.FormatMessage(IDS_WARN_RELOCATEREALLY_TASK1, (LPCTSTR)dlg.m_sFromUrl, (LPCTSTR)dlg.m_sToUrl);
                CTaskDialog taskdlg(sInfo,
                                    CString(MAKEINTRESOURCE(IDS_WARN_RELOCATEREALLY_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
                taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_WARN_RELOCATEREALLY_TASK3)));
                taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_WARN_RELOCATEREALLY_TASK4)));
                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskdlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_WARN_RELOCATEREALLY_TASK5)));
                taskdlg.SetDefaultCommandControl(2);
                taskdlg.SetMainIcon(TD_WARNING_ICON);
                if (taskdlg.DoModal(GetExplorerHWND()) == 1)
                    bPossibleSwitch = false;
            }
            else
            {
                CString sWarning, sWarningTitle, sHelpPath;
                sWarning.FormatMessage(IDS_WARN_RELOCATEREALLY, (LPCTSTR)dlg.m_sFromUrl, (LPCTSTR)dlg.m_sToUrl);
                sWarningTitle.LoadString(IDS_WARN_RELOCATEREALLYTITLE);
                sHelpPath = theApp.m_pszHelpFilePath;
                sHelpPath += L"::/tsvn-dug-relocate.html";
                if (TSVNMessageBox((GetExplorerHWND()), sWarning, sWarningTitle, MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2|MB_HELP, sHelpPath)==IDYES)
                    bPossibleSwitch = false;
            }
        }

        if (!bPossibleSwitch)
        {
            SVN s;

            CProgressDlg progress;
            if (progress.IsValid())
            {
                progress.SetTitle(IDS_PROC_RELOCATING);
                progress.ShowModeless(GetExplorerHWND());
            }
            if (!s.Relocate(cmdLinePath, CTSVNPath(dlg.m_sFromUrl), CTSVNPath(dlg.m_sToUrl), !!dlg.m_bIncludeExternals))
            {
                progress.Stop();
                s.ShowErrorDialog(GetExplorerHWND(), cmdLinePath);
            }
            else
            {
                progress.Stop();
                CString strMessage;
                strMessage.Format(IDS_PROC_RELOCATEFINISHED, (LPCTSTR)dlg.m_sToUrl);
                ::MessageBox(GetExplorerHWND(), strMessage, L"TortoiseSVN", MB_ICONINFORMATION);
                bRet = true;
            }
        }
    }
    return bRet;
}
