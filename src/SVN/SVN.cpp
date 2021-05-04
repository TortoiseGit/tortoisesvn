// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2021 - TortoiseSVN

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
#include "SVN.h"
#include "svn_props.h"
#include "svn_sorts.h"
#include "client.h"
#include "private/svn_client_shelf.h"
#include "private/svn_client_shelf2.h"
#pragma warning(pop)

#include "TortoiseProc.h"
#include "ProgressDlg.h"
#include "UnicodeUtils.h"
#include "DirFileEnum.h"
#include "TSVNPath.h"
#include "registry.h"
#include "SVNHelpers.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "SVNAdminDir.h"
#include "SVNConfig.h"
#include "SVNError.h"
#include "SVNLogQuery.h"
#include "SVNDiffOptions.h"
#include "CacheLogQuery.h"
#include "RepositoryInfo.h"
#include "LogCacheSettings.h"
#include "CriticalSection.h"
#include "SVNTrace.h"
#include "FormatMessageWrapper.h"
#include "Hooks.h"

#ifdef _DEBUG
#    define new DEBUG_NEW
#    undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDYESTOALL 19

LCID SVN::m_sLocale          = MAKELCID(static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))), SORT_DEFAULT);
bool SVN::m_sUseSystemLocale = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\UseSystemLocaleForDates", TRUE));

/* Number of micro-seconds between the beginning of the Windows epoch
* (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970)
*/
#define APR_DELTA_EPOCH_IN_USEC APR_TIME_C(11644473600000000);

void AprTimeToFileTime(LPFILETIME pft, apr_time_t t)
{
    t += APR_DELTA_EPOCH_IN_USEC;
    LONGLONG ll         = t * 10;
    pft->dwLowDateTime  = static_cast<DWORD>(ll);
    pft->dwHighDateTime = static_cast<DWORD>(ll >> 32);
    return;
}

SVN::SVNLock::SVNLock()
    : creationDate(0)
    , expirationDate(0)
{
}

SVN::SVNProgress::SVNProgress()
    : progress(0)
    , total(0)
    , overallTotal(0)
    , bytesPerSecond(0)
{
}

SVN::SVN(bool suppressUI)
    : SVNBase()
    , m_prompt(suppressUI)
    , m_commitRev(-1)
    , m_pbCancel(nullptr)
    , m_resolveKind(static_cast<svn_wc_conflict_kind_t>(-1))
    , m_resolveResult(svn_wc_conflict_choose_postpone)
    , m_bAssumeCacheEnabled(false)
    , m_progressWnd(nullptr)
    , m_pProgressDlg(nullptr)
    , m_progressWndIsCProgress(false)
    , m_bShowProgressBar(false)
    , m_progressTotal(0)
    , m_progressAverageHelper(0)
    , m_progressLastProgress(0)
    , m_progressLastTotal(0)
    , m_progressLastTicks(0)
{
    parentPool = svn_pool_create(NULL);
    m_pool     = svn_pool_create(parentPool);

    SVNInit();
}

void SVN::SVNReInit()
{
    svn_pool_clear(m_pool);
    SVNInit();
}

void SVN::SVNInit()
{
    svn_ra_initialize(m_pool);

    svn_error_clear(svn_client_create_context2(&m_pCtx, SVNConfig::Instance().GetConfig(parentPool), m_pool));
    if (m_pCtx->config == nullptr)
    {
        svn_pool_destroy(m_pool);
        svn_pool_destroy(parentPool);
        exit(-1);
    }

    // set up authentication
    m_prompt.Init(parentPool, m_pCtx);

    m_pCtx->log_msg_func3   = svnClGetLogMessage;
    m_pCtx->log_msg_baton3  = logMessage(L"");
    m_pCtx->notify_func2    = notify;
    m_pCtx->notify_baton2   = this;
    m_pCtx->notify_func     = nullptr;
    m_pCtx->notify_baton    = nullptr;
    m_pCtx->conflict_func   = nullptr;
    m_pCtx->conflict_baton  = nullptr;
    m_pCtx->conflict_func2  = nullptr;
    m_pCtx->conflict_baton2 = this;
    m_pCtx->cancel_func     = cancel;
    m_pCtx->cancel_baton    = this;
    m_pCtx->progress_func   = progress_func;
    m_pCtx->progress_baton  = this;
    m_pCtx->client_name     = SVNHelper::GetUserAgentString(m_pool);
}

SVN::~SVN()
{
    ResetLogCachePool();

    svn_error_clear(m_err);
    svn_pool_destroy(parentPool);
}

#pragma warning(push)
#pragma warning(disable : 4100) // unreferenced formal parameter

BOOL SVN::Cancel()
{
    if (m_pbCancel)
    {
        return *m_pbCancel;
    }
    return FALSE;
}

BOOL         SVN::Notify(const CTSVNPath& path, const CTSVNPath& url, svn_wc_notify_action_t action,
                 svn_node_kind_t kind, const CString& mimeType,
                 svn_wc_notify_state_t contentState,
                 svn_wc_notify_state_t propState, svn_revnum_t rev,
                 const svn_lock_t* lock, svn_wc_notify_lock_state_t lockState,
                 const CString&     changeListName,
                 const CString&     propertyName,
                 svn_merge_range_t* range,
                 svn_error_t* err, apr_pool_t* pool) { return TRUE; };
BOOL         SVN::Log(svn_revnum_t rev, const std::string& author, const std::string& message, apr_time_t time, const MergeInfo* mergeInfo) { return TRUE; }
BOOL         SVN::BlameCallback(LONG lineNumber, bool localChange, svn_revnum_t revision,
                        const CString& author, const CString& date, svn_revnum_t mergedRevision,
                        const CString& mergedAuthor, const CString& mergedDate,
                        const CString& mergedPath, const CStringA& line,
                        const CStringA& logMsg, const CStringA& mergedLogMsg) { return TRUE; }
svn_error_t* SVN::DiffSummarizeCallback(const CTSVNPath& path, svn_client_diff_summarize_kind_t kind, bool propChanged, svn_node_kind_t node) { return nullptr; }
BOOL         SVN::ReportList(const CString& path, svn_node_kind_t kind,
                     svn_filesize_t size, bool hasProps,
                     svn_revnum_t createdRev, apr_time_t time,
                     const CString& author, const CString& lockToken,
                     const CString& lockOwner, const CString& lockComment,
                     bool isDavComment, apr_time_t lockCreationdate,
                     apr_time_t lockExpirationdate, const CString& absolutePath,
                     const CString& externalParentUrl, const CString& externalTarget) { return TRUE; }

#pragma warning(pop)

// ReSharper disable CppInconsistentNaming
struct LogMsgBaton3
{
    const char* message;          /* the message. */
    const char* message_encoding; /* the locale/encoding of the message. */
    const char* base_dir;         /* the base directory for an external edit. UTF-8! */
    const char* tmpfile_left;     /* the tmpfile left by an external edit. UTF-8! */
    apr_pool_t* pool;             /* a pool. */
};

struct shelf_status_baton
{
    const char* target_abspath;
    const char* target_path;

    int                  num_paths_shelved;
    int                  num_paths_not_shelved;
    std::vector<CString> not_shelved_paths;
    CProgressDlg*        pProgressDlg;
    svn_client_ctx_t*    ctx;
};
// ReSharper restore CppInconsistentNaming

svn_error_t* SVN::shelved_func(void* baton, const char* path, const svn_client_status_t* /*status*/, apr_pool_t* /*pool*/)
{
    shelf_status_baton* ssb = static_cast<shelf_status_baton*>(baton);
    ssb->num_paths_shelved++;
    if (ssb->pProgressDlg)
    {
        ssb->pProgressDlg->SetLine(2, CUnicodeUtils::GetUnicode(path), true);
        if (ssb->pProgressDlg->HasUserCancelled())
        {
            CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
            return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(message));
        }
    }

    return nullptr;
}

svn_error_t* SVN::not_shelved_func(void* baton, const char* path, const svn_client_status_t* /*status*/, apr_pool_t* /*pool*/)
{
    shelf_status_baton* ssb = static_cast<shelf_status_baton*>(baton);
    ssb->num_paths_not_shelved++;
    auto uPath = CUnicodeUtils::GetUnicode(path);
    ssb->not_shelved_paths.push_back(uPath);
    if (ssb->pProgressDlg)
    {
        ssb->pProgressDlg->SetLine(2, uPath, true);
        if (ssb->pProgressDlg->HasUserCancelled())
        {
            CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
            return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(message));
        }
    }
    return nullptr;
}

bool SVN::Shelve(const CString& shelveName, const CTSVNPathList& pathlist, const CString& logMsg, svn_depth_t depth, bool revert)
{
    SVNPool subPool(m_pool);

    Prepare();

    char*   pValue;
    size_t  len;
    errno_t err = _dupenv_s(&pValue, &len, "SVN_EXPERIMENTAL_COMMANDS");
    if ((err == 0) && pValue && strstr(pValue, "shelf3"))
    {
        svn_client__shelf_t* shelf = nullptr;
        // open or create the shelf
        m_err = svn_client__shelf_open_or_create(&shelf,
                                                 CUnicodeUtils::GetUTF8(shelveName),
                                                 pathlist[0].GetSVNApiPath(subPool),
                                                 m_pCtx, subPool);
        if (m_err == nullptr)
        {
            // get the last version if there is one
            svn_client__shelf_version_t* previousVersion = nullptr;
            m_err                                        = svn_client__shelf_get_newest_version(&previousVersion, shelf, subPool, subPool);
            if (m_err == nullptr)
            {
                // set up the callback baton
                shelf_status_baton sb;
                sb.target_abspath        = shelf->wc_root_abspath;
                sb.target_path           = "";
                sb.num_paths_shelved     = 0;
                sb.num_paths_not_shelved = 0;
                sb.pProgressDlg          = m_pProgressDlg;
                sb.ctx                   = m_pCtx;

                svn_client__shelf_version_t* newVersion = nullptr;
                // now create the new version of the shelf
                m_err = svn_client__shelf_save_new_version3(&newVersion, shelf,
                                                            pathlist.MakePathArray(subPool), depth, nullptr,
                                                            shelved_func, &sb,
                                                            not_shelved_func, &sb,
                                                            subPool);

                if (sb.num_paths_not_shelved > 0)
                {
                    if (m_err)
                        svn_error_clear(m_err);
                    // some paths could not be shelved, so delete the just created version
                    svn_error_clear(svn_client__shelf_delete_newer_versions(shelf, previousVersion, subPool));
                    apr_array_header_t* versionsP = nullptr;
                    svn_error_clear(svn_client__shelf_get_all_versions(&versionsP, shelf, subPool, subPool));
                    svn_error_clear(svn_client__shelf_close(shelf, subPool));
                    if (versionsP && versionsP->nelts == 0)
                    {
                        svn_error_clear(svn_client__shelf_delete(CUnicodeUtils::GetUTF8(shelveName),
                                                                 pathlist[0].GetSVNApiPath(subPool), false,
                                                                 m_pCtx, subPool));
                    }

                    // create an error with the message indicating that
                    // paths failed to be shelved, and also provide the paths that
                    // were not shelved.
                    CString sPaths;
                    for (size_t i = 0; i < sb.not_shelved_paths.size(); ++i)
                    {
                        if (i > 0)
                            sPaths += L"\n";
                        sPaths += sb.not_shelved_paths[i];
                        if ((i > 5) && (sb.not_shelved_paths.size() > 6))
                        {
                            // only provide 5-6 paths in the error message to avoid
                            // getting a too large error dialog.
                            sPaths += L"\n";
                            CString sMore;
                            sMore.Format(IDS_ERR_SHELVE_FAILED_MORE, static_cast<int>(sb.not_shelved_paths.size() - i));
                            sPaths += sMore;
                            break;
                        }
                    }
                    CString sError;
                    sError.Format(IDS_ERR_SHELVE_FAILED_PATHS, static_cast<LPCWSTR>(sPaths));
                    m_err = svn_error_create(SVN_ERR_ILLEGAL_TARGET, nullptr, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(sError)));
                }
                else if (sb.num_paths_shelved == 0 || !newVersion)
                {
                    if (m_err)
                        svn_error_clear(m_err);
                    svn_error_clear(svn_client__shelf_delete_newer_versions(shelf, previousVersion, subPool));
                    apr_array_header_t* versionsP = nullptr;
                    svn_error_clear(svn_client__shelf_get_all_versions(&versionsP, shelf, subPool, subPool));
                    m_err = svn_client__shelf_close(shelf, subPool);
                    if (versionsP && versionsP->nelts == 0)
                    {
                        svn_error_clear(svn_client__shelf_delete(CUnicodeUtils::GetUTF8(shelveName),
                                                                 pathlist[0].GetSVNApiPath(subPool), false,
                                                                 m_pCtx, subPool));
                    }
                    if (m_err == nullptr)
                        m_err = svn_error_create(SVN_ERR_ILLEGAL_TARGET, nullptr, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_ERR_SHELVE_NOTHING_WAS_SHELVED)))));
                }
                else
                {
                    if (revert)
                    {
                        // revert the shelved files so they appear as not modified
                        m_err = svn_client__shelf_unapply(newVersion, false, subPool);
                    }
                    if (m_err == nullptr)
                    {
                        svn_string_t* propVal = svn_string_create(CUnicodeUtils::GetUTF8(logMsg), subPool);

                        apr_hash_t* revpropTable = apr_hash_make(subPool);
                        apr_hash_set(revpropTable, SVN_PROP_REVISION_LOG, APR_HASH_KEY_STRING, propVal);
                        m_err = svn_client__shelf_revprop_set_all(shelf, revpropTable, subPool);
                        if (m_err == nullptr)
                        {
                            m_err = svn_client__shelf_close(shelf, subPool);
                        }
                    }
                }
            }
        }
    }
    else
    {
        svn_client__shelf2_t* shelf = nullptr;
        // open or create the shelf
        m_err = svn_client__shelf2_open_or_create(&shelf,
                                                  CUnicodeUtils::GetUTF8(shelveName),
                                                  pathlist[0].GetSVNApiPath(subPool),
                                                  m_pCtx, subPool);
        if (m_err == nullptr)
        {
            // get the last version if there is one
            svn_client__shelf2_version_t* previousVersion = nullptr;
            m_err                                         = svn_client__shelf2_get_newest_version(&previousVersion, shelf, subPool, subPool);
            if (m_err == nullptr)
            {
                // set up the callback baton
                shelf_status_baton sb;
                sb.target_abspath        = shelf->wc_root_abspath;
                sb.target_path           = "";
                sb.num_paths_shelved     = 0;
                sb.num_paths_not_shelved = 0;
                sb.pProgressDlg          = m_pProgressDlg;
                sb.ctx                   = m_pCtx;

                svn_client__shelf2_version_t* newVersion = nullptr;
                // now create the new version of the shelf
                m_err = svn_client__shelf2_save_new_version3(&newVersion, shelf,
                                                             pathlist.MakePathArray(subPool), depth, nullptr,
                                                             shelved_func, &sb,
                                                             not_shelved_func, &sb,
                                                             subPool);

                if (sb.num_paths_not_shelved > 0)
                {
                    if (m_err)
                        svn_error_clear(m_err);
                    // some paths could not be shelved, so delete the just created version
                    svn_error_clear(svn_client__shelf2_delete_newer_versions(shelf, previousVersion, subPool));
                    apr_array_header_t* versionsP = nullptr;
                    svn_error_clear(svn_client__shelf2_get_all_versions(&versionsP, shelf, subPool, subPool));
                    svn_error_clear(svn_client__shelf2_close(shelf, subPool));
                    if (versionsP && versionsP->nelts == 0)
                    {
                        svn_error_clear(svn_client__shelf2_delete(CUnicodeUtils::GetUTF8(shelveName),
                                                                  pathlist[0].GetSVNApiPath(subPool), false,
                                                                  m_pCtx, subPool));
                    }

                    // create an error with the message indicating that
                    // paths failed to be shelved, and also provide the paths that
                    // were not shelved.
                    CString sPaths;
                    for (size_t i = 0; i < sb.not_shelved_paths.size(); ++i)
                    {
                        if (i > 0)
                            sPaths += L"\n";
                        sPaths += sb.not_shelved_paths[i];
                        if ((i > 5) && (sb.not_shelved_paths.size() > 6))
                        {
                            // only provide 5-6 paths in the error message to avoid
                            // getting a too large error dialog.
                            sPaths += L"\n";
                            CString sMore;
                            sMore.Format(IDS_ERR_SHELVE_FAILED_MORE, static_cast<int>(sb.not_shelved_paths.size() - i));
                            sPaths += sMore;
                            break;
                        }
                    }
                    CString sError;
                    sError.Format(IDS_ERR_SHELVE_FAILED_PATHS, static_cast<LPCWSTR>(sPaths));
                    m_err = svn_error_create(SVN_ERR_ILLEGAL_TARGET, nullptr, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(sError)));
                }
                else if (sb.num_paths_shelved == 0 || !newVersion)
                {
                    if (m_err)
                        svn_error_clear(m_err);
                    svn_error_clear(svn_client__shelf2_delete_newer_versions(shelf, previousVersion, subPool));
                    apr_array_header_t* versionsP = nullptr;
                    svn_error_clear(svn_client__shelf2_get_all_versions(&versionsP, shelf, subPool, subPool));
                    m_err = svn_client__shelf2_close(shelf, subPool);
                    if (versionsP && versionsP->nelts == 0)
                    {
                        svn_error_clear(svn_client__shelf2_delete(CUnicodeUtils::GetUTF8(shelveName),
                                                                  pathlist[0].GetSVNApiPath(subPool), false,
                                                                  m_pCtx, subPool));
                    }
                    if (m_err == nullptr)
                        m_err = svn_error_create(SVN_ERR_ILLEGAL_TARGET, nullptr, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_ERR_SHELVE_NOTHING_WAS_SHELVED)))));
                }
                else
                {
                    if (revert)
                    {
                        // revert the shelved files so they appear as not modified
                        m_err = svn_client__shelf2_unapply(newVersion, false, subPool);
                    }
                    if (m_err == nullptr)
                    {
                        svn_string_t* propVal = svn_string_create(CUnicodeUtils::GetUTF8(logMsg), subPool);

                        apr_hash_t* revpropTable = apr_hash_make(subPool);
                        apr_hash_set(revpropTable, SVN_PROP_REVISION_LOG, APR_HASH_KEY_STRING, propVal);
                        m_err = svn_client__shelf2_revprop_set_all(shelf, revpropTable, subPool);
                        if (m_err == nullptr)
                        {
                            m_err = svn_client__shelf2_close(shelf, subPool);
                        }
                    }
                }
            }
        }
    }
    free(pValue);

    return (m_err == nullptr);
}

