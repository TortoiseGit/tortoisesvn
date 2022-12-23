// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016-2018, 2020-2022 - TortoiseSVN

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
#include "resource.h"
#include "SVNConflictInfo.h"
#include "SVNConfig.h"
#include "SVNHelpers.h"
#include "SVNTrace.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "TempFile.h"

SVNConflictOption::SVNConflictOption(
    svn_client_conflict_option_t *  option,
    svn_client_conflict_option_id_t id,
    const CString &                 label,
    const CString &                 description,
    int                             preferredMovedTargetIdx,
    int                             preferredMovedReltargetIdx)
    : m_option(option)
    , m_id(id)
    , m_preferredMovedTargetIdx(preferredMovedTargetIdx)
    , m_preferredMovedReltargetIdx(preferredMovedReltargetIdx)
    , m_label(label)
    , m_description(description)
{
}

void SVNConflictOption::SetMergedPropVal(const svn_string_t *propVal) const
{
    svn_client_conflict_option_set_merged_propval(m_option, propVal);
}

svn_error_t *SVNConflictOption::SetMergedPropValFile(const CTSVNPath &filePath, apr_pool_t *pool) const
{
    svn_stringbuf_t *propVal;

    SVN_ERR(svn_stringbuf_from_file2(&propVal, filePath.GetSVNApiPath(pool), pool));
    SetMergedPropVal(svn_string_create_from_buf(propVal, pool));

    return nullptr;
}

SVNConflictOptions::SVNConflictOptions()
{
    m_pool = svn_pool_create(nullptr);
}

SVNConflictOptions::~SVNConflictOptions()
{
    svn_pool_destroy(m_pool);
}

SVNConflictOption *SVNConflictOptions::FindOptionById(svn_client_conflict_option_id_t id)
{
    for (std::unique_ptr<SVNConflictOption> &opt : *this)
    {
        if (opt->GetId() == id)
        {
            return opt.get();
        }
    }

    return nullptr;
}

