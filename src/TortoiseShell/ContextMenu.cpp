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
#include "ItemIDList.h"
#include <string>

#include "UnicodeStrings.h"
#include "PreserveChdir.h"

#include "SVNStatus.h"


#define GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

int g_shellidlist=RegisterClipboardFormat(CFSTR_SHELLIDLIST);

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY /* hRegKey */)
{
	files_.clear();
	folder_.erase();	
	isOnlyOneItemSelected = false;
	isInSVN = false;
	isConflicted = false;
	isFolder = false;
	isFolderInSVN = false;
	isNormal = false;
	isIgnored = false;
	isInVersionedFolder = false;

	// get selected files/folders
	if (pDataObj)
	{
		STGMEDIUM medium;
		FORMATETC fmte = {g_shellidlist,
			(DVTARGETDEVICE FAR *)NULL, 
			DVASPECT_CONTENT, 
			-1, 
			TYMED_HGLOBAL};
		HRESULT hres = pDataObj->GetData(&fmte, &medium);

		if (SUCCEEDED(hres) && medium.hGlobal)
		{
			if (m_State == DropHandler)
			{
				FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg = { TYMED_HGLOBAL };
				if ( FAILED( pDataObj->GetData ( &etc, &stg )))
					return E_INVALIDARG;


				TCHAR m_szFileName[MAX_PATH];
				HDROP drop = (HDROP)GlobalLock(stg.hGlobal);
				if ( NULL == drop )
				{
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}

				int count = DragQueryFile(drop, (UINT)-1, NULL, 0);
				for (int i = 0; i < count; i++)
				{
					if (0 == DragQueryFile(drop, i, m_szFileName, sizeof(m_szFileName)))
						continue;
					stdstring str = stdstring(m_szFileName);
					if (str.empty() == false)
					{
						files_.push_back(str);
						//get the Subversion status of the item
						svn_wc_status_kind status = SVNStatus::GetAllStatus(str.c_str());
						if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored))
							isInSVN = true;
						if (status == svn_wc_status_ignored)
							isIgnored = true;
						if (status == svn_wc_status_normal)
							isNormal = true;
						if (status == svn_wc_status_conflicted)
							isConflicted = true;
					}
				} // for (int i = 0; i < count; i++)
				GlobalUnlock ( drop );
			} // if (m_State == DropHandler)
			else
			{
				//Enumerate PIDLs which the user has selected
				CIDA* cida = (CIDA*)medium.hGlobal;
				ItemIDList parent( GetPIDLFolder (cida));

				int count = cida->cidl;
				for (int i = 0; i < count; ++i)
				{
					ItemIDList child (GetPIDLItem (cida, i), &parent);
					stdstring str = child.toString();
					if (str.empty() == false)
					{
						files_.push_back(str);
						//get the Subversion status of the item
						svn_wc_status_kind status = SVNStatus::GetAllStatus(str.c_str());
						if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored))
							isInSVN = true;
						if (status == svn_wc_status_ignored)
							isIgnored = true;
						if (status == svn_wc_status_normal)
							isNormal = true;
						if (status == svn_wc_status_conflicted)
							isConflicted = true;
					}
				} // for (int i = 0; i < count; ++i)
				ItemIDList child (GetPIDLItem (cida, 0), &parent);
				TCHAR buf[MAX_PATH+1];
				_tcsncpy(buf, child.toString().c_str(), MAX_PATH);
				TCHAR * ptr = _tcsrchr(buf, '\\');
				if (ptr != 0)
				{
					*ptr = 0;
					_tcsncat(buf, _T("\\"), MAX_PATH);
					_tcsncat(buf, _T(SVN_WC_ADM_DIR_NAME), MAX_PATH);
					if (PathFileExists(buf))
						isInVersionedFolder = true;
				}
			}


			if (medium.pUnkForRelease)
			{
				IUnknown* relInterface = (IUnknown*)medium.pUnkForRelease;
				relInterface->Release();
			}
		}
	}

	// get folder background
	if (pIDFolder) 
	{
		ItemIDList list(pIDFolder);
		folder_ = list.toString();
		svn_wc_status_kind status = SVNStatus::GetAllStatus(folder_.c_str());
		if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored))
		{
			isFolderInSVN = true;
			isInSVN = true;
		}
		if (status == svn_wc_status_ignored)
			isIgnored = true;
		isFolder = true;
	}
	if ((files_.size() == 1)&&(m_State != DropHandler))
	{
		isOnlyOneItemSelected = true;
		if (PathIsDirectory(files_.front().c_str()))
		{
			folder_ = files_.front();
			svn_wc_status_kind status = SVNStatus::GetAllStatus(folder_.c_str());
			if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored))
				isFolderInSVN = true;
			if (status == svn_wc_status_ignored)
				isIgnored = true;
			isFolder = true;
		}
	}
	if ((isFolder == true)&&(isIgnored == true))
		isInSVN = true;
		
	return NOERROR;
}

