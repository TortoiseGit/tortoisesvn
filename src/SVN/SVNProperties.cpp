// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014, 2016, 2021 - TortoiseSVN

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
#include "SVNProperties.h"
#include "SVNStatus.h"
#include "SVNHelpers.h"
#include "SVNTrace.h"

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
    const char* message;         /* the message. */
    const char* messageEncoding; /* the locale/encoding of the message. */
    const char* baseDir;         /* the base directory for an external edit. UTF-8! */
    const char* tmpfileLeft;     /* the tmpfile left by an external edit. UTF-8! */
    apr_pool_t* pool;            /* a pool. */
};

#ifdef _MFC_VER
SVNProperties::SVNProperties(SVNRev rev, bool bRevProps, bool bIncludeInherited)
    : SVNReadProperties(rev, bRevProps, bIncludeInherited)
#else
SVNProperties::SVNProperties(bool bRevProps, bool bIncludeInherited)
    : SVNReadProperties(bRevProps, bIncludeInherited)
#endif
    , m_revSet(0)
{
}

#ifdef _MFC_VER
SVNProperties::SVNProperties(const CTSVNPath& filepath, SVNRev rev, bool bRevProps, bool bIncludeInherited)
    : SVNReadProperties(filepath, rev, bRevProps, bIncludeInherited)
#else
SVNProperties::SVNProperties(const CTSVNPath& filepath, bool bRevProps, bool bIncludeInherited)
    : SVNReadProperties(filepath, bRevProps, bIncludeInherited)
#endif
    , m_revSet(0)
{
}

SVNProperties::~SVNProperties()
{
}

BOOL SVNProperties::Add(const std::string& name, const std::string& value, bool force, svn_depth_t depth, const wchar_t* message)
{
    svn_string_t* pVal = nullptr;

    SVNPool subPool(m_pool);
    svn_error_clear(m_err);
    m_err = nullptr;

    pVal = svn_string_ncreate(value.c_str(), value.size(), subPool);

    if ((!m_bRevProps) && ((!m_path.IsDirectory() && !m_path.IsUrl()) && (((strncmp(name.c_str(), "bugtraq:", 8) == 0) || (strncmp(name.c_str(), "tsvn:", 5) == 0) || (strncmp(name.c_str(), "webviewer:", 10) == 0)))))
    {
        // bugtraq:, tsvn: and webviewer: properties are not allowed on files.
#ifdef _MFC_VER
        CString temp;
        temp.LoadString(IDS_ERR_PROPNOTONFILE);
        CStringA tempA = CUnicodeUtils::GetUTF8(temp);
        m_err          = svn_error_create(NULL, nullptr, tempA);
#else
        wchar_t string[1024] = {0};
        LoadStringEx(g_hResInst, IDS_ERR_PROPNOTONFILE, string, _countof(string), static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))));
        std::string stringA = CUnicodeUtils::StdGetUTF8(string);
        m_err               = svn_error_create(NULL, nullptr, stringA.c_str());
#endif
        return FALSE;
    }
    if (strncmp(name.c_str(), "bugtraq:message", 15) == 0)
    {
        // make sure that the bugtraq:message property only contains one single line!
        for (apr_size_t i = 0; i < pVal->len; ++i)
        {
            if ((pVal->data[i] == '\n') || (pVal->data[0] == '\r'))
            {
#ifdef _MFC_VER
                CString temp;
                temp.LoadString(IDS_ERR_PROPNOMULTILINE);
                CStringA tempA = CUnicodeUtils::GetUTF8(temp);
                m_err          = svn_error_create(NULL, nullptr, tempA);
#else
                wchar_t string[1024] = {0};
                LoadStringEx(g_hResInst, IDS_ERR_PROPNOMULTILINE, string, 1024, static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))));
                std::string stringA = CUnicodeUtils::StdGetUTF8(string);
                m_err               = svn_error_create(NULL, nullptr, stringA.c_str());
#endif
                return FALSE;
            }
        }
    }
