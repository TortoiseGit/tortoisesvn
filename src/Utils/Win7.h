// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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

// helper:
// declares and defines stuff which is not available in the Vista SDK or
// which isn't available in the Win7 SDK but not unless NTDDI_VERSION is 
// set to NTDDI_WIN7
#pragma once
#if (NTDDI_VERSION < 0x06010000)



/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* Compiler settings for objectarray.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
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

#ifndef __objectarray_h__
#define __objectarray_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IObjectArray_FWD_DEFINED__
#define __IObjectArray_FWD_DEFINED__
typedef interface IObjectArray IObjectArray;
#endif 	/* __IObjectArray_FWD_DEFINED__ */


#ifndef __IObjectCollection_FWD_DEFINED__
#define __IObjectCollection_FWD_DEFINED__
typedef interface IObjectCollection IObjectCollection;
#endif 	/* __IObjectCollection_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IObjectArray_INTERFACE_DEFINED__
#define __IObjectArray_INTERFACE_DEFINED__

/* interface IObjectArray */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IObjectArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("92CA9DCD-5622-4bba-A805-5E9F541BD8C9")
    IObjectArray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ __RPC__out UINT *pcObjects) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAt( 
            /* [in] */ UINT uiIndex,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IObjectArray * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IObjectArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IObjectArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            __RPC__in IObjectArray * This,
            /* [out] */ __RPC__out UINT *pcObjects);
        
        HRESULT ( STDMETHODCALLTYPE *GetAt )( 
            __RPC__in IObjectArray * This,
            /* [in] */ UINT uiIndex,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);
        
        END_INTERFACE
    } IObjectArrayVtbl;

    interface IObjectArray
    {
        CONST_VTBL struct IObjectArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectArray_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IObjectArray_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IObjectArray_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IObjectArray_GetCount(This,pcObjects)	\
    ( (This)->lpVtbl -> GetCount(This,pcObjects) ) 

#define IObjectArray_GetAt(This,uiIndex,riid,ppv)	\
    ( (This)->lpVtbl -> GetAt(This,uiIndex,riid,ppv) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IObjectArray_INTERFACE_DEFINED__ */


#ifndef __IObjectCollection_INTERFACE_DEFINED__
#define __IObjectCollection_INTERFACE_DEFINED__

/* interface IObjectCollection */
/* [unique][object][uuid] */ 


EXTERN_C const IID IID_IObjectCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5632b1a4-e38a-400a-928a-d4cd63230295")
    IObjectCollection : public IObjectArray
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddObject( 
            /* [in] */ __RPC__in_opt IUnknown *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddFromArray( 
            /* [in] */ __RPC__in_opt IObjectArray *poaSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveObjectAt( 
            /* [in] */ UINT uiIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IObjectCollection * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IObjectCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IObjectCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            __RPC__in IObjectCollection * This,
            /* [out] */ __RPC__out UINT *pcObjects);
        
        HRESULT ( STDMETHODCALLTYPE *GetAt )( 
            __RPC__in IObjectCollection * This,
            /* [in] */ UINT uiIndex,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *AddObject )( 
            __RPC__in IObjectCollection * This,
            /* [in] */ __RPC__in_opt IUnknown *punk);
        
        HRESULT ( STDMETHODCALLTYPE *AddFromArray )( 
            __RPC__in IObjectCollection * This,
            /* [in] */ __RPC__in_opt IObjectArray *poaSource);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveObjectAt )( 
            __RPC__in IObjectCollection * This,
            /* [in] */ UINT uiIndex);
        
        HRESULT ( STDMETHODCALLTYPE *Clear )( 
            __RPC__in IObjectCollection * This);
        
        END_INTERFACE
    } IObjectCollectionVtbl;

    interface IObjectCollection
    {
        CONST_VTBL struct IObjectCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectCollection_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IObjectCollection_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IObjectCollection_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IObjectCollection_GetCount(This,pcObjects)	\
    ( (This)->lpVtbl -> GetCount(This,pcObjects) ) 

#define IObjectCollection_GetAt(This,uiIndex,riid,ppv)	\
    ( (This)->lpVtbl -> GetAt(This,uiIndex,riid,ppv) ) 


#define IObjectCollection_AddObject(This,punk)	\
    ( (This)->lpVtbl -> AddObject(This,punk) ) 

#define IObjectCollection_AddFromArray(This,poaSource)	\
    ( (This)->lpVtbl -> AddFromArray(This,poaSource) ) 

#define IObjectCollection_RemoveObjectAt(This,uiIndex)	\
    ( (This)->lpVtbl -> RemoveObjectAt(This,uiIndex) ) 

#define IObjectCollection_Clear(This)	\
    ( (This)->lpVtbl -> Clear(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IObjectCollection_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif






extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0188_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0188_v0_0_s_ifspec;

#ifndef __ICustomDestinationList_INTERFACE_DEFINED__
#define __ICustomDestinationList_INTERFACE_DEFINED__

/* interface ICustomDestinationList */
/* [unique][object][uuid] */ 

typedef /* [v1_enum] */ 
enum KNOWNDESTCATEGORY
    {	KDC_FREQUENT	= 1,
	KDC_RECENT	= ( KDC_FREQUENT + 1 ) 
    } 	KNOWNDESTCATEGORY;


EXTERN_C const IID IID_ICustomDestinationList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6332debf-87b5-4670-90c0-5e57b408a49e")
    ICustomDestinationList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAppID( 
            /* [string][in] */ __RPC__in_string LPCWSTR pszAppID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginList( 
            /* [out] */ __RPC__out UINT *pcMinSlots,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppendCategory( 
            /* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
            /* [in] */ __RPC__in_opt IObjectArray *poa) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppendKnownCategory( 
            /* [in] */ KNOWNDESTCATEGORY category) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddUserTasks( 
            /* [in] */ __RPC__in_opt IObjectArray *poa) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommitList( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRemovedDestinations( 
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteList( 
            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AbortList( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICustomDestinationListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ICustomDestinationList * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ICustomDestinationList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ICustomDestinationList * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAppID )( 
            __RPC__in ICustomDestinationList * This,
            /* [string][in] */ __RPC__in_string LPCWSTR pszAppID);
        
        HRESULT ( STDMETHODCALLTYPE *BeginList )( 
            __RPC__in ICustomDestinationList * This,
            /* [out] */ __RPC__out UINT *pcMinSlots,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *AppendCategory )( 
            __RPC__in ICustomDestinationList * This,
            /* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
            /* [in] */ __RPC__in_opt IObjectArray *poa);
        
        HRESULT ( STDMETHODCALLTYPE *AppendKnownCategory )( 
            __RPC__in ICustomDestinationList * This,
            /* [in] */ KNOWNDESTCATEGORY category);
        
        HRESULT ( STDMETHODCALLTYPE *AddUserTasks )( 
            __RPC__in ICustomDestinationList * This,
            /* [in] */ __RPC__in_opt IObjectArray *poa);
        
        HRESULT ( STDMETHODCALLTYPE *CommitList )( 
            __RPC__in ICustomDestinationList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRemovedDestinations )( 
            __RPC__in ICustomDestinationList * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteList )( 
            __RPC__in ICustomDestinationList * This,
            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID);
        
        HRESULT ( STDMETHODCALLTYPE *AbortList )( 
            __RPC__in ICustomDestinationList * This);
        
        END_INTERFACE
    } ICustomDestinationListVtbl;

    interface ICustomDestinationList
    {
        CONST_VTBL struct ICustomDestinationListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICustomDestinationList_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICustomDestinationList_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICustomDestinationList_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICustomDestinationList_SetAppID(This,pszAppID)	\
    ( (This)->lpVtbl -> SetAppID(This,pszAppID) ) 

#define ICustomDestinationList_BeginList(This,pcMinSlots,riid,ppv)	\
    ( (This)->lpVtbl -> BeginList(This,pcMinSlots,riid,ppv) ) 

#define ICustomDestinationList_AppendCategory(This,pszCategory,poa)	\
    ( (This)->lpVtbl -> AppendCategory(This,pszCategory,poa) ) 

#define ICustomDestinationList_AppendKnownCategory(This,category)	\
    ( (This)->lpVtbl -> AppendKnownCategory(This,category) ) 

#define ICustomDestinationList_AddUserTasks(This,poa)	\
    ( (This)->lpVtbl -> AddUserTasks(This,poa) ) 

#define ICustomDestinationList_CommitList(This)	\
    ( (This)->lpVtbl -> CommitList(This) ) 

#define ICustomDestinationList_GetRemovedDestinations(This,riid,ppv)	\
    ( (This)->lpVtbl -> GetRemovedDestinations(This,riid,ppv) ) 

#define ICustomDestinationList_DeleteList(This,pszAppID)	\
    ( (This)->lpVtbl -> DeleteList(This,pszAppID) ) 

#define ICustomDestinationList_AbortList(This)	\
    ( (This)->lpVtbl -> AbortList(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICustomDestinationList_INTERFACE_DEFINED__ */


#ifndef __IApplicationDestinations_INTERFACE_DEFINED__
#define __IApplicationDestinations_INTERFACE_DEFINED__

/* interface IApplicationDestinations */
/* [unique][object][uuid] */ 


EXTERN_C const IID IID_IApplicationDestinations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("12337d35-94c6-48a0-bce7-6a9c69d4d600")
    IApplicationDestinations : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAppID( 
            /* [in] */ __RPC__in LPCWSTR pszAppID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveDestination( 
            /* [in] */ __RPC__in_opt IUnknown *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllDestinations( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApplicationDestinationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IApplicationDestinations * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IApplicationDestinations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IApplicationDestinations * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAppID )( 
            __RPC__in IApplicationDestinations * This,
            /* [in] */ __RPC__in LPCWSTR pszAppID);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveDestination )( 
            __RPC__in IApplicationDestinations * This,
            /* [in] */ __RPC__in_opt IUnknown *punk);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveAllDestinations )( 
            __RPC__in IApplicationDestinations * This);
        
        END_INTERFACE
    } IApplicationDestinationsVtbl;

    interface IApplicationDestinations
    {
        CONST_VTBL struct IApplicationDestinationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApplicationDestinations_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IApplicationDestinations_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IApplicationDestinations_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IApplicationDestinations_SetAppID(This,pszAppID)	\
    ( (This)->lpVtbl -> SetAppID(This,pszAppID) ) 

#define IApplicationDestinations_RemoveDestination(This,punk)	\
    ( (This)->lpVtbl -> RemoveDestination(This,punk) ) 

#define IApplicationDestinations_RemoveAllDestinations(This)	\
    ( (This)->lpVtbl -> RemoveAllDestinations(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */

#endif 	/* __IApplicationDestinations_INTERFACE_DEFINED__ */

EXTERN_C const CLSID CLSID_DestinationList;

#ifdef __cplusplus

class DECLSPEC_UUID("77f10cf0-3db5-4966-b520-b7c54fd35ed6")
DestinationList;
#endif

EXTERN_C const CLSID CLSID_ApplicationDestinations;

#ifdef __cplusplus

class DECLSPEC_UUID("86c14003-4d6b-4ef3-a7b4-0506663b2e68")
ApplicationDestinations;
#endif

EXTERN_C const CLSID CLSID_EnumerableObjectCollection;

#ifdef __cplusplus

class DECLSPEC_UUID("2d3468c1-36a7-43b6-ac24-d3f02fd9607a")
EnumerableObjectCollection;
#endif

DEFINE_PROPERTYKEY(PKEY_Title, 0xF29F85E0, 0x4FF9, 0x1068, 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9, 2);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_IsDestListSeparator, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 6);



#endif /* (NTDDI_VERSION < NTDDI_WIN7) */
