// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "StdAfx.h"
#include "Shellupdater.h"
#include "MessageBox.h"
#include "TortoiseProc.h"
#include "Registry.h"

CShellUpdater::CShellUpdater(void)
{
	m_hInvalidationEvent = CreateEvent(NULL, FALSE, FALSE, _T("TortoiseSVNCacheInvalidationEvent"));
}

CShellUpdater::~CShellUpdater(void)
{
	Flush();

	CloseHandle(m_hInvalidationEvent);
}

CShellUpdater& CShellUpdater::Instance()
{
	static CShellUpdater instance;
	return instance;
}

/** 
* Add a single path for updating.
* The update will happen at some suitable time in the future
*/
void CShellUpdater::AddPathForUpdate(const CTSVNPath& path)
{
	// Tell the shell extension to purge its cache - we'll redo this when 
	// we actually do the shell-updates, but sometimes there's an earlier update, which
	// might benefit from cache invalidation
	SetEvent(m_hInvalidationEvent);

	m_pathsForUpdating.AddPath(path);
}
/** 
* Add a list of paths for updating.
* The update will happen when the list is destroyed, at the end of excecution
*/
void CShellUpdater::AddPathsForUpdate(const CTSVNPathList& pathList)
{
	for(int nPath=0; nPath < pathList.GetCount(); nPath++)
	{
		AddPathForUpdate(pathList[nPath]);
	}
}

void CShellUpdater::Flush()
{
	if(m_pathsForUpdating.GetCount() > 0)
	{
		TRACE("Flushing shell update list\n");

		UpdateShell();
		m_pathsForUpdating.Clear();
	}
}

void CShellUpdater::UpdateShell()
{
	// Tell the shell extension to purge its cache
	TRACE("Setting cache invalidation event %d\n", GetTickCount());
	SetEvent(m_hInvalidationEvent);

	// For each item, we also add each of its parents
	// This is rather inefficient, and almost all of them will be discarded by the 
	// de-duplication, but it's a simple and reliable way of ensuring that updates occur all 
	// the way up the hierarchy, which is important if recursive status is in use
	int nPaths = m_pathsForUpdating.GetCount();
	for(int nPath = 0; nPath < nPaths; nPath++)
	{
		CTSVNPath parentDir = m_pathsForUpdating[nPath].GetContainingDirectory();
		while(!parentDir.IsEmpty())
		{
			m_pathsForUpdating.AddPath(parentDir);
			parentDir = parentDir.GetContainingDirectory();
		}
	}

	// We use the SVN 'notify' call-back to add items to the list
	// Because this might call-back more than once per file (for example, when committing)
	// it's possible that there may be duplicates in the list.
	// There's no point asking the shell to do more than it has to, so we remove the duplicates before
	// passing the list on
	m_pathsForUpdating.RemoveDuplicates();

	DWORD brute = CRegDWORD(_T("Software\\TortoiseSVN\\ForceShellUpdate"), 0);
	if (brute)
	{
		//this method actually works, i.e. the icon overlays are updated as they
		//should. The problem is that _every_ icon is refreshed, even those
		//located on a server share. And this can block the explorer for about 5
		//seconds on my computer in the office. So use this function only if the
		//user requests it!
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSHNOWAIT, 0, 0);
	}
	else
	{
		//updating the left pane (tree view) of the explorer
		//is more difficult (if not impossible) than I thought.
		//Using SHChangeNotify() doesn't work at all. I found that
		//the shell receives the message, but then checks the files/folders
		//itself for changes. And since the folders which are shown
		//in the tree view haven't changed the icon-overlay is
		//not updated!
		//a workaround for this problem would be if this method would
		//rename the folders, do a SHChangeNotify(SHCNE_RMDIR, ...),
		//rename the folders back and do an SHChangeNotify(SHCNE_UPDATEDIR, ...)
		//
		//But I'm not sure if that is really a good workaround - it'll possibly
		//slows down the explorer and also causes more HD usage.
		//
		//So this method only updates the files and folders in the normal
		//explorer view by telling the explorer that the folder icon itself
		//has changed.

		for(int nPath = 0; nPath < m_pathsForUpdating.GetCount(); nPath++)
		{
			TRACE("Shell Item Update for %ws (%d)\n", m_pathsForUpdating[nPath].GetWinPathString(), GetTickCount());
			SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSH, m_pathsForUpdating[nPath].GetWinPath(), NULL);
		}
	}
}
