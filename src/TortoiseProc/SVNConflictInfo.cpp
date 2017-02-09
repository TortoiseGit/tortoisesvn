// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016-2017 - TortoiseSVN

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
    svn_client_conflict_option_t *option,
    svn_client_conflict_option_id_t id,
    const CString & label,
    const CString & description,
    int preferred_moved_target_idx,
    int preferred_moved_reltarget_idx)
    : m_option(option)
    , m_id(id)
    , m_label(label)
    , m_description(description)
    , m_preferred_moved_target_idx(preferred_moved_target_idx)
    , m_preferred_moved_reltarget_idx(preferred_moved_reltarget_idx)
{
}

void SVNConflictOption::SetMergedPropVal(const svn_string_t *propval)
{
    svn_client_conflict_option_set_merged_propval(m_option, propval);
}

svn_error_t * SVNConflictOption::SetMergedPropValFile(const CTSVNPath & filePath, apr_pool_t * pool)
{
    svn_stringbuf_t *propval;

    SVN_ERR(svn_stringbuf_from_file2(&propval, filePath.GetSVNApiPath(pool), pool));
    SetMergedPropVal(svn_string_create_from_buf(propval, pool));

    return SVN_NO_ERROR;
}

SVNConflictOptions::SVNConflictOptions()
{
    m_pool = svn_pool_create(NULL);
}

SVNConflictOptions::~SVNConflictOptions()
{
    svn_pool_destroy(m_pool);
}

SVNConflictOption * SVNConflictOptions::FindOptionById(svn_client_conflict_option_id_t id)
{
    for (std::unique_ptr<SVNConflictOption> & opt : *this)
    {
        if (opt->GetId() == id)
        {
            return opt.get();
        }
    }

    return NULL;
}

