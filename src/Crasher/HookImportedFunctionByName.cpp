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
        HookImportedFunctionsByName ( HMODULE         hModule     ,
                                      LPCSTR          szImportMod ,
                                      UINT            uiCount     ,
                                      LPHOOKFUNCDESCA paHookArray ,
                                      PROC *          paOrigFuncs ,
                                      LPDWORD         pdwHooked    )
{
    // Assert the parameters.
    ASSERT ( FALSE == IsBadReadPtr ( hModule                     ,
                                     sizeof ( IMAGE_DOS_HEADER )  ) ) ;
    ASSERT ( FALSE == IsBadStringPtr ( szImportMod , MAX_PATH ) ) ;
    ASSERT ( 0 != uiCount ) ;
    ASSERT ( NULL != paHookArray ) ;
    ASSERT ( FALSE == IsBadReadPtr ( paHookArray ,
                                     sizeof (HOOKFUNCDESC) * uiCount ));

    // In debug builds, perform deep validation of paHookArray.
#ifdef _DEBUG
    if ( NULL != paOrigFuncs )
    {
        ASSERT ( FALSE == IsBadWritePtr ( paOrigFuncs ,
                                          sizeof ( PROC ) * uiCount ) );

    }
    if ( NULL != pdwHooked )
    {
        ASSERT ( FALSE == IsBadWritePtr ( pdwHooked , sizeof ( UINT )));
    }

    // Check each item in the hook array.
    {
        for ( UINT i = 0 ; i < uiCount ; i++ )
        {
            ASSERT ( NULL != paHookArray[ i ].szFunc  ) ;
            ASSERT ( '\0' != *paHookArray[ i ].szFunc ) ;
            // If the function address isn't NULL, it is validated.
            if ( NULL != paHookArray[ i ].pProc )
            {
                ASSERT ( FALSE == IsBadCodePtr ( paHookArray[i].pProc));
            }
        }
    }
#endif

    // Perform the error checking for the parameters.
    if ( ( 0    == uiCount      )                                 ||
         ( NULL == szImportMod  )                                 ||
         ( TRUE == IsBadReadPtr ( paHookArray ,
                                  sizeof ( HOOKFUNCDESC ) * uiCount ) ))
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }
    if ( ( NULL != paOrigFuncs )                                &&
         ( TRUE == IsBadWritePtr ( paOrigFuncs ,
                                   sizeof ( PROC ) * uiCount ) )  )
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }
    if ( ( NULL != pdwHooked )                                    &&
         ( TRUE == IsBadWritePtr ( pdwHooked , sizeof ( UINT ) ) )  )
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( FALSE ) ;
    }

    // Is this a system DLL above the 2-GB line, which Windows 98 won't
    // let you patch?
    if ( ( FALSE == IsNT ( ) ) && ( (DWORD)hModule >= 0x80000000 ) )
    {
        SetLastErrorEx ( ERROR_INVALID_HANDLE , SLE_ERROR ) ;
        return ( FALSE ) ;
    }


    // TODO TODO
    // Should each item in the hook array be checked in release builds?

    if ( NULL != paOrigFuncs )
    {
        // Set all the values in paOrigFuncs to NULL.
        FillMemory ( paOrigFuncs , sizeof ( PROC ) * uiCount, NULL ) ;
    }
    if ( NULL != pdwHooked )
    {
        // Set the number of functions hooked to 0.
        *pdwHooked = 0 ;
    }

    // Get the specific import descriptor.
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc =
                     GetNamedImportDescriptor ( hModule , szImportMod );
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
    // Get the array the pImportDesc->FirstThunk points to because
    // I'll do the actual bashing and hooking there.
    PIMAGE_THUNK_DATA pRealThunk = MakePtr ( PIMAGE_THUNK_DATA       ,
                                             hModule                 ,
                                             pImportDesc->FirstThunk  );

    // Loop through and find the functions to hook.
    while ( NULL != pOrigThunk->u1.Function )
    {
        // Look only at functions that are imported by name, not those
        // that are imported by ordinal value.
        if (  IMAGE_ORDINAL_FLAG !=
                        ( pOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ))
        {
            // Look at the name of this imported function.
            PIMAGE_IMPORT_BY_NAME pByName ;

            pByName = MakePtr ( PIMAGE_IMPORT_BY_NAME    ,
                                hModule                  ,
                                pOrigThunk->u1.AddressOfData  ) ;

            // If the name starts with NULL, skip it.
            if ( '\0' == pByName->Name[ 0 ] )
            {
                continue ;
            }

            // Determines whether I hook the function
            BOOL bDoHook = FALSE ;

            // TODO TODO
            // Might want to consider bsearch here.

            // See whether the imported function name is in the hook
            // array. Consider requiring paHookArray to be sorted by
            // function name so that bsearch can be used, which
            // will make the lookup faster. However, the size of
            // uiCount coming into this function should be rather
            // small, so it's OK to search the entire paHookArray for
            // each function imported by szImportMod.
            for ( UINT i = 0 ; i < uiCount ; i++ )
            {
                if ( ( paHookArray[i].szFunc[0] ==
                                                pByName->Name[0] ) &&
                     ( 0 == strcmpi ( paHookArray[i].szFunc ,
                                      (char*)pByName->Name   )   )    )
                {
                    // If the function address is NULL, exit now;
                    // otherwise, go ahead and hook the function.
                    if ( NULL != paHookArray[ i ].pProc )
                    {
                        bDoHook = TRUE ;
                    }
                    break ;
                }
            }

            if ( TRUE == bDoHook )
            {
                // I found a function to hook. Now I need to change
                // the memory protection to writable before I overwrite
                // the function pointer. Note that I'm now writing into
                // the real thunk area!

                MEMORY_BASIC_INFORMATION mbi_thunk ;

                VirtualQuery ( pRealThunk                          ,
                               &mbi_thunk                          ,
                               sizeof ( MEMORY_BASIC_INFORMATION ) ) ;

                if ( FALSE == VirtualProtect ( mbi_thunk.BaseAddress ,
                                               mbi_thunk.RegionSize  ,
                                               PAGE_READWRITE        ,
                                               &mbi_thunk.Protect     ))
                {
                    ASSERT ( !"VirtualProtect failed!" ) ;
                    SetLastErrorEx ( ERROR_INVALID_HANDLE , SLE_ERROR );
                    return ( FALSE ) ;
                }

                // Save the original address if requested.
                if ( NULL != paOrigFuncs )
                {
                    paOrigFuncs[i] = (PROC)pRealThunk->u1.Function ;
                }

                // Microsoft has two different definitions of the
                // PIMAGE_THUNK_DATA fields as they are moving to
                // support Win64. The W2K RC2 Platform SDK is the
                // latest header, so I'll use that one and force the
                // Visual C++ 6 Service Pack 3 headers to deal with it.

                // Hook the function.
                DWORD * pTemp = (DWORD*)&pRealThunk->u1.Function ;
                *pTemp = (DWORD)(paHookArray[i].pProc);

                DWORD dwOldProtect ;

                // Change the protection back to what it was before I
                // overwrote the function pointer.
                VERIFY ( VirtualProtect ( mbi_thunk.BaseAddress ,
                                          mbi_thunk.RegionSize  ,
                                          mbi_thunk.Protect     ,
                                          &dwOldProtect          ) ) ;

                if ( NULL != pdwHooked )
                {
                    // Increment the total number of functions hooked.
                    *pdwHooked += 1 ;
                }
            }
        }
        // Increment both tables.
        pOrigThunk++ ;
        pRealThunk++ ;
    }

    // All OK, JumpMaster!
    SetLastError ( ERROR_SUCCESS ) ;
    return ( TRUE ) ;
}

