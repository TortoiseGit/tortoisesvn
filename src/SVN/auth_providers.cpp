// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "stdafx.h"
#include <apr_pools.h>
#include <apr_md5.h>
#include "svn_client.h"
#include "auth_providers.h"
#include "svn_error.h"
#include "svn_utf.h"
#include "svn_config.h"
#include "svn_md5.h"

#include "registry.h"
#include "UnicodeUtils.h"
#include "Wincrypt.h"
/*-----------------------------------------------------------------------*/
/* File provider                                                         */
/*-----------------------------------------------------------------------*/

/* The keys that will be stored on disk */
#define TSVN_CLIENT__AUTHFILE_USERNAME_KEY			"username"
#define TSVN_CLIENT__AUTHFILE_PASSWORD_KEY			"password"
#define TSVN_CLIENT__AUTH_REGISTRYKEY				"Software\\TortoiseSVN\\Auth\\"

#define TSVN_AUTH_OK   0
#define TSVN_NO_AUTH   1
#define TSVN_CRYPT_ERR 2

typedef BOOL (CALLBACK * LPFN_CryptProtectData)(DATA_BLOB * /*pDataIn*/, 
												LPCWSTR /*szDataDescr*/, 
												DATA_BLOB * /*pOptionalEntropy*/, 
												PVOID /*pvReserved*/, 
												CRYPTPROTECT_PROMPTSTRUCT* /*pPromptStruct*/, 
												DWORD /*dwFlags*/, 
												DATA_BLOB* /*pDataOut*/);
typedef BOOL (CALLBACK * LPFN_CryptUnProtectData)(DATA_BLOB* /*pDataIn*/,
												  LPWSTR* /*ppszDataDescr*/,
												  DATA_BLOB* /*pOptionalEntropy*/,
												  PVOID /*pvReserved*/,
												  CRYPTPROTECT_PROMPTSTRUCT* /*pPromptStruct*/,
												  DWORD /*dwFlags*/,
												  DATA_BLOB* /*pDataOut*/);

/* check if we're on Win2k or better */
bool IsWin2kOrLater(void)
{
	OSVERSIONINFOEX VersionInformation;
	ZeroMemory(&VersionInformation, sizeof(OSVERSIONINFOEX));
	VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&VersionInformation);
	return (VersionInformation.dwMajorVersion >= 5);
}

void tsvn_write_to_registry(std::string key, BYTE * data, DWORD len)
{
	HKEY hKey;
	DWORD disp;
	std::string::size_type pos = key.find_last_of(_T('\\'));
	std::string path = key.substr(0, pos);
	std::string _key = key.substr(pos + 1);
	if (RegCreateKeyExA(HKEY_CURRENT_USER, path.c_str(), 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disp)!=ERROR_SUCCESS)
	{
		return;
	}
	RegSetValueExA(hKey, _key.c_str(), 0, REG_BINARY, data, len);
	RegCloseKey(hKey);
}

void tsvn_read_from_registry(std::string key, BYTE ** data, DWORD * len)
{
	HKEY hKey;
	DWORD type;
	DWORD size;
	std::string::size_type pos = key.find_last_of(_T('\\'));
	std::string path = key.substr(0, pos);
	std::string _key = key.substr(pos + 1);
	*len = 0;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, path.c_str(), 0, KEY_EXECUTE, &hKey)!=ERROR_SUCCESS)
	{
		return;
	}
	RegQueryValueExA(hKey, _key.c_str(), NULL, &type, NULL, (LPDWORD) &size);
	*data = (BYTE *)LocalAlloc(LPTR, size);
	RegQueryValueExA(hKey, _key.c_str(), NULL, &type, *data, (LPDWORD) &size);
	*len = size;
	RegCloseKey(hKey);
}