SVNConflictInfo::SVNConflictInfo()
    : m_pool(nullptr)
    , m_infoPool(nullptr)
    , m_conflict(nullptr)
    , m_propConflicts(nullptr)
{
    m_pool     = svn_pool_create(nullptr);
    m_infoPool = svn_pool_create(m_pool);
    svn_error_clear(svn_client_create_context2(&m_pCtx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

    m_prompt.Init(m_pool, m_pCtx);

    m_pCtx->cancel_func   = cancelCallback;
    m_pCtx->cancel_baton  = this;
    m_pCtx->client_name   = SVNHelper::GetUserAgentString(m_pool);
    m_pCtx->notify_func2  = notifyCallback;
    m_pCtx->notify_baton2 = this;
}

SVNConflictInfo::~SVNConflictInfo()
{
    ClearSVNError();
    svn_pool_destroy(m_pool);
}

bool SVNConflictInfo::Get(const CTSVNPath &path)
{
    ClearSVNError();
    // Clear existing conflict info.
    svn_pool_clear(m_infoPool);
    m_conflict = nullptr;

    SVNPool     scratchPool(m_pool);
    const char *svnPath = path.GetSVNApiPath(scratchPool);

    SVNTRACE(
        m_err = svn_client_conflict_get(&m_conflict, svnPath, m_pCtx, m_infoPool, scratchPool),
        svnPath);
    if (m_err != nullptr && m_err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
        svn_error_clear(m_err);
        m_err            = nullptr;
        m_path           = path;
        m_textConflicted = FALSE;
        m_propConflicts  = apr_array_make(m_infoPool, 0, sizeof(const char *));
        m_treeConflicted = FALSE;
        m_incomingChangeSummary.Empty();
        m_detailedIncomingChangeSummary.Empty();
        m_localChangeSummary.Empty();
        return true;
    }
    else if (m_err != nullptr)
    {
        return false;
    }

    m_path = path;

    SVNTRACE(
        m_err = svn_client_conflict_get_conflicted(&m_textConflicted, &m_propConflicts, &m_treeConflicted, m_conflict, m_infoPool, scratchPool),
        svnPath);

    if (m_err != nullptr)
        return false;

    if (m_treeConflicted)
    {
        const char *incomingChange = nullptr;
        const char *localChange    = nullptr;
        SVNTRACE(
            m_err = svn_client_conflict_tree_get_description(&incomingChange, &localChange, m_conflict,
                                                             m_pCtx, scratchPool, scratchPool),
            svnPath);

        if (m_err != nullptr)
            return false;

        m_incomingChangeSummary = CUnicodeUtils::GetUnicode(incomingChange);
        m_localChangeSummary    = CUnicodeUtils::GetUnicode(localChange);
    }
    if (m_propConflicts && m_propConflicts->nelts > 0)
    {
        const char *propDesc = nullptr;
        SVNTRACE(
            m_err = svn_client_conflict_prop_get_description(&propDesc, m_conflict,
                                                             scratchPool, scratchPool),
            svnPath);
        if (m_err != nullptr)
            return false;

        m_propDescription = CUnicodeUtils::GetUnicode(propDesc);
    }
    return true;
}

svn_wc_operation_t SVNConflictInfo::GetOperation() const
{
    return svn_client_conflict_get_operation(m_conflict);
}

svn_wc_conflict_action_t SVNConflictInfo::GetIncomingChange() const
{
    return svn_client_conflict_get_incoming_change(m_conflict);
}

svn_wc_conflict_reason_t SVNConflictInfo::GetLocalChange() const
{
    return svn_client_conflict_get_local_change(m_conflict);
}

svn_client_conflict_option_id_t SVNConflictInfo::GetRecommendedOptionId() const
{
    return svn_client_conflict_get_recommended_option_id(m_conflict);
}

bool SVNConflictInfo::IsBinary() const
{
    const char *mimeType = svn_client_conflict_text_get_mime_type(m_conflict);

    if (mimeType && svn_mime_type_is_binary(mimeType))
        return true;
    else
        return false;
}

int SVNConflictInfo::GetPropConflictCount() const
{
    ASSERT(m_propConflicts);
    return m_propConflicts->nelts;
}

CString SVNConflictInfo::GetPropConflictName(int idx) const
{
    ASSERT(m_propConflicts);
    ASSERT(idx < m_propConflicts->nelts);

    const char *propName = APR_ARRAY_IDX(m_propConflicts, idx, const char *);
    return CUnicodeUtils::GetUnicode(propName);
}

svn_error_t *SVNConflictInfo::createPropValFiles(const char *propName, const char *mergedFile, const char *baseFile, const char *theirFile, const char *myFile, apr_pool_t *pool) const
{
    const svn_string_t *myPropVal{};
    const svn_string_t *basePropVal{};
    const svn_string_t *theirPropVal{};
    svn_stringbuf_t *   mergedPropVal = svn_stringbuf_create_empty(pool);

    SVN_ERR(svn_client_conflict_prop_get_propvals(
        NULL, &myPropVal, &basePropVal,
        &theirPropVal, m_conflict, propName,
        pool));

    if (basePropVal)
        SVN_ERR(svn_io_write_atomic2(baseFile, basePropVal->data, basePropVal->len, NULL, FALSE, pool));

    if (theirPropVal)
        SVN_ERR(svn_io_write_atomic2(theirFile, theirPropVal->data, theirPropVal->len, NULL, FALSE, pool));

    if (myPropVal)
        SVN_ERR(svn_io_write_atomic2(myFile, myPropVal->data, myPropVal->len, NULL, FALSE, pool));

    svn_diff_file_options_t *options = svn_diff_file_options_create(pool);
    svn_diff_t *             diff;

    // If any of the property values is missing, use an empty value instead for the purpose of showing a diff.
    if (basePropVal == nullptr)
        basePropVal = svn_string_create_empty(pool);

    if (theirPropVal == nullptr)
        theirPropVal = svn_string_create_empty(pool);

    if (myPropVal == nullptr)
        myPropVal = svn_string_create_empty(pool);

    options->ignore_eol_style = TRUE;
    SVN_ERR(svn_diff_mem_string_diff3(&diff, basePropVal, myPropVal, theirPropVal, options, pool));

    SVN_ERR(svn_diff_mem_string_output_merge3(
        svn_stream_from_stringbuf(mergedPropVal, pool),
        diff, basePropVal, myPropVal, theirPropVal,
        "||||||| ORIGINAL",
        "<<<<<<< MINE",
        ">>>>>>> THEIRS",
        "=======",
        svn_diff_conflict_display_modified_original_latest,
        NULL, NULL, pool));

    SVN_ERR(svn_io_write_atomic2(mergedFile, mergedPropVal->data, mergedPropVal->len, NULL, FALSE, pool));

    return nullptr;
}

bool SVNConflictInfo::GetPropValFiles(const CString &propertyName, CTSVNPath &mergedfile,
                                      CTSVNPath &basefile, CTSVNPath &theirfile, CTSVNPath &myfile)
{
    ClearSVNError();
    mergedfile = CTempFiles::Instance().GetTempFilePath(false, GetPath());
    basefile   = CTempFiles::Instance().GetTempFilePath(false);
    theirfile  = CTempFiles::Instance().GetTempFilePath(false);
    myfile     = CTempFiles::Instance().GetTempFilePath(false);

    {
        SVNPool scratchPool(m_pool);

        const char *path = svn_client_conflict_get_local_abspath(m_conflict);
        SVNTRACE(
            m_err = createPropValFiles(CUnicodeUtils::GetUTF8(propertyName),
                                       mergedfile.GetSVNApiPath(scratchPool),
                                       basefile.GetSVNApiPath(scratchPool),
                                       theirfile.GetSVNApiPath(scratchPool),
                                       myfile.GetSVNApiPath(scratchPool),
                                       scratchPool),
            path);
    }

    if (m_err)
    {
        // Delete files on error.
        mergedfile.Delete(false);
        basefile.Delete(false);
        theirfile.Delete(false);
        myfile.Delete(false);
        return false;
    }
    return true;
}

CString SVNConflictInfo::GetPropDiff(const CString &propertyName)
{
    SVNPool scratchPool(m_pool);

    const svn_string_t *myPropVal{};
    const svn_string_t *basePropVal{};
    const svn_string_t *theirPropVal{};
    svn_stringbuf_t *   mergedPropVal = svn_stringbuf_create_empty(scratchPool);

    m_err = svn_client_conflict_prop_get_propvals(
        nullptr, &myPropVal, &basePropVal,
        &theirPropVal, m_conflict, CUnicodeUtils::GetUTF8(propertyName),
        scratchPool);
    if (m_err)
        return {};

    svn_diff_file_options_t *options = svn_diff_file_options_create(scratchPool);
    svn_diff_t *             diff;

    // If any of the property values is missing, use an empty value instead for the purpose of showing a diff.
    if (basePropVal == nullptr)
        basePropVal = svn_string_create_empty(scratchPool);

    if (theirPropVal == nullptr)
        theirPropVal = svn_string_create_empty(scratchPool);

    if (myPropVal == nullptr)
        myPropVal = svn_string_create_empty(scratchPool);

    options->ignore_eol_style = TRUE;
    m_err                     = svn_diff_mem_string_diff3(&diff, basePropVal, myPropVal, theirPropVal, options, scratchPool);
    if (m_err)
        return {};

    m_err = svn_diff_mem_string_output_merge3(
        svn_stream_from_stringbuf(mergedPropVal, scratchPool),
        diff, basePropVal, myPropVal, theirPropVal,
        "||||||| ORIGINAL",
        "<<<<<<< MINE",
        ">>>>>>> THEIRS",
        "=======",
        svn_diff_conflict_display_modified_original_latest,
        nullptr, nullptr, scratchPool);
    if (m_err)
        return {};

    CStringA propValA(mergedPropVal->data, static_cast<int>(mergedPropVal->len));
    return CUnicodeUtils::GetUnicode(propValA);
}

bool SVNConflictInfo::GetTextContentFiles(CTSVNPath &basefile, CTSVNPath &theirfile, CTSVNPath &myfile)
{
    ClearSVNError();
    SVNPool     scratchPool(m_pool);
    const char *myAbsPath    = nullptr;
    const char *baseAbsPath  = nullptr;
    const char *theirAbsPath = nullptr;

    const char *path = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        m_err = svn_client_conflict_text_get_contents(NULL, &myAbsPath,
                                                      &baseAbsPath, &theirAbsPath,
                                                      m_conflict, scratchPool, scratchPool),
        path);

    if (m_err)
        return false;

    if (baseAbsPath)
        basefile.SetFromSVN(baseAbsPath);
    else
        basefile = CTempFiles::Instance().GetTempFilePath(true);

    if (theirAbsPath)
        theirfile.SetFromSVN(theirAbsPath);
    else
        theirfile = CTempFiles::Instance().GetTempFilePath(true);

    if (myAbsPath)
        myfile.SetFromSVN(myAbsPath);
    else
        myfile.SetFromSVN(path);

    return true;
}

bool SVNConflictInfo::GetTreeResolutionOptions(SVNConflictOptions &result)
{
    ClearSVNError();
    SVNPool scratchPool(m_pool);

    apr_array_header_t *options = nullptr;
    const char *        path    = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        m_err = svn_client_conflict_tree_get_resolution_options(&options, m_conflict,
                                                                m_pCtx, result.GetPool(), scratchPool),
        path);

    if (m_err != nullptr)
        return false;

    for (int i = 0; i < options->nelts; i++)
    {
        svn_client_conflict_option_t *opt = APR_ARRAY_IDX(options, i, svn_client_conflict_option_t *);

        svn_client_conflict_option_id_t id          = svn_client_conflict_option_get_id(opt);
        const char *                    label       = svn_client_conflict_option_get_label(opt, scratchPool);
        const char *                    description = svn_client_conflict_option_get_description(opt, scratchPool);

        bool bResultAdded = false;
        // if we get only one possible path from all candidates, then we don't add
        // custom buttons but use the one that's already provided: it will be the same.
        // if we get more candidates, then we only add from the second one on, since
        // the first one is already set by the normal conflict options.
        apr_array_header_t *possibleMovedToReposRelpaths = nullptr;

        m_err = svn_client_conflict_option_get_moved_to_repos_relpath_candidates2(&possibleMovedToReposRelpaths, opt, result.GetPool(), scratchPool);
        // TODO: what should we do if the number of candidates gets higher than e.g. 4?
        // we can't just add a button for every candidate: the dialog would get a bigger height than the monitor.
        if ((m_err == nullptr) && possibleMovedToReposRelpaths && (possibleMovedToReposRelpaths->nelts > 1))
        {
            for (int j = 0; j < possibleMovedToReposRelpaths->nelts; ++j)
            {
                svn_client_conflict_option_set_moved_to_repos_relpath2(opt, j, m_pCtx, scratchPool);

                label       = svn_client_conflict_option_get_label(opt, scratchPool);
                description = svn_client_conflict_option_get_description(opt, scratchPool);

                result.push_back(std::make_unique<SVNConflictOption>(opt, id,
                                                                     CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description), -1, j));
                bResultAdded = true;
            }
        }

        apr_array_header_t *possibleMovedToAbspaths = nullptr;

        m_err = svn_client_conflict_option_get_moved_to_abspath_candidates2(&possibleMovedToAbspaths, opt, result.GetPool(), scratchPool);
        if ((m_err == nullptr) && possibleMovedToAbspaths && (possibleMovedToAbspaths->nelts > 1))
        {
            for (int j = 0; j < possibleMovedToAbspaths->nelts; ++j)
            {
                svn_client_conflict_option_set_moved_to_abspath2(opt, j, m_pCtx, scratchPool);

                label       = svn_client_conflict_option_get_label(opt, scratchPool);
                description = svn_client_conflict_option_get_description(opt, scratchPool);

                result.push_back(std::make_unique<SVNConflictOption>(opt, id, CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description), j, -1));
                bResultAdded = true;
            }
        }

        if (!bResultAdded)
        {
            result.push_back(std::make_unique<SVNConflictOption>(opt, id, CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description)));
        }
    }

    return true;
}

