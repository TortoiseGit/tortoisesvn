// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "guids.h"
#include "svnstatus.h"
#include "UnicodeStrings.h"
#include "PreserveChdir.h"
#include <string>
#include <Shlwapi.h>
#include <wininet.h>

// If this is set the SVN columns will always show if the current directory contains SVN content
// Otherwise the user will have to add the columns via the "more..." button
#define COLUMN_ALWAYS_SHOW 1

#ifdef COLUMN_ALWAYS_SHOW
const static int ColumnFlags = SHCOLSTATE_TYPE_STR|SHCOLSTATE_ONBYDEFAULT;
#else
const static int ColumnFlags = SHCOLSTATE_TYPE_STR|SHCOLSTATE_SECONDARYUI;
#endif

// IColumnProvider members
STDMETHODIMP CShellExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
	if (dwIndex > 4)
		return S_FALSE;

	wide_string ws;
	switch (dwIndex)
	{
		case 0:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = 15;
			psci->csFlags = ColumnFlags;

			MAKESTRING(IDS_COLTITLESTATUS);
#ifdef UNICODE
			lstrcpyW(psci->wszTitle, stringtablebuffer);
#else
			lstrcpyW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str());
#endif
			MAKESTRING(IDS_COLDESCSTATUS);
#ifdef UNICODE
			lstrcpyW(psci->wszDescription, stringtablebuffer);
#else
			lstrcpyW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str());
#endif
			break;
		case 1:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = 15;
			psci->csFlags = ColumnFlags;

			MAKESTRING(IDS_COLTITLEREV);
#ifdef UNICODE
			lstrcpyW(psci->wszTitle, stringtablebuffer);
#else
			lstrcpyW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str());
#endif
			MAKESTRING(IDS_COLDESCREV);
#ifdef UNICODE
			lstrcpyW(psci->wszDescription, stringtablebuffer);
#else
			lstrcpyW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str());
#endif
			break;
		case 2:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = 30;
			psci->csFlags = ColumnFlags;

			MAKESTRING(IDS_COLTITLEURL);
#ifdef UNICODE
			lstrcpyW(psci->wszTitle, stringtablebuffer);
#else
			lstrcpyW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str());
#endif
			MAKESTRING(IDS_COLDESCURL);
#ifdef UNICODE
			lstrcpyW(psci->wszDescription, stringtablebuffer);
#else
			lstrcpyW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str());
#endif
			break;
		case 3:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = 30;
			psci->csFlags = ColumnFlags;

			MAKESTRING(IDS_COLTITLESHORTURL);
#ifdef UNICODE
			lstrcpyW(psci->wszTitle, stringtablebuffer);
#else
			lstrcpyW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str());
#endif
			MAKESTRING(IDS_COLDESCSHORTURL);
#ifdef UNICODE
			lstrcpyW(psci->wszDescription, stringtablebuffer);
#else
			lstrcpyW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str());
#endif
			break;
		case 4:
			psci->scid.fmtid = FMTID_SummaryInformation;	// predefined FMTID
			psci->scid.pid   = PIDSI_AUTHOR;				// Predefined - author
			psci->vt         = VT_LPSTR;					// We'll return the data as a string
			psci->fmt        = LVCFMT_LEFT;					// Text will be left-aligned in the column
			psci->csFlags    = SHCOLSTATE_TYPE_STR;			// Data should be sorted as strings
			psci->cChars     = 32;							// Default col width in chars
			break;
	}

	return S_OK;
}

STDMETHODIMP CShellExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
	if (pscid->fmtid == CLSID_TortoiseSVN_UPTODATE && pscid->pid < 4) 
	{
		PreserveChdir preserveChdir;
		stdstring szInfo;
#ifdef UNICODE
		std::wstring path = pscd->wszFile;
#else
		std::string path = WideToUTF8(pscd->wszFile);
#endif

		TCHAR buf[MAX_PATH];

		CRegStdWORD driveremote(_T("Software\\TortoiseSVN\\DriveMaskRemote"));
		CRegStdWORD drivefixed(_T("Software\\TortoiseSVN\\DriveMaskFixed"));
		CRegStdWORD drivecdrom(_T("Software\\TortoiseSVN\\DriveMaskCDROM"));
		CRegStdWORD driveremove(_T("Software\\TortoiseSVN\\DriveMaskRemovable"));
		TCHAR pathbuf[MAX_PATH+4];
		_tcscpy(pathbuf, path.c_str());
		PathRemoveFileSpec(pathbuf);
		PathAddBackslash(pathbuf);
		UINT drivetype = GetDriveType(pathbuf);
		if ((drivetype == DRIVE_REMOVABLE)&&(driveremove == 0))
			return S_FALSE;
		if ((drivetype == DRIVE_FIXED)&&(drivefixed == 0))
			return S_FALSE;
		if ((drivetype == DRIVE_REMOTE)&&(driveremote == 0))
			return S_FALSE;
		if ((drivetype == DRIVE_CDROM)&&(drivecdrom == 0))
			return S_FALSE;

		switch (pscid->pid) 
		{
			case 0:
				GetColumnStatus(path);
				SVNStatus::GetStatusString(g_hmodThisDll, filestatus, buf, sizeof(buf), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				szInfo = buf;
				break;
			case 1:
				GetColumnStatus(path);
				if (columnrev >= 0)
					_sntprintf(buf, MAX_PATH, _T("%d"), columnrev);
				else
					buf[0] = '\0';
				szInfo = buf;
				break;
			case 2:
				GetColumnStatus(path);
				szInfo = itemurl;
				break;
			case 3:
				GetColumnStatus(path);
				szInfo = itemshorturl;
				break;
			default:
				return S_FALSE;
		}
#ifdef UNICODE
		wide_string wsInfo = szInfo;
#else
		wide_string wsInfo = MultibyteToWide(szInfo);
#endif
		V_VT(pvarData) = VT_BSTR;
		V_BSTR(pvarData) = SysAllocString(wsInfo.c_str());
		return S_OK;
	}
	if (pscid->fmtid == FMTID_SummaryInformation)
	{
		PreserveChdir preserveChdir;

		stdstring szInfo;
#ifdef UNICODE
		std::wstring path = pscd->wszFile;
#else
		std::string path = WideToMultibyte(pscd->wszFile);
#endif

		switch (pscid->pid)
		{
			case PIDSI_AUTHOR:			// author
				GetColumnStatus(path);
				szInfo = columnauthor;
				break;
			default:
				return S_FALSE;
		} // switch (pscid->pid)
#ifdef UNICODE
		wide_string wsInfo = szInfo;
#else
		wide_string wsInfo = MultibyteToWide(szInfo);
#endif
		V_VT(pvarData) = VT_BSTR;
		V_BSTR(pvarData) = SysAllocString(wsInfo.c_str());
		return S_OK;
	}

	return S_FALSE;
}

