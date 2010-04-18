// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

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
#pragma once

/// used to enable optimized implementations on 64 bit systems

#if (defined (_WIN64) || (__WORDSIZE == 64)) && !defined(_64BITS)
#define _64BITS
#endif

/**
 * defines global types and their associated "invalid" values.
 */
namespace LogCache
{
    /// log caching index
#ifdef HUGE_LOG_CACHE
    typedef size_t index_t;
#else
    typedef unsigned index_t;
#endif

    /// revision number
    typedef index_t revision_t;

    enum
    {
        /// invalid/unknown index
        NO_INDEX = (index_t)(-1),
        /// invalid/unknown revision
        NO_REVISION = (revision_t)(-1),
    };
};