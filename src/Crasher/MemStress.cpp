/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

// Get everything included.
#include "stdafx.h"
#include "BugslayerUtil.h"
#include "MemStressConstants.h"
#include "CRTDBG_Internals.h"

////////////////////////////////////////////////////////////////////////
// Internal Data Structures
////////////////////////////////////////////////////////////////////////
// The struct that holds a single file marked for failure and the line
// in the file allocations are supposed to fail.  If the line is -1,
// then any allocations from the file are supposed to fail.
// BIG NOTE: Notice that even in UNICODE builds, this structure only has
// character buffers.  Since __FILE__ is always defined in terms of
// chars, no matter what so everything is converted when it is read in.
typedef struct tag_FAILFILE
{
    char szFile[ MAX_PATH + 1 ] ;
    long lLine                  ;
} FAILFILE , * LPFAILFILE ;

// The struct that holds all of the options as they come out of the INI
// file.
typedef struct tag_FAILUREINFO
{
    // The CRT options that should be set for the user.
    BOOL        bCRTCheckMemory     ;
    BOOL        bCRTDelayMemFrees   ;
    BOOL        bCRTLeakChecking    ;
    // The general options for the hook.  If any of the UINT types are
    // zero, then they are not active.
    BOOL        bGENFailAllAllocs   ;
    UINT        uiGENFailEveryN     ;
    UINT        uiGENFailAfterX     ;
    UINT        uiGENFailOverY      ;
    BOOL        bGENAskOnEach       ;
    // The total file count.
    UINT        uiFileCount         ;
    // The array of files to fail.
    LPFAILFILE  pFailFiles          ;
} FAILUREINFO , * LPFAILUREINFO ;

////////////////////////////////////////////////////////////////////////
// Unfortunately, the CRT hooks do not get told if the memory being
// reallocated is being done in place or will cause a free first.
// Thus, it is impossible to know for sure exactly how much memory is
// outstanding in the system.  The statistics keeps are only for calls
// to allocation functions (new and malloc) and deallocation functions
// (delete and free).
// Memory statistics are recorded before any processing for failures
// takes place.
typedef struct tag_MEMSTATS
{
    // The total highwater mark of allocations so far.
    long lMaxAllocated      ;
    // The total number of calls to allocation functions.
    long lTotalAllocCalls   ;
    // The total number of calls to realloc functions.
    long lTotalReAllocCalls ;
    // The total number of calls to release functions.
    long lTotalReleaseCalls ;
    // The total amount of memory that is currently in use.
    long lCurrentlyInUse    ;
} MEMSTATS , * LPMEMSTATS ;

////////////////////////////////////////////////////////////////////////
// Static Variable Declarations
////////////////////////////////////////////////////////////////////////
// The critical section that everything will be protected with.
static CRITICAL_SECTION g_stCritSec ;
// The thread-local storage for the thread recursion count. This is to
// protect against re-entry when the code in this library calls
// something that would cause an allocation in the thread.  The perfect
// example is when the user wants to be asked on each allocation if they
// want the allocation to succeed.  I will have to pop up a message box
// in the middle of AllocationHook.  Since MFC does all of the window
// subclassing to paint the gray background, the internal MFC windows
// procedure will get called.  In that windows procedure, MFC does it's
// handle table mapping and allocates memory.  Thus a happy case of
// recursion in a single thread.  To get around the recursion, I keep a
// per-thread value that tells me the recursion count.  In
// AllocationHook, if the the recursion variable is 1, I just return
// TRUE and leave.
static DWORD g_tlsRecursionIndex = TLS_OUT_OF_INDEXES ;
// The one global flag that indicates that the library was properly
// initialized and ready for use.
static BOOL g_bLibIsInit = FALSE ;
// The private heap that will hold all of the allocations for this
// module.  Keep in mind that this library cannot do any normal CRT
// memory allocations because it could get into nasty recursive
// situations.
static HANDLE g_hHeap = NULL ;
// The failure information for this run of the library.
static LPFAILUREINFO g_pFailInfo = NULL ;
// The internal statistics.
static MEMSTATS g_stMemStats ;
// The previously installed hook.
static _CRT_ALLOC_HOOK pfnPrevHook ;

////////////////////////////////////////////////////////////////////////
// Internal Function Declarations
////////////////////////////////////////////////////////////////////////
// The function that actually does the INI reading.
static LPFAILUREINFO ProcessINIFileA ( LPCSTR szProgram ) ;
// The actual allocation hook.
static int AllocationHook ( int                   nAllocType ,
                            void *                pvData     ,
                            size_t                nSize      ,
                            int                   nBlockUse  ,
                            long                  lRequest   ,
                            const unsigned char * szFileName ,
                            int                   nLine       ) ;

