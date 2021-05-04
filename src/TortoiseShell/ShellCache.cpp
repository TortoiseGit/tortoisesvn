// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2017, 2020-2021 - TortoiseSVN

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

#include "Globals.h"
#include "SVNHelpers.h"
#include "TSVNPath.h"

ShellCache::ShellCache()
{
    cacheType                    = CRegStdDWORD(L"Software\\TortoiseSVN\\CacheType", GetSystemMetrics(SM_REMOTESESSION) ? CacheType::Dll : CacheType::Exe, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    onlyNonElevated              = CRegStdDWORD(L"Software\\TortoiseSVN\\ShowOverlaysOnlyNonElevated", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    showRecursive                = CRegStdDWORD(L"Software\\TortoiseSVN\\RecursiveOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    folderOverlay                = CRegStdDWORD(L"Software\\TortoiseSVN\\FolderOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveRemote                  = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskRemote", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveFixed                   = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskFixed", TRUE);
    driveCdRom                   = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskCDROM", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveRemove                  = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskRemovable", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveFloppy                  = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskFloppy", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveRam                     = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskRAM", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveUnknown                 = CRegStdDWORD(L"Software\\TortoiseSVN\\DriveMaskUnknown", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    shellMenuAccelerators        = CRegStdDWORD(L"Software\\TortoiseSVN\\ShellMenuAccelerators", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    unversionedAsModified        = CRegStdDWORD(L"Software\\TortoiseSVN\\UnversionedAsModified", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    ignoreOnCommitIgnored        = CRegStdDWORD(L"Software\\TortoiseSVN\\IgnoreOnCommitIgnored", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    hideMenusForUnversionedItems = CRegStdDWORD(L"Software\\TortoiseSVN\\HideMenusForUnversionedItems", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    getLockTop                   = CRegStdDWORD(L"Software\\TortoiseSVN\\GetLockTop", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    excludedAsNormal             = CRegStdDWORD(L"Software\\TortoiseSVN\\ShowExcludedFoldersAsNormal", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    alwaysExtended               = CRegStdDWORD(L"Software\\TortoiseSVN\\AlwaysExtendedMenu", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveTypeTicker              = 0;
    menuLayoutLow                = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntries", MENUCHECKOUT | MENUUPDATE | MENUCOMMIT, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    menuLayoutHigh               = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntrieshigh", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    menuMaskLowLm                = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskLow", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
    menuMaskHighLm               = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskHigh", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
    menuMaskLowCu                = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskLow", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    menuMaskHighCu               = CRegStdDWORD(L"Software\\TortoiseSVN\\ContextMenuEntriesMaskHigh", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    langId                       = CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    blockStatus                  = CRegStdDWORD(L"Software\\TortoiseSVN\\BlockStatus", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    columnsEverywhere            = CRegStdDWORD(L"Software\\TortoiseSVN\\ColumnsEveryWhere", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    std::fill_n(driveTypeCache, 27, static_cast<UINT>(-1));
    if (static_cast<DWORD>(driveFloppy) == 0)
    {
        // A: and B: are floppy disks
        driveTypeCache[0] = DRIVE_REMOVABLE;
        driveTypeCache[1] = DRIVE_REMOVABLE;
    }
    wchar_t szBuffer[5] = {0};
    SecureZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], _countof(szDecSep));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], _countof(szThousandsSep));
    columnrevformat.lpDecimalSep  = szDecSep;
    columnrevformat.lpThousandSep = szThousandsSep;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], _countof(szBuffer));
    columnrevformat.Grouping = _wtoi(szBuffer);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], _countof(szBuffer));
    columnrevformat.NegativeOrder = _wtoi(szBuffer);
    columnRevTormatTicker         = GetTickCount64();
    noContextPaths                = CRegStdString(L"Software\\TortoiseSVN\\NoContextPaths", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
    driveTypePathCache[0]         = 0;
    // Use RegNotifyChangeKeyValue() to get a notification event whenever a registry value
    // below HKCU\Software\TortoiseSVN is changed. If a value has changed, re-read all
    // the registry variables to ensure we use the latest ones
    RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\TortoiseSVN", 0, KEY_NOTIFY | KEY_WOW64_64KEY, &m_hNotifyRegKey);
    m_registryChangeEvent = CreateEvent(nullptr, true, false, nullptr);
    if (RegNotifyChangeKeyValue(m_hNotifyRegKey, false, REG_NOTIFY_CHANGE_LAST_SET, m_registryChangeEvent, TRUE) != ERROR_SUCCESS)
    {
        CloseHandle(m_registryChangeEvent);
        m_registryChangeEvent = nullptr;
        RegCloseKey(m_hNotifyRegKey);
        m_hNotifyRegKey = nullptr;
    }

    // find out if we're elevated
    isElevated    = false;
    HANDLE hToken = nullptr;
    if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION te             = {0};
        DWORD           dwReturnLength = 0;

        if (::GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &dwReturnLength))
        {
            isElevated = (te.TokenIsElevated != 0);
        }
        ::CloseHandle(hToken);
    }
}

ShellCache::~ShellCache()
{
    if (m_registryChangeEvent)
        CloseHandle(m_registryChangeEvent);
    m_registryChangeEvent = nullptr;
    if (m_hNotifyRegKey)
        RegCloseKey(m_hNotifyRegKey);
    m_hNotifyRegKey = nullptr;
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
        m_registryChangeEvent = nullptr;
        RegCloseKey(m_hNotifyRegKey);
        m_hNotifyRegKey = nullptr;
    }

    cacheType.read();
    onlyNonElevated.read();
    showRecursive.read();
    folderOverlay.read();
    driveRemote.read();
    driveFixed.read();
    driveCdRom.read();
    driveRemove.read();
    driveFloppy.read();
    driveRam.read();
    driveUnknown.read();
    shellMenuAccelerators.read();
    unversionedAsModified.read();
    ignoreOnCommitIgnored.read();
    excludedAsNormal.read();
    alwaysExtended.read();
    hideMenusForUnversionedItems.read();
    menuLayoutLow.read();
    menuLayoutHigh.read();
    langId.read();
    blockStatus.read();
    columnsEverywhere.read();
    getLockTop.read();
    menuMaskLowLm.read();
    menuMaskHighLm.read();
    menuMaskLowCu.read();
    menuMaskHighCu.read();
    noContextPaths.read();

    Locker lock(m_critSec);
    pathFilter.Refresh();
    return signalled;
}