SVNConflictInfo::SVNConflictInfo()
    : m_conflict(NULL)
    , m_pool(NULL)
    , m_infoPool(NULL)
    , m_prop_conflicts(NULL)
{
    m_pool = svn_pool_create(NULL);
    m_infoPool = svn_pool_create(m_pool);
    svn_error_clear(svn_client_create_context2(&m_pctx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

    m_prompt.Init(m_pool, m_pctx);

    m_pctx->cancel_func = cancelCallback;
    m_pctx->cancel_baton = this;
    m_pctx->client_name = SVNHelper::GetUserAgentString(m_pool);
    m_pctx->notify_func2 = notifyCallback;
    m_pctx->notify_baton2 = this;
}

SVNConflictInfo::~SVNConflictInfo()
{
    ClearSVNError();
    svn_pool_destroy(m_pool);
}

bool SVNConflictInfo::Get(const CTSVNPath & path)
{
    ClearSVNError();
    // Clear existing conflict info.
    svn_pool_clear(m_infoPool);
    m_conflict = NULL;

    SVNPool scratchpool(m_pool);
    const char* svnPath = path.GetSVNApiPath(scratchpool);

    SVNTRACE(
        Err = svn_client_conflict_get(&m_conflict, svnPath, m_pctx, m_infoPool, scratchpool),
        svnPath
    );
    if (Err != NULL && Err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
        svn_error_clear(Err);
        Err = NULL;
        m_path = path;
        m_text_conflicted = FALSE;
        m_prop_conflicts = apr_array_make(m_infoPool, 0, sizeof(const char*));
        m_tree_conflicted = FALSE;
        m_incomingChangeSummary.Empty();
        m_detailedIncomingChangeSummary.Empty();
        m_localChangeSummary.Empty();
        return true;
    }
    else if (Err != NULL)
    {
        return false;
    }

    m_path = path;

    SVNTRACE(
        Err = svn_client_conflict_get_conflicted(&m_text_conflicted, &m_prop_conflicts, &m_tree_conflicted, m_conflict, m_infoPool, scratchpool),
        svnPath
    );

    if (Err != NULL)
        return false;

    if (m_tree_conflicted)
    {
        const char *incoming_change;
        const char *local_change;
        SVNTRACE(
            Err = svn_client_conflict_tree_get_description(&incoming_change, &local_change, m_conflict,
                                                           m_pctx, scratchpool, scratchpool),
            svnPath
        );

        if (Err != NULL)
            return false;

        m_incomingChangeSummary = CUnicodeUtils::GetUnicode(incoming_change);
        m_localChangeSummary = CUnicodeUtils::GetUnicode(local_change);
    }
    if (m_prop_conflicts && m_prop_conflicts->nelts > 0)
    {
        const char * propdesc;
        SVNTRACE(
            Err = svn_client_conflict_prop_get_description(&propdesc, m_conflict,
                                                           scratchpool, scratchpool),
            svnPath
        );
        if (Err != nullptr)
            return false;

        m_propDescription = CUnicodeUtils::GetUnicode(propdesc);
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

bool SVNConflictInfo::IsBinary()
{
    const char *mime_type = svn_client_conflict_text_get_mime_type(m_conflict);

    if (mime_type && svn_mime_type_is_binary(mime_type))
        return true;
    else
        return false;
}

int SVNConflictInfo::GetPropConflictCount() const
{
    ASSERT(m_prop_conflicts);
    return m_prop_conflicts->nelts;
}

CString SVNConflictInfo::GetPropConflictName(int idx) const
{
    ASSERT(m_prop_conflicts);
    ASSERT(idx < m_prop_conflicts->nelts);

    const char *propname = APR_ARRAY_IDX(m_prop_conflicts, idx, const char *);
    return CUnicodeUtils::GetUnicode(propname);
}

svn_error_t * SVNConflictInfo::createPropValFiles(const char *propname, const char *mergedfile, const char *basefile, const char *theirfile, const char *myfile, apr_pool_t *pool)
{
    const svn_string_t *my_propval;
    const svn_string_t *base_propval;
    const svn_string_t *their_propval;
    svn_stringbuf_t *merged_propval = svn_stringbuf_create_empty(pool);

    SVN_ERR(svn_client_conflict_prop_get_propvals(
        NULL, &my_propval, &base_propval,
        &their_propval, m_conflict, propname,
        pool));

    if (base_propval)
        SVN_ERR(svn_io_write_atomic2(basefile, base_propval->data, base_propval->len, NULL, FALSE, pool));

    if (their_propval)
        SVN_ERR(svn_io_write_atomic2(theirfile, their_propval->data, their_propval->len, NULL, FALSE, pool));

    if (my_propval)
        SVN_ERR(svn_io_write_atomic2(myfile, my_propval->data, my_propval->len, NULL, FALSE, pool));

    svn_diff_file_options_t *options = svn_diff_file_options_create(pool);
    svn_diff_t *diff;

    // If any of the property values is missing, use an empty value instead for the purpose of showing a diff.
    if (base_propval == NULL)
        base_propval = svn_string_create_empty(pool);

    if (their_propval == NULL)
        their_propval = svn_string_create_empty(pool);

    if (my_propval == NULL)
        my_propval = svn_string_create_empty(pool);

    options->ignore_eol_style = TRUE;
    SVN_ERR(svn_diff_mem_string_diff3(&diff, base_propval, my_propval, their_propval, options, pool));

    SVN_ERR(svn_diff_mem_string_output_merge3(
        svn_stream_from_stringbuf(merged_propval, pool),
        diff, base_propval, my_propval, their_propval,
        "||||||| ORIGINAL",
        "<<<<<<< MINE",
        ">>>>>>> THEIRS",
        "=======",
        svn_diff_conflict_display_modified_original_latest,
        NULL, NULL, pool));

    SVN_ERR(svn_io_write_atomic2(mergedfile, merged_propval->data, merged_propval->len, NULL, FALSE, pool));

    return SVN_NO_ERROR;
}

bool SVNConflictInfo::GetPropValFiles(const CString & propertyName, CTSVNPath & mergedfile,
                                      CTSVNPath & basefile, CTSVNPath & theirfile, CTSVNPath & myfile)
{
    ClearSVNError();
    mergedfile = CTempFiles::Instance().GetTempFilePath(false, GetPath());
    basefile = CTempFiles::Instance().GetTempFilePath(false);
    theirfile = CTempFiles::Instance().GetTempFilePath(false);
    myfile = CTempFiles::Instance().GetTempFilePath(false);

    {
        SVNPool scratchpool(m_pool);

        const char *path = svn_client_conflict_get_local_abspath(m_conflict);
        SVNTRACE(
            Err = createPropValFiles(CUnicodeUtils::GetUTF8(propertyName),
                                     mergedfile.GetSVNApiPath(scratchpool),
                                     basefile.GetSVNApiPath(scratchpool),
                                     theirfile.GetSVNApiPath(scratchpool),
                                     myfile.GetSVNApiPath(scratchpool),
                                     scratchpool),
            path
        );
    }

    if (Err)
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

CString SVNConflictInfo::GetPropDiff(const CString & propertyName)
{
    SVNPool scratchpool(m_pool);

    const svn_string_t *my_propval;
    const svn_string_t *base_propval;
    const svn_string_t *their_propval;
    svn_stringbuf_t *merged_propval = svn_stringbuf_create_empty(scratchpool);

    Err = svn_client_conflict_prop_get_propvals(
        NULL, &my_propval, &base_propval,
        &their_propval, m_conflict, CUnicodeUtils::GetUTF8(propertyName),
        scratchpool);
    if (Err)
        return{};

    svn_diff_file_options_t *options = svn_diff_file_options_create(scratchpool);
    svn_diff_t *diff;

    // If any of the property values is missing, use an empty value instead for the purpose of showing a diff.
    if (base_propval == NULL)
        base_propval = svn_string_create_empty(scratchpool);

    if (their_propval == NULL)
        their_propval = svn_string_create_empty(scratchpool);

    if (my_propval == NULL)
        my_propval = svn_string_create_empty(scratchpool);

    options->ignore_eol_style = TRUE;
    Err = svn_diff_mem_string_diff3(&diff, base_propval, my_propval, their_propval, options, scratchpool);
    if (Err)
        return{};

    Err = svn_diff_mem_string_output_merge3(
        svn_stream_from_stringbuf(merged_propval, scratchpool),
        diff, base_propval, my_propval, their_propval,
        "||||||| ORIGINAL",
        "<<<<<<< MINE",
        ">>>>>>> THEIRS",
        "=======",
        svn_diff_conflict_display_modified_original_latest,
        NULL, NULL, scratchpool);
    if (Err)
        return{};

    CStringA propValA(merged_propval->data, (int)merged_propval->len);
    return CUnicodeUtils::GetUnicode(propValA);
}

bool SVNConflictInfo::GetTextContentFiles(CTSVNPath & basefile, CTSVNPath & theirfile, CTSVNPath & myfile)
{
    ClearSVNError();
    SVNPool scratchpool(m_pool);
    const char *my_abspath;
    const char *base_abspath;
    const char *their_abspath;

    const char *path = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        Err = svn_client_conflict_text_get_contents(NULL, &my_abspath,
                                                    &base_abspath, &their_abspath,
                                                    m_conflict, scratchpool, scratchpool),
        path
    );

    if (Err)
        return false;

    if (base_abspath)
        basefile.SetFromSVN(base_abspath);
    else
        basefile = CTempFiles::Instance().GetTempFilePath(true);

    if (their_abspath)
        theirfile.SetFromSVN(their_abspath);
    else
        theirfile = CTempFiles::Instance().GetTempFilePath(true);

    if (my_abspath)
        myfile.SetFromSVN(my_abspath);
    else
        myfile.SetFromSVN(path);

    return true;
}

bool SVNConflictInfo::GetTreeResolutionOptions(SVNConflictOptions & result)
{
    ClearSVNError();
    SVNPool scratchpool(m_pool);

    apr_array_header_t *options;
    const char *path = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        Err = svn_client_conflict_tree_get_resolution_options(&options, m_conflict,
                                                              m_pctx, result.GetPool(), scratchpool),
        path
    );

    if (Err != NULL)
        return false;

    for (int i = 0; i < options->nelts; i++)
    {
        svn_client_conflict_option_t *opt = APR_ARRAY_IDX(options, i, svn_client_conflict_option_t *);

        svn_client_conflict_option_id_t id = svn_client_conflict_option_get_id(opt);
        const char *label = svn_client_conflict_option_get_label(opt, scratchpool);
        const char *description = svn_client_conflict_option_get_description(opt, scratchpool);

        bool bResultAdded = false;
        if (id == svn_client_conflict_option_incoming_move_file_text_merge ||
            id == svn_client_conflict_option_incoming_move_dir_merge)
        {
            // if we get only one possible path from all candidates, then we don't add
            // custom buttons but use the one that's already provided: it will be the same.
            // if we get more candidates, then we only add from the second one on, since
            // the first one is already set by the normal conflict options.
            apr_array_header_t *possible_moved_to_repos_relpaths = nullptr;
            Err = svn_client_conflict_option_get_moved_to_repos_relpath_candidates(&possible_moved_to_repos_relpaths, opt, result.GetPool(), scratchpool);
            // TODO: what should we do if the number of candidates gets higher than e.g. 4?
            // we can't just add a button for every candidate: the dialog would get a bigger height than the monitor.
            if ((Err == nullptr) && possible_moved_to_repos_relpaths && (possible_moved_to_repos_relpaths->nelts > 1))
            {
                for (int j = 0; j < possible_moved_to_repos_relpaths->nelts; ++j)
                {
                    svn_client_conflict_option_set_moved_to_repos_relpath(opt, j, m_pctx, scratchpool);

                    label = svn_client_conflict_option_get_label(opt, scratchpool);
                    description = svn_client_conflict_option_get_description(opt, scratchpool);

                    result.push_back(std::unique_ptr<SVNConflictOption>(new SVNConflictOption(opt, id,
                                                                                              CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description), -1, j)));
                    bResultAdded = true;
                }
            }

            apr_array_header_t *possible_moved_to_abspaths = nullptr;
            Err = svn_client_conflict_option_get_moved_to_abspath_candidates(&possible_moved_to_abspaths, opt, result.GetPool(), scratchpool);
            if ((Err == nullptr) && possible_moved_to_abspaths && (possible_moved_to_abspaths->nelts > 1))
            {
                for (int j = 0; j < possible_moved_to_abspaths->nelts; ++j)
                {
                    svn_client_conflict_option_set_moved_to_abspath(opt, j, m_pctx, scratchpool);

                    label = svn_client_conflict_option_get_label(opt, scratchpool);
                    description = svn_client_conflict_option_get_description(opt, scratchpool);

                    result.push_back(std::unique_ptr<SVNConflictOption>(new SVNConflictOption(opt, id,
                                                                                              CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description), j, -1)));
                    bResultAdded = true;
                }
            }

        }
        if (!bResultAdded)
        {
            result.push_back(std::unique_ptr<SVNConflictOption>(new SVNConflictOption(opt, id,
                                                                                      CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description))));
        }
    }

    return true;
}

