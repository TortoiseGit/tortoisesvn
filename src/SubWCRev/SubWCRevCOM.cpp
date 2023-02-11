// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2015, 2017, 2021, 2023 - TortoiseSVN

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

#pragma warning(push)
#include "apr_pools.h"
#include "svn_error.h"
#include "svn_client.h"
#include "svn_dirent_uri.h"
#include "svn_dso.h"
#pragma warning(pop)
#include <objbase.h>
#include "SubWCRevCOM_h.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "SubWCRevCOM_i.c"
#include "SubWCRevCOM.h"

#include "Register.h"
#include "UnicodeUtils.h"
#include <atlbase.h>

#include <windows.h>
#include <shlwapi.h>
#include <Shellapi.h>
#include <comutil.h>
#include <memory>

STDAPI      DllRegisterServer();
STDAPI      DllUnregisterServer();
static void AutomationMain();
static void RunOutprocServer();
static void ImplWinMain();

int APIENTRY wWinMain(HINSTANCE /*hInstance*/,
                      HINSTANCE /*hPrevInstance*/,
                      LPWSTR /*lpCmdLine*/,
                      int /*nCmdShow*/)
{
    ImplWinMain();
    return 0;
}

static void ImplWinMain()
{
    int     argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (argv == nullptr)
        return;

    if ((argc >= 2) && (argc <= 5))
    {
        if (wcscmp(argv[1], L"/automation") == 0)
            AutomationMain();
        else if (wcscmp(argv[1], L"unregserver") == 0)
            DllUnregisterServer();
        else if (wcscmp(argv[1], L"regserver") == 0)
            DllRegisterServer();
    }
    LocalFree(argv);
}

static void AutomationMain()
{
    // initialize the COM library
    HRESULT hr = ::CoInitialize(nullptr);
    if (FAILED(hr))
    {
        return;
    }

    apr_initialize();
    svn_dso_initialize2();

    apr_pool_t* pool;
    apr_pool_create_ex(&pool, nullptr, nullptr, nullptr);

    size_t ret = 0;
    getenv_s(&ret, nullptr, 0, "SVN_ASP_DOT_NET_HACK");
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
        ::PostMessage(nullptr, WM_QUIT, 0, 0);
}

//
// Constructor
//
SubWCRev::SubWCRev()
    : m_cRef(1)
    , m_ptInfo(nullptr)
{
    LockServer(true);
    LoadTypeInfo(&m_ptInfo, LIBID_LibSubWCRev, IID_ISubWCRev, 0);
}

//
// Destructor
//
SubWCRev::~SubWCRev()
{
    LockServer(false);
    if (m_ptInfo != nullptr)
        m_ptInfo->Release();
}

