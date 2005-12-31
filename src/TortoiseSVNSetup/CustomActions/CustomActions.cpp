// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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


/* BIG FAT WARNING: Do not use any functions which require the C-Runtime library
   in this custom action dll! The runtimes might not be installed yet!
*/

#include "stdafx.h"
#include "shlwapi.h"
#pragma comment(lib, "shlwapi")

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
		for (int i=0; i<10; ++i)
		{
			Sleep(500);
			if (!IsWindow(hWnd))
			{
				// Cache is gone!
				return ERROR_SUCCESS;
			}
		}
		// Don't return ERROR_FUNCTION_FAILED, because even if the cache is still
		// running, the installer will overwrite the file, and we require a
		// reboot anyway after upgrading.
		return ERROR_SUCCESS;
	}
	// cache wasn't even running
	return ERROR_SUCCESS;
}

UINT __stdcall ClearRegistry(MSIHANDLE hModule)
{
	TCHAR entries[37][255] = {	_T("Software\\Classes\\Drive\\shellex\\ContextMenuHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\Directory\\shellex\\ContextMenuHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\Directory\\Background\\shellex\\ContextMenuHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\Folder\\shellex\\ContextMenuHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\lnkfile\\shellex\\ContextMenuHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\InternetShortcut\\shellex\\ContextMenuHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\*\\shellex\\ContextMenuHandlers\\TortoiseSVN"),

		_T("Software\\Classes\\Drive\\shellex\\PropertySheetHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\Directory\\shellex\\PropertySheetHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\*\\shellex\\PropertySheetHandlers\\TortoiseSVN"),

		_T("Software\\Classes\\Directory\\shellex\\DragDropHandlers\\TortoiseSVN"),
		_T("Software\\Classes\\Folder\\shellex\\DragDropHandlers\\TortoiseSVN"),

		_T("Software\\Classes\\Folder\\shellex\\ColumnHandlers\\{30351349-7B7D-4FCC-81B4-1E394CA267EB}"),

		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\2TortoiseSVN"),

		_T("Software\\Classes\\CLSID\\{30351349-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{3035134A-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{3035134E-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{3035134D-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{3035134C-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{30351346-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{3035134B-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{30351347-7B7D-4FCC-81B4-1E394CA267EB}"),
		_T("Software\\Classes\\CLSID\\{30351348-7B7D-4FCC-81B4-1E394CA267EB}"),

		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\1TortoiseSVN"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\2TortoiseSVN"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\3TortoiseSVN"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\4TortoiseSVN"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\5TortoiseSVN"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\6TortoiseSVN"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\7TortoiseSVN"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\TortoiseSVN1"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\TortoiseSVN2"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\TortoiseSVN3"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\TortoiseSVN4"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\TortoiseSVN5"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\TortoiseSVN6"),
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\TortoiseSVN7")
	};

	for (int i=0; i<37; ++i)
	{
		HKEY hKey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, entries[i], 0, KEY_WRITE, &hKey)==ERROR_SUCCESS)
		{
			SHDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)entries[i]);
		}
	}
	for (int i=0; i<37; ++i)
	{
		HKEY hKey;
		if (RegOpenKeyEx(HKEY_CURRENT_USER, entries[i], 0, KEY_WRITE, &hKey)==ERROR_SUCCESS)
		{
			SHDeleteKey(HKEY_CURRENT_USER, (LPCTSTR)entries[i]);
		}
	}
	return ERROR_SUCCESS;
}

UINT __stdcall MsgBox(MSIHANDLE hModule)
{
	MessageBox(NULL, _T("CustomAction \"MsgBox\" running"), _T("Installer"), MB_ICONINFORMATION);
	return ERROR_SUCCESS;
}
