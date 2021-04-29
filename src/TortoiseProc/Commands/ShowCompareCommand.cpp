// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2013-2014, 2018, 2021 - TortoiseSVN

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
#include "ShowCompareCommand.h"
#include "SVNDiff.h"

bool ShowCompareCommand::Execute()
{
    bool    bRet = false;
    SVNDiff diff(nullptr, GetExplorerHWND());

    SVNRev          rev1;
    SVNRev          rev2;
    SVNRev          pegRev;
    SVNRev          headPeg;
    CString         diffOptions;
    svn_node_kind_t nodeKind = svn_node_unknown;

    CTSVNPath url1           = CTSVNPath(parser.GetVal(L"url1"));
    CTSVNPath url2           = CTSVNPath(parser.GetVal(L"url2"));
    bool      ignoreAncestry = !!parser.HasKey(L"ignoreancestry");
    bool      blame          = !!parser.HasKey(L"blame");
    bool      unified        = !!parser.HasKey(L"unified");
    bool      ignoreProps    = !!parser.HasKey(L"ignoreprops");
    bool      prettyPrint    = !!parser.HasKey(L"prettyprint");

    if (parser.HasVal(L"revision1"))
        rev1 = SVNRev(parser.GetVal(L"revision1"));
    if (parser.HasVal(L"revision2"))
        rev2 = SVNRev(parser.GetVal(L"revision2"));
    if (parser.HasVal(L"pegrevision"))
        pegRev = SVNRev(parser.GetVal(L"pegrevision"));
    if (parser.HasVal(L"headpegrevision"))
        diff.SetHEADPeg(SVNRev(parser.GetVal(L"headpegrevision")));
    if (parser.HasVal(L"diffoptions"))
        diffOptions = parser.GetVal(L"diffoptions");
    diff.SetAlternativeTool(!!parser.HasKey(L"alternatediff"));
    if (parser.HasVal(L"nodekind"))
        nodeKind = static_cast<svn_node_kind_t>(parser.GetLongVal(L"nodekind"));
    diff.SetJumpLine(parser.GetLongVal(L"line"));

    if (unified)
        bRet = diff.ShowUnifiedDiff(url1, rev1, url2, rev2, pegRev, prettyPrint, diffOptions, ignoreAncestry, blame, ignoreProps);
    else
        bRet = diff.ShowCompare(url1, rev1, url2, rev2, pegRev, ignoreProps, prettyPrint, diffOptions, ignoreAncestry, blame, nodeKind);

    return bRet;
}
