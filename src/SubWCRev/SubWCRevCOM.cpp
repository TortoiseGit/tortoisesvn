// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2014 - TortoiseSVN

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
#include <objbase.h>
#include "SubWCRevCOM_h.h"
#include "SubWCRevCOM_i.c"
#include "SubWCRevCOM.h"

#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <Shellapi.h>
#include <comutil.h>
#include <memory>

#pragma warning(push)
#include "apr_pools.h"
#include "svn_error.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_dso.h"
#pragma warning(pop)
#include "Register.h"
#include "UnicodeUtils.h"
#include <atlbase.h>

STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();
static void AutomationMain();
static void RunOutprocServer();
static void ImplWinMain();

int APIENTRY _tWinMain(HINSTANCE /*hInstance*/,
                       HINSTANCE /*hPrevInstance*/,
                       LPTSTR    /*lpCmdLine*/,
                       int       /*nCmdShow*/)
{
    ImplWinMain();
    return 0;
}

static void ImplWinMain()
{
    int argc = 0;
    LPWSTR * argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (argv == 0)
        return;

    if ((argc >= 2) && (argc <= 5))
    {
        if (_tcscmp(argv[1], _T("/automation"))==0)
            AutomationMain();
        else if (_tcscmp(argv[1], _T("unregserver"))==0)
            DllUnregisterServer();
        else if (_tcscmp(argv[1], _T("regserver"))==0)
            DllRegisterServer();
    }
    LocalFree(argv);
}

static void AutomationMain()
{
    // initialize the COM library
    HRESULT hr = ::CoInitialize(NULL);
    if( FAILED( hr ) ) {
        return;
    }

    apr_initialize();
    svn_dso_initialize2();

    apr_pool_t * pool;
    apr_pool_create_ex (&pool, NULL, NULL, NULL);

    size_t ret = 0;
    getenv_s(&ret, NULL, 0, "SVN_ASP_DOT_NET_HACK");
    if (ret)
    {
        svn_wc_set_adm_dir("_svn", pool);
    }

    RunOutprocServer();

    apr_pool_destroy(pool);
    apr_terminate2();

    //
    ::CoUninitialize();
}

// Count of locks to the server - one for each non-factory live object
// plus one for each call of IClassFactory::LockServer(true) not yet
// followed by IClassFactory::LockServer(false)
static long g_cServerLocks = 0;

static void LockServer(bool doLock)
{
    if (doLock)
    {
        InterlockedIncrement(&g_cServerLocks);
        return;
    }
    long newLockCount = InterlockedDecrement(&g_cServerLocks);
    if (newLockCount == 0)
        ::PostMessage(NULL,WM_QUIT,0,0);
}

//
// Constructor
//
SubWCRev::SubWCRev() :
    m_cRef(1),
    SubStat(),
    m_ptinfo(0)
{
    LockServer(true);
    LoadTypeInfo(&m_ptinfo, LIBID_LibSubWCRev, IID_ISubWCRev, 0);
}

//
// Destructor
//
SubWCRev::~SubWCRev()
{
    LockServer(false);
    if (m_ptinfo != 0)
        m_ptinfo->Release();
}

//
// IUnknown implementation
//
HRESULT __stdcall SubWCRev::QueryInterface(const IID& iid, void** ppv)
{
    if(ppv == 0)
        return E_POINTER;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_ISubWCRev) || IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = static_cast<ISubWCRev*>(this) ;
    }
    else
    {
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    AddRef();
    return S_OK ;
}