int tsvn_read_auth_data (apr_hash_t **hash,
						 const char *cred_kind,
						 const char *realmstring,
						 apr_pool_t *pool)
{
	unsigned char digest[APR_MD5_DIGESTSIZE];
	const char * hexname;
	HINSTANCE hDLL;
	LPFN_CryptUnProtectData lpfnCryptUnProtectData;
	
	if (!IsWin2kOrLater())
		return TSVN_CRYPT_ERR;

	apr_md5 (digest, realmstring, strlen(realmstring));
	hexname = svn_md5_digest_to_cstring (digest, pool);
	std::string regbase = std::string(TSVN_CLIENT__AUTH_REGISTRYKEY);
	regbase += cred_kind;
	regbase += "\\";
	regbase += hexname;
	regbase += "\\";
	std::string reguser = regbase+TSVN_CLIENT__AUTHFILE_USERNAME_KEY;
	std::string regkey = regbase+TSVN_CLIENT__AUTHFILE_PASSWORD_KEY;
	hDLL = LoadLibrary(_T("Crypt32.dll"));
	if (hDLL == NULL)
		return TSVN_CRYPT_ERR;
	lpfnCryptUnProtectData = (LPFN_CryptUnProtectData)GetProcAddress(hDLL, "CryptUnprotectData");
	if (lpfnCryptUnProtectData)
	{
		DATA_BLOB blobin;
		DATA_BLOB blobout;
		tsvn_read_from_registry(reguser, &blobin.pbData, &blobin.cbData);
		if (blobin.cbData == 0)
		{
			FreeLibrary(hDLL);
			return TSVN_NO_AUTH;
		}
		if (lpfnCryptUnProtectData(&blobin, NULL, NULL, NULL, NULL, NULL, &blobout))
		{
			LocalFree(blobin.pbData);
			*hash = apr_hash_make (pool);
			if (*hash == NULL)
			{
				FreeLibrary(hDLL);
				return TSVN_CRYPT_ERR;
			}
			apr_hash_set(*hash, TSVN_CLIENT__AUTHFILE_USERNAME_KEY, APR_HASH_KEY_STRING, svn_string_ncreate((const char *)blobout.pbData, blobout.cbData, pool));
			LocalFree(blobout.pbData);
			tsvn_read_from_registry(regkey, &blobin.pbData, &blobin.cbData);
			if (blobin.cbData == 0)
			{
				FreeLibrary(hDLL);
				return TSVN_NO_AUTH;
			}
			if (lpfnCryptUnProtectData(&blobin, NULL, NULL, NULL, NULL, NULL, &blobout))
			{
				LocalFree(blobin.pbData);
				apr_hash_set(*hash, TSVN_CLIENT__AUTHFILE_PASSWORD_KEY, APR_HASH_KEY_STRING, svn_string_ncreate((const char *)blobout.pbData, blobout.cbData, pool));
				LocalFree(blobout.pbData);
				FreeLibrary(hDLL);
				return TSVN_AUTH_OK;
			}
			LocalFree(blobin.pbData);
		}
		LocalFree(blobin.pbData);
	}
	FreeLibrary(hDLL);
	return TSVN_CRYPT_ERR;
}

int tsvn_write_auth_data (apr_hash_t *hash,
							const char *cred_kind,
							const char *realmstring,
							apr_pool_t *pool)
{
	unsigned char digest[APR_MD5_DIGESTSIZE];
	const char * hexname;
	HINSTANCE hDLL;
	LPFN_CryptProtectData lpfnCryptProtectData;
	if (!IsWin2kOrLater())
		return TSVN_CRYPT_ERR;

	apr_md5 (digest, realmstring, strlen(realmstring));
	hexname = svn_md5_digest_to_cstring (digest, pool);
	std::string regbase = std::string(TSVN_CLIENT__AUTH_REGISTRYKEY);
	regbase += cred_kind;
	regbase += "\\";
	regbase += hexname;
	regbase += "\\";

	std::string reguser = regbase+TSVN_CLIENT__AUTHFILE_USERNAME_KEY;
	std::string regkey = regbase+TSVN_CLIENT__AUTHFILE_PASSWORD_KEY;

	hDLL = LoadLibrary(_T("Crypt32.dll"));
	if (hDLL == NULL)
		return TSVN_CRYPT_ERR;
	lpfnCryptProtectData = (LPFN_CryptProtectData)GetProcAddress(hDLL, "CryptProtectData");
	if (lpfnCryptProtectData)
	{
		DATA_BLOB blobin;
		DATA_BLOB blobout;
		svn_string_t *username = (svn_string_t *)apr_hash_get (hash,
			SVN_AUTH_PARAM_DEFAULT_USERNAME,
			APR_HASH_KEY_STRING);
		svn_string_t *password = (svn_string_t *)apr_hash_get (hash,
			SVN_AUTH_PARAM_DEFAULT_PASSWORD,
			APR_HASH_KEY_STRING);
		if ((!username) || (!password))
		{
			FreeLibrary(hDLL);
			return TSVN_NO_AUTH;
		}
		blobin.cbData = username->len;
		blobin.pbData = (BYTE *)username->data;
		if (lpfnCryptProtectData(&blobin, NULL, NULL, NULL, NULL, NULL, &blobout))
		{
			tsvn_write_to_registry(reguser, blobout.pbData, blobout.cbData);
			LocalFree(blobout.pbData);
			blobin.cbData = password->len;
			blobin.pbData = (BYTE *)password->data;
			if (lpfnCryptProtectData(&blobin, NULL, NULL, NULL, NULL, NULL, &blobout))
			{
				tsvn_write_to_registry(regkey, blobout.pbData, blobout.cbData);
				LocalFree(blobout.pbData);
				FreeLibrary(hDLL);
				return TSVN_AUTH_OK;
			}
		}
	}
	FreeLibrary(hDLL);
	return TSVN_CRYPT_ERR;
}

