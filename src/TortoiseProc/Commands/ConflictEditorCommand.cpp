// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseSVN

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
	SVNStatus stat;
	stat.GetStatus(merge);
	if (stat.status == NULL)
		return false;

	if (stat.status->text_status == svn_wc_status_conflicted)
	{
		// we have a text conflict, use our merge tool to resolve the conflict

		CTSVNPath theirs(directory);
		CTSVNPath mine(directory);
		CTSVNPath base(directory);
		bool bConflictData = false;

		if ((stat.status->entry)&&(stat.status->entry->conflict_new))
		{
			theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
			bConflictData = true;
		}
		if ((stat.status->entry)&&(stat.status->entry->conflict_old))
		{
			base.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old));
			bConflictData = true;
		}
		if ((stat.status->entry)&&(stat.status->entry->conflict_wrk))
		{
			mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
			bConflictData = true;
		}
		else
		{
			mine = merge;
		}
		if (bConflictData)
			bRet = !!CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(bAlternativeTool), 
												base, theirs, mine, merge);
	}

	if (stat.status->prop_status == svn_wc_status_conflicted)
	{
		// we have a property conflict
		CTSVNPath prej(directory);
		if ((stat.status->entry)&&(stat.status->entry->prejfile))
		{
			prej.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->prejfile));
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

	if (stat.status->tree_conflict)
	{
		// we have a tree conflict
		SVNInfo info;
		const SVNInfoData * pInfoData = info.GetFirstFileInfo(merge, SVNRev(), SVNRev());
		if (pInfoData)
		{
			if (pInfoData->treeconflict_kind == svn_wc_conflict_kind_text)
			{
				CTSVNPath theirs(directory);
				CTSVNPath mine(directory);
				CTSVNPath base(directory);
				bool bConflictData = false;

				if (pInfoData->treeconflict_theirfile)
				{
					theirs.AppendPathString(pInfoData->treeconflict_theirfile);
					bConflictData = true;
				}
				if (pInfoData->treeconflict_basefile)
				{
					base.AppendPathString(pInfoData->treeconflict_basefile);
					bConflictData = true;
				}
				if (pInfoData->treeconflict_myfile)
				{
					mine.AppendPathString(pInfoData->treeconflict_myfile);
					bConflictData = true;
				}
				else
				{
					mine = merge;
				}
				if (bConflictData)
					bRet = !!CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(bAlternativeTool),
														base, theirs, mine, merge);
			}
			else if (pInfoData->treeconflict_kind == svn_wc_conflict_kind_tree)
			{
				CString sConflictAction;
				CString sConflictReason;
				CString sResolveTheirs;
				CString sResolveMine;
				CTSVNPath treeConflictPath = CTSVNPath(pInfoData->treeconflict_path);
				CString sItemName = treeConflictPath.GetUIFileOrDirectoryName();
				
				if (pInfoData->treeconflict_nodekind == svn_node_file)
				{
					switch (pInfoData->treeconflict_operation)
					{
					case svn_wc_operation_update:
						switch (pInfoData->treeconflict_action)
						{
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
						}
						break;
					case svn_wc_operation_switch:
						switch (pInfoData->treeconflict_action)
						{
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
						}
						break;
					case svn_wc_operation_merge:
						switch (pInfoData->treeconflict_action)
						{
						case svn_wc_conflict_action_edit:
							sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEEDIT, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_add:
							sResolveTheirs.Format(IDS_TREECONFLICT_FILEMERGEADD, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
							break;
						case svn_wc_conflict_action_delete:
							sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEDELETE, (LPCTSTR)sItemName);
							sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
							break;
						}
						break;
					}
				}
				else if (pInfoData->treeconflict_nodekind == svn_node_dir)
				{
					switch (pInfoData->treeconflict_operation)
					{
					case svn_wc_operation_update:
						switch (pInfoData->treeconflict_action)
						{
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
						}
						break;
					case svn_wc_operation_switch:
						switch (pInfoData->treeconflict_action)
						{
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
						}
						break;
					case svn_wc_operation_merge:
						switch (pInfoData->treeconflict_action)
						{
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
						}
						break;
					}
				}

				UINT uReasonID = 0;
				switch (pInfoData->treeconflict_reason)
				{ 
				case svn_wc_conflict_reason_edited:
					uReasonID = IDS_TREECONFLICT_REASON_EDITED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				case svn_wc_conflict_reason_obstructed:
					uReasonID = IDS_TREECONFLICT_REASON_OBSTRUCTED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				case svn_wc_conflict_reason_deleted:
					uReasonID = IDS_TREECONFLICT_REASON_DELETED;
					sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MARKASRESOLVED);
					break;
				case svn_wc_conflict_reason_added:
					uReasonID = IDS_TREECONFLICT_REASON_ADDED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				case svn_wc_conflict_reason_missing:
					uReasonID = IDS_TREECONFLICT_REASON_MISSING;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_REMOVEDIR : IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
					break;
				case svn_wc_conflict_reason_unversioned:
					uReasonID = IDS_TREECONFLICT_REASON_UNVERSIONED;
					sResolveMine.LoadString(pInfoData->treeconflict_nodekind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
					break;
				}
				sConflictReason.Format(uReasonID, (LPCTSTR)sConflictAction);

				CTreeConflictEditorDlg dlg;
				dlg.SetConflictInfoText(sConflictReason);
				dlg.SetResolveTexts(sResolveTheirs, sResolveMine);
				dlg.SetPath(treeConflictPath);
				dlg.SetConflictSources(stat.status->tree_conflict->src_left_version, stat.status->tree_conflict->src_right_version);
				dlg.SetConflictReason(pInfoData->treeconflict_reason);
				dlg.SetConflictAction(pInfoData->treeconflict_action);
				INT_PTR dlgRet = dlg.DoModal();
				bRet = (dlgRet != IDCANCEL);
			}
		}
	}

	return bRet;
}


