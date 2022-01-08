// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013-2015, 2020-2022 - TortoiseSVN

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
#include "svn_hash.h"
#pragma warning(pop)
#include "resource.h"
#include "SVNAuthData.h"
#include "UnicodeUtils.h"
#include "StringUtils.h"
#include "SVNConfig.h"
#include "SVNHelpers.h"
#include "SVNGlobal.h"

#include <Wincrypt.h>


SVNAuthData::SVNAuthData()
    : SVNBase()
    , m_prompt(false)
{
    m_pool = svn_pool_create(nullptr);

    svn_error_clear(svn_client_create_context2(&m_pCtx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

    // set up authentication
    m_prompt.Init(m_pool, m_pCtx);
    const char *path = nullptr;
    svn_config_get_user_config_path(&path, g_pConfigDir, nullptr, m_pool);

    svn_auth_set_parameter(m_pCtx->auth_baton, SVN_AUTH_PARAM_CONFIG_DIR, path);

    m_pCtx->client_name = SVNHelper::GetUserAgentString(m_pool);
}

SVNAuthData::~SVNAuthData()
{
    svn_error_clear(m_err);
    svn_pool_destroy(m_pool); // free the allocated memory
}

static constexpr WCHAR  description[] = L"auth_svn.simple.wincrypt";
const svn_string_t *SVNAuthData::decrypt_data(const svn_string_t *crypted, apr_pool_t *pool)
{
    crypted = svn_base64_decode_string(crypted, pool);

    DATA_BLOB           blobin{};
    DATA_BLOB           blobout{};
    LPWSTR              descr{};
    const svn_string_t *orig = nullptr;

    blobin.cbData = static_cast<DWORD>(crypted->len);
    blobin.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(crypted->data));
    if (CryptUnprotectData(&blobin, &descr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &blobout))
    {
        if (0 == lstrcmpW(descr, description))
            orig = svn_string_ncreate(reinterpret_cast<const char *>(blobout.pbData), blobout.cbData, pool);
        LocalFree(blobout.pbData);
        LocalFree(descr);
    }
    return orig;
}

const svn_string_t *SVNAuthData::encrypt_data(const svn_string_t *orig, apr_pool_t *pool)
{
    DATA_BLOB           blobIn{};
    DATA_BLOB           blobOut{};
    const svn_string_t *crypted = nullptr;

    blobIn.cbData = static_cast<DWORD>(orig->len);
    blobIn.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(orig->data));
    if (CryptProtectData(&blobIn, description, nullptr, nullptr, nullptr,
                         CRYPTPROTECT_UI_FORBIDDEN, &blobOut))
    {
        const svn_string_t *crypt = svn_string_ncreate(reinterpret_cast<const char *>(blobOut.pbData),
                                                       blobOut.cbData, pool);
        if (crypt)
            crypted = svn_base64_encode_string2(crypt, false, pool);
        LocalFree(blobOut.pbData);
    }
    return crypted;
}

