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
			// Enumerate PIDLs which the user has selected
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
					if (status != svn_wc_status_unversioned)
						isInSVN = true;
					if (status == svn_wc_status_normal)
						isNormal = true;
					if (status == svn_wc_status_conflicted)
						isConflicted = true;
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
		if (status != svn_wc_status_unversioned)
		{
			isFolderInSVN = true;
			isInSVN = true;
		}
		isFolder = true;
	}
	if (files_.size() == 1)
	{
		isOnlyOneItemSelected = true;
		if (PathIsDirectory(files_.front().c_str()))
		{
			folder_ = files_.front();
			svn_wc_status_kind status = SVNStatus::GetAllStatus(folder_.c_str());
			if (status != svn_wc_status_unversioned)
				isFolderInSVN = true;
			isFolder = true;
		}
	}

	return NOERROR;
}

void CShellExt::InsertSVNMenu(HMENU menu, UINT pos, UINT flags, UINT_PTR id, UINT stringid, UINT idCmdFirst, SVNCommands com)
{
	MAKESTRING(stringid);
	InsertMenu(menu, pos, flags, id, stringtablebuffer);
	// We store the relative and absolute diameter
	// (drawitem callback uses absolute, others relative)
	myIDMap[id - idCmdFirst] = com;
	myIDMap[id] = com;
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
		if ((!isInSVN)&&(isFolderInSVN))
			InsertSVNMenu(hMenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst, IDS_DROPCOPYADDMENU, idCmdFirst, DropCopyAdd);
		else if ((isInSVN)&&(isFolderInSVN))
			InsertSVNMenu(hMenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst, IDS_DROPMOVEMENU, idCmdFirst, DropMove);

		return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, 1));;
	}

//missing commands:
//Create Patch		//to create patch files
//Apply Patch		//whats the complement of Diff?

