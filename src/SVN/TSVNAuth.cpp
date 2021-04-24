// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008, 2011-2014, 2021 - TortoiseSVN

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
#include "TSVNAuth.h"

std::map<CStringA, Creds> tsvn_creds;

/* Get cached encrypted credentials from the simple provider's cache. */
svn_error_t *tsvnSimpleFirstCreds(void **credentials,
                                  void **iterBaton,
                                  void * /*provider_baton*/,
                                  apr_hash_t * /*parameters*/,
                                  const char *realmString,
                                  apr_pool_t *pool)
{
    *iterBaton = nullptr;
    if (tsvn_creds.find(realmString) != tsvn_creds.end())
    {
        *credentials                  = nullptr;
        Creds                   cr    = tsvn_creds[realmString];
        svn_auth_cred_simple_t *creds = static_cast<svn_auth_cred_simple_t *>(apr_pcalloc(pool, sizeof(*creds)));
        auto                    t     = cr.GetUsername();
        if (t.get() == nullptr)
            return nullptr;
        creds->username = static_cast<char *>(apr_pcalloc(pool, strlen(t.get()) + 1));
        strcpy_s(const_cast<char *>(creds->username), strlen(t.get()) + 1, t.get());
        SecureZeroMemory(t.get(), strlen(t.get()));
        t = cr.GetPassword();
        if (t == nullptr)
            return nullptr;
        creds->password = static_cast<char *>(apr_pcalloc(pool, strlen(t.get()) + 1));
        strcpy_s(const_cast<char *>(creds->password), strlen(t.get()) + 1, t.get());
        SecureZeroMemory(t.get(), strlen(t.get()));
        creds->may_save = false;
        *credentials    = creds;
    }
    else
        *credentials = nullptr;

    return nullptr;
}

const svn_auth_provider_t tsvn_simple_provider = {
    SVN_AUTH_CRED_SIMPLE,
    tsvnSimpleFirstCreds,
    nullptr,
    nullptr};

void svnAuthGetTsvnSimpleProvider(svn_auth_provider_object_t **provider,
                                  apr_pool_t *                 pool)
{
    svn_auth_provider_object_t *po = static_cast<svn_auth_provider_object_t *>(apr_pcalloc(pool, sizeof(*po)));

    po->vtable = &tsvn_simple_provider;
    *provider  = po;
}

bool Creds::SetUsername(const char *user)
{
    username = CStringUtils::Encrypt(user);
    return !username.IsEmpty();
}

bool Creds::SetPassword(const char *pass)
{
    password = CStringUtils::Encrypt(pass);
    return !password.IsEmpty();
}
