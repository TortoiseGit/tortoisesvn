// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "..\\TortoiseShell\\resource.h"
#include "registry.h"  // Just provides stdstring def
#include "SVNProperties.h"
#include "SVNStatus.h"
#include "UnicodeStrings.h"
#include "SVNHelpers.h"

#ifdef _MFC_VER
#	include "SVN.h"
#	include "UnicodeUtils.h"
#	include "registry.h"
#	include "Utils.h"
#else
#include "registry.h"
extern	HINSTANCE			g_hResInst;
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
	m_error = svn_client_proplist2 (&m_props,
								m_path.GetSVNApiPath(),
								&rev,
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

SVNProperties::SVNProperties(const CTSVNPath& filepath, SVNRev rev)
: m_rev(SVNRev::REV_WC)
{
	m_rev = rev;
#else

SVNProperties::SVNProperties(const CTSVNPath& filepath)
{
#endif
	m_error = NULL;
	m_pool = svn_pool_create (NULL);				// create the memory pool

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", m_pool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", m_pool);

	memset (&m_ctx, 0, sizeof (m_ctx));

#ifdef _MFC_VER
	svn_config_ensure(NULL, m_pool);
	// set up the configuration
	if (svn_config_get_config (&(m_ctx.config), g_pConfigDir, m_pool))
	{
		::MessageBox(NULL, this->GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (m_pool);					// free the allocated memory
		return;
	}
#endif
	
	m_path = filepath;
#ifdef _MFC_VER
	m_prompt.Init(m_pool, &m_ctx);

	m_path = filepath;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_ctx.config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
	//SVN::UseIEProxySettings(ctx.config);
#endif

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
	svn_string_t *propval = NULL;
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
			if ((svn_prop_needs_translation (pname_utf8))||(strncmp(pname_utf8, "bugtraq:", 8)==0)||(strncmp(pname_utf8, "tsvn:", 5)==0))
			{
				m_error = svn_subst_detranslate_string (&propval, propval, FALSE, m_pool);
				if (m_error != NULL)
					return _T("");
			}
		}
	}

	if (name)
		return key_native;
	else if (propval)
		return (TCHAR *)propval->data;
	else
		return NULL;
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

	SVNPool subpool(m_pool);

	pval = svn_string_create (Value, subpool);

	pname_utf8 = StringToUTF8(Name);
	if ((svn_prop_needs_translation (pname_utf8.c_str()))||(strncmp(pname_utf8.c_str(), "bugtraq:", 8)==0)||(strncmp(pname_utf8.c_str(), "tsvn:", 5)==0))
	{
		m_error = svn_subst_translate_string (&pval, pval, NULL, subpool);
		if (m_error != NULL)
			return FALSE;
	}
	if ((!m_path.IsDirectory())&&(((strncmp(pname_utf8.c_str(), "bugtraq:", 8)==0)||(strncmp(pname_utf8.c_str(), "tsvn:", 5)==0))))
	{
		//bugtraq: and tsvn: properties are not allowed on files.
#ifdef _MFC_VER
		CString temp;
		temp.LoadString(IDS_ERR_PROPNOTONFILE);
		CStringA tempA = CStringA(temp);
		m_error = svn_error_create(NULL, NULL, tempA);
#else
		TCHAR string[1024];
		LoadStringEx(g_hResInst, IDS_ERR_PROPNOTONFILE, string, 1024, (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
		std::string stringA = WideToMultibyte(wide_string(string));
		m_error = svn_error_create(NULL, NULL, stringA.c_str());
#endif
		return FALSE;
	}
	if (strncmp(pname_utf8.c_str(), "bugtraq:message", 15)==0)
	{
		// make sure that the bugtraq:message property only contains one single line!
		for (apr_size_t i=0; i<pval->len; ++i)
		{
			if ((pval->data[i] == '\n')||(pval->data[0] == '\r'))
			{
#ifdef _MFC_VER
				CString temp;
				temp.LoadString(IDS_ERR_PROPNOMULTILINE);
				CStringA tempA = CStringA(temp);
				m_error = svn_error_create(NULL, NULL, tempA);
#else
				TCHAR string[1024];
				LoadStringEx(g_hResInst, IDS_ERR_PROPNOMULTILINE, string, 1024, (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				std::string stringA = WideToMultibyte(wide_string(string));
				m_error = svn_error_create(NULL, NULL, stringA.c_str());
#endif
				return FALSE;
			}
		}
	}
	if ((recurse)&&((strncmp(pname_utf8.c_str(), "bugtraq:", 8)==0)||(strncmp(pname_utf8.c_str(), "tsvn:", 5)==0)))
	{
		//The bugtraq and tsvn properties must only be set on folders.
		CTSVNPath path;
		SVNStatus stat;
		svn_wc_status2_t * status = NULL;
		status = stat.GetFirstFileStatus(m_path, path);
		do 
		{
			if (status)
			{
				if ((status->entry)&&(status->entry->kind == svn_node_dir))
				{
					// a versioned folder, so set the property!
					m_error = svn_client_propset2 (pname_utf8.c_str(), pval, path.GetSVNApiPath(), false, false, &m_ctx, subpool);
#ifdef _MFC_VER
					//CShellUpdater::Instance().AddPathForUpdate(path);
#endif
				}
			}
			status = stat.GetNextFileStatus(path);
		} while ((status != 0)&&(m_error == NULL));
	}
	else 
	{
		m_error = svn_client_propset2 (pname_utf8.c_str(), pval, m_path.GetSVNApiPath(), recurse, false, &m_ctx, subpool);
#ifdef _MFC_VER
		//CShellUpdater::Instance().AddPathForUpdate(m_path);
#endif
	}
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

	m_error = svn_client_propset2 (pname_utf8.c_str(), NULL, m_path.GetSVNApiPath(), recurse, false, &m_ctx, m_pool);
#ifdef _MFC_VER
	//CShellUpdater::Instance().AddPathForUpdate(m_path);
#endif
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
	char errbuf[256];

	if (m_error != NULL)
	{
		svn_error_t * ErrPtr = m_error;
		if (ErrPtr->message)
			msg = UTF8ToString(ErrPtr->message);
		else
		{
			/* Is this a Subversion-specific error code? */
			if ((ErrPtr->apr_err > APR_OS_START_USEERR)
				&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
				msg = UTF8ToString(svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
			/* Otherwise, this must be an APR error code. */
			else
			{
				svn_error_t *temp_err = NULL;
				const char * err_string = NULL;
				temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
				if (temp_err)
				{
					svn_error_clear (temp_err);
					msg = _T("Can't recode error string from APR");
				}
				else
				{
					msg = UTF8ToString(err_string);
				}
			}
		}
		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			if (ErrPtr->message)
			{
				msg += _T("\n");
				msg += UTF8ToString(ErrPtr->message);
			}
		} 
		return msg;
	} 
	return msg;
}
