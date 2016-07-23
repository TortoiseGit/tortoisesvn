// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2016 - TortoiseSVN

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
#include "ShellCache.h"
#include "SVNHelpers.h"
#include "TSVNPath.h"

ShellCache::ShellCache()
{
    cachetype = CRegStdDWORD(L"Software\\TortoiseSVN\\CacheType", GetSystemMetrics(SM_REMOTESESSION) ? dll : exe, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    showrecursive = CRegStdDWORD(L"Software\\TortoiseSVN\\RecursiveOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    folderoverlay = CRegStdDWORD(L"Software\\TortoiseSVN\\FolderOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveremote = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskRemote", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    drivefixed = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskFixed", TRUE);
    drivecdrom = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskCDROM", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveremove = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskRemovable", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    drivefloppy = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskFloppy", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveram = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskRAM", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveunknown = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskUnknown", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    shellmenuaccelerators = CRegStdDWORD(L"Software\\TortoiseSVN\\ShellMenuAccelerators", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    unversionedasmodified = CRegStdDWORD(L"Software\\TortoiseSVN\\UnversionedAsModified", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    ignoreoncommitignored = CRegStdDWORD(L"Software\\TortoiseSVN\\IgnoreOnCommitIgnored", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    hidemenusforunversioneditems = CRegStdDWORD(L"Software\\TortoiseSVN\\HideMenusForUnversionedItems", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    getlocktop = CRegStdDWORD(L"Software\\TortoiseSVN\\GetLockTop", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    excludedasnormal = CRegStdDWORD(L"Software\\TortoiseSVN\\ShowExcludedFoldersAsNormal", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    alwaysextended = CRegStdDWORD(L"Software\\TortoiseSVN\\AlwaysExtendedMenu", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    drivetypeticker = 0;
    menulayoutlow = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntries", MENUCHECKOUT | MENUUPDATE | MENUCOMMIT, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    menulayouthigh = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntrieshigh", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    menumasklow_lm = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskLow", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
    menumaskhigh_lm = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskHigh", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
    menumasklow_cu = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskLow", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    menumaskhigh_cu = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskHigh", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    langid = CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    blockstatus = CRegStdDWORD(L"Software\\TortoiseSVN\\BlockStatus", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    columnseverywhere = CRegStdDWORD(L"Software\\TortoiseSVN\\ColumnsEveryWhere", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    std::fill_n(drivetypecache, 27, (UINT)-1);
    if (DWORD(drivefloppy) == 0)
    {
        // A: and B: are floppy disks
        drivetypecache[0] = DRIVE_REMOVABLE;
        drivetypecache[1] = DRIVE_REMOVABLE;
    }
    TCHAR szBuffer[5] = { 0 };
    SecureZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], _countof(szDecSep));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], _countof(szThousandsSep));
    columnrevformat.lpDecimalSep = szDecSep;
    columnrevformat.lpThousandSep = szThousandsSep;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], _countof(szBuffer));
    columnrevformat.Grouping = _wtoi(szBuffer);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], _countof(szBuffer));
    columnrevformat.NegativeOrder = _wtoi(szBuffer);
    columnrevformatticker = GetTickCount64();
    nocontextpaths = CRegStdString(L"Software\\TortoiseSVN\\NoContextPaths", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    drivetypepathcache[0] = 0;
    m_critSec.Init();
    // Use RegNotifyChangeKeyValue() to get a notification event whenever a registry value
    // below HKCU\Software\TortoiseSVN is changed. If a value has changed, re-read all
    // the registry variables to ensure we use the latest ones
    RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\TortoiseSVN", 0, KEY_NOTIFY, &m_hNotifyRegKey);
    m_registryChangeEvent = CreateEvent(NULL, true, false, NULL);
    if (RegNotifyChangeKeyValue(m_hNotifyRegKey, false, REG_NOTIFY_CHANGE_LAST_SET, m_registryChangeEvent, TRUE) != ERROR_SUCCESS)
    {
        CloseHandle(m_registryChangeEvent);
        m_registryChangeEvent = NULL;
        RegCloseKey(m_hNotifyRegKey);
        m_hNotifyRegKey = 0;
    }
}

ShellCache::~ShellCache()
{
    if (m_registryChangeEvent)
        CloseHandle(m_registryChangeEvent);
    m_registryChangeEvent = NULL;
    if (m_hNotifyRegKey)
        RegCloseKey(m_hNotifyRegKey);
    m_hNotifyRegKey = 0;
}