bool SVN::UnShelve(const CString& shelveName, int version, const CTSVNPath& localAbspath)
{
    SVNPool subPool(m_pool);

    Prepare();

    char*   pValue;
    size_t  len;
    errno_t err = _dupenv_s(&pValue, &len, "SVN_EXPERIMENTAL_COMMANDS");
    if ((err == 0) && pValue && strstr(pValue, "shelf3"))
    {
        svn_client__shelf_t* shelf = nullptr;

        m_err = svn_client__shelf_open_existing(&shelf,
                                                CUnicodeUtils::GetUTF8(shelveName),
                                                localAbspath.GetSVNApiPath(subPool), m_pCtx, subPool);
        if (m_err == nullptr)
        {
            svn_client__shelf_version_t* shelfVersion = nullptr;
            if (version > 0)
                m_err = svn_client__shelf_version_open(&shelfVersion, shelf, version, subPool, subPool);
            else
                m_err = svn_client__shelf_get_newest_version(&shelfVersion, shelf, subPool, subPool);
            if (m_err == nullptr)
            {
                m_err = svn_client__shelf_apply(shelfVersion, false, subPool);
                if (m_err == nullptr)
                {
                    svn_error_clear(svn_client__shelf_delete_newer_versions(shelf, shelfVersion, subPool));

                    m_err = svn_client__shelf_close(shelf, subPool);
                }
            }
        }
    }
    else
    {
        svn_client__shelf2_t* shelf = nullptr;

        m_err = svn_client__shelf2_open_existing(&shelf,
                                                 CUnicodeUtils::GetUTF8(shelveName),
                                                 localAbspath.GetSVNApiPath(subPool), m_pCtx, subPool);
        if (m_err == nullptr)
        {
            svn_client__shelf2_version_t* shelfVersion = nullptr;
            if (version > 0)
                m_err = svn_client__shelf2_version_open(&shelfVersion, shelf, version, subPool, subPool);
            else
                m_err = svn_client__shelf2_get_newest_version(&shelfVersion, shelf, subPool, subPool);
            if (m_err == nullptr)
            {
                m_err = svn_client__shelf2_apply(shelfVersion, false, subPool);
                if (m_err == nullptr)
                {
                    svn_error_clear(svn_client__shelf2_delete_newer_versions(shelf, shelfVersion, subPool));

                    m_err = svn_client__shelf2_close(shelf, subPool);
                }
            }
        }
    }
    free(pValue);
    return (m_err == nullptr);
}

ShelfInfo SVN::GetShelfInfo(const CString& shelfName, const CTSVNPath& localAbspath)
{
    SVNPool subPool(m_pool);

    Prepare();

    ShelfInfo info;

    char*   pValue = nullptr;
    size_t  len    = 0;
    errno_t err    = _dupenv_s(&pValue, &len, "SVN_EXPERIMENTAL_COMMANDS");
    if ((err == 0) && pValue && strstr(pValue, "shelf3"))
    {
        svn_client__shelf_t* shelf = nullptr;
        m_err                      = svn_client__shelf_open_existing(&shelf,
                                                CUnicodeUtils::GetUTF8(shelfName),
                                                localAbspath.GetSVNApiPath(subPool), m_pCtx, subPool);
        if (m_err == nullptr)
        {
            info.name    = shelfName;
            char* logMsg = nullptr;
            m_err        = svn_client__shelf_get_log_message(&logMsg, shelf, subPool);
            if (m_err == nullptr)
            {
                info.logMessage = CUnicodeUtils::GetUnicode(logMsg);
            }
            apr_array_header_t* versionsP = nullptr;
            m_err                         = svn_client__shelf_get_all_versions(&versionsP, shelf, subPool, subPool);
            if (m_err == nullptr)
            {
                for (int i = 0; i < versionsP->nelts; i++)
                {
                    svn_client__shelf_version_t* shelfVersion = static_cast<svn_client__shelf_version_t*>(APR_ARRAY_IDX(versionsP, i, void*));

                    CTSVNPathList changedFiles;
                    apr_hash_t*   affectedPaths = nullptr;
                    m_err                       = svn_client__shelf_paths_changed(&affectedPaths, shelfVersion, subPool, subPool);
                    if (m_err == nullptr)
                    {
                        for (apr_hash_index_t* hi = apr_hash_first(subPool, affectedPaths); hi; hi = apr_hash_next(hi))
                        {
                            const char* path = static_cast<const char*>(apr_hash_this_key(hi));
                            changedFiles.AddPath(CTSVNPath(CUnicodeUtils::GetUnicode(path)));
                        }
                    }
                    info.versions.push_back(std::make_tuple(shelfVersion->mtime, changedFiles));
                }
            }
            m_err = svn_client__shelf_close(shelf, subPool);
        }
    }
    else
    {
        svn_client__shelf2_t* shelf = nullptr;
        m_err                       = svn_client__shelf2_open_existing(&shelf,
                                                 CUnicodeUtils::GetUTF8(shelfName),
                                                 localAbspath.GetSVNApiPath(subPool), m_pCtx, subPool);
        if (m_err == nullptr)
        {
            info.name    = shelfName;
            char* logMsg = nullptr;
            m_err        = svn_client__shelf2_get_log_message(&logMsg, shelf, subPool);
            if (m_err == nullptr)
            {
                info.logMessage = CUnicodeUtils::GetUnicode(logMsg);
            }
            apr_array_header_t* versionsP = nullptr;
            m_err                         = svn_client__shelf2_get_all_versions(&versionsP, shelf, subPool, subPool);
            if (m_err == nullptr)
            {
                for (int i = 0; i < versionsP->nelts; i++)
                {
                    svn_client__shelf2_version_t* shelfVersion = static_cast<svn_client__shelf2_version_t*>(APR_ARRAY_IDX(versionsP, i, void*));

                    CTSVNPathList changedFiles;
                    apr_hash_t*   affectedPaths = nullptr;
                    m_err                       = svn_client__shelf2_paths_changed(&affectedPaths, shelfVersion, subPool, subPool);
                    if (m_err == nullptr)
                    {
                        for (apr_hash_index_t* hi = apr_hash_first(subPool, affectedPaths); hi; hi = apr_hash_next(hi))
                        {
                            const char* path = static_cast<const char*>(apr_hash_this_key(hi));
                            changedFiles.AddPath(CTSVNPath(CUnicodeUtils::GetUnicode(path)));
                        }
                    }
                    info.versions.push_back(std::make_tuple(shelfVersion->mtime, changedFiles));
                }
            }
            m_err = svn_client__shelf2_close(shelf, subPool);
        }
    }
    free(pValue);
    return info;
}

bool SVN::ShelvesList(std::vector<CString>& names, const CTSVNPath& localAbspath)
{
    SVNPool subPool(m_pool);

    Prepare();
    apr_hash_t* namesHash = nullptr;

    char*   pValue = nullptr;
    size_t  len    = 0;
    errno_t err    = _dupenv_s(&pValue, &len, "SVN_EXPERIMENTAL_COMMANDS");
    if ((err == 0) && pValue && strstr(pValue, "shelf3"))
    {
        SVNTRACE(
            m_err = svn_client__shelf_list(&namesHash,
                                           localAbspath.GetSVNApiPath(subPool),
                                           m_pCtx,
                                           subPool, subPool),
            NULL);
        if (m_err)
            return false;

        apr_hash_index_t* hi = nullptr;
        for (hi = apr_hash_first(subPool, namesHash); hi; hi = apr_hash_next(hi))
        {
            CString name = CUnicodeUtils::GetUnicode(static_cast<const char*>(apr_hash_this_key(hi)));
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
    }
    else
    {
        SVNTRACE(
            m_err = svn_client__shelf2_list(&namesHash,
                                            localAbspath.GetSVNApiPath(subPool),
                                            m_pCtx,
                                            subPool, subPool),
            NULL);
        if (m_err)
            return false;

        apr_hash_index_t* hi = nullptr;
        for (hi = apr_hash_first(subPool, namesHash); hi; hi = apr_hash_next(hi))
        {
            CString name = CUnicodeUtils::GetUnicode(static_cast<const char*>(apr_hash_this_key(hi)));
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
    }
    free(pValue);

    return (m_err == nullptr);
}

bool SVN::DropShelf(const CString& shelveName, const CTSVNPath& localAbspath)
{
    SVNPool subPool(m_pool);

    Prepare();
    char*   pValue = nullptr;
    size_t  len    = 0;
    errno_t err    = _dupenv_s(&pValue, &len, "SVN_EXPERIMENTAL_COMMANDS");
    if ((err == 0) && pValue && strstr(pValue, "shelf3"))
    {
        SVNTRACE(
            m_err = svn_client__shelf_delete(CUnicodeUtils::GetUTF8(shelveName),
                                             localAbspath.GetSVNApiPath(subPool),
                                             false,
                                             m_pCtx, subPool),
            NULL);
    }
    else
    {
        SVNTRACE(
            m_err = svn_client__shelf2_delete(CUnicodeUtils::GetUTF8(shelveName),
                                              localAbspath.GetSVNApiPath(subPool),
                                              false,
                                              m_pCtx, subPool),
            NULL);
    }
    free(pValue);
    return (m_err == nullptr);
}

bool SVN::Checkout(const CTSVNPath& moduleName, const CTSVNPath& destPath, const SVNRev& pegRev,
                   const SVNRev& revision, svn_depth_t depth, bool bIgnoreExternals,
                   bool bAllowUnverObstructions)
{
    SVNPool subPool(m_pool);
    Prepare();

    CHooks::Instance().PreConnect(CTSVNPathList(moduleName));
    const char* svnPath = moduleName.GetSVNApiPath(subPool);
    SVNTRACE(
        m_err = svn_client_checkout3(NULL, // we don't need the resulting revision
                                     svnPath,
                                     destPath.GetSVNApiPath(subPool),
                                     pegRev,
                                     revision,
                                     depth,
                                     bIgnoreExternals,
                                     bAllowUnverObstructions,
                                     m_pCtx,
                                     subPool),
        svnPath);
    ClearCAPIAuthCacheOnError();

    return (m_err == nullptr);
}

bool SVN::Remove(const CTSVNPathList& pathlist, bool force, bool keepLocal, const CString& message, const RevPropHash& revProps)
{
    // svn_client_delete needs to run on a sub-pool, so that after it's run, the pool
    // cleanups get run.  For example, after a failure do to an unforced delete on
    // a modified file, if you don't do a cleanup, the WC stays locked
    SVNPool subPool(m_pool);
    Prepare();
    m_pCtx->log_msg_baton3 = logMessage(message);

    apr_hash_t* revPropHash = MakeRevPropHash(revProps, subPool);

    CallPreConnectHookIfUrl(pathlist);

    SVNTRACE(
        m_err = svn_client_delete4(pathlist.MakePathArray(subPool),
                                   force,
                                   keepLocal,
                                   revPropHash,
                                   commitCallback2,
                                   this,
                                   m_pCtx,
                                   subPool),
        NULL);

    ClearCAPIAuthCacheOnError();

    if (m_err != nullptr)
    {
        return FALSE;
    }

    for (int nPath = 0; nPath < pathlist.GetCount(); nPath++)
    {
        if ((!keepLocal) && (!pathlist[nPath].IsDirectory()))
        {
            SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSHNOWAIT, pathlist[nPath].GetWinPath(), nullptr);
        }
        else
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, pathlist[nPath].GetWinPath(), nullptr);
        }
    }

    return true;
}

bool SVN::Revert(const CTSVNPathList& pathList, const CStringArray& changeLists, bool recurse, bool clearChangeLists, bool metadataOnly)
{
    TRACE("Reverting list of %d files\n", pathList.GetCount());
    SVNPool             subPool(m_pool);
    apr_array_header_t* cLists = MakeChangeListArray(changeLists, subPool);

    Prepare();

    SVNTRACE(
        m_err = svn_client_revert4(pathList.MakePathArray(subPool),
                                   recurse ? svn_depth_infinity : svn_depth_empty,
                                   cLists,
                                   clearChangeLists,
                                   metadataOnly,
                                   TRUE,
                                   m_pCtx,
                                   subPool),
        NULL);

    return (m_err == nullptr);
}

bool SVN::Add(const CTSVNPathList& pathList, ProjectProperties* props, svn_depth_t depth, bool force, bool bUseAutoprops, bool noIgnore, bool addParents)
{
    SVNTRACE_BLOCK

    // the add command should use the mime-type file
    const char* mimetypesFile = nullptr;
    Prepare();

    svn_config_t* opt = static_cast<svn_config_t*>(apr_hash_get(m_pCtx->config, SVN_CONFIG_CATEGORY_CONFIG,
                                                                APR_HASH_KEY_STRING));
    if (opt)
    {
        if (bUseAutoprops)
        {
            svn_config_get(opt, &mimetypesFile,
                           SVN_CONFIG_SECTION_MISCELLANY,
                           SVN_CONFIG_OPTION_MIMETYPES_FILE, nullptr);
            if (mimetypesFile && *mimetypesFile)
            {
                m_err = svn_io_parse_mimetypes_file(&(m_pCtx->mimetypes_map),
                                                    mimetypesFile, m_pool);
                if (m_err)
                    return false;
            }
            if (props)
                props->InsertAutoProps(opt);
        }
    }

    for (int nItem = 0; nItem < pathList.GetCount(); nItem++)
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": add file %s\n", pathList[nItem].GetWinPath());
        if (Cancel())
        {
            m_err = svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
            return false;
        }
        SVNPool subPool(m_pool);
        m_err = svn_client_add5(pathList[nItem].GetSVNApiPath(subPool), depth, force, noIgnore, !bUseAutoprops, addParents, m_pCtx, subPool);
        if (m_err != nullptr)
        {
            return false;
        }
    }

    return true;
}