#ifdef _MFC_VER
    if (m_path.IsUrl() || (!m_rev.IsWorking() && !m_rev.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(m_path));
    PrepareMsgForUrl(message, subPool);
#else
    UNREFERENCED_PARAMETER(message);
#endif
    if ((!m_bRevProps) && (!m_path.IsUrl()) && ((depth != svn_depth_empty) && IsFolderOnlyProperty(name)))
    {
        // The bugtraq and tsvn properties must only be set on folders.
        CTSVNPath            path;
        SVNStatus            stat;
        svn_client_status_t* status = stat.GetFirstFileStatus(m_path, path, false, depth, true, true);
        if (status == nullptr)
        {
            m_err = svn_error_dup(const_cast<svn_error_t*>(stat.GetSVNError()));
            return FALSE;
        }
        do
        {
            m_revSet = SVN_INVALID_REVNUM;
            if ((status) && (status->kind == svn_node_dir) && (status->node_status != svn_wc_status_missing) && (status->node_status != svn_wc_status_deleted) && (status->node_status != svn_wc_status_unversioned))
            {
#ifdef _MFC_VER
                if (m_pProgress)
                    m_pProgress->SetLine(2, path.GetWinPath(), true);
#endif
                // a versioned folder, so set the property!
                SVNPool       setPool(static_cast<apr_pool_t*>(subPool));
                CTSVNPathList target = CTSVNPathList(path);
                SVNTRACE(
                    m_err = svn_client_propset_local(name.c_str(), pVal, target.MakePathArray(setPool), svn_depth_empty, false, NULL, m_pCtx, setPool),
                    NULL)
            }
            status = stat.GetNextFileStatus(path);
#ifdef _MFC_VER
            if ((m_pProgress) && (m_pProgress->HasUserCancelled()))
                m_err = svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
#endif
        } while ((status != nullptr) && (m_err == nullptr));
    }
    else
    {
        m_revSet            = SVN_INVALID_REVNUM;
        const char* svnPath = m_path.GetSVNApiPath(subPool);
        if (m_bRevProps)
        {
            SVNTRACE(
                m_err = svn_client_revprop_set2(name.c_str(), pVal, NULL, svnPath, m_rev, &m_revSet, force, m_pCtx, subPool),
                svnPath)
        }
        else
        {
            if (m_path.IsUrl())
            {
                SVNTRACE(
                    m_err = svn_client_propset_remote(name.c_str(), pVal, svnPath, force, m_rev, NULL, &CommitCallback, &m_revSet, m_pCtx, subPool),
                    svnPath)
            }
            else
            {
                CTSVNPathList target = CTSVNPathList(m_path);
                SVNTRACE(
                    m_err = svn_client_propset_local(name.c_str(), pVal, target.MakePathArray(subPool), depth, false, NULL, m_pCtx, subPool),
                    NULL)
            }
        }
    }
    if (m_err != nullptr)
    {
        return FALSE;
    }

    if (m_revSet != SVN_INVALID_REVNUM)
        m_rev = m_revSet;

    return TRUE;
}

