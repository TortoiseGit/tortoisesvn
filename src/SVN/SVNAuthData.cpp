// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013-2015 - TortoiseSVN

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
#include "svn_x509.h"
#include "svn_base64.h"
#pragma warning(pop)
#include "resource.h"
#include "SVNAuthData.h"
#include "UnicodeUtils.h"
#include "SVNConfig.h"
#include "SVNHelpers.h"

#include <Wincrypt.h>


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

static const WCHAR description[] = L"auth_svn.simple.wincrypt";
const svn_string_t * SVNAuthData::decrypt_data(const svn_string_t *crypted, apr_pool_t *pool)
{
    crypted = svn_base64_decode_string(crypted, pool);

    DATA_BLOB blobin;
    DATA_BLOB blobout;
    LPWSTR descr;
    const svn_string_t * orig = NULL;

    blobin.cbData = (DWORD)crypted->len;
    blobin.pbData = (BYTE *)crypted->data;
    if (CryptUnprotectData(&blobin, &descr, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobout))
    {
        if (0 == lstrcmpW(descr, description))
            orig = svn_string_ncreate((const char *)blobout.pbData, blobout.cbData, pool);
        LocalFree(blobout.pbData);
        LocalFree(descr);
    }
    return orig;
}

const svn_string_t * SVNAuthData::encrypt_data(const svn_string_t *orig, apr_pool_t *pool)
{
    DATA_BLOB blobin;
    DATA_BLOB blobout;
    const svn_string_t * crypted = NULL;

    blobin.cbData = (DWORD)orig->len;
    blobin.pbData = (BYTE *)orig->data;
    if (CryptProtectData(&blobin, description, NULL, NULL, NULL,
        CRYPTPROTECT_UI_FORBIDDEN, &blobout))
    {
        const svn_string_t * crypt = svn_string_ncreate((const char *)blobout.pbData,
                                     blobout.cbData, pool);
        if (crypt)
            crypted = svn_base64_encode_string2(crypt, false, pool);
        LocalFree(blobout.pbData);
    }
    return crypted;
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
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.password = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_PASSPHRASE_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.passphrase = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_PASSTYPE_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.passtype = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_USERNAME_KEY) == 0)
        {
            CStringA data(value->data, (int)value->len);
            authinfodata.username = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_ASCII_CERT_KEY) == 0)
        {
            const svn_string_t * der_cert = nullptr;
            svn_x509_certinfo_t * certinfo = nullptr;
            const apr_array_header_t * hostnames = nullptr;
            svn_error_t *err;

            /* Convert header-less PEM to DER by undoing base64 encoding. */
            der_cert = svn_base64_decode_string(value, scratch_pool);

            err = svn_x509_parse_cert(&certinfo, der_cert->data, der_cert->len,
                                      scratch_pool, scratch_pool);
            if (err)
                continue;
            authinfodata.subject = svn_x509_certinfo_get_subject(certinfo, scratch_pool);
            authinfodata.validfrom = svn_time_to_human_cstring(svn_x509_certinfo_get_valid_from(certinfo), scratch_pool);
            authinfodata.validuntil = svn_time_to_human_cstring(svn_x509_certinfo_get_valid_to(certinfo), scratch_pool);
            authinfodata.issuer = svn_x509_certinfo_get_issuer(certinfo, scratch_pool);
            authinfodata.fingerprint = svn_checksum_to_cstring_display(svn_x509_certinfo_get_digest(certinfo), scratch_pool);

            hostnames = svn_x509_certinfo_get_hostnames(certinfo);
            if (hostnames && !apr_is_empty_array(hostnames))
            {
                int i;
                svn_stringbuf_t *buf = svn_stringbuf_create_empty(scratch_pool);
                for (i = 0; i < hostnames->nelts; ++i)
                {
                    const char *hostname = APR_ARRAY_IDX(hostnames, i, const char*);
                    if (i > 0)
                        svn_stringbuf_appendbytes(buf, ", ", 2);
                    svn_stringbuf_appendbytes(buf, hostname, strlen(hostname));
                }
                authinfodata.hostname = buf->data;
            }
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