bool SVN::AddToChangeList(const CTSVNPathList& pathList, const CString& changelist, svn_depth_t depth, const CStringArray& changeLists)
{
    SVNPool subPool(m_pool);
    Prepare();

    if (changelist.IsEmpty())
        return RemoveFromChangeList(pathList, CStringArray(), depth);

    apr_array_header_t* cLists = MakeChangeListArray(changeLists, subPool);

    m_err = svn_client_add_to_changelist(pathList.MakePathArray(subPool),
                                         CUnicodeUtils::GetUTF8(changelist),
                                         depth,
                                         cLists,
                                         m_pCtx,
                                         subPool);

    return (m_err == nullptr);
}

bool SVN::RemoveFromChangeList(const CTSVNPathList& pathList, const CStringArray& changeLists, svn_depth_t depth)
{
    SVNPool subPool(m_pool);
    Prepare();
    apr_array_header_t* cLists = MakeChangeListArray(changeLists, subPool);

    m_err = svn_client_remove_from_changelists(pathList.MakePathArray(subPool),
                                               depth,
                                               cLists,
                                               m_pCtx,
                                               subPool);

    return (m_err == nullptr);
}

bool SVN::Update(const CTSVNPathList& pathList, const SVNRev& revision,
                 svn_depth_t depth, bool depthIsSticky,
                 bool ignoreExternals, bool bAllowUnverObstructions,
                 bool makeParents)
{
    SVNPool localPool(m_pool);
    Prepare();
    CHooks::Instance().PreConnect(pathList);
    SVNTRACE(
        m_err = svn_client_update4(NULL,
                                   pathList.MakePathArray(localPool),
                                   revision,
                                   depth,
                                   depthIsSticky,
                                   ignoreExternals,
                                   bAllowUnverObstructions,
                                   true, // adds_as_modifications (why would anyone want a tree conflict? Set to true unconditionally)
                                   makeParents,
                                   m_pCtx,
                                   localPool),
        NULL);
    ClearCAPIAuthCacheOnError();
    return (m_err == nullptr);
}

svn_revnum_t SVN::Commit(const CTSVNPathList& pathList, const CString& message,
                         const CStringArray& changeLists, bool keepChangeList, svn_depth_t depth, bool keepLocks, const RevPropHash& revProps)
{
    SVNPool localPool(m_pool);

    Prepare();

    apr_array_header_t* cLists = MakeChangeListArray(changeLists, localPool);

    apr_hash_t* revpropTable = MakeRevPropHash(revProps, localPool);

    m_pCtx->log_msg_baton3 = logMessage(message);

    CHooks::Instance().PreConnect(pathList);

    SVNTRACE(
        m_err = svn_client_commit6(
            pathList.MakePathArray(localPool),
            depth,
            keepLocks,
            keepChangeList,
            true,  // commit_as_operations
            true,  // include file externals
            false, // include dir externals
            cLists,
            revpropTable,
            commitCallback2,
            this,
            m_pCtx,
            localPool),
        NULL);
    m_pCtx->log_msg_baton3 = logMessage(L"");
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
    {
        return 0;
    }

    return -1;
}

bool SVN::Copy(const CTSVNPathList& srcPathList, const CTSVNPath& destPath,
               const SVNRev& revision, const SVNRev& pegRev, const CString& logMsg, bool copyAsChild,
               bool makeParents, bool ignoreExternals, bool pinExternals, SVNExternals externalsToTag,
               const RevPropHash& revProps)
{
    SVNPool subPool(m_pool);

    Prepare();

    m_pCtx->log_msg_baton3  = logMessage(logMsg);
    apr_hash_t* revPropHash = MakeRevPropHash(revProps, subPool);

    apr_hash_t* externalsToPin = nullptr;
    if (!externalsToTag.empty())
    {
        externalsToPin = externalsToTag.GetHash(revision.IsWorking() || revision.IsBase(), subPool);
    }

    CallPreConnectHookIfUrl(srcPathList, destPath);

    SVNTRACE(
        m_err = svn_client_copy7(MakeCopyArray(srcPathList, revision, pegRev),
                                 destPath.GetSVNApiPath(subPool),
                                 copyAsChild,
                                 makeParents,
                                 ignoreExternals,
                                 false, // metadata only
                                 pinExternals,
                                 externalsToPin,
                                 revPropHash,
                                 commitCallback2,
                                 this,
                                 m_pCtx,
                                 subPool),
        NULL);
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
    {
        return false;
    }

    return true;
}

bool SVN::Move(const CTSVNPathList& srcPathList, const CTSVNPath& destPath,
               const CString& message /* = L""*/,
               bool moveAsChild /* = false*/, bool makeParents /* = false */,
               bool               allowMixed /* = false */,
               bool               metadataOnly /* = false */,
               const RevPropHash& revProps /* = RevPropHash() */)
{
    SVNPool subPool(m_pool);

    Prepare();
    m_pCtx->log_msg_baton3  = logMessage(message);
    apr_hash_t* revPropHash = MakeRevPropHash(revProps, subPool);
    CallPreConnectHookIfUrl(srcPathList, destPath);
    SVNTRACE(
        m_err = svn_client_move7(srcPathList.MakePathArray(subPool),
                                 destPath.GetSVNApiPath(subPool),
                                 moveAsChild,
                                 makeParents,
                                 allowMixed,
                                 metadataOnly,
                                 revPropHash,
                                 commitCallback2,
                                 this,
                                 m_pCtx,
                                 subPool),
        NULL);

    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
    {
        return false;
    }

    return true;
}

bool SVN::MakeDir(const CTSVNPathList& pathList, const CString& message, bool makeParents, const RevPropHash& revProps)
{
    Prepare();
    SVNPool subPool(m_pool);
    m_pCtx->log_msg_baton3  = logMessage(message);
    apr_hash_t* revPropHash = MakeRevPropHash(revProps, subPool);

    CallPreConnectHookIfUrl(pathList);

    SVNTRACE(
        m_err = svn_client_mkdir4(pathList.MakePathArray(subPool),
                                  makeParents,
                                  revPropHash,
                                  commitCallback2,
                                  this,
                                  m_pCtx,
                                  subPool),
        NULL);
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
    {
        return false;
    }

    return true;
}

bool SVN::CleanUp(const CTSVNPath& path, bool breakLocks, bool fixTimeStamps, bool clearDavCache, bool vacuumPristines, bool includeExternals)
{
    Prepare();
    SVNPool     subPool(m_pool);
    const char* svnPath = path.GetSVNApiPath(subPool);
    SVNTRACE(
        m_err = svn_client_cleanup2(svnPath, breakLocks, fixTimeStamps, clearDavCache, vacuumPristines, includeExternals, m_pCtx, subPool),
        svnPath)

    return (m_err == nullptr);
}

bool SVN::Vacuum(const CTSVNPath& path, bool unversioned, bool ignored, bool fixTimeStamps, bool pristines, bool includeExternals)
{
    Prepare();
    SVNPool     subPool(m_pool);
    const char* svnPath = path.GetSVNApiPath(subPool);
    SVNTRACE(
        m_err = svn_client_vacuum(svnPath, unversioned, ignored, fixTimeStamps, pristines, includeExternals, m_pCtx, subPool),
        svnPath)

    return (m_err == nullptr);
}

bool SVN::Resolve(const CTSVNPath& path, svn_wc_conflict_choice_t result, bool recurse, bool typeOnly, svn_wc_conflict_kind_t kind)
{
    SVNPool subPool(m_pool);
    Prepare();

    if (typeOnly)
    {
        m_resolveKind   = kind;
        m_resolveResult = result;
        // change result to unspecified so the conflict resolve callback is invoked
        result = svn_wc_conflict_choose_unspecified;
    }

    switch (kind)
    {
        case svn_wc_conflict_kind_property:
        case svn_wc_conflict_kind_tree:
            break;
        case svn_wc_conflict_kind_text:
        default:
        {
            // before marking a file as resolved, we move the conflicted parts
            // to the trash bin: just in case the user later wants to get those
            // files back anyway
            SVNInfo            info;
            const SVNInfoData* infoData = info.GetFirstFileInfo(path, SVNRev(), SVNRev());
            if (infoData)
            {
                CTSVNPathList conflictedEntries;

                for (auto it = infoData->conflicts.cbegin(); it != infoData->conflicts.cend(); ++it)
                {
                    if ((it->conflictNew.GetLength()) && (result != svn_wc_conflict_choose_theirs_full))
                    {
                        conflictedEntries.AddPath(CTSVNPath(it->conflictNew));
                    }
                    if ((it->conflictOld.GetLength()) && (result != svn_wc_conflict_choose_merged))
                    {
                        conflictedEntries.AddPath(CTSVNPath(it->conflictOld));
                    }
                    if ((it->conflictWrk.GetLength()) && (result != svn_wc_conflict_choose_mine_full))
                    {
                        conflictedEntries.AddPath(CTSVNPath(it->conflictWrk));
                    }
                }
                conflictedEntries.DeleteAllPaths(true, false, nullptr);
            }
        }
        break;
    }

    const char* svnPath = path.GetSVNApiPath(subPool);
    SVNTRACE(
        m_err = svn_client_resolve(svnPath,
                                   recurse ? svn_depth_infinity : svn_depth_empty,
                                   result,
                                   m_pCtx,
                                   subPool),
        svnPath);

    // reset the conflict resolve callback data
    m_resolveKind   = static_cast<svn_wc_conflict_kind_t>(-1);
    m_resolveResult = svn_wc_conflict_choose_postpone;
    return (m_err == nullptr);
}

bool SVN::ResolveTreeConflict(svn_client_conflict_t* conflict, svn_client_conflict_option_t* option, int preferredMovedTargetIdx, int preferredMovedReltargetIdx)
{
    SVNPool scratchPool(m_pool);
    Prepare();

    const char* svnPath = svn_client_conflict_get_local_abspath(conflict);

    if (preferredMovedTargetIdx >= 0)
    {
        svn_client_conflict_option_set_moved_to_abspath(option, preferredMovedTargetIdx, m_pCtx, scratchPool);
    }
    if (preferredMovedReltargetIdx >= 0)
    {
        svn_client_conflict_option_set_moved_to_repos_relpath(option, preferredMovedReltargetIdx, m_pCtx, scratchPool);
    }

    SVNTRACE(
        m_err = svn_client_conflict_tree_resolve(conflict, option, m_pCtx, scratchPool),
        svnPath);

    return (m_err == nullptr);
}

bool SVN::ResolveTextConflict(svn_client_conflict_t* conflict, svn_client_conflict_option_t* option)
{
    SVNPool scratchPool(m_pool);
    Prepare();

    const char* svnPath = svn_client_conflict_get_local_abspath(conflict);

    SVNTRACE(
        m_err = svn_client_conflict_text_resolve(conflict, option, m_pCtx, scratchPool),
        svnPath);

    return (m_err == nullptr);
}

bool SVN::ResolvePropConflict(svn_client_conflict_t* conflict, const CString& propName, svn_client_conflict_option_t* option)
{
    SVNPool scratchPool(m_pool);
    Prepare();

    const char* svnPath = svn_client_conflict_get_local_abspath(conflict);

    SVNTRACE(
        m_err = svn_client_conflict_prop_resolve(conflict, CUnicodeUtils::GetUTF8(propName), option, m_pCtx, scratchPool),
        svnPath);

    return (m_err == nullptr);
}

bool SVN::Export(const CTSVNPath& srcPath, const CTSVNPath& destPath, const SVNRev& pegRev, const SVNRev& revision,
                 bool force, bool bIgnoreExternals, bool bIgnoreKeywords, svn_depth_t depth, HWND hWnd,
                 SVNExportType extended, const CString& eol)
{
    Prepare();

    if (revision.IsWorking())
    {
        SVNTRACE_BLOCK

        if (g_SVNAdminDir.IsAdminDirPath(srcPath.GetWinPath()))
            return false;
        // files are special!
        if (!srcPath.IsDirectory())
        {
            CopyFile(srcPath.GetWinPath(), destPath.GetWinPath(), FALSE);
            SetFileAttributes(destPath.GetWinPath(), FILE_ATTRIBUTE_NORMAL);
            return true;
        }
        // our own "export" function with a callback and the ability to export
        // unversioned items too. With our own function, we can show the progress
        // of the export with a progress bar - that's not possible with the
        // Subversion API.
        // BUG?: If a folder is marked as deleted, we export that folder too!
        CProgressDlg progress;
        progress.SetTitle(IDS_PROC_EXPORT_3);
        progress.FormatNonPathLine(1, IDS_SVNPROGRESS_EXPORTINGWAIT);
        progress.SetTime(true);
        progress.ShowModeless(hWnd);
        std::map<CTSVNPath, CTSVNPath> copyMap;
        switch (extended)
        {
            case SVNExportNormal:
            case SVNExportOnlyLocalChanges:
            {
                CTSVNPath            statusPath;
                svn_client_status_t* s;
                SVNStatus            status;
                if ((s = status.GetFirstFileStatus(srcPath, statusPath, false, svn_depth_infinity, true, !!bIgnoreExternals)) != nullptr)
                {
                    if (SVNStatus::GetMoreImportant(s->node_status, svn_wc_status_unversioned) != svn_wc_status_unversioned)
                    {
                        CTSVNPath destination = destPath;
                        destination.AppendPathString(statusPath.GetWinPathString().Mid(srcPath.GetWinPathString().GetLength()));
                        copyMap[statusPath] = destination;
                    }
                    while ((s = status.GetNextFileStatus(statusPath)) != nullptr)
                    {
                        if ((s->node_status == svn_wc_status_unversioned) ||
                            (s->node_status == svn_wc_status_ignored) ||
                            (s->node_status == svn_wc_status_none) ||
                            (s->node_status == svn_wc_status_missing) ||
                            (s->node_status == svn_wc_status_deleted))
                            continue;
                        if ((extended == SVNExportOnlyLocalChanges) &&
                            (s->node_status == svn_wc_status_normal))
                            continue;
                        CTSVNPath destination = destPath;
                        destination.AppendPathString(statusPath.GetWinPathString().Mid(srcPath.GetWinPathString().GetLength()));
                        copyMap[statusPath] = destination;
                    }
                }
                else
                {
                    m_err = svn_error_create(status.GetSVNError()->apr_err, const_cast<svn_error_t*>(status.GetSVNError()), nullptr);
                    return false;
                }
            }
            break;
            case SVNExportIncludeUnversioned:
            {
                CString      srcFile;
                CDirFileEnum lister(srcPath.GetWinPathString());
                copyMap[srcPath] = destPath;
                while (lister.NextFile(srcFile, nullptr))
                {
                    if (g_SVNAdminDir.IsAdminDirPath(srcFile))
                        continue;
                    CTSVNPath destination = destPath;
                    destination.AppendPathString(srcFile.Mid(srcPath.GetWinPathString().GetLength()));
                    copyMap[CTSVNPath(srcFile)] = destination;
                }
            }
            break;
        }
        size_t count = 0;
        for (std::map<CTSVNPath, CTSVNPath>::iterator it = copyMap.begin(); (it != copyMap.end()) && (!progress.HasUserCancelled()); ++it)
        {
            const auto& src = it->first;
            const auto& dst = it->second;
            progress.FormatPathLine(1, IDS_SVNPROGRESS_EXPORTING, src.GetWinPath());
            progress.FormatPathLine(2, IDS_SVNPROGRESS_EXPORTINGTO, dst.GetWinPath());
            progress.SetProgress64(count, copyMap.size());
            count++;
            if (src.IsDirectory())
                CPathUtils::MakeSureDirectoryPathExists(dst.GetWinPath());
            else
            {
                if (!CopyFile(src.GetWinPath(), dst.GetWinPath(), !force))
                {
                    auto lastError = GetLastError();
                    if (lastError == ERROR_PATH_NOT_FOUND)
                    {
                        CPathUtils::MakeSureDirectoryPathExists(dst.GetContainingDirectory().GetWinPath());
                        if (!CopyFile(src.GetWinPath(), dst.GetWinPath(), !force))
                            lastError = GetLastError();
                        else
                            lastError = 0;
                    }
                    if ((lastError == ERROR_ALREADY_EXISTS) || (lastError == ERROR_FILE_EXISTS))
                    {
                        lastError = 0;

                        UINT    ret = 0;
                        CString strMessage;
                        strMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, dst.GetWinPath());
                        CTaskDialog taskDlg(strMessage,
                                            CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK2)),
                                            L"TortoiseSVN",
                                            0,
                                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                        taskDlg.AddCommandControl(IDYES, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK3)));
                        taskDlg.AddCommandControl(IDCANCEL, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK4)));
                        taskDlg.SetDefaultCommandControl(IDCANCEL);
                        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                        taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITE_CONFIRM_TASK5)));
                        taskDlg.SetMainIcon(TD_WARNING_ICON);
                        ret = static_cast<UINT>(taskDlg.DoModal(GetExplorerHWND()));
                        if (taskDlg.GetVerificationCheckboxState())
                            force = true;

                        if ((ret == IDYESTOALL) || (ret == IDYES))
                        {
                            if (!CopyFile(src.GetWinPath(), dst.GetWinPath(), FALSE))
                            {
                                lastError = GetLastError();
                            }
                            SetFileAttributes(dst.GetWinPath(), FILE_ATTRIBUTE_NORMAL);
                        }
                    }
                    if (lastError)
                    {
                        CFormatMessageWrapper errorDetails(lastError);
                        if (!errorDetails)
                            return false;
                        m_err = svn_error_create(NULL, nullptr, CUnicodeUtils::GetUTF8(static_cast<LPCWSTR>(errorDetails)));
                        return false;
                    }
                }
                SetFileAttributes(dst.GetWinPath(), FILE_ATTRIBUTE_NORMAL);
            }
        }
        if (progress.HasUserCancelled())
        {
            progress.Stop();
            m_err = svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
            return false;
        }
        progress.Stop();
    }
    else
    {
        SVNPool     subPool(m_pool);
        const char* source = srcPath.GetSVNApiPath(subPool);
        CHooks::Instance().PreConnect(CTSVNPathList(srcPath));
        SVNTRACE(
            m_err = svn_client_export5(NULL, //no resulting revision needed
                                       source,
                                       destPath.GetSVNApiPath(subPool),
                                       pegRev,
                                       revision,
                                       force,
                                       bIgnoreExternals,
                                       bIgnoreKeywords,
                                       depth,
                                       eol.IsEmpty() ? NULL : static_cast<LPCSTR>(CStringA(eol)),
                                       m_pCtx,
                                       subPool),
            source);
        ClearCAPIAuthCacheOnError();
        if (m_err != nullptr)
        {
            return false;
        }
    }
    return true;
}

