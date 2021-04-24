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
#include "resource.h"

#include "SVNConfig.h"
#include "SVNInfo.h"
#include "SVNHelpers.h"
#include "UnicodeUtils.h"
#ifdef _MFC_VER
#    include "SVN.h"
#    include "Hooks.h"
#endif
#include "TSVNPath.h"
#include "SVNTrace.h"

SVNConflictData::SVNConflictData()
    : kind(svn_wc_conflict_kind_text)
    , treeConflictNodeKind(svn_node_none)
    , treeConflictBinary(false)
    , treeConflictAction(svn_wc_conflict_action_edit)
    , treeConflictReason(svn_wc_conflict_reason_edited)
    , treeConflictOperation(svn_wc_operation_none)
    , srcRightVersionKind(svn_node_none)
    , srcLeftVersionKind(svn_node_none)
{
}

SVNInfoData::SVNInfoData()
    : kind(svn_node_none)
    , lastChangedTime(0)
    , lockDavComment(false)
    , lockCreateTime(0)
    , lockExpirationTime(0)
    , size64(0)
    , hasWcInfo(false)
    , schedule(svn_wc_schedule_normal)
    , textTime(0)
    , depth(svn_depth_unknown)
    , workingSize64(0)
{
}

#ifdef _MFC_VER
SVNInfo::SVNInfo(bool suppressUI)
#else
SVNInfo::SVNInfo(bool)
#endif
    : SVNBase()
    , m_pos(0)
#ifdef _MFC_VER
    , m_prompt(suppressUI)
#endif
{
    m_pool = svn_pool_create(nullptr);

    svn_error_clear(svn_client_create_context2(&m_pCtx, SVNConfig::Instance().GetConfig(m_pool), m_pool));

#ifdef _MFC_VER
    // set up authentication
    m_prompt.Init(m_pool, m_pCtx);
#endif
    m_pCtx->cancel_func  = cancel;
    m_pCtx->cancel_baton = this;
    m_pCtx->client_name  = SVNHelper::GetUserAgentString(m_pool);

    if (m_err)
    {
#ifdef _MFC_VER
        ShowErrorDialog(nullptr);
#endif
        svn_error_clear(m_err);
        svn_pool_destroy(m_pool); // free the allocated memory
    }
}

SVNInfo::~SVNInfo()
{
    svn_error_clear(m_err);
    svn_pool_destroy(m_pool); // free the allocated memory
}

BOOL SVNInfo::Cancel() { return FALSE; };
void SVNInfo::Receiver(SVNInfoData* /*data*/) { return; }

svn_error_t* SVNInfo::cancel(void* baton)
{
    SVNInfo* pThis = static_cast<SVNInfo*>(baton);
    if (pThis->Cancel())
    {
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(temp));
    }
    return nullptr;
}

const SVNInfoData* SVNInfo::GetFirstFileInfo(const CTSVNPath& path, SVNRev pegRev, SVNRev revision,
                                             svn_depth_t depth /*= svn_depth_empty*/,
                                             bool fetchExcluded /*= true */, bool fetchActualOnly /*= true*/, bool includeExternals /*= false */)
{
    svn_error_clear(m_err);
    m_err = nullptr;
    m_arInfo.clear();
    m_pos = 0;

    const char* svnPath = path.GetSVNApiPath(m_pool);
    if ((svnPath == nullptr) || (svnPath[0] == 0))
        return nullptr;
#ifdef _MFC_VER
    if (path.IsUrl() || (!pegRev.IsWorking() && !pegRev.IsValid()) || (!revision.IsWorking() && !revision.IsValid()))
        CHooks::Instance().PreConnect(CTSVNPathList(path));
#endif
    SVNTRACE(
        m_err = svn_client_info4(svnPath, pegRev, revision, depth, fetchExcluded, fetchActualOnly, includeExternals, NULL, infoReceiver, this, m_pCtx, m_pool),
        svnPath)
    ClearCAPIAuthCacheOnError();
    if (m_err != nullptr)
        return nullptr;
    if (m_arInfo.empty())
        return nullptr;
    return &m_arInfo[0];
}