ULONG __stdcall SubWCRev::AddRef()
{
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall SubWCRev::Release()
{
    const LONG refCount = InterlockedDecrement(&m_cRef);
    if (refCount == 0)
        delete this;
    return refCount;
}

HRESULT SubWCRev::GetWCInfoInternal(/*[in]*/ BSTR wcPath, /*[in]*/VARIANT_BOOL folders, /*[in]*/VARIANT_BOOL externals)
{
    SubStat.bFolders = folders;
    SubStat.bExternals = externals;

    apr_pool_t * pool;
    apr_pool_create_ex (&pool, NULL, NULL, NULL);

    size_t ret = 0;
    getenv_s(&ret, NULL, 0, "SVN_ASP_DOT_NET_HACK");
    if (ret)
    {
        svn_wc_set_adm_dir("_svn", pool);
    }

    char *wc_utf8 = Utf16ToUtf8((WCHAR*)wcPath, pool);
    const char * internalpath = svn_path_internal_style (wc_utf8, pool);

    svn_client_ctx_t * ctx;
    svn_client_create_context2(&ctx, NULL, pool);

    svn_error_t * svnerr = svn_status(  internalpath,   //path
        &SubStat,       //status_baton
        TRUE,           //noignore
        ctx,
        pool);

    HRESULT hr = S_OK;
    if (svnerr)
    {
        hr = S_FALSE;
        svn_error_clear(svnerr);
    }
    apr_pool_destroy(pool);
    return hr;
}
//
// ISubWCRev implementation
//
HRESULT __stdcall SubWCRev::GetWCInfo2(/*[in]*/ BSTR wcPath, /*[in]*/VARIANT_BOOL folders, /*[in]*/VARIANT_BOOL externals, /*[in]*/VARIANT_BOOL externalsNoMixed)
{
    if (wcPath==NULL)
        return E_INVALIDARG;

    SecureZeroMemory(&SubStat, sizeof(SubStat));
    SubStat.bExternalsNoMixedRevision = externalsNoMixed;
    return GetWCInfoInternal(wcPath, folders, externals);
}

HRESULT __stdcall SubWCRev::GetWCInfo(/*[in]*/ BSTR wcPath, /*[in]*/VARIANT_BOOL folders, /*[in]*/VARIANT_BOOL externals)
{
    if (wcPath==NULL)
        return E_INVALIDARG;

    SecureZeroMemory(&SubStat, sizeof(SubStat));
    return GetWCInfoInternal(wcPath, folders, externals);
}

HRESULT __stdcall SubWCRev::get_Revision(/*[out, retval]*/VARIANT* rev)
{
    return LongToVariant(SubStat.CmtRev, rev);
}

HRESULT __stdcall SubWCRev::get_MinRev(/*[out, retval]*/VARIANT* rev)
{
    return LongToVariant(SubStat.MinRev, rev);
}

HRESULT __stdcall SubWCRev::get_MaxRev(/*[out, retval]*/VARIANT* rev)
{
    return LongToVariant(SubStat.MaxRev, rev);
}

HRESULT __stdcall SubWCRev::get_Date(/*[out, retval]*/VARIANT* date)
{
    if(date == 0)
        return E_POINTER;

    date->vt = VT_BSTR;

    WCHAR destbuf[32] = { 0 };
    HRESULT result = CopyDateToString(destbuf, _countof(destbuf), SubStat.CmtDate) ? S_OK : S_FALSE;
    if(S_FALSE == result)
    {
        _stprintf_s(destbuf, _T(""));
    }

    date->bstrVal = SysAllocStringLen(destbuf, (UINT)_tcslen(destbuf));
    return result;
}

HRESULT __stdcall SubWCRev::get_Url(/*[out, retval]*/VARIANT* url)
{
    return Utf8StringToVariant(SubStat.Url, url);
}

HRESULT __stdcall SubWCRev::get_Author(/*[out, retval]*/VARIANT* author)
{
    return Utf8StringToVariant(SubStat.Author, author);
}

HRESULT SubWCRev::Utf8StringToVariant(const char* string, VARIANT* result )
{
    if(result == 0)
        return E_POINTER;

    result->vt = VT_BSTR;
    const size_t len = strlen(string);
    std::unique_ptr<WCHAR[]> buf(new WCHAR[len*4 + 1]);
    SecureZeroMemory(buf.get(), (len*4 + 1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, string, -1, buf.get(), (int)len*4);
    result->bstrVal = SysAllocString(buf.get());
    return S_OK;
}

HRESULT __stdcall SubWCRev::get_HasModifications(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(SubStat.HasMods, modifications);
}

HRESULT __stdcall SubWCRev::get_HasUnversioned(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(SubStat.HasUnversioned, modifications);
}

HRESULT __stdcall SubWCRev::get_HasMixedRevisions(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(((SubStat.MinRev != SubStat.MaxRev) || SubStat.bIsExternalMixed), modifications);
}

HRESULT __stdcall SubWCRev::get_HaveExternalsAllFixedRevision(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(!SubStat.bIsExternalsNotFixed, modifications);
}

HRESULT __stdcall SubWCRev::get_IsWcTagged(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(SubStat.bIsTagged, modifications);
}

HRESULT __stdcall SubWCRev::get_IsSvnItem(/*[out, retval]*/VARIANT_BOOL* svn_item)
{
    return BoolToVariantBool(SubStat.bIsSvnItem, svn_item);
}

HRESULT __stdcall SubWCRev::get_NeedsLocking(/*[out, retval]*/VARIANT_BOOL* needs_locking)
{
    return BoolToVariantBool(SubStat.LockData.NeedsLocks, needs_locking);
}

HRESULT __stdcall SubWCRev::get_IsLocked(/*[out, retval]*/VARIANT_BOOL* locked)
{
    return BoolToVariantBool(SubStat.LockData.IsLocked, locked);
}

HRESULT SubWCRev::BoolToVariantBool(BOOL value, VARIANT_BOOL* result)
{
    if(result == 0)
        return E_POINTER;
    *result = (value == 0) ? VARIANT_FALSE : VARIANT_TRUE;
    return S_OK;
}

HRESULT SubWCRev::LongToVariant(LONG value, VARIANT* result)
{
    if(result == 0)
        return E_POINTER;

    result->vt = VT_I4;
    result->lVal = value;
    return S_OK;
}

HRESULT __stdcall SubWCRev::get_LockCreationDate(/*[out, retval]*/VARIANT* date)
{
    if(date == 0)
        return E_POINTER;

    date->vt = VT_BSTR;

    WCHAR destbuf[32] = { 0 };
    HRESULT result = S_OK;
    if(FALSE == IsLockDataAvailable())
    {
        _stprintf_s(destbuf, _T(""));
        result = S_FALSE;
    }
    else
    {
        result = CopyDateToString(destbuf, _countof(destbuf), SubStat.LockData.CreationDate) ? S_OK : S_FALSE;
        if(S_FALSE == result)
        {
            _stprintf_s(destbuf, _T(""));
        }
    }

    date->bstrVal = SysAllocStringLen(destbuf, (UINT)_tcslen(destbuf));
    return result;
}

HRESULT __stdcall SubWCRev::get_LockOwner(/*[out, retval]*/VARIANT* owner)
{
    if(owner == 0)
        return E_POINTER;

    owner->vt = VT_BSTR;

    HRESULT result;
    size_t len;

    if(FALSE == IsLockDataAvailable())
    {
        len = 0;
        result = S_FALSE;
    }
    else
    {
        len = strlen(SubStat.LockData.Owner);
        result = S_OK;
    }

    std::unique_ptr<WCHAR[]> buf (new WCHAR[len*4 + 1]);
    SecureZeroMemory(buf.get(), (len*4 + 1)*sizeof(WCHAR));

    if(TRUE == SubStat.LockData.NeedsLocks)
    {
        MultiByteToWideChar(CP_UTF8, 0, SubStat.LockData.Owner, -1, buf.get(), (int)len*4);
    }

    owner->bstrVal = SysAllocString(buf.get());
    return result;
}

HRESULT __stdcall SubWCRev::get_LockComment(/*[out, retval]*/VARIANT* comment)
{
    if(comment == 0)
        return E_POINTER;

    comment->vt = VT_BSTR;

    HRESULT result;
    size_t len;

    if(FALSE == IsLockDataAvailable())
    {
        len = 0;
        result = S_FALSE;
    }
    else
    {
        len = strlen(SubStat.LockData.Comment);
        result = S_OK;
    }

    std::unique_ptr<WCHAR[]> buf (new WCHAR[len*4 + 1]);
    SecureZeroMemory(buf.get(), (len*4 + 1)*sizeof(WCHAR));

    if(TRUE == SubStat.LockData.NeedsLocks)
    {
        MultiByteToWideChar(CP_UTF8, 0, SubStat.LockData.Comment, -1, buf.get(), (int)len*4);
    }

    comment->bstrVal = SysAllocString(buf.get());
    return result;
}

//
// Get a readable string from a apr_time_t date
//
BOOL SubWCRev::CopyDateToString(WCHAR *destbuf, int buflen, apr_time_t time)
{
    const int min_buflen = 32;
    if((NULL == destbuf) || (min_buflen > buflen))
    {
        return FALSE;
    }

    const __time64_t ttime = time/1000000L;

    struct tm newtime;
    if (_localtime64_s(&newtime, &ttime))
        return FALSE;
    // Format the date/time in international format as yyyy/mm/dd hh:mm:ss
    _stprintf_s(destbuf, min_buflen, _T("%04d/%02d/%02d %02d:%02d:%02d"),
        newtime.tm_year + 1900,
        newtime.tm_mon + 1,
        newtime.tm_mday,
        newtime.tm_hour,
        newtime.tm_min,
        newtime.tm_sec);
    return TRUE;
}

BOOL SubWCRev::IsLockDataAvailable()
{
    BOOL bResult = (SubStat.LockData.NeedsLocks && SubStat.LockData.IsLocked);
    return bResult;
}

HRESULT SubWCRev::LoadTypeInfo(ITypeInfo ** pptinfo, const CLSID &libid, const CLSID &iid, LCID lcid)
{
    if(pptinfo == 0)
        return E_POINTER;
    *pptinfo = NULL;

    // Load type library.
    CComPtr<ITypeLib> ptlib;
    HRESULT hr = LoadRegTypeLib(libid, 1, 0, lcid, &ptlib);
    if (FAILED(hr))
        return hr;

    // Get type information for interface of the object.
    LPTYPEINFO ptinfo = NULL;
    hr = ptlib->GetTypeInfoOfGuid(iid, &ptinfo);
    if (FAILED(hr))
        return hr;

    *pptinfo = ptinfo;
    return S_OK;
}

HRESULT __stdcall SubWCRev::GetTypeInfoCount(UINT* pctinfo)
{
    if(pctinfo == 0)
        return E_POINTER;

    *pctinfo = 1;
    return S_OK;
}

HRESULT __stdcall SubWCRev::GetTypeInfo(UINT itinfo, LCID /*lcid*/, ITypeInfo** pptinfo)
{
    if(pptinfo == 0)
        return E_POINTER;

    *pptinfo = NULL;

    if(itinfo != 0)
        return ResultFromScode(DISP_E_BADINDEX);

    if (m_ptinfo == 0)
        return E_UNEXPECTED;

    m_ptinfo->AddRef();      // AddRef and return pointer to cached
    // typeinfo for this object.
    *pptinfo = m_ptinfo;

    return S_OK;
}

HRESULT __stdcall SubWCRev::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames, UINT cNames,
                                             LCID /*lcid*/, DISPID* rgdispid)
{
    return DispGetIDsOfNames(m_ptinfo, rgszNames, cNames, rgdispid);
}

HRESULT __stdcall SubWCRev::Invoke(DISPID dispidMember, REFIID /*riid*/,
                                      LCID /*lcid*/, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                                      EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    return DispInvoke(
        this, m_ptinfo,
        dispidMember, wFlags, pdispparams,
        pvarResult, pexcepinfo, puArgErr);
}

//
// Class factory IUnknown implementation
//
HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{
    if(ppv == 0)
        return E_POINTER;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IClassFactory))
    {
        *ppv = static_cast<IClassFactory*>(this) ;
    }
    else
    {
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    AddRef();
    return S_OK ;
}

