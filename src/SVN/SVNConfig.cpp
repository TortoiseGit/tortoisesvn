// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010, 2012 - TortoiseSVN

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
    pool = svn_pool_create (parentpool);

    Err = svn_config_ensure(NULL, pool);
    if (Err == nullptr)
        // set up the configuration
        Err = svn_config_get_config (&(config), g_pConfigDir, pool);

    if (config)
        SetUpSSH();
}

SVNConfig::~SVNConfig(void)
{
    svn_error_clear(Err);
    svn_pool_destroy (pool);
    svn_pool_destroy (parentpool);
    m_critSec.Term();
    delete m_pInstance;
}

BOOL SVNConfig::GetDefaultIgnores()
{
    if (config == nullptr)
        return FALSE;
    svn_error_t * err;
    patterns = NULL;
    err = svn_wc_get_default_ignores (&(patterns), config, pool);
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
    if (config == nullptr)
        return FALSE;
    svn_boolean_t no_unlock = FALSE;
    svn_config_t * opt = (svn_config_t *)apr_hash_get (config, SVN_CONFIG_CATEGORY_CONFIG,
        APR_HASH_KEY_STRING);
    if (opt)
        svn_error_clear(svn_config_get_bool(opt, &no_unlock, SVN_CONFIG_SECTION_MISCELLANY, SVN_CONFIG_OPTION_NO_UNLOCK, FALSE));
    return no_unlock;
}

bool SVNConfig::SetUpSSH()
{
    bool bRet = false;
    if (config == nullptr)
        return bRet;
    //set up the SVN_SSH param
    CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
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
                tsvn_ssh = _T("\"") + CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe") + _T("\"");
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
    apr_pool_t * pool2 = pool;
    apr_hash_t * config2 = nullptr;
    if (config)
        pool2 = svn_pool_create (parentpool);

    svn_error_clear(Err);
    Err = svn_config_ensure(NULL, pool2);
    if (Err == nullptr)
        Err = svn_config_get_config (&(config2), g_pConfigDir, pool2);

    if (config2 == nullptr)
    {
        apr_pool_destroy(pool2);
    }
    else if (config)
    {
        apr_pool_destroy(pool);
        pool = pool2;
        config = config2;
    }
    if (config)
        SetUpSSH();
}


// TODO: remove these forward declarations
// once svn 1.8 is out and the config copy
// APIs can be used.
struct svn_config_t
{
  /* Table of cfg_section_t's. */
  apr_hash_t *sections;

  /* Pool for hash tables, table entries and unexpanded values */
  apr_pool_t *pool;

  /* Pool for expanded values -- this is separate, so that we can
     clear it when modifying the config data. */
  apr_pool_t *x_pool;

  /* Indicates that some values in the configuration have been expanded. */
  svn_boolean_t x_values;

  /* Temporary string used for lookups.  (Using a stringbuf so that
     frequent resetting is efficient.) */
  svn_stringbuf_t *tmp_key;

  /* Temporary value used for expanded default values in svn_config_get.
     (Using a stringbuf so that frequent resetting is efficient.) */
  svn_stringbuf_t *tmp_value;

  /* Specifies whether section names are populated case sensitively. */
  svn_boolean_t section_names_case_sensitive;
};

/* Section table entries. */
typedef struct cfg_section_t cfg_section_t;
struct cfg_section_t
{
  /* The section name. */
  const char *name;

  /* The section name, converted into a hash key. */
  const char *hash_key;

  /* Table of cfg_option_t's. */
  apr_hash_t *options;
};


/* Option table entries. */
typedef struct cfg_option_t cfg_option_t;
struct cfg_option_t
{
  /* The option name. */
  const char *name;

  /* The option name, converted into a hash key. */
  const char *hash_key;

  /* The unexpanded option value. */
  const char *value;

  /* The expanded option value. */
  const char *x_value;

  /* Expansion flag. If this is TRUE, this value has already been expanded.
     In this case, if x_value is NULL, no expansions were necessary,
     and value should be used directly. */
  svn_boolean_t expanded;
};

apr_hash_t * SVNConfig::GetConfig( apr_pool_t * pool )
{
    if (config == NULL)
        return NULL;
    // create a copy of the config object we have
    // to avoid threading issues
    CComCritSecLock<CComCriticalSection> lock(m_critSec);

    // TODO: once 1.8 is out, use svn_config_copy_config() instead
    // of this custom copy stuff.
    apr_hash_t * rethash = apr_hash_make(pool);
    for (apr_hash_index_t * cidx = apr_hash_first(pool, config); cidx != NULL; cidx = apr_hash_next(cidx))
    {
        const void *ckey = NULL;
        void *cval = NULL;
        apr_ssize_t ckeyLength = 0;

        apr_hash_this(cidx, &ckey, &ckeyLength, &cval);
        svn_config_t * c = (svn_config_t*)cval;


        svn_config_t * newconfig = NULL;
        svn_config_create(&newconfig, false, pool);

        newconfig->sections = apr_hash_make(pool);
        newconfig->pool = pool;
        newconfig->x_pool = svn_pool_create(pool);
        newconfig->x_values = c->x_values;
        newconfig->tmp_key = svn_stringbuf_dup(c->tmp_key, pool);
        newconfig->tmp_value = svn_stringbuf_dup(c->tmp_value, pool);
        newconfig->section_names_case_sensitive = c->section_names_case_sensitive;

        for (apr_hash_index_t * sectidx = apr_hash_first(pool, c->sections); sectidx != NULL; sectidx = apr_hash_next(sectidx))
        {
            const void *sectkey = NULL;
            void *sectval = NULL;
            apr_ssize_t sectkeyLength = 0;
            apr_hash_this(sectidx, &sectkey, &sectkeyLength, &sectval);
            cfg_section_t * sect = (cfg_section_t*)sectval;

            cfg_section_t * newsec = (cfg_section_t *)apr_palloc(pool, sizeof(*newsec));
            newsec->name = apr_pstrdup(pool, sect->name);
            newsec->hash_key = apr_pstrdup(pool, sect->hash_key);
            newsec->options = apr_hash_make(pool);
            for (apr_hash_index_t * optidx = apr_hash_first(pool, sect->options); optidx != NULL; optidx = apr_hash_next(optidx))
            {
                const void *optkey = NULL;
                void *optval = NULL;
                apr_ssize_t optkeyLength = 0;
                apr_hash_this(optidx, &optkey, &optkeyLength, &optval);
                cfg_option_t * opt = (cfg_option_t*)optval;

                cfg_option_t * newopt = (cfg_option_t*)apr_palloc(pool, sizeof(*newopt));
                newopt->name = apr_pstrdup(pool, opt->name);
                newopt->hash_key = apr_pstrdup(pool, opt->hash_key);
                newopt->value = apr_pstrdup(pool, opt->value);
                newopt->x_value = apr_pstrdup(pool, opt->x_value);
                newopt->expanded = opt->expanded;
                apr_hash_set(newsec->options, apr_pstrdup(pool, (const char*)optkey), optkeyLength, newopt);
            }
            apr_hash_set(newconfig->sections, apr_pstrdup(pool, (const char*)sectkey), sectkeyLength, newsec);
        }
        apr_hash_set(rethash, apr_pstrdup(pool, (const char*)ckey), ckeyLength, newconfig);
    }

    return rethash;
}
