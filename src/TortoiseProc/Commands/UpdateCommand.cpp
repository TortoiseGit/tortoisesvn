// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012 - TortoiseSVN

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
#include "UpdateCommand.h"

#include "UpdateDlg.h"
#include "SVNProgressDlg.h"
#include "Hooks.h"
#include "MessageBox.h"

bool UpdateCommand::Execute()
{
    CRegDWORD updateExternals(_T("Software\\TortoiseSVN\\IncludeExternals"), true);
    SVNRev rev = SVNRev(_T("HEAD"));
    int options = DWORD(updateExternals) ? 0 : ProgOptIgnoreExternals;
    svn_depth_t depth = svn_depth_unknown;
    DWORD exitcode = 0;
    CString error;
    if (pathList.GetCount() == 0)
        return false;
    ProjectProperties props;
    props.ReadPropsPathList(pathList);
    CHooks::Instance().SetProjectProperties(pathList.GetCommonRoot(), props);
    if (CHooks::Instance().StartUpdate(GetExplorerHWND(), pathList, exitcode, error))
    {
        if (exitcode)
        {
            CString temp;
            temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
            ::MessageBox(GetExplorerHWND(), temp, _T("TortoiseSVN"), MB_ICONERROR);
            return FALSE;
        }
    }
    std::map<CString,svn_depth_t> checkoutDepths;
    CSVNProgressDlg::Command cmd = CSVNProgressDlg::SVNProgress_Update;
    if ((parser.HasKey(_T("rev")))&&(!parser.HasVal(_T("rev"))))
    {
        CUpdateDlg dlg;
        if (pathList.GetCount()>0)
            dlg.m_wcPath = pathList[0];
        if (dlg.DoModal() == IDOK)
        {
            rev = dlg.Revision;
            depth = dlg.m_depth;
            if (dlg.m_bNoExternals)
                options |= ProgOptIgnoreExternals;
            else
                options &= ~ProgOptIgnoreExternals;
            if (dlg.m_bStickyDepth)
                options |= ProgOptStickyDepth;
            else
                options &= ~ProgOptStickyDepth;

            if (dlg.m_checkoutDepths.size())
            {
                checkoutDepths = dlg.m_checkoutDepths;
                cmd = CSVNProgressDlg::SVNProgress_SparseCheckout;
            }
        }
        else
            return FALSE;
    }
    else
    {
        if (parser.HasVal(_T("rev")))
            rev = SVNRev(parser.GetVal(_T("rev")));
        if (parser.HasKey(_T("nonrecursive")))
            depth = svn_depth_empty;
        if (parser.HasKey(_T("ignoreexternals")))
            options |= ProgOptIgnoreExternals;
        if (parser.HasKey(_T("updateexternals")))
            options &= ~ProgOptIgnoreExternals;
        if (parser.HasKey(_T("stickydepth")))
            options |= ProgOptStickyDepth;
    }

    CSVNProgressDlg progDlg;
    theApp.m_pMainWnd = &progDlg;
    progDlg.SetCommand(cmd);
    progDlg.SetAutoClose (parser);
    progDlg.SetOptions(options);
    progDlg.SetPathList(pathList);
    progDlg.SetRevision(rev);
    progDlg.SetProjectProperties(props);
    if (!checkoutDepths.empty())
        progDlg.SetPathDepths(checkoutDepths);
    else
        progDlg.SetDepth(depth);
    progDlg.DoModal();
    return !progDlg.DidErrorsOccur();
}