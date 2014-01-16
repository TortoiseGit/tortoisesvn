#pragma once

// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009, 2014 - TortoiseSVN

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

//////////////////////////////////////////////////////////////////////
// required includes
//////////////////////////////////////////////////////////////////////

#include "HighResClock.h"

/**
* RAII class that encapsulates a single execution of a profiled block / line.
* Writes the results to TSVN's OutputDebugString wrapper.
*/

class CSVNTrace
{
private:

    /// global instance counter

    static volatile LONG counter;

    /// used to measure the execution time until \ref Stop() gets called

    CHighResClock clock;

    /// value of \ref counter upon construction,
    /// i.e. unique call number

    LONG id;

    /// thread identifier

    DWORD threadID;

public:

    /// construction: write call description to ODS and start clock

    CSVNTrace ( const wchar_t* name     ///< name of the calling function
              , int lineNo              ///< line number in the same source file
              , const wchar_t* line     ///< svn API call
              , const char* svnPath = NULL  ///< usually the main parameter
              );

    /// destruction: call \ref Stop, if it has not been called, yet

    ~CSVNTrace();

    /// write 'call finished' info and time taken to ODS

    void Stop();
};

/**
* Tracing macros
*/

#define SVNTRACE_CONCAT3( a, b )  a##b
#define SVNTRACE_CONCAT2( a, b )  SVNTRACE_CONCAT3( a, b )
#define SVNTRACE_CONCAT( a, b )   SVNTRACE_CONCAT2( a, b )

#define SVNTRACE(line,path)\
    CSVNTrace SVNTRACE_CONCAT(__svnTrace,__LINE__) (_T(__FUNCTION__),__LINE__,_T(#line),path);\
    line;\
    SVNTRACE_CONCAT(__svnTrace,__LINE__).Stop();

#define SVNTRACE_BLOCK\
    CSVNTrace SVNTRACE_CONCAT(__svnTrace,__LINE__) (_T(__FUNCTION__),__LINE__,L"<whole_block>",NULL);

