// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "ShellExt.h"
#include "ItemIDList.h"
#include "PreserveChdir.h"
#include "UnicodeUtils.h"
#include "SVNProperties.h"
#include "SVNStatus.h"

#define GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

int g_shellidlist=RegisterClipboardFormat(CFSTR_SHELLIDLIST);

CShellExt::MenuInfo CShellExt::menuInfo[] =
{
	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuCheckout,					MENUCHECKOUT,		IDI_CHECKOUT,			IDS_MENUCHECKOUT,			IDS_MENUDESCCHECKOUT,
	ITEMIS_FOLDER, ITEMIS_INSVN,	0, 0, 0, 0, 0, 0 },

	{ ShellMenuUpdate,						MENUUPDATE,			IDI_UPDATE,				IDS_MENUUPDATE,				IDS_MENUDESCUPDATE,				
	ITEMIS_INSVN,	ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuCommit,						MENUCOMMIT,			IDI_COMMIT,				IDS_MENUCOMMIT,				IDS_MENUDESCCOMMIT,
	ITEMIS_INSVN, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuDiff,						MENUDIFF,			IDI_DIFF,				IDS_MENUDIFF,				IDS_MENUDESCDIFF,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_NORMAL|ITEMIS_FOLDER|ITEMIS_EXTENDED, ITEMIS_TWO, 0, 0, 0, 0, 0 },

	{ ShellMenuUrlDiff,						MENUURLDIFF,		IDI_DIFF,				IDS_MENUURLDIFF,			IDS_MENUDESCURLDIFF,
	ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuLog,							MENULOG,			IDI_LOG,				IDS_MENULOG,				IDS_MENUDESCLOG,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_ADDED, 0, 0, 0, 0 },

	{ ShellMenuRepoBrowse,					MENUREPOBROWSE,		IDI_REPOBROWSE,			IDS_MENUREPOBROWSE,			IDS_MENUDESCREPOBROWSE,
	ITEMIS_ONLYONE|ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuShowChanged,					MENUSHOWCHANGED,	IDI_SHOWCHANGED,		IDS_MENUSHOWCHANGED,		IDS_MENUDESCSHOWCHANGED,
	ITEMIS_INSVN|ITEMIS_ONLYONE, 0, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0},

	{ ShellMenuRevisionGraph,				MENUREVISIONGRAPH,	IDI_REVISIONGRAPH,		IDS_MENUREVISIONGRAPH,		IDS_MENUDESCREVISIONGRAPH,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_ADDED, 0, 0, 0, 0},

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuConflictEditor,				MENUCONFLICTEDITOR,	IDI_CONFLICT,			IDS_MENUCONFLICT,			IDS_MENUDESCCONFLICT,
	ITEMIS_INSVN|ITEMIS_CONFLICTED, ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuResolve,						MENURESOLVE,		IDI_RESOLVE,			IDS_MENURESOLVE,			IDS_MENUDESCRESOLVE,
	ITEMIS_INSVN|ITEMIS_CONFLICTED, 0, ITEMIS_INSVN|ITEMIS_FOLDER, 0, 0, 0, 0, 0 },

	{ ShellMenuUpdateExt,					MENUUPDATEEXT,		IDI_UPDATE,				IDS_MENUUPDATEEXT,			IDS_MENUDESCUPDATEEXT,
	ITEMIS_INSVN, ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRename,						MENURENAME,			IDI_RENAME,				IDS_MENURENAME,				IDS_MENUDESCRENAME,
	ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_INVERSIONEDFOLDER, ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRemove,						MENUREMOVE,			IDI_DELETE,				IDS_MENUREMOVE,				IDS_MENUDESCREMOVE,
	ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER, ITEMIS_ADDED|ITEMIS_EXTENDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRemoveKeep,					MENUREMOVE,			IDI_DELETE,				IDS_MENUREMOVEKEEP,			IDS_MENUDESCREMOVEKEEP,
	ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER|ITEMIS_EXTENDED, ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRevert,						MENUREVERT,			IDI_REVERT,				IDS_MENUREVERT,				IDS_MENUDESCREVERT,
	ITEMIS_INSVN, ITEMIS_NORMAL, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuDelUnversioned,				MENUDELUNVERSIONED,	IDI_DELUNVERSIONED,		IDS_MENUDELUNVERSIONED,		IDS_MENUDESCDELUNVERSIONED,
	ITEMIS_FOLDER|ITEMIS_INSVN|ITEMIS_EXTENDED, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuCleanup,						MENUCLEANUP,		IDI_CLEANUP,			IDS_MENUCLEANUP,			IDS_MENUDESCCLEANUP,
	ITEMIS_INSVN|ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuLock,						MENULOCK,			IDI_LOCK,				IDS_MENU_LOCK,				IDS_MENUDESC_LOCK,
	ITEMIS_INSVN, ITEMIS_LOCKED|ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuUnlock,						MENUUNLOCK,			IDI_UNLOCK,				IDS_MENU_UNLOCK,			IDS_MENUDESC_UNLOCK,
	ITEMIS_INSVN|ITEMIS_LOCKED, 0, ITEMIS_FOLDER, ITEMIS_EXTENDED, 0, 0, 0, 0 },

	{ ShellMenuUnlockForce,					MENUUNLOCK,			IDI_UNLOCK,				IDS_MENU_UNLOCKFORCE,		IDS_MENUDESC_UNLOCKFORCE,
	ITEMIS_INSVN|ITEMIS_LOCKED, 0, ITEMIS_FOLDER|ITEMIS_EXTENDED, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuCopy,						MENUCOPY,			IDI_COPY,				IDS_MENUBRANCH,				IDS_MENUDESCCOPY,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuSwitch,						MENUSWITCH,			IDI_SWITCH,				IDS_MENUSWITCH,				IDS_MENUDESCSWITCH,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuMerge,						MENUMERGE,			IDI_MERGE,				IDS_MENUMERGE,				IDS_MENUDESCMERGE,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuExport,						MENUEXPORT,			IDI_EXPORT,				IDS_MENUEXPORT,				IDS_MENUDESCEXPORT,
	ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRelocate,					MENURELOCATE,		IDI_RELOCATE,			IDS_MENURELOCATE,			IDS_MENUDESCRELOCATE,
	ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuCreateRepos,					MENUCREATEREPOS,	IDI_CREATEREPOS,		IDS_MENUCREATEREPOS,		IDS_MENUDESCCREATEREPOS,
	ITEMIS_FOLDER, ITEMIS_INSVN|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuAdd,							MENUADD,			IDI_ADD,				IDS_MENUADD,				IDS_MENUDESCADD,
	ITEMIS_INVERSIONEDFOLDER, ITEMIS_INSVN, ITEMIS_INSVN|ITEMIS_FOLDER, 0, ITEMIS_IGNORED, 0, ITEMIS_DELETED, ITEMIS_FOLDER|ITEMIS_ONLYONE },

	{ ShellMenuAddAsReplacement,			MENUADD,			IDI_ADD,				IDS_MENUADDASREPLACEMENT,	IDS_MENUADDASREPLACEMENT,
	ITEMIS_DELETED|ITEMIS_ONLYONE, ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuImport,						MENUIMPORT,			IDI_IMPORT,				IDS_MENUIMPORT,				IDS_MENUDESCIMPORT,
	ITEMIS_FOLDER, ITEMIS_INSVN, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuBlame,						MENUBLAME,			IDI_BLAME,				IDS_MENUBLAME,				IDS_MENUDESCBLAME,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuIgnoreSub,					MENUIGNORE,			IDI_IGNORE,				IDS_MENUIGNORE,				IDS_MENUDESCIGNORE,
	ITEMIS_INVERSIONEDFOLDER, ITEMIS_IGNORED|ITEMIS_INSVN, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuUnIgnoreSub,					MENUIGNORE,			IDI_IGNORE,				IDS_MENUUNIGNORE,			IDS_MENUDESCUNIGNORE,
	ITEMIS_IGNORED, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuCreatePatch,					MENUCREATEPATCH,	IDI_CREATEPATCH,		IDS_MENUCREATEPATCH,		IDS_MENUDESCCREATEPATCH,
	ITEMIS_INSVN, ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuApplyPatch,					MENUAPPLYPATCH,		IDI_PATCH,				IDS_MENUAPPLYPATCH,			IDS_MENUDESCAPPLYPATCH,
	ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_ADDED, ITEMIS_ONLYONE|ITEMIS_PATCHFILE, 0, 0, 0, 0, 0 },

	{ ShellMenuProperties,					MENUPROPERTIES,		IDI_PROPERTIES,			IDS_MENUPROPERTIES,			IDS_MENUDESCPROPERTIES,
	ITEMIS_INSVN, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuSettings,					MENUSETTINGS,		IDI_SETTINGS,			IDS_MENUSETTINGS,			IDS_MENUDESCSETTINGS,
	ITEMIS_FOLDER, 0, 0, ITEMIS_FOLDER, 0, 0, 0, 0 },
	{ ShellMenuHelp,						MENUHELP,			IDI_HELP,				IDS_MENUHELP,				IDS_MENUDESCHELP,
	ITEMIS_FOLDER, 0, 0, ITEMIS_FOLDER, 0, 0, 0, 0 },
	{ ShellMenuAbout,						MENUABOUT,			IDI_ABOUT,				IDS_MENUABOUT,				IDS_MENUDESCABOUT,
	ITEMIS_FOLDER, 0, 0, ITEMIS_FOLDER, 0, 0, 0, 0 },

	// the submenus - they're not added like the the commands, therefore the menu ID is zero
	// but they still need to be in here, because we use the icon and string information anyway.
	{ ShellSubMenu,							NULL,				IDI_APP,				IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuFile,						NULL,				IDI_MENUFILE,			IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuFolder,					NULL,				IDI_MENUFOLDER,			IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuLink,						NULL,				IDI_MENULINK,			IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuMultiple,					NULL,				IDI_MENUMULTIPLE,		IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	// mark the last entry to tell the loop where to stop iterating over this array
	{ ShellMenuLastEntry,					0,					0,						0,							0,
	0, 0, 0, 0, 0, 0, 0, 0 },
};


STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY /* hRegKey */)
{
	ATLTRACE("Shell :: Initialize\n");
	PreserveChdir preserveChdir;
	files_.clear();
	folder_.erase();
	itemStates = 0;
	stdstring statuspath;
	svn_wc_status_kind fetchedstatus = svn_wc_status_unversioned;
	// get selected files/folders
	if (pDataObj)
	{
		STGMEDIUM medium;
		FORMATETC fmte = {(CLIPFORMAT)g_shellidlist,
			(DVTARGETDEVICE FAR *)NULL, 
			DVASPECT_CONTENT, 
			-1, 
			TYMED_HGLOBAL};
		HRESULT hres = pDataObj->GetData(&fmte, &medium);

		if (SUCCEEDED(hres) && medium.hGlobal)
		{
			if (m_State == FileStateDropHandler)
			{
				FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg = { TYMED_HGLOBAL };
				if ( FAILED( pDataObj->GetData ( &etc, &stg )))
					return E_INVALIDARG;


				HDROP drop = (HDROP)GlobalLock(stg.hGlobal);
				if ( NULL == drop )
				{
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}

				int count = DragQueryFile(drop, (UINT)-1, NULL, 0);
				if (count == 1)
					itemStates |= ITEMIS_ONLYONE;
				for (int i = 0; i < count; i++)
				{
					// find the path length in chars
					UINT len = DragQueryFile(drop, i, NULL, 0);
					if (len == 0)
						continue;
					TCHAR * szFileName = new TCHAR[len+1];
					if (0 == DragQueryFile(drop, i, szFileName, len+1))
					{
						delete szFileName;
						continue;
					}
					stdstring str = stdstring(szFileName);
					delete szFileName;
					if (str.empty() == false)
					{
						if (itemStates & ITEMIS_ONLYONE)
						{
							CTSVNPath strpath;
							strpath.SetFromWin(str.c_str());
							itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".diff"))==0) ? ITEMIS_PATCHFILE : 0;
							itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".patch"))==0) ? ITEMIS_PATCHFILE : 0;
						}
						files_.push_back(str);
						if (i == 0)
						{
							//get the Subversion status of the item
							svn_wc_status_kind status = svn_wc_status_unversioned;
							CTSVNPath askedpath;
							askedpath.SetFromWin(str.c_str());
							try
							{
								SVNStatus stat;
								stat.GetStatus(CTSVNPath(str.c_str()), false, true, true);
								if (stat.status)
								{
									statuspath = str;
									status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
									fetchedstatus = status;
									if ((stat.status->entry)&&(stat.status->entry->lock_token))
										itemStates |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
									if ((stat.status->entry)&&(stat.status->entry->present_props))
									{
										if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
											itemStates |= ITEMIS_NEEDSLOCK;
									}
								}
								else
								{
									// sometimes, svn_client_status() returns with an error.
									// in that case, we have to check if the working copy is versioned
									// anyway to show the 'correct' context menu
									if (askedpath.HasAdminDir())
										status = svn_wc_status_normal;
								}
							}
							catch ( ... )
							{
								ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
							}
							if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
								itemStates |= ITEMIS_INSVN;
							if (status == svn_wc_status_ignored)
								itemStates |= ITEMIS_IGNORED;
							if (status == svn_wc_status_normal)
								itemStates |= ITEMIS_NORMAL;
							if (status == svn_wc_status_conflicted)
								itemStates |= ITEMIS_CONFLICTED;
							if (status == svn_wc_status_added)
								itemStates |= ITEMIS_ADDED;
							if (status == svn_wc_status_deleted)
								itemStates |= ITEMIS_DELETED;
						}
					}
				} // for (int i = 0; i < count; i++)
				GlobalUnlock ( drop );
			} // if (m_State == FileStateDropHandler) 
			else
			{
				//Enumerate PIDLs which the user has selected
				CIDA* cida = (CIDA*)GlobalLock(medium.hGlobal);
				ItemIDList parent( GetPIDLFolder (cida));

				int count = cida->cidl;
				BOOL statfetched = FALSE;
				for (int i = 0; i < count; ++i)
				{
					ItemIDList child (GetPIDLItem (cida, i), &parent);
					stdstring str = child.toString();
					if (str.empty() == false)
					{
						//check if our menu is requested for a subversion admin directory
						if (g_SVNAdminDir.IsAdminDirPath(str.c_str()))
							continue;

						files_.push_back(str);
						CTSVNPath strpath;
						strpath.SetFromWin(str.c_str());
						itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".diff"))==0) ? ITEMIS_PATCHFILE : 0;
						itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".patch"))==0) ? ITEMIS_PATCHFILE : 0;
						itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".lnk"))==0) ? ITEMIS_SHORTCUT : 0;
						if (!statfetched)
						{
							//get the Subversion status of the item
							svn_wc_status_kind status = svn_wc_status_unversioned;
							try
							{
								SVNStatus stat;
								stat.GetStatus(CTSVNPath(strpath), false, true, true);
								if (stat.status)
								{
									statuspath = str;
									status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
									fetchedstatus = status;
									if ((stat.status->entry)&&(stat.status->entry->lock_token))
										itemStates |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
									if ((stat.status->entry)&&(stat.status->entry->kind == svn_node_dir))
									{
										itemStates |= ITEMIS_FOLDER;
										if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
											itemStates |= ITEMIS_FOLDERINSVN;
									}
									if ((stat.status->entry)&&(stat.status->entry->conflict_wrk))
										itemStates |= ITEMIS_CONFLICTED;
									if ((stat.status->entry)&&(stat.status->entry->present_props))
									{
										if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
											itemStates |= ITEMIS_NEEDSLOCK;
									}
								}	
								else
								{
									// sometimes, svn_client_status() returns with an error.
									// in that case, we have to check if the working copy is versioned
									// anyway to show the 'correct' context menu
									if (CTSVNPath(strpath).HasAdminDir())
										status = svn_wc_status_normal;
								}
								statfetched = TRUE;
							}
							catch ( ... )
							{
								ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
							}
							if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
								itemStates |= ITEMIS_INSVN;
							if (status == svn_wc_status_ignored)
							{
								itemStates |= ITEMIS_IGNORED;
								// the item is ignored. Get the svn:ignored properties so we can (maybe) later
								// offer a 'remove from ignored list' entry
								SVNProperties props(strpath.GetContainingDirectory());
								ignoredprops.empty();
								for (int p=0; p<props.GetCount(); ++p)
								{
									if (props.GetItemName(p).compare(stdstring(_T("svn:ignore")))==0)
									{
										std::string st = props.GetItemValue(p);
										ignoredprops = MultibyteToWide(st.c_str());
										break;
									}
								}
							}
							if (status == svn_wc_status_normal)
								itemStates |= ITEMIS_NORMAL;
							if (status == svn_wc_status_conflicted)
								itemStates |= ITEMIS_CONFLICTED;
							if (status == svn_wc_status_added)
								itemStates |= ITEMIS_ADDED;
							if (status == svn_wc_status_deleted)
								itemStates |= ITEMIS_DELETED;
						}
					}
				} // for (int i = 0; i < count; ++i)
				ItemIDList child (GetPIDLItem (cida, 0), &parent);
				if (g_ShellCache.HasSVNAdminDir(child.toString().c_str(), FALSE))
					itemStates |= ITEMIS_INVERSIONEDFOLDER;
				GlobalUnlock(medium.hGlobal);

				// if the item is a versioned folder, check if there's a patchfile
				// in the clipboard to be used in "Apply Patch"
				UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
				if (cFormat)
				{
					if (OpenClipboard(NULL))
					{
						UINT enumFormat = 0;
						do 
						{
							if (enumFormat == cFormat)
							{
								// yes, there's a patchfile in the clipboard
								itemStates |= ITEMIS_PATCHINCLIPBOARD;
								break;
							}
						} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
						CloseClipboard();
					}
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
		svn_wc_status_kind status = svn_wc_status_unversioned;
		if (folder_.compare(statuspath)!=0)
		{
			CTSVNPath askedpath;
			askedpath.SetFromWin(folder_.c_str());
			try
			{
				SVNStatus stat;
				stat.GetStatus(CTSVNPath(folder_.c_str()), false, true, true);
				if (stat.status)
				{
					status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
					if ((stat.status->entry)&&(stat.status->entry->lock_token))
						itemStates |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
					if ((stat.status->entry)&&(stat.status->entry->present_props))
					{
						if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
							itemStates |= ITEMIS_NEEDSLOCK;
					}
				}
				else
				{
					// sometimes, svn_client_status() returns with an error.
					// in that case, we have to check if the working copy is versioned
					// anyway to show the 'correct' context menu
					if (askedpath.HasAdminDir())
						status = svn_wc_status_normal;
				}
			}
			catch ( ... )
			{
				ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
			}
		}
		else
		{
			status = fetchedstatus;
		}
		if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
		{
			itemStates |= (ITEMIS_INSVN|ITEMIS_FOLDERINSVN);
		}
		if (status == svn_wc_status_ignored)
			itemStates |= ITEMIS_IGNORED;
		itemStates |= ITEMIS_FOLDER;
	}
	if (files_.size() == 2)
		itemStates |= ITEMIS_TWO;
	if ((files_.size() == 1)&&(m_State != FileStateDropHandler))
	{
		itemStates |= ITEMIS_ONLYONE;
		if ((folder_.size())&&(PathIsDirectory(files_.front().c_str())))
		{
			folder_ = files_.front();
			svn_wc_status_kind status = svn_wc_status_unversioned;
			if (folder_.compare(statuspath)!=0)
			{
				CTSVNPath askedpath;
				askedpath.SetFromWin(folder_.c_str());
				try
				{
					SVNStatus stat;
					stat.GetStatus(CTSVNPath(folder_.c_str()), false, true, true);
					if (stat.status)
					{
						status = SVNStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
						if ((stat.status->entry)&&(stat.status->entry->lock_token))
							itemStates |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
						if ((stat.status->entry)&&(stat.status->entry->present_props))
						{
							if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
								itemStates |= ITEMIS_NEEDSLOCK;
						}
					}
				}
				catch ( ... )
				{
					ATLTRACE2(_T("Exception in SVNStatus::GetAllStatus()\n"));
				}
			}
			else
			{
				status = fetchedstatus;
			}
			if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
				itemStates |= ITEMIS_FOLDERINSVN;
			if (status == svn_wc_status_ignored)
				itemStates |= ITEMIS_IGNORED;
			itemStates |= ITEMIS_FOLDER;
			if (status == svn_wc_status_added)
				itemStates |= ITEMIS_ADDED;
			if (status == svn_wc_status_deleted)
				itemStates |= ITEMIS_DELETED;
		}
	}
		
	return NOERROR;
}

