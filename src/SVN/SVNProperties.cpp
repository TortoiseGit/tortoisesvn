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

#include "SVNProperties.h"
#include "UnicodeStrings.h"

svn_error_t*	SVNProperties::Refresh()
{
	svn_error_t*				error = NULL;
	svn_opt_revision_t			rev;

	m_propCount = 0;
	rev.kind = svn_opt_revision_unspecified;
	rev.value.number = -1;
	error = svn_client_proplist (&m_props,
								StringToUTF8(m_path).c_str(), 
								&rev,
								false,	//recurse
								&m_ctx,
								m_pool);
	if(error != NULL)
		return error;


	for (int j = 0; j < m_props->nelts; j++)
	{
		svn_client_proplist_item_t *item = ((svn_client_proplist_item_t **)m_props->elts)[j];

		const char *node_name_native;
		error = svn_utf_cstring_from_utf8_stringbuf (&node_name_native,
												item->node_name,
												m_pool);

		if (error != NULL)
			return error;

		apr_hash_index_t *hi;

		for (hi = apr_hash_first (m_pool, item->prop_hash); hi; hi = apr_hash_next (hi))
		{
			m_propCount++;
		} 
	}
	return NULL;
}

SVNProperties::SVNProperties(const TCHAR * filepath)
{
	m_pool = svn_pool_create (NULL);				// create the memory pool
	svn_config_ensure(NULL, m_pool);
	memset (&m_ctx, 0, sizeof (m_ctx));

	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	m_path = UTF8ToString(svn_path_internal_style (StringToUTF8(filepath).c_str(), m_pool));

	// set up authentication

    /* The whole list of registered providers */
    apr_array_header_t *providers
      = apr_array_make (m_pool, 1, sizeof (svn_auth_provider_object_t *));

    /* The main disk-caching auth providers, for both
       'username/password' creds and 'username' creds.  */
    svn_auth_provider_object_t *simple_wc_provider = (svn_auth_provider_object_t *)apr_pcalloc (m_pool, sizeof(*simple_wc_provider));

    svn_auth_provider_object_t *username_wc_provider = (svn_auth_provider_object_t *)apr_pcalloc (m_pool, sizeof(*username_wc_provider));

    svn_client_get_simple_provider (&(simple_wc_provider), m_pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = simple_wc_provider;

    svn_client_get_username_provider(&(username_wc_provider), m_pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = username_wc_provider;

	svn_auth_open (&m_auth_baton, providers, m_pool);

	//m_ctx.prompt_func = NULL;
	//m_ctx.prompt_baton = NULL;
	m_ctx.auth_baton = m_auth_baton;
	// set up the configuration
	svn_config_get_config (&(m_ctx.config), NULL, m_pool);

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
	svn_error_t*	error = NULL;

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

		error = svn_utf_cstring_from_utf8_stringbuf (&node_name_native,
													item->node_name,
													m_pool);
		if (error != NULL)
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
				error = svn_subst_detranslate_string (&propval, propval, FALSE, m_pool);
				if (error != NULL)
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
	svn_error_t*	error = NULL;
	svn_string_t*	pval;
	std::string		pname_utf8;


	pval = svn_string_create (Value, m_pool);

	pname_utf8 = StringToUTF8(Name);
	if (svn_prop_needs_translation (pname_utf8.c_str()))
	{
		error = svn_subst_translate_string (&pval, pval, NULL, m_pool);
		if (error != NULL)
			return FALSE;
	}

	error = svn_client_propset (pname_utf8.c_str(), pval, StringToUTF8(m_path).c_str(), recurse, m_pool);
	if (error != NULL)
	{
		return FALSE;
	}

	//rebuild the property list
	error = SVNProperties::Refresh();
	if (error != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL SVNProperties::Remove(const TCHAR * Name, BOOL recurse)
{
	std::string		pname_utf8;
	svn_error_t*	error = NULL;

	pname_utf8 = StringToUTF8(Name);

	error = svn_client_propset (pname_utf8.c_str(), NULL, StringToUTF8(m_path).c_str(), recurse, m_pool);
	if (error != NULL)
	{
		return FALSE;
	}

	//rebuild the property list
	error = Refresh();
	if (error != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