bool SVNConflictInfo::GetTextResolutionOptions(SVNConflictOptions & result)
{
    ClearSVNError();
    SVNPool scratchpool(m_pool);

    apr_array_header_t *options;
    const char *path = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        Err = svn_client_conflict_text_get_resolution_options(&options, m_conflict,
                                                              m_pctx, result.GetPool(), scratchpool),
        path
    );

    if (Err != NULL)
        return false;

    for (int i = 0; i < options->nelts; i++)
    {
        svn_client_conflict_option_t *opt = APR_ARRAY_IDX(options, i, svn_client_conflict_option_t *);

        svn_client_conflict_option_id_t id = svn_client_conflict_option_get_id(opt);
        const char *label = svn_client_conflict_option_get_label(opt, scratchpool);
        const char *description = svn_client_conflict_option_get_description(opt, scratchpool);

        result.push_back(std::unique_ptr<SVNConflictOption>(new SVNConflictOption(opt, id,
                                                                                  CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description))));
    }

    return true;
}

bool SVNConflictInfo::GetPropResolutionOptions(SVNConflictOptions & result)
{
    ClearSVNError();
    SVNPool scratchpool(m_pool);

    apr_array_header_t *options;
    const char *path = svn_client_conflict_get_local_abspath(m_conflict);
    SVNTRACE(
        Err = svn_client_conflict_prop_get_resolution_options(&options, m_conflict,
                                                              m_pctx, result.GetPool(), scratchpool),
        path
    );

    if (Err != NULL)
        return false;

    for (int i = 0; i < options->nelts; i++)
    {
        svn_client_conflict_option_t *opt = APR_ARRAY_IDX(options, i, svn_client_conflict_option_t *);

        svn_client_conflict_option_id_t id = svn_client_conflict_option_get_id(opt);
        const char *label = svn_client_conflict_option_get_label(opt, scratchpool);
        const char *description = svn_client_conflict_option_get_description(opt, scratchpool);

        result.push_back(std::unique_ptr<SVNConflictOption>(new SVNConflictOption(opt, id,
                                                                                  CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description))));
    }

    return true;
}

