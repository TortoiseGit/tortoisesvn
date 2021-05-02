// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2014, 2016, 2021 - TortoiseSVN

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

#ifndef __AFXWIN_H__
#    error include 'stdafx.h' before including this file for PCH
#endif

class CTSVNPath;
class CTSVNPathList;

/**
 * \ingroup TortoiseProc
 * Main class of the TortoiseProc.exe\n
 * It is the entry point when calling the TortoiseProc.exe and
 * handles the command line. Depending on the command line
 * other 'modules' are called, usually dialog boxes which
 * themselves then execute a specific function.\n\n
 * Many commands are executed using the CSVNProgressDlg which
 * just displays the common notify callbacks of the Subversion commands.
 */

class CTortoiseProcApp : public CWinAppEx
{
public:
    CTortoiseProcApp();
    ~CTortoiseProcApp() override;

    // Overrides
public:
    BOOL InitInstance() override;
    int  ExitInstance() override;

    static void CheckUpgrade();
    void        InitializeJumpList(const CString& appid);
    static void DoInitializeJumpList(const CString& appid);

    HWND GetExplorerHWND() const { return (::IsWindow(hWndExplorer) ? hWndExplorer : nullptr); }

    // Implementation

private:
    DECLARE_MESSAGE_MAP()
private:
    bool        retSuccess;
    HWND        hWndExplorer;
    apr_pool_t* m_globalPool;
    void        CheckForNewerVersion() const;
    static void Sync();
};

extern CTortoiseProcApp theApp;
extern CString          sOrigCwd;
extern CString          g_sGroupingUuid;
// ReSharper disable once CppInconsistentNaming
HWND GetExplorerHWND();
HWND FindParentWindow(HWND hWnd);
