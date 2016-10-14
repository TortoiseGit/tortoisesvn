// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseSVN

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
    const CString & description)
    : m_option(option)
    , m_id(id)
    , m_label(label)
    , m_description(description)
{
}

void SVNConflictOption::SetMergedPropVal(const svn_string_t *propval)
{
    svn_client_conflict_option_set_merged_propval(m_option, propval);
}

svn_error_t * SVNConflictOption::SetMergedPropValFile(const CTSVNPath & filePath)
{
    SVNPool pool;
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
}

SVNConflictInfo::~SVNConflictInfo()
{
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
    if (Err != NULL)
        return false;

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

        result.push_back(std::unique_ptr<SVNConflictOption>(new SVNConflictOption(opt, id,
            CUnicodeUtils::GetUnicode(label), CUnicodeUtils::GetUnicode(description))));
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