bool SVNConflictInfo::FetchTreeDetails()
{
    ClearSVNError();
    SVNPool scratchpool(m_pool);

    const char* svnPath = m_path.GetSVNApiPath(scratchpool);
    SVNTRACE(
        Err = svn_client_conflict_tree_get_details(m_conflict, m_pctx, m_infoPool),
        svnPath
    );

    ClearCAPIAuthCacheOnError();

    if (m_tree_conflicted)
    {
        const char *incoming_change;
        const char *local_change;
        SVNTRACE(
            Err = svn_client_conflict_tree_get_description(&incoming_change, &local_change, m_conflict,
                                                           m_pctx, scratchpool, scratchpool),
            svnPath
        );

        if (Err == NULL)
            m_detailedIncomingChangeSummary = CUnicodeUtils::GetUnicode(incoming_change);
    }

    return (Err == NULL);
}

svn_error_t* SVNConflictInfo::cancelCallback(void *baton)
{
    SVNConflictInfo * pThis = (SVNConflictInfo *)baton;

    if (pThis->m_pProgress && pThis->m_pProgress->HasUserCancelled())
    {
        CString message(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED));
        return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(message));
    }
    return SVN_NO_ERROR;
}

void SVNConflictInfo::notifyCallback(void *baton, const svn_wc_notify_t *notify, apr_pool_t * /*pool*/)
{
    SVNConflictInfo * pThis = (SVNConflictInfo *)baton;
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
        }
    }
}
