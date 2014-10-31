// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2013-2014 - TortoiseSVN

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
#include "LogCommand.h"
#include "LogDialog/LogDlg.h"
#include "StringUtils.h"

bool LogCommand::Execute()
{
    //the log command line looks like this:
    //command:log path:<path_to_file_or_directory_to_show_the_log_messages> [startrev:<startrevision>] [endrev:<endrevision>]
    CString val = parser.GetVal(L"startrev");
    if ( val.IsEmpty() )
    {
        // support deprecated parameter prior 1.5.0
        val = parser.GetVal(L"revstart");
    }
    SVNRev revstart = val.IsEmpty() ? SVNRev() : SVNRev(val);
    val = parser.GetVal(L"endrev");
    if ( val.IsEmpty() )
    {
        // support deprecated parameter prior 1.5.0
        val = parser.GetVal(L"revend");
    }
    SVNRev revend = val.IsEmpty() ? SVNRev() : SVNRev(val);
    val = parser.GetVal(L"limit");
    int limit = _tstoi(val);
    val = parser.GetVal(L"pegrev");
    if ( val.IsEmpty() )
    {
        // support deprecated parameter prior 1.5.0
        val = parser.GetVal(L"revpeg");
    }
    SVNRev pegrev = val.IsEmpty() ? SVNRev() : SVNRev(val);
    if (!revstart.IsValid())
        revstart = SVNRev::REV_HEAD;
    if (!revend.IsValid())
        revend = 0;

    if (limit == 0)
    {
        CRegDWORD reg = CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100);
        limit = (int)(LONG)reg;
    }
    BOOL bStrict = (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LastLogStrict", FALSE);
    if (parser.HasKey(L"strict"))
    {
        bStrict = TRUE;
    }
    CString findStr = parser.GetVal(L"findstring");
    LONG findType = parser.GetLongVal(L"findtype");
    bool findRegex = !!CRegDWORD(L"Software\\TortoiseSVN\\UseRegexFilter", FALSE);
    if (parser.HasKey(L"findtext"))
        findRegex = false;
    if (parser.HasKey(L"findregex"))
        findRegex = true;
    CString sFilterDateFrom;
    CString sFilterDateTo;
    if (parser.HasVal(L"datemin"))
        sFilterDateFrom = parser.GetVal(L"datemin");
    if (parser.HasVal(L"datemax"))
        sFilterDateTo = parser.GetVal(L"datemax");

    CLogDlg dlg;
    theApp.m_pMainWnd = &dlg;
    dlg.SetParams(cmdLinePath, pegrev, revstart, revend, bStrict, TRUE, limit);
    dlg.SetFilter(findStr, findType, findRegex, sFilterDateFrom, sFilterDateTo);
    dlg.SetIncludeMerge(!!parser.HasKey(L"merge"));
    val = parser.GetVal(L"propspath");
    if (!val.IsEmpty())
        dlg.SetProjectPropertiesPath(CTSVNPath(val));
    if (parser.HasVal(L"outfile"))
        dlg.SetSelect(true);
    dlg.DoModal();
    if (parser.HasVal(L"outfile"))
    {
        CString sText = dlg.GetSelectedRevRanges().ToListString();
        CStringUtils::WriteStringToTextFile(parser.GetVal(L"outfile"), (LPCTSTR)sText, true);
    }
    return true;
}