const SVNInfoData* SVNInfo::GetNextFileInfo()
{
    m_pos++;
    if (m_arInfo.size() > m_pos)
        return &m_arInfo[m_pos];
    return nullptr;
}

svn_error_t* SVNInfo::infoReceiver(void* baton, const char* path, const svn_client_info2_t* info, apr_pool_t* pool)
{
    if ((path == nullptr) || (info == nullptr))
        return nullptr;

    SVNInfo* pThis = static_cast<SVNInfo*>(baton);

    SVNInfoData data;
    data.path.SetFromUnknown(CUnicodeUtils::GetUnicode(path));
    if (info->URL)
        data.url = CUnicodeUtils::GetUnicode(info->URL);
    data.rev  = SVNRev(info->rev);
    data.kind = info->kind;
    if (info->repos_root_URL)
        data.reposRoot = CUnicodeUtils::GetUnicode(info->repos_root_URL);
    if (info->repos_UUID)
        data.reposUuid = CUnicodeUtils::GetUnicode(info->repos_UUID);
    data.lastChangedRev  = SVNRev(info->last_changed_rev);
    data.lastChangedTime = info->last_changed_date / 1000000L;
    if (info->last_changed_author)
        data.author = CUnicodeUtils::GetUnicode(info->last_changed_author);
    data.size64 = info->size;

    if (info->lock)
    {
        if (info->lock->path)
            data.lockPath = CUnicodeUtils::GetUnicode(info->lock->path);
        if (info->lock->token)
            data.lockToken = CUnicodeUtils::GetUnicode(info->lock->token);
        if (info->lock->owner)
            data.lockOwner = CUnicodeUtils::GetUnicode(info->lock->owner);
        if (info->lock->comment)
            data.lockComment = CUnicodeUtils::GetUnicode(info->lock->comment);
        data.lockDavComment     = !!info->lock->is_dav_comment;
        data.lockCreateTime     = info->lock->creation_date / 1000000L;
        data.lockExpirationTime = info->lock->expiration_date / 1000000L;
    }

    data.hasWcInfo = info->wc_info != nullptr;
    if (info->wc_info)
    {
        data.depth    = info->wc_info->depth;
        data.schedule = info->wc_info->schedule;
        if (info->wc_info->copyfrom_url)
            data.copyFromUrl = CUnicodeUtils::GetUnicode(info->wc_info->copyfrom_url);
        data.copyFromRev = SVNRev(info->wc_info->copyfrom_rev);
        data.textTime    = info->wc_info->recorded_time / 1000000L;
        if (info->wc_info->checksum)
        {
            const char* cs = svn_checksum_to_cstring_display(info->wc_info->checksum, pool);
            data.checksum  = CUnicodeUtils::GetUnicode(cs);
        }
        if (info->wc_info->changelist)
            data.changeList = CUnicodeUtils::GetUnicode(info->wc_info->changelist);
        data.workingSize64 = info->wc_info->recorded_size;
        if (info->wc_info->wcroot_abspath)
            data.wcRoot = CUnicodeUtils::GetUnicode(info->wc_info->wcroot_abspath);
        if (info->wc_info->moved_from_abspath)
            data.movedFromAbspath = CUnicodeUtils::GetUnicode(info->wc_info->moved_from_abspath);
        if (info->wc_info->moved_to_abspath)
            data.movedToAbspath = CUnicodeUtils::GetUnicode(info->wc_info->moved_to_abspath);

        if (info->wc_info->conflicts)
        {
            for (int i = 0; i < info->wc_info->conflicts->nelts; ++i)
            {
                const svn_wc_conflict_description2_t* conflict = APR_ARRAY_IDX(info->wc_info->conflicts, i, const svn_wc_conflict_description2_t*);
                SVNConflictData                       cdata;
                cdata.kind = conflict->kind;
                switch (conflict->kind)
                {
                    case svn_wc_conflict_kind_text:
                        if (conflict->their_abspath)
                            cdata.conflictNew = CUnicodeUtils::GetUnicode(conflict->their_abspath);
                        if (conflict->base_abspath)
                            cdata.conflictOld = CUnicodeUtils::GetUnicode(conflict->base_abspath);
                        if (conflict->my_abspath)
                            cdata.conflictWrk = CUnicodeUtils::GetUnicode(conflict->my_abspath);
                        break;
                    case svn_wc_conflict_kind_property:
                        if (conflict->their_abspath)
                            cdata.prejFile = CUnicodeUtils::GetUnicode(conflict->their_abspath);
                        if (conflict->prop_reject_abspath)
                            cdata.prejFile = CUnicodeUtils::GetUnicode(conflict->prop_reject_abspath);
                        if (conflict->property_name)
                            cdata.propName = conflict->property_name;

                        if (conflict->prop_value_base)
                            cdata.propValueBase = std::string(conflict->prop_value_base->data, conflict->prop_value_base->len);
                        if (conflict->prop_value_working)
                            cdata.propValueWorking = std::string(conflict->prop_value_working->data, conflict->prop_value_working->len);
                        if (conflict->prop_value_incoming_old)
                            cdata.propValueIncomingOld = std::string(conflict->prop_value_incoming_old->data, conflict->prop_value_incoming_old->len);
                        if (conflict->prop_value_incoming_new)
                            cdata.propValueIncomingNew = std::string(conflict->prop_value_incoming_new->data, conflict->prop_value_incoming_new->len);
                        break;
                    case svn_wc_conflict_kind_tree:
                    {
                        if (conflict->local_abspath)
                            cdata.treeConflictPath = CUnicodeUtils::GetUnicode(conflict->local_abspath);
                        cdata.treeConflictNodeKind = conflict->node_kind;
                        if (conflict->property_name)
                            cdata.treeConflictPropertyName = CUnicodeUtils::GetUnicode(conflict->property_name);
                        cdata.treeConflictBinary = !!conflict->is_binary;
                        if (conflict->mime_type)
                            cdata.treeConflictMimeType = CUnicodeUtils::GetUnicode(conflict->mime_type);
                        cdata.treeConflictAction    = conflict->action;
                        cdata.treeConflictReason    = conflict->reason;
                        cdata.treeConflictOperation = conflict->operation;
                        if (conflict->base_abspath)
                            cdata.treeConflictBaseFile = CUnicodeUtils::GetUnicode(conflict->base_abspath);
                        if (conflict->their_abspath)
                            cdata.treeConflictTheirFile = CUnicodeUtils::GetUnicode(conflict->their_abspath);
                        if (conflict->my_abspath)
                            cdata.treeConflictMyFile = CUnicodeUtils::GetUnicode(conflict->my_abspath);
                        if (conflict->merged_file)
                            cdata.treeConflictMergedFile = CUnicodeUtils::GetUnicode(conflict->merged_file);

                        if (conflict->src_right_version)
                        {
                            if (conflict->src_right_version->repos_url)
                                cdata.srcRightVersionUrl = CUnicodeUtils::GetUnicode(conflict->src_right_version->repos_url);
                            if (conflict->src_right_version->path_in_repos)
                                cdata.srcRightVersionPath = CUnicodeUtils::GetUnicode(conflict->src_right_version->path_in_repos);
                            cdata.srcRightVersionRev  = conflict->src_right_version->peg_rev;
                            cdata.srcRightVersionKind = conflict->src_right_version->node_kind;
                        }
                        if (conflict->src_left_version)
                        {
                            if (conflict->src_left_version->repos_url)
                                cdata.srcLeftVersionUrl = CUnicodeUtils::GetUnicode(conflict->src_left_version->repos_url);
                            if (conflict->src_left_version->path_in_repos)
                                cdata.srcLeftVersionPath = CUnicodeUtils::GetUnicode(conflict->src_left_version->path_in_repos);
                            cdata.srcLeftVersionRev  = conflict->src_left_version->peg_rev;
                            cdata.srcLeftVersionKind = conflict->src_left_version->node_kind;
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
    return nullptr;
}

// convenience methods

bool SVNInfo::IsFile(const CTSVNPath& path, const SVNRev& revision)
{
    SVNInfo            info;
    const SVNInfoData* infoData = info.GetFirstFileInfo(path, revision, revision);

    return (infoData != nullptr) && (infoData->kind == svn_node_file);
}

#ifdef _MFC_VER
void SVNInfo::SetPromptParentWindow(HWND hWnd)
{
    m_prompt.SetParentWindow(hWnd);
}
#endif
