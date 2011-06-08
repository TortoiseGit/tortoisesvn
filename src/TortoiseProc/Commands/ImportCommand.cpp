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
#include "ImportCommand.h"

#include "ImportDlg.h"
#include "SVNProgressDlg.h"
#include "StringUtils.h"

bool ImportCommand::Execute()
{
    bool bRet = false;
    CString msg;
    if (parser.HasKey(_T("logmsg")))
    {
        msg = parser.GetVal(_T("logmsg"));
    }
    if (parser.HasKey(_T("logmsgfile")))
    {
        CString logmsgfile = parser.GetVal(_T("logmsgfile"));
        CStringUtils::ReadStringFromTextFile(logmsgfile, msg);
    }
    CImportDlg dlg;
    dlg.m_path = cmdLinePath;
    dlg.m_sMessage = msg;
    if (parser.HasVal(_T("url")))
        dlg.m_url = parser.GetVal(_T("url"));
    if (dlg.DoModal() == IDOK)
    {
        CSVNProgressDlg progDlg;
        theApp.m_pMainWnd = &progDlg;
        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Import);
        progDlg.SetAutoClose (parser);
        DWORD dwOpts = dlg.m_bIncludeIgnored ? ProgOptIncludeIgnored : ProgOptNone;
        if (dlg.m_UseAutoprops)
            dwOpts |= ProgOptUseAutoprops;
        progDlg.SetOptions(dwOpts);
        progDlg.SetPathList(pathList);
        progDlg.SetUrl(dlg.m_url);
        progDlg.SetCommitMessage(dlg.m_sMessage);
        ProjectProperties props;
        props.ReadPropsPathList(pathList);
        progDlg.SetProjectProperties(props);
        progDlg.DoModal();
        bRet = !progDlg.DidErrorsOccur();
    }
    return bRet;
}
