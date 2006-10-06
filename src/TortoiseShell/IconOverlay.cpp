// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "stdafx.h"
#include "ShellExt.h"
#include "Guids.h"
#include "PreserveChdir.h"
#include "UnicodeUtils.h"
#include "SVNStatus.h"
#include "..\TSVNCache\CacheInterface.h"

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."

STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	PreserveChdir preserveChdir;

    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if( !GetVersionEx(&osv) )
    {
        return S_FALSE;
    }
    // Under NT (and 95?), file open dialog crashes apps upon shutdown,
    // so we disable our icon overlays unless we are in Explorer process
    // space.
    bool bAllowOverlayInFileDialogs = osv.dwMajorVersion > 4 || // allow anything major > 4
        (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
        osv.dwMajorVersion == 4 && osv.dwMinorVersion > 0); // plus Windows 98/Me
	
	// There is also a user-option to suppress this
	if(bAllowOverlayInFileDialogs && CRegStdWORD(_T("Software\\TortoiseSVN\\OverlaysOnlyInExplorer"), FALSE))
	{
		bAllowOverlayInFileDialogs = false;
	}

	if(!bAllowOverlayInFileDialogs)
	{
		// Test if we are in Explorer
		DWORD modpathlen = 0;
		TCHAR * buf = NULL;
		DWORD pathLength = 0;
		do 
		{
			modpathlen += MAX_PATH;		// MAX_PATH is not the limit here!
			if (buf)
				delete buf;
			buf = new TCHAR[modpathlen];
			pathLength = GetModuleFileName(NULL, buf, modpathlen);
		} while (pathLength == modpathlen);
		if(pathLength >= 13)
		{
			if ((_tcsicmp(&buf[pathLength-13], _T("\\explorer.exe"))) != 0)
			{
				delete buf;
				return S_FALSE;
			}
		}
		delete buf;
	}

	int nInstalledOverlays = GetInstalledOverlays();
	
	if ((m_State == FileStateAddedOverlay)&&(nInstalledOverlays > 12))
		return S_FALSE;		// don't use the 'added' overlay
	if ((m_State == FileStateLockedOverlay)&&(nInstalledOverlays > 13))
		return S_FALSE;		// don't show the 'locked' overlay

    // Get folder icons from registry
	// Default icons are stored in LOCAL MACHINE
	// User selected icons are stored in CURRENT USER
	TCHAR regVal[1024];
	DWORD len = 1024;

	stdstring icon;
	stdstring iconName;

	HKEY hkeys [] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
	switch (m_State)
	{
		case FileStateVersioned		: iconName = _T("InSubversionIcon"); break;
		case FileStateModified		: iconName = _T("ModifiedIcon"); break;
		case FileStateConflict		: iconName = _T("ConflictIcon"); break;
		case FileStateDeleted		: iconName = _T("DeletedIcon"); break;
		case FileStateReadOnly		: iconName = _T("ReadOnlyIcon"); break;
		case FileStateLockedOverlay	: iconName = _T("LockedIcon"); break;
		case FileStateAddedOverlay	: iconName = _T("AddedIcon"); break;
	}

	for (int i = 0; i < 2; ++i)
	{
		HKEY hkey = 0;

		if (::RegOpenKeyEx (hkeys[i],
			_T("Software\\TortoiseSVN"),
                    0,
                    KEY_QUERY_VALUE,
                    &hkey) != ERROR_SUCCESS)
			continue;

		if (icon.empty() == true
			&& (::RegQueryValueEx (hkey,
							 iconName.c_str(),
							 NULL,
							 NULL,
							 (LPBYTE) regVal,
							 &len)) == ERROR_SUCCESS)
			icon.assign (regVal, len);

		::RegCloseKey(hkey);

	}

    // Add name of appropriate icon
    if (icon.empty() == false)
        wcsncpy_s (pwszIconFile, cchMax, icon.c_str(), cchMax);
    else
        return S_FALSE;

	// Now here's where we can find out if due to lack of enough overlay
	// slots some of our overlays won't be shown.
	// To do that we have to mark every overlay handler that's successfully
	// loaded, so we can later check if some are missing
	switch (m_State)
	{
		case FileStateVersioned		: g_normalovlloaded = true; break;
		case FileStateModified		: g_modifiedovlloaded = true; break;
		case FileStateConflict		: g_conflictedovlloaded = true; break;
		case FileStateDeleted		: g_deletedovlloaded = true; break;
		case FileStateReadOnly		: g_readonlyovlloaded = true; break;
		case FileStateLockedOverlay	: g_lockedovlloaded = true; break;
		case FileStateAddedOverlay	: g_addedovlloaded = true; break;
	}

	ATLTRACE2(_T("Icon loaded : %s\n"), icon.c_str());
    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE;
    return S_OK;
};