bool SVNConflictInfo::GetTextResolutionOptions(SVNConflictOptions &result)
{
    ClearSVNError();
    SVNPool scratchPool(m_pool);

    apr_array_header_t *options = nullptr;
    const char *        path    = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        m_err = svn_client_conflict_text_get_resolution_options(&options, m_conflict,
                                                                m_pCtx, result.GetPool(), scratchPool),
        path);

    if (m_err != nullptr)
        return false;

    for (int i = 0; i < options->nelts; i++)
    {
        svn_client_conflict_option_t *opt = APR_ARRAY_IDX(options, i, svn_client_conflict_option_t *);

        svn_client_conflict_option_id_t id          = svn_client_conflict_option_get_id(opt);
        const char *                    label       = svn_client_conflict_option_get_label(opt, scratchPool);
        const char *                    description = svn_client_conflict_option_get_description(opt, scratchPool);

        result.push_back(std::make_unique<SVNConflictOption>(opt, id, CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description)));
    }

    return true;
}

bool SVNConflictInfo::GetPropResolutionOptions(SVNConflictOptions &result)
{
    ClearSVNError();
    SVNPool scratchPool(m_pool);

    apr_array_header_t *options = nullptr;
    const char *        path    = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        m_err = svn_client_conflict_prop_get_resolution_options(&options, m_conflict,
                                                                m_pCtx, result.GetPool(), scratchPool),
        path);

    if (m_err != nullptr)
        return false;

    for (int i = 0; i < options->nelts; i++)
    {
        svn_client_conflict_option_t *opt = APR_ARRAY_IDX(options, i, svn_client_conflict_option_t *);

        svn_client_conflict_option_id_t id          = svn_client_conflict_option_get_id(opt);
        const char *                    label       = svn_client_conflict_option_get_label(opt, scratchPool);
        const char *                    description = svn_client_conflict_option_get_description(opt, scratchPool);

        result.push_back(std::make_unique<SVNConflictOption>(opt, id, CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description)));
    }

    return true;
}

