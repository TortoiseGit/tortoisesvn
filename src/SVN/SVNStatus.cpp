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
#pragma warning(push)
#include "svn_config.h"
#pragma warning(pop)
#include "resource.h"
#include "..\TortoiseShell\resource.h"
#include "SVNStatus.h"
#include "UnicodeUtils.h"
#include "SVNGlobal.h"
#include "SVNHelpers.h"
#include "SVNTrace.h"
#ifdef _MFC_VER
#   include "SVN.h"
#   include "registry.h"
#   include "TSVNPath.h"
#   include "PathUtils.h"
#   include "Hooks.h"
#endif

#ifdef _MFC_VER
SVNStatus::SVNStatus(bool * pbCancelled, bool suppressUI)
    : SVNBase()
    , status(NULL)
    , m_prompt (suppressUI)
#else
SVNStatus::SVNStatus(bool * pbCancelled, bool)
    : SVNBase()
    , status(NULL)
#endif
{
    m_pool = svn_pool_create (NULL);

    svn_error_clear(svn_client_create_context(&m_pctx, m_pool));

    if (pbCancelled)
    {
        m_pctx->cancel_func = cancel;
        m_pctx->cancel_baton = pbCancelled;
    }

#ifdef _MFC_VER
    svn_error_clear(svn_config_ensure(NULL, m_pool));

    // set up the configuration
    Err = svn_config_get_config (&(m_pctx->config), g_pConfigDir, m_pool);

    // set up authentication
    m_prompt.Init(m_pool, m_pctx);

    if (Err)
    {
        ShowErrorDialog(NULL);
        svn_error_clear(Err);
        svn_pool_destroy (m_pool);                  // free the allocated memory
        exit(-1);
    }

    // set up the SVN_SSH param
    CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
    if (tsvn_ssh.IsEmpty())
        tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
    tsvn_ssh.Replace('\\', '/');
    if (!tsvn_ssh.IsEmpty())
    {
        svn_config_t * cfg = (svn_config_t *)apr_hash_get ((apr_hash_t *)m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
            APR_HASH_KEY_STRING);
        svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
    }
#else
    svn_error_clear(svn_config_ensure(NULL, m_pool));

    // set up the configuration
    Err = svn_config_get_config (&(m_pctx->config), g_pConfigDir, m_pool);

#endif
}

SVNStatus::~SVNStatus(void)
{
    svn_error_clear(Err);
    svn_pool_destroy (m_pool);                  // free the allocated memory
}

void SVNStatus::ClearPool()
{
    svn_pool_clear(m_pool);
}

// static method
svn_wc_status_kind SVNStatus::GetAllStatus(const CTSVNPath& path, svn_depth_t depth)
{
    svn_client_ctx_t *          ctx;
    svn_wc_status_kind          statuskind;
    apr_pool_t *                pool;
    svn_error_t *               err;
    BOOL                        isDir;

    isDir = path.IsDirectory();

    pool = svn_pool_create (NULL);              // create the memory pool

    svn_error_clear(svn_client_create_context(&ctx, pool));

    svn_revnum_t youngest = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    statuskind = svn_wc_status_none;

    const char* svnPath = path.GetSVNApiPath(pool);
    SVNTRACE (
        err = svn_client_status5 (&youngest,
                                ctx,
                                svnPath,
                                &rev,
                                depth,
                                TRUE,           // get all
                                FALSE,          // update
                                TRUE,           // no ignore
                                FALSE,          // ignore externals
                                TRUE,           // depth as sticky
                                NULL,
                                getallstatus,
                                &statuskind,
                                pool),
        svnPath
    )

    // Error present
    if (err != NULL)
    {
        svn_error_clear(err);
        svn_pool_destroy (pool);                //free allocated memory
        return svn_wc_status_none;
    }

    svn_pool_destroy (pool);                //free allocated memory
    return statuskind;
}

// static method
svn_wc_status_kind SVNStatus::GetAllStatusRecursive(const CTSVNPath& path)
{
    return GetAllStatus(path, svn_depth_infinity);
}