void CShellExt::InsertSVNMenu(BOOL ownerdrawn, BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, SVNCommands com)
{
	TCHAR menutextbuffer[255];
	TCHAR verbsbuffer[255];
	ZeroMemory(menutextbuffer, sizeof(menutextbuffer));
	MAKESTRING(stringid);

	if (istop)
	{
		//menu entry for the top context menu, so append an "SVN " before
		//the menu text to indicate where the entry comes from
		_tcscpy_s(menutextbuffer, 255, _T("SVN "));
	}
	_tcscat_s(menutextbuffer, 255, stringtablebuffer);
	if (ownerdrawn==1) 
	{
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING | MF_OWNERDRAW, id, menutextbuffer);
	}
	else if (ownerdrawn==0)
	{
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING , id, menutextbuffer);
		HBITMAP bmp = IconToBitmap(icon, (COLORREF)GetSysColor(COLOR_MENU)); 
		SetMenuItemBitmaps(menu, pos, MF_BYPOSITION, bmp, bmp);
	}
	else
	{
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING , id, menutextbuffer);
	}
	if (istop)
	{
		//menu entry for the top context menu, so append an "SVN " before
		//the menu text to indicate where the entry comes from
		_tcscpy_s(menutextbuffer, 255, _T("SVN "));
	}
	LoadString(g_hResInst, stringid, verbsbuffer, sizeof(verbsbuffer));
	_tcscat_s(menutextbuffer, 255, verbsbuffer);
	stdstring verb = stdstring(menutextbuffer);
	if (verb.find('&') != -1)
	{
		verb.erase(verb.find('&'),1);
	}
	myVerbsMap[verb] = id - idCmdFirst;
	myVerbsMap[verb] = id;
	myVerbsIDMap[id - idCmdFirst] = verb;
	myVerbsIDMap[id] = verb;
	// We store the relative and absolute diameter
	// (drawitem callback uses absolute, others relative)
	myIDMap[id - idCmdFirst] = com;
	myIDMap[id] = com;
	if (!istop)
		mySubMenuMap[pos] = com;
}

