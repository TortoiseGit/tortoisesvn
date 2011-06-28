// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "guids.h"
#include "PreserveChdir.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "SVNStatus.h"
#include "PathUtils.h"
#include "SysInfo.h"
#include "..\TSVNCache\CacheInterface.h"


const static int ColumnFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;

// copied from ../TortoiseProc/ProjectProperties.h
#define PROJECTPROPNAME_USERFILEPROPERTY  "tsvn:userfileproperties"
#define PROJECTPROPNAME_USERDIRPROPERTY   "tsvn:userdirproperties"

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
STDMETHODIMP CShellExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
    if (psci == 0)
        return E_POINTER;

    PreserveChdir preserveChdir;
    ShellCache::CacheType cachetype = g_ShellCache.GetCacheType();
    LoadLangDll();
    switch (dwIndex)
    {
        case 0: // SVN Status
            if (cachetype == ShellCache::none)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 15, IDS_COLTITLESTATUS, IDS_COLDESCSTATUS);
            break;
        case 1: // SVN Revision
            if (cachetype == ShellCache::none)
                return S_FALSE;
            psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
            psci->scid.pid = dwIndex;
            psci->vt = VT_I4;
            psci->fmt = LVCFMT_RIGHT;
            psci->cChars = 15;
            psci->csFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT;

            MAKESTRING(IDS_COLTITLEREV);
            lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
            MAKESTRING(IDS_COLDESCREV);
            lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
            break;
        case 2: // SVN Url
            if (cachetype == ShellCache::none)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEURL, IDS_COLDESCURL);
            break;
        case 3: // SVN Short Url
            if (cachetype == ShellCache::none)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLESHORTURL, IDS_COLDESCSHORTURL);
            break;
        case 4: // Author
            if (cachetype == ShellCache::none)
                return S_FALSE;
            psci->scid.fmtid = FMTID_SummaryInformation;    // predefined FMTID
            psci->scid.pid   = PIDSI_AUTHOR;                // Predefined - author
            psci->vt         = VT_LPSTR;                    // We'll return the data as a string
            psci->fmt        = LVCFMT_LEFT;                 // Text will be left-aligned in the column
            psci->csFlags    = SHCOLSTATE_TYPE_STR;         // Data should be sorted as strings
            psci->cChars     = 32;                          // Default col width in chars
            break;
        case 5: // SVN mime-type
            if (cachetype == ShellCache::none)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEMIMETYPE, IDS_COLDESCMIMETYPE);
            break;
        case 6: // SVN Lock Owner
            if (cachetype == ShellCache::none)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEOWNER, IDS_COLDESCOWNER);
            break;
        case 7: // SVN eol-style
            if (cachetype == ShellCache::none)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEEOLSTYLE, IDS_COLDESCEOLSTYLE);
            break;
        case 8: // SVN Author
            if (cachetype == ShellCache::none)
                return S_FALSE;
            GetColumnInfo(psci, dwIndex, 30, IDS_COLTITLEAUTHOR, IDS_COLDESCAUTHOR);
            break;
        case 9: // SVN Status
            if (cachetype == ShellCache::none)
                return S_FALSE;
            psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
            psci->scid.pid = dwIndex;
            psci->vt = VT_I4;
            psci->fmt = LVCFMT_RIGHT;
            psci->cChars = 15;
            psci->csFlags = SHCOLSTATE_TYPE_INT;
            MAKESTRING(IDS_COLTITLESTATUSNUMBER);
            lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
            MAKESTRING(IDS_COLDESCSTATUS);
            lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
            break;
        default:
			// SVN custom properties
            if (dwIndex - 9 > columnuserprops.size())
                return S_FALSE;
            psci->scid.fmtid = FMTID_UserDefinedProperties;
            psci->scid.pid = dwIndex - 10;
            psci->vt = VT_BSTR;
            psci->fmt = LVCFMT_LEFT;
            psci->cChars = 30;
            psci->csFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_SECONDARYUI | SHCOLSTATE_SLOW;
            lstrcpynW(psci->wszTitle, columnuserprops.at(psci->scid.pid).first.c_str(), MAX_COLUMN_NAME_LEN);
            lstrcpynW(psci->wszDescription, columnuserprops.at(psci->scid.pid).first.c_str(), MAX_COLUMN_DESC_LEN);
            break;
    }

	return S_OK;
}