//
// IUnknown implementation
//
HRESULT __stdcall SubWCRev::QueryInterface(const IID& iid, void** ppv)
{
    if (ppv == nullptr)
        return E_POINTER;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_ISubWCRev) || IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = static_cast<ISubWCRev*>(this);
    }
    else
    {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG __stdcall SubWCRev::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall SubWCRev::Release()
{
    const LONG refCount = InterlockedDecrement(&m_cRef);
    if (refCount == 0)
        delete this;
    return refCount;
}

HRESULT SubWCRev::GetWCInfoInternal(/*[in]*/ BSTR wcPath, /*[in]*/ VARIANT_BOOL folders, /*[in]*/ VARIANT_BOOL externals)
{
    m_subStat.bFolders   = folders;
    m_subStat.bExternals = externals;

    // clear all possible previous data
    m_subStat.minRev               = 0;
    m_subStat.maxRev               = 0;
    m_subStat.cmtRev               = 0;
    m_subStat.cmtDate              = 0;
    m_subStat.hasMods              = FALSE;
    m_subStat.hasUnversioned       = FALSE;
    m_subStat.bHexPlain            = FALSE;
    m_subStat.bHexX                = FALSE;
    m_subStat.bIsSvnItem           = FALSE;
    m_subStat.bIsExternalsNotFixed = FALSE;
    m_subStat.bIsExternalMixed     = FALSE;
    m_subStat.bIsTagged            = FALSE;
    SecureZeroMemory(m_subStat.url, sizeof(m_subStat.url));
    SecureZeroMemory(m_subStat.rootUrl, sizeof(m_subStat.rootUrl));
    SecureZeroMemory(m_subStat.author, sizeof(m_subStat.author));
    SecureZeroMemory(&m_subStat.lockData, sizeof(m_subStat.lockData));

    if ((wcPath == nullptr) || (wcPath[0] == 0))
        return S_FALSE;
    if (!PathFileExists(wcPath))
        return S_FALSE;

    apr_pool_t* pool = nullptr;
    apr_pool_create_ex(&pool, nullptr, nullptr, nullptr);

    size_t ret = 0;
    getenv_s(&ret, nullptr, 0, "SVN_ASP_DOT_NET_HACK");
    if (ret)
    {
        svn_wc_set_adm_dir("_svn", pool);
    }

    char*       wcUTF8       = Utf16ToUtf8(wcPath, pool);
    const char* internalPath = svn_dirent_internal_style(wcUTF8, pool);

    apr_hash_t* config = nullptr;
    ;
    svn_config_get_config(&(config), nullptr, pool);
    svn_client_ctx_t* ctx;
    svn_client_create_context2(&ctx, config, pool);

    svn_error_t* svnErr = nullptr;
    const char*  wcRoot;
    svnErr = svn_client_get_wc_root(&wcRoot, internalPath, ctx, pool, pool);
    if ((svnErr == nullptr) && wcRoot)
        loadIgnorePatterns(wcRoot, &m_subStat);
    svn_error_clear(svnErr);
    loadIgnorePatterns(internalPath, &m_subStat);

    svnErr = svnStatus(internalPath, //path
                       &m_subStat,   //status_baton
                       TRUE,         //noignore
                       ctx,
                       pool);

    HRESULT hr = S_OK;
    if (svnErr)
    {
        hr = S_FALSE;
        svn_error_clear(svnErr);
    }
    apr_pool_destroy(pool);
    return hr;
}
//
// ISubWCRev implementation
//
HRESULT __stdcall SubWCRev::GetWCInfo2(/*[in]*/ BSTR wcPath, /*[in]*/ VARIANT_BOOL folders, /*[in]*/ VARIANT_BOOL externals, /*[in]*/ VARIANT_BOOL externalsNoMixed)
{
    if (wcPath == nullptr)
        return E_INVALIDARG;

    m_subStat.bExternalsNoMixedRevision = externalsNoMixed;
    return GetWCInfoInternal(wcPath, folders, externals);
}

HRESULT __stdcall SubWCRev::GetWCInfo(/*[in]*/ BSTR wcPath, /*[in]*/ VARIANT_BOOL folders, /*[in]*/ VARIANT_BOOL externals)
{
    if (wcPath == nullptr)
        return E_INVALIDARG;

    return GetWCInfoInternal(wcPath, folders, externals);
}

HRESULT __stdcall SubWCRev::get_Revision(/*[out, retval]*/ VARIANT* rev)
{
    return LongToVariant(m_subStat.cmtRev, rev);
}

HRESULT __stdcall SubWCRev::get_MinRev(/*[out, retval]*/ VARIANT* rev)
{
    return LongToVariant(m_subStat.minRev, rev);
}

HRESULT __stdcall SubWCRev::get_MaxRev(/*[out, retval]*/ VARIANT* rev)
{
    return LongToVariant(m_subStat.maxRev, rev);
}

HRESULT __stdcall SubWCRev::get_Date(/*[out, retval]*/ VARIANT* date)
{
    if (date == nullptr)
        return E_POINTER;

    date->vt = VT_BSTR;

    WCHAR   destbuf[32] = {0};
    HRESULT result      = CopyDateToString(destbuf, _countof(destbuf), m_subStat.cmtDate) ? S_OK : S_FALSE;
    if (S_FALSE == result)
    {
        swprintf_s(destbuf, L"");
    }

    date->bstrVal = SysAllocStringLen(destbuf, static_cast<UINT>(wcslen(destbuf)));
    return result;
}

HRESULT __stdcall SubWCRev::get_Url(/*[out, retval]*/ VARIANT* url)
{
    return Utf8StringToVariant(m_subStat.url, url);
}

HRESULT __stdcall SubWCRev::get_Author(/*[out, retval]*/ VARIANT* author)
{
    return Utf8StringToVariant(m_subStat.author, author);
}

HRESULT SubWCRev::Utf8StringToVariant(const char* string, VARIANT* result)
{
    if (result == nullptr)
        return E_POINTER;

    result->vt       = VT_BSTR;
    const size_t len = strlen(string);
    auto         buf = std::make_unique<wchar_t[]>(len * 4 + 1);
    SecureZeroMemory(buf.get(), (len * 4 + 1) * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, string, -1, buf.get(), static_cast<int>(len) * 4);
    result->bstrVal = SysAllocString(buf.get());
    return S_OK;
}

HRESULT __stdcall SubWCRev::get_HasModifications(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(m_subStat.hasMods, modifications);
}

HRESULT __stdcall SubWCRev::get_HasUnversioned(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(m_subStat.hasUnversioned, modifications);
}

HRESULT __stdcall SubWCRev::get_HasMixedRevisions(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(((m_subStat.minRev != m_subStat.maxRev) || m_subStat.bIsExternalMixed), modifications);
}

HRESULT __stdcall SubWCRev::get_HaveExternalsAllFixedRevision(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(!m_subStat.bIsExternalsNotFixed, modifications);
}

HRESULT __stdcall SubWCRev::get_IsWcTagged(VARIANT_BOOL* modifications)
{
    return BoolToVariantBool(m_subStat.bIsTagged, modifications);
}

HRESULT __stdcall SubWCRev::get_IsSvnItem(/*[out, retval]*/ VARIANT_BOOL* svnItem)
{
    return BoolToVariantBool(m_subStat.bIsSvnItem, svnItem);
}

HRESULT __stdcall SubWCRev::get_NeedsLocking(/*[out, retval]*/ VARIANT_BOOL* needsLocking)
{
    return BoolToVariantBool(m_subStat.lockData.needsLocks, needsLocking);
}

HRESULT __stdcall SubWCRev::get_IsLocked(/*[out, retval]*/ VARIANT_BOOL* locked)
{
    return BoolToVariantBool(m_subStat.lockData.isLocked, locked);
}

HRESULT SubWCRev::BoolToVariantBool(BOOL value, VARIANT_BOOL* result)
{
    if (result == nullptr)
        return E_POINTER;
    *result = (value == 0) ? VARIANT_FALSE : VARIANT_TRUE;
    return S_OK;
}

HRESULT SubWCRev::LongToVariant(LONG value, VARIANT* result)
{
    if (result == nullptr)
        return E_POINTER;

    result->vt   = VT_I4;
    result->lVal = value;
    return S_OK;
}

HRESULT __stdcall SubWCRev::get_LockCreationDate(/*[out, retval]*/ VARIANT* date)
{
    if (date == nullptr)
        return E_POINTER;

    date->vt = VT_BSTR;

    WCHAR   destbuf[32] = {0};
    HRESULT result      = S_OK;
    if (FALSE == IsLockDataAvailable())
    {
        swprintf_s(destbuf, L"");
        result = S_FALSE;
    }
    else
    {
        result = CopyDateToString(destbuf, _countof(destbuf), m_subStat.lockData.creationDate) ? S_OK : S_FALSE;
        if (S_FALSE == result)
        {
            swprintf_s(destbuf, L"");
        }
    }

    date->bstrVal = SysAllocStringLen(destbuf, static_cast<UINT>(wcslen(destbuf)));
    return result;
}

HRESULT __stdcall SubWCRev::get_LockOwner(/*[out, retval]*/ VARIANT* owner)
{
    if (owner == nullptr)
        return E_POINTER;

    owner->vt = VT_BSTR;

    HRESULT result;
    size_t  len;

    if (FALSE == IsLockDataAvailable())
    {
        len    = 0;
        result = S_FALSE;
    }
    else
    {
        len    = strlen(m_subStat.lockData.owner);
        result = S_OK;
    }

    auto buf = std::make_unique<wchar_t[]>(len * 4 + 1);
    SecureZeroMemory(buf.get(), (len * 4 + 1) * sizeof(WCHAR));

    if (TRUE == m_subStat.lockData.needsLocks)
    {
        MultiByteToWideChar(CP_UTF8, 0, m_subStat.lockData.owner, -1, buf.get(), static_cast<int>(len) * 4);
    }

    owner->bstrVal = SysAllocString(buf.get());
    return result;
}

HRESULT __stdcall SubWCRev::get_LockComment(/*[out, retval]*/ VARIANT* comment)
{
    if (comment == nullptr)
        return E_POINTER;

    comment->vt = VT_BSTR;

    HRESULT result = 0;
    size_t  len    = 0;

    if (FALSE == IsLockDataAvailable())
    {
        len    = 0;
        result = S_FALSE;
    }
    else
    {
        len    = strlen(m_subStat.lockData.comment);
        result = S_OK;
    }

    auto buf = std::make_unique<wchar_t[]>(len * 4 + 1);
    SecureZeroMemory(buf.get(), (len * 4 + 1) * sizeof(WCHAR));

    if (TRUE == m_subStat.lockData.needsLocks)
    {
        MultiByteToWideChar(CP_UTF8, 0, m_subStat.lockData.comment, -1, buf.get(), static_cast<int>(len) * 4);
    }

    comment->bstrVal = SysAllocString(buf.get());
    return result;
}

HRESULT __stdcall SubWCRev::get_RepoRoot(/*[out, retval]*/ VARIANT* url)
{
    return Utf8StringToVariant(m_subStat.rootUrl, url);
}

//
// Get a readable string from a apr_time_t date
//
BOOL SubWCRev::CopyDateToString(WCHAR* destbuf, int buflen, apr_time_t time) const
{
    const int minBuflen = 32;
    if ((nullptr == destbuf) || (minBuflen > buflen))
    {
        return FALSE;
    }

    const __time64_t tTime = time / 1000000L;

    struct tm newTime;
    if (_localtime64_s(&newTime, &tTime))
        return FALSE;
    // Format the date/time in international format as yyyy/mm/dd hh:mm:ss
    swprintf_s(destbuf, minBuflen, L"%04d/%02d/%02d %02d:%02d:%02d",
               newTime.tm_year + 1900,
               newTime.tm_mon + 1,
               newTime.tm_mday,
               newTime.tm_hour,
               newTime.tm_min,
               newTime.tm_sec);
    return TRUE;
}

BOOL SubWCRev::IsLockDataAvailable() const
{
    BOOL bResult = (m_subStat.lockData.needsLocks && m_subStat.lockData.isLocked);
    return bResult;
}

HRESULT SubWCRev::LoadTypeInfo(ITypeInfo** pptinfo, const CLSID& libid, const CLSID& iid, LCID lcid)
{
    if (pptinfo == nullptr)
        return E_POINTER;
    *pptinfo = nullptr;

    // Load type library.
    CComPtr<ITypeLib> ptLib;
    HRESULT           hr = LoadRegTypeLib(libid, 1, 0, lcid, &ptLib);
    if (FAILED(hr))
        return hr;

    // Get type information for interface of the object.
    LPTYPEINFO ptinfo = nullptr;
    hr                = ptLib->GetTypeInfoOfGuid(iid, &ptinfo);
    if (FAILED(hr))
        return hr;

    *pptinfo = ptinfo;
    return S_OK;
}

HRESULT __stdcall SubWCRev::GetTypeInfoCount(UINT* pctinfo)
{
    if (pctinfo == nullptr)
        return E_POINTER;

    *pctinfo = 1;
    return S_OK;
}

HRESULT __stdcall SubWCRev::GetTypeInfo(UINT itinfo, LCID /*lcid*/, ITypeInfo** pptinfo)
{
    if (pptinfo == nullptr)
        return E_POINTER;

    *pptinfo = nullptr;

    if (itinfo != 0)
        return ResultFromScode(DISP_E_BADINDEX);

    if (m_ptInfo == nullptr)
        return E_UNEXPECTED;

    m_ptInfo->AddRef(); // AddRef and return pointer to cached
    // typeinfo for this object.
    *pptinfo = m_ptInfo;

    return S_OK;
}

HRESULT __stdcall SubWCRev::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames, UINT cNames,
                                          LCID /*lcid*/, DISPID* rgdispid)
{
    return DispGetIDsOfNames(m_ptInfo, rgszNames, cNames, rgdispid);
}