bool ShellCache::RefreshIfNeeded()
{
    // don't wait for the registry change event but only test if such an event
    // has occurred since the last time we got here.
    // if the event has occurred, re-read all registry variables and of course
    // re-set the notification event to get further notifications of registry changes.
    bool signalled = WaitForSingleObjectEx(m_registryChangeEvent, 0, true) != WAIT_TIMEOUT;
    if (!signalled)
        return signalled;

    if (RegNotifyChangeKeyValue(m_hNotifyRegKey, false, REG_NOTIFY_CHANGE_LAST_SET, m_registryChangeEvent, TRUE) != ERROR_SUCCESS)
    {
        CloseHandle(m_registryChangeEvent);
        m_registryChangeEvent = NULL;
        RegCloseKey(m_hNotifyRegKey);
        m_hNotifyRegKey = 0;
    }

    cachetype.read();
    showrecursive.read();
    folderoverlay.read();
    driveremote.read();
    drivefixed.read();
    drivecdrom.read();
    driveremove.read();
    drivefloppy.read();
    driveram.read();
    driveunknown.read();
    shellmenuaccelerators.read();
    unversionedasmodified.read();
    ignoreoncommitignored.read();
    excludedasnormal.read();
    alwaysextended.read();
    hidemenusforunversioneditems.read();
    menulayoutlow.read();
    menulayouthigh.read();
    langid.read();
    blockstatus.read();
    columnseverywhere.read();
    getlocktop.read();
    menumasklow_lm.read();
    menumaskhigh_lm.read();
    menumasklow_cu.read();
    menumaskhigh_cu.read();
    nocontextpaths.read();

    Locker lock(m_critSec);
    pathFilter.Refresh();
    return signalled;
}

ShellCache::CacheType ShellCache::GetCacheType()
{
    RefreshIfNeeded();
    return CacheType(DWORD((cachetype)));
}

DWORD ShellCache::BlockStatus()
{
    RefreshIfNeeded();
    return (blockstatus);
}

unsigned __int64 ShellCache::GetMenuLayout()
{
    RefreshIfNeeded();
    unsigned __int64 temp = unsigned __int64(DWORD(menulayouthigh)) << 32;
    temp |= unsigned __int64(DWORD(menulayoutlow));
    return temp;
}

unsigned __int64 ShellCache::GetMenuMask()
{
    RefreshIfNeeded();
    DWORD low = (DWORD)menumasklow_lm | (DWORD)menumasklow_cu;
    DWORD high = (DWORD)menumaskhigh_lm | (DWORD)menumaskhigh_cu;
    unsigned __int64 temp = unsigned __int64(high) << 32;
    temp |= unsigned __int64(low);
    return temp;
}

BOOL ShellCache::IsRecursive()
{
    RefreshIfNeeded();
    return (showrecursive);
}

BOOL ShellCache::IsFolderOverlay()
{
    RefreshIfNeeded();
    return (folderoverlay);
}


BOOL ShellCache::HasShellMenuAccelerators()
{
    RefreshIfNeeded();
    return (shellmenuaccelerators != 0);
}

BOOL ShellCache::IsUnversionedAsModified()
{
    RefreshIfNeeded();
    return (unversionedasmodified);
}

BOOL ShellCache::IsIgnoreOnCommitIgnored()
{
    RefreshIfNeeded();
    return (ignoreoncommitignored);
}

BOOL ShellCache::IsGetLockTop()
{
    RefreshIfNeeded();
    return (getlocktop);
}

BOOL ShellCache::ShowExcludedAsNormal()
{
    RefreshIfNeeded();
    return (excludedasnormal);
}

BOOL ShellCache::AlwaysExtended()
{
    RefreshIfNeeded();
    return (alwaysextended);
}

BOOL ShellCache::HideMenusForUnversionedItems()
{
    RefreshIfNeeded();
    return (hidemenusforunversioneditems);
}

BOOL ShellCache::IsRemote()
{
    RefreshIfNeeded();
    return (driveremote);
}

BOOL ShellCache::IsFixed()
{
    RefreshIfNeeded();
    return (drivefixed);
}

BOOL ShellCache::IsCDRom()
{
    RefreshIfNeeded();
    return (drivecdrom);
}

BOOL ShellCache::IsRemovable()
{
    RefreshIfNeeded();
    return (driveremove);
}

BOOL ShellCache::IsRAM()
{
    RefreshIfNeeded();
    return (driveram);
}

BOOL ShellCache::IsUnknown()
{
    RefreshIfNeeded();
    return (driveunknown);
}

