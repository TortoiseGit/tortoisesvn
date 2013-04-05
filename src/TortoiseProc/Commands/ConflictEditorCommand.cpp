// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseSVN

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
                CTSVNPath treeConflictPath = CTSVNPath(conflIt->treeconflict_path);

                CTreeConflictEditorDlg dlg;
                dlg.SetPath(treeConflictPath);
                dlg.SetConflictLeftSources(conflIt->src_left_version_url, conflIt->src_left_version_path, conflIt->src_left_version_rev, conflIt->src_left_version_kind);
                dlg.SetConflictRightSources(conflIt->src_right_version_url, conflIt->src_right_version_path, conflIt->src_right_version_rev, conflIt->src_right_version_kind);
                dlg.SetConflictReason(conflIt->treeconflict_reason);
                dlg.SetConflictAction(conflIt->treeconflict_action);
                dlg.SetConflictOperation(conflIt->treeconflict_operation);
                dlg.SetKind(conflIt->treeconflict_nodekind);
                INT_PTR dlgRet = dlg.DoModal();
                bRet = (dlgRet != IDCANCEL);
            }
            break;
        }
    }

    return bRet;
}


