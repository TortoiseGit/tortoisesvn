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
#include "guids.h"
#include "svnstatus.h"
#include "UnicodeStrings.h"
#include "PreserveChdir.h"
#include "SVNProperties.h"
#include <string>
#include <Shlwapi.h>
#include <wininet.h>
#include <atlexcept.h>


const static int ColumnFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;

// Defines that revision numbers occupy at most MAX_REV_STRING_LEN characters.
// There are Perforce repositories out there that have several 100,000 revs.
// So, don't be too restrictive by limiting this to 6 digits + 1 seperator, 
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
	if (dwIndex > 5)
		return S_FALSE;

	// Read and select the system's locale settings.
	// There seems to be no easy way to update this info (after the user
	// changed her settings). Hence, we set it once and for all.

	setlocale (LC_ALL, "");

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
			psci->vt = VT_BSTR;
			psci->fmt = LVCFMT_RIGHT;
			psci->cChars = 15;
			psci->csFlags = ColumnFlags;

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
	}

	return S_OK;
}

STDMETHODIMP CShellExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
	LoadLangDll();
	DWORD dwWaitResult = 0;
	if (pscid->fmtid == CLSID_TortoiseSVN_UPTODATE && pscid->pid < 6) 
	{
		PreserveChdir preserveChdir;
		stdstring szInfo;
#ifdef UNICODE
		std::wstring path = pscd->wszFile;
#else
		std::string path = WideToUTF8(pscd->wszFile);
#endif

		// reserve for the path + trailing \0

		TCHAR buf[MAX_PATH+1];
		ZeroMemory(buf, MAX_PATH);
		switch (pscid->pid) 
		{
			case 0:
				dwWaitResult = WaitForSingleObject(hMutex, 100);
				if (dwWaitResult == WAIT_OBJECT_0)
				{
					GetColumnStatus(path);
					SVNStatus::GetStatusString(g_hResInst, filestatus, buf, sizeof(buf), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
					szInfo = buf;
				} // if (dwWaitResult == WAIT_OBJECT_0)
				ReleaseMutex(hMutex);
				break;
			case 1:
				dwWaitResult = WaitForSingleObject(hMutex, 100);
				if (dwWaitResult == WAIT_OBJECT_0)
				{
					GetColumnStatus(path);
					if (columnrev >= 0)
					{
						// First, have to convert the revision number into a string.
						// Allocate a "sufficient" number of bytes for that.
						// (3 decimals per 8 bits + 1 for the trailing \0)

						TCHAR plainNumber[sizeof(columnrev)* 3 + 1];
						_i64tot (columnrev, plainNumber, 10);

						if (GetNumberFormat (LOCALE_USER_DEFAULT, 0, 
											 plainNumber, g_ShellCache.GetNumberFmt(), 
											 buf, MAX_PATH) != 0)
						{
							// output:
							// write leading spaces followed by the previously formatted number string
							size_t spacesToAdd = max(0, (int)MAX_REV_STRING_LEN - (int)_tcslen (buf));
							std::swap (szInfo, stdstring (spacesToAdd, _T(' ')) + buf);
						}
						// else: szInfo simply remains empty
					}
				} // if (dwWaitResult == WAIT_OBJECT_0)

				ReleaseMutex(hMutex);
				break;
			case 2:
				dwWaitResult = WaitForSingleObject(hMutex, 100);
				if (dwWaitResult == WAIT_OBJECT_0)
				{
					GetColumnStatus(path);
					szInfo = itemurl;
				} // if (dwWaitResult == WAIT_OBJECT_0)
				ReleaseMutex(hMutex);
				break;
			case 3:
				dwWaitResult = WaitForSingleObject(hMutex, 100);
				if (dwWaitResult == WAIT_OBJECT_0)
				{
					GetColumnStatus(path);
					szInfo = itemshorturl;
				} // if (dwWaitResult == WAIT_OBJECT_0)
				ReleaseMutex(hMutex);
				break;
			case 5:
				dwWaitResult = WaitForSingleObject(hMutex, 100);
				if (dwWaitResult == WAIT_OBJECT_0)
				{
					if (g_ShellCache.IsPathAllowed(path.c_str()))
					{
						SVNProperties props = SVNProperties(path.c_str());
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
				} // if (dwWaitResult == WAIT_OBJECT_0)
				ReleaseMutex(hMutex);
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
	} // if (pscid->fmtid == CLSID_TortoiseSVN_UPTODATE && pscid->pid < 4) 
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
				dwWaitResult = WaitForSingleObject(hMutex, 10);
				if (dwWaitResult == WAIT_OBJECT_0)
				{
					GetColumnStatus(path);
					szInfo = columnauthor;
				} // if (dwWaitResult == WAIT_OBJECT_0)
				ReleaseMutex(hMutex);
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
		TCHAR buf[MAX_PATH];
		_tcscpy(buf, path.c_str());
		_tcscat(buf, _T("\\"));
		_tcscat(buf, _T(SVN_WC_ADM_DIR_NAME));
		if (!PathFileExists(buf))
			return E_FAIL;
	}
	return S_OK;
}

void CShellExt::GetColumnStatus(stdstring path)
{
	if (columnfilepath.compare(path)==0)
		return;
	LoadLangDll();
	columnfilepath = path;
	filestatuscache * status = g_CachedStatus.GetFullStatus(path.c_str());
	filestatus = status->status;

#ifdef UNICODE
	columnauthor = UTF8ToWide(status->author);
#else
	columnauthor = status->author;
#endif
	columnrev = status->rev;
#ifdef UNICODE
	itemurl = UTF8ToWide(status->url);
#else
	itemurl = status->url;
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