void CShellExt::InsertSVNMenu(BOOL ownerdrawn, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, SVNCommands com)
{
	MAKESTRING(stringid);

	if (ownerdrawn) {
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING | MF_OWNERDRAW, id, stringtablebuffer);
	}
	else {
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING , id, stringtablebuffer);
		HBITMAP bmp = IconToBitmap(icon, (COLORREF)GetSysColor(COLOR_MENU)); 
		SetMenuItemBitmaps(menu, pos, MF_BYPOSITION, bmp, bmp);
	}

	// We store the relative and absolute diameter
	// (drawitem callback uses absolute, others relative)
	myIDMap[id - idCmdFirst] = com;
	myIDMap[id] = com;
}

HBITMAP CShellExt::IconToBitmap(UINT uIcon, COLORREF transparentColor)
{
	HICON hIcon = (HICON)LoadImage(g_hResInst, MAKEINTRESOURCE(uIcon), IMAGE_ICON, 10, 10, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;

	RECT     rect;

	rect.right = ::GetSystemMetrics(SM_CXMENUCHECK);
	rect.bottom = ::GetSystemMetrics(SM_CYMENUCHECK);

	rect.left =
		rect.top  = 0;

	HWND desktop    = ::GetDesktopWindow();
	HDC  screen_dev = ::GetDC(desktop);

	// Create a compatible DC
	HDC dst_hdc = ::CreateCompatibleDC(screen_dev);

	// Create a new bitmap of icon size
	HBITMAP bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);

	// Select it into the compatible DC
	HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);

	// Fill the background of the compatible DC with the given colour
	HBRUSH brush = ::CreateSolidBrush(transparentColor);
	::FillRect(dst_hdc, &rect, brush);
	::DeleteObject(brush);

	// Draw the icon into the compatible DC
	::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

	// Restore settings
	::SelectObject(dst_hdc, old_dst_bmp);
	::DeleteDC(dst_hdc);
	::ReleaseDC(desktop, screen_dev); 
	DestroyIcon(hIcon);
	return bmp;
}


stdstring CShellExt::WriteFileListToTempFile()
{
	//write all selected files and paths to a temporary file
	//for TortoiseProc.exe to read out again.
	TCHAR path[MAX_PATH];
	TCHAR tempFile[MAX_PATH];

	DWORD len = ::GetTempPath (MAX_PATH, path);
	UINT unique = ::GetTempFileName (path, _T("svn"), 0, tempFile);

	HANDLE file = ::CreateFile (tempFile,
								GENERIC_WRITE, 
								FILE_SHARE_READ, 
								0, 
								CREATE_ALWAYS, 
								FILE_ATTRIBUTE_TEMPORARY,
								0);

	DWORD written = 0;
	if (files_.empty())
	{
		::WriteFile (file, folder_.c_str(), folder_.size()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	}

	for (std::vector<stdstring>::iterator I = files_.begin(); I != files_.end(); ++I)
	{
		::WriteFile (file, I->c_str(), I->size()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	}
	::CloseHandle(file);
	return stdstring(tempFile);
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT idCmdLast,
                                         UINT uFlags)
{
	//first check if our drophandler is called
	//and then (if true) provide the context menu for the
	//drop handler
	if (m_State == DropHandler)
	{
		LoadLangDll();
		PreserveChdir preserveChdir;

		if ((uFlags & CMF_DEFAULTONLY)!=0)
			return NOERROR;					//we don't change the default action

		if ((files_.size() == 0)&&(folder_.size() == 0))
			return NOERROR;

		if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
			return NOERROR;
		//check if our menu is requested for the start menu
		//TCHAR buf[MAX_PATH];
		//SHGetSpecialFolderPath(NULL, buf, CSIDL_STARTMENU, FALSE);
		//if (_tcscmp(buf, folder_.c_str())==0)
		//	return NOERROR;

		//the drophandler only has two commands, but only one is visible at a time:
		//if the source file(s) are under version control then those files can be moved
		//to the new location, if they are unversioned then they can be added to the
		//working copy
		UINT idCmd = idCmdFirst;

		if ((!isInSVN)&&(isFolderInSVN))
			InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYADDMENU, 0, idCmdFirst, DropCopyAdd);
		else if ((isInSVN)&&(isFolderInSVN))
			InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVEMENU, 0, idCmdFirst, DropMove);
		if ((isInSVN)&&(isFolderInSVN))
			InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYMENU, 0, idCmdFirst, DropCopy);

		return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
	}

//missing commands:
//Create Patch		//to create patch files
//Apply Patch		//whats the complement of Diff?

	PreserveChdir preserveChdir;

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return NOERROR;					//we don't change the default action

	if ((uFlags & CMF_VERBSONLY)!=0)
		return NOERROR;					//we don't show the menu on shortcuts

	if ((files_.size() == 0)&&(folder_.size() == 0))
		return NOERROR;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return NOERROR;
	//check if our menu is requested for the start menu
	TCHAR buf[MAX_PATH];
	SHGetSpecialFolderPath(NULL, buf, CSIDL_STARTMENU, FALSE);
	if (_tcscmp(buf, folder_.c_str())==0)
		return NOERROR;
	if (_tcsstr(folder_.c_str(), _T(SVN_WC_ADM_DIR_NAME))!=0)
		return NOERROR;

	LoadLangDll();
	bool extended = ((uFlags & CMF_EXTENDEDVERBS)!=0);		//true if shift was pressed for the context menu
	UINT idCmd = idCmdFirst;

	//create the submenu
	HMENU subMenu = CreateMenu();
	int indexSubMenu = 0;
	int lastSeparator = 0;

	DWORD topmenu = g_ShellCache.GetMenuLayout();
