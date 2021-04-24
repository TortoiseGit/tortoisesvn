// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2018, 2021 - TortoiseSVN

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
#include "ShellExt.h"
#include "Guids.h"
#include "PreserveChdir.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "SVNStatus.h"
#include "PathUtils.h"
#include "../TSVNCache/CacheInterface.h"
#include "resource.h"
#include <strsafe.h>

const static int ColumnFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;

// copied from ../TortoiseProc/ProjectProperties.h
#define PROJECTPROPNAME_USERFILEPROPERTY "tsvn:userfileproperties"
#define PROJECTPROPNAME_USERDIRPROPERTY  "tsvn:userdirproperties"

// Defines that revision numbers occupy at most MAX_REV_STRING_LEN characters.
// There are Perforce repositories out there that have several 100,000 revs.
// So, don't be too restrictive by limiting this to 6 digits + 1 separator,
// for instance.
//
// Because shorter strings will be extended to have exactly MAX_REV_STRING_LEN
// characters, large numbers will produce large strings. These, in turn, will
// affect column auto sizing. This setting is a reasonable compromise.
//
// Max rev correctly sorted: 99,999,999 for MAX_REV_STRING_LEN == 10

#define MAX_REV_STRING_LEN 10

// IColumnProvider members
STDMETHODIMP CShellExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO* psci)
{
    if (psci == nullptr)
        return E_POINTER;

    PreserveChdir preserveChdir;
    auto          cachetype = g_shellCache.GetCacheType();
    LoadLangDll();
    switch (dwIndex)
    {
        case 0: // SVN Status
            if (cachetype == ShellCache::None)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 15, IDS_COLTITLESTATUS, IDS_COLDESCSTATUS);
            break;
        case 1: // SVN Revision
            if (cachetype == ShellCache::None)
                return S_FALSE;
            psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
            psci->scid.pid   = dwIndex;
            psci->vt         = VT_I4;
            psci->fmt        = LVCFMT_RIGHT;
            psci->cChars     = 15;
            psci->csFlags    = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT;

            MAKESTRING(IDS_COLTITLEREV);
            StringCchCopy(psci->wszTitle, _countof(psci->wszTitle), stringTableBuffer);
            MAKESTRING(IDS_COLDESCREV);
            StringCchCopy(psci->wszDescription, _countof(psci->wszDescription), stringTableBuffer);
            break;
        case 2: // SVN Url
            if (cachetype == ShellCache::None)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEURL, IDS_COLDESCURL);
            break;
        case 3: // SVN Short Url
            if (cachetype == ShellCache::None)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLESHORTURL, IDS_COLDESCSHORTURL);
            break;
        case 4: // Author
            if (cachetype == ShellCache::None)
                return S_FALSE;
            psci->scid.fmtid = FMTID_SummaryInformation; // predefined FMTID
            psci->scid.pid   = PIDSI_AUTHOR;             // Predefined - author
            psci->vt         = VT_LPSTR;                 // We'll return the data as a string
            psci->fmt        = LVCFMT_LEFT;              // Text will be left-aligned in the column
            psci->csFlags    = SHCOLSTATE_TYPE_STR;      // Data should be sorted as strings
            psci->cChars     = 32;                       // Default col width in chars
            break;
        case 5: // SVN mime-type
            if (cachetype == ShellCache::None)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEMIMETYPE, IDS_COLDESCMIMETYPE);
            break;
        case 6: // SVN Lock Owner
            if (cachetype == ShellCache::None)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEOWNER, IDS_COLDESCOWNER);
            break;
        case 7: // SVN eol-style
            if (cachetype == ShellCache::None)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEEOLSTYLE, IDS_COLDESCEOLSTYLE);
            break;
        case 8: // SVN Author
            if (cachetype == ShellCache::None)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEAUTHOR, IDS_COLDESCAUTHOR);
            break;
        case 9: // SVN Status
            if (cachetype == ShellCache::None)
                return S_FALSE;
            psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
            psci->scid.pid   = dwIndex;
            psci->vt         = VT_I4;
            psci->fmt        = LVCFMT_RIGHT;
            psci->cChars     = 15;
            psci->csFlags    = SHCOLSTATE_TYPE_INT;
            MAKESTRING(IDS_COLTITLESTATUSNUMBER);
            StringCchCopy(psci->wszTitle, _countof(psci->wszTitle), stringTableBuffer);
            MAKESTRING(IDS_COLDESCSTATUS);
            StringCchCopy(psci->wszDescription, _countof(psci->wszDescription), stringTableBuffer);
            break;
        default:
            // SVN custom properties
            if (dwIndex - 9 > columnUserProps.size())
                return S_FALSE;
            psci->scid.fmtid = FMTID_UserDefinedProperties;
            psci->scid.pid   = dwIndex - 10;
            psci->vt         = VT_BSTR;
            psci->fmt        = LVCFMT_LEFT;
            psci->cChars     = 30;
            psci->csFlags    = SHCOLSTATE_TYPE_STR | SHCOLSTATE_SECONDARYUI | SHCOLSTATE_SLOW;
            StringCchCopy(psci->wszTitle, _countof(psci->wszTitle), columnUserProps.at(psci->scid.pid).first.c_str());
            StringCchCopy(psci->wszDescription, _countof(psci->wszDescription), columnUserProps.at(psci->scid.pid).first.c_str());
            break;
    }

    return S_OK;
}