ShellCache::CacheType ShellCache::GetCacheType()
{
    RefreshIfNeeded();
    return static_cast<CacheType>(static_cast<DWORD>(cacheType));
}

DWORD ShellCache::BlockStatus()
{
    RefreshIfNeeded();
    return (blockStatus);
}

unsigned __int64 ShellCache::GetMenuLayout()
{
    RefreshIfNeeded();
    unsigned __int64 temp = static_cast<unsigned long long>(static_cast<DWORD>(menuLayoutHigh)) << 32;
    temp |= static_cast<unsigned long long>(static_cast<DWORD>(menuLayoutLow));
    return temp;
}

unsigned __int64 ShellCache::GetMenuMask()
{
    RefreshIfNeeded();
    DWORD            low  = static_cast<DWORD>(menuMaskLowLm) | static_cast<DWORD>(menuMaskLowCu);
    DWORD            high = static_cast<DWORD>(menuMaskHighLm) | static_cast<DWORD>(menuMaskHighCu);
    unsigned __int64 temp = static_cast<unsigned long long>(high) << 32;
    temp |= static_cast<unsigned long long>(low);
    return temp;
}

bool ShellCache::IsProcessElevated() const
{
    return isElevated;
}

BOOL ShellCache::IsOnlyNonElevated()
{
    RefreshIfNeeded();
    return (onlyNonElevated);
}

BOOL ShellCache::IsRecursive()
{
    RefreshIfNeeded();
    return (showRecursive);
}

BOOL ShellCache::IsFolderOverlay()
{
    RefreshIfNeeded();
    return (folderOverlay);
}

BOOL ShellCache::HasShellMenuAccelerators()
{
    RefreshIfNeeded();
    return (shellMenuAccelerators != 0);
}

BOOL ShellCache::IsUnversionedAsModified()
{
    RefreshIfNeeded();
    return (unversionedAsModified);
}

BOOL ShellCache::IsIgnoreOnCommitIgnored()
{
    RefreshIfNeeded();
    return (ignoreOnCommitIgnored);
}

BOOL ShellCache::IsGetLockTop()
{
    RefreshIfNeeded();
    return (getLockTop);
}

