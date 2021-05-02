// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2012, 2018, 2021 - TortoiseSVN

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
#pragma once

#include <string>

class CCallback : public IBindStatusCallback
    , public IAuthenticate
    , public IHttpSecurity
{
public:
    CCallback();
    virtual ~CCallback();

    void SetAuthData(const std::wstring& username, const std::wstring& password)
    {
        m_username = username;
        m_password = password;
    }
    void SetAuthParentWindow(HWND hWnd) { m_hWnd = hWnd; }

    // IHttpSecurity method
    HRESULT STDMETHODCALLTYPE OnSecurityProblem(DWORD /*dwProblem*/) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetWindow(/* [in] */ REFGUID /*rguidReason*/,
                                        /* [out] */ HWND* /*phwnd*/) override
    {
        return E_NOTIMPL;
    }

    // IAuthenticate method
    HRESULT STDMETHODCALLTYPE Authenticate(/* [out] */ HWND*   phwnd,
                                           /* [out] */ LPWSTR* pszUsername,
                                           /* [out] */ LPWSTR* pszPassword) override;

    // IBindStatusCallback methods.  Note that the only method called by IE
    // is OnProgress(), so the others just return E_NOTIMPL.

    HRESULT STDMETHODCALLTYPE OnStartBinding(/* [in] */ DWORD /*dwReserved*/,
                                             /* [in] */ IBinding __RPC_FAR* /*pib*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetPriority(/* [out] */ LONG __RPC_FAR* /*pnPriority*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE OnLowResource(/* [in] */ DWORD /*reserved*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE OnProgress(/* [in] */ ULONG /*ulProgress*/,
                                         /* [in] */ ULONG /*ulProgressMax*/,
                                         /* [in] */ ULONG /*ulStatusCode*/,
                                         /* [in] */ LPCWSTR /*wszStatusText*/) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnStopBinding(/* [in] */ HRESULT /*hresult*/,
                                            /* [unique][in] */ LPCWSTR /*szError*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetBindInfo(/* [out] */ DWORD                __RPC_FAR* /*grfBINDF*/,
                                          /* [unique][out][in] */ BINDINFO __RPC_FAR* /*pbindinfo*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE OnDataAvailable(/* [in] */ DWORD /*grfBSCF*/,
                                              /* [in] */ DWORD /*dwSize*/,
                                              /* [in] */ FORMATETC __RPC_FAR* /*pformatetc*/,
                                              /* [in] */ STGMEDIUM __RPC_FAR* /*pstgmed*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE OnObjectAvailable(/* [in] */ REFIID /*riid*/,
                                                /* [iid_is][in] */ IUnknown __RPC_FAR* /*punk*/) override
    {
        return E_NOTIMPL;
    }

    // IUnknown methods.  Note that IE never calls any of these methods, since
    // the caller owns the IBindStatusCallback interface, so the methods all
    // return zero/E_NOTIMPL.

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return 0;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(/* [in] */ REFIID        riid,
                                             /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject) override
    {
        if (riid == IID_IAuthenticate)
        {
            *ppvObject = static_cast<void*>(static_cast<IAuthenticate*>(this));
            return S_OK;
        }
        else if (riid == IID_IUnknown)
        {
            *ppvObject = static_cast<void*>(static_cast<IUnknown*>(static_cast<IBindStatusCallback*>(this)));
            return S_OK;
        }
        else if (riid == IID_IBindStatusCallback)
        {
            *ppvObject = static_cast<void*>(static_cast<IBindStatusCallback*>(this));
            return S_OK;
        }
        else if (riid == IID_IHttpSecurity)
        {
            *ppvObject = static_cast<void*>(static_cast<IHttpSecurity*>(this));
            return S_OK;
        }
        else if (riid == IID_IWindowForBindingUI)
        {
            *ppvObject = static_cast<void*>(static_cast<IWindowForBindingUI*>(this));
            return S_OK;
        }
        return E_NOINTERFACE;
    }

private:
    ULONG        m_cRef;
    std::wstring m_username;
    std::wstring m_password;
    HWND         m_hWnd;
};
