// CustomActions.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

UINT __stdcall TerminateCache(MSIHANDLE hModule)
{
	HWND hWnd = FindWindow(_T("TSVNCacheWindow"), _T("TSVNCacheWindow"));
	if (hWnd)
	{
		// First, delete the registry key telling the shell where to find
		// the cache. This is to make sure that the cache won't be started
		// again right after we close it.
		HKEY hKey = 0;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\TortoiseSVN"), 0, KEY_WRITE, &hKey)==ERROR_SUCCESS)
		{
			RegDeleteValue(hKey, _T("CachePath"));
			RegCloseKey(hKey);
		}
		PostMessage(hWnd, WM_CLOSE, NULL, NULL);
		Sleep(1500);
		if (!IsWindow(hWnd))
		{
			// Cache is gone!
			return ERROR_SUCCESS;
		}
		return ERROR_FUNCTION_FAILED;
	}
	// cache wasn't even running
	return ERROR_SUCCESS;
}

