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
		ATLTRACE("Flushing shell update list\n");

		UpdateShell();
		m_pathsForUpdating.Clear();
	}
}

void CShellUpdater::UpdateShell()
{
	// Tell the shell extension to purge its cache
	ATLTRACE("Setting cache invalidation event %d\n", GetTickCount());
	SetEvent(m_hInvalidationEvent);

/*
WGD - I've removed this again, because I don't like all the flickering and flashing it causes
It would be better to do this more intelligently by telling the cache what's changed, and letting
it decide if parent directory shell-updates are required

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
*/

	// We use the SVN 'notify' call-back to add items to the list
	// Because this might call-back more than once per file (for example, when committing)
	// it's possible that there may be duplicates in the list.
	// There's no point asking the shell to do more than it has to, so we remove the duplicates before
	// passing the list on
	m_pathsForUpdating.RemoveDuplicates();

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
		ATLTRACE("Shell Item Update for %ws (%d)\n", m_pathsForUpdating[nPath].GetWinPathString(), GetTickCount());
		SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSH, m_pathsForUpdating[nPath].GetWinPath(), NULL);
	}
}

void CShellUpdater::RebuildIcons()
{
	const int BUFFER_SIZE = 1024;
	TCHAR *buf = NULL;
	HKEY hRegKey = 0;
	DWORD dwRegValue;
	DWORD dwRegValueTemp;
	DWORD dwSize;
	DWORD dwResult;
	LONG lRegResult;
	stdstring sFilename;
	stdstring sRegValueName;

	lRegResult = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop\\WindowMetrics"),
		0, KEY_READ | KEY_WRITE, &hRegKey);
	if (lRegResult != ERROR_SUCCESS)
		goto Cleanup;

	buf = new TCHAR[BUFFER_SIZE];
	if(buf == NULL)
		goto Cleanup;

	// we're going to change the color depth
	sRegValueName = _T("Shell Icon BPP");

	// Read registry value
	dwSize = BUFFER_SIZE;
	lRegResult = RegQueryValueEx(hRegKey, sRegValueName.c_str(), NULL, NULL, 
		(LPBYTE) buf, &dwSize);
	if (lRegResult == ERROR_FILE_NOT_FOUND)
	{
		_tcsncpy(buf, _T("32"), BUFFER_SIZE);
	}
	else if (lRegResult != ERROR_SUCCESS)
		goto Cleanup;

	// Change registry value
	dwRegValue = _ttoi(buf);
	if (dwRegValue == 4)
	{
		dwRegValueTemp = 32;
	}
	else
	{
		dwRegValueTemp = 4;
	}

	dwSize = _sntprintf(buf, BUFFER_SIZE, _T("%d"), dwRegValueTemp) + 1; 
	lRegResult = RegSetValueEx(hRegKey, sRegValueName.c_str(), 0, REG_SZ, 
		(LPBYTE) buf, dwSize); 
	if (lRegResult != ERROR_SUCCESS)
		goto Cleanup;


	// Update all windows
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 
		0, SMTO_ABORTIFHUNG, 5000, &dwResult);

	// Reset registry value
	dwSize = _sntprintf(buf, BUFFER_SIZE, _T("%d"), dwRegValue) + 1; 
	lRegResult = RegSetValueEx(hRegKey, sRegValueName.c_str(), 0, REG_SZ, 
		(LPBYTE) buf, dwSize); 
	if(lRegResult != ERROR_SUCCESS)
		goto Cleanup;

	// Update all windows
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 
		0, SMTO_ABORTIFHUNG, 5000, &dwResult);

Cleanup:
	if (hRegKey != 0)
	{
		RegCloseKey(hRegKey);
	}
	if (buf != NULL)
	{
		delete buf;
	}
	return;

}