bool SVN::Switch(const CTSVNPath& path, const CTSVNPath& url, const SVNRev& revision, const SVNRev& pegRev, svn_depth_t depth, bool depthIsSticky, bool ignoreExternals, bool allowUnverObstruction, bool ignoreAncestry)
{
    SVNPool subPool(m_pool);
    Prepare();

    const char* svnPath = path.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE(
        m_err = svn_client_switch3(NULL,
                                   svnPath,
                                   url.GetSVNApiPath(subPool),
                                   pegRev,
                                   revision,
                                   depth,
                                   depthIsSticky,
                                   ignoreExternals,
                                   allowUnverObstruction,
                                   ignoreAncestry,
                                   m_pCtx,
                                   subPool),
        svnPath);

    ClearCAPIAuthCacheOnError();
    return (m_err == nullptr);
}

svn_error_t* SVN::import_filter(void*                   baton,
                                svn_boolean_t*          filtered,
                                const char*             localAbspath,
                                const svn_io_dirent2_t* dirent,
                                apr_pool_t*             scratchPool)
{
    // TODO: we could use this when importing files in the repo browser
    // via drag/drop maybe.
    UNREFERENCED_PARAMETER(baton);
    UNREFERENCED_PARAMETER(localAbspath);
    UNREFERENCED_PARAMETER(dirent);
    UNREFERENCED_PARAMETER(scratchPool);
    if (filtered)
        *filtered = FALSE;
    return nullptr;
}

bool SVN::Import(const CTSVNPath& path, const CTSVNPath& url, const CString& message,
                 ProjectProperties* props, svn_depth_t depth, bool bUseAutoProps,
                 bool noIgnore, bool ignoreUnknown,
                 const RevPropHash& revProps)
{
    // the import command should use the mime-type file
    const char* mimetypesFile = nullptr;
    Prepare();
    svn_config_t* opt = static_cast<svn_config_t*>(apr_hash_get(m_pCtx->config, SVN_CONFIG_CATEGORY_CONFIG,
                                                                APR_HASH_KEY_STRING));
    if (bUseAutoProps)
    {
        svn_config_get(opt, &mimetypesFile,
                       SVN_CONFIG_SECTION_MISCELLANY,
                       SVN_CONFIG_OPTION_MIMETYPES_FILE, nullptr);
        if (mimetypesFile && *mimetypesFile)
        {
            m_err = svn_io_parse_mimetypes_file(&(m_pCtx->mimetypes_map),
                                                mimetypesFile, m_pool);
            if (m_err)
                return FALSE;
        }
        if (props)
            props->InsertAutoProps(opt);
    }

    SVNPool subPool(m_pool);
    m_pCtx->log_msg_baton3  = logMessage(message);
    apr_hash_t* revPropHash = MakeRevPropHash(revProps, subPool);

    const char* svnPath = path.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE(
        m_err = svn_client_import5(svnPath,
                                   url.GetSVNApiPath(subPool),
                                   depth,
                                   noIgnore,
                                   !bUseAutoProps,
                                   ignoreUnknown,
                                   revPropHash,
                                   import_filter,
                                   this,
                                   commitCallback2,
                                   this,
                                   m_pCtx,
                                   subPool),
        svnPath);
    m_pCtx->log_msg_baton3 = logMessage(L"");
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
    {
        return false;
    }

    return true;
}

bool SVN::Merge(const CTSVNPath& path1, const SVNRev& revision1, const CTSVNPath& path2, const SVNRev& revision2,
                const CTSVNPath& localPath, bool force, svn_depth_t depth, const CString& options,
                bool ignoreAnchestry, bool dryRun, bool recordOnly, bool allowMixedRevs)
{
    SVNPool subPool(m_pool);

    auto opts = svn_cstring_split(CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, subPool);

    Prepare();

    const char* svnPath = path1.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(path1));
    SVNTRACE(
        m_err = svn_client_merge5(svnPath,
                                  revision1,
                                  path2.GetSVNApiPath(subPool),
                                  revision2,
                                  localPath.GetSVNApiPath(subPool),
                                  depth,
                                  ignoreAnchestry,
                                  ignoreAnchestry,
                                  force,
                                  recordOnly,
                                  dryRun,
                                  allowMixedRevs,
                                  opts,
                                  m_pCtx,
                                  subPool),
        svnPath);
    ClearCAPIAuthCacheOnError();

    return (m_err == nullptr);
}

bool SVN::PegMerge(const CTSVNPath& source, const SVNRevRangeArray& revRangeArray, const SVNRev& pegRevision,
                   const CTSVNPath& destPath, bool force, svn_depth_t depth, const CString& options,
                   bool ignoreAncestry, bool dryRun, bool recordOnly, bool allowMixedRevs)
{
    SVNPool subPool(m_pool);

    auto opts = svn_cstring_split(CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, subPool);

    Prepare();

    const char* svnPath = source.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(source));
    SVNTRACE(
        m_err = svn_client_merge_peg5(svnPath,
                                      revRangeArray.GetCount() ? revRangeArray.GetAprArray(subPool) : NULL,
                                      pegRevision,
                                      destPath.GetSVNApiPath(subPool),
                                      depth,
                                      ignoreAncestry,
                                      ignoreAncestry,
                                      force,
                                      recordOnly,
                                      dryRun,
                                      allowMixedRevs,
                                      opts,
                                      m_pCtx,
                                      subPool),
        svnPath);
    ClearCAPIAuthCacheOnError();

    return (m_err == nullptr);
}

bool SVN::MergeReintegrate(const CTSVNPath& source, const SVNRev& pegRevision, const CTSVNPath& wcPath, bool dryRun, const CString& options)
{
    SVNPool subPool(m_pool);

    auto opts = svn_cstring_split(CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, subPool);

    Prepare();
    const char* svnPath = source.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(source));

#pragma warning(push)
#pragma warning(disable : 4996) // disable deprecated warning: we use svn_client_merge_reintegrate specifically for 'old style' merges
    SVNTRACE(
        m_err = svn_client_merge_reintegrate(svnPath,
                                             pegRevision,
                                             wcPath.GetSVNApiPath(subPool),
                                             dryRun,
                                             opts,
                                             m_pCtx,
                                             subPool),
        svnPath);
#pragma warning(pop)

    ClearCAPIAuthCacheOnError();
    return (m_err == nullptr);
}

bool SVN::SuggestMergeSources(const CTSVNPath& targetPath, const SVNRev& revision, CTSVNPathList& sourceUrLs)
{
    SVNPool             subPool(m_pool);
    apr_array_header_t* sourceUrls = nullptr;

    Prepare();
    sourceUrLs.Clear();
    const char* svnPath = targetPath.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(sourceUrLs);
    SVNTRACE(
        m_err = svn_client_suggest_merge_sources(&sourceUrls,
                                                 svnPath,
                                                 revision,
                                                 m_pCtx,
                                                 subPool),
        svnPath);

    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
    {
        return false;
    }

    for (int i = 0; i < sourceUrls->nelts; i++)
    {
        const char* path = (APR_ARRAY_IDX(sourceUrls, i, const char*));
        sourceUrLs.AddPath(CTSVNPath(CUnicodeUtils::GetUnicode(path)));
    }

    return true;
}

bool SVN::CreatePatch(const CTSVNPath& path1, const SVNRev& revision1,
                      const CTSVNPath& path2, const SVNRev& revision2,
                      const CTSVNPath& relativeToDir, svn_depth_t depth,
                      bool ignoreAncestry, bool noDiffAdded, bool noDiffDeleted, bool showCopiesAsAdds, bool ignoreContentType,
                      bool useGitFormat, bool ignoreProperties, bool propertiesOnly, bool prettyPrint, const CString& options, bool bAppend, const CTSVNPath& outputFile)
{
    // to create a patch, we need to remove any custom diff tools which might be set in the config file
    svn_config_t* cfg = static_cast<svn_config_t*>(apr_hash_get(m_pCtx->config, SVN_CONFIG_CATEGORY_CONFIG, APR_HASH_KEY_STRING));
    CStringA      diffCmd;
    CStringA      diff3Cmd;
    if (cfg)
    {
        const char* value;
        svn_config_get(cfg, &value, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF_CMD, nullptr);
        diffCmd = CStringA(value);
        svn_config_get(cfg, &value, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF3_CMD, nullptr);
        diff3Cmd = CStringA(value);

        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF_CMD, nullptr);
        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF3_CMD, nullptr);
    }

    bool bRet = Diff(path1, revision1, path2, revision2, relativeToDir, depth, ignoreAncestry, noDiffAdded, noDiffDeleted, showCopiesAsAdds, ignoreContentType, useGitFormat, ignoreProperties, propertiesOnly, prettyPrint, options, bAppend, outputFile, CTSVNPath());
    if (cfg)
    {
        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF_CMD, diffCmd);
        svn_config_set(cfg, SVN_CONFIG_SECTION_HELPERS, SVN_CONFIG_OPTION_DIFF3_CMD, diff3Cmd);
    }
    return bRet;
}

bool SVN::Diff(const CTSVNPath& path1, const SVNRev& revision1,
               const CTSVNPath& path2, const SVNRev& revision2,
               const CTSVNPath& relativeToDir, svn_depth_t depth,
               bool ignoreAncestry, bool noDiffAdded, bool noDiffDeleted, bool showCopiesAsAdds, bool ignoreContentType,
               bool useGitFormat, bool ignoreProperties, bool propertiesOnly, bool prettyPrint, const CString& options, bool bAppend, const CTSVNPath& outputFile)
{
    return Diff(path1, revision1, path2, revision2, relativeToDir, depth, ignoreAncestry, noDiffAdded, noDiffDeleted, showCopiesAsAdds, ignoreContentType, useGitFormat, ignoreProperties, propertiesOnly, prettyPrint, options, bAppend, outputFile, CTSVNPath());
}

bool SVN::Diff(const CTSVNPath& path1, const SVNRev& revision1,
               const CTSVNPath& path2, const SVNRev& revision2,
               const CTSVNPath& relativeToDir, svn_depth_t depth,
               bool ignoreAncestry, bool noDiffAdded, bool noDiffDeleted, bool showCopiesAsAdds, bool ignoreContentType,
               bool useGitFormat, bool ignoreProperties, bool propertiesOnly, bool prettyPrint, const CString& options, bool bAppend, const CTSVNPath& outputFile, const CTSVNPath& errorFile)
{
    bool                del     = false;
    apr_file_t*         outFile = nullptr;
    apr_file_t*         errFile = nullptr;
    apr_array_header_t* opts;

    SVNPool localPool(m_pool);
    Prepare();

    opts = svn_cstring_split(CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localPool);

    apr_int32_t flags = APR_WRITE | APR_CREATE | APR_BINARY;
    if (bAppend)
        flags |= APR_APPEND;
    else
        flags |= APR_TRUNCATE;
    m_err = svn_io_file_open(&outFile, outputFile.GetSVNApiPath(localPool),
                             flags,
                             APR_OS_DEFAULT, localPool);
    if (m_err)
        return false;

    svn_stream_t* outStream = svn_stream_from_aprfile2(outFile, false, localPool);
    svn_stream_t* errStream = svn_stream_from_aprfile2(errFile, false, localPool);

    CTSVNPath workingErrorFile;
    if (errorFile.IsEmpty())
    {
        workingErrorFile = CTempFiles::Instance().GetTempFilePath(true);
        del              = true;
    }
    else
    {
        workingErrorFile = errorFile;
    }

    m_err = svn_io_file_open(&errFile, workingErrorFile.GetSVNApiPath(localPool),
                             APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
                             APR_OS_DEFAULT, localPool);
    if (m_err)
        return false;

    const char* svnPath = path1.GetSVNApiPath(localPool);
    if (path1.IsUrl() || path2.IsUrl() || !revision1.IsWorking() || !revision2.IsWorking())
        CHooks::Instance().PreConnect(CTSVNPathList(path1));
    SVNTRACE(
        m_err = svn_client_diff7(opts,
                                 svnPath,
                                 revision1,
                                 path2.GetSVNApiPath(localPool),
                                 revision2,
                                 relativeToDir.IsEmpty() ? NULL : relativeToDir.GetSVNApiPath(localPool),
                                 depth,
                                 ignoreAncestry,
                                 noDiffAdded,
                                 noDiffDeleted,
                                 showCopiesAsAdds,
                                 ignoreContentType,
                                 ignoreProperties,
                                 propertiesOnly,
                                 useGitFormat,
                                 prettyPrint,
                                 "UTF-8",
                                 outStream,
                                 errStream,
                                 NULL, // we don't deal with change lists when diffing
                                 m_pCtx,
                                 localPool),
        svnPath);
    ClearCAPIAuthCacheOnError();
    if (m_err)
    {
        return false;
    }
    if (del)
    {
        svn_io_remove_file2(workingErrorFile.GetSVNApiPath(localPool), true, localPool);
    }
    return true;
}

bool SVN::PegDiff(const CTSVNPath& path, const SVNRev& pegRevision,
                  const SVNRev& startRev, const SVNRev& endRev,
                  const CTSVNPath& relativeToDir, svn_depth_t depth,
                  bool ignoreAncestry, bool noDiffAdded, bool noDiffDeleted, bool showCopiesAsAdds, bool ignoreContentType,
                  bool useGitFormat, bool ignoreProperties, bool propertiesOnly, bool prettyPrint, const CString& options, bool bAppend, const CTSVNPath& outputFile)
{
    return PegDiff(path, pegRevision, startRev, endRev, relativeToDir, depth, ignoreAncestry, noDiffAdded, noDiffDeleted, showCopiesAsAdds, ignoreContentType, useGitFormat, ignoreProperties, propertiesOnly, prettyPrint, options, bAppend, outputFile, CTSVNPath());
}