//#region menu
#define HMENU(x) ((topmenu & (x)) ? hMenu : subMenu)
#define INDEXMENU(x) ((topmenu & (x)) ? indexMenu++ : indexSubMenu++)
	//---- separator before
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	BOOL ownerdrawn = CRegStdWORD(_T("Software\\TortoiseSVN\\OwnerdrawnMenus"), TRUE);
	//now fill in the entries 
	if ((!isInSVN)&&(isFolder))
		InsertSVNMenu(ownerdrawn, HMENU(MENUCHECKOUT), INDEXMENU(MENUCHECKOUT), idCmd++, IDS_MENUCHECKOUT, IDI_CHECKOUT, idCmdFirst, Checkout);

	if (isInSVN)
		InsertSVNMenu(ownerdrawn, HMENU(MENUUPDATE), INDEXMENU(MENUUPDATE), idCmd++, IDS_MENUUPDATE, IDI_UPDATE, idCmdFirst, Update);

	if (isInSVN)
		InsertSVNMenu(ownerdrawn, HMENU(MENUCOMMIT), INDEXMENU(MENUCOMMIT), idCmd++, IDS_MENUCOMMIT, IDI_COMMIT, idCmdFirst, Commit);
	
	//---- separator 
	if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((isInSVN)&&(!isNormal)&&(isOnlyOneItemSelected)&&(!isFolder))
		InsertSVNMenu(ownerdrawn, HMENU(MENUDIFF),INDEXMENU(MENUDIFF), idCmd++, IDS_MENUDIFF, IDI_DIFF, idCmdFirst, Diff);

	if (files_.size() == 2)	//compares the two selected files
		InsertSVNMenu(ownerdrawn, HMENU(MENUDIFF), INDEXMENU(MENUDIFF), idCmd++, IDS_MENUDIFF, IDI_DIFF, idCmdFirst, Diff);

	if (((isInSVN)&&(isOnlyOneItemSelected))||((isFolder)&&(isFolderInSVN)))
		InsertSVNMenu(ownerdrawn, HMENU(MENULOG), INDEXMENU(MENULOG), idCmd++, IDS_MENULOG, IDI_LOG, idCmdFirst, Log);

	if ((isOnlyOneItemSelected)||(isFolder))
		InsertSVNMenu(ownerdrawn, HMENU(MENUREPOBROWSE), INDEXMENU(MENUREPOBROWSE), idCmd++, IDS_MENUREPOBROWSE, IDI_REPOBROWSE, idCmdFirst, RepoBrowse);
	if (((isInSVN)&&(isOnlyOneItemSelected))||((isFolder)&&(isFolderInSVN)))
		InsertSVNMenu(ownerdrawn, HMENU(MENUSHOWCHANGED), INDEXMENU(MENUSHOWCHANGED), idCmd++, IDS_MENUSHOWCHANGED, IDI_SHOWCHANGED, idCmdFirst, ShowChanged);

	//---- separator 
	if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((isInSVN)&&(isConflicted)&&(isOnlyOneItemSelected))
	{
		if(!isFolder)
			InsertSVNMenu(ownerdrawn, HMENU(MENUCONFLICTEDITOR), INDEXMENU(MENUCONFLICTEDITOR), idCmd++, IDS_MENUCONFLICT, IDI_CONFLICT, idCmdFirst, ConflictEditor);
		InsertSVNMenu(ownerdrawn, HMENU(MENURESOLVE), INDEXMENU(MENURESOLVE), idCmd++, IDS_MENURESOLVE, IDI_RESOLVE, idCmdFirst, Resolve);
	}
	if (isInSVN)
		InsertSVNMenu(ownerdrawn, HMENU(MENUUPDATEEXT), INDEXMENU(MENUUPDATEEXT), idCmd++, IDS_MENUUPDATEEXT, IDI_UPDATE, idCmdFirst, UpdateExt);
	if ((isInSVN)&&(isOnlyOneItemSelected))
		InsertSVNMenu(ownerdrawn, HMENU(MENURENAME), INDEXMENU(MENURENAME), idCmd++, IDS_MENURENAME, IDI_RENAME, idCmdFirst, Rename);
	if (isInSVN)
		InsertSVNMenu(ownerdrawn, HMENU(MENUREMOVE), INDEXMENU(MENUREMOVE), idCmd++, IDS_MENUREMOVE, IDI_DELETE, idCmdFirst, Remove);
	if (((isInSVN)&&(!isNormal))||((isFolder)&&(isFolderInSVN)))
		InsertSVNMenu(ownerdrawn, HMENU(MENUREVERT), INDEXMENU(MENUREVERT), idCmd++, IDS_MENUREVERT, IDI_REVERT, idCmdFirst, Revert);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(ownerdrawn, HMENU(MENUCLEANUP), INDEXMENU(MENUCLEANUP), idCmd++, IDS_MENUCLEANUP, IDI_CLEANUP, idCmdFirst, Cleanup);

	//---- separator 
	if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(ownerdrawn, HMENU(MENUCOPY), INDEXMENU(MENUCOPY), idCmd++, IDS_MENUBRANCH, IDI_COPY, idCmdFirst, Copy);
	if ((isInSVN)&&((isOnlyOneItemSelected)||((isFolder)&&(isFolderInSVN))))
		InsertSVNMenu(ownerdrawn, HMENU(MENUSWITCH), INDEXMENU(MENUSWITCH), idCmd++, IDS_MENUSWITCH, IDI_SWITCH, idCmdFirst, Switch);
	if ((isInSVN)&&(isOnlyOneItemSelected))
		InsertSVNMenu(ownerdrawn, HMENU(MENUMERGE), INDEXMENU(MENUMERGE), idCmd++, IDS_MENUMERGE, IDI_MERGE, idCmdFirst, Merge);
	if (isFolder)
		InsertSVNMenu(ownerdrawn, HMENU(MENUEXPORT), INDEXMENU(MENUEXPORT), idCmd++, IDS_MENUEXPORT, IDI_EXPORT, idCmdFirst, Export);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(ownerdrawn, HMENU(MENURELOCATE), INDEXMENU(MENURELOCATE), idCmd++, IDS_MENURELOCATE, IDI_RELOCATE, idCmdFirst, Relocate);

	//---- separator 
	if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((!isInSVN)&&(isFolder)&&(!isFolderInSVN))
		InsertSVNMenu(ownerdrawn, HMENU(MENUCREATEREPOS), INDEXMENU(MENUCREATEREPOS), idCmd++, IDS_MENUCREATEREPOS, IDI_CREATEREPOS, idCmdFirst, CreateRepos);
	if ((!isInSVN)&&(isInVersionedFolder))
		InsertSVNMenu(ownerdrawn, HMENU(MENUADD), INDEXMENU(MENUADD), idCmd++, IDS_MENUADD, IDI_ADD, idCmdFirst, Add);
	if ((!isInSVN)&&(isFolder))
		InsertSVNMenu(ownerdrawn, HMENU(MENUIMPORT), INDEXMENU(MENUIMPORT), idCmd++, IDS_MENUIMPORT, IDI_IMPORT, idCmdFirst, Import);
	if ((isInSVN)&&(!isFolder))
		InsertSVNMenu(ownerdrawn, HMENU(MENUBLAME), INDEXMENU(MENUBLAME), idCmd++, IDS_MENUBLAME, IDI_BLAME, idCmdFirst, Blame);
	if ((!isInSVN)&&(!isIgnored)&&(isInVersionedFolder))
		InsertSVNMenu(ownerdrawn, HMENU(MENUIGNORE), INDEXMENU(MENUIGNORE), idCmd++, IDS_MENUIGNORE, IDI_IGNORE, idCmdFirst, Ignore);

	//---- separator 
	if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	} // if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))

	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(ownerdrawn, HMENU(MENUCREATEPATCH), INDEXMENU(MENUCREATEPATCH), idCmd++, IDS_MENUCREATEPATCH, IDI_CREATEPATCH, idCmdFirst, CreatePatch);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(ownerdrawn, HMENU(MENUAPPLYPATCH), INDEXMENU(MENUAPPLYPATCH), idCmd++, IDS_MENUAPPLYPATCH, IDI_PATCH, idCmdFirst, ApplyPatch);

	//---- separator 
	if ((idCmd != (lastSeparator + 1)) && (indexSubMenu != 0))
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}
	InsertSVNMenu(ownerdrawn, subMenu, indexSubMenu++, idCmd++, IDS_MENUHELP, IDI_HELP, idCmdFirst, Help);
	InsertSVNMenu(ownerdrawn, subMenu, indexSubMenu++, idCmd++, IDS_MENUSETTINGS, IDI_SETTINGS, idCmdFirst, Settings);
	InsertSVNMenu(ownerdrawn, subMenu, indexSubMenu++, idCmd++, IDS_MENUABOUT, IDI_ABOUT, idCmdFirst, About);


	//add submenu to main context menu
	//don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
	//see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
	MENUITEMINFO menuiteminfo;
	ZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	OSVERSIONINFOEX inf;
	ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	if ((ownerdrawn)&&(fullver >= 0x0501))
		menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
	else
		menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_DATA;
	menuiteminfo.fType = MFT_OWNERDRAW;
 	menuiteminfo.dwTypeData = _T("TortoiseSVN\0\0");
	menuiteminfo.cch = _tcslen(menuiteminfo.dwTypeData);
	HBITMAP bmp = IconToBitmap(IDI_MENU, (COLORREF)GetSysColor(COLOR_MENU));
	menuiteminfo.hbmpChecked = bmp;
	menuiteminfo.hbmpUnchecked = bmp;
	menuiteminfo.hSubMenu = subMenu;
	menuiteminfo.wID = idCmd;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);
	myIDMap[idCmd - idCmdFirst] = SubMenu;
	myIDMap[idCmd++] = SubMenu;

	//separator after
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	//return number of menu items added
	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
