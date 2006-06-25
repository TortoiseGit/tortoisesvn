// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "MainWindow.h"
#include "CmdLineParser.h"
#include "TortoiseIDiff.h"

#ifndef WIN64
#	pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#	pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"") 
#endif

// Global Variables:
HINSTANCE hInst;								// current instance


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	CCmdLineParser parser(lpCmdLine);

	if (parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
	{
		TCHAR buf[1024];
		LoadString(hInstance, IDS_COMMANDLINEHELP, buf, sizeof(buf)/sizeof(TCHAR));
		MessageBox(NULL, buf, _T("TortoiseIDiff"), MB_ICONINFORMATION);
		return 0;
	}


	MSG msg;
	HACCEL hAccelTable;

	hInst = hInstance;

	INITCOMMONCONTROLSEX used = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_STANDARD_CLASSES | ICC_BAR_CLASSES
	};
	InitCommonControlsEx(&used);

	CMainWindow mainWindow(hInstance);


	mainWindow.SetLeft(parser.HasVal(_T("left")) ? parser.GetVal(_T("left")) : _T(""), parser.HasVal(_T("lefttitle")) ? parser.GetVal(_T("lefttitle")) : _T(""));
	mainWindow.SetRight(parser.HasVal(_T("right")) ? parser.GetVal(_T("right")) : _T(""), parser.HasVal(_T("righttitle")) ? parser.GetVal(_T("righttitle")) : _T(""));

	if (mainWindow.RegisterAndCreateWindow())
	{
		hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_TORTOISEIDIFF));
		if (!parser.HasVal(_T("left")))
		{
			PostMessage(mainWindow, WM_COMMAND, ID_FILE_OPEN, 0);
		}
		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return (int) msg.wParam;
	}
	return 1;
}