svn_error_t *SVNAuthData::cleanup_callback(svn_boolean_t *deleteCred, void *cleanupBaton,
                                           const char *credKind, const char *realmString,
                                           apr_hash_t *hash, apr_pool_t *scratchPool)
{
    std::tuple<std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> *, std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>> *tupleBaton =
        static_cast<std::tuple<std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> *, std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>> *>(cleanupBaton);

    auto authList = std::get<0>(*tupleBaton);
    auto delList  = std::get<1>(*tupleBaton);

    CString s1, s2;
    if (credKind)
        s1 = CUnicodeUtils::GetUnicode(credKind);
    if (realmString)
        s2 = CUnicodeUtils::GetUnicode(realmString);

    SVNAuthDataInfo authInfoData;

    for (apr_hash_index_t *hi = apr_hash_first(scratchPool, hash); hi; hi = apr_hash_next(hi))
    {
        const void *vKey;
        void *      val;

        apr_hash_this(hi, &vKey, nullptr, &val);
        const char *  key   = static_cast<const char *>(vKey);
        svn_string_t *value = static_cast<svn_string_t *>(val);
        if (strcmp(key, SVN_CONFIG_AUTHN_PASSWORD_KEY) == 0)
        {
            CStringA data(value->data, static_cast<int>(value->len));
            authInfoData.password = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_PASSPHRASE_KEY) == 0)
        {
            CStringA data(value->data, static_cast<int>(value->len));
            authInfoData.passPhrase = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_PASSTYPE_KEY) == 0)
        {
            CStringA data(value->data, static_cast<int>(value->len));
            authInfoData.passType = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_USERNAME_KEY) == 0)
        {
            CStringA data(value->data, static_cast<int>(value->len));
            authInfoData.userName = CUnicodeUtils::GetUnicode(data);
        }
        else if (strcmp(key, SVN_CONFIG_AUTHN_ASCII_CERT_KEY) == 0)
        {
            const svn_string_t *      derCert   = nullptr;
            svn_x509_certinfo_t *     certInfo  = nullptr;
            const apr_array_header_t *hostnames = nullptr;

            /* Convert header-less PEM to DER by undoing base64 encoding. */
            derCert = svn_base64_decode_string(value, scratchPool);

            auto err = svn_x509_parse_cert(&certInfo, derCert->data, derCert->len,
                                           scratchPool, scratchPool);
            if (err)
                continue;
            authInfoData.subject     = svn_x509_certinfo_get_subject(certInfo, scratchPool);
            authInfoData.validFrom   = svn_time_to_human_cstring(svn_x509_certinfo_get_valid_from(certInfo), scratchPool);
            authInfoData.validUntil  = svn_time_to_human_cstring(svn_x509_certinfo_get_valid_to(certInfo), scratchPool);
            authInfoData.issuer      = svn_x509_certinfo_get_issuer(certInfo, scratchPool);
            authInfoData.fingerPrint = svn_checksum_to_cstring_display(svn_x509_certinfo_get_digest(certInfo), scratchPool);

            hostnames = svn_x509_certinfo_get_hostnames(certInfo);
            if (hostnames && !apr_is_empty_array(hostnames))
            {
                int              i;
                svn_stringbuf_t *buf = svn_stringbuf_create_empty(scratchPool);
                for (i = 0; i < hostnames->nelts; ++i)
                {
                    const char *hostname = APR_ARRAY_IDX(hostnames, i, const char *);
                    if (i > 0)
                        svn_stringbuf_appendbytes(buf, ", ", 2);
                    svn_stringbuf_appendbytes(buf, hostname, strlen(hostname));
                }
                authInfoData.hostName = buf->data;
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
            s.Format(IDS_ERR_SSL_VALIDATE, static_cast<LPCWSTR>(authInfoData.hostName));
            authInfoData.failures = s;

            authInfoData.failures += L"\n";

            if (failures & SVN_AUTH_SSL_NOTYETVALID)
            {
                s.Format(IDS_ERR_SSL_NOTYETVALID, static_cast<LPCWSTR>(authInfoData.validFrom));
                authInfoData.failures += s;
                authInfoData.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_EXPIRED)
            {
                s.Format(IDS_ERR_SSL_EXPIRED, static_cast<LPCWSTR>(authInfoData.validUntil));
                authInfoData.failures += s;
                authInfoData.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_CNMISMATCH)
            {
                s.Format(IDS_ERR_SSL_CNMISMATCH, static_cast<LPCWSTR>(authInfoData.hostName));
                authInfoData.failures += s;
                authInfoData.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_UNKNOWNCA)
            {
                s.FormatMessage(IDS_ERR_SSL_UNKNOWNCA, static_cast<LPCWSTR>(authInfoData.fingerPrint), static_cast<LPCWSTR>(authInfoData.hostName));
                authInfoData.failures += s;
                authInfoData.failures += L"\n";
            }

            if (failures & SVN_AUTH_SSL_OTHER)
            {
                // unknown verification failure
            }
        }
    }

    for (auto& it : delList)
    {
        if ((s1.Compare(std::get<0>(it)) == 0) &&
            (s2.Compare(std::get<1>(it)) == 0))
        {
            *deleteCred = true;
        }
    }
    authList->emplace_back(s1, s2, authInfoData);

    return nullptr;
}

std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> SVNAuthData::GetAuthList()
{
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> authList;
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> delList;
    auto                                                       cleanupBaton = std::make_tuple(&authList, delList);
    SVNPool                                                    subPool(m_pool);
    m_err = svn_config_walk_auth_data(g_pConfigDir, cleanup_callback, &cleanupBaton, subPool);
    if (m_err)
    {
        authList.clear();
    }
    return authList;
}

std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> SVNAuthData::DeleteAuthList(const std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> &delList)
{
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> authList;
    auto                                                       cleanupBaton = std::make_tuple(&authList, delList);
    SVNPool                                                    subPool(m_pool);
    m_err = svn_config_walk_auth_data(g_pConfigDir, cleanup_callback, &cleanupBaton, subPool);
    CRegString rSyncPath(L"Software\\TortoiseSVN\\SyncPath");
    CTSVNPath  syncPath = CTSVNPath(CString(rSyncPath));
    if (!syncPath.IsEmpty() && syncPath.Exists())
    {
        svn_error_clear(svn_config_walk_auth_data(syncPath.GetSVNApiPath(subPool), cleanup_callback, &cleanupBaton, subPool));
    }
    return GetAuthList();
}

svn_error_t *SVNAuthData::auth_callback(svn_boolean_t * /*delete_cred*/, void *authBaton,
                                        const char *credKind, const char *realmString,
                                        apr_hash_t * /*hash*/, apr_pool_t * /*scratch_pool*/)
{
    std::vector<std::tuple<std::string, std::string>> *authdata = static_cast<std::vector<std::tuple<std::string, std::string>> *>(authBaton);

    authdata->emplace_back(credKind, realmString);

    return nullptr;
}

