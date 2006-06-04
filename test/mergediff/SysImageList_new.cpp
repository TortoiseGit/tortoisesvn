#include "stdafx.h"
#include "SysImageList.h"
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
	if (instance == NULL)
		instance = new CSysImageList;
	return *instance;
}

void CSysImageList::Cleanup()
{
	delete instance;
	instance = NULL;
}

int CSysImageList::GetDirIconIndex() const
{
	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(
		_T("blablah"),
		FILE_ATTRIBUTE_DIRECTORY,
		&sfi, sizeof sfi,
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

	return sfi.iIcon;
}

int CSysImageList::GetDefaultIconIndex() const
{
	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(_T(""), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof sfi, SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

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
	IconIndexMap::iterator it = m_indexCache.lower_bound(strExtension);
	if (it == m_indexCache.end() || 
		strExtension < it->first)
	{
		// We don't have this extension in the map
		int iconIndex = GetFileIconIndex(filePath.GetFilename());
		it = m_indexCache.insert(it, std::make_pair(strExtension, iconIndex));
	}
	// We must have found it
	return it->second;
}
