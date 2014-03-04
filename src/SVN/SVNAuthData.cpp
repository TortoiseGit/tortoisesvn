// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseSVN

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
#include "svn_auth.h"
#pragma warning(pop)
#include "resource.h"
#include "SVNAuthData.h"
#include "UnicodeUtils.h"
#include "SVNConfig.h"
#include "SVNHelpers.h"

SVNAuthData::SVNAuthData()
: SVNBase()
, m_prompt(false)
{
    m_pool = svn_pool_create(NULL);

    svn_error_clear(svn_client_create_context2(&m_pctx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

    // set up authentication
    m_prompt.Init(m_pool, m_pctx);
    const char * path = nullptr;
    svn_config_get_user_config_path(&path, g_pConfigDir, NULL, m_pool);

    svn_auth_set_parameter(m_pctx->auth_baton, SVN_AUTH_PARAM_CONFIG_DIR, path);

    m_pctx->client_name = SVNHelper::GetUserAgentString(m_pool);
}

SVNAuthData::~SVNAuthData(void)
{
    svn_error_clear(Err);
    svn_pool_destroy(m_pool);                  // free the allocated memory
}

svn_error_t * SVNAuthData::cleanup_callback(svn_boolean_t *delete_cred, void *cleanup_baton,
                                            const char *cred_kind, const char *realmstring,
                                            apr_hash_t * hash, apr_pool_t * scratch_pool)
{
    std::tuple<std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>*, std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>> * tupleBaton =
        (std::tuple<std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>*, std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>>*)cleanup_baton;

    auto authList = std::get<0>(*tupleBaton);
    auto delList = std::get<1>(*tupleBaton);

    CString s1, s2;
    if (cred_kind)
        s1 = CUnicodeUtils::GetUnicode(cred_kind);
    if (realmstring)
        s2 = CUnicodeUtils::GetUnicode(realmstring);

    SVNAuthDataInfo authinfodata;

    for (apr_hash_index_t *hi = apr_hash_first(scratch_pool, hash); hi; hi = apr_hash_next(hi))
    {
        const void *vkey;
        void *val;

        apr_hash_this(hi, &vkey, NULL, &val);
        const char * key = (const char*)vkey;
        svn_string_t *value = (svn_string_t *)val;
        if (strcmp(key, SVN_CONFIG_AUTHN_PASSWORD_KEY) == 0)
            continue;
        else if (strcmp(key, SVN_CONFIG_AUTHN_PASSPHRASE_KEY) == 0)
            continue;
        else if (strcmp(key, SVN_CONFIG_AUTHN_PASSTYPE_KEY) == 0)
            continue;
        else if (strcmp(key, SVN_CONFIG_AUTHN_USERNAME_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.username = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_ASCII_CERT_KEY) == 0)
            continue; // don't show data which is not human-readable
        else if (strcmp(key, SVN_CONFIG_AUTHN_HOSTNAME_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.hostname = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_VALID_FROM_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.validfrom = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_VALID_UNTIL_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.validuntil = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_ISSUER_DN_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.issuer = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_FINGERPRINT_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.fingerprint = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_FAILURES_KEY) == 0)
        {
            unsigned int failures;

            svn_cstring_atoui(&failures, value->data);

            if (0 == (failures & (SVN_AUTH_SSL_NOTYETVALID | SVN_AUTH_SSL_EXPIRED |
                SVN_AUTH_SSL_CNMISMATCH | SVN_AUTH_SSL_UNKNOWNCA |
                SVN_AUTH_SSL_OTHER)))
                continue;

            CString s;
            s.Format(IDS_ERR_SSL_VALIDATE, (LPCWSTR)authinfodata.hostname);
            authinfodata.failures = s;

            authinfodata.failures += L"\n";

            if (failures & SVN_AUTH_SSL_NOTYETVALID)
            {
                s.Format(IDS_ERR_SSL_NOTYETVALID, (LPCWSTR)authinfodata.validfrom);
                authinfodata.failures += s;
                authinfodata.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_EXPIRED)
            {
                s.Format(IDS_ERR_SSL_EXPIRED, (LPCWSTR)authinfodata.validuntil);
                authinfodata.failures += s;
                authinfodata.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_CNMISMATCH)
            {
                s.Format(IDS_ERR_SSL_CNMISMATCH, (LPCWSTR)authinfodata.hostname);
                authinfodata.failures += s;
                authinfodata.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_UNKNOWNCA)
            {
                s.FormatMessage(IDS_ERR_SSL_UNKNOWNCA, (LPCWSTR)authinfodata.fingerprint, (LPCWSTR)authinfodata.hostname);
                authinfodata.failures += s;
                authinfodata.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_OTHER)
            {
                // unknown verification failure
            }
        }
    }

    for (auto it : delList)
    {
        if ((s1.Compare(std::get<0>(it)) == 0) &&
            (s2.Compare(std::get<1>(it)) == 0))
        {
            *delete_cred = true;
        }
    }
    authList->push_back(std::make_tuple(s1, s2, authinfodata));

    return SVN_NO_ERROR;
}

std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> SVNAuthData::GetAuthList()
{
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> authList;
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> delList;
    auto cleanup_baton = std::make_tuple(&authList, delList);
    SVNPool subpool(m_pool);
    Err = svn_config_walk_auth_data(g_pConfigDir, cleanup_callback, &cleanup_baton, subpool);
    if (Err)
    {
        authList.clear();
    }
    return authList;
}

std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> SVNAuthData::DeleteAuthList(const std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>& delList)
{
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> authList;
    auto cleanup_baton = std::make_tuple(&authList, delList);
    SVNPool subpool(m_pool);
    Err = svn_config_walk_auth_data(g_pConfigDir, cleanup_callback, &cleanup_baton, subpool);
    return GetAuthList();
}
