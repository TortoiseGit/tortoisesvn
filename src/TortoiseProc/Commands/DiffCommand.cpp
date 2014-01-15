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
#include "DiffCommand.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "ChangedDlg.h"
#include "SVNDiff.h"
#include "SVNStatus.h"

bool DiffCommand::Execute()
{
    bool bRet = false;
    CString path2 = CPathUtils::GetLongPathname(parser.GetVal(_T("path2")));
    bool bAlternativeTool = !!parser.HasKey(_T("alternative"));
    bool bBlame = !!parser.HasKey(_T("blame"));
    bool ignoreprops = !!parser.HasKey(_T("ignoreprops"));
    if (path2.IsEmpty())
    {
        SVNDiff diff(NULL, GetExplorerHWND());
        diff.SetAlternativeTool(bAlternativeTool);
        diff.SetJumpLine(parser.GetLongVal(_T("line")));
        if ( parser.HasKey(_T("startrev")) && parser.HasKey(_T("endrev")) )
        {
            SVNRev StartRevision = SVNRev(parser.GetLongVal(_T("startrev")));
            SVNRev EndRevision = SVNRev(parser.GetLongVal(_T("endrev")));
            SVNRev pegRevision;
            if (parser.HasVal(L"pegrevision"))
                pegRevision = SVNRev(parser.GetVal(L"pegrevision"));
            CString diffoptions;
            if (parser.HasVal(L"diffoptions"))
                diffoptions = parser.GetVal(L"diffoptions");
            bRet = diff.ShowCompare(cmdLinePath, StartRevision, cmdLinePath, EndRevision, pegRevision, ignoreprops, diffoptions, false, bBlame);
        }
        else
        {
            svn_revnum_t baseRev = 0;
            if (parser.HasKey(L"unified"))
            {
                SVNRev pegRevision;
                if (parser.HasVal(L"pegrevision"))
                    pegRevision = SVNRev(parser.GetVal(L"pegrevision"));
                CString diffoptions;
                if (parser.HasVal(L"diffoptions"))
                    diffoptions = parser.GetVal(L"diffoptions");
                diff.ShowUnifiedDiff(cmdLinePath, SVNRev::REV_BASE, cmdLinePath, SVNRev::REV_WC, pegRevision, diffoptions, false, bBlame, false);
            }
            else
            {
                if (cmdLinePath.IsDirectory())
                {
                    if (!ignoreprops)
                        bRet = diff.DiffProps(cmdLinePath, SVNRev::REV_WC, SVNRev::REV_BASE, baseRev);
                    if (bRet == false)
                    {
                        CChangedDlg dlg;
                        dlg.m_pathList = CTSVNPathList(cmdLinePath);
                        dlg.DoModal();
                        bRet = true;
                    }
                }
                else
                {
                    bRet = diff.DiffFileAgainstBase(cmdLinePath, baseRev, ignoreprops);
                }
            }
        }
    }
    else
        bRet = CAppUtils::StartExtDiff(
            CTSVNPath(path2), cmdLinePath, CString(), CString(),
            CAppUtils::DiffFlags().AlternativeTool(bAlternativeTool), parser.GetLongVal(_T("line")), L"");
    return bRet;
}