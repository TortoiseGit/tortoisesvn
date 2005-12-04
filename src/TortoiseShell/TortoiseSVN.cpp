// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "ShellExt.h"
#include "Guids.h"
#include "ShellExtClassFactory.h"

UINT      g_cRefThisDll = 0;				///< reference count of this DLL.
HINSTANCE g_hmodThisDll = NULL;				///< handle to this DLL itself.
int		  g_cAprInit = 0;
CRemoteCacheLink	g_remoteCacheLink;
ShellCache g_ShellCache;					///< caching of registry entries, ...
CRegStdWORD			g_regLang;
DWORD				g_langid;
HINSTANCE			g_hResInst;
stdstring			g_filepath;
svn_wc_status_kind	g_filestatus;	///< holds the corresponding status to the file/dir above
bool				g_readonlyoverlay = false;
bool				g_lockedoverlay = false;

bool				g_normalovlloaded = false;
bool				g_modifiedovlloaded = false;
bool				g_conflictedovlloaded = false;
bool				g_readonlyovlloaded = false;
bool				g_deletedovlloaded = false;
bool				g_lockedovlloaded = false;
bool				g_addedovlloaded = false;
CComCriticalSection	g_csCacheGuard;

extern std::set<CShellExt *> g_exts;

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /* lpReserved */)
{
#ifdef _DEBUG
	// if no debugger is present, then don't load the dll.
	// this prevents other apps from loading the dll and locking
	// it.

	bool bInShellTest = false;
	TCHAR buf[_MAX_PATH + 1];		// MAX_PATH ok, the test really is for debugging anyway.
	DWORD pathLength = GetModuleFileName(NULL, buf, _MAX_PATH);
	if(pathLength >= 14)
	{
		if ((_tcsicmp(&buf[pathLength-14], _T("\\ShellTest.exe"))) == 0)
		{
			bInShellTest = true;
		}
	}

	if (!::IsDebuggerPresent() && !bInShellTest)
	{
		ATLTRACE("In debug load preventer\n");
		return FALSE;
	}
#endif

	// NOTE: Do *NOT* call apr_initialize() or apr_terminate() here in DllMain(),
	// because those functions call LoadLibrary() indirectly through malloc().
	// And LoadLibrary() inside DllMain() is not allowed and can lead to unexpected
	// behaviour and even may create dependency loops in the dll load order.
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		if (g_hmodThisDll == NULL)
		{
			g_csCacheGuard.Init();
		}

        // Extension DLL one-time initialization
        g_hmodThisDll = hInstance;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
		// sometimes an application doesn't release all COM objects
		// but still unloads the dll.
		// in that case, we do it ourselves
		if (g_cRefThisDll > 0)
		{
			while (g_cAprInit--)
			{
				g_SVNAdminDir.Close();
				apr_terminate();
			}
			std::set<CShellExt *>::iterator it = g_exts.begin();
			while (it != g_exts.end())
			{
				delete *it;
				it = g_exts.begin();
			}
		}
		g_csCacheGuard.Term();
    }
    return 1;   // ok
}

STDAPI DllCanUnloadNow(void)
{
	return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
	
    FileState state = Invalid;
    if (IsEqualIID(rclsid, CLSID_TortoiseSVN_UPTODATE))
        state = Versioned;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_MODIFIED))
        state = Modified;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_CONFLICTING))
        state = Conflict;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_UNCONTROLLED))
        state = Uncontrolled;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_DROPHANDLER))
		state = DropHandler;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_DELETED))
		state = Deleted;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_READONLY))
		state = ReadOnly;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_LOCKED))
		state = LockedOverlay;
	else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_ADDED))
		state = AddedOverlay;
	
    if (state != Invalid)
    {
		apr_initialize();
		g_SVNAdminDir.Init();
		g_cAprInit++;
		
		CShellExtClassFactory *pcf = new CShellExtClassFactory(state);
		return pcf->QueryInterface(riid, ppvOut);
    }
	
    return CLASS_E_CLASSNOTAVAILABLE;

}