// static method
svn_wc_status_kind SVNStatus::GetMoreImportant(svn_wc_status_kind status1, svn_wc_status_kind status2)
{
    if (GetStatusRanking(status1) >= GetStatusRanking(status2))
        return status1;
    return status2;
}
// static private method
int SVNStatus::GetStatusRanking(svn_wc_status_kind status)
{
    switch (status)
    {
        case svn_wc_status_none:
            return 0;
        case svn_wc_status_unversioned:
            return 1;
        case svn_wc_status_ignored:
            return 2;
        case svn_wc_status_incomplete:
            return 4;
        case svn_wc_status_normal:
        case svn_wc_status_external:
            return 5;
        case svn_wc_status_added:
            return 6;
        case svn_wc_status_missing:
            return 7;
        case svn_wc_status_deleted:
            return 8;
        case svn_wc_status_replaced:
            return 9;
        case svn_wc_status_modified:
            return 10;
        case svn_wc_status_merged:
            return 11;
        case svn_wc_status_conflicted:
            return 12;
        case svn_wc_status_obstructed:
            return 13;
    }
    return 0;
}

svn_revnum_t SVNStatus::GetStatus(const CTSVNPath& path, bool update /* = false */, bool noignore /* = false */, bool noexternals /* = false */)
{
    apr_hash_t *                statushash;
    apr_hash_t *                exthash;
    apr_array_header_t *        statusarray;
    const sort_item*            item;

    svn_error_clear(Err);
    statushash = apr_hash_make(m_pool);
    exthash = apr_hash_make(m_pool);
    svn_revnum_t youngest = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    struct hashbaton_t hashbaton;
    hashbaton.hash = statushash;
    hashbaton.exthash = exthash;
    hashbaton.pThis = this;

    const char* svnPath = path.GetSVNApiPath(m_pool);
#ifdef _MFC_VER
    if (update)
        CHooks::Instance().PreConnect(CTSVNPathList(path));
#endif
    SVNTRACE (
        Err = svn_client_status5 (&youngest,
                                m_pctx,
                                svnPath,
                                &rev,
                                svn_depth_empty,        // depth
                                TRUE,                   // get all
                                update,                 // update
                                noignore,
                                noexternals,
                                TRUE,           // depth as sticky
                                NULL,
                                getstatushash,
                                &hashbaton,
                                m_pool),
        svnPath
    );

    // Error present if function is not under version control
    if ((Err != NULL) || (apr_hash_count(statushash) == 0))
    {
        status = NULL;
        return -2;
    }

    // Convert the unordered hash to an ordered, sorted array
    statusarray = sort_hash (statushash,
                              sort_compare_items_as_paths,
                              m_pool);

    // only the first entry is needed (no recurse)
    item = &APR_ARRAY_IDX (statusarray, 0, const sort_item);

    status = (svn_client_status_t *) item->value;

    return youngest;
}
svn_client_status_t * SVNStatus::GetFirstFileStatus(const CTSVNPath& path, CTSVNPath& retPath, bool update, svn_depth_t depth, bool bNoIgnore /* = true */, bool bNoExternals /* = false */)
{
    const sort_item*            item;

    svn_error_clear(Err);
    m_statushash = apr_hash_make(m_pool);
    m_externalhash = apr_hash_make(m_pool);
    headrev = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    struct hashbaton_t hashbaton;
    hashbaton.hash = m_statushash;
    hashbaton.exthash = m_externalhash;
    hashbaton.pThis = this;
    m_statushashindex = 0;

    const char* svnPath = path.GetSVNApiPath(m_pool);
#ifdef _MFC_VER
    if (update)
        CHooks::Instance().PreConnect(CTSVNPathList(path));
#endif
    SVNTRACE (
        Err = svn_client_status5 (&headrev,
                                m_pctx,
                                svnPath,
                                &rev,
                                depth,
                                TRUE,           // get all
                                update,         // update
                                bNoIgnore,
                                bNoExternals,
                                TRUE,           // depth as sticky
                                NULL,
                                getstatushash,
                                &hashbaton,
                                m_pool),
        svnPath
    )

    // Error present if function is not under version control
    if ((Err != NULL) || (apr_hash_count(m_statushash) == 0))
    {
        return NULL;
    }

    // Convert the unordered hash to an ordered, sorted array
    m_statusarray = sort_hash (m_statushash,
                                sort_compare_items_as_paths,
                                m_pool);

    // only the first entry is needed (no recurse)
    m_statushashindex = 0;
    item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const sort_item);
    retPath.SetFromSVN((const char*)item->key);
    return (svn_client_status_t *) item->value;
}

