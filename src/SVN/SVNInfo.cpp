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
#include "resource.h"

#include "SVNInfo.h"
#include "UnicodeUtils.h"
#ifdef _MFC_VER
#include "SVN.h"
#include "MessageBox.h"
#include "Hooks.h"
#endif
#include "registry.h"
#include "TSVNPath.h"
#include "PathUtils.h"
#include "SVNTrace.h"

SVNInfoData::SVNInfoData()
    : kind(svn_node_none)
    , lastchangedtime(0)
    , lock_davcomment(false)
    , lock_createtime(0)
    , lock_expirationtime(0)
    , size64(0)
    , hasWCInfo(false)
    , schedule(svn_wc_schedule_normal)
    , texttime(0)
    , proptime(0)
    , depth(svn_depth_unknown)
    , working_size64(0)
    , treeconflict_binary(false)
{
}

#ifdef _MFC_VER
SVNInfo::SVNInfo (bool suppressUI)
#else
SVNInfo::SVNInfo (bool)
#endif
    : m_pctx(NULL)
    , m_pos(0)
    , m_err(NULL)
#ifdef _MFC_VER
    , m_prompt (suppressUI)
#endif
{
    m_pool = svn_pool_create (NULL);

    svn_error_clear(svn_client_create_context(&m_pctx, m_pool));

    svn_error_clear(svn_config_ensure(NULL, m_pool));

#ifdef _MFC_VER
    // set up the configuration
    m_err = svn_config_get_config (&(m_pctx->config), g_pConfigDir, m_pool);
    // set up authentication
    m_prompt.Init(m_pool, m_pctx);
#else
    // set up the configuration
    m_err = svn_config_get_config (&(m_pctx->config), NULL, m_pool);
#endif
    m_pctx->cancel_func = cancel;
    m_pctx->cancel_baton = this;


    if (m_err)
    {
#ifdef _MFC_VER
        ::MessageBox(NULL, this->GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
#endif
        svn_error_clear(m_err);
        svn_pool_destroy (m_pool);                  // free the allocated memory
    }
#ifdef _MFC_VER
    else
    {
        //set up the SVN_SSH param
        CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
        if (tsvn_ssh.IsEmpty())
            tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
        tsvn_ssh.Replace('\\', '/');
        if (!tsvn_ssh.IsEmpty())
        {
            svn_config_t * cfg = (svn_config_t *)apr_hash_get ((apr_hash_t *)m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
                APR_HASH_KEY_STRING);
            svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
        }
    }
#endif
}

SVNInfo::~SVNInfo(void)
{
    svn_error_clear(m_err);
    svn_pool_destroy (m_pool);                  // free the allocated memory
}

BOOL SVNInfo::Cancel() {return FALSE;};
void SVNInfo::Receiver(SVNInfoData * /*data*/) {return;}

svn_error_t* SVNInfo::cancel(void *baton)
{
    SVNInfo * pThis = (SVNInfo *)baton;
    if (pThis->Cancel())
    {
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
    }
    return SVN_NO_ERROR;
}

#ifdef _MFC_VER
CString SVNInfo::GetLastErrorMsg()
{
    return SVN::GetErrorString(m_err);
}
#endif

const SVNInfoData * SVNInfo::GetFirstFileInfo(const CTSVNPath& path, SVNRev pegrev, SVNRev revision, svn_depth_t depth /* = svn_depth_empty*/)
{
    svn_error_clear(m_err);
    m_arInfo.clear();
    m_pos = 0;

    const char* svnPath = path.GetSVNApiPath(m_pool);
#ifdef _MFC_VER
    if (path.IsUrl() || (!pegrev.IsWorking() && !pegrev.IsValid())|| (!revision.IsWorking() && !revision.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(path));
#endif
    SVNTRACE (
        m_err = svn_client_info3(svnPath, pegrev, revision, infoReceiver, this, depth, NULL, m_pctx, m_pool),
        svnPath
    )
    if (m_err != NULL)
        return NULL;
    if (m_arInfo.size() == 0)
        return NULL;
    return &m_arInfo[0];
}

const SVNInfoData * SVNInfo::GetNextFileInfo()
{
    m_pos++;
    if (m_arInfo.size()>m_pos)
        return &m_arInfo[m_pos];
    return NULL;
}

svn_error_t * SVNInfo::infoReceiver(void* baton, const char * path, const svn_info_t* info, apr_pool_t * /*pool*/)
{
    if ((path == NULL)||(info == NULL))
        return NULL;

    SVNInfo * pThis = (SVNInfo *)baton;

    SVNInfoData data;
    data.path.SetFromUnknown(CUnicodeUtils::GetUnicode(path));
    if (info->URL)
        data.url = CUnicodeUtils::GetUnicode(info->URL);
    data.rev = SVNRev(info->rev);
    data.kind = info->kind;
    if (info->repos_root_URL)
        data.reposRoot = CUnicodeUtils::GetUnicode(info->repos_root_URL);
    if (info->repos_UUID)
        data.reposUUID = CUnicodeUtils::GetUnicode(info->repos_UUID);
    data.lastchangedrev = SVNRev(info->last_changed_rev);
    data.lastchangedtime = info->last_changed_date/1000000L;
    if (info->last_changed_author)
        data.author = CUnicodeUtils::GetUnicode(info->last_changed_author);
    data.depth = info->depth;
    data.size64 = info->size64;

    if (info->lock)
    {
        if (info->lock->path)
            data.lock_path = CUnicodeUtils::GetUnicode(info->lock->path);
        if (info->lock->token)
            data.lock_token = CUnicodeUtils::GetUnicode(info->lock->token);
        if (info->lock->owner)
            data.lock_owner = CUnicodeUtils::GetUnicode(info->lock->owner);
        if (info->lock->comment)
            data.lock_comment = CUnicodeUtils::GetUnicode(info->lock->comment);
        data.lock_davcomment = !!info->lock->is_dav_comment;
        data.lock_createtime = info->lock->creation_date/1000000L;
        data.lock_expirationtime = info->lock->expiration_date/1000000L;
    }

    data.hasWCInfo = !!info->has_wc_info;
    if (info->has_wc_info)
    {
        data.schedule = info->schedule;
        if (info->copyfrom_url)
            data.copyfromurl = CUnicodeUtils::GetUnicode(info->copyfrom_url);
        data.copyfromrev = SVNRev(info->copyfrom_rev);
        data.texttime = info->text_time/1000000L;
        data.proptime = info->prop_time/1000000L;
        if (info->checksum)
            data.checksum = CUnicodeUtils::GetUnicode(info->checksum);
        if (info->conflict_new)
            data.conflict_new = CUnicodeUtils::GetUnicode(info->conflict_new);
        if (info->conflict_old)
            data.conflict_old = CUnicodeUtils::GetUnicode(info->conflict_old);
        if (info->conflict_wrk)
            data.conflict_wrk = CUnicodeUtils::GetUnicode(info->conflict_wrk);
        if (info->prejfile)
            data.prejfile = CUnicodeUtils::GetUnicode(info->prejfile);
        if (info->changelist)
            data.changelist = CUnicodeUtils::GetUnicode(info->changelist);
        data.working_size64 = info->working_size64;
    }
    if (info->tree_conflict)
    {
        if (info->tree_conflict->path)
            data.treeconflict_path = CUnicodeUtils::GetUnicode(info->tree_conflict->path);
        data.treeconflict_nodekind = info->tree_conflict->node_kind;
        data.treeconflict_kind = info->tree_conflict->kind;
        if (info->tree_conflict->property_name)
            data.treeconflict_propertyname = CUnicodeUtils::GetUnicode(info->tree_conflict->property_name);
        data.treeconflict_binary = !!info->tree_conflict->is_binary;
        if (info->tree_conflict->mime_type)
            data.treeconflict_mimetype = CUnicodeUtils::GetUnicode(info->tree_conflict->mime_type);
        data.treeconflict_action = info->tree_conflict->action;
        data.treeconflict_reason = info->tree_conflict->reason;
        data.treeconflict_operation = info->tree_conflict->operation;
        if (info->tree_conflict->base_file)
            data.treeconflict_basefile = CUnicodeUtils::GetUnicode(info->tree_conflict->base_file);
        if (info->tree_conflict->their_file)
            data.treeconflict_theirfile = CUnicodeUtils::GetUnicode(info->tree_conflict->their_file);
        if (info->tree_conflict->my_file)
            data.treeconflict_myfile = CUnicodeUtils::GetUnicode(info->tree_conflict->my_file);
        if (info->tree_conflict->merged_file)
            data.treeconflict_mergedfile = CUnicodeUtils::GetUnicode(info->tree_conflict->merged_file);

        if (info->tree_conflict->src_right_version)
        {
            if (info->tree_conflict->src_right_version->repos_url)
                data.src_right_version_url = CUnicodeUtils::GetUnicode(info->tree_conflict->src_right_version->path_in_repos);
            if (info->tree_conflict->src_right_version->path_in_repos)
                data.src_right_version_path = CUnicodeUtils::GetUnicode(info->tree_conflict->src_right_version->repos_url);
            data.src_right_version_rev = info->tree_conflict->src_right_version->peg_rev;
            data.src_right_version_kind = info->tree_conflict->src_right_version->node_kind;
    }
        if (info->tree_conflict->src_left_version)
        {
            if (info->tree_conflict->src_left_version->repos_url)
                data.src_left_version_url = CUnicodeUtils::GetUnicode(info->tree_conflict->src_left_version->path_in_repos);
            if (info->tree_conflict->src_left_version->path_in_repos)
                data.src_left_version_path = CUnicodeUtils::GetUnicode(info->tree_conflict->src_left_version->repos_url);
            data.src_left_version_rev = info->tree_conflict->src_left_version->peg_rev;
            data.src_left_version_kind = info->tree_conflict->src_left_version->node_kind;
        }
    }
    pThis->m_arInfo.push_back(data);
    pThis->Receiver(&data);
    return NULL;
}

// convenience methods

bool SVNInfo::IsFile (const CTSVNPath& path, const SVNRev& revision)
{
    SVNInfo info;
    const SVNInfoData* infoData
        = info.GetFirstFileInfo (path, revision, revision);

    return (infoData != NULL) && (infoData->kind == svn_node_file);
}