BOOL ShellCache::ShowExcludedAsNormal()
{
    RefreshIfNeeded();
    return (excludedAsNormal);
}

BOOL ShellCache::AlwaysExtended()
{
    RefreshIfNeeded();
    return (alwaysExtended);
}

BOOL ShellCache::HideMenusForUnversionedItems()
{
    RefreshIfNeeded();
    return (hideMenusForUnversionedItems);
}

BOOL ShellCache::IsRemote()
{
    RefreshIfNeeded();
    return (driveRemote);
}

BOOL ShellCache::IsFixed()
{
    RefreshIfNeeded();
    return (driveFixed);
}

BOOL ShellCache::IsCDRom()
{
    RefreshIfNeeded();
    return (driveCdRom);
}

BOOL ShellCache::IsRemovable()
{
    RefreshIfNeeded();
    return (driveRemove);
}

BOOL ShellCache::IsRAM()
{
    RefreshIfNeeded();
    return (driveRam);
}

BOOL ShellCache::IsUnknown()
{
    RefreshIfNeeded();
    return (driveUnknown);
}

BOOL ShellCache::IsContextPathAllowed(LPCWSTR path)
{
    Locker lock(m_critSec);
    ExcludeContextValid();
    for (std::vector<std::wstring>::iterator I = exContextVector.begin(); I != exContextVector.end(); ++I)
    {
        if (I->empty())
            continue;
        if (!I->empty() && I->at(I->size() - 1) == '*')
        {
            std::wstring str = I->substr(0, I->size() - 1);
            if (_wcsnicmp(str.c_str(), path, str.size()) == 0)
                return FALSE;
        }
        else if (_wcsicmp(I->c_str(), path) == 0)
            return FALSE;
    }
    return TRUE;
}

BOOL ShellCache::IsPathAllowed(LPCWSTR path)
{
    ValidatePathFilter();
    Locker         lock(m_critSec);
    svn_tristate_t allowed = pathFilter.IsPathAllowed(path);
    if (allowed != svn_tristate_unknown)
        return allowed == svn_tristate_true ? TRUE : FALSE;

    UINT driveType   = 0;
    int  driveNumber = PathGetDriveNumber(path);
    if ((driveNumber >= 0) && (driveNumber < 25))
    {
        driveType = driveTypeCache[driveNumber];
        if ((driveType == -1) || ((GetTickCount64() - driveTypeTicker) > DRIVETYPETIMEOUT))
        {
            if ((static_cast<DWORD>(driveFloppy) == 0) && ((driveNumber == 0) || (driveNumber == 1)))
                driveTypeCache[driveNumber] = DRIVE_REMOVABLE;
            else
            {
                driveTypeTicker               = GetTickCount64();
                wchar_t pathbuf[MAX_PATH + 4] = {0}; // MAX_PATH ok here. PathStripToRoot works with partial paths too.
                wcsncpy_s(pathbuf, path, _countof(pathbuf) - 1);
                PathStripToRoot(pathbuf);
                PathAddBackslash(pathbuf);
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": GetdriveType for %s, Drive %d\n", pathbuf, driveNumber);
                driveType                   = GetDriveType(pathbuf);
                driveTypeCache[driveNumber] = driveType;
            }
        }
    }
    else
    {
        wchar_t pathBuf[MAX_PATH + 4] = {0}; // MAX_PATH ok here. PathIsUNCServer works with partial paths too.
        wcsncpy_s(pathBuf, path, _countof(pathBuf) - 1);
        if (PathIsUNCServer(pathBuf))
            driveType = DRIVE_REMOTE;
        PathStripToRoot(pathBuf);
        if (PathIsNetworkPath(pathBuf))
            driveType = DRIVE_REMOTE;
        else
        {
            PathAddBackslash(pathBuf);
            if (wcsncmp(pathBuf, driveTypePathCache, MAX_PATH - 1) == 0) // MAX_PATH ok.
                driveType = driveTypeCache[26];
            else
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L"GetdriveType for %s\n", pathBuf);
                driveType          = GetDriveType(pathBuf);
                driveTypeCache[26] = driveType;
                wcsncpy_s(driveTypePathCache, pathBuf, MAX_PATH - 1); // MAX_PATH ok.
            }
        }
    }
    if ((driveType == DRIVE_REMOVABLE) && (!IsRemovable()))
        return FALSE;
    if ((driveType == DRIVE_FIXED) && (!IsFixed()))
        return FALSE;
    if (((driveType == DRIVE_REMOTE) || (driveType == DRIVE_NO_ROOT_DIR)) && (!IsRemote()))
        return FALSE;
    if ((driveType == DRIVE_CDROM) && (!IsCDRom()))
        return FALSE;
    if ((driveType == DRIVE_RAMDISK) && (!IsRAM()))
        return FALSE;
    if ((driveType == DRIVE_UNKNOWN) && (IsUnknown()))
        return FALSE;

    return TRUE;
}