////////////////////////////////////////////////////////////////////////
// Function Implementation Starts Here
////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
// The initialization function.
BOOL  __stdcall
    MemStressInitializeA ( const char * szProgName )
{
    // The return value.
    BOOL bRet = TRUE ;
    __try
    {
        __try
        {
            // Check all of the basic assumptions.
            ASSERT ( FALSE == g_bLibIsInit ) ;
            ASSERT ( NULL == g_hHeap ) ;
            ASSERT ( NULL == g_pFailInfo ) ;
            ASSERT ( NULL != szProgName ) ;

            // Initialize the few items that all routines in here will
            // have to share.
            InitializeCriticalSection ( &g_stCritSec ) ;
            // Now immediately, try and grab it.
            EnterCriticalSection ( &g_stCritSec ) ;

            FillMemory ( &g_stMemStats , sizeof ( MEMSTATS ), NULL ) ;

            // Create the private heap for this library.
            g_hHeap = HeapCreate ( HEAP_GENERATE_EXCEPTIONS , 0 , 0 ) ;

            // Get the failure information for the file.
            g_pFailInfo = ProcessINIFileA ( szProgName ) ;

            // Set the main CRT flags.
            int iFlags = _CrtSetDbgFlag ( _CRTDBG_REPORT_FLAG ) ;
            if ( TRUE == g_pFailInfo->bCRTCheckMemory )
            {
                iFlags |= _CRTDBG_CHECK_ALWAYS_DF ;
            }
            if ( TRUE == g_pFailInfo->bCRTDelayMemFrees )
            {
                iFlags |= _CRTDBG_DELAY_FREE_MEM_DF ;
            }
            if ( TRUE == g_pFailInfo->bCRTLeakChecking )
            {
                iFlags |= _CRTDBG_LEAK_CHECK_DF ;
            }
            _CrtSetDbgFlag ( iFlags ) ;

            // Set the allocation hook.
            pfnPrevHook =
                  _CrtSetAllocHook ( (_CRT_ALLOC_HOOK)AllocationHook ) ;

            g_bLibIsInit = TRUE ;

        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ASSERT ( !"Caught a crash in MemStressInitializeA!" ) ;
            // The library was unable to initialize.
            g_bLibIsInit = FALSE ;
            bRet = FALSE ;
        }
    }
    __finally
    {
        // Make sure that we always execute this!
        LeaveCriticalSection ( &g_stCritSec ) ;
    }
    return ( bRet ) ;
}
#else
BOOL  __stdcall
    MemStressInitializeA ( const char * )
{
    return ( TRUE ) ;
}
#endif  // _DEBUG

#ifndef _DEBUG
BOOL  __stdcall
    MemStressInitializeW ( const wchar_t * szProgName )
{
    ASSERT ( NULL != szProgName ) ;
    char szBuff[ MAX_PATH + 1 ] ;
    int  iRet ;

    iRet = WideCharToMultiByte ( CP_ACP             ,
                                 0                  ,
                                 szProgName         ,
                                 -1                 ,
                                 szBuff             ,
                                 sizeof ( szBuff )  ,
                                 NULL               ,
                                 NULL                ) ;
    ASSERT ( 0 != iRet ) ;
    return ( MemStressInitializeA ( szBuff ) ) ;
}
#else
BOOL  __stdcall
    MemStressInitializeW ( const wchar_t * )
{
    return ( TRUE ) ;
}
#endif  // _DEBUG

#ifdef _DEBUG
BOOL  __stdcall
    MemStressTerminate ( void )
{
    // The return value.
    BOOL bRet = TRUE ;

    __try
    {
        __try
        {
            ASSERT ( TRUE == g_bLibIsInit ) ;
            ASSERT ( NULL != g_pFailInfo ) ;

            // Now immediately, try and grab it.
            EnterCriticalSection ( &g_stCritSec ) ;

            // Notice that I do not explicitly free any of the memory
            // allocated out of the private heap.  Since the whole
            // block gets destroyed in a single call, there is no
            // reason to do each subblock.
            BOOL bHRet = HeapDestroy ( g_hHeap ) ;
            ASSERT ( FALSE != bHRet ) ;
            g_hHeap = NULL ;
            g_pFailInfo = NULL ;
            g_bLibIsInit = FALSE ;

            // Set the allocation hook back to what it was.
            _CrtSetAllocHook ( (_CRT_ALLOC_HOOK)pfnPrevHook ) ;

        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ASSERT ( !"Caught a crash in MemStressTerminate!" ) ;
            bRet = FALSE ;
        }
    }
    __finally
    {
        LeaveCriticalSection ( &g_stCritSec ) ;
        // Just get rid of the critical section.
        DeleteCriticalSection ( &g_stCritSec ) ;
    }
    return ( bRet ) ;
}
#else
BOOL  __stdcall
    MemStressTerminate ( void )
{
    return ( TRUE ) ;
}
#endif  // _DEBUG

