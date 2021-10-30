// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2015, 2017, 2021 - TortoiseSVN

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
#include "registry.h"
#include "Globals.h"
#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <shlobj.h>
#pragma warning(pop)

#define ADMINDIRTIMEOUT  10000
#define DRIVETYPETIMEOUT 300000 // 5 min
#define NUMBERFMTTIMEOUT 300000

using Locker = CComCritSecLock<CComCriticalSection>;

struct BoolTimeout
{
    bool      bBool;
    ULONGLONG timeout;
};

/**
 * \ingroup TortoiseShell
 * Helper class which caches access to the registry. Also provides helper methods
 * for checks against the settings stored in the registry.
 */
class ShellCache
{
public:
    enum CacheType
    {
        None,
        Exe,
        Dll
    };
    ShellCache();
    ~ShellCache();

    bool                   RefreshIfNeeded();
    CacheType              GetCacheType();
    DWORD                  BlockStatus();
    TSVNContextMenuEntries GetMenuLayout11();
    unsigned __int64       GetMenuLayout();
    unsigned __int64       GetMenuMask();
    bool                   IsProcessElevated() const;

    BOOL IsOnlyNonElevated();
    BOOL IsRecursive();
    BOOL IsFolderOverlay();
    BOOL HasShellMenuAccelerators();
    BOOL IsUnversionedAsModified();
    BOOL IsIgnoreOnCommitIgnored();
    BOOL IsGetLockTop();
    BOOL ShowExcludedAsNormal();
    BOOL AlwaysExtended();
    BOOL HideMenusForUnversionedItems();

    BOOL IsRemote();
    BOOL IsFixed();
    BOOL IsCDRom();
    BOOL IsRemovable();
    BOOL IsRAM();
    BOOL IsUnknown();

    BOOL       IsContextPathAllowed(LPCWSTR path);
    BOOL       IsPathAllowed(LPCWSTR path);
    DWORD      GetLangID();
    NUMBERFMT* GetNumberFmt();
    BOOL       IsVersioned(LPCWSTR path, bool bIsDir, bool mustbeok);
    bool       IsColumnsEveryWhere();

private:
    void ExcludeContextValid();
    void ValidatePathFilter();

    class CPathFilter
    {
    public:
        /// node in the lookup tree

        struct SEntry
        {
            std::wstring path;

            /// default (path spec did not end a '?').
            /// if this is not set, the default for all
            /// sub-paths is !included.
            /// This is a temporary setting an be invalid
            /// after @ref PostProcessData

            bool recursive;

            /// this is an "include" specification

            svn_tristate_t included;

            /// if @ref recursive is not set, this is
            /// the parent path status being passed down
            /// combined with the information of other
            /// entries for the same @ref path.

            svn_tristate_t subPathIncluded;

            /// do entries for sub-paths exist?

            bool hasSubFolderEntries;

            /// STL support
            /// For efficient folding, it is imperative that
            /// "recursive" entries are first

            bool operator<(const SEntry& rhs) const
            {
                int diff = _wcsicmp(path.c_str(), rhs.path.c_str());
                return (diff < 0) || ((diff == 0) && recursive && !rhs.recursive);
            }

            friend bool operator<(const SEntry& rhs, const std::pair<LPCWSTR, size_t>& lhs);
            friend bool operator<(const std::pair<LPCWSTR, size_t>& lhs, const SEntry& rhs);
        };

    private:
        /// lookup by path (all entries sorted by path)

        std::vector<SEntry> data;

        /// registry keys plus cached last content

        CRegStdString excludelist;
        std::wstring  excludeliststr;

        CRegStdString includelist;
        std::wstring  includeliststr;

        /// construct \ref data content

        void AddEntry(const std::wstring& s, bool include);
        void AddEntries(const std::wstring& s, bool include);

        /// for all paths, have at least one entry in data

        void PostProcessData();

        /// lookup. default result is "unknown".
        /// We must look for *every* parent path because of situations like:
        /// excluded: C:, C:\some\deep\path
        /// include: C:\some
        /// lookup for C:\some\deeper\path

        svn_tristate_t IsPathAllowed(LPCWSTR path, std::vector<SEntry>::const_iterator begin, std::vector<SEntry>::const_iterator end) const;

    public:
        /// construction

        CPathFilter();

        /// notify of (potential) registry settings

        void Refresh();

        /// data access

        svn_tristate_t IsPathAllowed(LPCWSTR path) const;
    };

    friend bool  operator<(const CPathFilter::SEntry& rhs, const std::pair<LPCWSTR, size_t>& lhs);
    friend bool  operator<(const std::pair<LPCWSTR, size_t>& lhs, const CPathFilter::SEntry& rhs);
    CRegStdDWORD cacheType;
    CRegStdDWORD blockStatus;
    CRegStdDWORD langId;
    CRegStdDWORD onlyNonElevated;
    CRegStdDWORD showRecursive;
    CRegStdDWORD folderOverlay;
    CRegStdDWORD getLockTop;
    CRegStdDWORD driveRemote;
    CRegStdDWORD driveFixed;
    CRegStdDWORD driveCdRom;
    CRegStdDWORD driveRemove;
    CRegStdDWORD driveFloppy;
    CRegStdDWORD driveRam;
    CRegStdDWORD driveUnknown;
    CRegStdQWORD menuLayout11;
    CRegStdDWORD menuLayoutLow;
    CRegStdDWORD menuLayoutHigh;
    CRegStdDWORD shellMenuAccelerators;
    CRegStdDWORD menuMaskLowLm;
    CRegStdDWORD menuMaskHighLm;
    CRegStdDWORD menuMaskLowCu;
    CRegStdDWORD menuMaskHighCu;
    CRegStdDWORD unversionedAsModified;
    CRegStdDWORD ignoreOnCommitIgnored;
    CRegStdDWORD excludedAsNormal;
    CRegStdDWORD alwaysExtended;
    CRegStdDWORD hideMenusForUnversionedItems;
    CRegStdDWORD columnsEverywhere;

    CPathFilter pathFilter;

    ULONGLONG                           driveTypeTicker;
    ULONGLONG                           columnRevTormatTicker;
    UINT                                driveTypeCache[27];
    wchar_t                             driveTypePathCache[MAX_PATH]; // MAX_PATH ok.
    NUMBERFMT                           columnrevformat;
    wchar_t                             szDecSep[5];
    wchar_t                             szThousandsSep[5];
    std::map<std::wstring, BoolTimeout> adminDirCache;
    CRegStdString                       noContextPaths;
    std::wstring                        excludeContextStr;
    std::vector<std::wstring>           exContextVector;
    CComAutoCriticalSection             m_critSec;
    HANDLE                              m_registryChangeEvent;
    HKEY                                m_hNotifyRegKey;
    bool                                isElevated;
};

inline bool operator<(const ShellCache::CPathFilter::SEntry& lhs, const std::pair<LPCWSTR, size_t>& rhs)
{
    return _wcsnicmp(lhs.path.c_str(), rhs.first, rhs.second) < 0;
}

inline bool operator<(const std::pair<LPCWSTR, size_t>& lhs, const ShellCache::CPathFilter::SEntry& rhs)
{
    return _wcsnicmp(lhs.first, rhs.path.c_str(), lhs.second) < 0;
}
