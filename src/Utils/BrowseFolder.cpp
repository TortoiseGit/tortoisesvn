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
#include "StdAfx.h"
#include "BrowseFolder.h"

CBrowseFolder::CBrowseFolder(void)
:	m_style(0),
	m_root(NULL)
{
	memset(m_displayName, 0, sizeof(m_displayName));
	memset(m_title, 0, sizeof(m_title));
}

CBrowseFolder::~CBrowseFolder(void)
{
}

//show the dialog
CBrowseFolder::retVal CBrowseFolder::Show(HWND parent, LPTSTR path)
{
	CString temp;
	temp = path;
	CBrowseFolder::retVal ret = Show(parent, temp);
	_tcscpy(path, temp);
	return ret;
}
CBrowseFolder::retVal CBrowseFolder::Show(HWND parent, CString& path)
{
	ASSERT(path);

	if (!path)
		return NOPATH;		//no path specified!

	retVal ret = OK;		//assume OK

	LPITEMIDLIST itemIDList;

	BROWSEINFO browseInfo;

	browseInfo.hwndOwner		= parent;
	browseInfo.pidlRoot			= m_root;
	browseInfo.pszDisplayName	= m_displayName;
	browseInfo.lpszTitle		= m_title;
	browseInfo.ulFlags			= m_style;
	browseInfo.lpfn				= NULL;
	browseInfo.lParam			= 0;
	
	itemIDList = SHBrowseForFolder(&browseInfo);

	//is the dialog cancelled?
	if (!itemIDList)
		ret = CANCEL;

	if (ret != CANCEL) 
	{
		if (!SHGetPathFromIDList(itemIDList, path.GetBuffer(MAX_PATH)))
			ret = NOPATH;

		path.ReleaseBuffer();
	
		LPMALLOC	shellMalloc;
		HRESULT		hr;

		hr = SHGetMalloc(&shellMalloc);

		if (SUCCEEDED(hr)) 
		{
			//free memory
			shellMalloc->Free(itemIDList);
			//release interface
			shellMalloc->Release();
		}
	}
	return ret;
}

void CBrowseFolder::SetInfo(LPCTSTR title)
{
	ASSERT(title);
	
	if (title)
		_tcscpy(m_title, title);
}