/*//////////////////////////////////////////////////////////////////////
       The following functions are active only in _DEBUG builds.
//////////////////////////////////////////////////////////////////////*/
#ifdef _DEBUG

/*----------------------------------------------------------------------
Function        :   AllocationHook
Discussion      :
    The memory allocation hook that is installed to handle the memory
failures.
Parameters      :
    nAllocType - The type of allocation operation, _HOOK_ALLOC,
                 _HOOK_REALLOC, or _HOOK_FREE.
    pvData     - The pointer to the data if nAllocType is _HOOK_FREE,
                 otherwise NULL.
    nSize      - The size of the memory block.
    nBlockUse  - The type of memory block, _CRT_BLOCK, etc.
    lRequest   - The block request number.
    szFileName - The file that had the memory operation.
    nLine      - The line where the memory operation took place.
Returns         :
    TRUE  - The allocation function should succeed.
    FALSE - The allocation function should fail.
----------------------------------------------------------------------*/
static int AllocationHook ( int                   nAllocType    ,
                            void *                pvData        ,
                            size_t                nSize         ,
                            int                   nBlockUse     ,
                            long                  /*lRequest*/  ,
                            const unsigned char * szFileName    ,
                            int                   nLine          )
{
    BOOL bRet = TRUE ;

    // Before I do anything else, double check that thread executing
    // is not recursing in because of my call to MessageBox below.
    DWORD dwRecursion = (DWORD)TlsGetValue ( g_tlsRecursionIndex ) ;
    if ( dwRecursion > 0 )
    {
        // Re-entering because of the message loop!  Get out of here.
        return ( bRet ) ;
    }
    dwRecursion++ ;
    TlsSetValue ( g_tlsRecursionIndex , (LPVOID)dwRecursion ) ;

    __try
    {
        __try
        {
            // Grab the critical section.
            EnterCriticalSection ( &g_stCritSec ) ;

            // The CRT blocks must be ignored or we will cause many
            // problems.
            if ( _CRT_BLOCK == nBlockUse )
            {
                __leave ;
            }

            // Skip all the processing for _HOOK_FREE type calls.
            if ( _HOOK_FREE == nAllocType )
            {
                _leave ;
            }

            // Now go through the litany of possible failure cases.
            if ( TRUE == g_pFailInfo->bGENFailAllAllocs )
            {
                bRet = FALSE ;
                __leave ;
            }
            if ( TRUE == g_pFailInfo->bGENAskOnEach )
            {
                char szBuff [ 1024 ] ;
                if ( NULL == szFileName )
                {
                    wsprintfA ( szBuff , "Unknown File & Line!" ) ;
                }
                else
                {
                    wsprintfA ( szBuff       ,
                                k_MSGFMT     ,
                                szFileName   ,
                                nLine         ) ;
                }


                if ( IDYES == MessageBoxA ( GetActiveWindow ( ) ,
                                            szBuff              ,
                                            k_MSGTITLE          ,
                                            MB_YESNO          ) )
                {
                    bRet = FALSE ;
                    __leave ;
                }
            }
            if ( 0 != g_pFailInfo->uiGENFailEveryN )
            {
                if ( 0 == g_stMemStats.lTotalAllocCalls %
                          g_pFailInfo->uiGENFailEveryN    )
                {
                    bRet = FALSE ;
                    __leave ;
                }
            }
            if ( 0 != g_pFailInfo->uiGENFailAfterX )
            {
                if ( g_pFailInfo->uiGENFailAfterX     <=
                     (UINT)g_stMemStats.lMaxAllocated    )
                {
                    bRet = FALSE ;
                    _leave ;
                }
            }
            if ( 0 != g_pFailInfo->uiGENFailOverY )
            {
                if ( nSize > g_pFailInfo->uiGENFailOverY )
                {
                    bRet = FALSE ;
                    _leave ;
                }
            }
            if ( 0 != g_pFailInfo->uiFileCount )
            {
                // If no filename is passed in, there is not much I can
                // do here.
                if ( NULL != szFileName )
                {

                    // Because the value for the __FILE__ macro seems to
                    // be randomly generated, I need to strip off any
                    // path stuff that may, or may not, be there.
                    char szJustFilename[ MAX_PATH ] ;
                    char szExt[ MAX_PATH ] ;

                    _splitpath ( (LPCSTR)szFileName   ,
                                  NULL                ,
                                  NULL                ,
                                  szJustFilename      ,
                                  szExt                ) ;

                    strcat ( szJustFilename , szExt ) ;

                    for ( UINT i = 0                   ;
                          i < g_pFailInfo->uiFileCount ;
                          i++                           )
                    {
                        if ( 0 ==
                            stricmp ( g_pFailInfo->pFailFiles[i].szFile,
                                      (LPCSTR)szJustFilename          ))
                        {
                            if((-1 ==g_pFailInfo->pFailFiles[i].lLine)||
                               ( nLine ==
                                      g_pFailInfo->pFailFiles[i].lLine))
                            {
                                bRet = FALSE ;
                                __leave ;
                            }
                        }
                    }
                }
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ASSERT ( !"Caught a crash in AllocationHook!\n" ) ;
        }
    }
    __finally
    {
        // Now that the allocation success or failure is known, do the
        // statistics upkeep.
        if ( _HOOK_FREE == nAllocType )
        {
            // Since the size is always zero when this is
            // called for a free function, get the
            // information ourselves.
            _CrtMemBlockHeader * pHead = pHdr(pvData) ;

            g_stMemStats.lTotalReleaseCalls++ ;
            g_stMemStats.lCurrentlyInUse -= pHead->nDataSize ;
            // Since the user can easily start and stop the hook
            // whenever they want to, we might see extra calls to frees
            // here.  If that is the case, don't let the currently
            // used go to less than zero.
            if ( g_stMemStats.lCurrentlyInUse <= 0 )
            {
                g_stMemStats.lCurrentlyInUse = 0 ;
            }
        }
        else if ( TRUE == bRet )
        {
            g_stMemStats.lMaxAllocated += nSize ;
            g_stMemStats.lCurrentlyInUse += nSize ;
            switch ( nAllocType )
            {
                case _HOOK_ALLOC    :
                    g_stMemStats.lTotalAllocCalls++ ;
                    break ;
                case _HOOK_REALLOC  :
                    g_stMemStats.lTotalReAllocCalls++ ;
                    break ;
                default     :
                    ASSERT ( FALSE ) ;
                    break ;
            }
        }

        // Make sure that we always execute this!
        LeaveCriticalSection ( &g_stCritSec ) ;

        // This thread is done, drop the recursion flag.
        TlsSetValue ( g_tlsRecursionIndex , 0 ) ;
    }
    return ( bRet ) ;
}


// The function that does the INI reading.
static LPFAILUREINFO ProcessINIFileA ( LPCSTR szProgram  )

{
    // Allocate the space for the buffer that will be returned.
    LPFAILUREINFO pFailInfo =
               (LPFAILUREINFO)HeapAlloc ( g_hHeap                    ,
                                          HEAP_GENERATE_EXCEPTIONS |
                                            HEAP_ZERO_MEMORY         ,
                                          sizeof ( FAILUREINFO )      );

    // Look to see if the section with the program name really exists.
    // If it does not the default section will be used.
    char    szBuff [ MAX_PATH + 1 ] ;
    DWORD   dwRet                   ;
    LPCSTR  lpAppName               ;

    dwRet = GetPrivateProfileSectionA ( szProgram         ,
                                        szBuff            ,
                                        sizeof ( szBuff ) ,
                                        k_INI_FILENAME     ) ;
    if ( 0 == dwRet )
    {
        // The section for this program does not exist so use the
        // default.
        lpAppName = k_INI_DEFAULTSECTION ;
    }
    else
    {
        lpAppName = szProgram ;
    }

    // Pound through and get all of the fields.
    pFailInfo->bCRTCheckMemory =
                           GetPrivateProfileIntA ( lpAppName         ,
                                                   k_INI_CRTCHECKMEM ,
                                                   1                 ,
                                                   k_INI_FILENAME     );
    pFailInfo->bCRTDelayMemFrees =
                           GetPrivateProfileIntA ( lpAppName         ,
                                                   k_INI_CRTDELAYMEM ,
                                                   1                 ,
                                                   k_INI_FILENAME     );
    pFailInfo->bCRTLeakChecking =
                           GetPrivateProfileIntA ( lpAppName           ,
                                                   k_INI_CRTCHECKLEAKS ,
                                                   1                   ,
                                                   k_INI_FILENAME     );

    pFailInfo->bGENFailAllAllocs =
                         GetPrivateProfileIntA ( lpAppName             ,
                                                 k_INI_GENFAILALLALLOCS,
                                                 0                     ,
                                                 k_INI_FILENAME       );
    pFailInfo->uiGENFailEveryN =
                      GetPrivateProfileIntA ( lpAppName                ,
                                             k_INI_GENFAILEVERYNALLOCS ,
                                             0                         ,
                                             k_INI_FILENAME           );
    pFailInfo->uiGENFailAfterX =
                       GetPrivateProfileIntA ( lpAppName               ,
                                              k_INI_GENFAILAFTERXBYTES ,
                                              0                        ,
                                              k_INI_FILENAME          );
    pFailInfo->uiGENFailOverY =
                        GetPrivateProfileIntA ( lpAppName              ,
                                               k_INI_GENFAILOVERYBYTES ,
                                               0                       ,
                                               k_INI_FILENAME         );
    pFailInfo->bGENAskOnEach =
                        GetPrivateProfileIntA ( lpAppName              ,
                                               k_INI_GENASKONEACHALLOC ,
                                               0                       ,
                                               k_INI_FILENAME         );

    // Now for the fun stuff, loop through and load the files to fail.
    pFailInfo->uiFileCount =
                       GetPrivateProfileIntA ( lpAppName               ,
                                              k_INI_GENNUMFILEFAILURES ,
                                              0                        ,
                                              k_INI_FILENAME          );
    if ( 0 != pFailInfo->uiFileCount )
    {
        // Allocate the space in the structure.
        pFailInfo->pFailFiles =
                    (LPFAILFILE)HeapAlloc ( g_hHeap                    ,
                                          HEAP_GENERATE_EXCEPTIONS |
                                              HEAP_ZERO_MEMORY         ,
                                          sizeof ( FAILFILE ) *
                                               pFailInfo->uiFileCount );
        char   szCurrFile [ 30 ] ;
        LPSTR  pEndOfConst       ;
        UINT   uiCurrIndex       ;
        UINT   uiCount           ;
        LPSTR  pTok1             ;
        LPSTR  pTok2             ;
        long   lData             ;

        lstrcpy ( szCurrFile , k_INI_GENFILEFAILPREFIX ) ;
        pEndOfConst = szCurrFile + lstrlen ( k_INI_GENFILEFAILPREFIX ) ;

        uiCurrIndex = 0 ;

        for ( uiCount = 1                       ;
              uiCount <= pFailInfo->uiFileCount ;
              uiCount++                          )
        {
            // Build up the key to read.
            itoa ( uiCount , pEndOfConst , 10 ) ;

            // Read the key.
            dwRet = GetPrivateProfileStringA ( lpAppName         ,
                                               szCurrFile        ,
                                               ""                ,
                                              szBuff             ,
                                              sizeof ( szBuff )  ,
                                              k_INI_FILENAME      ) ;

            // If nothing was read, then just go for the next one.
            if ( 0 == dwRet )
            {
                continue ;
            }

            // Extract the two parts of the string.
            pTok1 = strtok ( szBuff , k_SEP_FILEANDLINE ) ;
            pTok2 = strtok ( NULL , k_SEP_FILEANDLINE ) ;

            if ( ( NULL == pTok1 ) || ( NULL == pTok2 ) )
            {
                continue ;
            }

            // Convert the second string into a number.
            lData = Crasher::atol ( pTok2 ) ;

            if ( 0 == lData )
            {
                continue ;
            }

            // Whew!  We can go ahead and add this one.
            pFailInfo->pFailFiles[ uiCurrIndex ].lLine = lData ;
            lstrcpy ( pFailInfo->pFailFiles[uiCurrIndex].szFile ,
                     pTok1                                      ) ;

            // Update the array counter.
            uiCurrIndex++ ;
        }

        // Set the count to the proper count since some entries might
        // have been thrown away.
        pFailInfo->uiFileCount = uiCurrIndex ;
    }
    return ( pFailInfo ) ;
}


BOOL InternalMemStressInitialize ( void )
{
    // This function is called during the DllMain DLL_PROCESS_ATTACH
    // processing.  All I need to do is to initialize the thread
    // local storage index.
    g_tlsRecursionIndex = TlsAlloc ( ) ;

    return ( TLS_OUT_OF_INDEXES != g_tlsRecursionIndex ) ;

}

BOOL InternalMemStressShutdown ( void )
{
    // Free up the thread-local storage index.
    BOOL bRet = TlsFree ( g_tlsRecursionIndex ) ;
    g_tlsRecursionIndex = TLS_OUT_OF_INDEXES ;
    return ( bRet ) ;
}

#endif  // _DEBUG

