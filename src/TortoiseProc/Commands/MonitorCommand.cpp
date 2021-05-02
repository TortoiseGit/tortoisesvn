// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014, 2021 - TortoiseSVN

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
#include "MonitorCommand.h"
#include "LogDialog/LogDlg.h"
#include "StringUtils.h"
#include "SmartHandle.h"
#include "TortoiseProc.h"

static CString GetMonitorID()
{
    CString            t;
    CAutoGeneralHandle token;
    BOOL               result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.GetPointer());
    if (result)
    {
        DWORD len = 0;
        GetTokenInformation(token, TokenStatistics, nullptr, 0, &len);
        if (len >= sizeof(TOKEN_STATISTICS))
        {
            auto data = std::make_unique<BYTE[]>(len);
            GetTokenInformation(token, TokenStatistics, data.get(), len, &len);
            auto [LowPart, HighPart] = reinterpret_cast<PTOKEN_STATISTICS>(data.get())->AuthenticationId;
            t.Format(L"-%08x%08x", HighPart, LowPart);
        }
    }
    return t;
}

bool MonitorCommand::Execute()
{
    CAutoGeneralHandle hReloadProtection = ::CreateMutex(nullptr, FALSE, L"TSVN_Monitor_" + GetMonitorID());

    if ((!hReloadProtection) || (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        // An instance of the commit monitor is already running
        HWND hWnd = FindWindow(nullptr, CString(MAKEINTRESOURCE(IDS_MONITOR_DLGTITLE)));
        if (hWnd)
        {
            UINT TSVN_COMMITMONITOR_SHOWDLGMSG = RegisterWindowMessage(L"TSVNCommitMonitor_ShowDlgMsg");
            PostMessage(hWnd, TSVN_COMMITMONITOR_SHOWDLGMSG, 0, 0); //open the window of the already running app
            SetForegroundWindow(hWnd);                              //set the window to front
        }
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": TSVN Commit Monitor ignoring restart\n");
        return false;
    }

    CLogDlg dlg;
    theApp.m_pMainWnd = &dlg;
    dlg.SetMonitoringMode(!!parser.HasKey(L"tray"));
    dlg.DoModal();
    return true;
}
