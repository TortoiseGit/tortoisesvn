/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx.h"
#include "BugslayerUtil.h"

// The project internal header file.
#include "Internal.h"

HMODULE * /*BUGSUTIL_DLLINTERFACE*/ __stdcall
                      AllocAndFillProcessModuleList ( HANDLE hHeap    ,
                                                      LPDWORD pdwCount )

{
    ASSERT ( FALSE == IsBadWritePtr ( pdwCount , sizeof ( LPDWORD ) ) ) ;
    if ( TRUE == IsBadWritePtr ( pdwCount , sizeof ( LPDWORD ) ) )
    {
        SetLastError ( ERROR_INVALID_PARAMETER ) ;
        return ( NULL ) ;
    }

    ASSERT ( NULL != hHeap ) ;
    if ( NULL == hHeap )
    {
        SetLastError ( ERROR_INVALID_PARAMETER ) ;
        return ( NULL ) ;
    }

    // First, ask how many modules are really loaded.
    DWORD dwQueryCount ;

    BOOL bRet = GetLoadedModules ( GetCurrentProcessId ( ) ,
                                   0                       ,
                                   NULL                    ,
                                   &dwQueryCount            ) ;
    ASSERT ( TRUE == bRet ) ;
    ASSERT ( 0 != dwQueryCount ) ;

    if ( ( FALSE == bRet ) || ( 0 == dwQueryCount ) )
    {
        return ( NULL ) ;
    }

    // The HMODULE array.
    HMODULE * pModArray ;

    // Allocate the buffer to hold the returned array.
    pModArray = (HMODULE*)HeapAlloc ( hHeap             ,
                                      HEAP_ZERO_MEMORY  ,
                                      dwQueryCount *
                                                   sizeof ( HMODULE ) );

    ASSERT ( NULL != pModArray ) ;
    if ( NULL == pModArray )
    {
        return ( NULL ) ;
    }

    // bRet holds BOOLEAN return values.
    bRet = GetLoadedModules ( GetCurrentProcessId ( ) ,
                              dwQueryCount            ,
                              pModArray               ,
                              pdwCount                 ) ;
    // Save off the last error so that the assert can still fire and
    //  not change the value.
    DWORD dwLastError = GetLastError ( ) ;
    ASSERT ( TRUE == bRet ) ;

    if ( FALSE == bRet )
    {
        // Get rid of the last buffer.
        HeapFree ( hHeap, 0, pModArray ) ;
        pModArray = NULL ;
        SetLastError ( dwLastError ) ;
    }
    else
    {
        SetLastError ( ERROR_SUCCESS ) ;
    }
    // All OK, Jumpmaster!
    return ( pModArray ) ;
}

