#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <Commctrl.h>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Crypt32.lib")

int WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
{
    InitCommonControls();
    main(__argc,__argv);
}