STDMETHODIMP CShellExt::Initialize(LPCSHCOLUMNINIT psci)
{
#ifdef COLUMN_ALWAYS_SHOW
	// psci->wszFolder (WCHAR) holds the path to the folder to be displayed
	// Should check to see if its a "SVN" folder and if not return E_FAIL

	// The only problem with doing this is that we can't add this column in non-SVN folders.
	// Infact it shows up in non-SVN folders as some random other column).
	// Thats why this is turned off for !COLUMN_ALWAYS_SHOW
	PreserveChdir preserveChdir;
#ifdef UNICODE
	std::wstring path = psci->wszFolder;
#else
	std::string path = WideToMultibyte(psci->wszFolder);
#endif
	if (!path.empty())
	{
		if (svn_wc_status_unversioned==SVNStatus::GetAllStatus(path.c_str()))
			return E_FAIL;
	}
#else
#endif

	return S_OK;
}

void CShellExt::GetColumnStatus(stdstring path)
{
	if (columnfilepath.compare(path)==0)
		return;
	CRegStdWORD showrecursive(_T("Software\\TortoiseSVN\\RecursiveOverlay"));
	columnfilepath = path;
	SVNStatus status;
	if (status.GetStatus(path.c_str()) != (-2))
	{
		//if recursive is set in the registry then check directories recursive for status and show
		//the column info with the highest priority on the folder.
		//since this can be slow for big directories it is optional - but very neat.
		if ((showrecursive == 0)||(!PathIsDirectory(path.c_str())))
			filestatus = status.status->text_status > status.status->prop_status ? status.status->text_status : status.status->prop_status;
		else if (showrecursive != 0)
			filestatus = SVNStatus::GetAllStatusRecursive(path.c_str());
		else
			filestatus = status.status->text_status > status.status->prop_status ? status.status->text_status : status.status->prop_status;

		if (status.status->entry != NULL)
		{
			if (status.status->entry->cmt_author != NULL)
#ifdef UNICODE
				columnauthor = UTF8ToWide(status.status->entry->cmt_author);
#else
				columnauthor = status.status->entry->cmt_author;
#endif
			else
				columnauthor = _T(" ");
			columnrev = status.status->entry->cmt_rev;
			if (status.status->entry->url)
			{
#ifdef UNICODE
				itemurl = UTF8ToWide(status.status->entry->url);
#else
				itemurl = status.status->entry->url;
#endif
				TCHAR urlpath[INTERNET_MAX_URL_LENGTH+1];

				URL_COMPONENTS urlComponents;
				memset(&urlComponents, 0, sizeof(URL_COMPONENTS));
				urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
				urlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
				urlComponents.lpszUrlPath = urlpath;
				if (InternetCrackUrl(itemurl.c_str(), 0, ICU_DECODE, &urlComponents))
				{
					TCHAR * ptr = _tcsrchr(urlComponents.lpszUrlPath, '/');
					if (ptr == NULL)
						ptr = _tcsrchr(urlComponents.lpszUrlPath, '\\');
					if (ptr)
					{
						*ptr = '\0';
						itemshorturl = urlComponents.lpszUrlPath;
					} // if (ptr)
					else 
						itemshorturl = _T(" ");
				}
				else
					itemshorturl = _T(" ");

			} // if (status.status->entry->url)
			else
			{
				itemurl = _T(" ");
				itemshorturl = _T(" ");
			}
		} // if (status.status->entry != NULL)
		else
		{
			columnrev = -1;
			columnauthor = _T(" ");
			itemurl = _T(" ");
			itemshorturl = _T(" ");
		}
	} // if (status.GetStatus(path.c_str()) != (-2))
}

