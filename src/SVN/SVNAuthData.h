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

#pragma once

#include "SVNBase.h"
#include "SVNPrompt.h"
#include <vector>
#include <tuple>

struct SVNAuthDataInfo
{
    CString             username;
    CString             password;
    CString             passphrase;
    CString             passtype;
    CString             subject;
    CString             hostname;
    CString             validfrom;
    CString             validuntil;
    CString             issuer;
    CString             fingerprint;
    CString             failures;
};

/**
 * \ingroup SVN
 * Handles Subversion auth data
 */
class SVNAuthData : public SVNBase
{
private:
    SVNAuthData(const SVNAuthData&);
    SVNAuthData& operator=(SVNAuthData&);
public:
    SVNAuthData();
    ~SVNAuthData(void);

    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> GetAuthList();
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> DeleteAuthList(const std::vector<std::tuple<CString, CString, SVNAuthDataInfo>>& delList);

    bool ExportAuthData(const CString& targetpath, const CString& password, bool overwrite = false);
    bool ImportAuthData(const CString& importpath, const CString& password, bool overwrite = false);

    static const svn_string_t * SVNAuthData::decrypt_data(const svn_string_t *crypted, apr_pool_t *pool);
    static const svn_string_t * SVNAuthData::encrypt_data(const svn_string_t *orig, apr_pool_t *pool);

protected:
    apr_pool_t *                m_pool;         ///< the memory pool
    SVNPrompt                   m_prompt;       ///< auth_baton setup helper

    static svn_error_t * cleanup_callback(svn_boolean_t *delete_cred, void *cleanup_baton,
                                          const char *cred_kind, const char *realmstring,
                                          apr_hash_t *hash, apr_pool_t *scratch_pool);
    static svn_error_t * auth_callback(svn_boolean_t *delete_cred, void *auth_baton,
                                          const char *cred_kind, const char *realmstring,
                                          apr_hash_t *hash, apr_pool_t *scratch_pool);
};