unsigned int SVNStatus::GetVersionedCount() const
{
    unsigned int count = 0;
    const sort_item* item;
    for (unsigned int i=0; i<apr_hash_count(m_statushash); ++i)
    {
        item = &APR_ARRAY_IDX(m_statusarray, i, const sort_item);
        if (item)
        {
            if (SVNStatus::GetMoreImportant(((svn_client_status_t *)item->value)->node_status, svn_wc_status_ignored)!=svn_wc_status_ignored)
                count++;
        }
    }
    return count;
}

svn_client_status_t * SVNStatus::GetNextFileStatus(CTSVNPath& retPath)
{
    const sort_item*            item;

    if ((m_statushashindex+1) >= apr_hash_count(m_statushash))
        return NULL;
    m_statushashindex++;

    item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const sort_item);
    retPath.SetFromSVN((const char*)item->key);
    return (svn_client_status_t *) item->value;
}

bool SVNStatus::IsExternal(const CTSVNPath& path) const
{
    if (apr_hash_get(m_externalhash, path.GetSVNApiPath(m_pool), APR_HASH_KEY_STRING))
        return true;
    return false;
}

bool SVNStatus::IsInExternal(const CTSVNPath& path) const
{
    if (apr_hash_count(m_statushash) == 0)
        return false;

    SVNPool localpool(m_pool);
    apr_hash_index_t *hi;
    const char* key;
    for (hi = apr_hash_first(localpool, m_externalhash); hi; hi = apr_hash_next(hi))
    {
        apr_hash_this(hi, (const void**)&key, NULL, NULL);
        if (key)
        {
            if (CTSVNPath(CUnicodeUtils::GetUnicode(key)).IsAncestorOf(path))
                return true;
        }
    }
    return false;
}

void SVNStatus::GetExternals(std::set<CTSVNPath>& externals) const
{
    if (apr_hash_count(m_statushash) == 0)
        return;

    SVNPool localpool(m_pool);
    apr_hash_index_t *hi;
    const char* key;
    for (hi = apr_hash_first(localpool, m_externalhash); hi; hi = apr_hash_next(hi))
    {
        apr_hash_this(hi, (const void**)&key, NULL, NULL);
        if (key)
        {
            externals.insert(CTSVNPath(CUnicodeUtils::GetUnicode(key)));
        }
    }
}

void SVNStatus::GetStatusString(svn_wc_status_kind status, size_t buflen, TCHAR * string)
{
    TCHAR * buf;
    switch (status)
    {
        case svn_wc_status_none:
            buf = _T("none\0");
            break;
        case svn_wc_status_unversioned:
            buf = _T("unversioned\0");
            break;
        case svn_wc_status_normal:
            buf = _T("normal\0");
            break;
        case svn_wc_status_added:
            buf = _T("added\0");
            break;
        case svn_wc_status_missing:
            buf = _T("missing\0");
            break;
        case svn_wc_status_deleted:
            buf = _T("deleted\0");
            break;
        case svn_wc_status_replaced:
            buf = _T("replaced\0");
            break;
        case svn_wc_status_modified:
            buf = _T("modified\0");
            break;
        case svn_wc_status_merged:
            buf = _T("merged\0");
            break;
        case svn_wc_status_conflicted:
            buf = _T("conflicted\0");
            break;
        case svn_wc_status_obstructed:
            buf = _T("obstructed\0");
            break;
        case svn_wc_status_ignored:
            buf = _T("ignored");
            break;
        case svn_wc_status_external:
            buf = _T("external");
            break;
        case svn_wc_status_incomplete:
            buf = _T("incomplete\0");
            break;
        default:
            buf = _T("\0");
            break;
    }
    _stprintf_s(string, buflen, _T("%s"), buf);
}

