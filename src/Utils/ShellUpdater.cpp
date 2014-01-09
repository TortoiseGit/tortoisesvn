// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2014 - TortoiseSVN

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
#include "ShellUpdater.h"
#include "../TSVNCache/CacheInterface.h"
#include "registry.h"
#include "SmartHandle.h"

CShellUpdater::CShellUpdater(void)
{
    m_hInvalidationEvent = CreateEvent(NULL, FALSE, FALSE, _T("TortoiseSVNCacheInvalidationEvent"));
}

CShellUpdater::~CShellUpdater(void)
{
    Flush();

    CloseHandle(m_hInvalidationEvent);
}

CShellUpdater& CShellUpdater::Instance()
{
    static CShellUpdater instance;
    return instance;
}

/**
* Add a single path for updating.
* The update will happen at some suitable time in the future
*/
void CShellUpdater::AddPathForUpdate(const CTSVNPath& path)
{
    // Tell the shell extension to purge its cache - we'll redo this when
    // we actually do the shell-updates, but sometimes there's an earlier update, which
    // might benefit from cache invalidation
    SetEvent(m_hInvalidationEvent);

    m_pathsForUpdating.AddPath(path);
}
/**
* Add a list of paths for updating.
* The update will happen when the list is destroyed, at the end of execution
*/
void CShellUpdater::AddPathsForUpdate(const CTSVNPathList& pathList)
{
    for(int nPath=0; nPath < pathList.GetCount(); nPath++)
    {
        AddPathForUpdate(pathList[nPath]);
    }
}

void CShellUpdater::Flush()
{
    if(m_pathsForUpdating.GetCount() > 0)
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Flushing shell update list\n");

        UpdateShell();
        m_pathsForUpdating.Clear();
    }
}

void CShellUpdater::UpdateShell()
{
    // Tell the shell extension to purge its cache
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Setting cache invalidation event %U64u\n", GetTickCount64());
    SetEvent(m_hInvalidationEvent);

    // We use the SVN 'notify' call-back to add items to the list
    // Because this might call-back more than once per file (for example, when committing)
    // it's possible that there may be duplicates in the list.
    // There's no point asking the shell to do more than it has to, so we remove the duplicates before
    // passing the list on
    m_pathsForUpdating.RemoveDuplicates();

    // if we use the external cache, we tell the cache directly that something
    // has changed, without the detour via the shell.
    CAutoFile hPipe = CreateFile(
        GetCacheCommandPipeName(),      // pipe name
        GENERIC_READ |                  // read and write access
        GENERIC_WRITE,
        0,                              // no sharing
        NULL,                           // default security attributes
        OPEN_EXISTING,                  // opens existing pipe
        FILE_FLAG_OVERLAPPED,           // default attributes
        NULL);                          // no template file


    if (!hPipe)
        return;

    // The pipe connected; change to message-read mode.
    DWORD dwMode = PIPE_READMODE_MESSAGE;
    if(SetNamedPipeHandleState(
         hPipe,    // pipe handle
         &dwMode,  // new pipe mode
         NULL,     // don't set maximum bytes
         NULL) == 0)    // don't set maximum time
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": SetNamedPipeHandleState failed");
        return;
    }

    for(int nPath = 0; nPath < m_pathsForUpdating.GetCount(); nPath++)
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Cache Item Update for %s (%I64u)\n"), m_pathsForUpdating[nPath].GetDirectory().GetWinPathString(), GetTickCount64());
        if (!m_pathsForUpdating[nPath].IsDirectory())
        {
            // send notifications to the shell for changed files - folders are updated by the cache itself.
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, m_pathsForUpdating[nPath].GetWinPath(), NULL);
        }
        TSVNCacheCommand cmd;
        cmd.command = TSVNCACHECOMMAND_CRAWL;
        wcsncpy_s(cmd.path, m_pathsForUpdating[nPath].GetDirectory().GetWinPath(), _countof(cmd.path)-1);
        DWORD cbWritten;
        BOOL fSuccess = WriteFile(
            hPipe,          // handle to pipe
            &cmd,           // buffer to write from
            sizeof(cmd),    // number of bytes to write
            &cbWritten,     // number of bytes written
            NULL);          // not overlapped I/O

        if (! fSuccess || sizeof(cmd) != cbWritten)
        {
            DisconnectNamedPipe(hPipe);
            return;
        }
    }

    // now tell the cache we don't need it's command thread anymore
    TSVNCacheCommand cmd;
    cmd.command = TSVNCACHECOMMAND_END;
    DWORD cbWritten;
    WriteFile(
        hPipe,          // handle to pipe
        &cmd,           // buffer to write from
        sizeof(cmd),    // number of bytes to write
        &cbWritten,     // number of bytes written
        NULL);          // not overlapped I/O
    DisconnectNamedPipe(hPipe);
}

bool CShellUpdater::RebuildIcons()
{
    const int BUFFER_SIZE = 1024;
    TCHAR buf[BUFFER_SIZE] = { 0 };
    HKEY hRegKey = 0;
    DWORD dwRegValue;
    DWORD dwRegValueTemp;
    DWORD dwSize;
    DWORD_PTR dwResult;
    LONG lRegResult;
    bool bResult = false;

    lRegResult = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop\\WindowMetrics"),
        0, KEY_READ | KEY_WRITE, &hRegKey);
    if (lRegResult != ERROR_SUCCESS)
        goto Cleanup;

    // we're going to change the Shell Icon Size value
    const TCHAR* sRegValueName = _T("Shell Icon Size");

    // Read registry value
    dwSize = BUFFER_SIZE;
    lRegResult = RegQueryValueEx(hRegKey, sRegValueName, NULL, NULL,
        (LPBYTE) buf, &dwSize);
    if (lRegResult != ERROR_FILE_NOT_FOUND)
    {
        // If registry key doesn't exist create it using system current setting
        int iDefaultIconSize = ::GetSystemMetrics(SM_CXICON);
        if (0 == iDefaultIconSize)
            iDefaultIconSize = 32;
        _sntprintf_s(buf, BUFFER_SIZE, BUFFER_SIZE, _T("%d"), iDefaultIconSize);
    }
    else if (lRegResult != ERROR_SUCCESS)
        goto Cleanup;

    // Change registry value
    dwRegValue = _ttoi(buf);
    dwRegValueTemp = dwRegValue-1;

    dwSize = _sntprintf_s(buf, BUFFER_SIZE, BUFFER_SIZE, _T("%lu"), dwRegValueTemp) + sizeof(TCHAR);
    lRegResult = RegSetValueEx(hRegKey, sRegValueName, 0, REG_SZ,
        (LPBYTE) buf, dwSize);
    if (lRegResult != ERROR_SUCCESS)
        goto Cleanup;


    // Update all windows
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS,
        0, SMTO_ABORTIFHUNG, 5000, &dwResult);

    // Reset registry value
    dwSize = _sntprintf_s(buf, BUFFER_SIZE, BUFFER_SIZE, _T("%lu"), dwRegValue) + sizeof(TCHAR);
    lRegResult = RegSetValueEx(hRegKey, sRegValueName, 0, REG_SZ,
        (LPBYTE) buf, dwSize);
    if(lRegResult != ERROR_SUCCESS)
        goto Cleanup;

    // Update all windows
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS,
        0, SMTO_ABORTIFHUNG, 5000, &dwResult);

    bResult = true;

Cleanup:
    if (hRegKey != 0)
    {
        RegCloseKey(hRegKey);
    }
    return bResult;
}