bool SVN::PegDiff(const CTSVNPath& path, const SVNRev& pegRevision,
                  const SVNRev& startRev, const SVNRev& endRev,
                  const CTSVNPath& relativeToDir, svn_depth_t depth,
                  bool ignoreAncestry, bool noDiffAdded, bool noDiffDeleted, bool showCopiesAsAdds, bool ignoreContentType,
                  bool useGitFormat, bool ignoreProperties, bool propertiesOnly, bool prettyPrint, const CString& options, bool bAppend, const CTSVNPath& outputFile, const CTSVNPath& errorFile)
{
    bool                del     = false;
    apr_file_t*         outFile = nullptr;
    apr_file_t*         errFile = nullptr;
    apr_array_header_t* opts;

    SVNPool localpool(m_pool);
    Prepare();

    opts = svn_cstring_split(CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localpool);

    apr_int32_t flags = APR_WRITE | APR_CREATE | APR_BINARY;
    if (bAppend)
        flags |= APR_APPEND;
    else
        flags |= APR_TRUNCATE;
    m_err = svn_io_file_open(&outFile, outputFile.GetSVNApiPath(localpool),
                             flags,
                             APR_OS_DEFAULT, localpool);

    if (m_err)
        return false;

    CTSVNPath workingErrorFile;
    if (errorFile.IsEmpty())
    {
        workingErrorFile = CTempFiles::Instance().GetTempFilePath(true);
        del              = true;
    }
    else
    {
        workingErrorFile = errorFile;
    }

    m_err = svn_io_file_open(&errFile, workingErrorFile.GetSVNApiPath(localpool),
                             APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
                             APR_OS_DEFAULT, localpool);
    if (m_err)
        return false;

    svn_stream_t* outStream = svn_stream_from_aprfile2(outFile, false, localpool);
    svn_stream_t* errStream = svn_stream_from_aprfile2(errFile, false, localpool);

    const char* svnPath = path.GetSVNApiPath(localpool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE(
        m_err = svn_client_diff_peg7(opts,
                                     svnPath,
                                     pegRevision,
                                     startRev,
                                     endRev,
                                     relativeToDir.IsEmpty() ? NULL : relativeToDir.GetSVNApiPath(localpool),
                                     depth,
                                     ignoreAncestry,
                                     noDiffAdded,
                                     noDiffDeleted,
                                     showCopiesAsAdds,
                                     ignoreContentType,
                                     ignoreProperties,
                                     propertiesOnly,
                                     useGitFormat,
                                     prettyPrint,
                                     "UTF-8",
                                     outStream,
                                     errStream,
                                     NULL, // we don't deal with change lists when diffing
                                     m_pCtx,
                                     localpool),
        svnPath);
    ClearCAPIAuthCacheOnError();
    if (m_err)
    {
        return false;
    }
    if (del)
    {
        svn_io_remove_file2(workingErrorFile.GetSVNApiPath(localpool), true, localpool);
    }
    return true;
}

bool SVN::DiffSummarize(const CTSVNPath& path1, const SVNRev& rev1, const CTSVNPath& path2, const SVNRev& rev2, svn_depth_t depth, bool ignoreAncestry)
{
    SVNPool localPool(m_pool);
    Prepare();

    const char* svnPath = path1.GetSVNApiPath(localPool);
    if (path1.IsUrl() || path2.IsUrl() || !rev1.IsWorking() || !rev2.IsWorking())
        CHooks::Instance().PreConnect(CTSVNPathList(path1));
    SVNTRACE(
        m_err = svn_client_diff_summarize2(svnPath, rev1,
                                           path2.GetSVNApiPath(localPool), rev2,
                                           depth, ignoreAncestry, NULL,
                                           summarize_func, this,
                                           m_pCtx, localPool),
        svnPath);
    ClearCAPIAuthCacheOnError();

    return (m_err == nullptr);
}

bool SVN::DiffSummarizePeg(const CTSVNPath& path, const SVNRev& peg, const SVNRev& rev1, const SVNRev& rev2, svn_depth_t depth, bool ignoreAncestry)
{
    SVNPool localPool(m_pool);
    Prepare();

    const char* svnPath = path.GetSVNApiPath(localPool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE(
        m_err = svn_client_diff_summarize_peg2(svnPath, peg, rev1, rev2,
                                               depth, ignoreAncestry, NULL,
                                               summarize_func, this,
                                               m_pCtx, localPool),
        svnPath);
    ClearCAPIAuthCacheOnError();

    return (m_err == nullptr);
}

LogCache::CCachedLogInfo* SVN::GetLogCache(const CTSVNPath& path)
{
    if (!LogCache::CSettings::GetEnabled())
        return nullptr;

    CString uuid;
    CString root = GetLogCachePool()->GetRepositoryInfo().GetRepositoryRootAndUUID(path, uuid);
    return GetLogCachePool()->GetCache(uuid, root);
}

std::unique_ptr<const CCacheLogQuery>
    SVN::ReceiveLog(const CTSVNPathList& pathList, const SVNRev& revisionPeg,
                    const SVNRev& revisionStart, const SVNRev& revisionEnd,
                    int limit, bool strict, bool withMerges, bool refresh)
{
    Prepare();
    try
    {
        SVNPool localpool(m_pool);

        // query used internally to contact the repository if necessary

        CSVNLogQuery svnQuery(m_pCtx, localpool);

        // cached-based queries.
        // Use & update existing cache

        auto cacheQuery = std::make_unique<CCacheLogQuery>(GetLogCachePool(), &svnQuery);

        // run query through SVN but collect results in a temporary cache

        auto tempQuery = std::make_unique<CCacheLogQuery>(*this, &svnQuery);

        // select query and run it

        ILogQuery* query = !IsLogCacheEnabled() || refresh
                               ? tempQuery.get()
                               : cacheQuery.get();

        try
        {
            query->Log(pathList, revisionPeg, revisionStart, revisionEnd, limit, strict != FALSE, this, false, // changes will be fetched but not forwarded to receiver
                       withMerges != FALSE, true, false, TRevPropNames());
        }
        catch (SVNError& e)
        {
            // cancellation is no actual error condition

            if (e.GetCode() != SVN_ERR_CANCELLED)
                throw;

            // caller shall be able to detect the cancellation, though

            m_err = svn_error_create(e.GetCode(), nullptr, e.GetMessage());
        }

        // merge temp results with permanent cache, if applicable

        if (refresh && IsLogCacheEnabled())
        {
            // handle cache refresh results

            if (tempQuery->GotAnyData())
            {
                tempQuery->UpdateCache(cacheQuery.get());
            }
            else
            {
                // no connection to the repository but also not canceled
                // (no exception thrown) -> re-run from cache

                return ReceiveLog(pathList, revisionPeg, revisionStart, revisionEnd, limit, strict, withMerges, false);
            }
        }

        // return the cache that contains the log info

        return std::unique_ptr<const CCacheLogQuery>(IsLogCacheEnabled()
                                                         ? cacheQuery.release()
                                                         : tempQuery.release());
    }
    catch (SVNError& e)
    {
        m_err = svn_error_create(e.GetCode(), nullptr, e.GetMessage());
        return std::unique_ptr<const CCacheLogQuery>();
    }
}

bool SVN::Cat(const CTSVNPath& url, const SVNRev& pegRevision, const SVNRev& revision, const CTSVNPath& localPath)
{
    apr_file_t*   file   = nullptr;
    svn_stream_t* stream = nullptr;
    apr_status_t  status = {};
    SVNPool       localPool(m_pool);
    Prepare();

    CTSVNPath fullLocalPath(localPath);
    if (fullLocalPath.IsDirectory())
    {
        fullLocalPath.AppendPathString(url.GetFileOrDirectoryName());
    }
    ::DeleteFile(fullLocalPath.GetWinPath());

    status = apr_file_open(&file, fullLocalPath.GetSVNApiPath(localPool), APR_WRITE | APR_CREATE | APR_TRUNCATE, APR_OS_DEFAULT, localPool);
    if (status)
    {
        m_err = svn_error_wrap_apr(status, nullptr);
        return FALSE;
    }
    stream = svn_stream_from_aprfile2(file, true, localPool);

    const char* svnPath = url.GetSVNApiPath(localPool);
    if (url.IsUrl() || (!pegRevision.IsWorking() && !pegRevision.IsValid()) || (!revision.IsWorking() && !revision.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(url));
    SVNTRACE(
        m_err = svn_client_cat3(NULL, stream, svnPath, pegRevision, revision, true, m_pCtx, localPool, localPool),
        svnPath);

    apr_file_close(file);
    ClearCAPIAuthCacheOnError();

    return (m_err == nullptr);
}

bool SVN::CreateRepository(const CTSVNPath& path, const CString& fsType)
{
    svn_repos_t* repo   = nullptr;
    svn_error_t* err    = nullptr;
    apr_hash_t*  config = nullptr;

    SVNPool localPool;

    apr_hash_t* fsConfig = apr_hash_make(localPool);

    apr_hash_set(fsConfig, SVN_FS_CONFIG_BDB_TXN_NOSYNC,
                 APR_HASH_KEY_STRING, "0");

    apr_hash_set(fsConfig, SVN_FS_CONFIG_BDB_LOG_AUTOREMOVE,
                 APR_HASH_KEY_STRING, "1");

    config = SVNConfig::Instance().GetConfig(localPool);

    const char* fs_type = apr_pstrdup(localPool, CStringA(fsType));
    apr_hash_set(fsConfig, SVN_FS_CONFIG_FS_TYPE,
                 APR_HASH_KEY_STRING,
                 fs_type);
    err = svn_repos_create(&repo, path.GetSVNApiPath(localPool), nullptr, nullptr, config, fsConfig, localPool);

    bool ret = (err == nullptr);
    svn_error_clear(err);
    return ret;
}

bool SVN::Blame(const CTSVNPath& path, const SVNRev& startRev, const SVNRev& endRev, const SVNRev& peg, const CString& diffOptions, bool ignoreMimeType, bool includeMerge)
{
    Prepare();
    SVNPool                  subPool(m_pool);
    apr_array_header_t*      opts    = nullptr;
    svn_diff_file_options_t* options = svn_diff_file_options_create(subPool);
    opts                             = svn_cstring_split(CUnicodeUtils::GetUTF8(diffOptions), " \t\n\r", TRUE, subPool);
    svn_error_clear(svn_diff_file_options_parse(options, opts, subPool));

    // Subversion < 1.4 silently changed a revision WC to BASE. Due to a bug
    // report this was changed: now Subversion returns an error 'not implemented'
    // since it actually blamed the BASE file and not the working copy file.
    // Until that's implemented, we 'fall back' here to the old behavior and
    // just change and REV_WC to REV_BASE.
    SVNRev rev1 = startRev;
    SVNRev rev2 = endRev;
    if (rev1.IsWorking())
        rev1 = SVNRev::REV_BASE;
    if (rev2.IsWorking())
        rev2 = SVNRev::REV_BASE;

    const char* svnPath = path.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE(
        m_err = svn_client_blame6(nullptr, nullptr,
                                  svnPath,
                                  peg,
                                  rev1,
                                  rev2,
                                  options,
                                  ignoreMimeType,
                                  includeMerge,
                                  blameReceiver,
                                  this,
                                  m_pCtx,
                                  subPool),
        svnPath);
    ClearCAPIAuthCacheOnError();

    if ((m_err != nullptr) && ((m_err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE) || (m_err->apr_err == SVN_ERR_FS_NOT_FOUND) || (m_err->apr_err == SVN_ERR_CLIENT_UNRELATED_RESOURCES)) && (includeMerge))
    {
        Prepare();
        SVNTRACE(
            m_err = svn_client_blame6(nullptr, nullptr,
                                      svnPath,
                                      peg,
                                      rev1,
                                      rev2,
                                      options,
                                      ignoreMimeType,
                                      false,
                                      blameReceiver,
                                      this,
                                      m_pCtx,
                                      subPool),
            svnPath)
    }
    ClearCAPIAuthCacheOnError();

    return (m_err == nullptr);
}

svn_error_t* SVN::blameReceiver(void*               baton,
                                apr_int64_t         lineNo,
                                svn_revnum_t        revision,
                                apr_hash_t*         revProps,
                                svn_revnum_t        mergedRevision,
                                apr_hash_t*         mergedRevProps,
                                const char*         mergedPath,
                                const svn_string_t* line,
                                svn_boolean_t       localChange,
                                apr_pool_t*         pool)
{
    svn_error_t* error = nullptr;
    CString      authorNative, mergedAuthorNative;
    CString      mergedPathNative;
    CStringA     logMsg;
    CStringA     mergedLogMsg;
    CStringA     lineNative;
    wchar_t      dateNative[SVN_DATE_BUFFER]       = {0};
    wchar_t      mergedDateNative[SVN_DATE_BUFFER] = {0};

    SVN* svn = static_cast<SVN*>(baton);

    const char* prop = svn_prop_get_value(revProps, SVN_PROP_REVISION_AUTHOR);
    if (prop)
        authorNative = CUnicodeUtils::GetUnicode(prop);
    prop = svn_prop_get_value(mergedRevProps, SVN_PROP_REVISION_AUTHOR);
    if (prop)
        mergedAuthorNative = CUnicodeUtils::GetUnicode(prop);

    prop = svn_prop_get_value(revProps, SVN_PROP_REVISION_LOG);
    if (prop)
        logMsg = prop;
    prop = svn_prop_get_value(mergedRevProps, SVN_PROP_REVISION_LOG);
    if (prop)
        mergedLogMsg = prop;

    if (mergedPath)
        mergedPathNative = CUnicodeUtils::GetUnicode(mergedPath);

    bool utf16           = ((line->len == 1) && svn->ignoredLastLine);
    svn->ignoredLastLine = false;
    if (line)
    {
        if (svn->unicodeType == SVN::UnicodeType::AUTOTYPE)
        {
            // encoding not yet determined. First check for BOM
            if (line->len >= 2)
            {
                if ((line->data[0] == 0xFE) && (line->data[1] == 0xFF))
                    svn->unicodeType = SVN::UnicodeType::UTF16_BEBOM;
                if ((line->data[0] == 0xFF) && (line->data[1] == 0xFE))
                    svn->unicodeType = SVN::UnicodeType::UTF16_LEBOM;
                if ((line->len >= 3) && (line->data[0] == 0xEF) && (line->data[1] == 0xBB) && (line->data[1] == 0xBF))
                    svn->unicodeType = SVN::UnicodeType::UTF8BOM;
            }

            if ((line->len > 10) && (svn->unicodeType == SVN::UnicodeType::AUTOTYPE))
            {
                // we don't have a BOM, so we have to try to figure out
                // the encoding using heuristics. But only if the line is long enough
                size_t nullCount = 0;
                for (size_t i = 0; i < line->len; ++i)
                {
                    if (line->data[i] == 0)
                        ++nullCount;
                }
                if (nullCount > (line->len / 10))
                {
                    // assume UTF16_LE
                    svn->unicodeType = SVN::UnicodeType::UTF16_LE;
                }
            }
        }
        lineNative = line->data;
        int wLen   = static_cast<int>(line->len / sizeof(wchar_t));
        if ((svn->unicodeType == SVN::UnicodeType::UTF16_LE) ||
            (lineNative.GetLength() < (wLen - 1)))
        {
            // assume utf16
            if ((line->len % 2) == 0)
            {
                std::wstring ws = std::wstring(reinterpret_cast<const wchar_t*>(line->data), wLen);
                lineNative      = CUnicodeUtils::StdGetUTF8(ws).c_str();
                utf16           = true;
            }
            // subsequent uf16 lines are off by one zero byte
            else if (line->data[0] == 0)
            {
                auto adjustedLine = line->data;
                ++adjustedLine;
                std::wstring ws = std::wstring(reinterpret_cast<const wchar_t*>(adjustedLine), wLen);
                lineNative      = CUnicodeUtils::StdGetUTF8(ws).c_str();
                utf16           = true;
            }
        }
    }
    prop = svn_prop_get_value(revProps, SVN_PROP_REVISION_DATE);
    if (prop)
    {
        // Convert date to a format for humans.
        apr_time_t timeTemp;

        error = svn_time_from_cstring(&timeTemp, prop, pool);
        if (error)
            return error;

        formatDate(dateNative, timeTemp, true);
    }
    else
        wcscat_s(dateNative, L"(no date)");

    prop = svn_prop_get_value(mergedRevProps, SVN_PROP_REVISION_DATE);
    if (prop)
    {
        // Convert date to a format for humans.
        apr_time_t timeTemp;

        error = svn_time_from_cstring(&timeTemp, prop, pool);
        if (error)
            return error;

        formatDate(mergedDateNative, timeTemp, true);
    }
    else
        wcscat_s(mergedDateNative, L"(no date)");

    if (!svn->ignoreNextLine)
    {
        if (!svn->BlameCallback(static_cast<LONG>(lineNo), !!localChange, revision, authorNative, dateNative, mergedRevision, mergedAuthorNative, mergedDateNative, mergedPathNative, lineNative, logMsg, mergedLogMsg))
        {
            return svn_error_create(SVN_ERR_CANCELLED, nullptr, "error in blame callback");
        }
    }
    else
        svn->ignoredLastLine = true;
    svn->ignoreNextLine = utf16 && !svn->ignoredLastLine;
    return error;
}

bool SVN::Lock(const CTSVNPathList& pathList, bool bStealLock, const CString& comment /* = CString( */)
{
    SVNTRACE_BLOCK

    Prepare();
    SVNPool subPool(m_pool);
    CHooks::Instance().PreConnect(pathList);
    m_err = svn_client_lock(pathList.MakePathArray(subPool), CUnicodeUtils::GetUTF8(comment), !!bStealLock, m_pCtx, subPool);
    ClearCAPIAuthCacheOnError();
    return (m_err == nullptr);
}

bool SVN::Unlock(const CTSVNPathList& pathList, bool bBreakLock)
{
    SVNTRACE_BLOCK

    Prepare();
    SVNPool subPool(m_pool);
    CHooks::Instance().PreConnect(pathList);
    m_err = svn_client_unlock(pathList.MakePathArray(subPool), bBreakLock, m_pCtx, subPool);
    ClearCAPIAuthCacheOnError();
    return (m_err == nullptr);
}

svn_error_t* SVN::summarize_func(const svn_client_diff_summarize_t* diff, void* baton, apr_pool_t* /*pool*/)
{
    SVN* svn = static_cast<SVN*>(baton);
    if (diff)
    {
        CTSVNPath path = CTSVNPath(CUnicodeUtils::GetUnicode(diff->path));
        return svn->DiffSummarizeCallback(path, diff->summarize_kind, !!diff->prop_changed, diff->node_kind);
    }
    return nullptr;
}
svn_error_t* SVN::listReceiver(void* baton, const char* path,
                               const svn_dirent_t* dirent,
                               const svn_lock_t*   lock,
                               const char*         absPath,
                               const char*         externalParentURL,
                               const char*         externalTarget,
                               apr_pool_t* /*pool*/)
{
    SVN*         svn    = static_cast<SVN*>(baton);
    bool         result = !!svn->ReportList(CUnicodeUtils::GetUnicode(path),
                                    dirent->kind,
                                    dirent->size,
                                    !!dirent->has_props,
                                    dirent->created_rev,
                                    dirent->time,
                                    CUnicodeUtils::GetUnicode(dirent->last_author),
                                    lock ? CUnicodeUtils::GetUnicode(lock->token) : CString(),
                                    lock ? CUnicodeUtils::GetUnicode(lock->owner) : CString(),
                                    lock ? CUnicodeUtils::GetUnicode(lock->comment) : CString(),
                                    lock ? !!lock->is_dav_comment : false,
                                    lock ? lock->creation_date : 0,
                                    lock ? lock->expiration_date : 0,
                                    CUnicodeUtils::GetUnicode(absPath),
                                    CUnicodeUtils::GetUnicode(externalParentURL),
                                    CUnicodeUtils::GetUnicode(externalTarget));
    svn_error_t* err    = nullptr;
    if ((result == false) || svn->Cancel())
    {
        CString temp;
        temp.LoadString(result ? IDS_SVN_USERCANCELLED : IDS_ERR_ERROR);
        err = svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(temp));
    }
    return err;
}

// implement ILogReceiver

void SVN::ReceiveLog(TChangedPaths* /* changes */
                     ,
                     svn_revnum_t rev, const StandardRevProps* stdRevProps, UserRevPropArray* userRevProps, const MergeInfo* mergeInfo)
{
    // check for user pressing "Cancel" somewhere

    cancel();

    if (rev == 0)
    {
        // only use revision 0 if either the author or a message is set, or if user props are set
        bool bEmpty = stdRevProps ? stdRevProps->GetAuthor().empty() : true;
        bEmpty      = bEmpty && (stdRevProps ? stdRevProps->GetMessage().empty() : true);
        bEmpty      = bEmpty && (userRevProps ? userRevProps->GetCount() == 0 : true);
        if (bEmpty)
            return;
    }

    // use the log info (in a derived class specific way)

    static const std::string emptyString;
    Log(rev, stdRevProps == nullptr ? emptyString : stdRevProps->GetAuthor(), stdRevProps == nullptr ? emptyString : stdRevProps->GetMessage(), stdRevProps == nullptr ? static_cast<apr_time_t>(0) : stdRevProps->GetTimeStamp(), mergeInfo);
}

void SVN::notify(void*                  baton,
                 const svn_wc_notify_t* notify,
                 apr_pool_t*            pool)
{
    SVN* svn = static_cast<SVN*>(baton);

    CTSVNPath tsvnPath;
    tsvnPath.SetFromSVN(notify->path);
    CTSVNPath url;
    url.SetFromSVN(notify->url);
    if (notify->action == svn_wc_notify_url_redirect)
        svn->m_redirectedUrl = url;

    CString mime;
    if (notify->mime_type)
        mime = CUnicodeUtils::GetUnicode(notify->mime_type);

    CString changelistname;
    if (notify->changelist_name)
        changelistname = CUnicodeUtils::GetUnicode(notify->changelist_name);

    CString propertyName;
    if (notify->prop_name)
        propertyName = CUnicodeUtils::GetUnicode(notify->prop_name);

    svn->Notify(tsvnPath, url, notify->action, notify->kind,
                mime, notify->content_state,
                notify->prop_state, notify->revision,
                notify->lock, notify->lock_state, changelistname, propertyName, notify->merge_range,
                notify->err, pool);
}

[[maybe_unused]] static svn_wc_conflict_choice_t
    trivialConflictChoice(const svn_wc_conflict_description2_t* cd)
{
    if ((cd->operation == svn_wc_operation_update || cd->operation == svn_wc_operation_switch) &&
        cd->reason == svn_wc_conflict_reason_moved_away &&
        cd->action == svn_wc_conflict_action_edit)
    {
        /* local move vs. incoming edit -> update move */
        return svn_wc_conflict_choose_mine_conflict;
    }

    return svn_wc_conflict_choose_unspecified;
}

svn_error_t* SVN::cancel(void* baton)
{
    SVN* svn = static_cast<SVN*>(baton);
    if ((svn->Cancel()) || ((svn->m_pProgressDlg) && (svn->m_pProgressDlg->HasUserCancelled())))
    {
        CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(message));
    }
    return nullptr;
}

void SVN::cancel()
{
    if (Cancel() || ((m_pProgressDlg != nullptr) && (m_pProgressDlg->HasUserCancelled())))
    {
        CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
        throw SVNError(SVN_ERR_CANCELLED, CUnicodeUtils::GetUTF8(message));
    }
}

void* SVN::logMessage(CString message, char* baseDirectory) const
{
    message.Remove('\r');

    LogMsgBaton3* baton = static_cast<LogMsgBaton3*>(apr_palloc(m_pool, sizeof(*baton)));
    baton->message        = apr_pstrdup(m_pool, CUnicodeUtils::GetUTF8(message));
    baton->base_dir       = baseDirectory ? baseDirectory : "";

    baton->message_encoding = nullptr;
    baton->tmpfile_left     = nullptr;
    baton->pool             = m_pool;

    return baton;
}

void SVN::PathToUrl(CString& path)
{
    bool bUnc = false;
    path.Trim();
    if (path.Left(2).Compare(L"\\\\") == 0)
        bUnc = true;
    // convert \ to /
    path.Replace('\\', '/');
    path.TrimLeft('/');
    // prepend file://
    if (bUnc)
        path.Insert(0, L"file://");
    else
        path.Insert(0, L"file:///");
    path.TrimRight(L"/\\"); //remove trailing slashes
}

void SVN::UrlToPath(CString& url)
{
    // we have to convert paths like file:///c:/myfolder
    // to c:/myfolder
    // and paths like file:////mymachine/c/myfolder
    // to //mymachine/c/myfolder
    url.Trim();
    url.Replace('\\', '/');
    url = url.Mid(7); // "file://" has seven chars
    url.TrimLeft('/');
    // if we have a ':' that means the file:// url points to an absolute path
    // like file:///D:/Development/test
    // if we don't have a ':' we assume it points to an UNC path, and those
    // actually _need_ the slashes before the paths
    if ((url.Find(':') < 0) && (url.Find('|') < 0))
        url.Insert(0, L"\\\\");
    SVN::preparePath(url);
    // now we need to unescape the url
    url = CPathUtils::PathUnescape(url);
}

void SVN::preparePath(CString& path)
{
    path.Trim();
    path.TrimRight(L"/\\"); //remove trailing slashes
    path.Replace('\\', '/');

    if (path.Left(10).CompareNoCase(L"file://///") == 0)
    {
        if (path.Find('%') < 0)
            path.Replace(L"file://///", L"file://");
        else
            path.Replace(L"file://///", L"file:////");
    }
    else if (path.Left(9).CompareNoCase(L"file:////") == 0)
    {
        if (path.Find('%') < 0)
            path.Replace(L"file:////", L"file://");
    }
}

svn_error_t* svnClGetLogMessage(const char** logMsg,
                                const char** tmpFile,
                                const apr_array_header_t* /*commit_items*/,
                                void*       baton,
                                apr_pool_t* pool)
{
    LogMsgBaton3* lmb = static_cast<LogMsgBaton3*>(baton);
    *tmpFile            = nullptr;
    if (lmb->message)
    {
        *logMsg = apr_pstrdup(pool, lmb->message);
    }

    return nullptr;
}

CString SVN::GetURLFromPath(const CTSVNPath& path)
{
    const char* URL;
    if (path.IsUrl())
        return path.GetSVNPathString();
    Prepare();
    SVNPool subPool(m_pool);
    m_err = svn_client_url_from_path2(&URL, path.GetSVNApiPath(subPool), m_pCtx, subPool, subPool);
    if (m_err)
        return L"";
    if (URL == nullptr)
        return L"";
    return CString(URL);
}

CTSVNPath SVN::GetWCRootFromPath(const CTSVNPath& path)
{
    const char* wcRoot = nullptr;
    Prepare();
    SVNPool     subPool(m_pool);
    const char* svnPath = path.GetSVNApiPath(subPool);
    if ((svnPath == nullptr) || svn_path_is_url(svnPath) || (svnPath[0] == 0))
        return CTSVNPath();
    SVNTRACE(
        m_err = svn_client_get_wc_root(&wcRoot, svnPath, m_pCtx, subPool, subPool),
        svnPath)

    if (m_err)
        return CTSVNPath();
    if (wcRoot == nullptr)
        return CTSVNPath();
    CTSVNPath ret;
    ret.SetFromSVN(wcRoot);
    return ret;
}

bool SVN::List(const CTSVNPath& url, const SVNRev& revision, const SVNRev& pegRev, svn_depth_t depth, bool fetchLocks, apr_uint32_t dirents, bool includeExternals)
{
    SVNPool subPool(m_pool);
    Prepare();

    const char* svnPath = url.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(url));

    SVNTRACE(
        m_err = svn_client_list4(svnPath,
                                 pegRev,
                                 revision,
                                 nullptr,
                                 depth,
                                 dirents,
                                 fetchLocks,
                                 includeExternals,
                                 listReceiver,
                                 this,
                                 m_pCtx,
                                 subPool),
        svnPath)
    ClearCAPIAuthCacheOnError();
    return (m_err == nullptr);
}

