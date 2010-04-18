// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "TSVNAuth.h"

std::map<CStringA,Creds> tsvn_creds;

/* Get cached encrypted credentials from the simple provider's cache. */
svn_error_t * tsvn_simple_first_creds(void **credentials,
                                      void **iter_baton,
                                      void * /*provider_baton*/,
                                      apr_hash_t * /*parameters*/,
                                      const char *realmstring,
                                      apr_pool_t *pool)
{
    *iter_baton = NULL;
    if (tsvn_creds.find(realmstring) != tsvn_creds.end())
    {
        Creds cr = tsvn_creds[realmstring];
        svn_auth_cred_simple_t *creds = (svn_auth_cred_simple_t *)apr_pcalloc(pool, sizeof(*creds));
        creds->username = cr.username;
        creds->password = cr.password;
        creds->may_save = false;
        *credentials = creds;
    }
    else
        *credentials = NULL;

    return SVN_NO_ERROR;
}


const svn_auth_provider_t tsvn_simple_provider = {
    SVN_AUTH_CRED_SIMPLE,
    tsvn_simple_first_creds,
    NULL,
    NULL
};

void svn_auth_get_tsvn_simple_provider(svn_auth_provider_object_t **provider,
                                       apr_pool_t *pool)
{
    svn_auth_provider_object_t *po = (svn_auth_provider_object_t *)apr_pcalloc(pool, sizeof(*po));

    po->vtable = &tsvn_simple_provider;
    *provider = po;
}

