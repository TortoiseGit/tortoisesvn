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
#include "SVNConflictInfo.h"
#include "SVNConfig.h"
#include "SVNHelpers.h"
#include "SVNTrace.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"

SVNConflictOption::SVNConflictOption(
    svn_client_conflict_option_t *option,
    svn_client_conflict_option_id_t id,
    const CString & description)
    : m_option(option)
    , m_id(id)
    , m_description(description)
{
}

SVNConflictOptions::SVNConflictOptions()
{
    m_pool = svn_pool_create(NULL);
}

SVNConflictOptions::~SVNConflictOptions()
{
    svn_pool_destroy(m_pool);
}

SVNConflictInfo::SVNConflictInfo()
    : m_conflict(NULL)
    , m_pool(NULL)
    , m_infoPool(NULL)
{
    m_pool = svn_pool_create(NULL);
    m_infoPool = svn_pool_create(m_pool);
    svn_error_clear(svn_client_create_context2(&m_pctx, SVNConfig::Instance().GetConfig(m_pool), m_pool));
}

SVNConflictInfo::~SVNConflictInfo()
{
    svn_pool_destroy(m_pool);
}

bool SVNConflictInfo::Get(const CTSVNPath & path)
{
    // Clear existing conflict info.
    svn_pool_clear(m_infoPool);
    m_conflict = NULL;

    SVNPool scratchpool(m_pool);
    const char* svnPath = path.GetSVNApiPath(scratchpool);

    SVNTRACE(
        Err = svn_client_conflict_get(&m_conflict, svnPath, m_pctx, m_infoPool, scratchpool),
        svnPath
    );

    return (Err == NULL);
}

bool SVNConflictInfo::HasTreeConflict()
{
    SVNPool scratchpool(m_pool);

    svn_boolean_t tree_conflicted = FALSE;
    SVNTRACE(
        Err = svn_client_conflict_get_conflicted(NULL, NULL, &tree_conflicted, m_conflict, scratchpool, scratchpool),
        svn_client_conflict_get_local_abspath(m_conflict)
    );

    return (Err == NULL) && tree_conflicted != FALSE;
}

bool SVNConflictInfo::GetTreeConflictDescription(CString & incomingChangeDescription,
                                                 CString & localChangeDescription)
{
    SVNPool scratchpool(m_pool);
    const char *incoming_change;
    const char *local_change;
    SVNTRACE(
        Err = svn_client_conflict_tree_get_description(&incoming_change, &local_change, m_conflict,
                                                       m_pctx, scratchpool, scratchpool),
        svn_client_conflict_get_local_abspath(m_conflict)
    );

    if (Err != NULL)
        return false;

    incomingChangeDescription = CUnicodeUtils::GetUnicode(incoming_change);
    localChangeDescription = CUnicodeUtils::GetUnicode(local_change);
    return true;
}

bool SVNConflictInfo::GetTreeResolutionOptions(SVNConflictOptions & result)
{
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
        const char *description;

        SVNTRACE(
            Err = svn_client_conflict_option_describe(&description, opt, scratchpool, scratchpool),
            path);

        if (Err != NULL)
            return false;

        svn_client_conflict_option_id_t id = svn_client_conflict_option_get_id(opt);

        result.push_back(std::unique_ptr<SVNConflictOption>(new SVNConflictOption(opt, id, CUnicodeUtils::GetUnicode(description))));
    }

    return true;
}
