// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012, 2014, 2021 - TortoiseSVN

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
#include "CopyCommand.h"

#include "CopyDlg.h"
#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "TortoiseProc.h"

bool CopyCommand::Execute()
{
    bool    bRet = false;
    CString msg;
    if (parser.HasKey(L"logmsg"))
    {
        msg = parser.GetVal(L"logmsg");
    }
    if (parser.HasKey(L"logmsgfile"))
    {
        CString logmsgfile = parser.GetVal(L"logmsgfile");
        CStringUtils::ReadStringFromTextFile(logmsgfile, msg);
    }

    BOOL     repeat = FALSE;
    CCopyDlg dlg;

    dlg.m_path          = cmdLinePath;
    CString url         = parser.GetVal(L"url");
    CString logMessage  = msg;
    SVNRev  copyRev     = SVNRev::REV_HEAD;
    BOOL    doSwitch    = parser.HasKey(L"switchaftercopy");
    BOOL    makeParents = parser.HasKey(L"makeparents");
    do
    {
        repeat             = FALSE;
        dlg.m_url          = url;
        dlg.m_sLogMessage  = logMessage;
        dlg.m_copyRev      = copyRev;
        dlg.m_bDoSwitch    = doSwitch;
        dlg.m_bMakeParents = makeParents;
        if (dlg.DoModal() == IDOK)
        {
            CRegDWORD updateExternals(L"Software\\TortoiseSVN\\IncludeExternals", true);
            bool      ignoreExternals = false;
            if (dlg.m_bDoSwitch)
                ignoreExternals = static_cast<DWORD>(updateExternals) == 0;

            theApp.m_pMainWnd = nullptr;
            CSVNProgressDlg progDlg;
            progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Copy);
            progDlg.SetAutoClose(parser);
            DWORD options = dlg.m_bDoSwitch ? ProgOptSwitchAfterCopy : ProgOptNone;
            options |= dlg.m_bMakeParents ? ProgOptMakeParents : ProgOptNone;
            options |= ignoreExternals ? ProgOptIgnoreExternals : ProgOptNone;
            progDlg.SetOptions(options);
            progDlg.SetPathList(pathList);
            progDlg.SetUrl(dlg.m_url);
            progDlg.SetCommitMessage(dlg.m_sLogMessage);
            progDlg.SetRevision(dlg.m_copyRev);
            progDlg.SetExternals(dlg.GetExternalsToTag());
            url         = dlg.m_url;
            logMessage  = dlg.m_sLogMessage;
            copyRev     = dlg.m_copyRev;
            doSwitch    = dlg.m_bDoSwitch;
            makeParents = dlg.m_bMakeParents;
            progDlg.DoModal();
            CRegDWORD err(L"Software\\TortoiseSVN\\ErrorOccurred", FALSE);
            err                   = static_cast<DWORD>(progDlg.DidErrorsOccur());
            bRet                  = !progDlg.DidErrorsOccur();
            repeat                = progDlg.DidErrorsOccur();
            CRegDWORD bFailRepeat = CRegDWORD(L"Software\\TortoiseSVN\\CommitReopen", FALSE);
            if (static_cast<DWORD>(bFailRepeat) == FALSE)
                repeat = false; // do not repeat if the user chose not to in the settings.
        }
    } while (repeat);
    return bRet;
}