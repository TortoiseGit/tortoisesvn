/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#ifndef _MEMSTRESS_H
#define _MEMSTRESS_H

// This file should not be included directly, include Bugslayer.h
// instead.
#ifndef _BUGSLAYERUTIL_H
#error "Include BugslayerUtil.h instead of this file directly!"
#endif  // _BUGSLAYERUTIL_H

#ifdef __cplusplus
extern "C" {
#endif      // __cplusplus

#ifdef _DEBUG

#include "MSJDBG.h"

/*----------------------------------------------------------------------
Function        :   MemStressInitialize
Discussion      :
    Initializes the memory stressing routines.
Parameters      :
    szProgName - The name of the program.  This is the same name that
                 a set of options was stored with in the C/C++ Memory
                 Stress window of the STRESS32.EXE application.  This
                 parameter is used to read the settings out of the
                 MEMSTRESS.INI file.
Returns         :
    1 - All initialization went correctly.
    0 - There was a problem initializing.  Read the output messages from
        OutputDebugStrings in the debugger to see what went wrong.
----------------------------------------------------------------------*/
int  __stdcall
    MemStressInitializeA ( const char * szProgName ) ;
int  __stdcall
    MemStressInitializeW ( const wchar_t * szProgName ) ;

#ifndef UNICODE
#define MemStressInitialize MemStressInitializeA
#else
#define MemStressInitialize MemStressInitializeW
#endif

/*----------------------------------------------------------------------
Function        :   TerminateMemStress
Discussion      :
    Terminates the memory stressing.
Parameters      :
    None.
Returns         :
    1 - The termination went correctly.
    0 - There was a problem terminating.  Read the output messages from
        OutputDebugStrings in the debugger to see what went wrong.
----------------------------------------------------------------------*/
int  __stdcall MemStressTerminate ( void ) ;

////////////////////////////////////////////////////////////////////////
// Helper Macros.
////////////////////////////////////////////////////////////////////////
#define MEMSTRESSINIT(x)        MemStressInitialize(x)
#define MEMSTRESSTERMINATE()     MemStressTerminate ( )

#else       // _DEBUG defined

#define MEMSTRESSINIT(x)
#define MEMSTRESSTERMINATE()

#endif      // _DEBUG

#ifdef __cplusplus
}
#endif      // __cplusplus

#endif      // _MEMSTRESS_H


