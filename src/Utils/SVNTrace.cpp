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

// global instance counter

volatile LONG CSVNTrace::counter = 0;

// construction: write call description to ODS and start clock

CSVNTrace::CSVNTrace 
    ( const char* name
    , int lineNo     
    , const char* line
    , const char* svnPath)
    : id (InterlockedIncrement (&counter))
    , threadID (GetCurrentThreadId())
{
    CStringA svnAPI = line;
    int assignPos = svnAPI.Find ('=');
    if (assignPos > 0)
        svnAPI.Delete (0, assignPos+1);

    svnAPI = svnAPI.TrimLeft().SpanExcluding (" \r\n\t(");
    CStringA path;
    if (svnPath)
        path = CStringA ("Path=") + svnPath;

    CStringA s;
    s.Format ( "#%d Thread:%d %s(%d) %s %s\n"
             , id
             , threadID
             , name
             , lineNo
             , (const char*)svnAPI
             , (const char*)path);

    OutputDebugStringA (s);
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
    s.Format ( "C:%d T:%d done (%d µs)\n"
             , id
             , threadID
             , clock.GetMusecsTaken());
    
    OutputDebugStringA (s);

    id = -1;
}