BOOL ShellCache::IsContextPathAllowed(LPCTSTR path)
{
    Locker lock(m_critSec);
    ExcludeContextValid();
    for (std::vector<tstring>::iterator I = excontextvector.begin(); I != excontextvector.end(); ++I)
    {
        if (I->empty())
            continue;
        if (!I->empty() && I->at(I->size() - 1) == '*')
        {
            tstring str = I->substr(0, I->size() - 1);
            if (_wcsnicmp(str.c_str(), path, str.size()) == 0)
                return FALSE;
        }
        else if (_wcsicmp(I->c_str(), path) == 0)
            return FALSE;
    }
    return TRUE;
}

BOOL ShellCache::IsPathAllowed(LPCTSTR path)
{
    ValidatePathFilter();
    Locker lock(m_critSec);
    svn_tristate_t allowed = pathFilter.IsPathAllowed(path);
    if (allowed != svn_tristate_unknown)
        return allowed == svn_tristate_true ? TRUE : FALSE;

    UINT drivetype = 0;
    int drivenumber = PathGetDriveNumber(path);
    if ((drivenumber >= 0) && (drivenumber < 25))
    {
        drivetype = drivetypecache[drivenumber];
        if ((drivetype == -1) || ((GetTickCount64() - drivetypeticker)>DRIVETYPETIMEOUT))
        {
            if ((DWORD(drivefloppy) == 0) && ((drivenumber == 0) || (drivenumber == 1)))
                drivetypecache[drivenumber] = DRIVE_REMOVABLE;
            else
            {
                drivetypeticker = GetTickCount64();
                TCHAR pathbuf[MAX_PATH + 4] = { 0 };      // MAX_PATH ok here. PathStripToRoot works with partial paths too.
                wcsncpy_s(pathbuf, path, _countof(pathbuf) - 1);
                PathStripToRoot(pathbuf);
                PathAddBackslash(pathbuf);
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": GetDriveType for %s, Drive %d\n", pathbuf, drivenumber);
                drivetype = GetDriveType(pathbuf);
                drivetypecache[drivenumber] = drivetype;
            }
        }
    }
    else
    {
        TCHAR pathbuf[MAX_PATH + 4] = { 0 };      // MAX_PATH ok here. PathIsUNCServer works with partial paths too.
        wcsncpy_s(pathbuf, path, _countof(pathbuf) - 1);
        if (PathIsUNCServer(pathbuf))
            drivetype = DRIVE_REMOTE;
        else
        {
            PathStripToRoot(pathbuf);
            PathAddBackslash(pathbuf);
            if (wcsncmp(pathbuf, drivetypepathcache, MAX_PATH - 1) == 0)       // MAX_PATH ok.
                drivetype = drivetypecache[26];
            else
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L"GetDriveType for %s\n", pathbuf);
                drivetype = GetDriveType(pathbuf);
                drivetypecache[26] = drivetype;
                wcsncpy_s(drivetypepathcache, pathbuf, MAX_PATH - 1);      // MAX_PATH ok.
            }
        }
    }
    if ((drivetype == DRIVE_REMOVABLE) && (!IsRemovable()))
        return FALSE;
    if ((drivetype == DRIVE_FIXED) && (!IsFixed()))
        return FALSE;
    if (((drivetype == DRIVE_REMOTE) || (drivetype == DRIVE_NO_ROOT_DIR)) && (!IsRemote()))
        return FALSE;
    if ((drivetype == DRIVE_CDROM) && (!IsCDRom()))
        return FALSE;
    if ((drivetype == DRIVE_RAMDISK) && (!IsRAM()))
        return FALSE;
    if ((drivetype == DRIVE_UNKNOWN) && (IsUnknown()))
        return FALSE;

    return TRUE;
}

DWORD ShellCache::GetLangID()
{
    RefreshIfNeeded();
    return (langid);
}

NUMBERFMT * ShellCache::GetNumberFmt()
{
    if ((GetTickCount64() - columnrevformatticker) > NUMBERFMTTIMEOUT)
    {
        TCHAR szBuffer[5] = { 0 };
        columnrevformatticker = GetTickCount64();
        SecureZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], _countof(szDecSep));
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], _countof(szThousandsSep));
        columnrevformat.lpDecimalSep = szDecSep;
        columnrevformat.lpThousandSep = szThousandsSep;
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], _countof(szBuffer));
        columnrevformat.Grouping = _wtoi(szBuffer);
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], _countof(szBuffer));
        columnrevformat.NegativeOrder = _wtoi(szBuffer);
    }
    return &columnrevformat;
}