STDMETHODIMP CShellExt::GetPriority(int *pPriority)
{
	switch (m_State)
	{
		case FileStateConflict:
			*pPriority = 0;
			break;
		case FileStateModified:
			*pPriority = 1;
			break;
		case FileStateDeleted:
			*pPriority = 2;
			break;
		case FileStateReadOnly:
			*pPriority = 3;
			break;
		case FileStateLockedOverlay:
			*pPriority = 4;
			break;
		case FileStateAddedOverlay:
			*pPriority = 5;
			break;
		case FileStateVersioned:
			*pPriority = 6;
			break;
		default:
			*pPriority = 100;
			return S_FALSE;
	}
    return S_OK;
}

// "Before painting an object's icon, the Shell passes the object's name to
//  each icon overlay handler's IShellIconOverlayIdentifier::IsMemberOf
//  method. If a handler wants to have its icon overlay displayed,
//  it returns S_OK. The Shell then calls the handler's
//  IShellIconOverlayIdentifier::GetOverlayInfo method to determine which icon
//  to display."

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD /*dwAttrib*/)
{
	PreserveChdir preserveChdir;
	svn_wc_status_kind status = svn_wc_status_unversioned;
	bool readonlyoverlay = false;
	bool lockedoverlay = false;
	if (pwszPath == NULL)
		return S_FALSE;
	const TCHAR* pPath = pwszPath;

	// the shell sometimes asks overlays for invalid paths, e.g. for network
	// printers (in that case the path is "0", at least for me here).
	if (_tcslen(pPath)<2)
		return S_FALSE;
	// since the shell calls each and every overlay handler with the same filepath
	// we use a small 'fast' cache of just one path here.
	// To make sure that cache expires, clear it as soon as one handler is used.
	if (_tcscmp(pPath, g_filepath.c_str())==0)
	{
		status = g_filestatus;
		readonlyoverlay = g_readonlyoverlay;
		lockedoverlay = g_lockedoverlay;
	}
	else
	{
		if (!g_ShellCache.IsPathAllowed(pPath))
		{
			return S_FALSE;
		}

		switch (g_ShellCache.GetCacheType())
		{
		case ShellCache::exe:
			{
				TSVNCacheResponse itemStatus;
				ZeroMemory(&itemStatus, sizeof(itemStatus));
				if (g_remoteCacheLink.GetStatusFromRemoteCache(CTSVNPath(pPath), &itemStatus, true))
				{
					status = SVNStatus::GetMoreImportant(itemStatus.m_status.text_status, itemStatus.m_status.prop_status);
					if ((itemStatus.m_kind == svn_node_file)&&(status == svn_wc_status_normal)&&((itemStatus.m_needslock && itemStatus.m_owner[0]==0)||(itemStatus.m_readonly)))
						readonlyoverlay = true;
					if (itemStatus.m_owner[0]!=0)
						lockedoverlay = true;
				}
			}
			break;
		case ShellCache::dll:
			{
				AutoLocker lock(g_csCacheGuard);

				// Look in our caches for this item 
				const FileStatusCacheEntry * s = g_pCachedStatus->GetCachedItem(CTSVNPath(pPath));
				if (s)
				{
					status = s->status;
				}
				else
				{
					// No cached status available 

					// since the dwAttrib param of the IsMemberOf() function does not
					// have the SFGAO_FOLDER flag set at all (it's 0 for files and folders!)
					// we have to check if the path is a folder ourselves :(
					if (PathIsDirectory(pPath))
					{
						if (g_ShellCache.HasSVNAdminDir(pPath, TRUE))
						{
							if ((!g_ShellCache.IsRecursive()) && (!g_ShellCache.IsFolderOverlay()))
							{
								status = svn_wc_status_normal;
							}
							else
							{
								const FileStatusCacheEntry * s = g_pCachedStatus->GetFullStatus(CTSVNPath(pPath), TRUE);
								status = s->status;
								status = SVNStatus::GetMoreImportant(svn_wc_status_normal, status);
							}
						}
						else
						{
							status = svn_wc_status_unversioned;
						}
					}
					else
					{
						const FileStatusCacheEntry * s = g_pCachedStatus->GetFullStatus(CTSVNPath(pPath), FALSE);
						status = s->status;
					}
				}
				if ((s)&&(status == svn_wc_status_normal)&&(s->needslock)&&(s->owner[0]==0))
					readonlyoverlay = true;
				if ((s)&&(s->owner[0]!=0))
					lockedoverlay = true;

			}
			break;
		default:
		case ShellCache::none:
			{
				// no cache means we only show a 'versioned' overlay on folders
				// with an admin directory
				AutoLocker lock(g_csCacheGuard);
				if (PathIsDirectory(pPath))
				{
					if (g_ShellCache.HasSVNAdminDir(pPath, TRUE))
					{
						status = svn_wc_status_normal;
					}
					else
					{
						status = svn_wc_status_unversioned;
					}
				}
				else
				{
					status = svn_wc_status_unversioned;
				}
			}
			break;
		}
		ATLTRACE("Status %d for file %ws\n", status, pwszPath);
	}
	g_filepath.clear();
	g_filepath = pPath;
	g_filestatus = status;
	g_readonlyoverlay = readonlyoverlay;
	g_lockedoverlay = lockedoverlay;
	
	//the priority system of the shell doesn't seem to work as expected (or as I expected):
	//as it seems that if one handler returns S_OK then that handler is used, no matter
	//if other handlers would return S_OK too (they're never called on my machine!)
	//So we return S_OK for ONLY ONE handler!
	switch (status)
	{
		// note: we can show other overlays if due to lack of enough free overlay
		// slots some of our overlays aren't loaded. But we assume that
		// at least the 'normal' and 'modified' overlay are available.
		case svn_wc_status_none:
			return S_FALSE;
		case svn_wc_status_unversioned:
			return S_FALSE;
		case svn_wc_status_normal:
		case svn_wc_status_external:
		case svn_wc_status_incomplete:
			if ((readonlyoverlay)&&(g_readonlyovlloaded))
			{
				if (m_State == FileStateReadOnly)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else if ((lockedoverlay)&&(g_lockedovlloaded))
			{
				if (m_State == FileStateLockedOverlay)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else if (m_State == FileStateVersioned)
			{
				g_filepath.clear();
				return S_OK;
			}
			else
				return S_FALSE;
		case svn_wc_status_missing:
		case svn_wc_status_deleted:
			if (g_deletedovlloaded)
			{
				if (m_State == FileStateDeleted)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'deleted' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == FileStateModified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		case svn_wc_status_replaced:
		case svn_wc_status_modified:
		case svn_wc_status_merged:
			if (m_State == FileStateModified)
			{
				g_filepath.clear();
				return S_OK;
			}
			else
				return S_FALSE;
		case svn_wc_status_added:
			if (g_addedovlloaded)
			{
				if (m_State== FileStateAddedOverlay)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'added' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == FileStateModified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		case svn_wc_status_conflicted:
		case svn_wc_status_obstructed:
			if (g_conflictedovlloaded)
			{
				if (m_State == FileStateConflict)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'conflicted' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == FileStateModified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		default:
			return S_FALSE;
	} // switch (status)
    //return S_FALSE;
}

int CShellExt::GetInstalledOverlays()
{
	// if there are more than 12 overlay handlers installed, then that means not all
	// of the overlay handlers can't be shown. Windows chooses the ones first
	// returned by RegEnumKeyEx() and just drops the ones that come last in
	// that enumeration.
	int nInstalledOverlayhandlers = 0;
	// scan the registry for installed overlay handlers
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers"),
		0, KEY_ENUMERATE_SUB_KEYS, &hKey)==ERROR_SUCCESS)
	{
		for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; i++)
		{ 
			TCHAR value[1024];
			DWORD size = sizeof value / sizeof TCHAR;
			FILETIME last_write_time;
			rc = RegEnumKeyEx(hKey, i, value, &size, NULL, NULL, NULL, &last_write_time);
			if (rc == ERROR_SUCCESS) 
			{
				ATLTRACE("installed handler %ws\n", value);
				nInstalledOverlayhandlers++;
			}
		}
	}
	RegCloseKey(hKey);
	return nInstalledOverlayhandlers;
}