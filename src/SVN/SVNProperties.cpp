// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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

#include "stdafx.h"
#include "..\\TortoiseShell\\resource.h"
#include "tstring.h"
#include "SVNProperties.h"
#include "SVNStatus.h"
#include "SVNHelpers.h"
#include "SVNTrace.h"
#pragma warning(push)
#include "svn_props.h"
#pragma warning(pop)

#ifdef _MFC_VER
#	include "SVN.h"
#	include "UnicodeUtils.h"
#	include "registry.h"
#	include "PathUtils.h"
#else
#include "registry.h"
extern	HINSTANCE			g_hResInst;
#endif


struct log_msg_baton3
{
	const char *message;  /* the message. */
	const char *message_encoding; /* the locale/encoding of the message. */
	const char *base_dir; /* the base directory for an external edit. UTF-8! */
	const char *tmpfile_left; /* the tmpfile left by an external edit. UTF-8! */
	apr_pool_t *pool; /* a pool. */
};

svn_error_t* svn_get_log_message(const char **log_msg,
								 const char **tmp_file,
								 const apr_array_header_t * /*commit_items*/,
								 void *baton, 
								 apr_pool_t * pool)
{
	log_msg_baton3 *lmb = (log_msg_baton3 *) baton;
	*tmp_file = NULL;
	if (lmb->message)
	{
		*log_msg = apr_pstrdup (pool, lmb->message);
	}

	return SVN_NO_ERROR;
}

svn_error_t*	SVNProperties::Refresh()
{
	svn_opt_revision_t			rev;
	svn_error_clear(m_error);
	m_error = NULL;

	if (m_propCount > 0)
	{
		m_propCount = 0;
		m_props.clear();
		// don't clear or even destroy the m_pool here, since
		// there are still subpools of that pool alive!
	}

#ifdef _MFC_VER
	rev.kind = ((const svn_opt_revision_t *)m_rev)->kind;
	rev.value = ((const svn_opt_revision_t *)m_rev)->value;
#else
	rev.kind = svn_opt_revision_unspecified;
	rev.value.number = -1;
#endif

    const char* svnPath = m_path.GetSVNApiPath(m_pool);
	if (m_bRevProps)
	{
		svn_revnum_t rev_set;
		apr_hash_t * props;

        SVNTRACE (
		    m_error = svn_client_revprop_list(	&props, 
											    svnPath,
											    &rev,
											    &rev_set,
											    m_pctx,
											    m_pool),
            svnPath
        )

		m_props[std::string("")] = apr_hash_copy(m_pool, props);
	}
	else
	{
        SVNTRACE (
		    m_error = svn_client_proplist3 (svnPath,
										    &rev,
										    &rev,
										    svn_depth_empty,
										    NULL,
										    proplist_receiver,
										    this,
										    m_pctx,
										    m_pool),
            svnPath
        )
	}
	if(m_error != NULL)
		return m_error;


	for (std::map<std::string, apr_hash_t *>::iterator it = m_props.begin(); it != m_props.end(); ++it)
	{
		m_propCount += apr_hash_count(it->second);
	}
	return NULL;
}