ULONG __stdcall CFactory::AddRef()
{
    // No true reference counting in global object
    return 1;
}

ULONG __stdcall CFactory::Release()
{
    // No true reference counting in global object
    return 1;
}

//
// IClassFactory implementation
//
HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{
    if (ppv == 0)
        return E_POINTER;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION ;
    }

    // Create component.
    ATL::CComPtr<SubWCRev> pA;
    pA.Attach(new (std::nothrow) SubWCRev());// refcount set to 1 in constructor
    if (pA == NULL)
    {
        return E_OUTOFMEMORY ;
    }

    return pA->QueryInterface(iid, ppv);
}

// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
    ::LockServer(bLock != 0);
    return S_OK ;
}

// Refcount set to 1 in constructor.
CFactory gClassFactory;


static void RunOutprocServer()
{
    // register ourself as a class object against the internal COM table
    DWORD nToken = 0;
    HRESULT hr = ::CoRegisterClassObject(CLSID_SubWCRev,
        &gClassFactory,
        CLSCTX_SERVER,
        REGCLS_MULTIPLEUSE,
        &nToken);

    // If the class factory is not registered no object will ever be instantiated,
    // hence no object will ever be released, hence WM_QUIT never appears in the
    // message queue and the message loop below will run forever.
    if(FAILED(hr))
        return;

    //
    // (loop ends if WM_QUIT message is received)
    //
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // unregister from the known table of class objects
    ::CoRevokeClassObject(nToken);
}

//
// Server registration
//
STDAPI DllRegisterServer()
{
    const HMODULE hModule = ::GetModuleHandle(NULL);

    HRESULT hr = RegisterServer(hModule,
        CLSID_SubWCRev,
        _T("SubWCRev Server Object"),
        _T("SubWCRev.object"),
        _T("SubWCRev.object.1"),
        LIBID_LibSubWCRev) ;
    if (SUCCEEDED(hr))
    {
        RegisterTypeLib(hModule, NULL);
    }
    return hr;
}

//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    const HMODULE hModule = ::GetModuleHandle(NULL);

    HRESULT hr = UnregisterServer(CLSID_SubWCRev,
        _T("SubWCRev.object"),
        _T("SubWCRev.object.1"),
        LIBID_LibSubWCRev) ;
    if (SUCCEEDED(hr))
    {
        UnRegisterTypeLib(hModule, NULL);
    }
    return hr;
}
