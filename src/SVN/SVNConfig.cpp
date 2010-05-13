// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010 - TortoiseSVN

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

#include "StdAfx.h"
#include "SVNConfig.h"
#include "UnicodeUtils.h"

SVNConfig::SVNConfig(void)
{
    svn_error_t * err;
    parentpool = svn_pool_create(NULL);
    svn_error_clear(svn_client_create_context(&ctx, parentpool));

    err = svn_config_ensure(NULL, parentpool);
    pool = svn_pool_create (parentpool);
    // set up the configuration
    if (err == 0)
        err = svn_config_get_config (&(ctx->config), g_pConfigDir, pool);

    patterns = NULL;

    if (err != 0)
    {
        svn_error_clear(err);
        svn_pool_destroy (pool);
        svn_pool_destroy (parentpool);
        exit(-1);
    }
}

SVNConfig::~SVNConfig(void)
{
    svn_pool_destroy (pool);
    svn_pool_destroy (parentpool);
}

BOOL SVNConfig::GetDefaultIgnores()
{
    svn_error_t * err;
    patterns = NULL;
    err = svn_wc_get_default_ignores (&(patterns), ctx->config, pool);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }

    return TRUE;
}

BOOL SVNConfig::MatchIgnorePattern(const CString& name)
{
    if (patterns == NULL)
        return FALSE;
    return svn_wc_match_ignore_list(CUnicodeUtils::GetUTF8(name), patterns, pool);
}

BOOL SVNConfig::KeepLocks()
{
    svn_boolean_t no_unlock = FALSE;
    svn_config_t * opt = (svn_config_t *)apr_hash_get (ctx->config, SVN_CONFIG_CATEGORY_CONFIG,
        APR_HASH_KEY_STRING);
    svn_error_clear(svn_config_get_bool(opt, &no_unlock, SVN_CONFIG_SECTION_MISCELLANY, SVN_CONFIG_OPTION_NO_UNLOCK, FALSE));
    return no_unlock;
}