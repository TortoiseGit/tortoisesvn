// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2012, 2014 - TortoiseSVN

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
#include "Callback.h"

#include <urlmon.h>
#include <shlwapi.h>                    // for StrFormatByteSize()

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCallback::CCallback()
    : m_cRef(0)
    , m_hWnd(NULL)
{
}

CCallback::~CCallback()
{
}

STDMETHODIMP CCallback::Authenticate( HWND * phwnd, LPWSTR * pszUsername, LPWSTR * pszPassword)
{
    *phwnd = m_hWnd;
    if (m_username.size())
    {
        if (pszUsername == nullptr)
            return E_INVALIDARG;
        *pszUsername = (LPWSTR)CoTaskMemAlloc((m_username.size() + 1) * 2);
        if (*pszUsername == nullptr)
            return E_OUTOFMEMORY;
        wcscpy_s(*pszUsername, m_username.size() + 1, m_username.c_str());
    }
    else
        pszUsername = NULL;
    if (m_password.size())
    {
        if (pszPassword == nullptr)
            return E_INVALIDARG;
        *pszPassword = (LPWSTR)CoTaskMemAlloc((m_password.size() + 1) * 2);
        if (*pszPassword == nullptr)
            return E_OUTOFMEMORY;
        wcscpy_s(*pszPassword, m_password.size()+1, m_password.c_str());
    }
    else
        pszPassword = NULL;
    return S_OK;
}