/*----------------------------------------------------------------------
FUNCTION        :   GetNamedImportDescriptor
DISCUSSION      :
    Gets the import descriptor for the requested module.  If the module
is not imported in hModule, NULL is returned.
    This is a potential useful function in the future.
PARAMETERS      :
    hModule      - The module to hook in.
    szImportMod  - The module name to get the import descriptor for.
RETURNS         :
    NULL  - The module was not imported or hModule is invalid.
    !NULL - The import descriptor.
----------------------------------------------------------------------*/
PIMAGE_IMPORT_DESCRIPTOR
                     GetNamedImportDescriptor ( HMODULE hModule     ,
                                                LPCSTR  szImportMod  )
{
    // Always check parameters.
    ASSERT ( NULL != szImportMod ) ;
    ASSERT ( NULL != hModule     ) ;
    if ( ( NULL == szImportMod ) || ( NULL == hModule ) )
    {
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( NULL ) ;
    }

    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule ;

    // Is this the MZ header?
    if ( ( TRUE == IsBadReadPtr ( pDOSHeader                  ,
                                 sizeof ( IMAGE_DOS_HEADER )  ) ) ||
         ( IMAGE_DOS_SIGNATURE != pDOSHeader->e_magic           )   )
    {
        ASSERT ( !"Unable to read MZ header!" ) ;
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( NULL ) ;
    }

    // Get the PE header.
    PIMAGE_NT_HEADERS pNTHeader = MakePtr ( PIMAGE_NT_HEADERS       ,
                                            pDOSHeader              ,
                                            pDOSHeader->e_lfanew     ) ;

    // Is this a real PE image?
    if ( ( TRUE == IsBadReadPtr ( pNTHeader ,
                                  sizeof ( IMAGE_NT_HEADERS ) ) ) ||
         ( IMAGE_NT_SIGNATURE != pNTHeader->Signature           )   )
    {
        ASSERT ( !"Not able to read PE header" ) ;
        SetLastErrorEx ( ERROR_INVALID_PARAMETER , SLE_ERROR ) ;
        return ( NULL ) ;
    }

    // If there is no imports section, leave now.
    if ( 0 == pNTHeader->OptionalHeader.
                         DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].
                                                      VirtualAddress   )
    {
        return ( NULL ) ;
    }

    // Get the pointer to the imports section.
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc
     = MakePtr ( PIMAGE_IMPORT_DESCRIPTOR ,
                 pDOSHeader               ,
                 pNTHeader->OptionalHeader.
                         DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].
                                                      VirtualAddress ) ;

    // Loop through the import module descriptors looking for the
    // module whose name matches szImportMod.
    while ( NULL != pImportDesc->Name )
    {
        PSTR szCurrMod = MakePtr ( PSTR              ,
                                   pDOSHeader        ,
                                   pImportDesc->Name  ) ;
        if ( 0 == stricmp ( szCurrMod , szImportMod ) )
        {
            // Found it.
            break ;
        }
        // Look at the next one.
        pImportDesc++ ;
    }

    // If the name is NULL, then the module is not imported.
    if ( NULL == pImportDesc->Name )
    {
        return ( NULL ) ;
    }

    // All OK, Jumpmaster!
    return ( pImportDesc ) ;

}

