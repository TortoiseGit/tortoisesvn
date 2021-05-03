// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2021 - TortoiseSVN

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
#include "../TortoiseShell/resource.h"
#include "SVNReadProperties.h"
#include "SVNStatus.h"
#include "SVNConfig.h"
#include "SVNTrace.h"
#pragma warning(push)
#include "svn_props.h"
#pragma warning(pop)

#ifdef _MFC_VER
#    include "SVN.h"
#    include "UnicodeUtils.h"
#    include "Hooks.h"
#else
#    include "registry.h"
extern HINSTANCE g_hResInst;
#endif

struct LogMsgBaton3
{
    const char *message;         /* the message. */
    const char *messageEncoding; /* the locale/encoding of the message. */
    const char *baseDir;         /* the base directory for an external edit. UTF-8! */
    const char *tmpfileLeft;     /* the tmpfile left by an external edit. UTF-8! */
    apr_pool_t *pool;            /* a pool. */
};

svn_error_t *svnGetLogMessage(const char **logMsg,
                              const char **tmpFile,
                              const apr_array_header_t * /*commit_items*/,
                              void *      baton,
                              apr_pool_t *pool)
{
    LogMsgBaton3 *lmb = static_cast<LogMsgBaton3 *>(baton);
    *tmpFile          = nullptr;
    if (lmb->message)
    {
        *logMsg = apr_pstrdup(pool, lmb->message);
    }

    return nullptr;
}

svn_error_t *cancelFunc(void *cancelbaton)
{
    SVNReadProperties *pReadProps = static_cast<SVNReadProperties *>(cancelbaton);
    if ((pReadProps) && (pReadProps->m_bCancelled) && (*pReadProps->m_bCancelled))
    {
#ifdef IDS_SVN_USERCANCELLED
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(temp));
#else
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, "");
#endif
    }
    return nullptr;
}

