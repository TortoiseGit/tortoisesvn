#pragma once
#include "resource.h"       // main symbols
#include "ExampleAtlPlugin_i.h"
#include "..\inc\IBugTraqProvider_h.h"

class ATL_NO_VTABLE CProvider :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CProvider, &CLSID_Provider>,
	public IBugTraqProvider
{
public:
	CProvider();

DECLARE_REGISTRY_RESOURCEID(IDR_PROVIDER)

DECLARE_NOT_AGGREGATABLE(CProvider)

BEGIN_COM_MAP(CProvider)
	COM_INTERFACE_ENTRY(IBugTraqProvider)
END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();
	void FinalRelease();

// IBugTraqProvider
public:
    virtual HRESULT STDMETHODCALLTYPE ValidateParameters( 
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [retval][out] */ VARIANT_BOOL *valid);
    
    virtual HRESULT STDMETHODCALLTYPE GetLinkText( 
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [retval][out] */ BSTR *linkText);
    
    virtual HRESULT STDMETHODCALLTYPE GetCommitMessage( 
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [in] */ BSTR commonRoot,
        /* [in] */ SAFEARRAY * pathList,
        /* [in] */ BSTR originalMessage,
        /* [retval][out] */ BSTR *newMessage);

private:
	typedef std::map< CString, CString > parameters_t;
	parameters_t ParseParameters(BSTR parameters) const;
};

OBJECT_ENTRY_AUTO(__uuidof(Provider), CProvider)