bool SVN::Relocate(const CTSVNPath& path, const CTSVNPath& from, const CTSVNPath& to, bool includeExternals)
{
    Prepare();
    SVNPool subPool(m_pool);
    CString uuid;

    const CString root = GetRepositoryRootAndUUID(path, false, uuid);

    const char* svnPath = path.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(path));
    SVNTRACE(
        m_err = svn_client_relocate2(
            svnPath,
            from.GetSVNApiPath(subPool),
            to.GetSVNApiPath(subPool),
            !includeExternals,
            m_pCtx, subPool),
        svnPath);

    ClearCAPIAuthCacheOnError();
    if (m_err == nullptr)
    {
        GetLogCachePool()->DropCache(uuid, root);
    }

    return (m_err == nullptr);
}

bool SVN::IsRepository(const CTSVNPath& path)
{
    Prepare();
    SVNPool subPool(m_pool);

    // The URL we get here is per definition properly encoded and escaped.
    const char* rootPath = svn_repos_find_root_path(path.GetSVNApiPath(subPool), subPool);
    if (rootPath)
    {
        svn_repos_t* pRepos = nullptr;
        m_err               = svn_repos_open3(&pRepos, rootPath, nullptr, subPool, subPool);
        if ((m_err) && (m_err->apr_err == SVN_ERR_FS_BERKELEY_DB))
            return true;
        if (m_err == nullptr)
            return true;
    }

    return false;
}

CString SVN::GetRepositoryRootAndUUID(const CTSVNPath& path, bool useLogCache, CString& sUuid)
{
    if (useLogCache && IsLogCacheEnabled())
        return GetLogCachePool()->GetRepositoryInfo().GetRepositoryRootAndUUID(path, sUuid);

    const char* retUrl = nullptr;
    const char* uuid   = nullptr;

    SVNPool localPool(m_pool);
    Prepare();

    // empty the sUUID first
    sUuid.Empty();

    if (path.IsUrl())
        CHooks::Instance().PreConnect(CTSVNPathList(path));

    // make sure the url is canonical.
    const char* goodurl = path.GetSVNApiPath(localPool);
    SVNTRACE(
        m_err = svn_client_get_repos_root(&retUrl, &uuid, goodurl, m_pCtx, localPool, localPool),
        goodurl);
    ClearCAPIAuthCacheOnError();
    if (m_err == nullptr)
    {
        sUuid = CString(uuid);
    }

    return CString(retUrl);
}

CString SVN::GetRepositoryRoot(const CTSVNPath& url)
{
    CString sUuid;
    return GetRepositoryRootAndUUID(url, true, sUuid);
}

CString SVN::GetUUIDFromPath(const CTSVNPath& path)
{
    CString sUuid;
    GetRepositoryRootAndUUID(path, true, sUuid);
    return sUuid;
}

svn_revnum_t SVN::GetHEADRevision(const CTSVNPath& path, bool cacheAllowed)
{
    svn_ra_session_t* raSession = nullptr;
    const char*       urla      = nullptr;
    svn_revnum_t      rev       = -1;

    SVNPool localPool(m_pool);
    Prepare();

    // make sure the url is canonical.
    const char* svnPath = path.GetSVNApiPath(localPool);
    if (!path.IsUrl())
    {
        SVNTRACE(
            m_err = svn_client_url_from_path2(&urla, svnPath, m_pCtx, localPool, localPool),
            svnPath);
    }
    else
    {
        urla = svnPath;
    }
    if (m_err)
        return -1;

    // use cached information, if allowed

    if (cacheAllowed && LogCache::CSettings::GetEnabled())
    {
        // look up in cached repository properties
        // (missing entries will be added automatically)

        CTSVNPath canonicalURL;
        canonicalURL.SetFromSVN(urla);

        CRepositoryInfo& cachedProperties = GetLogCachePool()->GetRepositoryInfo();
        CString          uuid             = cachedProperties.GetRepositoryUUID(canonicalURL);
        return cachedProperties.GetHeadRevision(uuid, canonicalURL);
    }
    else
    {
        // non-cached access

        CHooks::Instance().PreConnect(CTSVNPathList(path));
        /* use subpool to create a temporary RA session */
        SVNTRACE(
            m_err = svn_client_open_ra_session2(&raSession, urla, path.IsUrl() ? NULL : svnPath, m_pCtx, localPool, localPool),
            urla);
        ClearCAPIAuthCacheOnError();
        if (m_err)
            return -1;

        SVNTRACE(
            m_err = svn_ra_get_latest_revnum(raSession, &rev, localPool),
            urla);
        ClearCAPIAuthCacheOnError();
        if (m_err)
            return -1;
        return rev;
    }
}

