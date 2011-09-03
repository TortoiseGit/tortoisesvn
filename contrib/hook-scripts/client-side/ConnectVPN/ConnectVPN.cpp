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
//
#include "stdafx.h"

#include "..\..\..\..\src\Utils\auto_buffer.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


bool IsConnected(const wchar_t *szConnectionName)
{
    // RasEnumConnections enumerates active connections; disconnected ones are not listed at all.

    DWORD dwCb = 0;
    DWORD dwConnections = 0;

    if (ERROR_BUFFER_TOO_SMALL != ::RasEnumConnections(NULL, &dwCb, &dwConnections))
    {
        return false; // No connections or an error. Not connected, either way.
    }

    // The OS returns the buffer sized based on the largest RASCONN structure it knows about.
    // If we are compiled with an SDK from a later OS version that has a larger RASCONN structure
    // then the OS's suggested buffer size will not be large enough. So allocate the maximum of
    // what the OS tells us and what is actually needed.
    if (dwCb < (dwConnections * sizeof(RASCONN)))
    {
        dwCb = (dwConnections * sizeof(RASCONN));
    }

    auto_buffer<RASCONN> rasCons(dwConnections);

    // First RASCONN structure must have its size set.
    // (And this may be larger than the OS's RASCONN size! See above.)
    rasCons.get()[0].dwSize = sizeof(RASCONN);

    dwConnections = 0;

    if (ERROR_SUCCESS != ::RasEnumConnections(rasCons.get(), &dwCb, &dwConnections))
    {
        // BUGBUG: This could fail due to a race condition in RasEnumConnections, i.e. if another connection arrived
        // between asking for the buffer size and asking for the buffer to be filled in. If this code were more critical
        // then it would make sense to loop and try again a few times on ERROR_BUFFER_TOO_SMALL. I can't be bothered, though. :)
        return false;
    }

    for (DWORD i = 0; i < dwConnections; ++i)
    {
        if (0 == _wcsicmp(szConnectionName, rasCons.get()[i].szEntryName))
        {
            for (int j = 0; j < 320; ++j) // 320 x 500 ms => maximum 2 minute wait before giving up.
            {
                RASCONNSTATUS rasConStatus = {0};
                rasConStatus.dwSize = sizeof(RASCONNSTATUS);

                if (ERROR_SUCCESS != ::RasGetConnectStatus(rasCons.get()[j].hrasconn, &rasConStatus))
                {
                    return false;
                }

                if (rasConStatus.rasconnstate == RASCS_Connected)
                {
                    return true;
                }

                if ((rasConStatus.rasconnstate == RASCS_Disconnected) || (rasConStatus.dwError != 0))
                {
                    return false;
                }

                Sleep(500); // Presumably something else is in the process of establishing the connection. Wait and loop.
            }
        }
    }

    return false; // What we were looking for isn't there yet.
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    SetDllDirectory(L"");
    if (__argc == 0)
    {
        return -1;
    }

    if (__argc != 2)
    {
        MessageBox(NULL, L"Usage: ConnectVPN <name of RAS/VPN connection>\nIf the connection is already active then nothing happens.\nIf the connection is inactive the standard connection dialog will be displayed.\n", L"ConnectVPN", MB_ICONINFORMATION);
        return -1;
    }

    const wchar_t *szConnectionName = __wargv[1];

    if (IsConnected(szConnectionName))
    {
        return 0; // nothing for us to do.
    }

    // The connection is not connected (or doesn't even exist).

    RASDIALDLG rdd = {0};
    rdd.dwSize = sizeof(rdd);

    if (!::RasDialDlg(NULL, const_cast< wchar_t * >( szConnectionName ), NULL, &rdd))
    {
        WCHAR errMsg[2048] = {0};
        swprintf_s(errMsg, _countof(errMsg), L"Failed to connect to %s (%lu)", szConnectionName, rdd.dwError);
        MessageBox(NULL, errMsg, L"ConnectVPN", MB_ICONERROR);
        return -1;
    }

    return 0;
}
