/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

/*//////////////////////////////////////////////////////////////////////
                                Includes
//////////////////////////////////////////////////////////////////////*/
#include "stdafx.h"
#include "BugslayerUtil.h"
// The project internal header file.
#include "Internal.h"

/*//////////////////////////////////////////////////////////////////////
                         File Specific Defines
//////////////////////////////////////////////////////////////////////*/

/*//////////////////////////////////////////////////////////////////////
                        File Specific Prototypes
//////////////////////////////////////////////////////////////////////*/

/*//////////////////////////////////////////////////////////////////////
                             Implementation
//////////////////////////////////////////////////////////////////////*/

BOOL  __stdcall
                        HookOrdinalExport ( HMODULE hModule     ,
                                            LPCTSTR szImportMod ,
                                            DWORD   dwOrdinal   ,
                                            PROC    pHookFunc   ,
                                            PROC *  ppOrigAddr   )
{
    // Assert the parameters.
    ASSERT ( NULL != hModule ) ;
    ASSERT ( FALSE == IsBadStringPtr ( szImportMod , MAX_PATH ) ) ;
    ASSERT ( 0 != dwOrdinal ) ;
    ASSERT ( FALSE == IsBadCodePtr ( pHookFunc ) ) ;

    // Perform the error checking for the parameters.
    if ( ( NULL  == hModule                                   ) ||
         ( TRUE  == IsBadStringPtr ( szImportMod , MAX_PATH ) ) ||
         ( 0     == dwOrdinal                                 ) ||
         ( TRUE  == IsBadCodePtr ( pHookFunc )                )   )
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }

    if ( NULL != ppOrigAddr )
    {
        ASSERT ( FALSE ==
                     IsBadWritePtr ( ppOrigAddr , sizeof ( PROC ) ) ) ;
        if ( TRUE == IsBadWritePtr ( ppOrigAddr , sizeof ( PROC ) ) )
        {
            SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
            return ( FALSE ) ;
        }
    }

    // Get the specific import descriptor.
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc =
                    GetNamedImportDescriptor ( hModule , szImportMod ) ;

    if ( NULL == pImportDesc )
    {
        // The requested module wasn't imported. Don't return an error.
        return ( TRUE ) ;
    }

    // Get the original thunk information for this DLL. I can't use
    // the thunk information stored in pImportDesc->FirstThunk
    // because the loader has already changed that array to fix up
    // all the imports. The original thunk gives me access to the
    // function names.
    PIMAGE_THUNK_DATA pOrigThunk =
                        MakePtr ( PIMAGE_THUNK_DATA       ,
                                  hModule                 ,
                                  pImportDesc->OriginalFirstThunk  ) ;
    // Get the array that pImportDesc->FirstThunk points to because I'll
    // do the actual hooking there.
    PIMAGE_THUNK_DATA pRealThunk = MakePtr ( PIMAGE_THUNK_DATA       ,
                                             hModule                 ,
                                             pImportDesc->FirstThunk  );

    // The flag is going to be set from the thunk, so make it
    // easier to look up.
    DWORD dwCompareOrdinal = IMAGE_ORDINAL_FLAG | dwOrdinal ;

    // Loop through and find the function to hook.
    while ( NULL != pOrigThunk->u1.Function )
    {
        // Look only at functions that are imported by ordinal value,
        // not those that are imported by name.
        if (  IMAGE_ORDINAL_FLAG ==
                        ( pOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ))
        {
            // Did I find the function to hook?
            if ( dwCompareOrdinal == pOrigThunk->u1.Ordinal )
            {
                // I found the function to hook. Now I need to change
                // the memory protection to writable before I overwrite
                // the function pointer. Note that I'm now writing into
                // the real thunk area!
                MEMORY_BASIC_INFORMATION mbi_thunk ;

                VirtualQuery ( pRealThunk                          ,
                               &mbi_thunk                          ,
                               sizeof ( MEMORY_BASIC_INFORMATION )  ) ;

                if ( FALSE == VirtualProtect ( mbi_thunk.BaseAddress ,
                                               mbi_thunk.RegionSize  ,
                                               PAGE_READWRITE        ,
                                               &mbi_thunk.Protect     ))
                {
                    ASSERT ( !"VirtualProtect failed!" ) ;
                    // There's nothing I can do but fail the function.
                    SetLastErrorEx ( ERROR_INVALID_PARAMETER ,
                                     SLE_ERROR                ) ;
                    return ( FALSE ) ;
                }

                // Save the original address if requested.
                if ( NULL != ppOrigAddr )
                {
                    *ppOrigAddr = (PROC)pRealThunk->u1.Function ;
                }

                // Microsoft has two different definitions of the
                // PIMAGE_THUNK_DATA fields as they are moving to
                // support Win64. The W2K RC2 Platform SDK is the
                // latest header, so I'll use that one and force the
                // Visual C++ 6 Service Pack 3 headers to deal with it.

                // Hook the function.
                DWORD * pTemp = (DWORD*)&pRealThunk->u1.Function ;
                *pTemp = (DWORD)(pHookFunc) ;

                DWORD dwOldProtect ;

                // Change the protection back to what it was before I
                // overwrote the function pointer.
                VERIFY ( VirtualProtect ( mbi_thunk.BaseAddress ,
                                          mbi_thunk.RegionSize  ,
                                          mbi_thunk.Protect     ,
                                          &dwOldProtect          ) ) ;
                // Life is good.
                SetLastError ( ERROR_SUCCESS ) ;
                return ( TRUE ) ;
            }
        }
        // Increment both tables.
        pOrigThunk++ ;
        pRealThunk++ ;
    }

    // Nothing was hooked. Technically, this isn't an error. It just
    // means that the module is imported but the function isn't.
    SetLastError ( ERROR_SUCCESS ) ;
    return ( FALSE ) ;
}