bool SVN::GetRootAndHead(const CTSVNPath& path, CTSVNPath& url, svn_revnum_t& rev)
{
    svn_ra_session_t* raSession = nullptr;
    const char*       urla      = nullptr;
    const char*       retUrl    = nullptr;

    SVNPool localPool(m_pool);
    Prepare();

    // make sure the url is canonical.
    const char* svnPath = path.GetSVNApiPath(localPool);
    if (!path.IsUrl())
    {
        SVNTRACE(
            m_err = svn_client_url_from_path2(&urla, svnPath, m_pCtx, localPool, localPool),
            svnPath);
    }
    else
    {
        urla = svnPath;
    }

    if (m_err)
        return false;

    // use cached information, if allowed

    if (LogCache::CSettings::GetEnabled())
    {
        // look up in cached repository properties
        // (missing entries will be added automatically)

        CRepositoryInfo& cachedProperties = GetLogCachePool()->GetRepositoryInfo();
        CString          uuid;
        url.SetFromSVN(cachedProperties.GetRepositoryRootAndUUID(path, uuid));
        if (url.IsEmpty())
        {
            assert(m_err != NULL);
        }
        else
        {
            rev = cachedProperties.GetHeadRevision(uuid, path);
            if ((rev == NO_REVISION) && (m_err == nullptr))
            {
                CHooks::Instance().PreConnect(CTSVNPathList(path));
                SVNTRACE(
                    m_err = svn_client_open_ra_session2(&raSession, urla, path.IsUrl() ? NULL : svnPath, m_pCtx, localPool, localPool),
                    urla);
                ClearCAPIAuthCacheOnError();
                if (m_err)
                    return false;

                SVNTRACE(
                    m_err = svn_ra_get_latest_revnum(raSession, &rev, localPool),
                    urla);
                ClearCAPIAuthCacheOnError();
                if (m_err)
                    return false;
            }
        }

        return (m_err == nullptr);
    }
    else
    {
        // non-cached access

        CHooks::Instance().PreConnect(CTSVNPathList(path));
        /* use subpool to create a temporary RA session */

        SVNTRACE(
            m_err = svn_client_open_ra_session2(&raSession, urla, path.IsUrl() ? NULL : svnPath, m_pCtx, localPool, localPool),
            urla);
        ClearCAPIAuthCacheOnError();
        if (m_err)
            return FALSE;

        SVNTRACE(
            m_err = svn_ra_get_latest_revnum(raSession, &rev, localPool),
            urla);
        ClearCAPIAuthCacheOnError();
        if (m_err)
            return FALSE;

        SVNTRACE(
            m_err = svn_ra_get_repos_root2(raSession, &retUrl, localPool),
            urla);
        ClearCAPIAuthCacheOnError();
        if (m_err)
            return FALSE;

        url.SetFromSVN(CUnicodeUtils::GetUnicode(retUrl));
    }

    return true;
}

bool SVN::GetLocks(const CTSVNPath& url, std::map<CString, SVNLock>* locks)
{
    svn_ra_session_t* raSession = nullptr;

    SVNPool localPool(m_pool);
    Prepare();

    apr_hash_t* hash = apr_hash_make(localPool);

    /* use subpool to create a temporary RA session */
    const char* svnPath = url.GetSVNApiPath(localPool);
    CHooks::Instance().PreConnect(CTSVNPathList(url));
    SVNTRACE(
        m_err = svn_client_open_ra_session2(&raSession, svnPath, NULL, m_pCtx, localPool, localPool),
        svnPath);
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
        return false;

    SVNTRACE(
        m_err = svn_ra_get_locks2(raSession, &hash, "", svn_depth_infinity, localPool),
        svnPath)
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
        return false;
    apr_hash_index_t* hi  = nullptr;
    svn_lock_t*       val = nullptr;
    const char*       key = nullptr;
    for (hi = apr_hash_first(localPool, hash); hi; hi = apr_hash_next(hi))
    {
        apr_hash_this(hi, reinterpret_cast<const void**>(&key), nullptr, reinterpret_cast<void**>(&val));
        if (val)
        {
            SVNLock lock;
            lock.comment        = CUnicodeUtils::GetUnicode(val->comment);
            lock.creationDate   = val->creation_date / 1000000L;
            lock.expirationDate = val->expiration_date / 1000000L;
            lock.owner          = CUnicodeUtils::GetUnicode(val->owner);
            lock.path           = CUnicodeUtils::GetUnicode(val->path);
            lock.token          = CUnicodeUtils::GetUnicode(val->token);
            CString sKey        = CUnicodeUtils::GetUnicode(key);
            (*locks)[sKey]      = lock;
        }
    }
    return true;
}

bool SVN::GetWCRevisionStatus(const CTSVNPath& wcPath, bool bCommitted, svn_revnum_t& minRev, svn_revnum_t& maxRev, bool& switched, bool& modified, bool& sparse)
{
    SVNPool localPool(m_pool);
    Prepare();

    svn_wc_revision_status_t* revStatus = nullptr;
    const char*               svnPath   = wcPath.GetSVNApiPath(localPool);
    SVNTRACE(
        m_err = svn_wc_revision_status2(&revStatus, m_pCtx->wc_ctx, svnPath, NULL, bCommitted, SVN::cancel, this, localPool, localPool),
        svnPath)

    if ((m_err) || (revStatus == nullptr))
    {
        minRev   = 0;
        maxRev   = 0;
        switched = false;
        modified = false;
        sparse   = false;
        return false;
    }
    minRev   = revStatus->min_rev;
    maxRev   = revStatus->max_rev;
    switched = !!revStatus->switched;
    modified = !!revStatus->modified;
    sparse   = !!revStatus->sparse_checkout;
    return true;
}

bool SVN::GetWCMinMaxRevs(const CTSVNPath& wcPath, bool committed, svn_revnum_t& minRev, svn_revnum_t& maxRev)
{
    SVNPool localPool(m_pool);
    Prepare();

    const char* svnPath = wcPath.GetSVNApiPath(localPool);
    SVNTRACE(
        m_err = svn_client_min_max_revisions(&minRev, &maxRev, svnPath, committed, m_pCtx, localPool),
        svnPath)

    return (m_err == nullptr);
}

bool SVN::Upgrade(const CTSVNPath& wcpath)
{
    SVNPool localPool(m_pool);
    Prepare();

    m_err = svn_client_upgrade(wcpath.GetSVNApiPath(localPool), m_pCtx, localPool);

    return (m_err == nullptr);
}

svn_revnum_t SVN::RevPropertySet(const CString& sName, const CString& sValue, const CString& sOldValue, const CTSVNPath& url, const SVNRev& rev)
{
    svn_revnum_t  setRev = {};
    svn_string_t* pVal   = nullptr;
    svn_string_t* pVal2  = nullptr;
    Prepare();

    CStringA sValueA = CUnicodeUtils::GetUTF8(sValue);
    sValueA.Remove('\r');
    SVNPool subPool(m_pool);
    pVal = svn_string_create(sValueA, subPool);
    if (!sOldValue.IsEmpty())
        pVal2 = svn_string_create(CUnicodeUtils::GetUTF8(sOldValue), subPool);

    const char* svnPath = url.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(url));
    SVNTRACE(
        m_err = svn_client_revprop_set2(CUnicodeUtils::GetUTF8(sName),
                                        pVal,
                                        pVal2,
                                        svnPath,
                                        rev,
                                        &setRev,
                                        FALSE,
                                        m_pCtx,
                                        subPool),
        svnPath);
    ClearCAPIAuthCacheOnError();
    if (m_err)
        return 0;
    return setRev;
}

CString SVN::RevPropertyGet(const CString& sName, const CTSVNPath& url, const SVNRev& rev)
{
    svn_string_t* propVal = nullptr;
    svn_revnum_t  setRev  = {};
    Prepare();

    SVNPool     subPool(m_pool);
    const char* svnPath = url.GetSVNApiPath(subPool);
    CHooks::Instance().PreConnect(CTSVNPathList(url));
    SVNTRACE(
        m_err = svn_client_revprop_get(CUnicodeUtils::GetUTF8(sName), &propVal, svnPath, rev, &setRev, m_pCtx, subPool),
        svnPath);
    ClearCAPIAuthCacheOnError();
    if (m_err)
        return L"";
    if (propVal == nullptr)
        return L"";
    if (propVal->len == 0)
        return L"";
    return CUnicodeUtils::GetUnicode(propVal->data);
}

CTSVNPath SVN::GetPristinePath(const CTSVNPath& wcPath)
{
    SVNPool localPool;

    const char* pristinePath = nullptr;
    CTSVNPath   returnPath;

#pragma warning(push)
#pragma warning(disable : 4996) // deprecated warning
    // note: the 'new' function would be svn_wc_get_pristine_contents(), but that
    // function returns a stream instead of a path. Since we don't need a stream
    // but really a path here, that function is of no use and would require us
    // to create a temp file and copy the original contents to that temp file.
    //
    // We can't pass a stream to e.g. TortoiseMerge for diffing, that's why we
    // need a *path* and not a stream.
    auto err = svn_wc_get_pristine_copy_path(svn_path_internal_style(wcPath.GetSVNApiPath(localPool), localPool), &pristinePath, localPool);
#pragma warning(pop)

    if (err != nullptr)
    {
        svn_error_clear(err);
        return returnPath;
    }
    if (pristinePath != nullptr)
    {
        returnPath.SetFromSVN(pristinePath);
    }
    return returnPath;
}

bool SVN::PathIsURL(const CTSVNPath& path)
{
    return !!svn_path_is_url(CUnicodeUtils::GetUTF8(path.GetSVNPathString()));
}

void SVN::formatDate(wchar_t dateNative[], apr_time_t dateSvn, bool forceShortFmt)
{
    if (dateSvn == 0)
    {
        wcscpy_s(dateNative, SVN_DATE_BUFFER, L"(no date)");
        return;
    }

    FILETIME ft = {0};
    AprTimeToFileTime(&ft, dateSvn);
    formatDate(dateNative, ft, forceShortFmt);
}

void SVN::formatDate(wchar_t dateNative[], FILETIME& fileTime, bool forceShortFmt)
{
    enum
    {
        CACHE_SIZE = 16
    };
    static async::CCriticalSection mutex;
    static FILETIME                lastTime[CACHE_SIZE] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    static wchar_t                 lastResult[CACHE_SIZE][SVN_DATE_BUFFER];
    static bool                    formats[CACHE_SIZE];

    // we have to serialize access to the cache

    async::CCriticalSectionLock lock(mutex);

    // cache lookup

    wchar_t* result = nullptr;
    for (size_t i = 0; i < CACHE_SIZE; ++i)
        if ((lastTime[i].dwHighDateTime == fileTime.dwHighDateTime) && (lastTime[i].dwLowDateTime == fileTime.dwLowDateTime) && (formats[i] == forceShortFmt))
        {
            result = lastResult[i];
            break;
        }

    // set cache to new data, if old is no hit

    if (result == nullptr)
    {
        // evict an entry from the cache

        static size_t victim = 0;
        lastTime[victim]     = fileTime;
        result               = lastResult[victim];
        formats[victim]      = forceShortFmt;
        if (++victim == CACHE_SIZE)
            victim = 0;

        result[0] = '\0';

        // Convert UTC to local time
        SYSTEMTIME systemTime = {};
        VERIFY(FileTimeToSystemTime(&fileTime, &systemTime));

        static TIME_ZONE_INFORMATION timeZone = {-1};
        if (timeZone.Bias == -1)
            GetTimeZoneInformation(&timeZone);

        SYSTEMTIME localSystime = {};
        VERIFY(SystemTimeToTzSpecificLocalTime(&timeZone, &systemTime, &localSystime));

        wchar_t timeBuf[SVN_DATE_BUFFER] = {0};
        wchar_t dateBuf[SVN_DATE_BUFFER] = {0};

        LCID locale = m_sUseSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : m_sLocale;

        /// reusing this instance is vital for \ref formatDate performance

        static CRegDWORD logDateFormat(500, L"Software\\TortoiseSVN\\LogDateFormat");
        DWORD            flags = forceShortFmt || (logDateFormat == 1)
                                     ? DATE_SHORTDATE
                                     : DATE_LONGDATE;

        GetDateFormat(locale, flags, &localSystime, nullptr, dateBuf, SVN_DATE_BUFFER);
        GetTimeFormat(locale, 0, &localSystime, nullptr, timeBuf, SVN_DATE_BUFFER);
        wcsncat_s(result, SVN_DATE_BUFFER, dateBuf, SVN_DATE_BUFFER);
        wcsncat_s(result, SVN_DATE_BUFFER, L" ", SVN_DATE_BUFFER);
        wcsncat_s(result, SVN_DATE_BUFFER, timeBuf, SVN_DATE_BUFFER);
    }

    // copy formatted string to result

    wcsncpy_s(dateNative, SVN_DATE_BUFFER, result, SVN_DATE_BUFFER - 1);
}

bool SVN::AprTimeExplodeLocal(apr_time_exp_t* explodedTime, apr_time_t dateSvn)
{
    // apr_time_exp_lt() can crash because it does not check the return values of APIs it calls.
    // Put the call to apr_time_exp_lt() in a __try/__except to avoid those crashes.
    __try
    {
        return apr_time_exp_lt(explodedTime, dateSvn) == APR_SUCCESS;
    }
    __except (TRUE)
    {
        return false;
    }
}

bool SVN::IsLogCacheEnabled()
{
    return GetLogCachePool()->IsEnabled() || m_bAssumeCacheEnabled;
}

CString SVN::formatDate(apr_time_t dateSvn)
{
    apr_time_exp_t explodedTime = {0};

    SYSTEMTIME sysTime                  = {0, 0, 0, 0, 0, 0, 0, 0};
    wchar_t    dateBuf[SVN_DATE_BUFFER] = {0};

    LCID locale = m_sUseSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : m_sLocale;

    if (!AprTimeExplodeLocal(&explodedTime, dateSvn))
        return L"(no date)";

    sysTime.wDay       = static_cast<WORD>(explodedTime.tm_mday);
    sysTime.wDayOfWeek = static_cast<WORD>(explodedTime.tm_wday);
    sysTime.wMonth     = static_cast<WORD>(explodedTime.tm_mon) + 1;
    sysTime.wYear      = static_cast<WORD>(explodedTime.tm_year) + 1900;

    GetDateFormat(locale, DATE_SHORTDATE, &sysTime, nullptr, dateBuf, SVN_DATE_BUFFER);

    return dateBuf;
}

CString SVN::formatTime(apr_time_t dateSvn)
{
    apr_time_exp_t explodedTime = {0};

    SYSTEMTIME sysTime                  = {0, 0, 0, 0, 0, 0, 0, 0};
    wchar_t    timeBuf[SVN_DATE_BUFFER] = {0};

    LCID locale = m_sUseSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : m_sLocale;

    if (!AprTimeExplodeLocal(&explodedTime, dateSvn))
        return L"(no time)";

    sysTime.wDay          = static_cast<WORD>(explodedTime.tm_mday);
    sysTime.wDayOfWeek    = static_cast<WORD>(explodedTime.tm_wday);
    sysTime.wHour         = static_cast<WORD>(explodedTime.tm_hour);
    sysTime.wMilliseconds = static_cast<WORD>(explodedTime.tm_usec / 1000);
    sysTime.wMinute       = static_cast<WORD>(explodedTime.tm_min);
    sysTime.wMonth        = static_cast<WORD>(explodedTime.tm_mon) + 1;
    sysTime.wSecond       = static_cast<WORD>(explodedTime.tm_sec);
    sysTime.wYear         = static_cast<WORD>(explodedTime.tm_year) + 1900;

    GetTimeFormat(locale, 0, &sysTime, nullptr, timeBuf, SVN_DATE_BUFFER);

    return timeBuf;
}

