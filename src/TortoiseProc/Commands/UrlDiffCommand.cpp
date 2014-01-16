// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011, 2013-2014 - TortoiseSVN

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
#include "stdafx.h"
#include "UrlDiffCommand.h"

#include "UrlDiffDlg.h"
#include "SVNDiff.h"

bool UrlDiffCommand::Execute()
{
    bool bRet = false;
    CUrlDiffDlg dlg;
    dlg.m_path = cmdLinePath.GetWinPathString();
    if (dlg.DoModal() == IDOK)
    {
        SVNDiff diff(NULL, GetExplorerHWND());
        diff.SetJumpLine(parser.GetLongVal(L"line"));
        bool ignoreprops = !!parser.HasKey(L"ignoreprops");
        bRet = diff.ShowCompare(cmdLinePath, SVNRev::REV_WC, CTSVNPath(dlg.m_URL), dlg.Revision, SVNRev(), ignoreprops, L"");
    }
    return bRet;
}
