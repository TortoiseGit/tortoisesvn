// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseSVN

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
//
#pragma once
#include "stdafx.h"
#include "ConflictEditorCommand.h"
#include "SVNStatus.h"
#include "SVNDiff.h"
#include "SVNInfo.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "EditPropConflictDlg.h"
#include "TreeConflictEditorDlg.h"

bool ConflictEditorCommand::Execute()
{
    CTSVNPath merge = cmdLinePath;
    CTSVNPath directory = merge.GetDirectory();
    bool bRet = false;
    bool bAlternativeTool = !!parser.HasKey(_T("alternative"));

    // we have the conflicted file (%merged)
    // now look for the other required files

    SVNInfo info;
    const SVNInfoData * pInfoData = info.GetFirstFileInfo(merge, SVNRev(), SVNRev());
    if (pInfoData == NULL)
        return false;

    for (auto conflIt = pInfoData->conflicts.cbegin(); conflIt != pInfoData->conflicts.cend(); ++conflIt)
    {
        switch (conflIt->kind)
        {
        case svn_wc_conflict_kind_text:
            {
                // we have a text conflict, use our merge tool to resolve the conflict

                CTSVNPath theirs = CTSVNPath(conflIt->conflict_new);
                CTSVNPath mine   = CTSVNPath(conflIt->conflict_wrk);
                CTSVNPath base   = CTSVNPath(conflIt->conflict_old);
                if (mine.IsEmpty())
                    mine = merge;
                bRet = !!CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(bAlternativeTool),
                                                  base, theirs, mine, merge, true);
            }
            break;
        case svn_wc_conflict_kind_property:
            {
                // we have a property conflict
                CTSVNPath prej(conflIt->prejfile);
                if (!prej.IsEmpty())
                {
                    // there's a problem: the prej file contains a _description_ of the conflict, and
                    // that description string might be translated. That means we have no way of parsing
                    // the file to find out the conflicting values.
                    // The only thing we can do: show a dialog with the conflict description, then
                    // let the user either accept the existing property or open the property edit dialog
                    // to manually change the properties and values. And a button to mark the conflict as
                    // resolved.
                    CEditPropConflictDlg dlg;
                    dlg.SetPrejFile(prej);
                    dlg.SetConflictedItem(merge);
                    bRet = (dlg.DoModal() != IDCANCEL);
                }
            }
            break;
        case svn_wc_conflict_kind_tree:
            {
                CString sConflictAction;
                CString sConflictReason;
                CString sResolveTheirs;
                CString sResolveMine;
                CTSVNPath treeConflictPath = CTSVNPath(conflIt->treeconflict_path);
                CString sItemName = treeConflictPath.GetUIFileOrDirectoryName();

                if (conflIt->treeconflict_nodekind == svn_node_file)
                {
                    switch (conflIt->treeconflict_operation)
                    {
                    default:
                    case svn_wc_operation_none:
                    case svn_wc_operation_update:
                        switch (conflIt->treeconflict_action)
                        {
                        default:
                        case svn_wc_conflict_action_edit:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEEDIT, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                            break;
                        case svn_wc_conflict_action_add:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEADD, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                            break;
                        case svn_wc_conflict_action_delete:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEDELETE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                            break;
                        case svn_wc_conflict_action_replace:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEREPLACE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                            break;
                        }
                        break;
                    case svn_wc_operation_switch:
                        switch (conflIt->treeconflict_action)
                        {
                        default:
                        case svn_wc_conflict_action_edit:
                            sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHEDIT, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                            break;
                        case svn_wc_conflict_action_add:
                            sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHADD, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                            break;
                        case svn_wc_conflict_action_delete:
                            sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHDELETE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                            break;
                        case svn_wc_conflict_action_replace:
                            sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHREPLACE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                            break;
                        }
                        break;
                    case svn_wc_operation_merge:
                        switch (conflIt->treeconflict_action)
                        {
                        default:
                        case svn_wc_conflict_action_edit:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEEDIT, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                            break;
                        case svn_wc_conflict_action_add:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEADD, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                            break;
                        case svn_wc_conflict_action_delete:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEDELETE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                            break;
                        case svn_wc_conflict_action_replace:
                            sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEREPLACE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                            break;
                        }
                        break;
                    }
                }
                else //if (pInfoData->treeconflict_nodekind == svn_node_dir)
                {
                    switch (conflIt->treeconflict_operation)
                    {
                    default:
                    case svn_wc_operation_none:
                    case svn_wc_operation_update:
                        switch (conflIt->treeconflict_action)
                        {
                        default:
                        case svn_wc_conflict_action_edit:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEEDIT, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                            break;
                        case svn_wc_conflict_action_add:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEADD, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                            break;
                        case svn_wc_conflict_action_delete:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEDELETE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                            break;
                        case svn_wc_conflict_action_replace:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEREPLACE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                            break;
                        }
                        break;
                    case svn_wc_operation_switch:
                        switch (conflIt->treeconflict_action)
                        {
                        default:
                        case svn_wc_conflict_action_edit:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHEDIT, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                            break;
                        case svn_wc_conflict_action_add:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHADD, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                            break;
                        case svn_wc_conflict_action_delete:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHDELETE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                            break;
                        case svn_wc_conflict_action_replace:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHREPLACE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                            break;
                        }
                        break;
                    case svn_wc_operation_merge:
                        switch (conflIt->treeconflict_action)
                        {
                        default:
                        case svn_wc_conflict_action_edit:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEEDIT, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                            break;
                        case svn_wc_conflict_action_add:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEADD, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                            break;
                        case svn_wc_conflict_action_delete:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEDELETE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                            break;
                        case svn_wc_conflict_action_replace:
                            sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEREPLACE, (LPCTSTR)sItemName);
                            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                            break;
                        }
                        break;
                    }
                }

                UINT uReasonID = 0;
                switch (conflIt->treeconflict_reason)
                {
                case svn_wc_conflict_reason_edited:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_EDITED : IDS_TREECONFLICT_REASON_FILE_EDITED;
                    sResolveMine.LoadString(conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
                    break;
                case svn_wc_conflict_reason_obstructed:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_OBSTRUCTED : IDS_TREECONFLICT_REASON_FILE_OBSTRUCTED;
                    sResolveMine.LoadString(conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
                    break;
                case svn_wc_conflict_reason_deleted:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_DELETED : IDS_TREECONFLICT_REASON_FILE_DELETED;
                    sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MARKASRESOLVED);
                    break;
                case svn_wc_conflict_reason_added:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_ADDED : IDS_TREECONFLICT_REASON_FILE_ADDED;
                    sResolveMine.LoadString(conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
                    break;
                case svn_wc_conflict_reason_missing:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_MISSING : IDS_TREECONFLICT_REASON_FILE_MISSING;
                    sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MARKASRESOLVED);
                    break;
                case svn_wc_conflict_reason_unversioned:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_UNVERSIONED : IDS_TREECONFLICT_REASON_FILE_UNVERSIONED;
                    sResolveMine.LoadString(conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
                    break;
                case svn_wc_conflict_reason_replaced:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_REPLACED : IDS_TREECONFLICT_REASON_FILE_REPLACED;
                    sResolveMine.LoadString(conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
                    break;
                case svn_wc_conflict_reason_moved_away:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_MOVEDAWAY : IDS_TREECONFLICT_REASON_FILE_MOVEDAWAY;
                    sResolveMine.LoadString(conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
                    break;
                case svn_wc_conflict_reason_moved_here:
                    uReasonID = conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_MOVEDHERE : IDS_TREECONFLICT_REASON_FILE_MOVEDHERE;
                    sResolveMine.LoadString(conflIt->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
                    break;
                }
                sConflictReason.Format(uReasonID, (LPCTSTR)sConflictAction);

                CTreeConflictEditorDlg dlg;
                dlg.SetConflictInfoText(sConflictReason);
                dlg.SetResolveTexts(sResolveTheirs, sResolveMine);
                dlg.SetPath(treeConflictPath);
                dlg.SetConflictLeftSources(conflIt->src_left_version_url, conflIt->src_left_version_path, conflIt->src_left_version_rev, conflIt->src_left_version_kind);
                dlg.SetConflictRightSources(conflIt->src_right_version_url, conflIt->src_right_version_path, conflIt->src_right_version_rev, conflIt->src_right_version_kind);
                dlg.SetConflictReason(conflIt->treeconflict_reason);
                dlg.SetConflictAction(conflIt->treeconflict_action);
                INT_PTR dlgRet = dlg.DoModal();
                bRet = (dlgRet != IDCANCEL);
            }
            break;
        }
    }

    return bRet;
}


