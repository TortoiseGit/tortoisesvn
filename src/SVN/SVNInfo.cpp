// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013 - TortoiseSVN

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

#include "SVNConfig.h"
#include "SVNInfo.h"
#include "SVNHelpers.h"
#include "UnicodeUtils.h"
#ifdef _MFC_VER
#include "SVN.h"
#include "Hooks.h"
#endif
#include "registry.h"
#include "TSVNPath.h"
#include "PathUtils.h"
#include "SVNTrace.h"

SVNConflictData::SVNConflictData()
    : treeconflict_binary(false)
    , kind(svn_wc_conflict_kind_text)
    , treeconflict_nodekind(svn_node_none)
    , treeconflict_action(svn_wc_conflict_action_edit)
    , treeconflict_reason(svn_wc_conflict_reason_edited)
    , treeconflict_operation(svn_wc_operation_none)
    , src_right_version_kind(svn_node_none)
    , src_left_version_kind(svn_node_none)
{
}

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
    , depth(svn_depth_unknown)
    , working_size64(0)
{
}

#ifdef _MFC_VER
SVNInfo::SVNInfo (bool suppressUI)
#else
SVNInfo::SVNInfo (bool)
#endif
    : SVNBase()
    , m_pos(0)
#ifdef _MFC_VER
    , m_prompt (suppressUI)
