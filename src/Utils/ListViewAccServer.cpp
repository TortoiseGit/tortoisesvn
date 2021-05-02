// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015, 2021 - TortoiseSVN

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
#include "stdafx.h"
#include "ListViewAccServer.h"
#include <oleacc.h>

STDMETHODIMP ListViewAccServer::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
        return E_POINTER;
    *ppvObject = nullptr;
    if (IsEqualIID(IID_IUnknown, riid) || IsEqualIID(IID_IAccPropServer, riid))
        *ppvObject = static_cast<IAccPropServer*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
ListViewAccServer::AddRef()
{
    return ++m_ref;
}

STDMETHODIMP_(ULONG)
ListViewAccServer::Release()
{
    --m_ref;
    return m_ref;
}

HRESULT STDMETHODCALLTYPE ListViewAccServer::GetPropValue(
    const BYTE* pIDString,
    DWORD       dwIDStringLen,
    MSAAPROPID /*idProp*/,
    VARIANT* pvarValue,
    BOOL*    pfGotProp)
{
    if (pfGotProp == nullptr)
        return E_POINTER;

    // Default return values, in case we need to bail out…
    *pfGotProp    = FALSE;
    pvarValue->vt = VT_EMPTY;
    // Extract the idChild from the identity string…
    DWORD   idObject = 0, idChild = 0;
    HWND    dwHcontrol = nullptr;
    HRESULT hr         = m_pAccPropSvc->DecomposeHwndIdentityString(pIDString, dwIDStringLen, &dwHcontrol, &idObject, &idChild);
    if (hr != S_OK)
    {
        return S_OK;
    }

    HWND hwnd = dwHcontrol;
    // Only supply help string for child elements, not the
    // listview itself…
    if (idChild == CHILDID_SELF)
    {
        return S_OK;
    }

    // GetHelpString returns a UNICODE string corresponding to
    // the index it is passed.
    CString sHelpString = m_pAccProvider->GetListviewHelpString(hwnd, idChild - 1);
    if (sHelpString.IsEmpty())
    {
        return S_OK;
    }

    BSTR bStr          = SysAllocString(static_cast<LPCWSTR>(sHelpString));
    pvarValue->vt      = VT_BSTR;
    pvarValue->bstrVal = bStr;
    *pfGotProp         = TRUE;
    return S_OK;
}

ListViewAccServer* ListViewAccServer::CreateProvider(HWND hControl, ListViewAccProvider* provider)
{
    ATL::CComPtr<IAccPropServices> pAccPropSvc;
    HRESULT                        hr = pAccPropSvc.CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_SERVER);
    if (hr == S_OK && pAccPropSvc)
    {
        ListViewAccServer* pLVServer = new (std::nothrow) ListViewAccServer(pAccPropSvc);
        if (pLVServer)
        {
            pLVServer->m_pAccProvider = provider;

            MSAAPROPID propid = PROPID_ACC_HELP;
            pAccPropSvc->SetHwndPropServer(hControl, static_cast<DWORD>(OBJID_CLIENT), CHILDID_SELF, &propid, 1, pLVServer, ANNO_CONTAINER);
            pLVServer->Release();
        }
        return pLVServer;
    }
    return nullptr;
}

void ListViewAccServer::ClearProvider(HWND hControl)
{
    ATL::CComPtr<IAccPropServices> pAccPropSvc;
    HRESULT                        hr = pAccPropSvc.CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_SERVER);
    if (hr == S_OK && pAccPropSvc)
    {
        MSAAPROPID propId = PROPID_ACC_HELP;
        pAccPropSvc->ClearHwndProps(hControl, static_cast<DWORD>(OBJID_CLIENT), CHILDID_SELF, &propId, 1);
    }
}
