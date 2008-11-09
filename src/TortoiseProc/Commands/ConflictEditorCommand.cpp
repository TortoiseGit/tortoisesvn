// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "AppUtils.h"
#include "EditPropConflictDlg.h"
#include "TreeConflictEditorDlg.h"

bool ConflictEditorCommand::Execute()
{
	CTSVNPath merge = cmdLinePath;
	CTSVNPath directory = merge.GetDirectory();
	bool bRet = false;

	// we have the conflicted file (%merged)
	// now look for the other required files
	SVNStatus stat;
	stat.GetStatus(merge);
	if ((stat.status == NULL)||(stat.status->entry == NULL))
		return false;

	if (stat.status->text_status == svn_wc_status_conflicted)
	{
		// we have a text conflict, use our merge tool to resolve the conflict

		CTSVNPath theirs(directory);
		CTSVNPath mine(directory);
		CTSVNPath base(directory);
		bool bConflictData = false;

		if (stat.status->entry->conflict_new)
		{
			theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
			bConflictData = true;
		}
		if (stat.status->entry->conflict_old)
		{
			base.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old));
			bConflictData = true;
		}
		if (stat.status->entry->conflict_wrk)
		{
			mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
			bConflictData = true;
		}
		else
		{
			mine = merge;
		}
		if (bConflictData)
			bRet = !!CAppUtils::StartExtMerge(base,theirs,mine,merge);
	}

	if (stat.status->prop_status == svn_wc_status_conflicted)
	{
		// we have a property conflict
		CTSVNPath prej(directory);
		if (stat.status->entry->prejfile)
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

	if (stat.status->tree_conflicted)
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
					bRet = !!CAppUtils::StartExtMerge(base,theirs,mine,merge);
			}
			else if (pInfoData->treeconflict_kind == svn_wc_conflict_kind_tree)
			{
				CString sOperation;
				CString sFileOrFolder;
				CString sConflictAction;
				CString sConflictReason;
				CString sResolveTheirs;
				CString sResolveMine;
				
				if (pInfoData->treeconflict_nodekind == svn_node_file)
					sFileOrFolder.LoadString(IDS_TREECONFLICT_NODEFILE);
				else if (pInfoData->treeconflict_nodekind == svn_node_dir)
					sFileOrFolder.LoadString(IDS_TREECONFLICT_NODEDIR);
				else
					// we should *never* get here, but if we do, provide some info
					// for users so they might report this
					sFileOrFolder = _T("(don't know if it's a file or directory)");

				switch (pInfoData->treeconflict_operation)
				{
				case svn_wc_operation_update:
					sOperation.LoadString(IDS_TREECONFLICT_OPERATION_UPDATE);
					break;
				case svn_wc_operation_switch:
					sOperation.LoadString(IDS_TREECONFLICT_OPERATION_SWITCH);
					break;
				case svn_wc_operation_merge:
					sOperation.LoadString(IDS_TREECONFLICT_OPERATION_MERGE);
					break;
				}

				switch (pInfoData->treeconflict_action)
				{
				case svn_wc_conflict_action_edit:
					sConflictAction.LoadString(IDS_TREECONFLICT_ACTION_EDIT);
					sResolveTheirs.Format(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORY, (LPCTSTR)sFileOrFolder);
					break;
				case svn_wc_conflict_action_add:
					sConflictAction.LoadString(IDS_TREECONFLICT_ACTION_ADD);
					sResolveTheirs.Format(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORY, (LPCTSTR)sFileOrFolder);
					break;
				case svn_wc_conflict_action_delete:
					sConflictAction.LoadString(IDS_TREECONFLICT_ACTION_DELETE);
					sResolveTheirs.Format(IDS_TREECONFLICT_RESOLVE_REMOVE, (LPCTSTR)sFileOrFolder);
					break;
				}

				UINT uReasonID = 0;
				switch (pInfoData->treeconflict_reason)
				{ 
				case svn_wc_conflict_reason_edited:
					uReasonID = IDS_TREECONFLICT_REASON_EDITED;
					sResolveMine.Format(IDS_TREECONFLICT_RESOLVE_KEEPLOCAL, (LPCTSTR)sFileOrFolder); // keep local file/directory
					break;
				case svn_wc_conflict_reason_obstructed:
					uReasonID = IDS_TREECONFLICT_REASON_OBSTRUCTED;
					sResolveMine.Format(IDS_TREECONFLICT_RESOLVE_KEEPLOCAL, (LPCTSTR)sFileOrFolder);
					break;
				case svn_wc_conflict_reason_deleted:
					uReasonID = IDS_TREECONFLICT_REASON_DELETED;
					sResolveMine.Format(IDS_TREECONFLICT_RESOLVE_REMOVE), (LPCTSTR)sFileOrFolder;
					break;
				case svn_wc_conflict_reason_added:
					uReasonID = IDS_TREECONFLICT_REASON_ADDED;
					sResolveMine.Format(IDS_TREECONFLICT_RESOLVE_KEEPLOCAL, (LPCTSTR)sFileOrFolder);
					break;
				case svn_wc_conflict_reason_missing:
					uReasonID = IDS_TREECONFLICT_REASON_MISSING;
					sResolveMine.Format(IDS_TREECONFLICT_RESOLVE_REMOVE), (LPCTSTR)sFileOrFolder;
					break;
				case svn_wc_conflict_reason_unversioned:
					uReasonID = IDS_TREECONFLICT_REASON_UNVERSIONED;
					sResolveMine.Format(IDS_TREECONFLICT_RESOLVE_KEEPLOCAL, (LPCTSTR)sFileOrFolder);
					break;
				}
				// The last %s operation tried to %s the %s '%s', but ....
				sConflictReason.Format(uReasonID, (LPCTSTR)sOperation, (LPCTSTR)sConflictAction, (LPCTSTR)sFileOrFolder, (LPCTSTR)pInfoData->treeconflict_path);

				CTreeConflictEditorDlg dlg;
				dlg.SetConflictInfoText(sConflictReason);
				dlg.SetResolveTexts(sResolveTheirs, sResolveMine);
				dlg.SetPath(CTSVNPath(pInfoData->treeconflict_path));
				bRet = (dlg.DoModal() != IDCANCEL);
			}
		}
	}

	return bRet;
}