svn_error_t *SVNReadProperties::Refresh()
{
    svn_opt_revision_t rev{};
    svn_opt_revision_t pegRev{};
    svn_error_clear(m_err);
    m_err = nullptr;
    if (m_bCancelled)
        (*m_bCancelled) = false;
    if (m_propCount > 0)
    {
        m_propCount = 0;
        // don't clear or even destroy the m_pool here, since
        // there are still subpools of that pool alive!
    }

#ifdef _MFC_VER
    rev.kind     = static_cast<const svn_opt_revision_t *>(m_rev)->kind;
    rev.value    = static_cast<const svn_opt_revision_t *>(m_rev)->value;
    pegRev.kind  = static_cast<const svn_opt_revision_t *>(m_pegRev)->kind;
    pegRev.value = static_cast<const svn_opt_revision_t *>(m_pegRev)->value;

    if (m_path.IsUrl() || (!m_rev.IsWorking() && !m_rev.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(m_path));
#else
    rev.kind            = svn_opt_revision_unspecified;
    rev.value.number    = -1;
    pegRev.kind         = svn_opt_revision_unspecified;
    pegRev.value.number = -1;
#endif

    const char *svnPath = m_path.GetSVNApiPath(m_pool);
    if ((svnPath == nullptr) || (svnPath[0] == 0))
        return nullptr;
    m_props = nullptr;
    m_inheritedProperties.clear();
    if (m_bRevProps)
    {
        svn_revnum_t revSet{};
        apr_hash_t * props = nullptr;

        SVNTRACE(
            m_err = svn_client_revprop_list(&props,
                                            svnPath,
                                            &rev,
                                            &revSet,
                                            m_pCtx,
                                            m_pool),
            svnPath)
        ClearCAPIAuthCacheOnError();
        if (m_err == nullptr)
            m_props = apr_hash_copy(m_pool, props);
    }
    else
    {
        SVNTRACE(
            m_err = svn_client_proplist4(svnPath,
                                         &pegRev,
                                         &rev,
                                         svn_depth_empty,
                                         NULL,
                                         m_includeInherited,
                                         proplist_receiver,
                                         this,
                                         m_pCtx,
                                         m_pool),
            svnPath)
    }
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
        return m_err;

    if (m_props)
        m_propCount = apr_hash_count(m_props);

    return nullptr;
}

void SVNReadProperties::Construct()
{
    m_propCount = 0;

    m_pool = svn_pool_create(NULL); // create the memory pool
    m_err  = nullptr;
    svn_error_clear(svn_client_create_context2(&m_pCtx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

#ifdef _MFC_VER

    if (m_pCtx->config == nullptr)
    {
        svn_pool_destroy(m_pool); // free the allocated memory
        return;
    }

    m_prompt.Init(m_pool, m_pCtx);

    m_pCtx->log_msg_func3 = svnGetLogMessage;

#endif
    m_pCtx->cancel_func  = cancelFunc;
    m_pCtx->cancel_baton = this;
    if (m_bCancelled)
        (*m_bCancelled) = false;
}

#ifdef _MFC_VER
SVNReadProperties::SVNReadProperties(SVNRev rev, bool bRevProps, bool includeInherited)
    // ReSharper disable once CppMemberInitializersOrder
    : SVNBase()
    , m_rev(rev)
    , m_pegRev(rev)
    , m_bRevProps(bRevProps)
    , m_includeInherited(includeInherited)
    , m_pProgress(nullptr)
#else
SVNReadProperties::SVNReadProperties(bool bRevProps, bool includeInherited)
    : SVNBase()
    , m_bRevProps(bRevProps)
    , m_includeInherited(includeInherited)
#endif
    , m_propCount(0)
    , m_props(nullptr)
    , m_bCancelled(nullptr)
{
    Construct();
}

#ifdef _MFC_VER
SVNReadProperties::SVNReadProperties(const CTSVNPath &filepath, SVNRev rev, bool bRevProps, bool includeInherited)
    // ReSharper disable once CppMemberInitializersOrder
    : SVNBase()
    , m_path(filepath)
    , m_rev(rev)
    , m_pegRev(rev)
    , m_includeInherited(includeInherited)
    , m_pProgress(nullptr)
#else
SVNReadProperties::SVNReadProperties(const CTSVNPath &filepath, bool bRevProps, bool includeInherited)
    : SVNBase()
    , m_path(filepath)
    , m_includeInherited(includeInherited)
#endif
    , m_bRevProps(bRevProps)
    , m_propCount(0)
    , m_props(nullptr)
    , m_bCancelled(nullptr)
{
    Construct();
    Refresh();
}

#ifdef _MFC_VER
SVNReadProperties::SVNReadProperties(const CTSVNPath &filepath, SVNRev pegRev, SVNRev rev, bool suppressUI, bool includeInherited)
    : SVNBase()
    , m_bCancelled(nullptr)
    , m_path(filepath)
    , m_props(nullptr)
    , m_propCount(0)
    , m_rev(rev)
    , m_bRevProps(false)
    , m_includeInherited(includeInherited)
    , m_prompt(suppressUI)
    , m_pProgress(nullptr)
    , m_pegRev(pegRev)
{
    Construct();
    Refresh();
}
#endif

SVNReadProperties::~SVNReadProperties()
{
    svn_error_clear(m_err);
    svn_pool_destroy(m_pool); // free the allocated memory
}

void SVNReadProperties::SetFilePath(const CTSVNPath &filePath)
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
    const void *  key;
    void *        val;
    svn_string_t *propVal   = nullptr;
    const char *  pnameUtf8 = "";

    if (m_propCount == 0)
    {
        return "";
    }
    if (index >= m_propCount)
    {
        return "";
    }

    long ind = 0;

    apr_hash_index_t *hi = nullptr;

    for (hi = apr_hash_first(m_pool, m_props); hi; hi = apr_hash_next(hi))
    {
        if (ind++ != index)
            continue;

        apr_hash_this(hi, &key, nullptr, &val);
        propVal   = static_cast<svn_string_t *>(val);
        pnameUtf8 = static_cast<const char *>(key);
        break;
    }

    if (name)
        return pnameUtf8;
    else if (propVal)
        return std::string(propVal->data, propVal->len);
    else
        return "";
}

BOOL SVNReadProperties::IsSVNProperty(int index) const
{
    const char *pnameUtf8 = nullptr;
    std::string name      = SVNReadProperties::GetItem(index, true);

    svn_error_clear(svn_utf_cstring_to_utf8(&pnameUtf8, name.c_str(), m_pool));
    svn_boolean_t isSvnProp = svn_prop_needs_translation(pnameUtf8);

    return isSvnProp;
}

bool SVNReadProperties::IsBinary(int index) const
{
    std::string value = GetItem(index, false);
    return IsBinary(value);
}

bool SVNReadProperties::IsBinary(const std::string &value)
{
    const char *pValue = static_cast<const char *>(value.c_str());
    // check if there are any null bytes in the string
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (*pValue == '\0')
        {
            // if there are only null bytes until the end of the string,
            // we still treat it as not binary
            while (i < value.size())
            {
                if (*pValue != '\0')
                    return true;
                ++i;
                pValue++;
            }
            return false;
        }
        pValue++;
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

int SVNReadProperties::IndexOf(const std::string &name) const
{
    if (m_propCount == 0)
        return -1;

    long              index = 0;
    apr_hash_index_t *hi    = nullptr;

    for (hi = apr_hash_first(m_pool, m_props); hi; hi = apr_hash_next(hi))
    {
        const void *key = nullptr;
        void *      val = nullptr;

        apr_hash_this(hi, &key, nullptr, &val);
        const char *pnameUtf8 = static_cast<const char *>(key);
        if (strcmp(pnameUtf8, name.c_str()) == 0)
            return index;

        ++index;
    }

    return -1;
}

bool SVNReadProperties::HasProperty(const std::string &name) const
{
    return IndexOf(name) != -1;
}

namespace
{
void AddKeyValue(const char *key, size_t keyLength, const char *value, size_t valueLength, std::string &target)
{
    CStringA keyHeader;
    keyHeader.Format("K %d\n", static_cast<int>(keyLength));
    target.append(static_cast<const char *>(keyHeader), keyHeader.GetLength());

    target.append(key, keyLength);
    target.append('\n', 1);

    CStringA valueHeader;
    valueHeader.Format("V %d\n", static_cast<int>(valueLength));
    target.append(static_cast<const char *>(valueHeader), valueHeader.GetLength());

    target.append(value, valueLength);
    target.append('\n', 1);
}

void AddKeyValue(const std::string &key, const std::string &value, std::string &target)
{
    AddKeyValue(key.c_str(), key.length(), value.c_str(), value.length(), target);
}

} // namespace

std::string SVNReadProperties::GetSerializedForm() const
{
    std::string result;
    result.reserve(m_propCount * 100);

    std::string properties;
    result.reserve(m_propCount * 100);

    properties.clear();
    for (apr_hash_index_t *hi = apr_hash_first(m_pool, m_props); hi; hi = apr_hash_next(hi))
    {
        const void *key       = nullptr;
        void *      val       = nullptr;
        apr_ssize_t keyLength = 0;

        apr_hash_this(hi, &key, &keyLength, &val);

        const char *  pnameUtf8 = static_cast<const char *>(key);
        svn_string_t *propVal   = static_cast<svn_string_t *>(val);

        AddKeyValue(pnameUtf8, keyLength, propVal->data, propVal->len, properties);
    }

    AddKeyValue("name", m_path.GetSVNApiPath(m_pool), result);
    AddKeyValue("properties", properties, result);

    return result;
}

svn_error_t *SVNReadProperties::proplist_receiver(void *baton, const char *path, apr_hash_t *propHash, apr_array_header_t *inheritedProps, apr_pool_t *pool)
{
    SVNReadProperties *pThis = static_cast<SVNReadProperties *>(baton);
    if (pThis)
    {
        svn_error_t *error          = nullptr;
        const char * nodeNameNative = nullptr;
        error                       = svn_utf_cstring_from_utf8(&nodeNameNative, path, pool);
        if (error == nullptr)
        {
            if (inheritedProps)
            {
                // properties further down the tree override the properties from above
                for (int i = 0; i < inheritedProps->nelts; i++)
                {
                    svn_prop_inherited_item_t *iitem = (APR_ARRAY_IDX(inheritedProps, i, svn_prop_inherited_item_t *));
                    if (pThis->m_props)
                    {
                        pThis->m_props = apr_hash_overlay(pThis->m_pool, iitem->prop_hash, pThis->m_props);
                    }
                    else
                        pThis->m_props = apr_hash_copy(pThis->m_pool, iitem->prop_hash);

                    // just in case someone needs the properties from above even though the same
                    // property is set further below, we store all props in a custom array.
                    std::multimap<std::string, std::string> propmap;
                    for (apr_hash_index_t *hi = apr_hash_first(pool, iitem->prop_hash); hi; hi = apr_hash_next(hi))
                    {
                        const void *key;
                        void *      val;
                        apr_hash_this(hi, &key, nullptr, &val);
                        svn_string_t *propVal   = static_cast<svn_string_t *>(val);
                        const char *  pnameUtf8 = static_cast<const char *>(key);
                        propmap.emplace(pnameUtf8, std::string(propVal->data, propVal->len));
                    }
                    pThis->m_inheritedProperties.emplace_back(iitem->path_or_url, propmap);
                }
            }
            if (pThis->m_props && propHash)
                pThis->m_props = apr_hash_overlay(pThis->m_pool, propHash, pThis->m_props);
            else if (propHash)
                pThis->m_props = apr_hash_copy(pThis->m_pool, propHash);
        }

        return error;
    }
    return nullptr;
}

bool SVNReadProperties::IsFolderOnlyProperty(const std::string &name) const
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

    if (!m_folderProps.empty())
    {
        for (auto it = m_folderProps.cbegin(); it != m_folderProps.cend(); ++it)
        {
            if (it->compare(name) == 0)
                return true;
        }
    }

    return false;
}