void CShellExt::GetColumnInfo(SHCOLUMNINFO* psci, DWORD dwIndex, UINT charactersCount, UINT titleId, UINT descriptionId)
{
    psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
    psci->scid.pid = dwIndex;
    psci->vt = VT_BSTR;
    psci->fmt = LVCFMT_LEFT;
    psci->cChars = charactersCount;
    psci->csFlags = ColumnFlags;

    MAKESTRING(titleId);
    lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
    MAKESTRING(descriptionId);
    lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
}

STDMETHODIMP CShellExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    if((pscid == 0) || (pscd == 0))
        return E_INVALIDARG;
    if(pvarData == 0)
        return E_POINTER;

    PreserveChdir preserveChdir;
    if (!g_ShellCache.IsPathAllowed((TCHAR *)pscd->wszFile))
    {
        return S_FALSE;
    }
    LoadLangDll();
    ShellCache::CacheType cachetype = g_ShellCache.GetCacheType();
    if (pscid->fmtid == CLSID_TortoiseSVN_UPTODATE && pscid->pid != 8)
    {
        tstring szInfo;
        const TCHAR * path = (TCHAR *)pscd->wszFile;

        // reserve for the path + trailing \0

        TCHAR buf[MAX_STATUS_STRING_LENGTH+1];
        SecureZeroMemory(buf, sizeof(buf));
        switch (pscid->pid)
        {
            case 0: // SVN Status
                GetMainColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                SVNStatus::GetStatusString(g_hResInst, filestatus, buf, _countof(buf), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                szInfo = buf;
                break;
            case 1: // SVN Revision
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                if (columnrev >= 0)
                {
                    V_VT(pvarData) = VT_I4;
                    V_I4(pvarData) = columnrev;
                }
                return S_OK;
                break;
            case 2: // SVN Url
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                szInfo = itemurl;
                break;
            case 3: // SVN Short Url
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                szInfo = itemshorturl;
                break;
            case 5: // SVN mime-type
                if (cachetype == ShellCache::none)
                    return S_FALSE;
                if (g_ShellCache.IsPathAllowed(path))
                    ExtractProperty(path, SVN_PROP_MIME_TYPE, szInfo);
                break;
            case 6: // SVN Lock Owner
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                szInfo = owner;
                break;
            case 7: // SVN eol-style
                if (cachetype == ShellCache::none)
                    return S_FALSE;
                if (g_ShellCache.IsPathAllowed(path))
                    ExtractProperty(path, SVN_PROP_EOL_STYLE, szInfo);
                break;
            case 9: // SVN Status
                GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                V_VT(pvarData) = VT_I4;
                V_I4(pvarData) = filestatus;
                return S_OK;
                break;
            default:
                return S_FALSE;
        }
        const WCHAR * wsInfo = szInfo.c_str();
        V_VT(pvarData) = VT_BSTR;
        V_BSTR(pvarData) = SysAllocString(wsInfo);
        return S_OK;
    }
    if ((pscid->fmtid == FMTID_SummaryInformation)||(pscid->pid == 8))
    {
        tstring szInfo;
        const TCHAR * path = pscd->wszFile;

        if (cachetype == ShellCache::none)
            return S_FALSE;
        switch (pscid->pid)
        {
        case PIDSI_AUTHOR:          // author
        case 8:
            GetExtraColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            szInfo = columnauthor;
            break;
        default:
            return S_FALSE;
        }
        std::wstring wsInfo = szInfo;
        V_VT(pvarData) = VT_BSTR;
        V_BSTR(pvarData) = SysAllocString(wsInfo.c_str());
        return S_OK;
    }
	if(pscid->fmtid == FMTID_UserDefinedProperties && pscid->pid < columnuserprops.size())
	{
		// SVN custom properties
        tstring szInfo;
        const TCHAR * path = (TCHAR *)pscd->wszFile;
		if (g_ShellCache.IsPathAllowed(path))
            ExtractProperty(path, columnuserprops.at(pscid->pid).second.c_str(), szInfo);
        const WCHAR * wsInfo = szInfo.c_str();
        V_VT(pvarData) = VT_BSTR;
        V_BSTR(pvarData) = SysAllocString(wsInfo);
        return S_OK;
	}

    return S_FALSE;
}

