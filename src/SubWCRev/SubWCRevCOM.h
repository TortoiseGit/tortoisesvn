// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011, 2013 - TortoiseSVN

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
#pragma once

#include <initguid.h>
#include "SubWCRev.h"

/**
 * \ingroup SubWCRev
 * Implements the ISubWCRev interface of the COM object that SubWCRevCOM publishes.
 */
class SubWCRev : public ISubWCRev
{

    // Construction
public:
    SubWCRev();
    ~SubWCRev();

    // IUnknown implementation
    //
    virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) ;
    virtual ULONG __stdcall AddRef() ;
    virtual ULONG __stdcall Release() ;

    //IDispatch implementation
    virtual HRESULT __stdcall GetTypeInfoCount(UINT* pctinfo);
    virtual HRESULT __stdcall GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual HRESULT __stdcall GetIDsOfNames(REFIID riid,
        LPOLESTR* rgszNames, UINT cNames,
        LCID lcid, DISPID* rgdispid);
    virtual HRESULT __stdcall Invoke(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr);

    // ISubWCRev implementation
    //
    virtual HRESULT __stdcall GetWCInfo(/*[in]*/ BSTR   wcPath, /*[in]*/VARIANT_BOOL folders, /*[in]*/VARIANT_BOOL externals);
    virtual HRESULT __stdcall GetWCInfo2(/*[in]*/ BSTR   wcPath, /*[in]*/VARIANT_BOOL folders, /*[in]*/VARIANT_BOOL externals, /*[in]*/VARIANT_BOOL externalsNoMixed);

    virtual HRESULT __stdcall get_Revision(/*[out, retval]*/VARIANT* rev);

    virtual HRESULT __stdcall get_MinRev(/*[out, retval]*/VARIANT* rev);

    virtual HRESULT __stdcall get_MaxRev(/*[out, retval]*/VARIANT* rev);

    virtual HRESULT __stdcall get_Date(/*[out, retval]*/VARIANT* date);

    virtual HRESULT __stdcall get_Url(/*[out, retval]*/VARIANT* url);

    virtual HRESULT __stdcall get_Author(/*[out, retval]*/VARIANT* author);

    virtual HRESULT __stdcall get_HasModifications(/*[out, retval]*/VARIANT_BOOL* modifications);

    virtual HRESULT __stdcall get_HasUnversioned(/*[out, retval]*/VARIANT_BOOL* modifications);

    virtual HRESULT __stdcall get_HasMixedRevisions(/*[out, retval]*/VARIANT_BOOL* modifications);

    virtual HRESULT __stdcall get_HaveExternalsAllFixedRevision(/*[out, retval]*/VARIANT_BOOL* modifications);

    virtual HRESULT __stdcall get_IsWcTagged(/*[out, retval]*/VARIANT_BOOL* modifications);

    virtual HRESULT __stdcall get_IsSvnItem(/*[out, retval]*/VARIANT_BOOL* svn_item);

    virtual HRESULT __stdcall get_NeedsLocking(/*[out, retval]*/VARIANT_BOOL* needs_locking);

    virtual HRESULT __stdcall get_IsLocked(/*[out, retval]*/VARIANT_BOOL* locked);

    virtual HRESULT __stdcall get_LockCreationDate(/*[out, retval]*/VARIANT* date);

    virtual HRESULT __stdcall get_LockOwner(/*[out, retval]*/VARIANT* owner);

    virtual HRESULT __stdcall get_LockComment(/*[out, retval]*/VARIANT* comment);

private:
    BOOL CopyDateToString(WCHAR *destbuf, int buflen, apr_time_t time);
    BOOL IsLockDataAvailable();

    HRESULT LoadTypeInfo(ITypeInfo ** pptinfo, const CLSID& libid, const CLSID& iid, LCID lcid);
    static HRESULT BoolToVariantBool(BOOL value, VARIANT_BOOL* result);
    static HRESULT LongToVariant(LONG value, VARIANT* result);
    static HRESULT Utf8StringToVariant(const char* string, VARIANT* result );
    HRESULT __stdcall GetWCInfoInternal(/*[in]*/ BSTR   wcPath, /*[in]*/VARIANT_BOOL folders, /*[in]*/VARIANT_BOOL externals);

    // Reference count
    long        m_cRef ;
    LPTYPEINFO  m_ptinfo; // pointer to type-library

    SubWCRev_t SubStat;
};

/**
 * \ingroup SubWCRev
 * Implements the IClassFactory interface of the SubWCRev COM object.
 * Used as global object only - no true reference counting in this class.
 */
class CFactory : public IClassFactory
{
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) ;
    virtual ULONG   __stdcall AddRef() ;
    virtual ULONG   __stdcall Release() ;

    // Interface IClassFactory
    virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter,
        const IID& iid,
        void** ppv) ;
    virtual HRESULT __stdcall LockServer(BOOL bLock) ;

    CFactory() {}
    ~CFactory() {;}
} ;