HRESULT __stdcall SubWCRev::Invoke(DISPID dispidMember, REFIID /*riid*/,
                                   LCID /*lcid*/, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                                   EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    return DispInvoke(
        this, m_ptInfo,
        dispidMember, wFlags, pdispparams,
        pvarResult, pexcepinfo, puArgErr);
}

//
// Class factory IUnknown implementation
//
HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{
    if (ppv == nullptr)
        return E_POINTER;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IClassFactory))
    {
        *ppv = static_cast<IClassFactory*>(this);
    }
    else
    {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
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
HRESULT __stdcall CFactory::CreateInstance(IUnknown*  pUnknownOuter,
                                           const IID& iid,
                                           void**     ppv)
{
    if (ppv == nullptr)
        return E_POINTER;

    // Cannot aggregate.
    if (pUnknownOuter != nullptr)
    {
        return CLASS_E_NOAGGREGATION;
    }

    // Create component.
    ATL::CComPtr<SubWCRev> pA;
    pA.Attach(new (std::nothrow) SubWCRev()); // refcount set to 1 in constructor
    if (pA == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    return pA->QueryInterface(iid, ppv);
}

// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
    ::LockServer(bLock != 0);
    return S_OK;
}

// Refcount set to 1 in constructor.
CFactory gClassFactory;

static void RunOutprocServer()
{
    // register ourself as a class object against the internal COM table
    DWORD   nToken = 0;
    HRESULT hr     = ::CoRegisterClassObject(CLSID_SubWCRev,
                                         &gClassFactory,
                                         CLSCTX_SERVER,
                                         REGCLS_MULTIPLEUSE,
                                         &nToken);

    // If the class factory is not registered no object will ever be instantiated,
    // hence no object will ever be released, hence WM_QUIT never appears in the
    // message queue and the message loop below will run forever.
    if (FAILED(hr))
        return;

    //
    // (loop ends if WM_QUIT message is received)
    //
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
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
    const HMODULE hModule = ::GetModuleHandle(nullptr);

    HRESULT hr = RegisterServer(hModule,
                                CLSID_SubWCRev,
                                L"SubWCRev Server Object",
                                L"SubWCRev.object",
                                L"SubWCRev.object.1",
                                LIBID_LibSubWCRev);
    if (SUCCEEDED(hr))
    {
        RegisterTypeLib(hModule, nullptr);
    }
    return hr;
}

//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    const HMODULE hModule = ::GetModuleHandle(nullptr);

    HRESULT hr = UnregisterServer(CLSID_SubWCRev,
                                  L"SubWCRev.object",
                                  L"SubWCRev.object.1",
                                  LIBID_LibSubWCRev);
    if (SUCCEEDED(hr))
    {
        UnRegisterTypeLib(hModule, nullptr);
    }
    return hr;
}
