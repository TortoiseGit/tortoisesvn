// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "SVNProperties.h"
#include "UnicodeStrings.h"
#include "tchar.h"
#ifdef _MFC_VER
#	include "SVN.h"
#	include "UnicodeUtils.h"
#	include "registry.h"
#endif

svn_error_t*	SVNProperties::Refresh()
{
	svn_opt_revision_t			rev;
	m_error = NULL;

	m_propCount = 0;
#ifdef _MFC_VER
	rev.kind = ((svn_opt_revision_t *)m_rev)->kind;
	rev.value = ((svn_opt_revision_t *)m_rev)->value;
#else
	rev.kind = svn_opt_revision_unspecified;
	rev.value.number = -1;
#endif
	m_error = svn_client_proplist (&m_props,
								StringToUTF8(m_path).c_str(), 
								&rev,
								false,	//recurse
								&m_ctx,
								m_pool);
	if(m_error != NULL)
		return m_error;


	for (int j = 0; j < m_props->nelts; j++)
	{
		svn_client_proplist_item_t *item = ((svn_client_proplist_item_t **)m_props->elts)[j];

		const char *node_name_native;
		m_error = svn_utf_cstring_from_utf8_stringbuf (&node_name_native,
												item->node_name,
												m_pool);

		if (m_error != NULL)
			return m_error;

		apr_hash_index_t *hi;

		for (hi = apr_hash_first (m_pool, item->prop_hash); hi; hi = apr_hash_next (hi))
		{
			m_propCount++;
		} 
	}
	return NULL;
}

#ifdef _MFC_VER

void SVNProperties::SaveAuthentication(BOOL save)
{
	if (save)
	{
		svn_auth_set_parameter(ctx.auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, NULL);
	}
	else
	{
		svn_auth_set_parameter(ctx.auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, (void *) "");
	}
}