HBITMAP CShellExt::IconToBitmap(UINT uIcon, COLORREF transparentColor)
{
	std::map<UINT, HBITMAP>::iterator bitmap_it = bitmaps.lower_bound(uIcon);
	if (bitmap_it != bitmaps.end() && bitmap_it->first == uIcon)
		return bitmap_it->second;

	HICON hIcon = (HICON)LoadImage(g_hResInst, MAKEINTRESOURCE(uIcon), IMAGE_ICON, 12, 12, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;

	RECT     rect;

	rect.right = ::GetSystemMetrics(SM_CXMENUCHECK);
	rect.bottom = ::GetSystemMetrics(SM_CYMENUCHECK);

	rect.left =
		rect.top  = 0;

	HWND desktop    = ::GetDesktopWindow();
	if (desktop == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	HDC  screen_dev = ::GetDC(desktop);
	if (screen_dev == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	// Create a compatible DC
	HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
	if (dst_hdc == NULL)
	{
		DestroyIcon(hIcon);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Create a new bitmap of icon size
	HBITMAP bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
	if (bmp == NULL)
	{
		DestroyIcon(hIcon);
		::DeleteDC(dst_hdc);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Select it into the compatible DC
	HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
	if (old_dst_bmp == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	// Fill the background of the compatible DC with the given colour
	::SetBkColor(dst_hdc, transparentColor);
	::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	// Draw the icon into the compatible DC
	::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

	// Restore settings
	::SelectObject(dst_hdc, old_dst_bmp);
	::DeleteDC(dst_hdc);
	::ReleaseDC(desktop, screen_dev); 
	DestroyIcon(hIcon);
	if (bmp)
		bitmaps.insert(bitmap_it, std::make_pair(uIcon, bmp));
	return bmp;
}

stdstring CShellExt::WriteFileListToTempFile()
{
	//write all selected files and paths to a temporary file
	//for TortoiseProc.exe to read out again.
	DWORD pathlength = GetTempPath(0, NULL);
	TCHAR * path = new TCHAR[pathlength+1];
	TCHAR * tempFile = new TCHAR[pathlength + 100];
	GetTempPath (pathlength+1, path);
	GetTempFileName (path, _T("svn"), 0, tempFile);
	stdstring retFilePath = stdstring(tempFile);
	
	HANDLE file = ::CreateFile (tempFile,
								GENERIC_WRITE, 
								FILE_SHARE_READ, 
								0, 
								CREATE_ALWAYS, 
								FILE_ATTRIBUTE_TEMPORARY,
								0);

	delete path;
	delete tempFile;
	if (file == INVALID_HANDLE_VALUE)
		return stdstring();
		
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
	return retFilePath;
}

STDMETHODIMP CShellExt::QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT &indexMenu)
{
	LoadLangDll();
	PreserveChdir preserveChdir;

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return NOERROR;					//we don't change the default action

	if ((files_.size() == 0)&&(folder_.size() == 0))
		return NOERROR;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return NOERROR;

	//the drop handler only has eight commands, but not all are visible at the same time:
	//if the source file(s) are under version control then those files can be moved
	//to the new location or they can be moved with a rename, 
	//if they are unversioned then they can be added to the working copy
	//if they are versioned, they also can be exported to an unversioned location
	UINT idCmd = idCmdFirst;

	if ((itemStates & (ITEMIS_FOLDERINSVN|ITEMIS_INSVN))&&((~itemStates) & ITEMIS_ADDED))
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVEMENU, 0, idCmdFirst, ShellMenuDropMove);
	if ((itemStates & (ITEMIS_FOLDERINSVN|ITEMIS_INSVN|ITEMIS_ONLYONE))&&((~itemStates) & ITEMIS_ADDED))
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVERENAMEMENU, 0, idCmdFirst, ShellMenuDropMoveRename);
	if ((itemStates & (ITEMIS_FOLDERINSVN|ITEMIS_INSVN))&&((~itemStates) & ITEMIS_ADDED))
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYMENU, 0, idCmdFirst, ShellMenuDropCopy);
	if ((itemStates & (ITEMIS_FOLDERINSVN|ITEMIS_INSVN|ITEMIS_ONLYONE))&&((~itemStates) & ITEMIS_ADDED))
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYRENAMEMENU, 0, idCmdFirst, ShellMenuDropCopyRename);
	if ((itemStates & (ITEMIS_FOLDERINSVN|ITEMIS_INSVN))&&((~itemStates) & ITEMIS_ADDED))
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYADDMENU, 0, idCmdFirst, ShellMenuDropCopyAdd);
	if (itemStates & ITEMIS_INSVN)
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTMENU, 0, idCmdFirst, ShellMenuDropExport);
	if (itemStates & ITEMIS_INSVN)
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTEXTENDEDMENU, 0, idCmdFirst, ShellMenuDropExportExtended);
	if (itemStates & (ITEMIS_FOLDERINSVN|ITEMIS_PATCHFILE))
		InsertSVNMenu(FALSE, FALSE, hMenu, indexMenu++, idCmd++, IDS_MENUAPPLYPATCH, 0, idCmdFirst, ShellMenuApplyPatch);
	if (idCmd != idCmdFirst)
		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 

	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT /*idCmdLast*/,
                                         UINT uFlags)
{
	ATLTRACE("Shell :: QueryContextMenu\n");
	PreserveChdir preserveChdir;
	
	//first check if our drophandler is called
	//and then (if true) provide the context menu for the
	//drop handler
	if (m_State == FileStateDropHandler)
	{
		return QueryDropContext(uFlags, idCmdFirst, hMenu, indexMenu);
	}

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return NOERROR;					//we don't change the default action

	if ((files_.size() == 0)&&(folder_.size() == 0))
		return NOERROR;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return NOERROR;

	int csidlarray[] = 
	{
		CSIDL_BITBUCKET,
		CSIDL_CDBURN_AREA,
		CSIDL_COMMON_FAVORITES,
		CSIDL_COMMON_STARTMENU,
		CSIDL_COMPUTERSNEARME,
		CSIDL_CONNECTIONS,
		CSIDL_CONTROLS,
		CSIDL_COOKIES,
		CSIDL_FAVORITES,
		CSIDL_FONTS,
		CSIDL_HISTORY,
		CSIDL_INTERNET,
		CSIDL_INTERNET_CACHE,
		CSIDL_NETHOOD,
		CSIDL_NETWORK,
		CSIDL_PRINTERS,
		CSIDL_PRINTHOOD,
		CSIDL_RECENT,
		CSIDL_SENDTO,
		CSIDL_STARTMENU,
		0
	};
	if (IsIllegalFolder(folder_, csidlarray))
		return NOERROR;

	if (folder_.empty())
	{
		// folder is empty, but maybe files are selected
		if (files_.size() == 0)
			return NOERROR;	// nothing selected - we don't have a menu to show
		// check whether a selected entry is an UID - those are namespace extensions
		// which we can't handle
		for (std::vector<stdstring>::const_iterator it = files_.begin(); it != files_.end(); ++it)
		{
			if (_tcsncmp(it->c_str(), _T("::{"), 3)==0)
				return NOERROR;
		}
	}

	//check if our menu is requested for a subversion admin directory
	if (g_SVNAdminDir.IsAdminDirPath(folder_.c_str()))
		return NOERROR;

	BOOL ownerdrawn = CRegStdWORD(_T("Software\\TortoiseSVN\\OwnerdrawnMenus"), TRUE);
	// On Vista, owner drawn menus force the context menu to the 'old' UI style.
	// Since we like improved UIs, we don't want to do that so we simply
	// don't draw the menus on Vista ourselves.
	OSVERSIONINFOEX inf;
	ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	if (fullver >= 0x0600)
		ownerdrawn = 0;

	//check if we already added our menu entry for a folder.
	//we check that by iterating through all menu entries and check if 
	//the dwItemData member points to our global ID string. That string is set
	//by our shell extension when the folder menu is inserted.
	TCHAR menubuf[MAX_PATH];
	int count = GetMenuItemCount(hMenu);
	for (int i=0; i<count; ++i)
	{
		MENUITEMINFO miif;
		ZeroMemory(&miif, sizeof(MENUITEMINFO));
		miif.cbSize = sizeof(MENUITEMINFO);
		miif.fMask = MIIM_DATA;
		miif.dwTypeData = menubuf;
		miif.cch = sizeof(menubuf)/sizeof(TCHAR);
		GetMenuItemInfo(hMenu, i, TRUE, &miif);
		if (miif.dwItemData == (ULONG_PTR)g_MenuIDString)
			return NOERROR;
	}

	LoadLangDll();
	UINT idCmd = idCmdFirst;

	//create the submenu
	HMENU subMenu = CreateMenu();
	int indexSubMenu = 0;
	int lastSeparator = 0;

	unsigned __int64 topmenu = g_ShellCache.GetMenuLayout();
	unsigned __int64 menumask = g_ShellCache.GetMenuMask();

	int menuIndex = 0;
	bool bAddSeparator = false;
	bool bMenuEntryAdded = false;
	while (menuInfo[menuIndex].command != ShellMenuLastEntry)
	{
		if (menuInfo[menuIndex].command == ShellSeparator)
		{
			// we don't add a separator immediately. Because there might not be
			// another 'normal' menu entry after we insert a separator.
			// we simply set a flag here, indicating that before the next
			// 'normal' menu entry, a separator should be added.
			bAddSeparator = true;
		}
		else
		{
			// check the conditions whether to show the menu entry or not
			bool bInsertMenu = false;

			if (menuInfo[menuIndex].firstyes && menuInfo[menuIndex].firstno)
			{
				if (((menuInfo[menuIndex].firstyes & itemStates) == menuInfo[menuIndex].firstyes)
					&&
					((menuInfo[menuIndex].firstno & (~itemStates)) == menuInfo[menuIndex].firstno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].firstyes)&&((menuInfo[menuIndex].firstyes & itemStates) == menuInfo[menuIndex].firstyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].firstno)&&((menuInfo[menuIndex].firstno & (~itemStates)) == menuInfo[menuIndex].firstno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].secondyes && menuInfo[menuIndex].secondno)
			{
				if (((menuInfo[menuIndex].secondyes & itemStates) == menuInfo[menuIndex].secondyes)
					&&
					((menuInfo[menuIndex].secondno & (~itemStates)) == menuInfo[menuIndex].secondno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].secondyes)&&((menuInfo[menuIndex].secondyes & itemStates) == menuInfo[menuIndex].secondyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].secondno)&&((menuInfo[menuIndex].secondno & (~itemStates)) == menuInfo[menuIndex].secondno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].thirdyes && menuInfo[menuIndex].thirdno)
			{
				if (((menuInfo[menuIndex].thirdyes & itemStates) == menuInfo[menuIndex].thirdyes)
					&&
					((menuInfo[menuIndex].thirdno & (~itemStates)) == menuInfo[menuIndex].thirdno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].thirdyes)&&((menuInfo[menuIndex].thirdyes & itemStates) == menuInfo[menuIndex].thirdyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].thirdno)&&((menuInfo[menuIndex].thirdno & (~itemStates)) == menuInfo[menuIndex].thirdno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].fourthyes && menuInfo[menuIndex].fourthno)
			{
				if (((menuInfo[menuIndex].fourthyes & itemStates) == menuInfo[menuIndex].fourthyes)
					&&
					((menuInfo[menuIndex].fourthno & (~itemStates)) == menuInfo[menuIndex].fourthno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].fourthyes)&&((menuInfo[menuIndex].fourthyes & itemStates) == menuInfo[menuIndex].fourthyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].fourthno)&&((menuInfo[menuIndex].fourthno & (~itemStates)) == menuInfo[menuIndex].fourthno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].menuID & (~menumask))
			{
				if (bInsertMenu)
				{
					// insert a separator
					if ((bMenuEntryAdded)&&(bAddSeparator))
					{
						bAddSeparator = false;
						bMenuEntryAdded = false;
						InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
						lastSeparator = indexSubMenu;
					}
					
					// handle special cases (submenus)
					if ((menuInfo[menuIndex].command == ShellMenuIgnoreSub)||(menuInfo[menuIndex].command == ShellMenuUnIgnoreSub))
					{
						InsertIgnoreSubmenus(idCmd, idCmdFirst, ownerdrawn, hMenu, subMenu, indexMenu, indexSubMenu, topmenu);
						bMenuEntryAdded = true;
					}
					else
					{
						// the 'get lock' command is special
						bool bIsTop = ((topmenu & menuInfo[menuIndex].menuID) != 0);
						if (menuInfo[menuIndex].command == ShellMenuLock)
						{
							if ((itemStates & ITEMIS_NEEDSLOCK) && g_ShellCache.IsGetLockTop())
								bIsTop = true;
						}
						// insert the menu entry
						InsertSVNMenu(ownerdrawn,
							bIsTop,
							bIsTop ? hMenu : subMenu,
							bIsTop ? indexMenu++ : indexSubMenu++,
							idCmd++,
							menuInfo[menuIndex].menuTextID,
							menuInfo[menuIndex].iconID,
							idCmdFirst,
							menuInfo[menuIndex].command);
						if (!bIsTop)
							bMenuEntryAdded = true;
					}
				}
			}
		}
		menuIndex++;
	}

	//add submenu to main context menu
	//don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
	//see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
	MENUITEMINFO menuiteminfo;
	ZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	if ((ownerdrawn==1)&&(fullver >= 0x0501))
		menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
	else if (ownerdrawn == 0)
		menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_DATA;
	else
		menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU| MIIM_DATA;
	menuiteminfo.fType = MFT_OWNERDRAW;
 	menuiteminfo.dwTypeData = _T("TortoiseSVN\0\0");
	menuiteminfo.cch = (UINT)min(_tcslen(menuiteminfo.dwTypeData), UINT_MAX);

	HBITMAP bmp = NULL;
	if (folder_.size())
	{
		bmp = IconToBitmap(IDI_MENUFOLDER, (COLORREF)GetSysColor(COLOR_MENU));
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuFolder;
		myIDMap[idCmd] = ShellSubMenuFolder;
		menuiteminfo.dwItemData = (ULONG_PTR)g_MenuIDString;
	}
	else if (((~itemStates) & ITEMIS_SHORTCUT) && (files_.size()==1))
	{
		bmp = IconToBitmap(IDI_MENUFILE, (COLORREF)GetSysColor(COLOR_MENU));
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuFile;
		myIDMap[idCmd] = ShellSubMenuFile;
	}
	else if ((itemStates & ITEMIS_SHORTCUT) && (files_.size()==1))
	{
		bmp = IconToBitmap(IDI_MENULINK, (COLORREF)GetSysColor(COLOR_MENU));
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuLink;
		myIDMap[idCmd] = ShellSubMenuLink;
	}
	else if (files_.size() > 1)
	{
		bmp = IconToBitmap(IDI_MENUMULTIPLE, (COLORREF)GetSysColor(COLOR_MENU));
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuMultiple;
		myIDMap[idCmd] = ShellSubMenuMultiple;
	}
	else
	{
		bmp = IconToBitmap(IDI_APP, (COLORREF)GetSysColor(COLOR_MENU));
		myIDMap[idCmd - idCmdFirst] = ShellSubMenu;
		myIDMap[idCmd] = ShellSubMenu;
	}
	menuiteminfo.hbmpChecked = bmp;
	menuiteminfo.hbmpUnchecked = bmp;
	menuiteminfo.hSubMenu = subMenu;
	menuiteminfo.wID = idCmd++;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);

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

	std::string command;
	std::string parent;
	std::string file;

	if ((files_.size() > 0)||(folder_.size() > 0))
	{
		UINT idCmd = LOWORD(lpcmi->lpVerb);

		if (HIWORD(lpcmi->lpVerb))
		{
			stdstring verb = stdstring(MultibyteToWide(lpcmi->lpVerb));
			std::map<stdstring, UINT_PTR>::const_iterator verb_it = myVerbsMap.lower_bound(verb);
			if (verb_it != myVerbsMap.end() && verb_it->first == verb)
				idCmd = verb_it->second;
		}

		// See if we have a handler interface for this id
		std::map<UINT_PTR, UINT_PTR>::const_iterator id_it = myIDMap.lower_bound(idCmd);
		if (id_it != myIDMap.end() && id_it->first == idCmd)
		{
			STARTUPINFO startup;
			PROCESS_INFORMATION process;
			memset(&startup, 0, sizeof(startup));
			startup.cb = sizeof(startup);
			memset(&process, 0, sizeof(process));
			CRegStdString tortoiseProcPath(_T("Software\\TortoiseSVN\\ProcPath"), _T("TortoiseProc.exe"), false, HKEY_LOCAL_MACHINE);
			CRegStdString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T("TortoiseMerge.exe"), false, HKEY_LOCAL_MACHINE);

			//TortoiseProc expects a command line of the form:
			//"/command:<commandname> /pathfile:<path> /startrev:<startrevision> /endrev:<endrevision> /deletepathfile
			// or
			//"/command:<commandname> /path:<path> /startrev:<startrevision> /endrev:<endrevision>
			//
			//* path is a path to a single file/directory for commands which only act on single items (log, checkout, ...)
			//* pathfile is a path to a temporary file which contains a list of filepaths
			stdstring svnCmd = _T(" /command:");
			stdstring tempfile;
			switch (id_it->second)
			{
				//#region case
			case ShellMenuCheckout:
				svnCmd += _T("checkout /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuUpdate:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("update /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuUpdateExt:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("update /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /rev");
				break;
			case ShellMenuCommit:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("commit /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuAdd:
			case ShellMenuAddAsReplacement:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("add /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuIgnore:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("ignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuIgnoreCaseSensitive:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("ignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /onlymask");
				break;
			case ShellMenuUnIgnore:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("unignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuUnIgnoreCaseSensitive:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("unignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /onlymask");
				break;
			case ShellMenuRevert:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("revert /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuDelUnversioned:
				svnCmd += _T("delunversioned /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuCleanup:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("cleanup /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuResolve:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("resolve /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuSwitch:
				svnCmd += _T("switch /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuImport:
				svnCmd += _T("import /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuExport:
				svnCmd += _T("export /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuAbout:
				svnCmd += _T("about");
				break;
			case ShellMenuCreateRepos:
				svnCmd += _T("repocreate /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuMerge:
				svnCmd += _T("merge /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuCopy:
				svnCmd += _T("copy /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuSettings:
				svnCmd += _T("settings");
				break;
			case ShellMenuHelp:
				svnCmd += _T("help");
				break;
			case ShellMenuRename:
				svnCmd += _T("rename /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuRemove:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("remove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuRemoveKeep:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("remove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /keep");
				break;
			case ShellMenuDiff:
				svnCmd += _T("diff /path:\"");
				if (files_.size() == 1)
					svnCmd += files_.front();
				else if (files_.size() == 2)
				{
					std::vector<stdstring>::iterator I = files_.begin();
					svnCmd += *I;
					I++;
					svnCmd += _T("\" /path2:\"");
					svnCmd += *I;
				}
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuUrlDiff:
				svnCmd += _T("urldiff /path:\"");
				if (files_.size() == 1)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuDropCopyAdd:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropcopyadd /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"";)
					break;
			case ShellMenuDropCopy:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropcopy /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"";)
					break;
			case ShellMenuDropCopyRename:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropcopy /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\" /rename";)
					break;
			case ShellMenuDropMove:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropmove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuDropMoveRename:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropmove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\" /rename";)
				break;
			case ShellMenuDropExport:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropexport /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuDropExportExtended:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropexport /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				svnCmd += _T(" /extended");
				break;
			case ShellMenuLog:
				svnCmd += _T("log /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuConflictEditor:
				svnCmd += _T("conflicteditor /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuRelocate:
				svnCmd += _T("relocate /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuShowChanged:
				svnCmd += _T("repostatus /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuRepoBrowse:
				svnCmd += _T("repobrowser /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuBlame:
				svnCmd += _T("blame /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuCreatePatch:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("createpatch /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuApplyPatch:
				if ((itemStates & (ITEMIS_PATCHINCLIPBOARD|ITEMIS_EXTENDED)) && ((~itemStates) & ITEMIS_PATCHFILE))
				{
					// if there's a patchfile in the clipboard, we save it
					// to a temporary file and tell TortoiseMerge to use that one
					UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
					if ((cFormat)&&(OpenClipboard(NULL)))
					{ 
						HGLOBAL hglb = GetClipboardData(cFormat); 
						LPCSTR lpstr = (LPCSTR)GlobalLock(hglb); 

						DWORD len = GetTempPath(0, NULL);
						TCHAR * path = new TCHAR[len+1];
						TCHAR * tempF = new TCHAR[len+100];
						GetTempPath (len+1, path);
						GetTempFileName (path, TEXT("tsm"), 0, tempF);
						std::wstring sTempFile = std::wstring(tempF);
						delete path;
						delete tempF;

						FILE * outFile;
						size_t patchlen = strlen(lpstr);
						_tfopen_s(&outFile, sTempFile.c_str(), _T("wb"));
						if(outFile)
						{
							size_t size = fwrite(lpstr, sizeof(char), patchlen, outFile);
							if (size == patchlen)
							{
								itemStates |= ITEMIS_PATCHFILE;
								files_.clear();
								files_.push_back(sTempFile);
							}
							fclose(outFile);
						}
						GlobalUnlock(hglb); 
						CloseClipboard(); 
					} 
				}
				if (itemStates & ITEMIS_PATCHFILE)
				{
					svnCmd = _T(" /diff:\"");
					if (files_.size() > 0)
					{
						svnCmd += files_.front();
						if (itemStates & ITEMIS_FOLDERINSVN)
						{
							svnCmd += _T("\" /patchpath:\"");
							svnCmd += folder_;
						}
					}
					else
						svnCmd += folder_;
					if (itemStates & ITEMIS_INVERSIONEDFOLDER)
						svnCmd += _T("\" /wc");
					else
						svnCmd += _T("\"");
				}
				else
				{
					svnCmd = _T(" /patchpath:\"");
					if (files_.size() > 0)
						svnCmd += files_.front();
					else
						svnCmd += folder_;
					svnCmd += _T("\"");
				}
				myIDMap.clear();
				myVerbsIDMap.clear();
				myVerbsMap.clear();
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
				}
				CloseHandle(process.hThread);
				CloseHandle(process.hProcess);
				return NOERROR;
				break;
			case ShellMenuRevisionGraph:
				svnCmd += _T("revisiongraph /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuLock:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("lock /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuUnlock:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("unlock /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuUnlockForce:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("unlock /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /force");
				break;
			case ShellMenuProperties:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("properties /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			default:
				break;
				//#endregion
			} // switch (id_it->second) 
			svnCmd += _T(" /hwnd:");
			TCHAR buf[30];
			_stprintf_s(buf, 30, _T("%d"), lpcmi->hwnd);
			svnCmd += buf;
			myIDMap.clear();
			myVerbsIDMap.clear();
			myVerbsMap.clear();
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
			CloseHandle(process.hThread);
			CloseHandle(process.hProcess);
			hr = NOERROR;
		} // if (id_it != myIDMap.end() && id_it->first == idCmd) 
	} // if ((files_.size() > 0)||(folder_.size() > 0)) 
	return hr;

}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT uFlags,
                                         UINT FAR * /*reserved*/,
                                         LPSTR pszName,
                                         UINT cchMax)
{   
	//do we know the id?
	std::map<UINT_PTR, UINT_PTR>::const_iterator id_it = myIDMap.lower_bound(idCmd);
	if (id_it == myIDMap.end() || id_it->first != idCmd)
	{
		return E_INVALIDARG;		//no, we don't
	}

	LoadLangDll();
	HRESULT hr = E_INVALIDARG;

	MAKESTRING(IDS_MENUDESCDEFAULT);
	int menuIndex = 0;
	while (menuInfo[menuIndex].command != ShellMenuLastEntry)
	{
		if (menuInfo[menuIndex].command == (SVNCommands)id_it->second)
		{
			MAKESTRING(menuInfo[menuIndex].menuDescID);
			break;
		}
		menuIndex++;
	}

	const TCHAR * desc = stringtablebuffer;
	switch(uFlags)
	{
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
	case GCS_VERBA:
		{
			std::map<UINT_PTR, stdstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
			if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
			{
				std::string help = WideToMultibyte(verb_id_it->second);
				lstrcpynA(pszName, help.c_str(), cchMax);
				hr = S_OK;
			}
		}
		break;
	case GCS_VERBW:
		{
			std::map<UINT_PTR, stdstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
			if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
			{
				wide_string help = verb_id_it->second;
				ATLTRACE("verb : %ws\n", help.c_str());
				lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax); 
				hr = S_OK;
			}
		}
		break;
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
	PreserveChdir preserveChdir;

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
			if (lpmis==NULL)
				break;
			*pResult = TRUE;
			lpmis->itemWidth = 0;
			lpmis->itemHeight = 0;
			POINT size;
			//get the information about the shell dc, font, ...
			NONCLIENTMETRICS ncm;
			ncm.cbSize = sizeof(NONCLIENTMETRICS);
			if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0)==0)
				break;
			HDC hDC = CreateCompatibleDC(NULL);
			if (hDC == NULL)
				break;
			HFONT hFont = CreateFontIndirect(&ncm.lfMenuFont);
			if (hFont == NULL)
				break;
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
			if ((lpdis==NULL)||(lpdis->CtlType != ODT_MENU))
				return S_OK;		//not for a menu
			resource = GetMenuTextFromResource(myIDMap[lpdis->itemID]);
			if (resource == NULL)
				return S_OK;
			szItem = stringtablebuffer;
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

			HICON hIcon = (HICON)LoadImage(GetModuleHandle(_T("TortoiseSVN")), resource, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

			if (hIcon == NULL)
				return S_OK;
			//convert the icon into a bitmap
			ICONINFO ii;
			if (GetIconInfo(hIcon, &ii)==FALSE)
				return S_OK;
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
			UINT uFormat = DT_LEFT|DT_EXPANDTABS;
			// only draw accelerators on the submenu!
			// Reason: we only get the WM_MENUCHAR message if the *whole* menu is ownerdrawn,
			// which the top level context menu is *not*. So drawing there the accelerators
			// is futile because they won't get used.
			int menuID = myIDMap[lpdis->itemID];
			if ((_tcsncmp(szItem, _T("SVN"), 3)==0)||(menuID == ShellSubMenu)||(menuID == ShellSubMenuFile)||(menuID == ShellSubMenuFolder)||(menuID == ShellSubMenuLink)||(menuID == ShellSubMenuMultiple))
				uFormat |= DT_HIDEPREFIX;
			else
			{
				// If the "hide keyboard cues" option is turned off, we still
				// get the ODS_NOACCEL flag! So we have to check this setting
				// manually too.
				BOOL bShowAccels = FALSE;
				SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &bShowAccels, 0);
				uFormat |= ((lpdis->itemState & ODS_NOACCEL)&&(!bShowAccels)) ? DT_HIDEPREFIX : 0;
			}
			if (lpdis->itemState & ODS_GRAYED)
			{        
				SetBkMode(lpdis->hDC, TRANSPARENT);
				OffsetRect( &rt, 1, 1 );
				SetTextColor( lpdis->hDC, RGB(255,255,255) );
				DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, uFormat );
				OffsetRect( &rt, -1, -1 );
				SetTextColor( lpdis->hDC, RGB(128,128,128) );
				DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, uFormat );         
			}
			else
				DrawText( lpdis->hDC, szItem, lstrlen(szItem), &rt, uFormat );
			*pResult = TRUE;
		}
		break;
		case WM_MENUCHAR:
		{
			LPCTSTR resource;
			TCHAR *szItem;
			if (HIWORD(wParam) != MF_POPUP)
				return NOERROR;
			int nChar = LOWORD(wParam);
			if (_istascii((wint_t)nChar) && _istupper((wint_t)nChar))
				nChar = tolower(nChar);
			// we have the char the user pressed, now search that char in all our
			// menu items
			std::vector<int> accmenus;
			for (std::map<UINT_PTR, UINT_PTR>::iterator It = mySubMenuMap.begin(); It != mySubMenuMap.end(); ++It)
			{
				resource = GetMenuTextFromResource(mySubMenuMap[It->first]);
				if (resource == NULL)
					continue;
				szItem = stringtablebuffer;
				TCHAR * amp = _tcschr(szItem, '&');
				if (amp == NULL)
					continue;
				amp++;
				int ampChar = LOWORD(*amp);
				if (_istascii((wint_t)ampChar) && _istupper((wint_t)ampChar))
					ampChar = tolower(ampChar);
				if (ampChar == nChar)
				{
					// yep, we found a menu which has the pressed key
					// as an accelerator. Add that menu to the list to
					// process later.
					accmenus.push_back(It->first);
				}
			}
			if (accmenus.size() == 0)
			{
				// no menu with that accelerator key.
				*pResult = MAKELONG(0, MNC_IGNORE);
				return NOERROR;
			}
			if (accmenus.size() == 1)
			{
				// Only one menu with that accelerator key. We're lucky!
				// So just execute that menu entry.
				*pResult = MAKELONG(accmenus[0], MNC_EXECUTE);
				return NOERROR;
			}
			if (accmenus.size() > 1)
			{
				// we have more than one menu item with this accelerator key!
				MENUITEMINFO mif;
				mif.cbSize = sizeof(MENUITEMINFO);
				mif.fMask = MIIM_STATE;
				for (std::vector<int>::iterator it = accmenus.begin(); it != accmenus.end(); ++it)
				{
					GetMenuItemInfo((HMENU)lParam, *it, TRUE, &mif);
					if (mif.fState == MFS_HILITE)
					{
						// this is the selected item, so select the next one
						++it;
						if (it == accmenus.end())
							*pResult = MAKELONG(accmenus[0], MNC_SELECT);
						else
							*pResult = MAKELONG(*it, MNC_SELECT);
						return NOERROR;
					}
				}
				*pResult = MAKELONG(accmenus[0], MNC_SELECT);
			}
		}
		break;
		default:
			return NOERROR;
	}

	return NOERROR;
}

LPCTSTR CShellExt::GetMenuTextFromResource(int id)
{
	TCHAR textbuf[255];
	LPCTSTR resource = NULL;
	unsigned __int64 layout = g_ShellCache.GetMenuLayout();
	space = 6;

	int menuIndex = 0;
	while (menuInfo[menuIndex].command != ShellMenuLastEntry)
	{
		if (menuInfo[menuIndex].command == id)
		{
			MAKESTRING(menuInfo[menuIndex].menuTextID);
			resource = MAKEINTRESOURCE(menuInfo[menuIndex].iconID);
			switch (id)
			{
			case ShellMenuLock:
				// menu lock is special because it can be set to the top
				// with a separate option in the registry
				space = ((layout & MENULOCK) || ((itemStates & ITEMIS_NEEDSLOCK) && g_ShellCache.IsGetLockTop())) ? 0 : 6;
				if ((layout & MENULOCK) || ((itemStates & ITEMIS_NEEDSLOCK) && g_ShellCache.IsGetLockTop()))
				{
					_tcscpy_s(textbuf, 255, _T("SVN "));
					_tcscat_s(textbuf, 255, stringtablebuffer);
					_tcscpy_s(stringtablebuffer, 255, textbuf);
				}
				break;
				// the sub menu entries are special because they're *always* on the top level menu
			case ShellSubMenuMultiple:
			case ShellSubMenuLink:
			case ShellSubMenuFolder:
			case ShellSubMenuFile:
			case ShellSubMenu:
				space = 0;
				break;
			default:
				space = layout & menuInfo[menuIndex].menuID ? 0 : 6;
				if (layout & (menuInfo[menuIndex].menuID)) 
				{
					_tcscpy_s(textbuf, 255, _T("SVN "));
					_tcscat_s(textbuf, 255, stringtablebuffer);
					_tcscpy_s(stringtablebuffer, 255, textbuf);
				}
				break;
			}
			return resource;
		}
		menuIndex++;
	}
	return NULL;
}

bool CShellExt::IsIllegalFolder(std::wstring folder, int * cslidarray)
{
	int i=0;
	TCHAR buf[MAX_PATH];	//MAX_PATH ok, since SHGetSpecialFolderPath doesn't return the required buffer length!
	LPITEMIDLIST pidl = NULL;
	while (cslidarray[i])
	{
		++i;
		pidl = NULL;
		if (SHGetFolderLocation(NULL, cslidarray[i-1], NULL, 0, &pidl)!=S_OK)
			continue;
		if (!SHGetPathFromIDList(pidl, buf))
		{
			// not a file system path, definitely illegal for our use
			CoTaskMemFree(pidl);
			continue;
		}
		CoTaskMemFree(pidl);
		if (_tcslen(buf)==0)
			continue;
		if (_tcscmp(buf, folder.c_str())==0)
			return true;
	}
	return false;
}

void CShellExt::InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst, BOOL ownerdrawn, HMENU hMenu, HMENU subMenu, UINT &indexMenu, int &indexSubMenu, unsigned __int64 topmenu)
{
	HMENU ignoresubmenu = NULL;
	int indexignoresub = 0;
	bool bShowIgnoreMenu = false;
	TCHAR maskbuf[MAX_PATH];		// MAX_PATH is ok, since this only holds a filename
	TCHAR ignorepath[MAX_PATH];		// MAX_PATH is ok, since this only holds a filename
	if (files_.size() == 0)
		return;
	std::vector<stdstring>::iterator I = files_.begin();
	if (_tcsrchr(I->c_str(), '\\'))
		_tcscpy_s(ignorepath, MAX_PATH, _tcsrchr(I->c_str(), '\\')+1);
	else
		_tcscpy_s(ignorepath, MAX_PATH, I->c_str());
	if ((itemStates & ITEMIS_IGNORED)&&(ignoredprops.size() > 0))
	{
		// check if the item name is ignored or the mask
		size_t p = 0;
		while ( (p=ignoredprops.find( ignorepath,p )) != -1  )
		{
			if ( (p==0 || ignoredprops[p-1]==TCHAR('\n'))
				&& (p+_tcslen(ignorepath)==ignoredprops.length() || ignoredprops[p+_tcslen(ignorepath)+1]==TCHAR('\n')) )
			{
				break;
			}
			p++;
		}
		if (p!=-1)
		{
			ignoresubmenu = CreateMenu();
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
			stdstring verb = stdstring(ignorepath);
			myVerbsMap[verb] = idCmd - idCmdFirst;
			myVerbsMap[verb] = idCmd;
			myVerbsIDMap[idCmd - idCmdFirst] = verb;
			myVerbsIDMap[idCmd] = verb;
			myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnore;
			myIDMap[idCmd++] = ShellMenuUnIgnore;
			bShowIgnoreMenu = true;
		}
		_tcscpy_s(maskbuf, MAX_PATH, _T("*"));
		if (_tcsrchr(ignorepath, '.'))
		{
			_tcscat_s(maskbuf, MAX_PATH, _tcsrchr(ignorepath, '.'));
			p = ignoredprops.find(maskbuf);
			if ((p!=-1) &&
				((ignoredprops.compare(maskbuf)==0) || (ignoredprops.find('\n', p)==p+_tcslen(maskbuf)+1) || (ignoredprops.rfind('\n', p)==p-1)))
			{
				if (ignoresubmenu==NULL)
					ignoresubmenu = CreateMenu();

				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
				stdstring verb = stdstring(maskbuf);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreCaseSensitive;
				myIDMap[idCmd++] = ShellMenuUnIgnoreCaseSensitive;
				bShowIgnoreMenu = true;
			}
		}
	}
	else
	{
		bShowIgnoreMenu = true;
		ignoresubmenu = CreateMenu();
		if (itemStates & ITEMIS_ONLYONE)
		{
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
			myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
			myIDMap[idCmd++] = ShellMenuIgnore;

			_tcscpy_s(maskbuf, MAX_PATH, _T("*"));
			if (_tcsrchr(ignorepath, '.'))
			{
				_tcscat_s(maskbuf, MAX_PATH, _tcsrchr(ignorepath, '.'));
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
				stdstring verb = stdstring(maskbuf);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitive;
				myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitive;
			}
		}
		else
		{
			MAKESTRING(IDS_MENUIGNOREMULTIPLE);
			_stprintf_s(ignorepath, MAX_PATH, stringtablebuffer, files_.size());
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
			stdstring verb = stdstring(ignorepath);
			myVerbsMap[verb] = idCmd - idCmdFirst;
			myVerbsMap[verb] = idCmd;
			myVerbsIDMap[idCmd - idCmdFirst] = verb;
			myVerbsIDMap[idCmd] = verb;
			myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
			myIDMap[idCmd++] = ShellMenuIgnore;
		}
	}

	if (bShowIgnoreMenu)
	{
		MENUITEMINFO menuiteminfo;
		ZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
		menuiteminfo.cbSize = sizeof(menuiteminfo);
		OSVERSIONINFOEX inf;
		ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
		inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		GetVersionEx((OSVERSIONINFO *)&inf);
		WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
		if ((ownerdrawn==1)&&(fullver >= 0x0501))
			menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
		else if (ownerdrawn == 0)
			menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_DATA;
		else
			menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU| MIIM_DATA;
		menuiteminfo.fType = MFT_OWNERDRAW;
		HBITMAP bmp = IconToBitmap(IDI_IGNORE, (COLORREF)GetSysColor(COLOR_MENU));
		menuiteminfo.hbmpChecked = bmp;
		menuiteminfo.hbmpUnchecked = bmp;
		menuiteminfo.hSubMenu = ignoresubmenu;
		menuiteminfo.wID = idCmd;
		ZeroMemory(stringtablebuffer, sizeof(stringtablebuffer));
		if (itemStates & ITEMIS_IGNORED)
			GetMenuTextFromResource(ShellMenuUnIgnoreSub);
		else
			GetMenuTextFromResource(ShellMenuIgnoreSub);
		menuiteminfo.dwTypeData = stringtablebuffer;
		menuiteminfo.cch = (UINT)min(_tcslen(menuiteminfo.dwTypeData), UINT_MAX);

		InsertMenuItem((topmenu & MENUIGNORE) ? hMenu : subMenu, (topmenu & MENUIGNORE) ? indexMenu++ : indexSubMenu++, TRUE, &menuiteminfo);
		if (itemStates & ITEMIS_IGNORED)
		{
			myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreSub;
			myIDMap[idCmd++] = ShellMenuUnIgnoreSub;
		}
		else
		{
			myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreSub;
			myIDMap[idCmd++] = ShellMenuIgnoreSub;
		}
	}
}


