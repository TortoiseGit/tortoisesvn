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
#include "StdAfx.h"
#include "LogCacheSettings.h"

// TSVN log cache settings path within the registry

#define REGKEY(x) _T("Software\\TortoiseSVN\\LogCache\\" ## _T(x))
#define REGKEY15(x) _T("Software\\TortoiseSVN\\" ## _T(x))

// begin namespace LogCache

namespace LogCache
{

// singleton construction

CSettings::CSettings()
    : enableLogCaching (REGKEY ("UseLogCache"), TRUE)
	, supportAmbiguousURL (REGKEY ("SupportAmbiguousURL"), FALSE)
	, supportAmbiguousUUID (REGKEY ("SupportAmbiguousUUID"), FALSE)
    , defaultConnectionState (REGKEY ("DefaultConnectionState"), 0)
    , maxHeadAge (REGKEY ("HeadCacheAgeLimit"), 0)
    , cacheDropAge (REGKEY ("CacheDropAge"), 10)
    , cacheDropMaxSize (REGKEY ("CacheDropMaxSize"), 200)
    , maxFailuresUntilDrop (REGKEY ("MaxCacheFailures"), 0)
{
}

// singleton acccess

CSettings& CSettings::GetInstance()
{
    static CSettings settings;
    return settings;
}

// migrate settings from TSVN 1.5.x to 1.6.x

void CSettings::Migrate()
{
	CRegDWORD oldEnableLogCaching (REGKEY15 ("UseLogCache"));
    if (oldEnableLogCaching.LastError == ERROR_SUCCESS)
    {
        SetEnabled ((DWORD)oldEnableLogCaching != FALSE);
        oldEnableLogCaching.removeKey();
    }

    CRegDWORD oldSupportAmbiguousURL (REGKEY15 ("SupportAmbiguousURL"));
    if (oldSupportAmbiguousURL.LastError == ERROR_SUCCESS)
    {
        SetAllowAmbiguousURL ((DWORD)oldSupportAmbiguousURL != FALSE);
        oldSupportAmbiguousURL.removeKey();
    }

    CRegDWORD oldDefaultConnectionState (REGKEY15 ("DefaultConnectionState"));
    if (oldDefaultConnectionState.LastError == ERROR_SUCCESS)
    {
        CRepositoryInfo::ConnectionState state 
            = static_cast<CRepositoryInfo::ConnectionState>
                ((DWORD)oldDefaultConnectionState);

        SetDefaultConnectionState (state);
        oldDefaultConnectionState.removeKey();
    }

    CRegDWORD oldMaxHeadAge (REGKEY15 ("HeadCacheAgeLimit"));
    if (oldMaxHeadAge.LastError == ERROR_SUCCESS)
    {
        SetMaxHeadAge ((DWORD)oldMaxHeadAge);
        oldMaxHeadAge.removeKey();
    }
}

/// has log caching been enabled?

bool CSettings::GetEnabled()
{
    return (DWORD)GetInstance().enableLogCaching != FALSE;
}

void CSettings::SetEnabled (bool enabled)
{
    Store (enabled ? TRUE : FALSE, GetInstance().enableLogCaching);
}

/// cache lookup mode

bool CSettings::GetAllowAmbiguousURL()
{
    return (DWORD)GetInstance().supportAmbiguousURL != FALSE;
}

void CSettings::SetAllowAmbiguousURL (bool allowed)
{
    Store (allowed ? TRUE : FALSE, GetInstance().supportAmbiguousURL);
}

bool CSettings::GetAllowAmbiguousUUID()
{
    return (DWORD)GetInstance().supportAmbiguousUUID != FALSE;
}

void CSettings::SetAllowAmbiguousUUID (bool allowed)
{
    Store (allowed ? TRUE : FALSE, GetInstance().supportAmbiguousUUID);
}

/// "go offline" usage

CRepositoryInfo::ConnectionState CSettings::GetDefaultConnectionState()
{
    return static_cast<CRepositoryInfo::ConnectionState>
            ((DWORD)GetInstance().defaultConnectionState);
}

void CSettings::SetDefaultConnectionState (CRepositoryInfo::ConnectionState state)
{
    Store (state, GetInstance().defaultConnectionState);
}

/// controls when to bypass the repository HEAD lookup 

int CSettings::GetMaxHeadAge()
{
    return GetInstance().maxHeadAge;
}

void CSettings::SetMaxHeadAge (int limit)
{
    Store (limit, GetInstance().maxHeadAge);
}

/// control the removal of obsolete caches

int CSettings::GetCacheDropAge()
{
    return GetInstance().cacheDropAge;
}

void CSettings::SetCacheDropAge (int limit)
{
    Store (limit, GetInstance().cacheDropAge);
}

int CSettings::GetCacheDropMaxSize()
{
    return GetInstance().cacheDropMaxSize;
}

void CSettings::SetCacheDropMaxSize (int limit)
{
    Store (limit, GetInstance().cacheDropMaxSize);
}

/// debugging support

int CSettings::GetMaxFailuresUntilDrop()
{
    return GetInstance().maxFailuresUntilDrop;
}

void CSettings::SetMaxFailuresUntilDrop (int limit)
{
    Store (limit, GetInstance().maxFailuresUntilDrop);
}

// end namespace LogCache

}