#endif
{
    m_pool = svn_pool_create (NULL);

    svn_error_clear(svn_client_create_context2(&m_pctx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

#ifdef _MFC_VER
    // set up authentication
    m_prompt.Init(m_pool, m_pctx);
#endif
    m_pctx->cancel_func = cancel;
    m_pctx->cancel_baton = this;
    m_pctx->client_name = SVNHelper::GetUserAgentString(m_pool);


    if (Err)
    {
#ifdef _MFC_VER
        ShowErrorDialog(NULL);
#endif
        svn_error_clear(Err);
        svn_pool_destroy (m_pool);                  // free the allocated memory
    }
}

SVNInfo::~SVNInfo(void)
{
    svn_error_clear(Err);
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

const SVNInfoData * SVNInfo::GetFirstFileInfo(const CTSVNPath& path, SVNRev pegrev, SVNRev revision,
                                              svn_depth_t depth /* = svn_depth_empty*/,
                                              bool fetchExcluded /* = true */, bool fetchActualOnly /* = true */)
{
    svn_error_clear(Err);
    Err = NULL;
    m_arInfo.clear();
    m_pos = 0;

    const char* svnPath = path.GetSVNApiPath(m_pool);
    if ((svnPath == 0)||(svnPath[0] == 0))
        return NULL;
#ifdef _MFC_VER
    if (path.IsUrl() || (!pegrev.IsWorking() && !pegrev.IsValid())|| (!revision.IsWorking() && !revision.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(path));
#endif
    SVNTRACE (
        Err = svn_client_info3(svnPath, pegrev, revision, depth, fetchExcluded, fetchActualOnly, NULL, infoReceiver, this, m_pctx, m_pool),
        svnPath
    )
    ClearCAPIAuthCacheOnError();
    if (Err != NULL)
        return NULL;
    if (m_arInfo.empty())
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

svn_error_t * SVNInfo::infoReceiver(void* baton, const char * path, const svn_client_info2_t* info, apr_pool_t * pool)
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
    data.size64 = info->size;

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

    data.hasWCInfo = info->wc_info != NULL;
    if (info->wc_info)
    {
        data.depth = info->wc_info->depth;
        data.schedule = info->wc_info->schedule;
        if (info->wc_info->copyfrom_url)
            data.copyfromurl = CUnicodeUtils::GetUnicode(info->wc_info->copyfrom_url);
        data.copyfromrev = SVNRev(info->wc_info->copyfrom_rev);
        data.texttime = info->wc_info->recorded_time/1000000L;
        if (info->wc_info->checksum)
        {
            const char * cs = svn_checksum_to_cstring_display(info->wc_info->checksum, pool);
            data.checksum = CUnicodeUtils::GetUnicode(cs);
        }
        if (info->wc_info->changelist)
            data.changelist = CUnicodeUtils::GetUnicode(info->wc_info->changelist);
        data.working_size64 = info->wc_info->recorded_size;
        if (info->wc_info->wcroot_abspath)
            data.wcroot = CUnicodeUtils::GetUnicode(info->wc_info->wcroot_abspath);
        if (info->wc_info->moved_from_abspath)
            data.moved_from_abspath = CUnicodeUtils::GetUnicode(info->wc_info->moved_from_abspath);
        if (info->wc_info->moved_to_abspath)
            data.moved_to_abspath = CUnicodeUtils::GetUnicode(info->wc_info->moved_to_abspath);

        if (info->wc_info->conflicts)
        {
            for (int i = 0; i < info->wc_info->conflicts->nelts; ++i)
            {
                const svn_wc_conflict_description2_t *conflict = APR_ARRAY_IDX(info->wc_info->conflicts, i, const svn_wc_conflict_description2_t *);
                SVNConflictData cdata;
                cdata.kind = conflict->kind;
                switch (conflict->kind)
                {
                case svn_wc_conflict_kind_text:
                    if (conflict->their_abspath)
                        cdata.conflict_new = CUnicodeUtils::GetUnicode(conflict->their_abspath);
                    if (conflict->base_abspath)
                        cdata.conflict_old = CUnicodeUtils::GetUnicode(conflict->base_abspath);
                    if (conflict->my_abspath)
                        cdata.conflict_wrk = CUnicodeUtils::GetUnicode(conflict->my_abspath);
                    break;
                case svn_wc_conflict_kind_property:
                    if (conflict->their_abspath)
                        cdata.prejfile = CUnicodeUtils::GetUnicode(conflict->their_abspath);
                    break;
                case svn_wc_conflict_kind_tree:
                    {
                        if (conflict->local_abspath)
                            cdata.treeconflict_path = CUnicodeUtils::GetUnicode(conflict->local_abspath);
                        cdata.treeconflict_nodekind = conflict->node_kind;
                        if (conflict->property_name)
                            cdata.treeconflict_propertyname = CUnicodeUtils::GetUnicode(conflict->property_name);
                        cdata.treeconflict_binary = !!conflict->is_binary;
                        if (conflict->mime_type)
                            cdata.treeconflict_mimetype = CUnicodeUtils::GetUnicode(conflict->mime_type);
                        cdata.treeconflict_action = conflict->action;
                        cdata.treeconflict_reason = conflict->reason;
                        cdata.treeconflict_operation = conflict->operation;
                        if (conflict->base_abspath)
                            cdata.treeconflict_basefile = CUnicodeUtils::GetUnicode(conflict->base_abspath);
                        if (conflict->their_abspath)
                            cdata.treeconflict_theirfile = CUnicodeUtils::GetUnicode(conflict->their_abspath);
                        if (conflict->my_abspath)
                            cdata.treeconflict_myfile = CUnicodeUtils::GetUnicode(conflict->my_abspath);
                        if (conflict->merged_file)
                            cdata.treeconflict_mergedfile = CUnicodeUtils::GetUnicode(conflict->merged_file);

                        if (conflict->src_right_version)
                        {
                            if (conflict->src_right_version->repos_url)
                                cdata.src_right_version_url = CUnicodeUtils::GetUnicode(conflict->src_right_version->repos_url);
                            if (conflict->src_right_version->path_in_repos)
                                cdata.src_right_version_path = CUnicodeUtils::GetUnicode(conflict->src_right_version->path_in_repos);
                            cdata.src_right_version_rev = conflict->src_right_version->peg_rev;
                            cdata.src_right_version_kind = conflict->src_right_version->node_kind;
                        }
                        if (conflict->src_left_version)
                        {
                            if (conflict->src_left_version->repos_url)
                                cdata.src_left_version_url = CUnicodeUtils::GetUnicode(conflict->src_left_version->repos_url);
                            if (conflict->src_left_version->path_in_repos)
                                cdata.src_left_version_path = CUnicodeUtils::GetUnicode(conflict->src_left_version->path_in_repos);
                            cdata.src_left_version_rev = conflict->src_left_version->peg_rev;
                            cdata.src_left_version_kind = conflict->src_left_version->node_kind;
                        }
                    }
                    break;
                }
                data.conflicts.push_back(cdata);
            }
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

