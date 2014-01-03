// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2010, 2014 - TortoiseSVN

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

#include "TSVNPath.h"
#include <map>

struct WCRootsTimes
{
    __int64     FileTime;
    ULONGLONG   LastTicks;
};

class CWCRoots
{
public:
    CWCRoots();
    ~CWCRoots();

    /// Returns the last-write file time of the WC database file.
    /// If the path doesn't have a database file and also no parent
    /// path of it, the returned file time is 0.
    __int64 GetDBFileTime(const CTSVNPath& path);

    /// adds a possible wc path. If the path itself is not a WC root,
    /// this method tries to find the wc root itself.
    /// returns true if the path was added, i.e., a WC root was
    /// found for it. Returns false otherwise.
    bool AddPath(const CTSVNPath& path);

    /// Forces a re-read of the last-write-filetime of the wc.db
    /// file for the specified \c path.
    void NotifyChange(const CTSVNPath& path);

private:
    std::map<CTSVNPath, WCRootsTimes>::iterator AddPathInternal(const CTSVNPath& path);

    CComAutoCriticalSection                 m_critSec;
    std::map<CTSVNPath, WCRootsTimes>       m_WCDBs;
};