STDMETHODIMP CShellExt::Initialize(LPCSHCOLUMNINIT psci)
{
    if(psci == 0)
        return E_INVALIDARG;
    // psci->wszFolder (WCHAR) holds the path to the folder to be displayed
    // Should check to see if its a "SVN" folder and if not return E_FAIL

    PreserveChdir preserveChdir;
    if (g_ShellCache.IsColumnsEveryWhere())
        return S_OK;
    CString path = psci->wszFolder;
    if (!path.IsEmpty())
    {
        if (! g_ShellCache.IsVersioned(path, true, true))
            return E_FAIL;
    }

    maincolumnfilepath.clear();
    extracolumnfilepath.clear();

    // load SVN custom properties of the folder
    if(g_ShellCache.GetCacheType() != ShellCache::none && path != columnfolder)
    {
        columnfolder = path;
        columnuserprops.clear();

        if(g_ShellCache.IsPathAllowed(path))
        {
            // locate user directory and file properties
            CTSVNPath svnpath;
            svnpath.SetFromWin(path, true);
            std::string values;
            while(!svnpath.IsEmpty())
            {
                if(!g_ShellCache.IsVersioned(svnpath.GetWinPath(), true, true))
                    break;  // beyond root of working copy
                SVNProperties props(svnpath, false);
                for (int i=0; i<props.GetCount(); ++i)
                {
                    std::string name = props.GetItemName(i);
                    if (name.compare(PROJECTPROPNAME_USERFILEPROPERTY)==0 ||
                        name.compare(PROJECTPROPNAME_USERDIRPROPERTY)==0)
                        values.append(props.GetItemValue(i)).append("\n");
                }
                if(!values.empty())
                    break;  // user properties found
                svnpath = svnpath.GetContainingDirectory();
            }

            // parse user directory and file properties
            typedef std::set<std::string> props_type;
            props_type props;
            std::string::size_type from = values.find_first_not_of("\r\n");
            while(from != std::string::npos) {
                std::string::size_type to = values.find_first_of("\r\n", from);
                ASSERT(to != std::string::npos);    // holds as '\n' was appended last
                std::string value = values.substr(from, to - from);
                if(props.insert(value).second) {
                    // swap sequence avoiding unnecessary copies
                    columnuserprops.push_back(columnuserprop());
                    CUnicodeUtils::StdGetUnicode(value).swap(columnuserprops.back().first);
                    value.swap(columnuserprops.back().second);
                }
                from = values.find_first_not_of("\r\n", to);
            }
        }
    }

    return S_OK;
}

void CShellExt::SetExtraColumnStatus
    ( const TCHAR * path
    , const FileStatusCacheEntry * status)
{
    extracolumnfilepath = path;

    itemshorturl.clear();
    if (status)
    {
        columnauthor = CUnicodeUtils::StdGetUnicode(status->author);
        columnrev = status->rev;
        itemurl = CUnicodeUtils::StdGetUnicode(status->url);
        owner = CUnicodeUtils::StdGetUnicode(status->owner);
    }
    else
    {
        filestatus = svn_wc_status_none;
        columnauthor.clear();
        columnrev = 0;
        itemurl.clear();
        owner.clear();
        return;
    }

    TCHAR urlpath[INTERNET_MAX_URL_LENGTH+1];

    URL_COMPONENTS urlComponents;
    memset(&urlComponents, 0, sizeof(URL_COMPONENTS));
    urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    urlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
    urlComponents.lpszUrlPath = urlpath;

    if (InternetCrackUrl(itemurl.c_str(), 0, ICU_DECODE, &urlComponents))
    {
        // since the short url is shown as an additional column where the
        // file/foldername is shown too, we strip that name from the url
        // to make the url even shorter.
        TCHAR * ptr = _tcsrchr(urlComponents.lpszUrlPath, '/');
        if (ptr == NULL)
            ptr = _tcsrchr(urlComponents.lpszUrlPath, '\\');
        if (ptr)
        {
            *ptr = '\0';
            // to shorten the url even more, we check for 'trunk', 'branches' and 'tags'
            // and simply assume that these are the folders attached to the repository
            // root. If we find those, we strip the whole path before those folders too.
            // Note: this will strip too much if such a folder is *below* the repository
            // root - but it's called 'short url' and we're free to shorten it the way we
            // like :)
            ptr = _tcsstr(urlComponents.lpszUrlPath, _T("/trunk"));
            if (ptr == NULL)
                ptr = _tcsstr(urlComponents.lpszUrlPath, _T("\\trunk"));
            if ((ptr == NULL)||((*(ptr+6) != 0)&&(*(ptr+6) != '/')&&(*(ptr+6) != '\\')))
            {
                ptr = _tcsstr(urlComponents.lpszUrlPath, _T("/branches"));
                if (ptr == NULL)
                    ptr = _tcsstr(urlComponents.lpszUrlPath, _T("\\branches"));
                if ((ptr == NULL)||((*(ptr+9) != 0)&&(*(ptr+9) != '/')&&(*(ptr+9) != '\\')))
                {
                    ptr = _tcsstr(urlComponents.lpszUrlPath, _T("/tags"));
                    if (ptr == NULL)
                        ptr = _tcsstr(urlComponents.lpszUrlPath, _T("\\tags"));
                    if ((ptr)&&(*(ptr+5) != 0)&&(*(ptr+5) != '/')&&(*(ptr+5) != '\\'))
                        ptr = NULL;
                }
            }
            if (ptr)
                itemshorturl = ptr;
            else
                itemshorturl = urlComponents.lpszUrlPath;
        }
    }

    if (status)
    {
        char url[INTERNET_MAX_URL_LENGTH];
        strcpy_s(url, status->url);
        CPathUtils::Unescape(url);
        itemurl = CUnicodeUtils::StdGetUnicode(url);
    }
}

