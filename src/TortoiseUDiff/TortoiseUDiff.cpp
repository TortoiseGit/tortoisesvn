// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2012, 2014-2015 - TortoiseSVN

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
#include "TortoiseUDiff.h"
#include "MainWindow.h"
#include "CmdLineParser.h"
#include "TaskbarUUID.h"
#include "LangDll.h"
#include "../Utils/CrashReport.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")


#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE hInst;                                // current instance
HINSTANCE hResource;                            // the resource dll


int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE /*hPrevInstance*/,
                       LPTSTR    lpCmdLine,
                       int       /*nCmdShow*/)
{
    SetDllDirectory(L"");
    SetTaskIDPerUUID();
    MSG msg;
    HACCEL hAccelTable;

    CCrashReportTSVN crasher(L"TortoiseUDiff " _T(APP_X64_STRING));
    CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());
    CRegStdDWORD loc = CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033);
    long langId = loc;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    CLangDll langDLL;
    hResource = langDLL.Init(L"TortoiseUDiff", langId);
    if (hResource == NULL)
        hResource = hInstance;

    CCmdLineParser parser(lpCmdLine);

    if (parser.HasKey(L"?") || parser.HasKey(L"help"))
    {
        ResString rHelp(hResource, IDS_COMMANDLINEHELP);
        MessageBox(NULL, rHelp, L"TortoiseUDiff", MB_ICONINFORMATION);
        return 0;
    }

    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_STANDARD_CLASSES | ICC_BAR_CLASSES
    };
    InitCommonControlsEx(&used);


    HMODULE hSciLexerDll = ::LoadLibrary(L"SciLexer.DLL");
    if (hSciLexerDll == NULL)
        return FALSE;

    CMainWindow mainWindow(hResource);
    mainWindow.SetRegistryPath(L"Software\\TortoiseSVN\\UDiffViewerWindowPos");
    if (parser.HasVal(L"title"))
        mainWindow.SetTitle(parser.GetVal(L"title"));
    else if (parser.HasVal(L"patchfile"))
        mainWindow.SetTitle(parser.GetVal(L"patchfile"));
    else
    {
        ResString rPipeTitle(hResource, IDS_PIPETITLE);
        mainWindow.SetTitle(rPipeTitle);
    }

    if (!mainWindow.RegisterAndCreateWindow())
    {
        FreeLibrary(hSciLexerDll);
        return 0;
    }

    bool bLoadedSuccessfully = false;
    if ( (lpCmdLine[0] == 0) ||
        (parser.HasKey(L"p")) )
    {
        // input from console pipe
        // set console to raw mode
        DWORD oldMode;
        GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldMode);
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), oldMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));

        bLoadedSuccessfully = mainWindow.LoadFile(GetStdHandle(STD_INPUT_HANDLE));
    }
    else if (parser.HasVal(L"patchfile"))
        bLoadedSuccessfully = mainWindow.LoadFile(parser.GetVal(L"patchfile"));
    else if (lpCmdLine[0] != 0)
        bLoadedSuccessfully = mainWindow.LoadFile(lpCmdLine);


    if (!bLoadedSuccessfully)
    {
        FreeLibrary(hSciLexerDll);
        return 0;
    }

    ::ShowWindow(mainWindow.GetHWNDEdit(), SW_SHOW);
    ::SetFocus(mainWindow.GetHWNDEdit());

    hAccelTable = LoadAccelerators(hResource, MAKEINTRESOURCE(IDC_TORTOISEUDIFF));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(mainWindow, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    FreeLibrary(hSciLexerDll);
    return (int) msg.wParam;
}