bool SVNConflictInfo::FetchTreeDetails()
{
    ClearSVNError();
    SVNPool scratchPool(m_pool);

    const char *svnPath = m_path.GetSVNApiPath(scratchPool);
    SVNTRACE(
        m_err = svn_client_conflict_tree_get_details(m_conflict, m_pCtx, m_infoPool),
        svnPath);
    if (m_err)
    {
        // if the server is not reachable, auto resolving won't work but
        // the user might still want to resolve manually
        ClearSVNError();
    }
    ClearCAPIAuthCacheOnError();

    if (m_treeConflicted)
    {
        const char *incomingChange = nullptr;
        const char *localChange    = nullptr;
        SVNTRACE(
            m_err = svn_client_conflict_tree_get_description(&incomingChange, &localChange, m_conflict,
                                                             m_pCtx, scratchPool, scratchPool),
            svnPath);

        if (m_err == nullptr)
            m_detailedIncomingChangeSummary = CUnicodeUtils::GetUnicode(incomingChange);
    }

    return (m_err == nullptr);
}

svn_error_t *SVNConflictInfo::cancelCallback(void *baton)
{
    SVNConflictInfo *pThis = static_cast<SVNConflictInfo *>(baton);

    if (pThis->m_pProgress && pThis->m_pProgress->HasUserCancelled())
    {
        CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(message));
    }
    return nullptr;
}

void SVNConflictInfo::notifyCallback(void *baton, const svn_wc_notify_t *notify, apr_pool_t * /*pool*/)
{
    if (notify == nullptr)
        return;
    SVNConflictInfo *pThis = static_cast<SVNConflictInfo *>(baton);
    if (pThis->m_pProgress)
    {
        switch (notify->action)
        {
            case svn_wc_notify_begin_search_tree_conflict_details:
                pThis->m_pProgress->SetLine(2, L"");
                break;

            case svn_wc_notify_tree_conflict_details_progress:
                pThis->m_pProgress->FormatNonPathLine(2, IDS_PROGRS_FETCHING_TREE_CONFLICT_PROGRESS, notify->revision);
                break;

            case svn_wc_notify_end_search_tree_conflict_details:
                pThis->m_pProgress->SetLine(2, L"");
                break;
            default:
                break;
        }
    }
}