/* Get the username from the OS */
static const char *
get_os_username (apr_pool_t *pool)
{
	const char *username_utf8;
	char *username;
	apr_uid_t uid;
	apr_gid_t gid;

	if (apr_uid_current (&uid, &gid, pool) == APR_SUCCESS &&
		apr_uid_name_get (&username, uid, pool) == APR_SUCCESS)
	{
		svn_error_t *err;
		err = svn_utf_cstring_to_utf8 (&username_utf8, username, pool);
		svn_error_clear (err);
		if (! err)
			return username_utf8;
	}

	return NULL;
}

static svn_error_t *
tsvn_simple_first_creds (void **credentials,
					void **iter_baton,
					void *provider_baton,
					apr_hash_t *parameters,
					const char *realmstring,
					apr_pool_t *pool)
{
	const char *config_dir = (const char *)apr_hash_get (parameters,
		SVN_AUTH_PARAM_CONFIG_DIR,
		APR_HASH_KEY_STRING);
	const char *username = (const char *)apr_hash_get (parameters,
		SVN_AUTH_PARAM_DEFAULT_USERNAME,
		APR_HASH_KEY_STRING);
	const char *password = (const char *)apr_hash_get (parameters,
		SVN_AUTH_PARAM_DEFAULT_PASSWORD,
		APR_HASH_KEY_STRING);
	svn_boolean_t may_save = username || password;
	svn_error_t *err = NULL;

	/* If we don't have a usename and a password yet, we try the auth cache */
	if (! (username && password))
	{
		apr_hash_t *creds_hash = NULL;

		/* Try to load credentials from a file on disk, based on the
		realmstring.  Don't throw an error, though: if something went
		wrong reading the file, no big deal.  What really matters is that
		we failed to get the creds, so allow the auth system to try the
		next provider. */

		if (tsvn_read_auth_data(&creds_hash, SVN_AUTH_CRED_SIMPLE, realmstring, pool) != TSVN_AUTH_OK)
			err = svn_config_read_auth_data (&creds_hash, SVN_AUTH_CRED_SIMPLE,
				realmstring, config_dir, pool);
		svn_error_clear (err);
		if (! err && creds_hash)
		{
			svn_string_t *str;
			if (! username)
			{
				str = (svn_string_t *)apr_hash_get (creds_hash,
					TSVN_CLIENT__AUTHFILE_USERNAME_KEY,
					APR_HASH_KEY_STRING);
				if (str && str->data)
					username = str->data;
			}

			if (! password)
			{
				str = (svn_string_t *)apr_hash_get (creds_hash,
					TSVN_CLIENT__AUTHFILE_PASSWORD_KEY,
					APR_HASH_KEY_STRING);
				if (str && str->data)
					password = str->data;
			}
		}
	}

	/* Ask the OS for the username if we have a password but no
	username. */
	if (password && ! username)
		username = get_os_username (pool);

	if (username && password)
	{
		svn_auth_cred_simple_t *creds = (svn_auth_cred_simple_t *)apr_pcalloc (pool, sizeof(*creds));
		creds->username = username;
		creds->password = password;
		creds->may_save = may_save;
		*credentials = creds;
	}
	else
		*credentials = NULL;

	*iter_baton = NULL;

	return SVN_NO_ERROR;
}


