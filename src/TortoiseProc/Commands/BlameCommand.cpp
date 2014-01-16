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
#include "BlameCommand.h"

#include "BlameDlg.h"
#include "Blame.h"
#include "SVN.h"
#include "SVNInfo.h"
#include "AppUtils.h"


bool BlameCommand::Execute()
{
    bool bRet = false;
    bool bShowDialog = true;
    CBlameDlg dlg;
    CString options;
    dlg.EndRev = SVNRev::REV_HEAD;
    dlg.m_path = cmdLinePath;

    if (!cmdLinePath.IsUrl() && (!parser.HasKey(L"startrev") || !parser.HasKey(L"pegrev")))
    {
        // if the file has been moved/deleted/renamed in HEAD, the default
        // range from 1 to HEAD won't work so we find the WC revision
        // of the file and use that as the default end revision
        SVNInfo info;
        const SVNInfoData * idata = info.GetFirstFileInfo(cmdLinePath, SVNRev(), SVNRev());
        if (idata)
        {
            dlg.EndRev = idata->rev;
            dlg.PegRev = idata->rev;
        }
    }

    if (parser.HasKey(L"pegrev"))
        dlg.PegRev = SVNRev(parser.GetVal(L"pegrev"));

    if (parser.HasKey(L"startrev") && parser.HasKey(L"endrev"))
    {
        bShowDialog = false;
        dlg.StartRev = parser.GetLongVal(L"startrev");
        dlg.EndRev = parser.GetLongVal(L"endrev");
        if (parser.HasKey(L"ignoreeol") || parser.HasKey(L"ignorespaces") || parser.HasKey(L"ignoreallspaces"))
        {
            options = SVN::GetOptionsString(!!parser.HasKey(L"ignoreeol"), !!parser.HasKey(L"ignorespaces"), !!parser.HasKey(L"ignoreallspaces"));
        }
    }

    if ((!bShowDialog)||(dlg.DoModal() == IDOK))
    {
        CString tempfile;
        {
            CBlame blame;
            if (bShowDialog)
                options = SVN::GetOptionsString(!!dlg.m_bIgnoreEOL, dlg.m_IgnoreSpaces);

            tempfile = blame.BlameToTempFile(cmdLinePath, dlg.StartRev, dlg.EndRev,
                cmdLinePath.IsUrl() ? SVNRev() : SVNRev::REV_WC,
                options, dlg.m_bIncludeMerge, TRUE, TRUE);
            if (tempfile.IsEmpty())
            {
                blame.ShowErrorDialog(GetExplorerHWND(), cmdLinePath);
            }
        }
        if (!tempfile.IsEmpty())
        {
            if (dlg.m_bTextView)
            {
                //open the default text editor for the result file
                bRet = !!CAppUtils::StartTextViewer(tempfile);
            }
            else
            {
                CString sVal;
                if (parser.HasVal(L"line"))
                {
                    sVal = L"/line:";
                    sVal += parser.GetVal(L"line");
                    sVal += L" ";
                }
                sVal += L"/path:\"" + cmdLinePath.GetSVNPathString() + L"\" ";

                if (bShowDialog)
                {
                    if (dlg.m_bIgnoreEOL)
                        sVal += L"/ignoreeol ";
                    switch (dlg.m_IgnoreSpaces)
                    {
                    case svn_diff_file_ignore_space_change:
                        sVal += L"/ignorespaces ";
                        break;
                    case svn_diff_file_ignore_space_all:
                        sVal += L"/ignoreallspaces ";
                    }
                }
                else
                {
                    if (parser.HasKey(L"ignoreeol"))
                        sVal += L"/ignoreeol ";
                    if (parser.HasKey(L"ignorespaces"))
                        sVal += L"/ignorespaces ";
                    if (parser.HasKey(L"ignoreallspaces"))
                        sVal += L"/ignoreallspaces ";
                }

                bRet = CAppUtils::LaunchTortoiseBlame(tempfile,
                                                      cmdLinePath.GetFileOrDirectoryName(),
                                                      sVal,
                                                      dlg.StartRev,
                                                      dlg.EndRev,
                                                      dlg.PegRev);
            }
        }
    }
    return bRet;
}