bool SVNAuthData::ExportAuthData(const CString &targetpath, const CString &password, bool overwrite /*= false*/)
{
    std::vector<std::tuple<std::string, std::string>> authdata;
    SVNPool                                           subPool(m_pool);
    m_err = svn_config_walk_auth_data(g_pConfigDir, auth_callback, &authdata, subPool);

    CString tp = targetpath;
    tp.Replace('\\', '/');
    std::string targetPathA = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(tp));
    std::string passwordA   = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(password));
    for (const auto &ad : authdata)
    {
        apr_hash_t *hash = nullptr;
        m_err            = svn_config_read_auth_data(&hash, std::get<0>(ad).c_str(), std::get<1>(ad).c_str(), nullptr, subPool);
        if (m_err)
        {
            svn_error_clear(m_err);
            return false;
        }

        if (!overwrite)
        {
            apr_hash_t *hash2 = nullptr;
            m_err             = svn_config_read_auth_data(&hash2, std::get<0>(ad).c_str(), std::get<1>(ad).c_str(), targetPathA.c_str(), subPool);
            if ((m_err != nullptr) || (hash2 != nullptr))
            {
                svn_error_clear(m_err);
                continue;
            }

            svn_error_clear(m_err);
        }

        const svn_string_t *pw = static_cast<const svn_string_t *>(svn_hash_gets(hash, SVN_CONFIG_AUTHN_PASSWORD_KEY));
        if (pw)
        {
            auto p = decrypt_data(pw, subPool);
            if (p == nullptr)
                continue;
            auto crypt = CStringUtils::Encrypt(std::string(p->data), passwordA);
            if (crypt.empty())
                continue;
            svn_hash_sets(hash, SVN_CONFIG_AUTHN_PASSWORD_KEY, svn_string_create(crypt.c_str(), subPool));
        }

        const svn_string_t *ps = static_cast<const svn_string_t *>(svn_hash_gets(hash, SVN_CONFIG_AUTHN_PASSPHRASE_KEY));
        if (ps)
        {
            auto p = decrypt_data(pw, subPool);
            if (p == nullptr)
                continue;
            auto crypt = CStringUtils::Encrypt(std::string(p->data), passwordA);
            if (crypt.empty())
                continue;
            svn_hash_sets(hash, SVN_CONFIG_AUTHN_PASSPHRASE_KEY, svn_string_create(crypt.c_str(), subPool));
        }

        m_err = svn_config_write_auth_data(hash, std::get<0>(ad).c_str(), std::get<1>(ad).c_str(), targetPathA.c_str(), subPool);
        if (m_err)
        {
            svn_error_clear(m_err);
            return false;
        }
    }
    svn_error_clear(m_err);
    return true;
}

bool SVNAuthData::ImportAuthData(const CString &importPath, const CString &password, bool overwrite /*= false*/)
{
    CString ip = importPath;
    ip.Replace('\\', '/');
    std::string                                       importPathA = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(ip));
    std::vector<std::tuple<std::string, std::string>> authData;
    SVNPool                                           subPool(m_pool);

    m_err = svn_config_walk_auth_data(importPathA.c_str(), auth_callback, &authData, subPool);

    std::string passwordA = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(password));
    for (const auto &ad : authData)
    {
        apr_hash_t *hash = nullptr;
        m_err            = svn_config_read_auth_data(&hash, std::get<0>(ad).c_str(), std::get<1>(ad).c_str(), importPathA.c_str(), subPool);
        if (m_err)
        {
            svn_error_clear(m_err);
            return false;
        }

        if (!overwrite)
        {
            apr_hash_t *hash2 = nullptr;
            m_err             = svn_config_read_auth_data(&hash2, std::get<0>(ad).c_str(), std::get<1>(ad).c_str(), g_pConfigDir, subPool);
            if ((m_err != nullptr) || (hash2 != nullptr))
            {
                svn_error_clear(m_err);
                continue;
            }

            svn_error_clear(m_err);
        }

        const svn_string_t *pw = static_cast<const svn_string_t *>(svn_hash_gets(hash, SVN_CONFIG_AUTHN_PASSWORD_KEY));
        if (pw)
        {
            auto decrypt = CStringUtils::Decrypt(std::string(pw->data), passwordA);
            auto p       = encrypt_data(svn_string_create(decrypt.c_str(), subPool), subPool);
            if (p == nullptr)
                continue;
            svn_hash_sets(hash, SVN_CONFIG_AUTHN_PASSWORD_KEY, p);
        }

        const svn_string_t *ps = static_cast<const svn_string_t *>(svn_hash_gets(hash, SVN_CONFIG_AUTHN_PASSPHRASE_KEY));
        if (ps)
        {
            auto decrypt = CStringUtils::Decrypt(std::string(ps->data), passwordA);
            auto p       = encrypt_data(svn_string_create(decrypt.c_str(), subPool), subPool);
            if (p == nullptr)
                continue;
            svn_hash_sets(hash, SVN_CONFIG_AUTHN_PASSPHRASE_KEY, p);
        }

        m_err = svn_config_write_auth_data(hash, std::get<0>(ad).c_str(), std::get<1>(ad).c_str(), g_pConfigDir, subPool);
        if (m_err)
        {
            svn_error_clear(m_err);
            return false;
        }
    }
    svn_error_clear(m_err);
    return true;
}