SVNProperties::SVNProperties(const TCHAR * filepath, SVNRev rev)
: m_rev(SVNRev::REV_WC)
{
	m_rev = rev;
#else

SVNProperties::SVNProperties(const TCHAR * filepath)
{
#endif
	m_pool = svn_pool_create (NULL);				// create the memory pool

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", m_pool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", m_pool);

	memset (&m_ctx, 0, sizeof (m_ctx));

	svn_config_ensure(NULL, m_pool);
	// set up the configuration
	if (svn_config_get_config (&(m_ctx.config), NULL, m_pool))
	{
		::MessageBox(NULL, this->GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (m_pool);					// free the allocated memory
		return;
	}
	
	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	m_path = UTF8ToString(svn_path_internal_style (StringToUTF8(filepath).c_str(), m_pool));

	// set up authentication
	svn_auth_provider_object_t *provider;

    /* The whole list of registered providers */
    apr_array_header_t *providers
      = apr_array_make (m_pool, 10, sizeof (svn_auth_provider_object_t *));

    /* The main disk-caching auth providers, for both
       'username/password' creds and 'username' creds.  */
	svn_client_get_simple_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_username_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

#ifdef _MFC_VER
	/* The server-cert, client-cert, and client-cert-password providers. */
	svn_client_get_ssl_server_trust_file_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_cert_file_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_cert_pw_file_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Two prompting providers, one for username/password, one for
	just username. */
	svn_client_get_simple_prompt_provider (&provider, (svn_auth_simple_prompt_func_t)simpleprompt, this, 2, /* retry limit */ m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_username_prompt_provider (&provider, (svn_auth_username_prompt_func_t)userprompt, this, 2, /* retry limit */ m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Three prompting providers for server-certs, client-certs,
	and client-cert-passphrases.  */
	svn_client_get_ssl_server_trust_prompt_provider (&provider, sslserverprompt, this, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_cert_prompt_provider (&provider, sslclientprompt, this, 2, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_cert_pw_prompt_provider (&provider, sslpwprompt, this, 2, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get ((apr_hash_t *)m_ctx.config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
	SVN::UseIEProxySettings(m_ctx.config);
#endif
	svn_auth_open (&m_auth_baton, providers, m_pool);

	m_ctx.auth_baton = m_auth_baton;

	SVNProperties::Refresh();
}

SVNProperties::~SVNProperties(void)
{
	svn_pool_destroy (m_pool);					// free the allocated memory
}

int SVNProperties::GetCount()
{
	return m_propCount;
}

stdstring SVNProperties::GetItem(int index, BOOL name)
{
	const void *key;
	stdstring key_native;
	void *val;
	svn_string_t *propval;
	const char *node_name_native;
	const char *pname_utf8;
	m_error = NULL;

	if (m_props == NULL)
	{
		return NULL;
	}
	if (index >= m_propCount)
	{
		return NULL;
	}

	long ind = 0;

	for (int j = 0; j < m_props->nelts; j++)
	{
		svn_client_proplist_item_t *item = ((svn_client_proplist_item_t **)m_props->elts)[j];

		m_error = svn_utf_cstring_from_utf8_stringbuf (&node_name_native,
													item->node_name,
													m_pool);
		if (m_error != NULL)
		{
			return NULL;
		}

		apr_hash_index_t *hi;

		for (hi = apr_hash_first (m_pool, item->prop_hash); hi; hi = apr_hash_next (hi))
		{
			if(ind++ != index)
				continue;

			apr_hash_this (hi, &key, NULL, &val);
			key_native = UTF8ToString((char *)key);
			propval = (svn_string_t *)val;
			pname_utf8 = (char *)key;

			//If this is a special Subversion property, it is stored as
			//UTF8, so convert to the native format.
			if (svn_prop_needs_translation (pname_utf8))
			{
				m_error = svn_subst_detranslate_string (&propval, propval, FALSE, m_pool);
				if (m_error != NULL)
					return NULL;
			}
		} 
	}

	if (name)
		return key_native;
	else
		return (TCHAR *)propval->data;
}

BOOL SVNProperties::IsSVNProperty(int index)
{
	const char *pname_utf8;
	const char *name = StringToUTF8(SVNProperties::GetItem(index, true)).c_str();

	svn_utf_cstring_to_utf8 (&pname_utf8, name, m_pool);
	svn_boolean_t is_svn_prop = svn_prop_needs_translation (pname_utf8);

	return is_svn_prop;
}

stdstring SVNProperties::GetItemName(int index)
{
	return SVNProperties::GetItem(index, true);
}

stdstring SVNProperties::GetItemValue(int index)
{
	return SVNProperties::GetItem(index, false);
}

BOOL SVNProperties::Add(const TCHAR * Name, const char * Value, BOOL recurse)
{
	svn_string_t*	pval;
	std::string		pname_utf8;
	m_error = NULL;

	pval = svn_string_create (Value, m_pool);

	pname_utf8 = StringToUTF8(Name);
	if (svn_prop_needs_translation (pname_utf8.c_str()))
	{
		m_error = svn_subst_translate_string (&pval, pval, NULL, m_pool);
		if (m_error != NULL)
			return FALSE;
	}

	m_error = svn_client_propset (pname_utf8.c_str(), pval, StringToUTF8(m_path).c_str(), recurse, m_pool);
	if (m_error != NULL)
	{
		return FALSE;
	}

	//rebuild the property list
	m_error = SVNProperties::Refresh();
	if (m_error != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL SVNProperties::Remove(const TCHAR * Name, BOOL recurse)
{
	std::string		pname_utf8;
	m_error = NULL;

	pname_utf8 = StringToUTF8(Name);

	m_error = svn_client_propset (pname_utf8.c_str(), NULL, StringToUTF8(m_path).c_str(), recurse, m_pool);
	if (m_error != NULL)
	{
		return FALSE;
	}

	//rebuild the property list
	m_error = Refresh();
	if (m_error != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

stdstring SVNProperties::GetLastErrorMsg()
{
	stdstring msg;
	if (m_error != NULL)
	{
		svn_error_t * ErrPtr = m_error;
		msg = UTF8ToString(ErrPtr->message);
		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			msg += _T("\n");
			msg += UTF8ToString(ErrPtr->message);
		} 
		return msg;
	} 
	return msg;
}
