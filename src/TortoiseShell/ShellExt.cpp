// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

#pragma warning (disable : 4786)

// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "Guids.h"
#include "Globals.h"
#include "ShellExt.h"

#include "UnicodeStrings.h"



// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
{
    m_State = state;
	
    m_cRef = 0L;
	//if this is the first time the dll is loaded also start the watcher process.
	//the process itself makes sure that it is not started twice so the
	//check 'first start of dll' is enough.
#ifndef _DEBUG
	//if (g_cRefThisDll == 1)
	//{
	//	STARTUPINFO startup;
	//	PROCESS_INFORMATION process;
	//	memset(&startup, 0, sizeof(startup));
	//	startup.cb = sizeof(startup);
	//	memset(&process, 0, sizeof(process));
	//	//get location of TortoiseProc from the registry
	//	CRegStdString tortoiseProcPath("Software\\TortoiseSVN\\ProcPath", "TortoiseProc.exe", false, HKEY_LOCAL_MACHINE);
	//	CreateProcess(tortoiseProcPath, " /command:changewatcher", NULL, NULL, FALSE, 0, 0, 0, &startup, &process);
	//} // if (g_cRefThisDll == 1)
#endif
    g_cRefThisDll++;
	
    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
			ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES
    };
    InitCommonControlsEx(&used);
}

CShellExt::~CShellExt()
{
	g_cRefThisDll--;
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;
	
    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = (LPCONTEXTMENU)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu2))
    {
        *ppv = (LPCONTEXTMENU2)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppv = (LPCONTEXTMENU3)this;
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier))
    {
        *ppv = (IShellIconOverlayIdentifier*)this;
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = (LPSHELLPROPSHEETEXT)this;
    }
#if _WIN32_IE >= 0x0500
	else if (IsEqualIID(riid, IID_IColumnProvider)) 
	{ 
		*ppv = (IColumnProvider *)this;
	} 
#endif
    if (*ppv)
    {
        AddRef();
		
        return NOERROR;
    }
	
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;
	
    delete this;
	
    return 0L;
}

// IPersistFile members
STDMETHODIMP CShellExt::GetClassID(CLSID *pclsid) 
{
    *pclsid = CLSID_TortoiseSVN_UNCONTROLLED;
    return S_OK;
}

STDMETHODIMP CShellExt::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    return S_OK;
}

