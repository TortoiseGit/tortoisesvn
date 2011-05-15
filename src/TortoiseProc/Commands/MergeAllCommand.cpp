// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "MergeAllCommand.h"
#include "MergeAllDlg.h"
#include "SVNProgressDlg.h"

bool MergeAllCommand::Execute()
{
    CMergeAllDlg dlg;
    dlg.m_pathList = pathList;
    if (dlg.DoModal() == IDOK)
    {
        CSVNProgressDlg progDlg;
        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_MergeAll);
        progDlg.SetAutoClose (parser);
        progDlg.SetPathList(pathList);
        progDlg.SetDepth(dlg.m_depth);
        progDlg.SetDiffOptions(SVN::GetOptionsString(!!dlg.m_bIgnoreEOL, !!dlg.m_IgnoreSpaces));
        int options = dlg.m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
        options |= dlg.m_bForce ? ProgOptForce : 0;
        progDlg.SetOptions(options);

        SVNRevRangeArray tempRevArray;
        tempRevArray.AddRevRange(1, SVNRev::REV_HEAD);
        progDlg.SetRevisionRanges(tempRevArray);

        progDlg.DoModal();
        return !progDlg.DidErrorsOccur();
    }

    return false;
}