// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

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
#include "CleanupCommand.h"

#include "MessageBox.h"
#include "ProgressDlg.h"
#include "SVN.h"
#include "SVNGlobal.h"
#include "SVNAdminDir.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

bool CleanupCommand::Execute()
{
	bool bRet = false;
	CProgressDlg progress;
	CString tmp;
	progress.SetTitle(IDS_PROC_CLEANUP);
	progress.SetAnimation(IDR_CLEANUPANI);
	progress.SetShowProgressBar(false);
	tmp.Format(IDS_PROC_CLEANUP_INFO1, _T(""));
	progress.SetLine(1, tmp);
	progress.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO2)));
	progress.ShowModeless(hwndExplorer);
	
	CString strSuccessfullPaths, strFailedPaths;
	for (int i=0; i<pathList.GetCount(); ++i)
	{
		SVN svn;
		tmp.Format(IDS_PROC_CLEANUP_INFO1, (LPCTSTR)pathList[i].GetFileOrDirectoryName(), true);
		progress.SetLine(1, tmp);
		if (!svn.CleanUp(pathList[i]))
		{
			strFailedPaths += _T("- ") + pathList[i].GetWinPathString() + _T("\n");
			strFailedPaths += svn.GetLastErrorMessage() + _T("\n\n");
		}
		else
		{
			strSuccessfullPaths += _T("- ") + pathList[i].GetWinPathString() + _T("\n");

			// after the cleanup has finished, crawl the path downwards and send a change
			// notification for every directory to the shell. This will update the
			// overlays in the left tree view of the explorer.
			CDirFileEnum crawler(pathList[i].GetWinPathString());
			CString sPath;
			bool bDir = false;
			CTSVNPathList updateList;
			while (crawler.NextFile(sPath, &bDir))
			{
				if ((bDir) && (!g_SVNAdminDir.IsAdminDirPath(sPath)))
				{
					updateList.AddPath(CTSVNPath(sPath));
				}
			}
			updateList.AddPath(pathList[i]);
			CShellUpdater::Instance().AddPathsForUpdate(updateList);
			CShellUpdater::Instance().Flush();
			updateList.SortByPathname(true);
			for (INT_PTR j=0; j<updateList.GetCount(); ++j)
			{
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, updateList[j].GetWinPath(), NULL);
				ATLTRACE(_T("notify change for path %s\n"), updateList[j].GetWinPath());
			}
		}
	}
	progress.Stop();
	
	CString strMessage;
	if ( !strSuccessfullPaths.IsEmpty() )
	{
		tmp.Format(IDS_PROC_CLEANUPFINISHED, (LPCTSTR)strSuccessfullPaths);
		strMessage += tmp;
		bRet = true;
	}
	if ( !strFailedPaths.IsEmpty() )
	{
		if (!strMessage.IsEmpty())
			strMessage += _T("\n");
		tmp.Format(IDS_PROC_CLEANUPFINISHED_FAILED, (LPCTSTR)strFailedPaths);
		strMessage += tmp;
		bRet = false;
	}
	if (!parser.HasKey(_T("noui")))
		CMessageBox::Show(hwndExplorer, strMessage, _T("TortoiseSVN"), MB_OK | (strFailedPaths.IsEmpty()?MB_ICONINFORMATION:MB_ICONERROR));

	return bRet;
}