CString SVN::MakeUIUrlOrPath(const CStringA& urlOrPath)
{
    CString url;
    if (svn_path_is_url(urlOrPath))
    {
        size_t length = urlOrPath.GetLength();
        url           = CUnicodeUtils::GetUnicode(CPathUtils::ContainsEscapedChars(urlOrPath, length)
                                                      ? CPathUtils::PathUnescape(urlOrPath)
                                                      : urlOrPath);
    }
    else
        url = CUnicodeUtils::GetUnicode(urlOrPath);

    return url;
}

std::string SVN::MakeUIUrlOrPath(const char* urlOrPath)
{
    if (!svn_path_is_url(urlOrPath))
        return urlOrPath;

    std::string result = urlOrPath;
    if (CPathUtils::ContainsEscapedChars(result.c_str(), result.length()))
        CPathUtils::Unescape(&result[0]);

    return result;
}

bool SVN::EnsureConfigFile()
{
    SVNPool localPool;
    auto    err = svn_config_ensure(nullptr, localPool);
    if (err)
    {
        svn_error_clear(err);
        return false;
    }
    return true;
}

CString SVN::GetOptionsString(bool bIgnoreEOL, bool bIgnoreSpaces, bool bIgnoreAllSpaces)
{
    if (bIgnoreAllSpaces)
        return GetOptionsString(bIgnoreEOL, svn_diff_file_ignore_space_all);
    else if (bIgnoreSpaces)
        return GetOptionsString(bIgnoreEOL, svn_diff_file_ignore_space_change);
    else
        return GetOptionsString(bIgnoreEOL, svn_diff_file_ignore_space_none);
}

CString SVN::GetOptionsString(bool bIgnoreEOL, svn_diff_file_ignore_space_t space)
{
    SVNDiffOptions opts;
    opts.SetIgnoreEOL(bIgnoreEOL);
    opts.SetIgnoreSpace(space);

    return opts;
}

/**
 * Note that this is TSVN's global CLogCachePool creation /
 * destruction mutex. Don't access logCachePool or create a
 * CLogCachePool directly anywhere else in TSVN.
 */
async::CCriticalSection& SVN::GetLogCachePoolMutex()
{
    static async::CCriticalSection mutex;
    return mutex;
}

/**
 * Returns the log cache pool singleton. You will need that to
 * create \c CCacheLogQuery instances.
 */
LogCache::CLogCachePool* SVN::GetLogCachePool()
{
    // modifying the log cache is not thread safe.
    // In particular, we must synchronize the loading & file check.

    async::CCriticalSectionLock lock(GetLogCachePoolMutex());

    if (logCachePool.get() == nullptr)
    {
        CString cacheFolder = CPathUtils::GetAppDataDirectory() + L"logcache\\";
        logCachePool.reset(new LogCache::CLogCachePool(*this, cacheFolder));
    }

    return logCachePool.get();
}

/**
 * Close the logCachePool.
 */
void SVN::ResetLogCachePool()
{
    // this should be called by the ~SVN only but we make sure that
    // (illegal) concurrent access won't see zombie objects.

    async::CCriticalSectionLock lock(GetLogCachePoolMutex());
    if (logCachePool.get() != nullptr)
        logCachePool->Flush();

    logCachePool.reset();
}

/**
 * Returns the status of the encapsulated \ref SVNPrompt instance.
 */
bool SVN::PromptShown() const
{
    return m_prompt.PromptShown();
}

/**
 * Set the parent window of an authentication prompt dialog
 */
void SVN::SetPromptParentWindow(HWND hWnd)
{
    m_prompt.SetParentWindow(hWnd);
}
/**
 * Set the MFC Application object for a prompt dialog
 */
void SVN::SetPromptApp(CWinApp* pWinApp)
{
    m_prompt.SetApp(pWinApp);
}

apr_array_header_t* SVN::MakeCopyArray(const CTSVNPathList& pathList, const SVNRev& rev, const SVNRev& pegRev) const
{
    apr_array_header_t* sources = apr_array_make(m_pool, pathList.GetCount(),
                                                 sizeof(svn_client_copy_source_t*));

    for (int nItem = 0; nItem < pathList.GetCount(); ++nItem)
    {
        const char*               target                   = apr_pstrdup(m_pool, pathList[nItem].GetSVNApiPath(m_pool));
        svn_client_copy_source_t* source                   = static_cast<svn_client_copy_source_t*>(apr_palloc(m_pool, sizeof(*source)));
        source->path                                       = target;
        source->revision                                   = rev;
        source->peg_revision                               = pegRev;
        APR_ARRAY_PUSH(sources, svn_client_copy_source_t*) = source;
    }
    return sources;
}

apr_array_header_t* SVN::MakeChangeListArray(const CStringArray& changeLists, apr_pool_t* pool)
{
    // passing NULL if there are no change lists will work only partially: the subversion library
    // in that case executes the command, but fails to remove the existing change lists from the files
    // if 'keep change lists is set to false.
    // We therefore have to create an empty array instead of passing NULL, only then the
    // change lists are removed properly.
    int count = static_cast<int>(changeLists.GetCount());
    // special case: the changelist array contains one empty string
    if ((changeLists.GetCount() == 1) && (changeLists[0].IsEmpty()))
        count = 0;

    apr_array_header_t* arr = apr_array_make(pool, count, sizeof(const char*));

    if (count == 0)
        return arr;

    if (!changeLists.IsEmpty())
    {
        for (int nItem = 0; nItem < changeLists.GetCount(); nItem++)
        {
            const char* c                                     = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(changeLists[nItem]));
            (*static_cast<const char**>(apr_array_push(arr))) = c;
        }
    }
    return arr;
}

apr_hash_t* SVN::MakeRevPropHash(const RevPropHash& revProps, apr_pool_t* pool)
{
    apr_hash_t* revpropTable = nullptr;
    if (!revProps.empty())
    {
        revpropTable = apr_hash_make(pool);
        for (RevPropHash::const_iterator it = revProps.begin(); it != revProps.end(); ++it)
        {
            svn_string_t* propval = svn_string_create(CUnicodeUtils::GetUTF8(it->second), pool);
            apr_hash_set(revpropTable, apr_pstrdup(pool, CUnicodeUtils::GetUTF8(it->first)), APR_HASH_KEY_STRING, propval);
        }
    }

    return revpropTable;
}

svn_error_t* SVN::commitCallback2(const svn_commit_info_t* commitInfo, void* baton, apr_pool_t* localPool)
{
    if (commitInfo)
    {
        SVN* pThis         = static_cast<SVN*>(baton);
        pThis->m_commitRev = commitInfo->revision;
        if (SVN_IS_VALID_REVNUM(commitInfo->revision))
        {
            pThis->Notify(CTSVNPath(), CTSVNPath(), svn_wc_notify_update_completed, svn_node_none, L"",
                          svn_wc_notify_state_unknown, svn_wc_notify_state_unknown,
                          commitInfo->revision, nullptr, svn_wc_notify_lock_state_unchanged,
                          L"", L"", nullptr, nullptr, localPool);
        }
        if (commitInfo->post_commit_err)
        {
            pThis->m_postCommitErr = CUnicodeUtils::GetUnicode(commitInfo->post_commit_err);
        }
    }
    return nullptr;
}

void SVN::SetAndClearProgressInfo(HWND hWnd)
{
    m_progressWnd          = hWnd;
    m_pProgressDlg         = nullptr;
    m_progressTotal        = 0;
    m_progressLastProgress = 0;
    m_progressLastTotal    = 0;
    m_progressLastTicks    = GetTickCount64();
}

void SVN::SetAndClearProgressInfo(CProgressDlg* pProgressDlg, bool bShowProgressBar /* = false*/)
{
    m_progressWnd          = nullptr;
    m_pProgressDlg         = pProgressDlg;
    m_progressTotal        = 0;
    m_progressLastProgress = 0;
    m_progressLastTotal    = 0;
    m_progressLastTicks    = GetTickCount64();
    m_bShowProgressBar     = bShowProgressBar;
}

CString SVN::GetSummarizeActionText(svn_client_diff_summarize_kind_t kind)
{
    CString sAction;
    switch (kind)
    {
        case svn_client_diff_summarize_kind_normal:
            sAction.LoadString(IDS_SVN_SUMMARIZENORMAL);
            break;
        case svn_client_diff_summarize_kind_added:
            sAction.LoadString(IDS_SVN_SUMMARIZEADDED);
            break;
        case svn_client_diff_summarize_kind_modified:
            sAction.LoadString(IDS_SVN_SUMMARIZEMODIFIED);
            break;
        case svn_client_diff_summarize_kind_deleted:
            sAction.LoadString(IDS_SVN_SUMMARIZEDELETED);
            break;
    }
    return sAction;
}

void SVN::progress_func(apr_off_t progress, apr_off_t total, void* baton, apr_pool_t* /*pool*/)
{
    SVN* pSVN = static_cast<SVN*>(baton);
    if ((pSVN == nullptr) || ((pSVN->m_progressWnd == nullptr) && (pSVN->m_pProgressDlg == nullptr)))
        return;
    apr_off_t delta = progress;
    if ((progress >= pSVN->m_progressLastProgress) && ((total == pSVN->m_progressLastTotal) || (total < 0)))
        delta = progress - pSVN->m_progressLastProgress;

    pSVN->m_progressLastProgress = progress;
    pSVN->m_progressLastTotal    = total;

    ULONGLONG ticks = GetTickCount64();
    pSVN->m_progressVector.push_back(delta);
    pSVN->m_progressTotal += delta;
    //ATLTRACE("progress = %I64d, total = %I64d, delta = %I64d, overall total is : %I64d\n", progress, total, delta, pSVN->progress_total);
    if ((pSVN->m_progressLastTicks + 1000UL) < ticks)
    {
        double divBy = (static_cast<double>(ticks - pSVN->m_progressLastTicks) / 1000.0);
        if (divBy < 0.0001)
            divBy = 1;
        pSVN->m_svnProgressMsg.overallTotal = pSVN->m_progressTotal;
        pSVN->m_svnProgressMsg.progress     = progress;
        pSVN->m_svnProgressMsg.total        = total;
        pSVN->m_progressLastTicks           = ticks;
        apr_off_t average                   = 0;
        for (std::vector<apr_off_t>::iterator it = pSVN->m_progressVector.begin(); it != pSVN->m_progressVector.end(); ++it)
        {
            average += *it;
        }
        average = static_cast<apr_off_t>(static_cast<double>(average) / divBy);
        if (average >= 0x100000000LL)
        {
            // it seems that sometimes we get ridiculous numbers here.
            // Anyone *really* having more than 4GB/sec throughput?
            average = 0;
        }

        pSVN->m_svnProgressMsg.bytesPerSecond = average;
        if (average < 1024LL)
            pSVN->m_svnProgressMsg.speedString.Format(IDS_SVN_PROGRESS_BYTES_SEC, average);
        else
        {
            double averagekb = static_cast<double>(average) / 1024.0;
            pSVN->m_svnProgressMsg.speedString.Format(IDS_SVN_PROGRESS_KBYTES_SEC, averagekb);
        }
        CString s;
        s.Format(IDS_SVN_PROGRESS_SPEED, static_cast<LPCWSTR>(pSVN->m_svnProgressMsg.speedString));
        pSVN->m_svnProgressMsg.speedString = s;
        if (pSVN->m_progressWnd)
            SendMessage(pSVN->m_progressWnd, WM_SVNPROGRESS, 0, reinterpret_cast<LPARAM>(&pSVN->m_svnProgressMsg));
        else if (pSVN->m_pProgressDlg)
        {
            if ((pSVN->m_bShowProgressBar && (progress > 1000LL) && (total > 0LL)))
                pSVN->m_pProgressDlg->SetProgress64(progress, total);

            CString sTotal;
            CString temp;
            if (pSVN->m_svnProgressMsg.overallTotal < 1024LL)
                sTotal.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, pSVN->m_svnProgressMsg.overallTotal);
            else if (pSVN->m_svnProgressMsg.overallTotal < 1200000LL)
                sTotal.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, pSVN->m_svnProgressMsg.overallTotal / 1024LL);
            else
                sTotal.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, static_cast<double>(pSVN->m_svnProgressMsg.overallTotal) / 1024000.0);
            temp.FormatMessage(IDS_SVN_PROGRESS_TOTALANDSPEED, static_cast<LPCWSTR>(sTotal), static_cast<LPCWSTR>(pSVN->m_svnProgressMsg.speedString));

            pSVN->m_pProgressDlg->SetLine(2, temp);
        }
        pSVN->m_progressVector.clear();
    }
    return;
}

void SVN::CallPreConnectHookIfUrl(const CTSVNPathList& pathList, const CTSVNPath& path /* = CTSVNPath()*/)
{
    if (pathList.GetCount())
    {
        if (pathList[0].IsUrl())
            CHooks::Instance().PreConnect(pathList);
        else if (path.IsUrl())
            CHooks::Instance().PreConnect(pathList);
    }
}

CString SVN::GetChecksumString(svn_checksum_kind_t type, const CString& s, apr_pool_t* localPool)
{
    svn_checksum_t* checksum;
    CStringA        sa = CUnicodeUtils::GetUTF8(s);
    svn_checksum(&checksum, type, sa, sa.GetLength(), localPool);
    const char* hexname = svn_checksum_to_cstring(checksum, localPool);
    CString     hex     = CUnicodeUtils::GetUnicode(hexname);
    return hex;
}

CString SVN::GetChecksumString(svn_checksum_kind_t type, const CString& s) const
{
    return GetChecksumString(type, s, m_pool);
}

void SVN::Prepare()
{
    svn_error_clear(m_err);
    m_err = nullptr;
    m_redirectedUrl.Reset();
    m_postCommitErr.Empty();
}

void SVN::SetAuthInfo(const CString& username, const CString& password) const
{
    if (m_pCtx)
    {
        svn_auth_forget_credentials(m_pCtx->auth_baton, nullptr, nullptr, m_pool);
        svn_auth_set_parameter(m_pCtx->auth_baton, SVN_AUTH_PARAM_DONT_STORE_PASSWORDS, "");
        svn_auth_set_parameter(m_pCtx->auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, "");
        if (!username.IsEmpty())
        {
            svn_auth_set_parameter(m_pCtx->auth_baton,
                                   SVN_AUTH_PARAM_DEFAULT_USERNAME, apr_pstrdup(m_pool, CUnicodeUtils::GetUTF8(username)));
        }
        if (!password.IsEmpty())
        {
            svn_auth_set_parameter(m_pCtx->auth_baton,
                                   SVN_AUTH_PARAM_DEFAULT_PASSWORD, apr_pstrdup(m_pool, CUnicodeUtils::GetUTF8(password)));
        }
    }
}

svn_error_t* svnErrorHandleMalfunction(svn_boolean_t canReturn,
                                       const char* file, int line,
                                       const char* expr)
{
    // we get here every time Subversion encounters something very unexpected.
    // in previous versions, Subversion would just call abort() - now we can
    // show the user some information before we return.
    svn_error_t* err = svn_error_raise_on_malfunction(TRUE, file, line, expr);

    CString sErr(MAKEINTRESOURCE(IDS_ERR_SVNEXCEPTION));
    if (err)
    {
        sErr += L"\n\n" + SVN::GetErrorString(err);
        ::MessageBox(nullptr, sErr, L"Subversion Exception!", MB_ICONERROR);
        if (canReturn)
            return err;
        if (CRegDWORD(L"Software\\TortoiseSVN\\Debug", FALSE) == FALSE)
        {
            CCrashReport::Instance().AddUserInfoToReport(L"SVNException", sErr);
            CCrashReport::Instance().Uninstall();
            abort(); // ugly, ugly! But at least we showed a messagebox first
        }
    }

    CString sFormatErr;
    sFormatErr.FormatMessage(IDS_ERR_SVNFORMATEXCEPTION, static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(file)), line, static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(expr)));
    ::MessageBox(nullptr, sFormatErr, L"Subversion Exception!", MB_ICONERROR);
    if (CRegDWORD(L"Software\\TortoiseSVN\\Debug", FALSE) == FALSE)
    {
        CCrashReport::Instance().AddUserInfoToReport(L"SVNException", sFormatErr);
        CCrashReport::Instance().Uninstall();
        abort(); // ugly, ugly! But at least we showed a messagebox first
    }
    return nullptr; // never reached, only to silence compiler warning
}
