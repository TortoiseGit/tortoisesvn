// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008, 2011-2013 - TortoiseSVN

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
        *credentials = NULL;
        Creds cr = tsvn_creds[realmstring];
        svn_auth_cred_simple_t *creds = (svn_auth_cred_simple_t *)apr_pcalloc(pool, sizeof(*creds));
        char * t = cr.GetUsername();
        if (t==NULL)
            return SVN_NO_ERROR;
        creds->username = (char *)apr_pcalloc(pool, strlen(t)+1);
        strcpy_s((char*)creds->username, strlen(t)+1, t);
        SecureZeroMemory(t, strlen(t));
        delete [] t;
        t = cr.GetPassword();
        if (t==NULL)
            return SVN_NO_ERROR;
        creds->password = (char *)apr_pcalloc(pool, strlen(t)+1);
        strcpy_s((char*)creds->password, strlen(t)+1, t);
        SecureZeroMemory(t, strlen(t));
        delete [] t;
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


char * Creds::Decrypt( const char * text )
{
    DWORD dwLen = 0;
    if (CryptStringToBinaryA(text, (DWORD)strlen(text), CRYPT_STRING_HEX, NULL, &dwLen, NULL, NULL)==FALSE)
        return NULL;

    std::unique_ptr<BYTE[]> strIn(new BYTE[dwLen + 1]);
    if (CryptStringToBinaryA(text, (DWORD)strlen(text), CRYPT_STRING_HEX, strIn.get(), &dwLen, NULL, NULL)==FALSE)
        return NULL;

    DATA_BLOB blobin;
    blobin.cbData = dwLen;
    blobin.pbData = strIn.get();
    LPWSTR descr;
    DATA_BLOB blobout = {0};
    if (CryptUnprotectData(&blobin, &descr, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobout)==FALSE)
        return NULL;
    SecureZeroMemory(blobin.pbData, blobin.cbData);

    char * result = new char[blobout.cbData+1];
    strncpy_s(result, blobout.cbData+1, (const char*)blobout.pbData, blobout.cbData);
    SecureZeroMemory(blobout.pbData, blobout.cbData);
    LocalFree(blobout.pbData);
    LocalFree(descr);
    return result;
}

CStringA Creds::Encrypt( const char * text )
{
    DATA_BLOB blobin = {0};
    DATA_BLOB blobout = {0};
    CStringA result;

    blobin.cbData = (DWORD)strlen(text);
    blobin.pbData = (BYTE*) (LPCSTR)text;
    if (CryptProtectData(&blobin, L"TSVNAuth", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobout)==FALSE)
        return result;
    DWORD dwLen = 0;
    if (CryptBinaryToStringA(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX, NULL, &dwLen)==FALSE)
        return result;
    std::unique_ptr<char[]> strOut(new char[dwLen + 1]);
    if (CryptBinaryToStringA(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX, strOut.get(), &dwLen)==FALSE)
        return result;
    LocalFree(blobout.pbData);

    result = strOut.get();

    return result;
}