static svn_error_t *
tsvn_simple_save_creds (svn_boolean_t *saved,
				   void *credentials,
				   void *provider_baton,
				   apr_hash_t *parameters,
				   const char *realmstring,
				   apr_pool_t *pool)
{
	svn_auth_cred_simple_t *creds = (svn_auth_cred_simple_t *)credentials;
	apr_hash_t *creds_hash = NULL;
	const char *config_dir;
	svn_error_t *err = NULL;

	*saved = FALSE;

	if (! creds->may_save)
		return SVN_NO_ERROR;

	config_dir = (const char *)apr_hash_get (parameters,
		SVN_AUTH_PARAM_CONFIG_DIR,
		APR_HASH_KEY_STRING);

	/* Put the credentials in a hash and save it to disk */
	creds_hash = apr_hash_make (pool);
	apr_hash_set (creds_hash, SVN_AUTH_PARAM_DEFAULT_USERNAME,
		APR_HASH_KEY_STRING,
		svn_string_create (creds->username, pool));
	apr_hash_set (creds_hash, SVN_AUTH_PARAM_DEFAULT_PASSWORD,
		APR_HASH_KEY_STRING,
		svn_string_create (creds->password, pool));
	if (tsvn_write_auth_data(creds_hash, SVN_AUTH_CRED_SIMPLE, realmstring, pool) == TSVN_CRYPT_ERR)
		err = svn_config_write_auth_data (creds_hash, SVN_AUTH_CRED_SIMPLE,
			realmstring, config_dir, pool);
	svn_error_clear (err);
	*saved = ! err;

	return SVN_NO_ERROR;
}


static const svn_auth_provider_t tsvn_simple_provider = {
	SVN_AUTH_CRED_SIMPLE,
		tsvn_simple_first_creds,
		NULL,
		tsvn_simple_save_creds
};


/* Public API */
void
tsvn_client_get_simple_provider (svn_auth_provider_object_t **provider,
								apr_pool_t *pool)
{
	svn_auth_provider_object_t *po = (svn_auth_provider_object_t *)apr_pcalloc (pool, sizeof(*po));

	po->vtable = &tsvn_simple_provider;
	*provider = po;
}


/*-----------------------------------------------------------------------*/
/* Prompt provider                                                       */
/*-----------------------------------------------------------------------*/

/* Baton type for username/password prompting. */
typedef struct
{
	svn_auth_simple_prompt_func_t prompt_func;
	void *prompt_baton;

	/* how many times to re-prompt after the first one fails */
	int retry_limit;
} simple_prompt_provider_baton_t;


/* Iteration baton type for username/password prompting. */
typedef struct
{
	/* how many times we've reprompted */
	int retries;
} simple_prompt_iter_baton_t;



/*** Helper Functions ***/
static svn_error_t *
tsvn_prompt_for_simple_creds (svn_auth_cred_simple_t **cred_p,
						 simple_prompt_provider_baton_t *pb,
						 apr_hash_t *parameters,
						 const char *realmstring,
						 svn_boolean_t first_time,
						 svn_boolean_t may_save,
						 apr_pool_t *pool)
{
	const char *def_username = NULL, *def_password = NULL;

	*cred_p = NULL;

	/* If we're allowed to check for default usernames and passwords, do
	so. */
	if (first_time)
	{
		def_username = (const char *)apr_hash_get (parameters,
			SVN_AUTH_PARAM_DEFAULT_USERNAME,
			APR_HASH_KEY_STRING);

		/* No default username?  Try the UID. */
		if (! def_username)
			def_username = get_os_username (pool);

		def_password = (const char *)apr_hash_get (parameters,
			SVN_AUTH_PARAM_DEFAULT_PASSWORD,
			APR_HASH_KEY_STRING);
	}

	/* If we have defaults, just build the cred here and return it.
	*
	* ### I do wonder why this is here instead of in a separate
	* ### 'defaults' provider that would run before the prompt
	* ### provider... Hmmm.
	*/
	if (def_username && def_password)
	{
		*cred_p = (svn_auth_cred_simple_t *)apr_palloc (pool, sizeof (**cred_p));
		(*cred_p)->username = apr_pstrdup (pool, def_username);
		(*cred_p)->password = apr_pstrdup (pool, def_password);
		(*cred_p)->may_save = TRUE;
	}
	else
	{
		SVN_ERR (pb->prompt_func (cred_p, pb->prompt_baton, realmstring,
			def_username, may_save, pool));
	}

	return SVN_NO_ERROR;
}