BOOL SVNProperties::Remove(const std::string& name, svn_depth_t depth, const wchar_t* message)
{
    m_err = nullptr;

    SVNPool subPool(m_pool);
    svn_error_clear(m_err);
    m_err = nullptr;
    PrepareMsgForUrl(message, subPool);

    m_revSet            = SVN_INVALID_REVNUM;
    const char* svnPath = m_path.GetSVNApiPath(subPool);
#ifdef _MFC_VER
    if (m_path.IsUrl() || (!m_rev.IsWorking() && !m_rev.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(m_path));
#endif
    if (m_bRevProps)
    {
        SVNTRACE(
            m_err = svn_client_revprop_set2(name.c_str(), NULL, NULL, svnPath, m_rev, &m_revSet, false, m_pCtx, subPool),
            svnPath)
    }
    else
    {
        if (!m_path.IsUrl() && ((depth != svn_depth_empty) && IsFolderOnlyProperty(name)))
        {
            CTSVNPath            path;
            SVNStatus            stat;
            svn_client_status_t* status = stat.GetFirstFileStatus(m_path, path, false, depth, true, true);
            if (status == nullptr)
            {
                m_err = svn_error_dup(const_cast<svn_error_t*>(stat.GetSVNError()));
                return FALSE;
            }
            do
            {
                if ((status) && (status->kind == svn_node_dir) && (status->node_status != svn_wc_status_missing) && (status->node_status != svn_wc_status_deleted) && (status->node_status != svn_wc_status_unversioned))
                {
#ifdef _MFC_VER
                    if (m_pProgress)
                        m_pProgress->SetLine(2, path.GetWinPath(), true);
#endif
                    SVNPool       setPool(static_cast<apr_pool_t*>(subPool));
                    CTSVNPathList target = CTSVNPathList(path);
                    SVNTRACE(
                        m_err = svn_client_propset_local(name.c_str(), NULL, target.MakePathArray(setPool), svn_depth_empty, false, NULL, m_pCtx, setPool),
                        NULL)
                }
                status = stat.GetNextFileStatus(path);
#ifdef _MFC_VER
                if ((m_pProgress) && (m_pProgress->HasUserCancelled()))
                    m_err = svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(CString(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED))));
#endif
            } while ((status != nullptr) && (m_err == nullptr));
        }
        else
        {
            if (m_path.IsUrl())
            {
                SVNTRACE(
                    m_err = svn_client_propset_remote(name.c_str(), NULL, svnPath, true, m_rev, NULL, &CommitCallback, &m_revSet, m_pCtx, subPool),
                    svnPath)
            }
            else
            {
                CTSVNPathList target = CTSVNPathList(m_path);
                SVNTRACE(
                    m_err = svn_client_propset_local(name.c_str(), NULL, target.MakePathArray(subPool), depth, false, NULL, m_pCtx, subPool),
                    NULL)
            }
        }
    }

    if (m_err != nullptr)
    {
        return FALSE;
    }

    if (m_revSet != SVN_INVALID_REVNUM)
        m_rev = m_revSet;

    return TRUE;
}

namespace
{
std::pair<std::string, std::string> GetKeyValue(const std::string& source, size_t& offset)
{
    // read "K %d\n" line

    int keyLength = atoi(source.c_str() + offset + 2);
    for (offset += 4; source[offset - 1] != '\n'; ++offset)
    {
    }

    std::string key(source.c_str() + offset, static_cast<size_t>(keyLength));
    offset += keyLength + 1;

    // read "V %d\n" line

    int valueLength = atoi(source.c_str() + offset + 2);
    for (offset += 4; source[offset - 1] != '\n'; ++offset)
    {
    }

    std::string value(source.c_str() + offset, static_cast<size_t>(valueLength));
    offset += valueLength + 1;

    // terminating \n

    return std::pair<std::string, std::string>(key, value);
}

} // namespace

void SVNProperties::SetFromSerializedForm(const std::string& text)
{
    // clear properties list
    apr_hash_clear(m_props);

    // read all file contents

    for (size_t offset = 0; offset < text.length();)
    {
        // create hash for this file

        std::pair<std::string, std::string> props = GetKeyValue(text, offset);

        m_props = apr_hash_make(m_pool);

        // read all prop entries into hash

        for (size_t propOffset = 0; propOffset < props.second.length();)
        {
            std::pair<std::string, std::string> prop = GetKeyValue(props.second, propOffset);

            svn_string_t* value = svn_string_ncreate(prop.second.c_str(), prop.second.length(), m_pool);
            apr_hash_set(m_props, prop.first.c_str(), prop.first.length(), value);
        }
    }
}

void SVNProperties::PrepareMsgForUrl(const wchar_t* message, SVNPool& subPool) const
{
    if (m_path.IsUrl())
    {
        CString msg = message ? message : L"";
        msg.Remove('\r');
        LogMsgBaton3* baton    = static_cast<LogMsgBaton3*>(apr_palloc(subPool, sizeof(*baton)));
        baton->message         = apr_pstrdup(subPool, CUnicodeUtils::GetUTF8(msg));
        baton->baseDir         = "";
        baton->messageEncoding = nullptr;
        baton->tmpfileLeft     = nullptr;
        baton->pool            = subPool;
        m_pCtx->log_msg_baton3 = baton;
    }
}

svn_error_t* SVNProperties::CommitCallback(const svn_commit_info_t* commitInfo, void* baton, apr_pool_t* /*pool*/)
{
    svn_revnum_t* pRev = static_cast<svn_revnum_t*>(baton);
    *pRev              = commitInfo->revision;
    return nullptr;
}