BOOL ShellCache::IsVersioned(LPCTSTR path, bool bIsDir, bool mustbeok)
{
    tstring folder(path);
    if (!bIsDir)
    {
        size_t pos = folder.rfind('\\');
        if (pos != tstring::npos)
            folder.erase(pos);
    }
    std::map<tstring, BoolTimeout>::iterator iter;
    if ((iter = admindircache.find(folder)) != admindircache.end())
    {
        if ((GetTickCount64() - iter->second.timeout) < ADMINDIRTIMEOUT)
            return iter->second.bBool;
    }

    BoolTimeout bt;
    CTSVNPath p;
    p.SetFromWin(folder.c_str());
    bt.bBool = SVNHelper::IsVersioned(p, mustbeok);
    bt.timeout = GetTickCount64();
    Locker lock(m_critSec);
    admindircache[folder] = bt;
    return bt.bBool;
}

bool ShellCache::IsColumnsEveryWhere()
{
    RefreshIfNeeded();
    return !!(DWORD)columnseverywhere;
}

void ShellCache::ExcludeContextValid()
{
    if (RefreshIfNeeded())
    {
        Locker lock(m_critSec);
        if (excludecontextstr.compare((tstring)nocontextpaths) == 0)
            return;
        excludecontextstr = (tstring)nocontextpaths;
        excontextvector.clear();
        size_t pos = 0, pos_ant = 0;
        pos = excludecontextstr.find(L"\n", pos_ant);
        while (pos != tstring::npos)
        {
            tstring token = excludecontextstr.substr(pos_ant, pos - pos_ant);
            if (!token.empty())
                excontextvector.push_back(token);
            pos_ant = pos + 1;
            pos = excludecontextstr.find(L"\n", pos_ant);
        }
        if (!excludecontextstr.empty())
        {
            tstring token = excludecontextstr.substr(pos_ant, excludecontextstr.size() - 1);
            if (!token.empty())
                excontextvector.push_back(token);
        }
        excludecontextstr = (tstring)nocontextpaths;
    }
}

void ShellCache::ValidatePathFilter()
{
    if (RefreshIfNeeded())
    {
        Locker lock(m_critSec);
        pathFilter.Refresh();
    }
}

// construct \ref data content

void ShellCache::CPathFilter::AddEntry(const tstring& s, bool include)
{
    static wchar_t pathbuf[MAX_PATH*4] = { 0 };
    if (s.empty())
        return;

    TCHAR lastChar = *s.rbegin();

    SEntry entry;
    entry.hasSubFolderEntries = false;
    entry.recursive = lastChar != '?';
    entry.included = include ? svn_tristate_true : svn_tristate_false;
    entry.subPathIncluded = include == entry.recursive
        ? svn_tristate_true
        : svn_tristate_false;

    entry.path = s;
    if ((lastChar == '?') || (lastChar == '*'))
        entry.path.erase(s.length() - 1);
    if (!entry.path.empty() && (*entry.path.rbegin() == '\\'))
        entry.path.erase(entry.path.length() - 1);

    auto ret = ExpandEnvironmentStrings(entry.path.c_str(), pathbuf, _countof(pathbuf));
    if ((ret > 0) && (ret < _countof(pathbuf)))
        entry.path = pathbuf;
    data.push_back(entry);
}

void ShellCache::CPathFilter::AddEntries(const tstring& s, bool include)
{
    size_t pos = 0, pos_ant = 0;
    pos = s.find('\n', pos_ant);
    while (pos != tstring::npos)
    {
        AddEntry(s.substr(pos_ant, pos - pos_ant), include);
        pos_ant = pos + 1;
        pos = s.find('\n', pos_ant);
    }

    if (!s.empty())
        AddEntry(s.substr(pos_ant, s.size() - 1), include);
}

// for all paths, have at least one entry in data

