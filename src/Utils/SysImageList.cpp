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
#include "SysImageList.h"
#include "Utils.h"
#include "TSVNPath.h"


// Singleton constructor and destructor (private)

CSysImageList * CSysImageList::instance = 0;

CSysImageList::CSysImageList()
{
	SHFILEINFO ssfi;
	TCHAR windir[MAX_PATH];
	GetWindowsDirectory(windir, MAX_PATH);	// MAX_PATH ok.
	HIMAGELIST hSystemImageList =
		(HIMAGELIST)SHGetFileInfo(
			windir,
			0,
			&ssfi, sizeof ssfi,
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	Attach(hSystemImageList);
}

CSysImageList::~CSysImageList()
{
	Detach();
}


// Singleton specific operations

CSysImageList& CSysImageList::GetInstance()
{
	if (instance == 0)
		instance = new CSysImageList;
	return *instance;
}

void CSysImageList::Cleanup()
{
	delete instance;
	instance = 0;
}


// Operations

int CSysImageList::GetDirIconIndex() const
{
	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(
		_T("Doesn't matter"),
		FILE_ATTRIBUTE_DIRECTORY,
		&sfi, sizeof sfi,
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

	return sfi.iIcon;
}

int CSysImageList::GetDefaultIconIndex() const
{
	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(
		_T(""),
		FILE_ATTRIBUTE_NORMAL,
		&sfi, sizeof sfi,
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

	return sfi.iIcon;
}

int CSysImageList::GetFileIconIndex(const CString& file) const
{
	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(
		file,
		FILE_ATTRIBUTE_NORMAL,
		&sfi, sizeof sfi,
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

	return sfi.iIcon;
}

int CSysImageList::GetPathIconIndex(const CTSVNPath& filePath) const
{
	CString strExtension = filePath.GetFileExtension();
	strExtension.MakeUpper();
	IconIndexMap::const_iterator it = m_indexCache.find(strExtension);
	if(it == m_indexCache.end())
	{
		// We don't have this extension in the map
		int iconIndex = GetFileIconIndex(filePath.GetFilename());
		m_indexCache[strExtension] = iconIndex;
		return iconIndex;
	}
	// We must have found it
	return it->second;
}
