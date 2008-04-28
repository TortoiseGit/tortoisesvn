

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Mon Apr 28 14:05:17 2008
 */
/* Compiler settings for .\IBugTraqProvider.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __IBugTraqProvider_h_h__
#define __IBugTraqProvider_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IBugTraqProvider_FWD_DEFINED__
#define __IBugTraqProvider_FWD_DEFINED__
typedef interface IBugTraqProvider IBugTraqProvider;
#endif 	/* __IBugTraqProvider_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IBugTraqProvider_INTERFACE_DEFINED__
#define __IBugTraqProvider_INTERFACE_DEFINED__

/* interface IBugTraqProvider */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IBugTraqProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("298B927B-7220-423C-B7B4-6E241F00CD93")
    IBugTraqProvider : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ValidateParameters( 
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ VARIANT_BOOL *valid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLinkText( 
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ BSTR *linkText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCommitMessage( 
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR originalMessage,
            /* [retval][out] */ BSTR *newMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBugTraqProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBugTraqProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBugTraqProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBugTraqProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IBugTraqProvider * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IBugTraqProvider * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IBugTraqProvider * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IBugTraqProvider * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *ValidateParameters )( 
            IBugTraqProvider * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ VARIANT_BOOL *valid);
        
        HRESULT ( STDMETHODCALLTYPE *GetLinkText )( 
            IBugTraqProvider * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ BSTR *linkText);
        
        HRESULT ( STDMETHODCALLTYPE *GetCommitMessage )( 
            IBugTraqProvider * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR originalMessage,
            /* [retval][out] */ BSTR *newMessage);
        
        END_INTERFACE
    } IBugTraqProviderVtbl;

    interface IBugTraqProvider
    {
        CONST_VTBL struct IBugTraqProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBugTraqProvider_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBugTraqProvider_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBugTraqProvider_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBugTraqProvider_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IBugTraqProvider_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IBugTraqProvider_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IBugTraqProvider_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IBugTraqProvider_ValidateParameters(This,hParentWnd,parameters,valid)	\
    ( (This)->lpVtbl -> ValidateParameters(This,hParentWnd,parameters,valid) ) 

#define IBugTraqProvider_GetLinkText(This,hParentWnd,parameters,linkText)	\
    ( (This)->lpVtbl -> GetLinkText(This,hParentWnd,parameters,linkText) ) 

#define IBugTraqProvider_GetCommitMessage(This,hParentWnd,parameters,originalMessage,newMessage)	\
    ( (This)->lpVtbl -> GetCommitMessage(This,hParentWnd,parameters,originalMessage,newMessage) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBugTraqProvider_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


