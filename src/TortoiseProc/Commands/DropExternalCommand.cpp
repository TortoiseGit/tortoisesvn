// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014 - TortoiseSVN

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
#include "DropExternalCommand.h"
#include "SVN.h"
#include "SVNProperties.h"
#include "SVNStatus.h"

bool DropExternalCommand::Execute()
{
    bool bSuccess = false;
    CString droppath = parser.GetVal(L"droptarget");
    CTSVNPath droptsvnpath = CTSVNPath(droppath);
    if (droptsvnpath.IsAdminDir())
        droptsvnpath = droptsvnpath.GetDirectory();
    if (!droptsvnpath.IsDirectory())
        droptsvnpath = droptsvnpath.GetDirectory();

    // first get the svn:externals property from the target folder
    SVNProperties props(droptsvnpath, SVNRev::REV_WC, false, false);
    std::string sExternalsValue;
    for (int i = 0; i < props.GetCount(); ++i)
    {
        if (props.GetItemName(i).compare(SVN_PROP_EXTERNALS) == 0)
        {
            sExternalsValue = props.GetItemValue(i);
        }
    }

    // we don't add admin dirs as externals
    pathList.RemoveAdminPaths();
    if (pathList.GetCount() == 0)
        return bSuccess;

    SVNStatus status;
    status.GetStatus(droptsvnpath);
    CString sTargetRepoRootUrl;
    if (status.status && status.status->repos_root_url)
    {
        sTargetRepoRootUrl = CUnicodeUtils::GetUnicode(status.status->repos_root_url);
    }
    if (sTargetRepoRootUrl.IsEmpty())
    {
        // failed to get the status and/or the repo root url
        // TODO: show error dialog
    }
    SVN svn;
    for (auto i = 0; i < pathList.GetCount(); ++i)
    {
        CTSVNPath destPath = droptsvnpath;
        destPath.AppendPathString(pathList[i].GetFileOrDirectoryName());
        if (CopyFile(pathList[i].GetWinPath(), destPath.GetWinPath(), TRUE))
        {
            SVNStatus sourceStatus;
            sourceStatus.GetStatus(pathList[i]);
            if (sourceStatus.status && sourceStatus.status->repos_root_url)
            {
                CString sExternalRootUrl = CUnicodeUtils::GetUnicode(sourceStatus.status->repos_root_url);
                CString sExternalUrl = svn.GetURLFromPath(pathList[i]);
                CString sExtValue = sExternalUrl + L" " + pathList[i].GetFileOrDirectoryName();
                // check if the url is from the same repo as the target, and if it is
                // use a relative external url instead of a full url
                if (sTargetRepoRootUrl.Compare(sExternalRootUrl) == 0)
                {
                    sExtValue = L"^" + sExternalUrl.Mid(sTargetRepoRootUrl.GetLength()) + L" " + pathList[i].GetFileOrDirectoryName();
                }
                if (!sExternalsValue.empty())
                    sExternalsValue += "\n";
                sExternalsValue += CUnicodeUtils::StdGetUTF8((LPCWSTR)sExtValue);
                bSuccess = true;
            }
        }
        else
        {
            // the file already exists, there can't be an external with the
            // same name.

            // TODO: show error dialog
            bSuccess = false;
        }
    }
    if (bSuccess)
    {
        bSuccess = !!props.Add(SVN_PROP_EXTERNALS, sExternalsValue, true);
    }

    if (!bSuccess)
    {
        // adding the svn:externals property failed, remove all the copied files

        props.ShowErrorDialog(GetExplorerHWND());
    }
    // TODO: show a dialog asking whether to update the target
    // folder so that the externals get properly added

    return bSuccess != false;
}
