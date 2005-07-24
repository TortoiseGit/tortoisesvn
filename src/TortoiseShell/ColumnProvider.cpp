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
#include "stdafx.h"
#include "ShellExt.h"
#include "guids.h"
#include "PreserveChdir.h"
#include "SVNProperties.h"
#include "UnicodeStrings.h"
#include "SVNStatus.h"
#include "..\TSVNCache\CacheInterface.h"


const static int ColumnFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;

// Defines that revision numbers occupy at most MAX_REV_STRING_LEN characters.
// There are Perforce repositories out there that have several 100,000 revs.
// So, don't be too restrictive by limiting this to 6 digits + 1 separator, 
// for instance. 
//
// Because shorter strings will be extended to have exactly MAX_REV_STRING_LEN 
// characters, large numbers will produce large strings. These, in turn, will
// affect column autosizing. This setting is a reasonable compromise.
//
// Max rev correctly sorted: 99,999,999 for MAX_REV_STRING_LEN == 10

#define MAX_REV_STRING_LEN 10

// IColumnProvider members
STDMETHODIMP CShellExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
	PreserveChdir preserveChdir;
	if (dwIndex > 7)
		return S_FALSE;

	LoadLangDll();
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
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
#else
			lstrcpynW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_NAME_LEN);
#endif
			MAKESTRING(IDS_COLDESCSTATUS);
#ifdef UNICODE
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
#else
			lstrcpynW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_DESC_LEN);
#endif
			break;
		case 1:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_I4;
			psci->fmt = LVCFMT_RIGHT;
			psci->cChars = 15;
			psci->csFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT;

			MAKESTRING(IDS_COLTITLEREV);
#ifdef UNICODE
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
#else
			lstrcpynW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_NAME_LEN);
#endif
			MAKESTRING(IDS_COLDESCREV);
#ifdef UNICODE
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
#else
			lstrcpynW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_DESC_LEN);
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
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
#else
			lstrcpynW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_NAME_LEN);
#endif
			MAKESTRING(IDS_COLDESCURL);
#ifdef UNICODE
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
#else
			lstrcpynW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_DESC_LEN);
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
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
#else
			lstrcpynW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_NAME_LEN);
#endif
			MAKESTRING(IDS_COLDESCSHORTURL);
#ifdef UNICODE
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
#else
			lstrcpynW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_DESC_LEN);
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
		case 5:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = 30;
			psci->csFlags = ColumnFlags;

			MAKESTRING(IDS_COLTITLEMIMETYPE);
#ifdef UNICODE
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
#else
			lstrcpynW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_NAME_LEN);
#endif
			MAKESTRING(IDS_COLDESCMIMETYPE);
#ifdef UNICODE
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
#else
			lstrcpynW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_DESC_LEN);
#endif
			break;
		case 6:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = 30;
			psci->csFlags = ColumnFlags;

			MAKESTRING(IDS_COLTITLEOWNER);
#ifdef UNICODE
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
#else
			lstrcpynW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_NAME_LEN);
#endif
			MAKESTRING(IDS_COLDESCOWNER);
#ifdef UNICODE
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
#else
			lstrcpynW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_DESC_LEN);
#endif
			break;
		case 7:
			psci->scid.fmtid = CLSID_TortoiseSVN_UPTODATE;
			psci->scid.pid = dwIndex;
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_LEFT;
			psci->cChars = 30;
			psci->csFlags = ColumnFlags;

			MAKESTRING(IDS_COLTITLEAUTHOR);
#ifdef UNICODE
			lstrcpynW(psci->wszTitle, stringtablebuffer, MAX_COLUMN_NAME_LEN);
#else
			lstrcpynW(psci->wszTitle, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_NAME_LEN);
#endif
			MAKESTRING(IDS_COLDESCAUTHOR);
#ifdef UNICODE
			lstrcpynW(psci->wszDescription, stringtablebuffer, MAX_COLUMN_DESC_LEN);
#else
			lstrcpynW(psci->wszDescription, MultibyteToWide(stringtablebuffer).c_str(), MAX_COLUMN_DESC_LEN);
#endif
			break;
	}

	return S_OK;
}

