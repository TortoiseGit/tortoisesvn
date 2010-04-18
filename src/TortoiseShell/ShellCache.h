// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "SVNAdminDir.h"
#include <shlobj.h>

#define REGISTRYTIMEOUT 2000
#define EXCLUDELISTTIMEOUT 5000
#define ADMINDIRTIMEOUT 10000
#define DRIVETYPETIMEOUT 300000     // 5 min
#define NUMBERFMTTIMEOUT 300000
#define MENUTIMEOUT 100

typedef CComCritSecLock<CComCriticalSection> Locker;

struct BoolTimeout
{
    bool    bBool;
    DWORD   timeout;
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
        none,
        exe,
        dll
    };
    ShellCache();

    void ForceRefresh();
    CacheType GetCacheType();
    DWORD BlockStatus();
    unsigned __int64 GetMenuLayout();
    unsigned __int64 GetMenuMask();

    BOOL IsRecursive();
    BOOL IsFolderOverlay();
    BOOL IsSimpleContext();
    BOOL HasShellMenuAccelerators();
    BOOL IsUnversionedAsModified();
    BOOL IsGetLockTop();
    BOOL ShowExcludedAsNormal();
    BOOL AlwaysExtended();

    BOOL IsRemote();
    BOOL IsFixed();
    BOOL IsCDRom();
    BOOL IsRemovable();
    BOOL IsRAM();
    BOOL IsUnknown();

    BOOL IsContextPathAllowed(LPCTSTR path);
    BOOL IsPathAllowed(LPCTSTR path);
    DWORD GetLangID();
    NUMBERFMT * GetNumberFmt();
    BOOL HasSVNAdminDir(LPCTSTR path, BOOL bIsDir);
    bool IsColumnsEveryWhere();

private:

    void DriveValid();
    void ExcludeContextValid();
    void ValidatePathFilter();

    class CPathFilter
    {
    public:

        /// node in the lookup tree

        struct SEntry
        {
            tstring path;

            /// default (path spec did not end a '?').
            /// if this is not set, the default for all
            /// sub-paths is !included.
            /// This is a temporary setting an be invalid
            /// after @ref PostProcessData

            bool recursive;

            /// this is an "include" specification

            bool included;

            /// if @ref recursive is not set, this is
            /// the parent path status being passed down
            /// combined with the information of other
            /// entries for the same @ref path.

            bool subPathIncluded;

            /// do entries for sub-paths exist?

            bool hasSubFolderEntries;

            /// STL support
            /// For efficient folding, it is imperative that
            /// "recursive" entries are first

            bool operator<(const SEntry& rhs) const
            {
                int diff = _tcsicmp (path.c_str(), rhs.path.c_str());
                return (diff < 0)
                    || ((diff == 0) && recursive && !rhs.recursive);
            }

            friend bool operator<
                ( const SEntry& rhs
                , const std::pair<LPCTSTR, size_t>& lhs);
            friend bool operator<
                ( const std::pair<LPCTSTR
                , size_t>& lhs, const SEntry& rhs);
        };

    private:

        /// lookup by path (all entries sorted by path)

        typedef std::vector<SEntry> TData;
        TData data;

        /// registry keys plus cached last content

        CRegStdString excludelist;
        tstring excludeliststr;

        CRegStdString includelist;
        tstring includeliststr;

        /// construct \ref data content

        void AddEntry (const tstring& s, bool include);
        void AddEntries (const tstring& s, bool include);

        /// for all paths, have at least one entry in data

        void PostProcessData();

        /// lookup. default result is "true".
        /// We must look for *every* parent path because of situations like:
        /// excluded: C:, C:\some\deep\path
        /// include: C:\some\path
        /// lookup for C:\some\deeper\path

        bool IsPathAllowed
            ( LPCTSTR path
            , TData::const_iterator begin
            , TData::const_iterator end) const;

    public:

        /// construction

        CPathFilter();

        /// notify of (potential) registry settings

        void Refresh();

        /// data access

        bool IsPathAllowed (LPCTSTR path) const;
    };

    friend bool operator< (const CPathFilter::SEntry& rhs, const std::pair<LPCTSTR, size_t>& lhs);
    friend bool operator< (const std::pair<LPCTSTR, size_t>& lhs, const CPathFilter::SEntry& rhs);

    CRegStdDWORD cachetype;
    CRegStdDWORD blockstatus;
    CRegStdDWORD langid;
    CRegStdDWORD showrecursive;
    CRegStdDWORD folderoverlay;
    CRegStdDWORD getlocktop;
    CRegStdDWORD driveremote;
    CRegStdDWORD drivefixed;
    CRegStdDWORD drivecdrom;
    CRegStdDWORD driveremove;
    CRegStdDWORD drivefloppy;
    CRegStdDWORD driveram;
    CRegStdDWORD driveunknown;
    CRegStdDWORD menulayoutlow;
    CRegStdDWORD menulayouthigh;
    CRegStdDWORD simplecontext;
    CRegStdDWORD shellmenuaccelerators;
    CRegStdDWORD menumasklow_lm;
    CRegStdDWORD menumaskhigh_lm;
    CRegStdDWORD menumasklow_cu;
    CRegStdDWORD menumaskhigh_cu;
    CRegStdDWORD unversionedasmodified;
    CRegStdDWORD excludedasnormal;
    CRegStdDWORD alwaysextended;
    CRegStdDWORD columnseverywhere;

    CPathFilter pathFilter;

    DWORD cachetypeticker;
    DWORD recursiveticker;
    DWORD folderoverlayticker;
    DWORD getlocktopticker;
    DWORD driveticker;
    DWORD drivetypeticker;
    DWORD layoutticker;
    DWORD menumaskticker;
    DWORD langticker;
    DWORD blockstatusticker;
    DWORD columnrevformatticker;
    DWORD pathfilterticker;
    DWORD simplecontextticker;
    DWORD shellmenuacceleratorsticker;
    DWORD unversionedasmodifiedticker;
    DWORD excludedasnormalticker;
    DWORD alwaysextendedticker;
    DWORD columnseverywhereticker;
    UINT  drivetypecache[27];
    TCHAR drivetypepathcache[MAX_PATH];     // MAX_PATH ok.
    NUMBERFMT columnrevformat;
    TCHAR szDecSep[5];
    TCHAR szThousandsSep[5];
    std::map<tstring, BoolTimeout> admindircache;
    CRegStdString nocontextpaths;
    tstring excludecontextstr;
    std::vector<tstring> excontextvector;
    DWORD excontextticker;
    CComCriticalSection m_critSec;
};

inline bool operator<
    ( const ShellCache::CPathFilter::SEntry& lhs
    , const std::pair<LPCTSTR, size_t>& rhs)
{
    return _tcsnicmp (lhs.path.c_str(), rhs.first, rhs.second) < 0;
}

inline bool operator<
    ( const std::pair<LPCTSTR, size_t>& lhs
    , const ShellCache::CPathFilter::SEntry& rhs)
{
    return _tcsnicmp (lhs.first, rhs.path.c_str(), lhs.second) < 0;
}