//#endregion
}


// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	PreserveChdir preserveChdir;
	HRESULT hr = E_INVALIDARG;
	if (lpcmi == NULL)
		return hr;
	UINT idCmd = LOWORD(lpcmi->lpVerb);

	std::string command;
	std::string parent;
	std::string file;

	if (!HIWORD(lpcmi->lpVerb))
	{
		if ((files_.size() > 0)||(folder_.size() > 0))
		{
			UINT idCmd = LOWORD(lpcmi->lpVerb);

			// See if we have a handler interface for this id
			if (myIDMap.find(idCmd) != myIDMap.end())
			{
				STARTUPINFO startup;
				PROCESS_INFORMATION process;
				memset(&startup, 0, sizeof(startup));
				startup.cb = sizeof(startup);
				memset(&process, 0, sizeof(process));
				CRegStdString tortoiseProcPath(_T("Software\\TortoiseSVN\\ProcPath"), _T("TortoiseProc.exe"), false, HKEY_LOCAL_MACHINE);
				CRegStdString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T("TortoiseMerge.exe"), false, HKEY_LOCAL_MACHINE);

				//TortoiseProc expects a command line of the form:
				//"/command:<commandname> /path:<path> /revstart:<revisionstart> /revend:<revisionend>
				//path is either a path to a single file/directory for commands which only act on single files (log, checkout, ...)
				//or a path to a temporary file which contains a list of filepaths
				stdstring svnCmd = _T(" /command:");
				stdstring tempfile;
				switch (myIDMap[idCmd])
				{
					//#region case
					case Checkout:
						svnCmd += _T("checkout /path:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Update:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("update /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case UpdateExt:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("update /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\" /rev");
						break;
					case Commit:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("commit /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Add:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("add /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Ignore:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("ignore /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Revert:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("revert /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Cleanup:
						svnCmd += _T("cleanup /path:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Resolve:
						svnCmd += _T("resolve /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Switch:
						svnCmd += _T("switch /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Import:
						svnCmd += _T("import /path:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Export:
						svnCmd += _T("export /path:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case About:
						svnCmd += _T("about");
						break;
					case CreateRepos:
						svnCmd += _T("repocreate /path:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Merge:
						svnCmd += _T("merge /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Copy:
						svnCmd += _T("copy /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Settings:
						svnCmd += _T("settings");
						break;
					case Help:
						svnCmd += _T("help");
						break;
					case Rename:
						svnCmd += _T("rename /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Remove:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("remove /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						break;
					case Diff:
						svnCmd += _T("diff /path:\"");
						if (files_.size() == 1)
							svnCmd += files_.front().c_str();
						else if (files_.size() == 2)
						{
							std::vector<stdstring>::iterator I = files_.begin();
							svnCmd += I->c_str();
							I++;
							svnCmd += _T("\" /path2:\"");
							svnCmd += I->c_str();
						}
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case DropCopyAdd:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropcopyadd /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"";)
						break;
					case DropCopy:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropcopy /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"";)
						break;
					case DropMove:
						tempfile = WriteFileListToTempFile();
						svnCmd += _T("dropmove /path:\"");
						svnCmd += tempfile;
						svnCmd += _T("\"");
						svnCmd += _T(" /droptarget:\"");
						svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Log:
						svnCmd += _T("log /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case ConflictEditor:
						svnCmd += _T("conflicteditor /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Relocate:
						svnCmd += _T("relocate /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case ShowChanged:
						svnCmd += _T("repostatus /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case RepoBrowse:
						svnCmd += _T("repobrowser /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case Blame:
						svnCmd += _T("blame /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case CreatePatch:
						svnCmd += _T("createpatch /path:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						break;
					case ApplyPatch:
						svnCmd = _T(" /patchpath:\"");
						if (files_.size() > 0)
							svnCmd += files_.front().c_str();
						else
							svnCmd += folder_.c_str();
						svnCmd += _T("\"");
						myIDMap.clear();
						if (CreateProcess(tortoiseMergePath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
						{
							LPVOID lpMsgBuf;
							FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
								FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL 
								);
							MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
							LocalFree( lpMsgBuf );
						} // if (CreateProcess(tortoiseMergePath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 
						return NOERROR;
						break;
					default:
						break;
					//#endregion
				} // switch (myIDMap[idCmd])
				svnCmd += _T(" /hwnd:");
				TCHAR buf[30];
				_stprintf(buf, _T("%d"), lpcmi->hwnd);
				svnCmd += buf;
				myIDMap.clear();
				if (CreateProcess(tortoiseProcPath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
				{
					LPVOID lpMsgBuf;
					FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
									FORMAT_MESSAGE_FROM_SYSTEM | 
									FORMAT_MESSAGE_IGNORE_INSERTS,
									NULL,
									GetLastError(),
									MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
									(LPTSTR) &lpMsgBuf,
									0,
									NULL 
									);
					MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
					LocalFree( lpMsgBuf );
				} // if (CreateProcess(tortoiseProcPath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 
				hr = NOERROR;
			} // if (myIDMap.find(idCmd) != myIDMap.end()) 
		} // if ((files_.size() > 0)||(folder_.size() > 0)) 
	} // if (!HIWORD(lpcmi->lpVerb)) 
	return hr;

}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString(UINT idCmd,
                                         UINT uFlags,
                                         UINT FAR *reserved,
                                         LPSTR pszName,
                                         UINT cchMax)
{   
	//do we know the id?
	if (myIDMap.find(idCmd) == myIDMap.end())
	{
		return E_INVALIDARG;		//no, we don't
	}

	LoadLangDll();
	HRESULT hr = E_INVALIDARG;
	switch (myIDMap[idCmd])
	{
		case Checkout:
			MAKESTRING(IDS_MENUDESCCHECKOUT);
			break;
		case Update:
			MAKESTRING(IDS_MENUDESCUPDATE);
			break;
		case UpdateExt:
			MAKESTRING(IDS_MENUDESCUPDATEEXT);
			break;
		case Commit:
			MAKESTRING(IDS_MENUDESCCOMMIT);
			break;
		case Add:
			MAKESTRING(IDS_MENUDESCADD);
			break;
		case Revert:
			MAKESTRING(IDS_MENUDESCREVERT);
			break;
		case Cleanup:
			MAKESTRING(IDS_MENUDESCCLEANUP);
			break;
		case Resolve:
			MAKESTRING(IDS_MENUDESCRESOLVE);
			break;
		case Switch:
			MAKESTRING(IDS_MENUDESCSWITCH);
			break;
		case Import:
			MAKESTRING(IDS_MENUDESCIMPORT);
			break;
		case Export:
			MAKESTRING(IDS_MENUDESCEXPORT);
			break;
		case About:
			MAKESTRING(IDS_MENUDESCABOUT);
			break;
		case Help:
			MAKESTRING(IDS_MENUDESCHELP);
			break;
		case CreateRepos:
			MAKESTRING(IDS_MENUDESCCREATEREPOS);
			break;
		case Copy:
			MAKESTRING(IDS_MENUDESCCOPY);
			break;
		case Merge:
			MAKESTRING(IDS_MENUDESCMERGE);
			break;
		case Settings:
			MAKESTRING(IDS_MENUDESCSETTINGS);
			break;
		case Rename:
			MAKESTRING(IDS_MENUDESCRENAME);
			break;
		case Remove:
			MAKESTRING(IDS_MENUDESCREMOVE);
			break;
		case Diff:
			MAKESTRING(IDS_MENUDESCDIFF);
			break;
		case Log:
			MAKESTRING(IDS_MENUDESCLOG);
			break;
		case ConflictEditor:
			MAKESTRING(IDS_MENUDESCCONFLICT);
			break;
		case Relocate:
			MAKESTRING(IDS_MENUDESCRELOCATE);
			break;
		case ShowChanged:
			MAKESTRING(IDS_MENUDESCSHOWCHANGED);
			break;
		case RepoBrowse:
			MAKESTRING(IDS_MENUDESCREPOBROWSE);
			break;
		case Ignore:
			MAKESTRING(IDS_MENUDESCIGNORE);
			break;
		case Blame:
			MAKESTRING(IDS_MENUDESCBLAME);
			break;
		case ApplyPatch:
			MAKESTRING(IDS_MENUDESCAPPLYPATCH);
			break;
		case CreatePatch:
			MAKESTRING(IDS_MENUDESCCREATEPATCH);
			break;
		default:
			MAKESTRING(IDS_MENUDESCDEFAULT);
			break;
	} // switch (myIDMap[idCmd])
	const TCHAR * desc = stringtablebuffer;
	switch(uFlags)
	{
#ifdef UNICODE
	case GCS_HELPTEXTA:
		{
			std::string help = WideToMultibyte(desc);
			lstrcpynA(pszName, help.c_str(), cchMax);
			hr = S_OK;
			break; 
		}
	case GCS_HELPTEXTW: 
		{
			wide_string help = desc;
			lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax); 
			hr = S_OK;
			break; 
		}
#else
	case GCS_HELPTEXTA:
		{
			std::string help = desc;
			lstrcpynA(pszName, help.c_str(), cchMax);
			hr = S_OK;
			break; 
		}
	case GCS_HELPTEXTW: 
		{
			wide_string help = MultibyteToWide(desc);
			lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax); 
			hr = S_OK;
			break; 
		}
#endif
	}
	return hr;
}

STDMETHODIMP CShellExt::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT res;
   return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
//a great tutorial on owner drawn menus in shell extension can be found
//here: http://www.codeproject.com/shell/shellextguide7.asp

	LRESULT res;
	if (pResult == NULL)
		pResult = &res;
	*pResult = FALSE;

	LoadLangDll();
	switch (uMsg)
	{
		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* lpmis = (MEASUREITEMSTRUCT*)lParam;
			POINT size;
			//get the information about the shell dc, font, ...
			NONCLIENTMETRICS ncm;
			ncm.cbSize = sizeof(NONCLIENTMETRICS);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
			HDC hDC = CreateCompatibleDC(NULL);
			HFONT hFont = CreateFontIndirect(&ncm.lfMenuFont);
			HFONT oldFont = (HFONT)SelectObject(hDC, hFont);
			GetMenuTextFromResource(myIDMap[lpmis->itemID]);
			GetTextExtentPoint32(hDC, stringtablebuffer, _tcslen(stringtablebuffer), (SIZE*)&size);
			LPtoDP(hDC, &size, 1);
			//now release the font and dc
			SelectObject(hDC, oldFont);
			DeleteObject(hFont);
			DeleteDC(hDC);

			lpmis->itemWidth = size.x + size.y + 6;						//width of string + height of string (~ width of icon) + space between
			lpmis->itemHeight = max(size.y + 4, ncm.iMenuHeight);		//two pixels on both sides
			*pResult = TRUE;
		}
		break;
		case WM_DRAWITEM:
		{
			LPCTSTR resource;
			TCHAR *szItem;
			DRAWITEMSTRUCT* lpdis = (DRAWITEMSTRUCT*)lParam;
			if (lpdis->CtlType != ODT_MENU)
				return S_OK;		//not for a menu
			resource = GetMenuTextFromResource(myIDMap[lpdis->itemID]);
			if (resource == NULL)
				return S_OK;
			szItem = stringtablebuffer;
			if (lpdis->itemAction & (ODA_DRAWENTIRE|ODA_SELECT))
			{
				int ix, iy;
				RECT rt, rtTemp;
				SIZE size;
				//choose text colors
				if (lpdis->itemState & ODS_SELECTED)
				{
					COLORREF crText;
					if (lpdis->itemState & ODS_GRAYED)
						crText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_GRAYTEXT)); //RGB(128, 128, 128));
					else
						crText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
					SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
				}
				CopyRect(&rtTemp, &(lpdis->rcItem));

				ix = lpdis->rcItem.left + space;
				SetRect(&rt, ix, rtTemp.top, ix + 16, rtTemp.bottom);

				HICON hIcon = (HICON)LoadImage(GetModuleHandle(_T("TortoiseSVN")), resource, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

				//convert the icon into a bitmap
				ICONINFO ii;
				GetIconInfo(hIcon, &ii);
				HBITMAP hbmItem = ii.hbmColor; 
				if (hbmItem)
				{
					BITMAP bm;
					GetObject(hbmItem, sizeof(bm), &bm);

					int tempY = lpdis->rcItem.top + ((lpdis->rcItem.bottom - lpdis->rcItem.top) - bm.bmHeight) / 2;
					SetRect(&rt, ix, tempY, ix + 16, tempY + 16);

					ExtTextOut(lpdis->hDC, 0, 0, ETO_CLIPPED|ETO_OPAQUE, &rtTemp, NULL, 0, (LPINT)NULL);
					rtTemp.left = rt.right;

					HDC hDCTemp = CreateCompatibleDC(lpdis->hDC);
					SelectObject(hDCTemp, hbmItem);
					DrawIconEx(lpdis->hDC, rt.left , rt.top, hIcon, bm.bmWidth, bm.bmHeight,
						0, NULL, DI_NORMAL);

					DeleteDC(hDCTemp);
					DeleteObject(hbmItem);
				} // if (hbmItem)
				ix = rt.right + space;

				//free memory
				DeleteObject(ii.hbmColor);
				DeleteObject(ii.hbmMask);
				DestroyIcon(hIcon);

				//draw menu text
				GetTextExtentPoint32(lpdis->hDC, szItem, lstrlen(szItem), &size);
				iy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - size.cy) / 2;
				iy = lpdis->rcItem.top + (iy>=0 ? iy : 0);
				SetRect(&rt, ix , iy, lpdis->rcItem.right - 4, lpdis->rcItem.bottom);
				ExtTextOut(lpdis->hDC, ix, iy, ETO_CLIPPED|ETO_OPAQUE, &rtTemp, NULL, 0, (LPINT)NULL);
				if (lpdis->itemState & ODS_GRAYED)
				{        
					SetBkMode(lpdis->hDC, TRANSPARENT);
					OffsetRect( &rt, 1, 1 );
					SetTextColor( lpdis->hDC, RGB(255,255,255) );
					DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, DT_LEFT|DT_EXPANDTABS );
					OffsetRect( &rt, -1, -1 );
					SetTextColor( lpdis->hDC, RGB(128,128,128) );
					DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, DT_LEFT|DT_EXPANDTABS );         
				}
				else
					DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, DT_LEFT|DT_EXPANDTABS );
			}
			*pResult = TRUE;
		}
		break;
		default:
		return NOERROR;
	}

	return NOERROR;
}

LPCTSTR CShellExt::GetMenuTextFromResource(int id)
{
	LPCTSTR resource = NULL;
	DWORD layout = g_ShellCache.GetMenuLayout();
#define SETSPACE(x) space = ((layout & (x)) ? 0 : 6)
	switch (id)
	{
		case SubMenu:
			MAKESTRING(IDS_MENUSUBMENU);
			resource = MAKEINTRESOURCE(IDI_MENU);
			space = 0;
			break;
		case Checkout:
			MAKESTRING(IDS_MENUCHECKOUT);
			resource = MAKEINTRESOURCE(IDI_CHECKOUT);
			SETSPACE(MENUCHECKOUT);
			break;
		case Update:
			MAKESTRING(IDS_MENUUPDATE);
			resource = MAKEINTRESOURCE(IDI_UPDATE);
			SETSPACE(MENUUPDATE);
			break;
		case UpdateExt:
			MAKESTRING(IDS_MENUUPDATEEXT);
			resource = MAKEINTRESOURCE(IDI_UPDATE);
			SETSPACE(MENUUPDATEEXT);
			break;
		case Log:
			MAKESTRING(IDS_MENULOG);
			resource = MAKEINTRESOURCE(IDI_LOG);
			SETSPACE(MENULOG);
			break;
		case Commit:
			MAKESTRING(IDS_MENUCOMMIT);
			resource = MAKEINTRESOURCE(IDI_COMMIT);
			SETSPACE(MENUCOMMIT);
			break;
		case CreateRepos:
			MAKESTRING(IDS_MENUCREATEREPOS);
			resource = MAKEINTRESOURCE(IDI_CREATEREPOS);
			SETSPACE(MENUCREATEREPOS);
			break;
		case Add:
			MAKESTRING(IDS_MENUADD);
			resource = MAKEINTRESOURCE(IDI_ADD);
			SETSPACE(MENUADD);
			break;
		case Revert:
			MAKESTRING(IDS_MENUREVERT);
			resource = MAKEINTRESOURCE(IDI_REVERT);
			SETSPACE(MENUREVERT);
			break;
		case Cleanup:
			MAKESTRING(IDS_MENUCLEANUP);
			resource = MAKEINTRESOURCE(IDI_CLEANUP);
			SETSPACE(MENUCLEANUP);
			break;
		case Resolve:
			MAKESTRING(IDS_MENURESOLVE);
			resource = MAKEINTRESOURCE(IDI_RESOLVE);
			SETSPACE(MENURESOLVE);
			break;
		case Switch:
			MAKESTRING(IDS_MENUSWITCH);
			resource = MAKEINTRESOURCE(IDI_SWITCH);
			SETSPACE(MENUSWITCH);
			break;
		case Import:
			MAKESTRING(IDS_MENUIMPORT);
			resource = MAKEINTRESOURCE(IDI_IMPORT);
			SETSPACE(MENUIMPORT);
			break;
		case Export:
			MAKESTRING(IDS_MENUEXPORT);
			resource = MAKEINTRESOURCE(IDI_EXPORT);
			SETSPACE(MENUEXPORT);
			break;
		case Copy:
			MAKESTRING(IDS_MENUBRANCH);
			resource = MAKEINTRESOURCE(IDI_COPY);
			SETSPACE(MENUSWITCH);
			break;
		case Merge:
			MAKESTRING(IDS_MENUMERGE);
			resource = MAKEINTRESOURCE(IDI_MERGE);
			SETSPACE(MENUMERGE);
			break;
		case Remove:
			MAKESTRING(IDS_MENUREMOVE);
			resource = MAKEINTRESOURCE(IDI_DELETE);
			SETSPACE(MENUREMOVE);
			break;
		case Rename:
			MAKESTRING(IDS_MENURENAME);
			resource = MAKEINTRESOURCE(IDI_RENAME);
			SETSPACE(MENURENAME);
			break;
		case Diff:
			MAKESTRING(IDS_MENUDIFF);
			resource = MAKEINTRESOURCE(IDI_DIFF);
			SETSPACE(MENUDIFF);
			break;
		case ConflictEditor:
			MAKESTRING(IDS_MENUCONFLICT);
			resource = MAKEINTRESOURCE(IDI_CONFLICT);
			SETSPACE(MENUCONFLICTEDITOR);
			break;
		case Relocate:
			MAKESTRING(IDS_MENURELOCATE);
			resource = MAKEINTRESOURCE(IDI_RELOCATE);
			SETSPACE(MENURELOCATE);
			break;
		case Settings:
			MAKESTRING(IDS_MENUSETTINGS);
			resource = MAKEINTRESOURCE(IDI_SETTINGS);
			break;
		case About:
			MAKESTRING(IDS_MENUABOUT);
			resource = MAKEINTRESOURCE(IDI_ABOUT);
			break;
		case Help:
			MAKESTRING(IDS_MENUHELP);
			resource = MAKEINTRESOURCE(IDI_HELP);
			break;
		case ShowChanged:
			MAKESTRING(IDS_MENUSHOWCHANGED);
			resource = MAKEINTRESOURCE(IDI_SHOWCHANGED);
			SETSPACE(MENUSHOWCHANGED);
			break;
		case RepoBrowse:
			MAKESTRING(IDS_MENUREPOBROWSE);
			resource = MAKEINTRESOURCE(IDI_REPOBROWSE);
			SETSPACE(MENUREPOBROWSE);
			break;
		case Ignore:
			MAKESTRING(IDS_MENUIGNORE);
			resource = MAKEINTRESOURCE(IDI_IGNORE);
			SETSPACE(MENUIGNORE);
			break;
		case Blame:
			MAKESTRING(IDS_MENUBLAME);
			resource = MAKEINTRESOURCE(IDI_BLAME);
			SETSPACE(MENUBLAME);
			break;
		case ApplyPatch:
			MAKESTRING(IDS_MENUAPPLYPATCH);
			resource = MAKEINTRESOURCE(IDI_PATCH);
			SETSPACE(MENUAPPLYPATCH);
			break;
		case CreatePatch:
			MAKESTRING(IDS_MENUCREATEPATCH);
			resource = MAKEINTRESOURCE(IDI_CREATEPATCH);
			SETSPACE(MENUCREATEPATCH);
			break;
		default:
			return NULL;
	}
	return resource;
}


