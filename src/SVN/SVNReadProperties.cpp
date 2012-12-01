// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN

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
#include "SVNReadProperties.h"
#include "SVNStatus.h"
#include "SVNConfig.h"
#include "SVNHelpers.h"
#include "SVNTrace.h"
#pragma warning(push)
#include "svn_props.h"
#pragma warning(pop)

#ifdef _MFC_VER
#   include "SVN.h"
#   include "UnicodeUtils.h"
#   include "registry.h"
#   include "PathUtils.h"
#   include "Hooks.h"
#else
#include "registry.h"
extern  HINSTANCE           g_hResInst;
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

svn_error_t*    SVNReadProperties::Refresh()
{
    svn_opt_revision_t          rev;
    svn_opt_revision_t          peg_rev;
    svn_error_clear(Err);
    Err = NULL;

    if (m_propCount > 0)
    {
        m_propCount = 0;
        // don't clear or even destroy the m_pool here, since
        // there are still subpools of that pool alive!
    }

#ifdef _MFC_VER
    rev.kind = ((const svn_opt_revision_t *)m_rev)->kind;
    rev.value = ((const svn_opt_revision_t *)m_rev)->value;
    peg_rev.kind = ((const svn_opt_revision_t *)m_peg_rev)->kind;
    peg_rev.value = ((const svn_opt_revision_t *)m_peg_rev)->value;

    if (m_path.IsUrl() || (!m_rev.IsWorking() && !m_rev.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(m_path));
#else
    rev.kind = svn_opt_revision_unspecified;
    rev.value.number = -1;
    peg_rev.kind = svn_opt_revision_unspecified;
    peg_rev.value.number = -1;
#endif

    const char* svnPath = m_path.GetSVNApiPath(m_pool);
    if ((svnPath == 0)||(svnPath[0] == 0))
        return NULL;
    m_props = NULL;
    if (m_bRevProps)
    {
        svn_revnum_t rev_set;
        apr_hash_t * props;

        SVNTRACE (
            Err = svn_client_revprop_list(  &props,
                                                svnPath,
                                                &rev,
                                                &rev_set,
                                                m_pctx,
                                                m_pool),
            svnPath
        )
        if (Err == NULL)
            m_props = apr_hash_copy(m_pool, props);
    }
    else
    {
        SVNTRACE (
            Err = svn_client_proplist4 (svnPath,
                                        &peg_rev,
                                        &rev,
                                        svn_depth_empty,
                                        NULL,
                                        m_includeInherited,
                                        proplist_receiver,
                                        this,
                                        m_pctx,
                                        m_pool,
                                        m_pool),
            svnPath
        )
    }
    if(Err != NULL)
        return Err;

    if (m_props)
        m_propCount = apr_hash_count(m_props);

    return NULL;
}

void SVNReadProperties::Construct()
{
    m_propCount = 0;

    m_pool = svn_pool_create (NULL);                // create the memory pool
    Err = NULL;
    svn_error_clear(svn_client_create_context2(&m_pctx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

#ifdef _MFC_VER

    if (m_pctx->config == nullptr)
    {
        svn_pool_destroy (m_pool);                  // free the allocated memory
        return;
    }

    m_prompt.Init(m_pool, m_pctx);

    m_pctx->log_msg_func3 = svn_get_log_message;

#endif
}

#ifdef _MFC_VER
SVNReadProperties::SVNReadProperties(SVNRev rev, bool bRevProps, bool includeInherited)
    : SVNBase()
    , m_rev(rev)
    , m_peg_rev(rev)
    , m_bRevProps (bRevProps)
    , m_includeInherited(includeInherited)
    , m_pProgress(NULL)
#else
SVNReadProperties::SVNReadProperties(bool bRevProps, bool includeInherited)
    : SVNBase()
    , m_bRevProps (bRevProps)
    , m_includeInherited(includeInherited)
#endif
    , m_propCount(0)
    , m_props(NULL)
{
    Construct();
}

#ifdef _MFC_VER
SVNReadProperties::SVNReadProperties(const CTSVNPath& filepath, SVNRev rev, bool bRevProps, bool includeInherited)
    : SVNBase()
    , m_path (filepath)
    , m_rev(rev)
    , m_peg_rev(rev)
    , m_includeInherited(includeInherited)
    , m_pProgress(NULL)
#else
SVNReadProperties::SVNReadProperties(const CTSVNPath& filepath, bool bRevProps, bool includeInherited)
    : SVNBase()
    , m_path (filepath)
#endif
    , m_bRevProps (bRevProps)
    , m_propCount(0)
    , m_props(NULL)
{
    Construct();
    Refresh();
}

#ifdef _MFC_VER
SVNReadProperties::SVNReadProperties(const CTSVNPath& filepath, SVNRev pegRev, SVNRev rev, bool suppressUI, bool includeInherited)
    : SVNBase()
    , m_path (filepath)
    , m_peg_rev (pegRev)
    , m_rev (rev)
    , m_bRevProps (false)
    , m_includeInherited(includeInherited)
    , m_pProgress(NULL)
    , m_prompt (suppressUI)
    , m_propCount(0)
    , m_props(NULL)
{
    Construct();
    Refresh();
}
#endif

SVNReadProperties::~SVNReadProperties(void)
{
    svn_error_clear(Err);
    svn_pool_destroy (m_pool);                  // free the allocated memory
}

void SVNReadProperties::SetFilePath (const CTSVNPath& filePath)
{
    m_path = filePath;
    Refresh();
}

int SVNReadProperties::GetCount() const
{
    return m_propCount;
}

std::string SVNReadProperties::GetItem(int index, BOOL name) const
{
    const void *key;
    void *val;
    svn_string_t *propval = NULL;
    const char *pname_utf8 = "";

    if (m_propCount == 0)
    {
        return "";
    }
    if (index >= m_propCount)
    {
        return "";
    }

    long ind = 0;

    apr_hash_index_t *hi;

    for (hi = apr_hash_first(m_pool, m_props); hi; hi = apr_hash_next(hi))
    {
        if (ind++ != index)
            continue;

        apr_hash_this(hi, &key, NULL, &val);
        propval = (svn_string_t *)val;
        pname_utf8 = (char *)key;
        break;
    }

    if (name)
        return pname_utf8;
    else if (propval)
        return std::string(propval->data, propval->len);
    else
        return "";
}

BOOL SVNReadProperties::IsSVNProperty(int index) const
{
    const char *pname_utf8;
    std::string name = SVNReadProperties::GetItem(index, true);

    svn_error_clear(svn_utf_cstring_to_utf8 (&pname_utf8, name.c_str(), m_pool));
    svn_boolean_t is_svn_prop = svn_prop_needs_translation (pname_utf8);

    return is_svn_prop;
}

bool SVNReadProperties::IsBinary(int index) const
{
    std::string value = GetItem(index, false);
    return IsBinary(value);
}

bool SVNReadProperties::IsBinary(const std::string& value)
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

std::string SVNReadProperties::GetItemName(int index) const
{
    return SVNReadProperties::GetItem(index, true);
}

std::string SVNReadProperties::GetItemValue(int index) const
{
    return SVNReadProperties::GetItem(index, false);
}

int SVNReadProperties::IndexOf (const std::string& name) const
{
    if (m_propCount == 0)
        return -1;

    long index = 0;
    apr_hash_index_t *hi;

    for (hi = apr_hash_first(m_pool, m_props); hi; hi = apr_hash_next(hi))
    {
        const void *key = NULL;
        void *val = NULL;

        apr_hash_this(hi, &key, NULL, &val);
        const char* pname_utf8 = (char *)key;
        if (strcmp (pname_utf8, name.c_str()) == 0)
            return index;

        ++index;
    }

    return -1;
}

bool SVNReadProperties::HasProperty (const std::string& name) const
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

}

std::string SVNReadProperties::GetSerializedForm() const
{
    std::string result;
    result.reserve (m_propCount * 100);

    std::string properties;
    result.reserve (m_propCount * 100);

    properties.clear();
    for (apr_hash_index_t *hi = apr_hash_first(m_pool, m_props); hi; hi = apr_hash_next(hi))
    {
        const void *key = NULL;
        void *val = NULL;
        apr_ssize_t keyLength = 0;

        apr_hash_this(hi, &key, &keyLength, &val);

        const char* pname_utf8 = static_cast<const char *>(key);
        svn_string_t* propval = static_cast<svn_string_t *>(val);

        AddKeyValue (pname_utf8, keyLength, propval->data, propval->len, properties);
    }

    AddKeyValue ("name", m_path.GetSVNApiPath(m_pool), result);
    AddKeyValue ("properties", properties, result);


    return result;
}

svn_error_t * SVNReadProperties::proplist_receiver(void *baton, const char *path, apr_hash_t *prop_hash, apr_array_header_t *inherited_props, apr_pool_t *pool)
{
    SVNReadProperties * pThis = (SVNReadProperties*)baton;
    if (pThis)
    {
        svn_error_t * error;
        const char *node_name_native;
        error = svn_utf_cstring_from_utf8 (&node_name_native, path, pool);
        if (error == SVN_NO_ERROR)
        {
            if (inherited_props)
            {
                // properties further down the tree override the properties from above
                pThis->m_inheritedprops = apr_array_make (pool, 1, sizeof(svn_prop_inherited_item_t*));
                pThis->m_inheritedProperties.clear();
                for (int i = 0; i < inherited_props->nelts; i++)
                {
                    svn_prop_inherited_item_t * iitem = (APR_ARRAY_IDX (inherited_props, i, svn_prop_inherited_item_t*));
                    if (pThis->m_props)
                    {
                        pThis->m_props = apr_hash_overlay(pThis->m_pool, iitem->prop_hash, pThis->m_props);
                    }
                    else
                        pThis->m_props = apr_hash_copy(pThis->m_pool, iitem->prop_hash);

                    // just in case someone needs the properties from above even though the same
                    // property is set further below, we store all props in a custom array.
                    std::map<std::string,std::string> propmap;
                    for (apr_hash_index_t * hi = apr_hash_first(pool, iitem->prop_hash); hi; hi = apr_hash_next(hi))
                    {
                        const void *key;
                        void *val;
                        apr_hash_this(hi, &key, NULL, &val);
                        svn_string_t * propval = (svn_string_t *)val;
                        const char * pname_utf8 = (char *)key;
                        propmap[pname_utf8] = std::string(propval->data, propval->len);
                    }
                    pThis->m_inheritedProperties.push_back(std::make_tuple(iitem->path_or_url, propmap));
                }
            }
            if (pThis->m_props)
                pThis->m_props = apr_hash_overlay(pThis->m_pool, prop_hash, pThis->m_props);
            else
                pThis->m_props = apr_hash_copy(pThis->m_pool, prop_hash);
        }

        return error;
    }
    return SVN_NO_ERROR;
}

bool SVNReadProperties::IsFolderOnlyProperty( const std::string& name ) const
{
    if ((strncmp(name.c_str(), "bugtraq:", 8) == 0))
        return true;
    if ((strncmp(name.c_str(), "tsvn:", 5) == 0))
        return true;
    if ((strncmp(name.c_str(), "webviewer:", 10) == 0))
        return true;
    if (name.compare("svn:externals") == 0)
        return true;
    if (name.compare("svn:ignore") == 0)
        return true;

    if (!m_folderprops.empty())
    {
        for (auto it = m_folderprops.cbegin(); it != m_folderprops.cend(); ++it)
        {
            if (it->compare(name)==0)
                return true;
        }
    }

    return false;
}
