// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012, 2014-2015 - TortoiseSVN

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
#include "RepositoryInfo.h"
#include "Containers/CachedLogInfo.h"
#include "LogCacheSettings.h"

#include "svn_client.h"

#include "SVN.h"
#include "TSVNPath.h"
#include "PathUtils.h"
#include "resource.h"
#include "GoOffline.h"

#include "CriticalSection.h"

// begin namespace LogCache

namespace LogCache
{

// checking prefixes is not that simple ..

bool IsParentDirectory (const CString& parent, const CString& dir)
{
    return (parent == dir)
        || (   (dir.GetLength() > parent.GetLength())
            && (dir.GetAt (parent.GetLength()) == '/'));
}

CString UniqueFileName (const CString& fileName)
{
    CString base = fileName;
    for (int i = 0, count = base.GetLength(); i < count; ++i)
    {
        TCHAR c = base[i];
        if ((c == '?') || (c == '/') || (c == '\\') || (c == ':'))
            base.SetAt (i, '_');
    }

    int num = 0;
    CString result = base;
    while (GetFileAttributes (result) != INVALID_FILE_ATTRIBUTES)
        result.Format (L"%s(%d)", (LPCTSTR)result, ++num);

    return result.MakeLower();
}

// a lookup utility that scans an index range

CString CRepositoryInfo::CData::FindRoot
    ( TPartialIndex::const_iterator begin
    , TPartialIndex::const_iterator end
    , const CString& url) const
{
    for (TPartialIndex::const_iterator iter = begin; iter != end; ++iter)
    {
        const CString& root = iter->second->root;
        if (   (root == url.Left (root.GetLength())
            && IsParentDirectory (root, url)))
        {
            return root;
        }
    }

    return CString();
}

// construction / destruction

CRepositoryInfo::CData::CData()
{
}

CRepositoryInfo::CData::~CData()
{
    Clear();
}

// lookup (using current rules setting);
// pass empty strings for unknown values.

CString CRepositoryInfo::CData::FindRoot (const CString& uuid, const CString& url) const
{
    // lookup by UUID, if allowed
    // or if url is already a root

    CRepositoryInfo::SPerRepositoryInfo* info = Lookup (uuid, url);
    if (info != NULL)
        return info->root;

    // UUID missing?

    if (uuid.IsEmpty())
    {
        // URLs must be unique

        return CSettings::GetAllowAmbiguousURL()
            ? CString()
            : FindRoot ( urlIndex.begin()
                       , urlIndex.lower_bound (url)
                       , url);
    }
    else
    {
        // if URL is empty here, then lookup by UUID alone was not successful
        // -> it will fail here as well

        return FindRoot ( uuidIndex.lower_bound (uuid)
                        , uuidIndex.upper_bound (uuid)
                        , url);
    }
}

CRepositoryInfo::SPerRepositoryInfo*
CRepositoryInfo::CData::Lookup (const CString& uuid, const CString& root) const
{
    // the full index will only match if uuid and url are both given.
    // That repo info will be valid even if ambiguities are not allowed.

    TFullIndex::const_iterator iter
        = fullIndex.find (std::make_pair (uuid, root));
    if (iter != fullIndex.end())
        return iter->second;

    // try an URL-agnostic lookup, if that is allowed
    // (will automatically fail if the the UUID should not be given)

    if (!CSettings::GetAllowAmbiguousUUID())
    {
        TPartialIndex::const_iterator iter2 = uuidIndex.find (uuid);
        if (iter2 != uuidIndex.end())
            return iter2->second;
    }

    // try an UUID-agnostic lookup, if that is allowed
    // (will automatically fail if the the URL should not be given)

    if (!CSettings::GetAllowAmbiguousURL())
    {
        TPartialIndex::const_iterator iter2 = urlIndex.find (root);
        if (iter2 != urlIndex.end())
            return iter2->second;
    }

    // not found

    return NULL;
}

// modification

CRepositoryInfo::SPerRepositoryInfo*
CRepositoryInfo::CData::AutoInsert (const CString& uuid, const CString& root)
{
    // do we already have a suitable entry?

    CRepositoryInfo::SPerRepositoryInfo* result = Lookup (uuid, root);
    if (result != NULL)
        return result;

    // no -> add one & return it

    Add (uuid, root);
    return Lookup (uuid, root);
}

void CRepositoryInfo::CData::Add (const SPerRepositoryInfo& info)
{
    SPerRepositoryInfo* newInfo = new SPerRepositoryInfo (info);
    data.push_back (newInfo);

    urlIndex.insert (std::make_pair (newInfo->root, newInfo));
    uuidIndex.insert (std::make_pair (newInfo->uuid, newInfo));
    fullIndex.insert (std::make_pair (std::make_pair (newInfo->uuid, newInfo->root), newInfo));
}

void CRepositoryInfo::CData::Add (const CString& uuid, const CString& root)
{
    SPerRepositoryInfo info;
    info.headRevision = (revision_t)NO_REVISION;
    info.headLookupTime = -1;
    info.connectionState = online;
    info.root = root;
    info.uuid = uuid;
    info.fileName = UniqueFileName (info.root.Left (60) + info.uuid);

    Add (info);
}

void CRepositoryInfo::CData::Remove (SPerRepositoryInfo* info)
{
    // remove info from the list

    TData::iterator iter = std::find (data.begin(), data.end(), info);
    assert (iter != data.end());

    *iter = data.back();
    data.pop_back();

    // remove it from the indices

    for ( TPartialIndex::iterator iter2 = urlIndex.lower_bound (info->root)
        , end = urlIndex.upper_bound (info->root)
        ; iter2 != end
        ; ++iter2)
    {
        if (iter2->second == info)
        {
            urlIndex.erase (iter2);
            break;
        }
    }

    for ( TPartialIndex::iterator iter2 = uuidIndex.lower_bound (info->uuid)
        , end = uuidIndex.upper_bound (info->uuid)
        ; iter2 != end
        ; ++iter2)
    {
        if (iter2->second == info)
        {
            uuidIndex.erase (iter2);
            break;
        }
    }

    fullIndex.erase (std::make_pair (info->uuid, info->root));

    // finally, free the memory

    delete info;
}

// read / write file

void CRepositoryInfo::CData::Load (const CString& fileName)
{
    CFile file;
    if (!file.Open (fileName, CFile::modeRead | CFile::shareDenyWrite))
        return;

    try
    {
        CArchive stream (&file, CArchive::load);

        // format ID

        int version = 0;
        stream >> version;

        // ignore newer formats

        if (version > VERSION)
            return;

        // number of entries to read
        // (old file don't have a version info -> "version" is the count)

        int count = 0;
        if (version >= MIN_COMPATIBLE_VERSION)
            stream >> count;
        else
            count = version;

        // actually load the data

        for (int i = 0; i < count; ++i)
        {
            int connectionState = online;

            SPerRepositoryInfo info;
            stream >> info.root
                   >> info.uuid
                   >> info.headURL
                   >> info.headRevision
                   >> info.headLookupTime;

            if (version >= MIN_COMPATIBLE_VERSION)
                stream >> connectionState;

            info.connectionState = static_cast<ConnectionState>(connectionState);

            if (version >= MIN_FILENAME_VERSION)
                stream >> info.fileName;
            else
                info.fileName = info.uuid;

            // caches from 1.5.x may have a number of alias entries
            // -> use at most one

            if (   (version >= MIN_FILENAME_VERSION)
                || (uuidIndex.find (info.uuid) == uuidIndex.end()))
            {
                Add (info);
            }
        }
    }
    catch (...)
    {
        return;
    }
}

void CRepositoryInfo::CData::Save (const CString& fileName) const
{
    CFile file (fileName, CFile::modeWrite | CFile::modeCreate);
    CArchive stream (&file, CArchive::store);

    stream << static_cast<int>(VERSION);
    stream << static_cast<int>(data.size());

    for ( size_t i = 0, count = data.size(); i < count; ++i)
    {
        SPerRepositoryInfo* info = data[i];

        // temp offline -> be online the next time

        ConnectionState connectionState
            = static_cast<ConnectionState>(info->connectionState & offline);

        stream << info->root
               << info->uuid
               << info->headURL
               << info->headRevision
               << info->headLookupTime
               << connectionState
               << info->fileName;
    }
}

void CRepositoryInfo::CData::Clear()
{
    uuidIndex.clear();
    urlIndex.clear();
    fullIndex.clear();

    for (size_t i = 0, count = data.size(); i < count; ++i)
        delete data[i];

    data.clear();
}

// status info

bool CRepositoryInfo::CData::empty() const
{
    return data.empty();
}

// data access

size_t CRepositoryInfo::CData::size() const
{
    return data.size();
}

const CRepositoryInfo::SPerRepositoryInfo*
CRepositoryInfo::CData::operator[](size_t index) const
{
    return data[index];
}

// share the repository info pool thoughout this application
// (it is unique per computer anyway)

CRepositoryInfo::CData CRepositoryInfo::data;

// construct the dump file name

CString CRepositoryInfo::GetFileName() const
{
    return cacheFolder + L"Repositories.dat";
}

// used to sync access to the global "data"

async::CCriticalSection& CRepositoryInfo::GetDataMutex()
{
    static async::CCriticalSection mutex;
    return mutex;
}

// read the dump file

void CRepositoryInfo::Load()
{
    modified = false;

    // any cached info at all?

    if (GetFileAttributes (GetFileName()) == INVALID_FILE_ATTRIBUTES)
        return;

    data.Load (GetFileName());
}

// does the user want to be this repository off-line?

bool CRepositoryInfo::IsOffline (SPerRepositoryInfo* info) const
{
    // is this repository already off-line?

    if (info->connectionState != online)
        return true;

    // something went wrong.

    if ((CSettings::GetDefaultConnectionState() == online) && !svn.IsSuppressedUI())
    {
        // Default behavior is "Ask the user what to do"

        // TODO: improve the dialog with
        // * the error message (why do we think the repository is offline?)
        //   this could be shown in the dialog itself in a label, a separate popup
        //   from a "show error" button or simply a tooltip
        // * a button to retry
        //
        // for this, the IsOffline() method needs changing:
        // * requires a param for the error message (or the SVNError exception object)
        // * an int return type which tells either to cancel, go offline, retry, ...
        CGoOffline dialog;
        dialog.DoModal();
        if (dialog.asDefault)
            CSettings::SetDefaultConnectionState (dialog.selection);

        info->connectionState = dialog.selection;
        return info->connectionState != online;
    }
    else
    {
        // set default

        info->connectionState = CSettings::GetDefaultConnectionState();
        return true;
    }
}

// try to get the HEAD revision from the log cache

void CRepositoryInfo::SetHeadFromCache (SPerRepositoryInfo* info)
{
    SVN _svn;
    CCachedLogInfo* cache = _svn.GetLogCachePool()->GetCache (info->uuid, info->root);
    info->headRevision = cache != NULL
        ? cache->GetRevisions().GetLastCachedRevision()-1
        : NO_REVISION;

    // HEAD info is outdated

    info->headLookupTime = 0;
}

// construction / destruction: auto-load and save

CRepositoryInfo::CRepositoryInfo (SVN& svn, const CString& cacheFolderPath)
    : cacheFolder (cacheFolderPath)
    , modified (false)
    , svn (svn)
{
    // load the list only if the URL->UUID,head etc. mapping cache shall be used

    async::CCriticalSectionLock lock (GetDataMutex());

    if (data.empty())
        Load();
}

CRepositoryInfo::~CRepositoryInfo(void)
{
}

// look-up and ask SVN if the info is not in cache.
// cache the result.

CString CRepositoryInfo::GetRepositoryRoot (const CTSVNPath& url)
{
    CString uuid;
    return GetRepositoryRootAndUUID (url, uuid);
}

CString CRepositoryInfo::GetRepositoryUUID (const CTSVNPath& url)
{
    CString uuid;
    GetRepositoryRootAndUUID (url, uuid);
    return uuid;
}

CString CRepositoryInfo::GetRepositoryRootAndUUID ( const CTSVNPath& url
                                                  , CString& uuid)
{
    // reset (potentially) outdated info to prevent it from being
    // the basis for FindRoot / Lookup.

    uuid.Empty();

    async::CCriticalSectionLock lock (GetDataMutex());

    CString sURL = url.GetSVNPathString();
    CString root = data.FindRoot (uuid, sURL);
    SPerRepositoryInfo* info = root.IsEmpty()
                             ? NULL
                             : data.Lookup (uuid, root);

    // complete data, if lookup failed with the provided data

    if (info == NULL)
    {
        root = svn.GetRepositoryRootAndUUID (url, false, uuid);
        info = data.Lookup (uuid, root);
    }
    else
    {
        uuid = info->uuid;
    }

    // add new cache entry if none is available, yet

    if ((svn.GetSVNError() == NULL) && (info == NULL))
        data.Add (uuid, root);

    // done

    return root;
}

revision_t CRepositoryInfo::GetHeadRevision (CString uuid, const CTSVNPath& url)
{
    async::CCriticalSectionLock lock (GetDataMutex());

    // get the entry for that repository

    CString sURL = url.GetSVNPathString();
    CString root = GetRepositoryRootAndUUID (url, uuid);
    SPerRepositoryInfo* info = data.Lookup (uuid, root);

    if (info == NULL)
    {
        // there was some problem connecting to the repository

        return (revision_t)NO_REVISION;
    }

    // get time stamps and maximum head info age (default: 0 mins)

    __time64_t now = CTime::GetCurrentTime().GetTime();

    // is the current info outdated?
    // (and don't try to update the info if we are off-line,
    //  as long as we have at least *some* HEAD info).

    bool outdated = info->connectionState == online
        ? now - info->headLookupTime > CSettings::GetMaxHeadAge()
        : false;

    // is there a valid cached entry?

    if (   outdated
        || !IsParentDirectory (info->headURL, sURL)
        || (info->headRevision == NO_REVISION))
    {
        // entry outdated or for not valid for this path

        info->headLookupTime = now;
        info->headURL = sURL;
        info->headRevision = svn.GetHEADRevision (url, false);

        // if we couldn't connect to the server, ask the user

        bool cancelled = svn.GetSVNError() && (svn.GetSVNError()->apr_err == SVN_ERR_CANCELLED);
        if (   !svn.IsSuppressedUI() && !cancelled
            && (info->headRevision == NO_REVISION)
            && IsOffline (info))
        {
            // user wants to go off-line

            SetHeadFromCache (info);

            // we just ignore our latest error

            svn.ClearSVNError();
        }

        modified = true;
    }

    // ready

    return info->headRevision;
}

// make sure, we will ask the repository for the HEAD

void CRepositoryInfo::ResetHeadRevision (const CString& uuid, const CString& root)
{
    async::CCriticalSectionLock lock (GetDataMutex());

    // get the entry for that repository

    SPerRepositoryInfo* info = data.Lookup (uuid, root);
    if (info != NULL)
    {
        // there is actually a cache for this URL.
        // Invalidate the HEAD info and make sure we will
        // connect the server for an update the next time
        // we want to get connect.

        info->headLookupTime = 0;
        info->connectionState = online;
    }
}

// is the repository offline?
// Don't modify the state if autoSet is false.

bool CRepositoryInfo::IsOffline (const CString& uuid, const CString& root, bool autoSet)
{
    async::CCriticalSectionLock lock (GetDataMutex());

    // find the info

    SPerRepositoryInfo* info = data.Lookup (uuid, root);

    // no info -> assume online (i.e. just try to reach the server)

    if (info == NULL)
        return false;

    // update the online/offline state by contacting the user?
    // (the dialog will only be shown if online and no
    // offline-defaults have been set)

    if (autoSet)
        IsOffline (info);

    // return state

    return info->connectionState != online;
}

// get the connection state (uninterpreted)

ConnectionState
CRepositoryInfo::GetConnectionState (const CString& uuid, const CString& url)
{
    async::CCriticalSectionLock lock (GetDataMutex());

    // find the info

    SPerRepositoryInfo* info = data.Lookup (uuid, url);

    // no info -> assume online (i.e. just try to reach the server)

    return info == NULL
        ? online
        : info->connectionState;
}

// remove a specific entry

void CRepositoryInfo::DropEntry (CString uuid, CString url)
{
    async::CCriticalSectionLock lock (GetDataMutex());

    for ( SPerRepositoryInfo* info = data.Lookup (uuid, url)
        ; info != NULL
        ; info = data.Lookup (uuid, url))
    {
        data.Remove (info);
        modified = true;
    }
}

// write all changes to disk

void CRepositoryInfo::Flush()
{
    if (!modified)
        return;

    async::CCriticalSectionLock lock (GetDataMutex());

    CString fileName = GetFileName();
    CPathUtils::MakeSureDirectoryPathExists (fileName.Left (fileName.ReverseFind ('\\')));
    try
    {
        data.Save (fileName);
        modified = false;
    }
    catch (CException* e)
    {
        e->Delete();
    }
}

// clear cache

void CRepositoryInfo::Clear()
{
    async::CCriticalSectionLock lock (GetDataMutex());

    data.Clear();
}

// get the owning SVN instance

SVN& CRepositoryInfo::GetSVN() const
{
    return svn;
}

// access to the result of the last SVN operation

const svn_error_t* CRepositoryInfo::GetLastError() const
{
    return svn.GetSVNError();
}

// end namespace LogCache

}