void CShellExt::GetColumnInfo(SHCOLUMNINFO* psci, DWORD dwIndex, UINT charactersCount, UINT titleId, UINT descriptionId)
{
    psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
    psci->scid.pid   = dwIndex;
    psci->vt         = VT_BSTR;
    psci->fmt        = LVCFMT_LEFT;
    psci->cChars     = charactersCount;
    psci->csFlags    = ColumnFlags;

    MAKESTRING(titleId);
    StringCchCopy(psci->wszTitle, _countof(psci->wszTitle), stringTableBuffer);
    MAKESTRING(descriptionId);
    StringCchCopy(psci->wszDescription, _countof(psci->wszDescription), stringTableBuffer);
}

STDMETHODIMP CShellExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
    if ((pscid == nullptr) || (pscd == nullptr))
        return E_INVALIDARG;
    if (pvarData == nullptr)
        return E_POINTER;

    PreserveChdir preserveChdir;
    if (!g_shellCache.IsPathAllowed(const_cast<wchar_t*>(pscd->wszFile)))
    {
        return S_FALSE;
    }
    LoadLangDll();
    ShellCache::CacheType cacheType = g_shellCache.GetCacheType();
    if (pscid->fmtid == CLSID_TortoiseSVN_UPTODATE && pscid->pid != 8)
    {
        std::wstring   szInfo;
        const wchar_t* path = const_cast<wchar_t*>(pscd->wszFile);

        // reserve for the path + trailing \0

        wchar_t buf[MAX_STATUS_STRING_LENGTH + 1] = {0};
        switch (pscid->pid)
        {
            case 0: // SVN Status
                GetMainColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                SVNStatus::GetStatusString(g_hResInst, fileStatus, buf, _countof(buf), static_cast<WORD>(g_shellCache.GetLangID()));
                szInfo = buf;
                break;
            case 1: // SVN Revision
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                if (columnRev >= 0)
                {
                    V_VT(pvarData) = VT_I4;
                    V_I4(pvarData) = columnRev;
                }
                return S_OK;
            case 2: // SVN Url
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                szInfo = itemUrl;
                break;
            case 3: // SVN Short Url
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                szInfo = itemShortUrl;
                break;
            case 5: // SVN mime-type
                if (cacheType == ShellCache::None)
                    return S_FALSE;
                if (g_shellCache.IsPathAllowed(path))
                    ExtractProperty(path, SVN_PROP_MIME_TYPE, szInfo);
                break;
            case 6: // SVN Lock Owner
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                szInfo = owner;
                break;
            case 7: // SVN eol-style
                if (cacheType == ShellCache::None)
                    return S_FALSE;
                if (g_shellCache.IsPathAllowed(path))
                    ExtractProperty(path, SVN_PROP_EOL_STYLE, szInfo);
                break;
            case 9: // SVN Status
                GetMainColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                V_VT(pvarData) = VT_I4;
                V_I4(pvarData) = fileStatus;
                return S_OK;
            default:
                return S_FALSE;
        }
        const wchar_t* wsInfo = szInfo.c_str();
        V_VT(pvarData)        = VT_BSTR;
        V_BSTR(pvarData)      = SysAllocString(wsInfo);
        return S_OK;
    }
    if ((pscid->fmtid == FMTID_SummaryInformation) || (pscid->pid == 8))
    {
        if (cacheType == ShellCache::None)
            return S_FALSE;

        std::wstring   szInfo;
        const wchar_t* path = pscd->wszFile;

        switch (pscid->pid)
        {
            case PIDSI_AUTHOR: // author
            case 8:
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                szInfo = columnAuthor;
                break;
            default:
                return S_FALSE;
        }
        std::wstring wsInfo = szInfo;
        V_VT(pvarData)      = VT_BSTR;
        V_BSTR(pvarData)    = SysAllocString(wsInfo.c_str());
        return S_OK;
    }
    if (pscid->fmtid == FMTID_UserDefinedProperties && pscid->pid < columnUserProps.size())
    {
        // SVN custom properties
        std::wstring szInfo;
        const auto*  path = const_cast<wchar_t*>(pscd->wszFile);
        if (g_shellCache.IsPathAllowed(path))
            ExtractProperty(path, columnUserProps.at(pscid->pid).second.c_str(), szInfo);
        const WCHAR* wsInfo = szInfo.c_str();
        V_VT(pvarData)      = VT_BSTR;
        V_BSTR(pvarData)    = SysAllocString(wsInfo);
        return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP CShellExt::Initialize(LPCSHCOLUMNINIT psci)
{
    if (psci == nullptr)
        return E_INVALIDARG;
    // psci->wszFolder (WCHAR) holds the path to the folder to be displayed
    // Should check to see if its a "SVN" folder and if not return E_FAIL

    PreserveChdir preserveChdir;
    if (g_shellCache.IsColumnsEveryWhere())
        return S_OK;
    CString path = psci->wszFolder;
    if (!path.IsEmpty())
    {
        if (!g_shellCache.IsVersioned(path, true, true))
            return E_FAIL;
    }

    mainColumnFilePath.clear();
    extraColumnFilePath.clear();

    // load SVN custom properties of the folder
    if (g_shellCache.GetCacheType() != ShellCache::None && path != columnFolder)
    {
        columnFolder = path;
        columnUserProps.clear();

        if (g_shellCache.IsPathAllowed(path))
        {
            // locate user directory and file properties
            CTSVNPath svnPath;
            svnPath.SetFromWin(path, true);
            std::string values;
            while (!svnPath.IsEmpty())
            {
                if (!g_shellCache.IsVersioned(svnPath.GetWinPath(), true, true))
                    break; // beyond root of working copy
                SVNProperties props(svnPath, false, false);
                for (int i = 0; i < props.GetCount(); ++i)
                {
                    std::string name = props.GetItemName(i);
                    if (name.compare(PROJECTPROPNAME_USERFILEPROPERTY) == 0 ||
                        name.compare(PROJECTPROPNAME_USERDIRPROPERTY) == 0)
                        values.append(props.GetItemValue(i)).append("\n");
                }
                if (!values.empty())
                    break; // user properties found
                svnPath = svnPath.GetContainingDirectory();
            }

            // parse user directory and file properties
            std::set<std::string>  props;
            std::string::size_type from = values.find_first_not_of("\r\n");
            while (from != std::string::npos)
            {
                std::string::size_type to = values.find_first_of("\r\n", from);
                ASSERT(to != std::string::npos); // holds as '\n' was appended last
                std::string value = values.substr(from, to - from);
                if (props.insert(value).second)
                {
                    // swap sequence avoiding unnecessary copies
                    columnUserProps.push_back({});
                    CUnicodeUtils::StdGetUnicode(value).swap(columnUserProps.back().first);
                    value.swap(columnUserProps.back().second);
                }
                from = values.find_first_not_of("\r\n", to);
            }
        }
    }

    return S_OK;
}

void CShellExt::SetExtraColumnStatus(const wchar_t*              path,
                                     const FileStatusCacheEntry* status)
{
    extraColumnFilePath = path;

    itemShortUrl.clear();
    if (status)
    {
        columnAuthor = CUnicodeUtils::StdGetUnicode(status->author);
        columnRev    = status->rev;
        itemUrl      = CUnicodeUtils::StdGetUnicode(status->url);
        owner        = CUnicodeUtils::StdGetUnicode(status->owner);
    }
    else
    {
        fileStatus = svn_wc_status_none;
        columnAuthor.clear();
        columnRev = 0;
        itemUrl.clear();
        owner.clear();
        return;
    }

    wchar_t urlPath[INTERNET_MAX_URL_LENGTH + 1];

    URL_COMPONENTS urlComponents  = {0};
    urlComponents.dwStructSize    = sizeof(URL_COMPONENTS);
    urlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
    urlComponents.lpszUrlPath     = urlPath;

    if (InternetCrackUrl(itemUrl.c_str(), 0, ICU_DECODE, &urlComponents))
    {
        // since the short url is shown as an additional column where the
        // file/foldername is shown too, we strip that name from the url
        // to make the url even shorter.
        wchar_t* ptr = wcsrchr(urlComponents.lpszUrlPath, '/');
        if (ptr == nullptr)
            ptr = wcsrchr(urlComponents.lpszUrlPath, '\\');
        if (ptr)
        {
            *ptr = '\0';
            // to shorten the url even more, we check for 'trunk', 'branches' and 'tags'
            // and simply assume that these are the folders attached to the repository
            // root. If we find those, we strip the whole path before those folders too.
            // Note: this will strip too much if such a folder is *below* the repository
            // root - but it's called 'short url' and we're free to shorten it the way we
            // like :)
            ptr = wcsstr(urlComponents.lpszUrlPath, L"/trunk");
            if (ptr == nullptr)
                ptr = wcsstr(urlComponents.lpszUrlPath, L"\\trunk");
            if ((ptr == nullptr) || ((*(ptr + 6) != 0) && (*(ptr + 6) != '/') && (*(ptr + 6) != '\\')))
            {
                ptr = wcsstr(urlComponents.lpszUrlPath, L"/branches");
                if (ptr == nullptr)
                    ptr = wcsstr(urlComponents.lpszUrlPath, L"\\branches");
                if ((ptr == nullptr) || ((*(ptr + 9) != 0) && (*(ptr + 9) != '/') && (*(ptr + 9) != '\\')))
                {
                    ptr = wcsstr(urlComponents.lpszUrlPath, L"/tags");
                    if (ptr == nullptr)
                        ptr = wcsstr(urlComponents.lpszUrlPath, L"\\tags");
                    if ((ptr) && (*(ptr + 5) != 0) && (*(ptr + 5) != '/') && (*(ptr + 5) != '\\'))
                        ptr = nullptr;
                }
            }
            if (ptr)
                itemShortUrl = ptr;
            else
                itemShortUrl = urlComponents.lpszUrlPath;
        }
    }

    if (status)
    {
        char url[INTERNET_MAX_URL_LENGTH] = {0};
        strcpy_s(url, status->url);
        CPathUtils::Unescape(url);
        itemUrl = CUnicodeUtils::StdGetUnicode(url);
    }
}

void CShellExt::GetExtraColumnStatus(const wchar_t* path, BOOL bIsDir)
{
    if (wcscmp(path, extraColumnFilePath.c_str()) == 0)
        return;

    PreserveChdir preserveChdir;
    LoadLangDll();

    const FileStatusCacheEntry* status = nullptr;
    AutoLocker                  lock(g_csGlobalComGuard);
    const ShellCache::CacheType cacheType = g_shellCache.GetCacheType();

    CTSVNPath tsvnPath(path);
    switch (cacheType)
    {
        case ShellCache::Exe:
        {
            TSVNCacheResponse itemStatus;
            if (m_remoteCacheLink.GetStatusFromRemoteCache(tsvnPath, &itemStatus, true))
                status = m_cachedStatus.GetFullStatus(tsvnPath, bIsDir, TRUE);
        }
        break;
        case ShellCache::Dll:
        {
            status = m_cachedStatus.GetFullStatus(tsvnPath, bIsDir, TRUE);
        }
        break;
        default:
            break;
    }

    SetExtraColumnStatus(path, status);
}

void CShellExt::GetMainColumnStatus(const wchar_t* path, BOOL bIsDir)
{
    if (wcscmp(path, mainColumnFilePath.c_str()) == 0)
        return;

    PreserveChdir preserveChdir;
    LoadLangDll();
    mainColumnFilePath = path;

    AutoLocker                  lock(g_csGlobalComGuard);
    const ShellCache::CacheType cacheType = g_shellCache.GetCacheType();

    fileStatus = svn_wc_status_none;
    switch (cacheType)
    {
        case ShellCache::Exe:
        {
            TSVNCacheResponse itemStatus;
            if (m_remoteCacheLink.GetStatusFromRemoteCache(CTSVNPath(path), &itemStatus, true))
                fileStatus = static_cast<svn_wc_status_kind>(itemStatus.m_status);
        }
        break;
        case ShellCache::Dll:
        {
            const FileStatusCacheEntry* status = m_cachedStatus.GetFullStatus(CTSVNPath(path), bIsDir, TRUE);

            fileStatus = status->status;
            SetExtraColumnStatus(path, status);
        }
        break;
        default:
        case ShellCache::None:
        {
            if (g_shellCache.IsVersioned(path, !!bIsDir, true))
                fileStatus = svn_wc_status_normal;
            else
                fileStatus = svn_wc_status_none;
        }
        break;
    }
}

void CShellExt::ExtractProperty(const wchar_t* path, const char* propertyName, std::wstring& to)
{
    SVNProperties props(CTSVNPath(path), false, false);
    for (int i = 0; i < props.GetCount(); i++)
    {
        if (props.GetItemName(i).compare(propertyName) == 0)
        {
            to = CUnicodeUtils::StdGetUnicode(props.GetItemValue(i));
        }
    }
}