void SVNStatus::GetStatusString(HINSTANCE hInst, svn_wc_status_kind status, TCHAR * string, int size, WORD lang)
{
    enum {MAX_STATUS_LENGTH = 240};

    struct SCacheEntry
    {
        TCHAR buffer[MAX_STATUS_LENGTH];

        HINSTANCE hInst;
        svn_wc_status_kind status;
        WORD lang;
    };

    static std::vector<SCacheEntry> cache;
    for (size_t count = cache.size(), i = 0; i < count; ++i)
    {
        const SCacheEntry& entry = cache[i];
        if (   (entry.status == status)
            && (entry.hInst == hInst)
            && (entry.lang == lang))
        {
            wcsncpy_s ( string
                      , size
                      , entry.buffer
                      , min (size, MAX_STATUS_LENGTH)-1);
            return;
        }
    }

    cache.push_back (SCacheEntry());
    SCacheEntry& entry = cache.back();
    entry.status = status;
    entry.hInst = hInst;
    entry.lang = lang;

    switch (status)
    {
        case svn_wc_status_none:
            LoadStringEx(hInst, IDS_STATUSNONE, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_unversioned:
            LoadStringEx(hInst, IDS_STATUSUNVERSIONED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_normal:
            LoadStringEx(hInst, IDS_STATUSNORMAL, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_added:
            LoadStringEx(hInst, IDS_STATUSADDED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_missing:
            LoadStringEx(hInst, IDS_STATUSABSENT, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_deleted:
            LoadStringEx(hInst, IDS_STATUSDELETED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_replaced:
            LoadStringEx(hInst, IDS_STATUSREPLACED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_modified:
            LoadStringEx(hInst, IDS_STATUSMODIFIED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_merged:
            LoadStringEx(hInst, IDS_STATUSMERGED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_conflicted:
            LoadStringEx(hInst, IDS_STATUSCONFLICTED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_ignored:
            LoadStringEx(hInst, IDS_STATUSIGNORED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_obstructed:
            LoadStringEx(hInst, IDS_STATUSOBSTRUCTED, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_external:
            LoadStringEx(hInst, IDS_STATUSEXTERNAL, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        case svn_wc_status_incomplete:
            LoadStringEx(hInst, IDS_STATUSINCOMPLETE, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
        default:
            LoadStringEx(hInst, IDS_STATUSNONE, entry.buffer, MAX_STATUS_LENGTH, lang);
            break;
    }

    wcsncpy_s (string, size, entry.buffer, min (size, MAX_STATUS_LENGTH)-1);
}

#ifdef _MFC_VER
const CString& SVNStatus::GetDepthString (svn_depth_t depth)
{
    static const CString sUnknown (MAKEINTRESOURCE(IDS_SVN_DEPTH_UNKNOWN));
    static const CString sExclude (MAKEINTRESOURCE(IDS_SVN_DEPTH_EXCLUDE));
    static const CString sEmpty (MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY));
    static const CString sFiles (MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES));
    static const CString sImmediate (MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE));
    static const CString sInfinite (MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE));
    static const CString sDefault;

    switch (depth)
    {
    case svn_depth_unknown:
        return sUnknown;
    case svn_depth_exclude:
        return sExclude;
    case svn_depth_empty:
        return sEmpty;
    case svn_depth_files:
        return sFiles;
    case svn_depth_immediates:
        return sImmediate;
    case svn_depth_infinity:
        return sInfinite;
    }

    return sDefault;
}
#endif

void SVNStatus::GetDepthString(HINSTANCE hInst, svn_depth_t depth, TCHAR * string, int size, WORD lang)
{
    switch (depth)
    {
    case svn_depth_unknown:
        LoadStringEx(hInst, IDS_SVN_DEPTH_UNKNOWN, string, size, lang);
        break;
    case svn_depth_exclude:
        LoadStringEx(hInst, IDS_SVN_DEPTH_EXCLUDE, string, size, lang);
        break;
    case svn_depth_empty:
        LoadStringEx(hInst, IDS_SVN_DEPTH_EMPTY, string, size, lang);
        break;
    case svn_depth_files:
        LoadStringEx(hInst, IDS_SVN_DEPTH_FILES, string, size, lang);
        break;
    case svn_depth_immediates:
        LoadStringEx(hInst, IDS_SVN_DEPTH_IMMEDIATE, string, size, lang);
        break;
    case svn_depth_infinity:
        LoadStringEx(hInst, IDS_SVN_DEPTH_INFINITE, string, size, lang);
        break;
    }
}


int SVNStatus::LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
    const STRINGRESOURCEIMAGE* pImage;
    const STRINGRESOURCEIMAGE* pImageEnd;
    ULONG nResourceSize;
    HGLOBAL hGlobal;
    UINT iIndex;
    int ret;

    HRSRC hResource =  FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(((uID>>4)+1)), wLanguage);
    if (!hResource)
    {
        // try the default language before giving up!
        hResource = FindResource(hInstance, MAKEINTRESOURCE(((uID>>4)+1)), RT_STRING);
        if (!hResource)
            return 0;
    }
    hGlobal = LoadResource(hInstance, hResource);
    if (!hGlobal)
        return 0;
    pImage = (const STRINGRESOURCEIMAGE*)::LockResource(hGlobal);
    if(!pImage)
        return 0;

    nResourceSize = ::SizeofResource(hInstance, hResource);
    pImageEnd = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+nResourceSize);
    iIndex = uID&0x000f;

    while ((iIndex > 0) && (pImage < pImageEnd))
    {
        pImage = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+(sizeof(STRINGRESOURCEIMAGE)+(pImage->nLength*sizeof(WCHAR))));
        iIndex--;
    }
    if (pImage >= pImageEnd)
        return 0;
    if (pImage->nLength == 0)
        return 0;

    ret = pImage->nLength;
    if (pImage->nLength > nBufferMax)
    {
        wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength-1);
        lpBuffer[nBufferMax-1] = 0;
    }
    else
    {
        wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength);
        lpBuffer[ret] = 0;
    }
    return ret;
}

svn_error_t * SVNStatus::getallstatus(void * baton, const char * /*path*/, const svn_client_status_t * status, apr_pool_t * /*pool*/)
{
    svn_wc_status_kind * s = (svn_wc_status_kind *)baton;
    *s = SVNStatus::GetMoreImportant(*s, status->node_status);
    return SVN_NO_ERROR;
}

svn_error_t * SVNStatus::getstatushash(void * baton, const char * path, const svn_client_status_t * status, apr_pool_t * /*pool*/)
{
    hashbaton_t * hash = (hashbaton_t *)baton;
    if (status->node_status == svn_wc_status_external)
    {
        apr_hash_set (hash->exthash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, (const void*)1);
        return SVN_NO_ERROR;
    }
    svn_client_status_t * statuscopy = svn_client_status_dup (status, hash->pThis->m_pool);
    apr_hash_set (hash->hash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, statuscopy);
    return SVN_NO_ERROR;
}

apr_array_header_t * SVNStatus::sort_hash (apr_hash_t *ht,
                                        int (*comparison_func) (const SVNStatus::sort_item *, const SVNStatus::sort_item *),
                                        apr_pool_t *pool)
{
    apr_hash_index_t *hi;
    apr_array_header_t *ary;

    /* allocate an array with only one element to begin with. */
    ary = apr_array_make (pool, 1, sizeof(sort_item));

    /* loop over hash table and push all keys into the array */
    for (hi = apr_hash_first (pool, ht); hi; hi = apr_hash_next (hi))
    {
        sort_item *item = (sort_item*)apr_array_push (ary);

        apr_hash_this (hi, &item->key, &item->klen, &item->value);
    }

    /* now quick sort the array.  */
    qsort (ary->elts, ary->nelts, ary->elt_size,
        (int (*)(const void *, const void *))comparison_func);

    return ary;
}

int SVNStatus::sort_compare_items_as_paths (const sort_item *a, const sort_item *b)
{
    const char *astr, *bstr;

    astr = (const char*)a->key;
    bstr = (const char*)b->key;
    return svn_path_compare_paths (astr, bstr);
}

svn_error_t* SVNStatus::cancel(void *baton)
{
    volatile bool * canceled = (bool *)baton;
    if (*canceled)
    {
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
    }
    return SVN_NO_ERROR;
}

#ifdef _MFC_VER

//// Set-up a filter to restrict the files which will have their status stored by a get-status
//void SVNStatus::SetFilter(const CTSVNPathList& fileList)
//{
//    m_filterFileList.clear();
//    for(int fileIndex = 0; fileIndex < fileList.GetCount(); fileIndex++)
//    {
//        m_filterFileList.push_back(fileList[fileIndex].GetSVNApiPath(m_pool));
//    }
//    // Sort the list so that we can do binary searches
//    std::sort(m_filterFileList.begin(), m_filterFileList.end());
//}
//
//void SVNStatus::ClearFilter()
//{
//    m_filterFileList.clear();
//}

#endif // _MFC_VER