void CShellExt::GetExtraColumnStatus(const TCHAR * path, BOOL bIsDir)
{
    if (_tcscmp(path, extracolumnfilepath.c_str())==0)
        return;

    PreserveChdir preserveChdir;
    LoadLangDll();

    const FileStatusCacheEntry * status = NULL;
    AutoLocker lock(g_csGlobalCOMGuard);
    const ShellCache::CacheType cacheType = g_ShellCache.GetCacheType();

    CTSVNPath tsvnPath (path);
    switch (cacheType)
    {
    case ShellCache::exe:
        {
            TSVNCacheResponse itemStatus;
            if (m_remoteCacheLink.GetStatusFromRemoteCache(tsvnPath, &itemStatus, true))
                status = m_CachedStatus.GetFullStatus(tsvnPath, bIsDir, TRUE);
        }
        break;
    case ShellCache::dll:
        {
            status = m_CachedStatus.GetFullStatus(tsvnPath, bIsDir, TRUE);
        }
        break;
    }

    SetExtraColumnStatus (path, status);
}

void CShellExt::GetMainColumnStatus(const TCHAR * path, BOOL bIsDir)
{
    if (_tcscmp(path, maincolumnfilepath.c_str())==0)
        return;

    PreserveChdir preserveChdir;
    LoadLangDll();
    maincolumnfilepath = path;

    AutoLocker lock(g_csGlobalCOMGuard);
    const ShellCache::CacheType cacheType = g_ShellCache.GetCacheType();

    filestatus = svn_wc_status_none;
    switch (cacheType)
    {
    case ShellCache::exe:
        {
            TSVNCacheResponse itemStatus;
            if (m_remoteCacheLink.GetStatusFromRemoteCache(CTSVNPath(path), &itemStatus, true))
                filestatus = (svn_wc_status_kind)itemStatus.m_Status;
        }
        break;
    case ShellCache::dll:
        {
            const FileStatusCacheEntry * status 
                = m_CachedStatus.GetFullStatus(CTSVNPath(path), bIsDir, TRUE);

            filestatus = status->status;
            SetExtraColumnStatus (path, status);
        }
        break;
    default:
    case ShellCache::none:
        {
            if (g_ShellCache.IsVersioned(path, !!bIsDir, true))
                filestatus = svn_wc_status_normal;
            else
                filestatus = svn_wc_status_none;
        }
        break;
    }
}

void CShellExt::ExtractProperty(const TCHAR* path, const char* propertyName, tstring& to)
{
    SVNProperties props(CTSVNPath(path), false);
    for (int i=0; i<props.GetCount(); i++)
    {
        if (props.GetItemName(i).compare(propertyName)==0)
        {
            to = CUnicodeUtils::StdGetUnicode(props.GetItemValue(i));
        }
    }
}
