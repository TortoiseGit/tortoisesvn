// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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
#include "DropVendorCommand.h"
#include "SVNStatus.h"
#include "DirFileEnum.h"
#include "SVN.h"
#include "SVNAdminDir.h"
#include "ProgressDlg.h"
#include "FormatMessageWrapper.h"

bool DropVendorCommand::Execute()
{
    CString droppath = parser.GetVal(_T("droptarget"));
    CTSVNPath droptsvnpath = CTSVNPath(droppath);
    if (droptsvnpath.IsAdminDir())
        return FALSE;
    CString sAsk;
    sAsk.Format(IDS_PROC_VENDORDROP_CONFIRM, (LPCWSTR)droptsvnpath.GetFileOrDirectoryName());
    if (MessageBox(GetExplorerHWND(), sAsk, L"TortoiseSVN", MB_YESNO|MB_ICONQUESTION) != IDYES)
        return FALSE;

    CProgressDlg progress;
    progress.SetTitle(IDS_PROC_VENDORDROP_TITLE);
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_VENDORDROP_GETDATA1)));
    progress.SetAnimation(IDR_MOVEANI);
    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));

    std::map<CString,bool> versionedFiles;
    CTSVNPath path;
    SVNStatus st;
    svn_client_status_t * status = st.GetFirstFileStatus(droptsvnpath, path, false, svn_depth_infinity, true, true);
    if (status)
    {
        while (((status = st.GetNextFileStatus(path))!=NULL) && (!progress.HasUserCancelled()))
            versionedFiles[path.GetWinPathString().Mid(droppath.GetLength()+1)] = status->kind == svn_node_dir;
    }

    pathList.RemoveAdminPaths();
    std::map<CString,bool> vendorFiles;
    CTSVNPath root = pathList.GetCommonRoot();
    int rootlen = root.GetWinPathString().GetLength()+1;
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_VENDORDROP_GETDATA2)));
    for(int nPath = 0; (nPath < pathList.GetCount()) && (!progress.HasUserCancelled()); nPath++)
    {
        CDirFileEnum crawler(pathList[nPath].GetWinPathString());
        CString sPath;
        bool bDir = false;
        while (crawler.NextFile(sPath, &bDir, true))
        {
            if (!g_SVNAdminDir.IsAdminDirPath(sPath))
                vendorFiles[sPath.Mid(rootlen)] = bDir;
        }
    }

    // now start copying the vendor files over the versioned files
    ProjectProperties projectproperties;
    projectproperties.ReadPropsPathList(pathList);
    SVN svn;
    progress.SetTime(true);
    progress.SetProgress(0, (DWORD)vendorFiles.size());
    DWORD count = 0;
    BOOL bSuccess = FALSE;
    for (auto it = vendorFiles.cbegin(); (it != vendorFiles.cend()) && (!progress.HasUserCancelled()); ++it)
    {
        CString srcPath = root.GetWinPathString() + L"\\" + it->first;
        CString dstPath = droppath + L"\\" + it->first;

        progress.FormatPathLine(1, IDS_PROC_COPYINGPROG, (LPCWSTR)srcPath);
        progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, (LPCWSTR)dstPath);
        progress.SetProgress(++count, (DWORD)vendorFiles.size());

        auto found = versionedFiles.find(it->first);
        if (found != versionedFiles.end())
        {
            versionedFiles.erase(found);
            if (!it->second)
            {
                bSuccess = CopyFile(srcPath, dstPath, FALSE);
            }
        }
        else
        {
            if (PathFileExists(dstPath))
            {
                // case rename!
                for (auto v = versionedFiles.begin(); v != versionedFiles.end(); ++v)
                {
                    if (v->first.CompareNoCase(it->first) == 0)
                    {
                        CString dstsrc = droppath + L"\\" + v->first;
                        svn.Move(CTSVNPathList(CTSVNPath(dstsrc)), CTSVNPath(dstPath));
                        bSuccess = CopyFile(srcPath, dstPath, FALSE);
                        versionedFiles.erase(v);
                        break;
                    }
                }
            }
            else
            {
                CTSVNPathList plist = CTSVNPathList(CTSVNPath(dstPath));
                if (!it->second)
                    bSuccess = CopyFile(srcPath, dstPath,  FALSE);
                else
                    bSuccess = CreateDirectory(dstPath, NULL);
                svn.Add(plist, &projectproperties, svn_depth_infinity, true, true, true, true);
            }
        }
    }

    // now go through all files still in the versionedFiles map
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_VENDORDROP_REMOVE)));
    progress.SetLine(2, L"");
    progress.SetProgress(0, 0);
    for (auto it = versionedFiles.cbegin(); (it != versionedFiles.cend()) && (!progress.HasUserCancelled()); ++it)
    {
        // first move the file to the recycle bin so it's possible to retrieve it later
        // again in case removing it was done accidentally
        CTSVNPath delpath = CTSVNPath(droppath + L"\\" + it->first);
        delpath.Delete(true);
        svn.Remove(CTSVNPathList(delpath), true, false);
    }

    return bSuccess != false;
}
