// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010-2015, 2018, 2021 - TortoiseSVN

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

SVNConfig::SVNConfig()
    : m_config(nullptr)
    , m_patterns(nullptr)
    , m_err(nullptr)
{
    m_parentPool    = svn_pool_create(nullptr);
    m_pool          = svn_pool_create(m_parentPool);
    m_wcIgnoresPool = svn_pool_create(m_pool);

    m_err = svn_config_ensure(nullptr, m_pool);
    if (m_err == nullptr)
        // set up the configuration
        m_err = svn_config_get_config(&(m_config), g_pConfigDir, m_pool);

    if (m_config)
        SetUpSSH();
}

SVNConfig::~SVNConfig()
{
    svn_error_clear(m_err);
    svn_pool_destroy(m_pool);
    svn_pool_destroy(m_parentPool);
    delete m_pInstance;
}

BOOL SVNConfig::GetDefaultIgnores()
{
    if (m_config == nullptr)
        return FALSE;

    svn_pool_clear(m_wcIgnoresPool);
    m_patterns = nullptr;
    auto err   = svn_wc_get_default_ignores(&m_patterns, m_config, m_wcIgnoresPool);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }
    m_lastWcIgnorePath.Reset();
    return TRUE;
}

BOOL SVNConfig::GetWCIgnores(const CTSVNPath& path)
{
    if (m_config == nullptr)
        return FALSE;

    svn_pool_clear(m_wcIgnoresPool);
    m_patterns             = nullptr;
    svn_wc_context_t* pctx = nullptr;
    svn_error_t*      err  = svn_wc_context_create(&pctx, nullptr, m_wcIgnoresPool, m_wcIgnoresPool);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }
    err = svn_wc_get_ignores2(&m_patterns, pctx, path.GetSVNApiPath(m_wcIgnoresPool), m_config, m_wcIgnoresPool, m_wcIgnoresPool);
    svn_wc_context_destroy(pctx);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }
    m_lastWcIgnorePath = path;
    return TRUE;
}

BOOL SVNConfig::MatchIgnorePattern(const CString& name) const
{
    if (m_patterns == nullptr)
        return FALSE;
    return svn_wc_match_ignore_list(CUnicodeUtils::GetUTF8(name), m_patterns, m_wcIgnoresPool);
}

BOOL SVNConfig::KeepLocks() const
{
    if (m_config == nullptr)
        return FALSE;
    svn_boolean_t noUnlock = FALSE;
    svn_config_t* opt      = static_cast<svn_config_t*>(apr_hash_get(m_config, SVN_CONFIG_CATEGORY_CONFIG,
                                                                APR_HASH_KEY_STRING));
    if (opt)
        svn_error_clear(svn_config_get_bool(opt, &noUnlock, SVN_CONFIG_SECTION_MISCELLANY, SVN_CONFIG_OPTION_NO_UNLOCK, FALSE));
    return noUnlock;
}

BOOL SVNConfig::ConfigGetBool(const char* section, const char* option, bool defbool) const
{
    if (m_config == nullptr)
        return defbool;
    svn_boolean_t val = defbool;
    svn_config_t* opt = static_cast<svn_config_t*>(apr_hash_get(m_config, SVN_CONFIG_CATEGORY_CONFIG,
                                                                APR_HASH_KEY_STRING));
    if (opt)
        svn_error_clear(svn_config_get_bool(opt, &val, section, option, defbool));
    return val;
}

bool SVNConfig::SetUpSSH() const
{
    bool bRet = false;
    if (m_config == nullptr)
        return bRet;
    //set up the SVN_SSH param
    CString tsvnSsh = CRegString(L"Software\\TortoiseSVN\\SSH");
    if (tsvnSsh.IsEmpty() && m_config)
    {
        // check whether the ssh client is already set in the Subversion m_config
        svn_config_t* cfg = static_cast<svn_config_t*>(apr_hash_get(m_config, SVN_CONFIG_CATEGORY_CONFIG,
                                                                    APR_HASH_KEY_STRING));
        if (cfg)
        {
            const char* sshValue = nullptr;
            svn_config_get(cfg, &sshValue, SVN_CONFIG_SECTION_TUNNELS, "ssh", "");
            if ((sshValue == nullptr) || (sshValue[0] == 0))
                tsvnSsh = L"\"" + CPathUtils::GetAppDirectory() + L"TortoisePlink.exe" + L"\"";
        }
    }
    tsvnSsh.Replace('\\', '/');
    if (!tsvnSsh.IsEmpty())
    {
        svn_config_t* cfg = static_cast<svn_config_t*>(apr_hash_get(m_config, SVN_CONFIG_CATEGORY_CONFIG,
                                                                    APR_HASH_KEY_STRING));
        if (cfg)
        {
            svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvnSsh));
            bRet = true;
        }
    }
    return bRet;
}

void SVNConfig::Refresh()
{
    // try to reload the configuration
    // if it fails, use the originally loaded configuration
    // if it succeeds, use the reloaded m_config and clear
    // the memory of the old configuration
    apr_pool_t* pool    = m_pool;
    apr_hash_t* config2 = nullptr;
    if (m_config)
        pool = svn_pool_create(m_parentPool);

    svn_error_clear(m_err);
    m_err = svn_config_ensure(nullptr, pool);
    if (m_err == nullptr)
        m_err = svn_config_get_config(&(config2), g_pConfigDir, pool);

    if (config2 == nullptr)
    {
        apr_pool_destroy(pool);
    }
    else if (m_config)
    {
        apr_pool_destroy(m_pool);
        m_pool   = pool;
        m_config = config2;
    }
    if (m_config)
        SetUpSSH();
}

apr_hash_t* SVNConfig::GetConfig(apr_pool_t* pool)
{
    if (m_config == nullptr)
        return nullptr;
    // create a copy of the m_config object we have
    // to avoid threading issues
    CComCritSecLock<CComCriticalSection> lock(m_critSec);

    apr_hash_t*  retHash = apr_hash_make(pool);
    svn_error_t* error   = svn_config_copy_config(&retHash, m_config, pool);
    if (error)
    {
        svn_error_clear(error);
        return nullptr;
    }

    return retHash;
}