void ShellCache::CPathFilter::PostProcessData()
{
    if (data.empty())
        return;

    std::sort(data.begin(), data.end());

    // update subPathIncluded props and remove duplicate entries

    TData::iterator begin = data.begin();
    TData::iterator end = data.end();
    TData::iterator dest = begin;
    for (TData::iterator source = begin; source != end; ++source)
    {
        if (_wcsicmp(source->path.c_str(), dest->path.c_str()) == 0)
        {
            // multiple entries for the same path -> merge them

            // update subPathIncluded
            // (all relevant parent info has already been normalized)

            if (!source->recursive)
                source->subPathIncluded
                = IsPathAllowed(source->path.c_str(), begin, dest);

            // multiple specs for the same path
            // -> merge them into the existing entry @ dest

            if (!source->recursive && dest->recursive)
            {
                // reset the marker for the this case

                dest->recursive = false;
                dest->included = source->included;
            }
            else
            {
                // include beats exclude

                if (source->included == svn_tristate_true)
                    dest->included = svn_tristate_true;
                if (source->recursive
                    && source->subPathIncluded == svn_tristate_true)
                {
                    dest->subPathIncluded = svn_tristate_true;
                }
            }
        }
        else
        {
            // new path -> don't merge this entry

            size_t destSize = dest->path.size();
            dest->hasSubFolderEntries
                = (source->path.size() > destSize)
                && (source->path[destSize] == '\\')
                && (_wcsnicmp(source->path.substr(0, destSize).c_str()
                              , dest->path.c_str()
                              , destSize)
                    == 0);

            *++dest = *source;

            // update subPathIncluded
            // (all relevant parent info has already been normalized)

            if (!dest->recursive)
                dest->subPathIncluded
                = IsPathAllowed(source->path.c_str(), begin, dest);
        }
    }

    // remove duplicate info
    if (begin != end)
        data.erase(++dest, end);
}

// lookup. default result is "unknown".
// We must look for *every* parent path because of situations like:
// excluded: C:, C:\some\deep\path
// include: C:\some
// lookup for C:\some\deeper\path

svn_tristate_t ShellCache::CPathFilter::IsPathAllowed
(LPCTSTR path
 , TData::const_iterator begin
 , TData::const_iterator end) const
{
    svn_tristate_t result = svn_tristate_unknown;

    // handle special cases

    if (begin == end)
        return result;

    size_t maxLength = wcslen(path);
    if (maxLength == 0)
        return result;

    // look for the most specific entry, start at the root

    size_t pos = 0;
    do
    {
        LPCTSTR backslash = wcschr(path + pos + 1, _T('\\'));
        pos = backslash == NULL ? maxLength : backslash - path;

        std::pair<LPCTSTR, size_t> toFind(path, pos);
        TData::const_iterator iter
            = std::lower_bound(begin, end, toFind);

        // found a relevant entry?

        if ((iter != end)
            && (iter->path.length() == pos)
            && (_wcsnicmp(iter->path.c_str(), path, pos) == 0))
        {
            // exact match?

            if (pos == maxLength)
                return iter->included;

            // parent match

            result = iter->subPathIncluded;

            // done?

            if (iter->hasSubFolderEntries)
                begin = iter;
            else
                return result;
        }
        else
        {
            // set a (potentially) closer lower limit

            if (iter != begin)
                begin = --iter;
        }

        // set a (potentially) closer upper limit

        end = std::upper_bound(begin, end, toFind);
    } while ((pos < maxLength) && (begin != end));

    // nothing more specific found

    return result;
}

// construction

ShellCache::CPathFilter::CPathFilter()
    : excludelist(L"Software\\TortoiseSVN\\OverlayExcludeList", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY)
    , includelist(L"Software\\TortoiseSVN\\OverlayIncludeList", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY)
{
    Refresh();
}

// notify of (potential) registry settings

void ShellCache::CPathFilter::Refresh()
{
    excludelist.read();
    includelist.read();

    if ((excludeliststr.compare((tstring)excludelist) == 0)
        && (includeliststr.compare((tstring)includelist) == 0))
    {
        return;
    }

    excludeliststr = (tstring)excludelist;
    includeliststr = (tstring)includelist;
    AddEntries(excludeliststr, false);
    AddEntries(includeliststr, true);

    PostProcessData();
}

// data access

svn_tristate_t ShellCache::CPathFilter::IsPathAllowed(LPCTSTR path) const
{
    if (path == NULL)
        return svn_tristate_unknown;
    // always ignore the recycle bin
    PTSTR pFound = StrStrI(path, L":\\RECYCLER");
    if (pFound != NULL)
    {
        if ((*(pFound + 10) == '\0') || (*(pFound + 10) == '\\'))
            return svn_tristate_false;
    }
    pFound = StrStrI(path, L":\\$Recycle.Bin");
    if (pFound != NULL)
    {
        if ((*(pFound + 14) == '\0') || (*(pFound + 14) == '\\'))
            return svn_tristate_false;
    }
    return IsPathAllowed(path, data.begin(), data.end());
}
