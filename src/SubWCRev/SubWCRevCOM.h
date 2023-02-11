// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011, 2013, 2021, 2023 - TortoiseSVN

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
    virtual ~SubWCRev();

    // IUnknown implementation
    //
    HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) override;
    ULONG __stdcall AddRef() override;
    ULONG __stdcall Release() override;

    //IDispatch implementation
    HRESULT __stdcall GetTypeInfoCount(UINT* pctinfo) override;
    HRESULT __stdcall GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) override;
    HRESULT __stdcall GetIDsOfNames(REFIID    riid,
                                    LPOLESTR* rgszNames, UINT cNames,
                                    LCID lcid, DISPID* rgdispid) override;
    HRESULT __stdcall Invoke(DISPID dispidMember, REFIID riid,
                             LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                             EXCEPINFO* pexcepinfo, UINT* puArgErr) override;

    // ISubWCRev implementation
    //
    HRESULT __stdcall GetWCInfo(/*[in]*/ BSTR wcPath, /*[in]*/ VARIANT_BOOL folders, /*[in]*/ VARIANT_BOOL externals) override;
    HRESULT __stdcall GetWCInfo2(/*[in]*/ BSTR wcPath, /*[in]*/ VARIANT_BOOL folders, /*[in]*/ VARIANT_BOOL externals, /*[in]*/ VARIANT_BOOL externalsNoMixed) override;

    HRESULT __stdcall get_Revision(/*[out, retval]*/ VARIANT* rev) override;

    HRESULT __stdcall get_MinRev(/*[out, retval]*/ VARIANT* rev) override;

    HRESULT __stdcall get_MaxRev(/*[out, retval]*/ VARIANT* rev) override;

    HRESULT __stdcall get_Date(/*[out, retval]*/ VARIANT* date) override;

    HRESULT __stdcall get_Url(/*[out, retval]*/ VARIANT* url) override;

    HRESULT __stdcall get_Author(/*[out, retval]*/ VARIANT* author) override;

    HRESULT __stdcall get_HasModifications(/*[out, retval]*/ VARIANT_BOOL* modifications) override;

    HRESULT __stdcall get_HasUnversioned(/*[out, retval]*/ VARIANT_BOOL* modifications) override;

    HRESULT __stdcall get_HasMixedRevisions(/*[out, retval]*/ VARIANT_BOOL* modifications) override;

    HRESULT __stdcall get_HaveExternalsAllFixedRevision(/*[out, retval]*/ VARIANT_BOOL* modifications) override;

    HRESULT __stdcall get_IsWcTagged(/*[out, retval]*/ VARIANT_BOOL* modifications) override;

    HRESULT __stdcall get_IsSvnItem(/*[out, retval]*/ VARIANT_BOOL* svnItem) override;

    HRESULT __stdcall get_NeedsLocking(/*[out, retval]*/ VARIANT_BOOL* needsLocking) override;

    HRESULT __stdcall get_IsLocked(/*[out, retval]*/ VARIANT_BOOL* locked) override;

    HRESULT __stdcall get_LockCreationDate(/*[out, retval]*/ VARIANT* date) override;

    HRESULT __stdcall get_LockOwner(/*[out, retval]*/ VARIANT* owner) override;

    HRESULT __stdcall get_LockComment(/*[out, retval]*/ VARIANT* comment) override;
    
    HRESULT __stdcall get_RepoRoot(/*[out, retval]*/ VARIANT* url) override;

private:
    BOOL CopyDateToString(WCHAR* destbuf, int buflen, apr_time_t time) const;
    BOOL IsLockDataAvailable() const;

    static HRESULT LoadTypeInfo(ITypeInfo** pptinfo, const CLSID& libid, const CLSID& iid, LCID lcid);
    static HRESULT BoolToVariantBool(BOOL value, VARIANT_BOOL* result);
    static HRESULT LongToVariant(LONG value, VARIANT* result);
    static HRESULT Utf8StringToVariant(const char* string, VARIANT* result);
    HRESULT __stdcall GetWCInfoInternal(/*[in]*/ BSTR wcPath, /*[in]*/ VARIANT_BOOL folders, /*[in]*/ VARIANT_BOOL externals);

    // Reference count
    long       m_cRef;
    LPTYPEINFO m_ptInfo; // pointer to type-library
    SubWCRevT  m_subStat;
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
    HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) override;
    ULONG __stdcall AddRef() override;
    ULONG __stdcall Release() override;

    // Interface IClassFactory
    HRESULT __stdcall CreateInstance(IUnknown*  pUnknownOuter,
                                     const IID& iid,
                                     void**     ppv) override;
    HRESULT __stdcall LockServer(BOOL bLock) override;

    CFactory() {}
    virtual ~CFactory() { ; }
};