//#region menu
	PreserveChdir preserveChdir;

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return NOERROR;					//we don't change the default action

	if ((files_.size() == 0)&&(folder_.size() == 0))
		return NOERROR;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return NOERROR;
	//check if our menu is requested for the start menu
	TCHAR buf[MAX_PATH];
	SHGetSpecialFolderPath(NULL, buf, CSIDL_STARTMENU, FALSE);
	if (_tcscmp(buf, folder_.c_str())==0)
		return NOERROR;

	bool extended = ((uFlags & CMF_EXTENDEDVERBS)!=0);		//true if shift was pressed for the context menu
	UINT idCmd = idCmdFirst;

	////separator before
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	//now fill in the entries 
	if ((!isInSVN)&&(isFolder))
		InsertSVNMenu(hMenu, indexMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUCHECKOUT, idCmdFirst, Checkout);
	if ((isInSVN)&&(!extended))
		InsertSVNMenu(hMenu, indexMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUUPDATE, idCmdFirst, Update);
	if (isInSVN)
		InsertSVNMenu(hMenu, indexMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUCOMMIT, idCmdFirst, Commit);
	
	//create the submenu
	HMENU subMenu = CreateMenu();
	int indexSubMenu = 0;
	int lastSeparator = 0;

	if (isInSVN)
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUUPDATEEXT, idCmdFirst, UpdateExt);
	if ((isInSVN)&&(isOnlyOneItemSelected))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENURENAME, idCmdFirst, Rename);
	if (isInSVN)
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUREMOVE, idCmdFirst, Remove);
	if (((isInSVN)&&(isOnlyOneItemSelected))||((isFolder)&&(isFolderInSVN)))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENULOG, idCmdFirst, Log);

	if (idCmd != (lastSeparator + 1) && indexSubMenu != 0)
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	if ((!isInSVN)&&(isFolder)&&(!isFolderInSVN))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION, idCmd++, IDS_MENUCREATEREPOS, idCmdFirst, CreateRepos);
	if (!isInSVN)
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUADD, idCmdFirst, Add);
	if (((isInSVN)&&(!isNormal))||((isFolder)&&(isFolderInSVN)))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUREVERT, idCmdFirst, Revert);
	if ((isInSVN)&&(!isNormal)&&(isOnlyOneItemSelected)&&(!isFolder))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUDIFF, idCmdFirst, Diff);
	if (files_.size() == 2)	//compares the two selected files
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUDIFF, idCmdFirst, Diff);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUCLEANUP, idCmdFirst, Cleanup);
	if ((isInSVN)&&(isConflicted)&&(isOnlyOneItemSelected))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENURESOLVE, idCmdFirst, Resolve);
	if ((isInSVN)&&((isOnlyOneItemSelected)||((isFolder)&&(isFolderInSVN))))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUSWITCH, idCmdFirst, Switch);
	if ((isInSVN)&&(isOnlyOneItemSelected))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUMERGE, idCmdFirst, Merge);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUBRANCH, idCmdFirst, Copy);
	if ((!isInSVN)&&(isFolder))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUIMPORT, idCmdFirst, Import);
	if ((isInSVN)&&(isFolder)&&(isFolderInSVN))
		InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUEXPORT, idCmdFirst, Export);

	if (idCmd != (lastSeparator + 1) && indexSubMenu != 0)
	{
		InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
		lastSeparator = idCmd++;
	}

	InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUSETTINGS, idCmdFirst, Settings);
	InsertSVNMenu(subMenu, indexSubMenu++, MF_STRING|MF_BYPOSITION|MF_OWNERDRAW, idCmd++, IDS_MENUABOUT, idCmdFirst, About);



	//add submenu to main context menu
	//don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
	//see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
	MENUITEMINFO menuiteminfo;
	ZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
	menuiteminfo.fType = MFT_OWNERDRAW;
	menuiteminfo.dwTypeData = _T("TortoiseSVN\0\0");
	menuiteminfo.cch = _tcslen(menuiteminfo.dwTypeData);
	menuiteminfo.hSubMenu = subMenu;
	menuiteminfo.wID = idCmd;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);
	myIDMap[idCmd] = SubMenu;

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
					default:
						break;
					//#endregion
				} // switch (myIDMap[idCmd])
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
				}
				hr = NOERROR;
			}
		}
	}
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
						crText = SetTextColor(lpdis->hDC, RGB(128, 128, 128));
					else
						crText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
					SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
				}
				CopyRect(&rtTemp, &(lpdis->rcItem));

				ix = lpdis->rcItem.left + 6;
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
				}
				ix = rt.right + 6;//GetSystemMetrics(SM_CXFRAME) - 4;

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
		}
		break;
		default:
		return S_OK;
	}

	return S_OK;
}
LPCTSTR CShellExt::GetMenuTextFromResource(int id)
{
	LPCTSTR resource = NULL;

	switch (id)
	{
		case SubMenu:
			MAKESTRING(IDS_MENUSUBMENU);
			resource = MAKEINTRESOURCE(IDI_MENU);
			break;
		case Checkout:
			MAKESTRING(IDS_MENUCHECKOUT);
			resource = MAKEINTRESOURCE(IDI_CHECKOUT);
			break;
		case Update:
			MAKESTRING(IDS_MENUUPDATE);
			resource = MAKEINTRESOURCE(IDI_UPDATE);
			break;
		case UpdateExt:
			MAKESTRING(IDS_MENUUPDATEEXT);
			resource = MAKEINTRESOURCE(IDI_UPDATE);
			break;
		case Log:
			MAKESTRING(IDS_MENULOG);
			resource = MAKEINTRESOURCE(IDI_LOG);
			break;
		case Commit:
			MAKESTRING(IDS_MENUCOMMIT);
			resource = MAKEINTRESOURCE(IDI_COMMIT);
			break;
		case Add:
			MAKESTRING(IDS_MENUADD);
			resource = MAKEINTRESOURCE(IDI_ADD);
			break;
		case Revert:
			MAKESTRING(IDS_MENUREVERT);
			resource = MAKEINTRESOURCE(IDI_REVERT);
			break;
		case Cleanup:
			MAKESTRING(IDS_MENUCLEANUP);
			resource = MAKEINTRESOURCE(IDI_CLEANUP);
			break;
		case Resolve:
			MAKESTRING(IDS_MENURESOLVE);
			resource = MAKEINTRESOURCE(IDI_RESOLVE);
			break;
		case Switch:
			MAKESTRING(IDS_MENUSWITCH);
			resource = MAKEINTRESOURCE(IDI_SWITCH);
			break;
		case Import:
			MAKESTRING(IDS_MENUIMPORT);
			resource = MAKEINTRESOURCE(IDI_IMPORT);
			break;
		case Export:
			MAKESTRING(IDS_MENUEXPORT);
			resource = MAKEINTRESOURCE(IDI_EXPORT);
			break;
		case Copy:
			MAKESTRING(IDS_MENUBRANCH);
			resource = MAKEINTRESOURCE(IDI_COPY);
			break;
		case Merge:
			MAKESTRING(IDS_MENUMERGE);
			resource = MAKEINTRESOURCE(IDI_MERGE);
			break;
		case Remove:
			MAKESTRING(IDS_MENUREMOVE);
			resource = MAKEINTRESOURCE(IDI_DELETE);
			break;
		case Rename:
			MAKESTRING(IDS_MENURENAME);
			resource = MAKEINTRESOURCE(IDI_RENAME);
			break;
		case Diff:
			MAKESTRING(IDS_MENUDIFF);
			resource = MAKEINTRESOURCE(IDI_DIFF);
			break;
		case Settings:
			MAKESTRING(IDS_MENUSETTINGS);
			resource = MAKEINTRESOURCE(IDI_SETTINGS);
			break;
		case About:
			MAKESTRING(IDS_MENUABOUT);
			resource = MAKEINTRESOURCE(IDI_ABOUT);
			break;
		default:
			return NULL;
	}
	return resource;
}