DWORD ShellCache::GetLangID()
{
    RefreshIfNeeded();
    return (langId);
}

NUMBERFMT* ShellCache::GetNumberFmt()
{
    if ((GetTickCount64() - columnRevTormatTicker) > NUMBERFMTTIMEOUT)
    {
        wchar_t szBuffer[5]   = {0};
        columnRevTormatTicker = GetTickCount64();
        SecureZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], _countof(szDecSep));
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], _countof(szThousandsSep));
        columnrevformat.lpDecimalSep  = szDecSep;
        columnrevformat.lpThousandSep = szThousandsSep;
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], _countof(szBuffer));
        columnrevformat.Grouping = _wtoi(szBuffer);
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], _countof(szBuffer));
        columnrevformat.NegativeOrder = _wtoi(szBuffer);
    }
    return &columnrevformat;
}

BOOL ShellCache::IsVersioned(LPCWSTR path, bool bIsDir, bool mustbeok)
{
    std::wstring folder(path);
    if (!bIsDir)
    {
        size_t pos = folder.rfind('\\');
        if (pos != std::wstring::npos)
            folder.erase(pos);
    }
    std::map<std::wstring, BoolTimeout>::iterator iter;
    if ((iter = adminDirCache.find(folder)) != adminDirCache.end())
    {
        if ((GetTickCount64() - iter->second.timeout) < ADMINDIRTIMEOUT)
            return iter->second.bBool;
    }

    BoolTimeout bt;
    CTSVNPath   p;
    p.SetFromWin(folder.c_str());
    bt.bBool   = SVNHelper::IsVersioned(p, mustbeok);
    bt.timeout = GetTickCount64();
    Locker lock(m_critSec);
    adminDirCache[folder] = bt;
    return bt.bBool;
}

bool ShellCache::IsColumnsEveryWhere()
{
    RefreshIfNeeded();
    return !!static_cast<DWORD>(columnsEverywhere);
}

void ShellCache::ExcludeContextValid()
{
    // Lock must be taken by caller, which is done by IsContextPathAllowed()
    RefreshIfNeeded();
    if (excludeContextStr.compare(static_cast<std::wstring>(noContextPaths)) == 0)
        return;

    excludeContextStr = static_cast<std::wstring>(noContextPaths);
    exContextVector.clear();
    size_t pos = 0, posAnt = 0;
    pos = excludeContextStr.find(L'\n', posAnt);
    while (pos != std::wstring::npos)
    {
        std::wstring token = excludeContextStr.substr(posAnt, pos - posAnt);
        if (!token.empty())
            exContextVector.push_back(token);
        posAnt = pos + 1;
        pos    = excludeContextStr.find(L'\n', posAnt);
    }
    if (!excludeContextStr.empty())
    {
        std::wstring token = excludeContextStr.substr(posAnt, excludeContextStr.size() - 1);
        if (!token.empty())
            exContextVector.push_back(token);
    }
}

void ShellCache::ValidatePathFilter()
{
    RefreshIfNeeded();
}

// construct \ref data content

void ShellCache::CPathFilter::AddEntry(const std::wstring& s, bool include)
{
    static wchar_t pathBuf[MAX_PATH * 4] = {0};
    if (s.empty())
        return;

    wchar_t lastChar = *s.rbegin();

    SEntry entry;
    entry.hasSubFolderEntries = false;
    entry.recursive           = lastChar != '?';
    entry.included            = include ? svn_tristate_true : svn_tristate_false;
    entry.subPathIncluded     = include == entry.recursive
                                    ? svn_tristate_true
                                    : svn_tristate_false;

    entry.path = s;
    if ((lastChar == '?') || (lastChar == '*'))
        entry.path.erase(s.length() - 1);
    if (!entry.path.empty() && (*entry.path.rbegin() == '\\'))
        entry.path.erase(entry.path.length() - 1);

    auto ret = ExpandEnvironmentStrings(entry.path.c_str(), pathBuf, _countof(pathBuf));
    if ((ret > 0) && (ret < _countof(pathBuf)))
        entry.path = pathBuf;
    data.push_back(entry);
}

