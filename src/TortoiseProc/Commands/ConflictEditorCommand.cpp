// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseSVN

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
    bool bAlternativeTool = !!parser.HasKey(L"alternative");

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
                CTSVNPath mine = CTSVNPath(conflIt->conflict_wrk);
                CTSVNPath base = CTSVNPath(conflIt->conflict_old);
                if (mine.IsEmpty())
                    mine = merge;
                bRet = !!CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(bAlternativeTool),
                                                  base, theirs, mine, merge, true, CString(), CString(), CString(), CString(), merge.GetFileOrDirectoryName());
            }
                break;
            case svn_wc_conflict_kind_property:
            {
                // we have a property conflict
                CTSVNPath prej(conflIt->prejfile);
                CEditPropConflictDlg dlg;
                dlg.SetPrejFile(prej);
                dlg.SetConflictedItem(merge);
                dlg.SetPropertyName(conflIt->propname);
                dlg.SetPropValues(conflIt->propvalue_base, conflIt->propvalue_working, conflIt->propvalue_incoming_old, conflIt->propvalue_incoming_new);
                bRet = (dlg.DoModal() != IDCANCEL);
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


