// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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


#include "ShellExt.h"
#include "Guids.h"
#include "UnicodeStrings.h"
#include "PreserveChdir.h"
#include "SVNStatus.h"
// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."

STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
    // HACK: Under NT (and 95?), file open dialog crashes apps upon shutdown
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if( !GetVersionEx(&osv) )
    {
        return S_FALSE;
    }
    // Under NT (and 95?), file open dialog crashes apps upon shutdown,
    // so we disable our icon overlays unless we are in Explorer process
    // space.
    bool allowExplorer = osv.dwMajorVersion > 4 || // allow anything major > 4
        (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
        osv.dwMajorVersion == 4 && osv.dwMinorVersion > 0); // plus Windows 98/Me
    // Test if we are in Explorer
    if (!allowExplorer)
    {
        TCHAR buf[_MAX_PATH + 1];
        GetModuleFileName(NULL, buf, _MAX_PATH);
        stdstring filename = buf;
        // Does this need case insensitivity?
        if (filename.find(_T("explorer"), 0) == std::string::npos)
            return S_FALSE;
    }

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
		case Versioned : iconName = _T("InSubversionIcon"); break;
		case Modified  : iconName = _T("ModifiedIcon"); break;
		case Conflict  : iconName = _T("ConflictIcon"); break;
		case Deleted   : iconName = _T("DeletedIcon"); break;
		case Added     : iconName = _T("AddedIcon"); break;
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
#ifdef UNICODE
        wcsncpy (pwszIconFile, icon.c_str(), cchMax);
#else
        wcsncpy (pwszIconFile, MultibyteToWide(icon.c_str()).c_str(), cchMax);
#endif		
    else
        return S_FALSE;

    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE;
    return S_OK;
};

STDMETHODIMP CShellExt::GetPriority(int *pPriority)
{
	switch (m_State)
	{
		case Conflict:
			*pPriority = 0;
			break;
		case Modified:
			*pPriority = 1;
			break;
		case Deleted:
			*pPriority = 2;
			break;
		case Added:
			*pPriority = 3;
			break;
		case Versioned:
			*pPriority = 4;
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

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD /* dwAttrib */)
{
	PreserveChdir preserveChdir;
	svn_wc_status_kind status;
	if (pwszPath == NULL)
		return S_FALSE;
#ifdef UNICODE
	std::wstring sPath = pwszPath;
#else
	std::string sPath = WideToUTF8(std::basic_string<wchar_t>(pwszPath));
#endif
	//if recursive is set in the registry then check directories recursive for status and show
	//the overlay with the highest priority on the folder.
	//since this can be slow for big directories it is optional - but very neat
	//also check if we already have the status for the path so we don't have to get it again (small cache)
	if (sPath.compare(filepath)==0)
	{
		status = filestatus;
	}
	else
	{
		if (! g_ShellCache.IsPathAllowed(sPath.c_str()))
			return S_FALSE;

		if (PathIsDirectory(sPath.c_str()))
		{
			TCHAR buf[MAX_PATH];
			_tcscpy(buf, sPath.c_str());
			_tcscat(buf, _T("\\"));
			_tcscat(buf, _T(SVN_WC_ADM_DIR_NAME));
			if (PathFileExists(buf))
			{
				if (!g_ShellCache.IsRecursive())
				{
					status = svn_wc_status_normal;
				}
				else
				{
					DWORD dwWaitResult = WaitForSingleObject(hMutex, 100);
					if (dwWaitResult == WAIT_OBJECT_0)
					{
						filestatuscache * s = g_CachedStatus.GetFullStatus(sPath.c_str());
						status = s->status;
					} // if (dwWaitResult == WAIT_OBJECT_0) 
					else
						status = svn_wc_status_normal;
					ReleaseMutex(hMutex);

				}
			} // if (PathFileExists(buf))
			else
			{
				status = svn_wc_status_unversioned;
			}
		} // if (PathIsDirectory(filepath))
		else
		{
			DWORD dwWaitResult = WaitForSingleObject(hMutex, 100);
			if (dwWaitResult == WAIT_OBJECT_0)
			{
				filestatuscache * s = g_CachedStatus.GetFullStatus(sPath.c_str());
				status = s->status;
			} // if (dwWaitResult == WAIT_OBJECT_0)
			ReleaseMutex(hMutex);
		}
		filepath.clear();
		filepath = sPath;
		filestatus = status;
	}

	//the priority system of the shell doesn't seem to work as expected (or as I expected):
	//as it seems that if one handler returns S_OK then that handler is used, no matter
	//if other handlers would return S_OK too (they're never called on my machine!)
	//So we return S_OK for ONLY ONE handler!
	switch (status)
	{
		case svn_wc_status_none:
			return S_FALSE;
		case svn_wc_status_unversioned:
			return S_FALSE;
		case svn_wc_status_normal:
		case svn_wc_status_external:
		case svn_wc_status_incomplete:
			if (m_State == Versioned)
				return S_OK;
			else
				return S_FALSE;
		case svn_wc_status_added:
			if (m_State == Added)
				return S_OK;
			else
				return S_FALSE;
		case svn_wc_status_missing:
		case svn_wc_status_deleted:
			if (m_State == Deleted)
				return S_OK;
			else
				return S_FALSE;
		case svn_wc_status_replaced:
			if (m_State == Modified)
				return S_OK;
			else
				return S_FALSE;
		case svn_wc_status_modified:
			if (m_State == Modified)
				return S_OK;
			else
				return S_FALSE;
		case svn_wc_status_merged:
			if (m_State == Modified)
				return S_OK;
			else
				return S_FALSE;
		case svn_wc_status_conflicted:
		case svn_wc_status_obstructed:
			if (m_State == Conflict)
				return S_OK;
			else
				return S_FALSE;
		default:
			return S_FALSE;
	} // switch (status)
    return S_FALSE;
}