STDMETHODIMP CShellExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
	PreserveChdir preserveChdir;
	LoadLangDll();
	if (pscid->fmtid == CLSID_TortoiseSVN_UPTODATE && pscid->pid < 7) 
	{
		stdstring szInfo;
#ifdef UNICODE
		const TCHAR * path = (TCHAR *)pscd->wszFile;
#else
		std::string stdpath = WideToMultibyte(pscd->wszFile);
		const TCHAR * path = stdpath.c_str();
#endif

		// reserve for the path + trailing \0

		TCHAR buf[MAX_STATUS_STRING_LENGTH+1];
		ZeroMemory(buf, MAX_STATUS_STRING_LENGTH);
		switch (pscid->pid) 
		{
			case 0:
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				SVNStatus::GetStatusString(g_hResInst, filestatus, buf, sizeof(buf), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				szInfo = buf;
				break;
			case 1:
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				if (columnrev >= 0)
				{
					V_VT(pvarData) = VT_I4;
					V_I4(pvarData) = columnrev;
				}
				return S_OK;
				break;
			case 2:
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				szInfo = itemurl;
				break;
			case 3:
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				szInfo = itemshorturl;
				break;
			case 5:
				if (g_ShellCache.IsPathAllowed(path))
				{
					SVNProperties props = SVNProperties(CTSVNPath(path));
					for (int i=0; i<props.GetCount(); i++)
					{
						if (props.GetItemName(i).compare(_T("svn:mime-type"))==0)
						{
#ifdef UNICODE
							szInfo = MultibyteToWide((char *)props.GetItemValue(i).c_str());
#else
							szInfo = props.GetItemValue(i);
#endif
						}
					}
				}
				break;
			case 6:
				GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
				szInfo = owner;
				break;
			default:
				return S_FALSE;
		}
#ifdef UNICODE
		const WCHAR * wsInfo = szInfo.c_str();
#else
		wide_string stdwsInfo = MultibyteToWide(szInfo);
		const WCHAR * wsInfo = stdwsInfo.c_str();
#endif
		V_VT(pvarData) = VT_BSTR;
		V_BSTR(pvarData) = SysAllocString(wsInfo);
		return S_OK;
	}
	if ((pscid->fmtid == FMTID_SummaryInformation)||(pscid->pid == 7))
	{
		stdstring szInfo;
#ifdef UNICODE
		const TCHAR * path = pscd->wszFile;
#else
		std::string stdpath = WideToMultibyte(pscd->wszFile);
		const TCHAR * path = stdpath.c_str();
#endif

		switch (pscid->pid)
		{
		case PIDSI_AUTHOR:			// author
		case 7:
			GetColumnStatus(path, pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			szInfo = columnauthor;
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

	return S_FALSE;
}

STDMETHODIMP CShellExt::Initialize(LPCSHCOLUMNINIT psci)
{
	// psci->wszFolder (WCHAR) holds the path to the folder to be displayed
	// Should check to see if its a "SVN" folder and if not return E_FAIL

	PreserveChdir preserveChdir;
#ifdef UNICODE
	std::wstring path = psci->wszFolder;
#else
	std::string path = WideToMultibyte(psci->wszFolder);
#endif
	if (!path.empty())
	{
		if (! g_ShellCache.HasSVNAdminDir(path.c_str(), TRUE))
			return E_FAIL;
	}
	return S_OK;
}

void CShellExt::GetColumnStatus(const TCHAR * path, BOOL /*bIsDir*/)
{
	PreserveChdir preserveChdir;
	if (_tcscmp(path, columnfilepath.c_str())==0)
		return;
	LoadLangDll();
	columnfilepath = path;
	AutoLocker lock(g_csCacheGuard);

	TSVNCacheResponse itemStatus;
	ZeroMemory(&itemStatus, sizeof(itemStatus));
	if(g_remoteCacheLink.GetStatusFromRemoteCache(CTSVNPath(path), &itemStatus, false))
	{
		filestatus = SVNStatus::GetMoreImportant(itemStatus.m_status.text_status, itemStatus.m_status.prop_status);
	}
	else
		return;	

#ifdef UNICODE
	columnauthor = UTF8ToWide(itemStatus.m_author);
#else
	columnauthor = itemStatus.m_author;
#endif
	columnrev = itemStatus.m_entry.cmt_rev;
#ifdef UNICODE
	itemurl = UTF8ToWide(itemStatus.m_url);
#else
	itemurl = itemStatus.m_url;
#endif
#ifdef UNICODE
	owner = UTF8ToWide(itemStatus.m_owner);
#else
	owner = itemStatus.m_owner;
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

}

