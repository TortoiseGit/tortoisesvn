/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#ifndef _MEMSTRESSLIBCONSTANTS_H
#define _MEMSTRESSLIBCONSTANTS_H

// The name of the INI file.
const LPCTSTR k_INI_FILENAME = _T ( "MEMSTRESS.INI" ) ;

// The name of the default program.
const LPCTSTR k_INI_DEFAULTSECTION = _T ( "(Defaults For All)" ) ;

// The keys written for each program.
const LPCTSTR k_INI_CRTCHECKMEM = _T ( "CRT Check Memory" ) ;
const LPCTSTR k_INI_CRTDELAYMEM = _T ( "CRT Delay Memory Frees" ) ;
const LPCTSTR k_INI_CRTCHECKLEAKS = _T ( "CRT Leak Checking" ) ;
const LPCTSTR k_INI_GENFAILALLALLOCS = _T ( "GEN OPT Fail All Allocs" );
const LPCTSTR k_INI_GENFAILEVERYNALLOCS =
                                  _T ( "GEN OPT Fail Every N Allocs" ) ;
const LPCTSTR k_INI_GENFAILAFTERXBYTES  =
                                   _T ( "GEN OPT Fail After X Bytes" ) ;
const LPCTSTR k_INI_GENFAILOVERYBYTES   =
                                   _T ( "GEN OPT Fail Over Y Bytes" ) ;
const LPCTSTR k_INI_GENASKONEACHALLOC   =
                                   _T ( "GEN OPT Ask On Each Alloc" ) ;
const LPCTSTR k_INI_GENNUMFILEFAILURES  =
                                   _T ( "Number of files to fail" ) ;
const LPCTSTR k_INI_GENFILEFAILPREFIX   = _T ( "File#" ) ;

// The seperator for the file and line number for file entries.
const LPCTSTR k_SEP_FILEANDLINE = _T ( " & " ) ;

// The message box constants for when asking the user.
const LPCTSTR k_MSGFMT =
               _T ( "Press 'Yes' to fail the allocation in %s at %d" ) ;
const LPCTSTR k_MSGTITLE = _T ( "C/C++ Memory Stress" ) ;

#endif      // _MEMSTRESSLIBCONSTANTS_H


