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

#include "stdafx.h"
#include "SVNTrace.h"
#include "DebugOutput.h"
#include "UnicodeUtils.h"

// global instance counter

volatile LONG CSVNTrace::counter = 0;

// construction: write call description to ODS and start clock

CSVNTrace::CSVNTrace
    ( const wchar_t* name
    , int lineNo
    , const wchar_t* line
    , const char* svnPath)
    : id (InterlockedIncrement (&counter))
    , threadID (GetCurrentThreadId())
{
    CString svnAPI = line;
    int assignPos = svnAPI.Find ('=');
    if (assignPos > 0)
        svnAPI.Delete (0, assignPos+1);

    svnAPI = svnAPI.TrimLeft().SpanExcluding (_T(" \r\n\t("));
    CString path;
    if (svnPath)
        path = CString (_T("Path=")) + CUnicodeUtils::GetUnicode(svnPath);

    CTraceToOutputDebugString::Instance()(_T("#%d Thread:%d %s(line %d) %s %s\n")
             , id
             , threadID
             , name
             , lineNo
             , (const wchar_t*)svnAPI
             , (const wchar_t*)path);
}


// destruction: call \ref Stop, if it has not been called, yet

CSVNTrace::~CSVNTrace()
{
    if (id != -1)
        Stop();
}

// write 'call finished' info and time taken to ODS

void CSVNTrace::Stop()
{
    clock.Stop();


    CStringA s;
    CTraceToOutputDebugString::Instance() ( _T("#%d Thread:%d done (%d µs)\n")
             , id
             , threadID
             , clock.GetMusecsTaken());

    id = -1;
}