void SVNProperties::Construct()
{
	m_propCount = 0;

	m_pool = svn_pool_create (NULL);				// create the memory pool
	m_error = NULL;
	svn_error_clear(svn_client_create_context(&m_pctx, m_pool));

#ifdef _MFC_VER

	svn_error_clear(svn_config_ensure(NULL, m_pool));
	// set up the configuration
	m_error = svn_config_get_config (&m_pctx->config, g_pConfigDir, m_pool);
	if (m_error)
	{
		::MessageBox(NULL, this->GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_error_clear(m_error);
		svn_pool_destroy (m_pool);					// free the allocated memory
		return;
	}

	m_prompt.Init(m_pool, m_pctx);

	m_pctx->log_msg_func3 = svn_get_log_message;

	// set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}

#endif
}

#ifdef _MFC_VER
SVNProperties::SVNProperties(SVNRev rev, bool bRevProps)
	: m_rev(rev)
	, m_bRevProps (bRevProps)
#else
SVNProperties::SVNProperties(bool bRevProps)
	: m_bRevProps (bRevProps)
#endif
{
	Construct();
}

#ifdef _MFC_VER
SVNProperties::SVNProperties(const CTSVNPath& filepath, SVNRev rev, bool bRevProps)
	: m_path (filepath)
	, m_rev(rev)
#else
SVNProperties::SVNProperties(const CTSVNPath& filepath, bool bRevProps)
	: m_path (filepath)
#endif
	, m_bRevProps (bRevProps)
{
	Construct();
	SVNProperties::Refresh();
}

SVNProperties::~SVNProperties(void)
{
	svn_error_clear(m_error);
	svn_pool_destroy (m_pool);					// free the allocated memory
}

void SVNProperties::SetFilePath (const CTSVNPath& filePath)
{
	m_path = filePath;
	Refresh();
}

int SVNProperties::GetCount() const
{
	return m_propCount;
}

std::string SVNProperties::GetItem(int index, BOOL name) const
{
	const void *key;
	void *val;
	svn_string_t *propval = NULL;
	const char *pname_utf8 = "";

	if (m_props.size() == 0)
	{
		return "";
	}
	if (index >= m_propCount)
	{
		return "";
	}

	long ind = 0;

	for (std::map<std::string, apr_hash_t *>::const_iterator it = m_props.begin(); it != m_props.end(); ++it)
	{
		apr_hash_index_t *hi;

		for (hi = apr_hash_first(m_pool, it->second); hi; hi = apr_hash_next(hi))
		{
			if (ind++ != index)
				continue;

			apr_hash_this(hi, &key, NULL, &val);
			propval = (svn_string_t *)val;
			pname_utf8 = (char *)key;
			break;
		}
	}

	if (name)
		return pname_utf8;
	else if (propval)
		return std::string(propval->data, propval->len);
	else
		return "";
}

BOOL SVNProperties::IsSVNProperty(int index) const
{
	const char *pname_utf8;
	const char *name = SVNProperties::GetItem(index, true).c_str();

	svn_error_clear(svn_utf_cstring_to_utf8 (&pname_utf8, name, m_pool));
	svn_boolean_t is_svn_prop = svn_prop_needs_translation (pname_utf8);

	return is_svn_prop;
}

bool SVNProperties::IsBinary(int index) const
{
	std::string value = GetItem(index, false);
	return IsBinary(value);
}

bool SVNProperties::IsBinary(const std::string& value)
{
	const char * pvalue = (const char *)value.c_str();
	// check if there are any null bytes in the string
	for (size_t i=0; i<value.size(); ++i)
	{
		if (*pvalue == '\0')
		{
			// if there are only null bytes until the end of the string,
			// we still treat it as not binary
			while (i<value.size())
			{
				if (*pvalue != '\0')
					return true;
				++i;
				pvalue++;
			}
			return false;
		}
		pvalue++;
	}
	return false;
}

std::string SVNProperties::GetItemName(int index) const
{
	return SVNProperties::GetItem(index, true);
}

std::string SVNProperties::GetItemValue(int index) const
{
	return SVNProperties::GetItem(index, false);
}

BOOL SVNProperties::Add(const std::string& name, const std::string& Value, svn_depth_t depth, const TCHAR * message)
{
	svn_string_t*	pval;
	m_error = NULL;

	SVNPool subpool(m_pool);
	svn_error_clear(m_error);

	pval = svn_string_ncreate (Value.c_str(), Value.size(), subpool);

	if ((!m_bRevProps)&&((!m_path.IsDirectory() && !m_path.IsUrl())&&(((strncmp(name.c_str(), "bugtraq:", 8)==0)||(strncmp(name.c_str(), "tsvn:", 5)==0)||(strncmp(name.c_str(), "webviewer:", 10)==0)))))
	{
		// bugtraq:, tsvn: and webviewer: properties are not allowed on files.
#ifdef _MFC_VER
		CString temp;
		temp.LoadString(IDS_ERR_PROPNOTONFILE);
		CStringA tempA = CUnicodeUtils::GetUTF8(temp);
		m_error = svn_error_create(NULL, NULL, tempA);
#else
		TCHAR string[1024];
		LoadStringEx(g_hResInst, IDS_ERR_PROPNOTONFILE, string, 1024, (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
		std::string stringA = WideToUTF8(std::wstring(string));
		m_error = svn_error_create(NULL, NULL, stringA.c_str());
#endif
		return FALSE;
	}
	if (strncmp(name.c_str(), "bugtraq:message", 15)==0)
	{
		// make sure that the bugtraq:message property only contains one single line!
		for (apr_size_t i=0; i<pval->len; ++i)
		{
			if ((pval->data[i] == '\n')||(pval->data[0] == '\r'))
			{
#ifdef _MFC_VER
				CString temp;
				temp.LoadString(IDS_ERR_PROPNOMULTILINE);
				CStringA tempA = CUnicodeUtils::GetUTF8(temp);
				m_error = svn_error_create(NULL, NULL, tempA);
#else
				TCHAR string[1024];
				LoadStringEx(g_hResInst, IDS_ERR_PROPNOMULTILINE, string, 1024, (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				std::string stringA = WideToUTF8(std::wstring(string));
				m_error = svn_error_create(NULL, NULL, stringA.c_str());
#endif
				return FALSE;
			}
		}
	}

	if ((!m_bRevProps)&&((depth != svn_depth_empty)&&((strncmp(name.c_str(), "bugtraq:", 8)==0)||(strncmp(name.c_str(), "tsvn:", 5)==0)||(strncmp(name.c_str(), "webviewer:", 10)==0))))
	{
		// The bugtraq and tsvn properties must only be set on folders.
		CTSVNPath path;
		SVNStatus stat;
		svn_wc_status2_t * status = NULL;
		status = stat.GetFirstFileStatus(m_path, path, false, svn_depth_infinity, true, true);
		do 
		{
			if (status)
			{
				if ((status->entry)&&(status->entry->kind == svn_node_dir))
				{
					// a versioned folder, so set the property!
					svn_commit_info_t *commit_info = svn_create_commit_info(subpool);
                    const char* svnPath = path.GetSVNApiPath(subpool);
                    SVNTRACE (
					    m_error = svn_client_propset3 (&commit_info, name.c_str(), pval, svnPath, svn_depth_empty, false, m_rev, NULL, NULL, m_pctx, subpool),
                        svnPath
                    )
				}
			}
			status = stat.GetNextFileStatus(path);
		} while ((status != 0)&&(m_error == NULL));
	}
	else 
	{
		svn_commit_info_t *commit_info = svn_create_commit_info(subpool);
        const char* svnPath = m_path.GetSVNApiPath(subpool);
		if (m_path.IsUrl())
		{
			CString msg = message ? message : _T("");
			msg.Replace(_T("\r"), _T(""));
			log_msg_baton3* baton = (log_msg_baton3 *) apr_palloc (subpool, sizeof (*baton));
			baton->message = apr_pstrdup(subpool, CUnicodeUtils::GetUTF8(msg));
			baton->base_dir = "";
			baton->message_encoding = NULL;
			baton->tmpfile_left = NULL;
			baton->pool = subpool;
			m_pctx->log_msg_baton3 = baton;
		}
		if (m_bRevProps)
		{
			svn_revnum_t rev_set;
            SVNTRACE (
			    m_error = svn_client_revprop_set2(name.c_str(), pval, NULL, svnPath, m_rev, &rev_set, false, m_pctx, subpool),
                svnPath
            )
		}
		else
		{
            SVNTRACE (
    			m_error = svn_client_propset3 (&commit_info, name.c_str(), pval, svnPath, depth, false, m_rev, NULL, NULL, m_pctx, subpool),
                svnPath
            )
		}
	}
	if (m_error != NULL)
	{
		return FALSE;
	}

	// rebuild the property list
	m_error = SVNProperties::Refresh();
	if (m_error != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL SVNProperties::Remove(const std::string& name, svn_depth_t depth, const TCHAR * message)
{
	m_error = NULL;

	SVNPool subpool(m_pool);
	svn_error_clear(m_error);

	svn_commit_info_t *commit_info = svn_create_commit_info(subpool);
	if (m_path.IsUrl())
	{
		CString msg = message ? message : _T("");
		msg.Replace(_T("\r"), _T(""));
		log_msg_baton3* baton = (log_msg_baton3 *) apr_palloc (subpool, sizeof (*baton));
		baton->message = apr_pstrdup(subpool, CUnicodeUtils::GetUTF8(msg));
		baton->base_dir = "";
		baton->message_encoding = NULL;
		baton->tmpfile_left = NULL;
		baton->pool = subpool;
		m_pctx->log_msg_baton3 = baton;
	}

    const char* svnPath = m_path.GetSVNApiPath(subpool);
	if (m_bRevProps)
	{
		svn_revnum_t rev_set;
        SVNTRACE (
		    m_error = svn_client_revprop_set2(name.c_str(), NULL, NULL, svnPath, m_rev, &rev_set, false, m_pctx, subpool),
            svnPath
        )
	}
	else
	{
        SVNTRACE (
    		m_error = svn_client_propset3 (&commit_info, name.c_str(), NULL, svnPath, depth, false, m_rev, NULL, NULL, m_pctx, subpool),
            svnPath
        )
	}

	if (m_error != NULL)
	{
		return FALSE;
	}

	// rebuild the property list
	m_error = Refresh();
	if (m_error != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

tstring SVNProperties::GetLastErrorMsg() const
{
	tstring msg;
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
				if (ErrPtr->message)
					msg += UTF8ToString(ErrPtr->message);
				else
				{
					/* Is this a Subversion-specific error code? */
					if ((ErrPtr->apr_err > APR_OS_START_USEERR)
						&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
						msg += UTF8ToString(svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
					/* Otherwise, this must be an APR error code. */
					else
					{
						svn_error_t *temp_err = NULL;
						const char * err_string = NULL;
						temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
						if (temp_err)
						{
							svn_error_clear (temp_err);
							msg += _T("Can't recode error string from APR");
						}
						else
						{
							msg += UTF8ToString(err_string);
						}
					}
				}
			}
		} 
		return msg;
	} 
	return msg;
}

int SVNProperties::IndexOf (const std::string& name) const
{
	if (m_propCount == 0)
		return -1;

	long index = 0;
	for (std::map<std::string, apr_hash_t *>::const_iterator it = m_props.begin(); it != m_props.end(); ++it)
	{
		apr_hash_index_t *hi;

		for (hi = apr_hash_first(m_pool, it->second); hi; hi = apr_hash_next(hi))
		{
			const void *key = NULL;
			void *val = NULL;

			apr_hash_this(hi, &key, NULL, &val);
			const char* pname_utf8 = (char *)key;
			if (strcmp (pname_utf8, name.c_str()) == 0)
				return index;

			++index;
		}
	}

	return -1;
}

bool SVNProperties::HasProperty (const std::string& name) const
{
	return IndexOf (name) != -1;
}

namespace
{

	void AddKeyValue 
		( const char* key
		, size_t keyLength
		, const char* value
		, size_t valueLength
		, std::string& target)
	{
		CStringA keyHeader;
		keyHeader.Format ("K %d\n", static_cast<int>(keyLength));
		target.append ((const char*)keyHeader, keyHeader.GetLength());

		target.append (key, keyLength);
		target.append ('\n', 1);

		CStringA valueHeader;
		valueHeader.Format ("V %d\n", static_cast<int>(valueLength));
		target.append ((const char*)valueHeader, valueHeader.GetLength());

		target.append (value, valueLength);
		target.append ('\n', 1);
	}

	void AddKeyValue 
		( const std::string& key
		, const std::string& value
		, std::string& target)
	{
		AddKeyValue ( key.c_str()
					, key.length()
					, value.c_str()
					, value.length()
					, target);
	}

	std::pair<std::string, std::string> GetKeyValue 
		( const std::string& source
		, size_t& offset)
	{
		// read "K %d\n" line

		int keyLength = atoi (source.c_str() + offset + 2);
		for (offset += 4; source[offset-1] != '\n'; ++offset)
		{
		}

		std::string key (source.c_str() + offset, static_cast<size_t>(keyLength));
		offset += keyLength+1;

		// read "V %d\n" line

		int valueLength = atoi (source.c_str() + offset + 2);
		for (offset += 4; source[offset-1] != '\n'; ++offset)
		{
		}

		std::string value (source.c_str() + offset, static_cast<size_t>(valueLength));
		offset += valueLength+1;

		// terminating \n

		return std::pair<std::string, std::string>(key, value);
	}

}

std::string SVNProperties::GetSerializedForm() const
{
	std::string result;
	result.reserve (m_propCount * 100);

	std::string properties;
	result.reserve (m_propCount * 100);

	for (std::map<std::string, apr_hash_t *>::const_iterator it = m_props.begin(); it != m_props.end(); ++it)
	{
		properties.clear();
		for (apr_hash_index_t *hi = apr_hash_first(m_pool, it->second); hi; hi = apr_hash_next(hi))
		{
			const void *key = NULL;
			void *val = NULL;
			apr_ssize_t keyLength = 0;

			apr_hash_this(hi, &key, &keyLength, &val);

			const char* pname_utf8 = static_cast<const char *>(key);
			svn_string_t* propval = static_cast<svn_string_t *>(val);

			AddKeyValue (pname_utf8, keyLength, propval->data, propval->len, properties);
		}

		AddKeyValue ("name", it->first, result);
		AddKeyValue ("properties", properties, result);

	}

	return result;
}

void SVNProperties::SetFromSerializedForm (const std::string& text)
{
	// clear properties list

	for (std::map<std::string, apr_hash_t *>::iterator it = m_props.begin(); it != m_props.end(); ++it)
		apr_hash_clear (it->second);

	m_props.clear();

	// read all file contents

	for (size_t offset = 0; offset < text.length(); ) 
	{
		// create hash for this file

		std::pair<std::string, std::string> props = GetKeyValue (text, offset);

		apr_hash_t*& hash = m_props[props.first];
		hash = apr_hash_make (m_pool);

		// read all prop entries into hash

		for (size_t propOffset = 0; propOffset < props.second.length(); ) 
		{
			std::pair<std::string, std::string> prop 
				= GetKeyValue (props.second, propOffset);

			svn_string_t* value = svn_string_ncreate ( prop.second.c_str()
													 , prop.second.length()
													 , m_pool);
			apr_hash_set (hash, prop.first.c_str(), prop.first.length(), value);
		}
	}
}

svn_error_t * SVNProperties::proplist_receiver(void *baton, const char *path, apr_hash_t *prop_hash, apr_pool_t *pool)
{
	SVNProperties * pThis = (SVNProperties*)baton;
	if (pThis)
	{
		svn_error_t * error;
		const char *node_name_native;
		error = svn_utf_cstring_from_utf8 (&node_name_native, path, pool);
		pThis->m_props[std::string(node_name_native)] = apr_hash_copy(pThis->m_pool, prop_hash);
		return error;
	}
	return SVN_NO_ERROR;
}
