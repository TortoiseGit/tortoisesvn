// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007, 2010-2011, 2013 - TortoiseSVN
#include "stdafx.h"

// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#include <initguid.h>
#include "Guids.h"
#include "ShellExt.h"

// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
{
    m_State = state;

    m_cRef = 0L;
    InterlockedIncrement(&g_cRefThisDll);
}

CShellExt::~CShellExt()
{
    for (auto it = m_dllpointers.begin(); it != m_dllpointers.end(); ++it)
    {
        if (it->pShellIconOverlayIdentifier != NULL)
        {
            it->pShellIconOverlayIdentifier->Release();
            it->pShellIconOverlayIdentifier = NULL;
        }
        if (it->hDll != NULL)
        {
            FreeLibrary(it->hDll);
            it->hDll = NULL;
        }

        it->pDllGetClassObject = NULL;
        it->pDllCanUnloadNow = NULL;
    }
    m_dllpointers.clear();

    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    if(ppv == 0)
        return E_POINTER;
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier))
    {
        *ppv = static_cast<IShellIconOverlayIdentifier*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
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
