// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010-2015 - TortoiseSVN

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
#include "SVNConfig.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "SVNGlobal.h"
#pragma warning(push)
#include "svn_error.h"
#include "svn_pools.h"
#include "svn_config.h"
#include "svn_wc.h"
#pragma warning(pop)

SVNConfig* SVNConfig::m_pInstance;


SVNConfig::SVNConfig(void)
    : config(nullptr)
    , patterns(nullptr)
    , Err(nullptr)
{
    m_critSec.Init();
    parentpool = svn_pool_create(NULL);
    m_pool = svn_pool_create (parentpool);
    wcignorespool = svn_pool_create(m_pool);

    Err = svn_config_ensure(NULL, m_pool);
    if (Err == nullptr)
        // set up the configuration
        Err = svn_config_get_config (&(config), g_pConfigDir, m_pool);

    if (config)
        SetUpSSH();
}

SVNConfig::~SVNConfig(void)
{
    svn_error_clear(Err);
    svn_pool_destroy (m_pool);
    svn_pool_destroy (parentpool);
    m_critSec.Term();
    delete m_pInstance;
}

BOOL SVNConfig::GetDefaultIgnores()
{
    if (config == nullptr)
        return FALSE;

    svn_pool_clear(wcignorespool);
    svn_error_t * err;
    patterns = nullptr;
    err = svn_wc_get_default_ignores(&patterns, config, wcignorespool);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }

    return TRUE;
}

BOOL SVNConfig::GetWCIgnores(const CTSVNPath& path)
{
    if (config == nullptr)
        return FALSE;

    svn_pool_clear(wcignorespool);
    patterns = nullptr;
    svn_wc_context_t * pctx = NULL;
    svn_error_t * err = svn_wc_context_create(&pctx, NULL, wcignorespool, wcignorespool);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }
    err = svn_wc_get_ignores2(&patterns, pctx, path.GetSVNApiPath(wcignorespool), config, wcignorespool, wcignorespool);
    svn_wc_context_destroy(pctx);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }

    return TRUE;
}

BOOL SVNConfig::MatchIgnorePattern(const CString& name)
{
    if (patterns == nullptr)
        return FALSE;
    return svn_wc_match_ignore_list(CUnicodeUtils::GetUTF8(name), patterns, wcignorespool);
}

BOOL SVNConfig::KeepLocks()
{
    if (config == nullptr)
        return FALSE;
    svn_boolean_t no_unlock = FALSE;
    svn_config_t * opt = (svn_config_t *)apr_hash_get (config, SVN_CONFIG_CATEGORY_CONFIG,
        APR_HASH_KEY_STRING);
    if (opt)
        svn_error_clear(svn_config_get_bool(opt, &no_unlock, SVN_CONFIG_SECTION_MISCELLANY, SVN_CONFIG_OPTION_NO_UNLOCK, FALSE));
    return no_unlock;
}

BOOL SVNConfig::ConfigGetBool(const char * section, const char * option, bool defbool)
{
    if (config == nullptr)
        return defbool;
    svn_boolean_t val = defbool;
    svn_config_t * opt = (svn_config_t *)apr_hash_get(config, SVN_CONFIG_CATEGORY_CONFIG,
                                                      APR_HASH_KEY_STRING);
    if (opt)
        svn_error_clear(svn_config_get_bool(opt, &val, section, option, defbool));
    return val;
}

bool SVNConfig::SetUpSSH()
{
    bool bRet = false;
    if (config == nullptr)
        return bRet;
    //set up the SVN_SSH param
    CString tsvn_ssh = CRegString(L"Software\\TortoiseSVN\\SSH");
    if (tsvn_ssh.IsEmpty() && config)
    {
        // check whether the ssh client is already set in the Subversion config
        svn_config_t * cfg = (svn_config_t *)apr_hash_get (config, SVN_CONFIG_CATEGORY_CONFIG,
            APR_HASH_KEY_STRING);
        if (cfg)
        {
            const char * sshValue = NULL;
            svn_config_get(cfg, &sshValue, SVN_CONFIG_SECTION_TUNNELS, "ssh", "");
            if ((sshValue == NULL)||(sshValue[0] == 0))
                tsvn_ssh = L"\"" + CPathUtils::GetAppDirectory() + L"TortoisePlink.exe" + L"\"";
        }
    }
    tsvn_ssh.Replace('\\', '/');
    if (!tsvn_ssh.IsEmpty())
    {
        svn_config_t * cfg = (svn_config_t *)apr_hash_get (config, SVN_CONFIG_CATEGORY_CONFIG,
            APR_HASH_KEY_STRING);
        if (cfg)
        {
            svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
            bRet = true;
        }
    }
    return bRet;
}

void SVNConfig::Refresh()
{
    // try to reload the configuration
    // if it fails, use the originally loaded configuration
    // if it succeeds, use the reloaded config and clear
    // the memory of the old configuration
    apr_pool_t * pool = m_pool;
    apr_hash_t * config2 = nullptr;
    if (config)
        pool = svn_pool_create (parentpool);

    svn_error_clear(Err);
    Err = svn_config_ensure(NULL, pool);
    if (Err == nullptr)
        Err = svn_config_get_config (&(config2), g_pConfigDir, pool);

    if (config2 == nullptr)
    {
        apr_pool_destroy(pool);
    }
    else if (config)
    {
        apr_pool_destroy(m_pool);
        m_pool = pool;
        config = config2;
    }
    if (config)
        SetUpSSH();
}

apr_hash_t * SVNConfig::GetConfig( apr_pool_t * pool )
{
    if (config == NULL)
        return NULL;
    // create a copy of the config object we have
    // to avoid threading issues
    CComCritSecLock<CComCriticalSection> lock(m_critSec);

    apr_hash_t * rethash = apr_hash_make(pool);
    svn_error_t * error = svn_config_copy_config(&rethash, config, pool);
    if (error)
    {
        svn_error_clear(error);
        return NULL;
    }

    return rethash;
}
