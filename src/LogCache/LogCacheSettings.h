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

///////////////////////////////////////////////////////////////
// required includes
///////////////////////////////////////////////////////////////

#include "ConnectionState.h"

#ifdef WIN32
#include "Registry.h"
#include "MiscUI/MessageBox.h"
#else
#include <iostream>
#include "RegistryDummy.h"
#endif

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

/**
 * Central storage for all repository specific log caches currently in use. New
 * caches will be created automatically.
 *
 * Currently, there is no method to delete unused cache files or to selectively
 * remove cache info from RAM.
 *
 * The filenames of the log caches are the repository UUIDs.
 */
class CSettings
{
private:

    /// registry connection

    CRegDWORD enableLogCaching;
    CRegDWORD supportAmbiguousURL;
    CRegDWORD supportAmbiguousUUID;
    CRegDWORD defaultConnectionState;

    CRegDWORD maxHeadAge;
    CRegDWORD cacheDropAge;
    CRegDWORD cacheDropMaxSize;

    CRegDWORD maxFailuresUntilDrop;

    /// singleton construction

    CSettings();

    /// singleton acccess

    static CSettings& GetInstance();

    /// utility method

    template<class T, class R>
    static void Store (T value, R& registryKey)
    {
        registryKey = value;
        if (registryKey.GetLastError() != ERROR_SUCCESS)
    #ifdef WIN32
            CMessageBox::Show (NULL, registryKey.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
    #else
            std::cerr << "Could not store settings" << std::endl;
    #endif
    }

public:

    /// migrate settings from TSVN 1.5.x to 1.6.x

    static void Migrate();

    /// has log caching been enabled?

    static bool GetEnabled();
    static void SetEnabled (bool enabled);

    /// cache lookup mode

    static bool GetAllowAmbiguousURL();
    static void SetAllowAmbiguousURL (bool allowed);

    static bool GetAllowAmbiguousUUID();
    static void SetAllowAmbiguousUUID (bool allowed);

    /// "go offline" usage

    static ConnectionState GetDefaultConnectionState();
    static void SetDefaultConnectionState (ConnectionState state);

    /// controls when to bypass the repository HEAD lookup

    static int GetMaxHeadAge();
    static void SetMaxHeadAge (int limit);

    /// control the removal of obsolete caches

    static int GetCacheDropAge();
    static void SetCacheDropAge (int limit);

    static int GetCacheDropMaxSize();
    static void SetCacheDropMaxSize (int limit);

    /// debugging support

    static int GetMaxFailuresUntilDrop();
    static void SetMaxFailuresUntilDrop (int limit);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