/* Our first attempt will use any default username/password passed
in, and prompt for the remaining stuff. */
static svn_error_t *
tsvn_simple_prompt_first_creds (void **credentials_p,
						   void **iter_baton,
						   void *provider_baton,
						   apr_hash_t *parameters,
						   const char *realmstring,
						   apr_pool_t *pool)
{
	simple_prompt_provider_baton_t *pb = (simple_prompt_provider_baton_t *)provider_baton;
	simple_prompt_iter_baton_t *ibaton = (simple_prompt_iter_baton_t *)apr_pcalloc (pool, sizeof (*ibaton));
	const char *no_auth_cache = (const char *)apr_hash_get (parameters,
		SVN_AUTH_PARAM_NO_AUTH_CACHE,
		APR_HASH_KEY_STRING);

	SVN_ERR (tsvn_prompt_for_simple_creds ((svn_auth_cred_simple_t **) credentials_p,
		pb, parameters, realmstring, TRUE,
		! no_auth_cache, pool));

	ibaton->retries = 0;
	*iter_baton = ibaton;

	return SVN_NO_ERROR;
}


/* Subsequent attempts to fetch will ignore the default values, and
simply re-prompt for both, up to a maximum of ib->pb->retry_limit. */
static svn_error_t *
tsvn_simple_prompt_next_creds (void **credentials_p,
						  void *iter_baton,
						  void *provider_baton,
						  apr_hash_t *parameters,
						  const char *realmstring,
						  apr_pool_t *pool)
{
	simple_prompt_iter_baton_t *ib = (simple_prompt_iter_baton_t *)iter_baton;
	simple_prompt_provider_baton_t *pb = (simple_prompt_provider_baton_t *)provider_baton;
	const char *no_auth_cache = (const char *)apr_hash_get (parameters,
		SVN_AUTH_PARAM_NO_AUTH_CACHE,
		APR_HASH_KEY_STRING);

	if (ib->retries >= pb->retry_limit)
	{
		/* give up, go on to next provider. */
		*credentials_p = NULL;
		return SVN_NO_ERROR;
	}
	ib->retries++;

	SVN_ERR (tsvn_prompt_for_simple_creds ((svn_auth_cred_simple_t **) credentials_p,
		pb, parameters, realmstring, FALSE,
		! no_auth_cache, pool));

	return SVN_NO_ERROR;
}


static const svn_auth_provider_t tsvn_simple_prompt_provider = {
	SVN_AUTH_CRED_SIMPLE,
		tsvn_simple_prompt_first_creds,
		tsvn_simple_prompt_next_creds,
		NULL,
};


/* Public API */
void
tsvn_client_get_simple_prompt_provider (
									   svn_auth_provider_object_t **provider,
									   svn_auth_simple_prompt_func_t prompt_func,
									   void *prompt_baton,
									   int retry_limit,
									   apr_pool_t *pool)
{
	svn_auth_provider_object_t *po = (svn_auth_provider_object_t *)apr_pcalloc (pool, sizeof(*po));
	simple_prompt_provider_baton_t *pb = (simple_prompt_provider_baton_t *)apr_pcalloc (pool, sizeof(*pb));

	pb->prompt_func = prompt_func;
	pb->prompt_baton = prompt_baton;
	pb->retry_limit = retry_limit;

	po->vtable = &tsvn_simple_prompt_provider;
	po->provider_baton = pb;
	*provider = po;
}

//////////////////////////////////////////////////////////////////////////////////////////

