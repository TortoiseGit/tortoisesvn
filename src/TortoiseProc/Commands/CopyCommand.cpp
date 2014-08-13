// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012, 2014 - TortoiseSVN

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

bool CopyCommand::Execute()
{
    bool bRet = false;
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

    BOOL repeat = FALSE;
    CCopyDlg dlg;

    dlg.m_path = cmdLinePath;
    CString url = parser.GetVal(L"url");
    CString logmessage = msg;
    SVNRev copyRev = SVNRev::REV_HEAD;
    BOOL doSwitch = parser.HasKey(L"switchaftercopy");
    BOOL makeParents = parser.HasKey(L"makeparents");
    do
    {
        repeat = FALSE;
        dlg.m_URL = url;
        dlg.m_sLogMessage = logmessage;
        dlg.m_CopyRev = copyRev;
        dlg.m_bDoSwitch = doSwitch;
        dlg.m_bMakeParents = makeParents;
        if (dlg.DoModal() == IDOK)
        {
            theApp.m_pMainWnd = NULL;
            CSVNProgressDlg progDlg;
            progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Copy);
            progDlg.SetAutoClose (parser);
            DWORD options = dlg.m_bDoSwitch ? ProgOptSwitchAfterCopy : ProgOptNone;
            options |= dlg.m_bMakeParents ? ProgOptMakeParents : ProgOptNone;
            progDlg.SetOptions(options);
            progDlg.SetPathList(pathList);
            progDlg.SetUrl(dlg.m_URL);
            progDlg.SetCommitMessage(dlg.m_sLogMessage);
            progDlg.SetRevision(dlg.m_CopyRev);
            progDlg.SetExternals(dlg.GetExternalsToTag());
            url = dlg.m_URL;
            logmessage = dlg.m_sLogMessage;
            copyRev = dlg.m_CopyRev;
            doSwitch = dlg.m_bDoSwitch;
            makeParents = dlg.m_bMakeParents;
            progDlg.DoModal();
            CRegDWORD err = CRegDWORD(L"Software\\TortoiseSVN\\ErrorOccurred", FALSE);
            err = (DWORD)progDlg.DidErrorsOccur();
            bRet = !progDlg.DidErrorsOccur();
            repeat = progDlg.DidErrorsOccur();
            CRegDWORD bFailRepeat = CRegDWORD(L"Software\\TortoiseSVN\\CommitReopen", FALSE);
            if (DWORD(bFailRepeat) == FALSE)
                repeat = false;     // do not repeat if the user chose not to in the settings.
        }
    } while(repeat);
    return bRet;
}