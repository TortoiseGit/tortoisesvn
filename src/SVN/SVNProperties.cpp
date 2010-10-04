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

#ifdef _MFC_VER
SVNProperties::SVNProperties(SVNRev rev, bool bRevProps)
    : SVNReadProperties(rev, bRevProps)
    , m_error(NULL)
#else
SVNProperties::SVNProperties(bool bRevProps)
    : SVNReadProperties (bRevProps)
    , m_error(NULL)
#endif
{
}

#ifdef _MFC_VER
SVNProperties::SVNProperties(const CTSVNPath& filepath, SVNRev rev, bool bRevProps)
    : SVNReadProperties (filepath, rev, bRevProps)
    , m_error(NULL)
#else
SVNProperties::SVNProperties(const CTSVNPath& filepath, bool bRevProps)
    : SVNReadProperties (filepath, bRevProps)
    , m_error(NULL)
#endif
{
}

SVNProperties::~SVNProperties(void)
{
}

BOOL SVNProperties::Add(const std::string& name, const std::string& Value, bool force, svn_depth_t depth, const TCHAR * message)
{
    svn_string_t*   pval;
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
#ifdef _MFC_VER
    if (m_path.IsUrl() || (!m_rev.IsWorking() && !m_rev.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(m_path));
#endif
    if ((!m_bRevProps)&&((depth != svn_depth_empty)&&IsFolderOnlyProperty(name)))
    {
        // The bugtraq and tsvn properties must only be set on folders.
        CTSVNPath path;
        SVNStatus stat;
        svn_client_status_t * status = NULL;
        status = stat.GetFirstFileStatus(m_path, path, false, depth, true, true);
        do
        {
            if ((status)&&(status->kind == svn_node_dir))
            {
#ifdef _MFC_VER
                if (m_pProgress)
                    m_pProgress->SetLine(2, path.GetWinPath(), true);
#endif
                // a versioned folder, so set the property!
                SVNPool setPool((apr_pool_t*)subpool);
                const char* svnPath = path.GetSVNApiPath(setPool);
                SVNTRACE (
                    m_error = svn_client_propset4 (name.c_str(), pval, svnPath, svn_depth_empty, false, m_rev, NULL, NULL, NULL, NULL, m_pctx, setPool),
                    svnPath
                    )
            }
            status = stat.GetNextFileStatus(path);
#ifdef _MFC_VER
            if ((m_pProgress)&&(m_pProgress->HasUserCancelled()))
                m_error = svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
#endif
        } while ((status != 0)&&(m_error == NULL));
    }
    else
    {
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
                m_error = svn_client_revprop_set2(name.c_str(), pval, NULL, svnPath, m_rev, &rev_set, force, m_pctx, subpool),
                svnPath
            )
        }
        else
        {
            SVNTRACE (
                m_error = svn_client_propset4 (name.c_str(), pval, svnPath, depth, force, m_rev, NULL, NULL, NULL, NULL, m_pctx, subpool),
                svnPath
            )
        }
    }
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
#ifdef _MFC_VER
    if (m_path.IsUrl() || (!m_rev.IsWorking() && !m_rev.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(m_path));
#endif
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
        if (((depth != svn_depth_empty)&&IsFolderOnlyProperty(name)))
        {
            CTSVNPath path;
            SVNStatus stat;
            svn_client_status_t * status = NULL;
            status = stat.GetFirstFileStatus(m_path, path, false, depth, true, true);
            do
            {
                if ((status)&&(status->kind == svn_node_dir))
                {
#ifdef _MFC_VER
                    if (m_pProgress)
                        m_pProgress->SetLine(2, path.GetWinPath(), true);
#endif
                    SVNPool setPool((apr_pool_t*)subpool);
                    const char* svnPath = path.GetSVNApiPath(setPool);
                    SVNTRACE (
                        m_error = svn_client_propset4 (name.c_str(), NULL, svnPath, svn_depth_empty, false, m_rev, NULL, NULL, NULL, NULL, m_pctx, setPool),
                        svnPath
                        )
                }
                status = stat.GetNextFileStatus(path);
#ifdef _MFC_VER
                if ((m_pProgress)&&(m_pProgress->HasUserCancelled()))
                    m_error = svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
#endif
            } while ((status != 0)&&(m_error == NULL));
        }
        else
        {
            SVNTRACE (
                m_error = svn_client_propset4 (name.c_str(), NULL, svnPath, depth, false, m_rev, NULL, NULL, NULL, NULL, m_pctx, subpool),
                svnPath
                )
        }
    }

    if (m_error != NULL)
    {
        return FALSE;
    }

    return TRUE;
}

namespace
{

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

