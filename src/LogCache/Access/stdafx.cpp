// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

const char* Time64ToZuluString (char* buffer, size_t size, __time64_t timeStamp)
{
    // default on error: empty string

    if (size <= 0)
        return buffer;

    buffer[0] = 0;

    // empty time stamps don't show up in the log

    if ( timeStamp == 0 )
        return buffer;

    // decode 64 bit time stamp

    int musecs = ( int ) ( timeStamp % 1000000 );
    timeStamp /= 1000000;
    tm time;

#ifdef WIN32
    if ( _gmtime64_s ( &time, &timeStamp ) != 0 )
        return buffer;
#else
    time_t temp = (time_t)(timeStamp);
    time = *gmtime (&temp);
#endif

    // fill the template & write to stream

    _snprintf_s ( buffer
                , size
                , size-1
                , "%04d-%02d-%02dT%02d:%02d:%02d.%06dZ"
                , time.tm_year + 1900
                , time.tm_mon + 1
                , time.tm_mday
                , time.tm_hour
                , time.tm_min
                , time.tm_sec
                , musecs );

    // done

    return buffer;
}