void ShellCache::CPathFilter::AddEntries(const std::wstring& s, bool include)
{
    size_t pos = 0, posAnt = 0;
    pos = s.find('\n', posAnt);
    while (pos != std::wstring::npos)
    {
        AddEntry(s.substr(posAnt, pos - posAnt), include);
        posAnt = pos + 1;
        pos    = s.find('\n', posAnt);
    }

    if (!s.empty())
        AddEntry(s.substr(posAnt, s.size() - 1), include);
}

// for all paths, have at least one entry in data

void ShellCache::CPathFilter::PostProcessData()
{
    if (data.empty())
        return;

    std::sort(data.begin(), data.end());

    // update subPathIncluded props and remove duplicate entries

    auto begin = data.begin();
    auto end   = data.end();
    auto dest  = begin;
    for (auto source = begin; source != end; ++source)
    {
        if (_wcsicmp(source->path.c_str(), dest->path.c_str()) == 0)
        {
            // multiple entries for the same path -> merge them

            // update subPathIncluded
            // (all relevant parent info has already been normalized)

            if (!source->recursive)
                source->subPathIncluded = IsPathAllowed(source->path.c_str(), begin, dest);

            // multiple specs for the same path
            // -> merge them into the existing entry @ dest

            if (!source->recursive && dest->recursive)
            {
                // reset the marker for the this case

                dest->recursive = false;
                dest->included  = source->included;
            }
            else
            {
                // include beats exclude

                if (source->included == svn_tristate_true)
                    dest->included = svn_tristate_true;
                if (source->recursive && source->subPathIncluded == svn_tristate_true)
                {
                    dest->subPathIncluded = svn_tristate_true;
                }
            }
        }
        else
        {
            // new path -> don't merge this entry

            size_t destSize           = dest->path.size();
            dest->hasSubFolderEntries = (source->path.size() > destSize) && (source->path[destSize] == '\\') && (_wcsnicmp(source->path.substr(0, destSize).c_str(), dest->path.c_str(), destSize) == 0);

            *++dest = *source;

            // update subPathIncluded
            // (all relevant parent info has already been normalized)

            if (!dest->recursive)
                dest->subPathIncluded = IsPathAllowed(source->path.c_str(), begin, dest);
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

svn_tristate_t ShellCache::CPathFilter::IsPathAllowed(LPCWSTR path, std::vector<SEntry>::const_iterator begin, std::vector<SEntry>::const_iterator end) const
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
        LPCWSTR backslash = wcschr(path + pos + 1, _T('\\'));
        pos               = backslash == nullptr ? maxLength : backslash - path;

        std::pair<LPCWSTR, size_t> toFind(path, pos);
        auto                       iter = std::lower_bound(begin, end, toFind);

        // found a relevant entry?

        if ((iter != end) && (iter->path.length() == pos) && (_wcsnicmp(iter->path.c_str(), path, pos) == 0))
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

    if ((excludeliststr.compare(static_cast<std::wstring>(excludelist)) == 0) && (includeliststr.compare(static_cast<std::wstring>(includelist)) == 0))
    {
        return;
    }

    excludeliststr = static_cast<std::wstring>(excludelist);
    includeliststr = static_cast<std::wstring>(includelist);
    data.clear();
    AddEntries(excludeliststr, false);
    AddEntries(includeliststr, true);

    PostProcessData();
}

// data access

svn_tristate_t ShellCache::CPathFilter::IsPathAllowed(LPCWSTR path) const
{
    if (path == nullptr)
        return svn_tristate_unknown;
    // always ignore the recycle bin
    PTSTR pFound = StrStrI(path, L":\\RECYCLER");
    if (pFound != nullptr)
    {
        if ((*(pFound + 10) == '\0') || (*(pFound + 10) == '\\'))
            return svn_tristate_false;
    }
    pFound = StrStrI(path, L":\\$Recycle.Bin");
    if (pFound != nullptr)
    {
        if ((*(pFound + 14) == '\0') || (*(pFound + 14) == '\\'))
            return svn_tristate_false;
    }
    return IsPathAllowed(path, data.begin(), data.end());
}
