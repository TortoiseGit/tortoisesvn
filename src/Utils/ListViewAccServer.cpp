// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010 - TortoiseSVN

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
	if (ppvObject == 0)
		return E_POINTER;
	*ppvObject = NULL;
	if (IID_IUnknown==riid || IID_IAccPropServer==riid)
		*ppvObject=this;

	if (NULL != *ppvObject)
	{
		((LPUNKNOWN)*ppvObject)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ListViewAccServer::AddRef(void)
{
	return ++m_Ref;
}

STDMETHODIMP_(ULONG) ListViewAccServer::Release(void)
{
	--m_Ref;
	return m_Ref;
}

HRESULT STDMETHODCALLTYPE ListViewAccServer::GetPropValue( 
	const BYTE *    pIDString,
	DWORD           dwIDStringLen,
	MSAAPROPID      /*idProp*/,
	VARIANT *       pvarValue,
	BOOL *          pfGotProp )
{
	// Default return values, in case we need to bail out…
	*pfGotProp = FALSE;
	pvarValue->vt = VT_EMPTY;
	// Extract the idChild from the identity string…
	DWORD dwHcontrol, idObject, idChild;
	HRESULT hr = m_pAccPropSvc->DecomposeHwndIdentityString(pIDString, dwIDStringLen, (HWND*)&dwHcontrol, &idObject, &idChild);
	if (hr != S_OK)
	{
		return S_OK;
	}

	HWND Hwnd = (HWND)dwHcontrol;
	// Only supply help string for child elements, not the 
	// listview itself… 
	if (idChild == CHILDID_SELF)
	{
		return S_OK;
	}

	// GetHelpString returns a UNICODE string corresponding to
	// the index it is passed.
	CString sHelpString = m_pAccProvider->GetListviewHelpString(Hwnd, idChild - 1);
	if (sHelpString.IsEmpty())
	{
		return S_OK;
	}

	BSTR bstr = SysAllocString((LPCTSTR)sHelpString);
	pvarValue->vt = VT_BSTR;
	pvarValue->bstrVal = bstr;
	*pfGotProp = TRUE;
	return S_OK;
}

ListViewAccServer * ListViewAccServer::CreateProvider(HWND hControl, ListViewAccProvider * provider)
{
	ATL::CComPtr<IAccPropServices> pAccPropSvc;
	HRESULT hr = pAccPropSvc.CoCreateInstance(CLSID_AccPropServices, NULL, CLSCTX_SERVER);
	if (hr == S_OK && pAccPropSvc)
	{
		ListViewAccServer * pLVServer = new (std::nothrow) ListViewAccServer(pAccPropSvc);
		if (pLVServer)
		{
			pLVServer->m_pAccProvider = provider;

			MSAAPROPID propid = PROPID_ACC_HELP;
			pAccPropSvc->SetHwndPropServer(hControl, (DWORD)OBJID_CLIENT, CHILDID_SELF, &propid, 1, pLVServer, ANNO_CONTAINER);
			pLVServer->Release();
		}
		return pLVServer;
	}
	return NULL;
}
