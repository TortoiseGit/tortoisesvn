// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#pragma once

#include "Globals.h"
#include "registry.h"
#include "resource.h"
#include "ShellCache.h"
#include "RemoteCacheLink.h"

extern	UINT				g_cRefThisDll;			// Reference count of this DLL.
extern	HINSTANCE			g_hmodThisDll;			// Instance handle for this DLL
extern	CRemoteCacheLink	g_remoteCacheLink;
extern	ShellCache			g_ShellCache;			// caching of registry entries, ...
extern	CRegStdWORD			g_regLang;
extern	DWORD				g_langid;
extern	HINSTANCE			g_hResInst;
extern	stdstring			g_filepath;
extern	svn_wc_status_kind	g_filestatus;	///< holds the corresponding status to the file/dir above
extern  bool				g_readonlyoverlay;		///< whether to show the readonly overlay or not
extern	bool				g_lockedoverlay;		///< whether to show the locked overlay or not

extern bool					g_normalovlloaded;
extern bool					g_modifiedovlloaded;
extern bool					g_conflictedovlloaded;
extern bool					g_readonlyovlloaded;
extern bool					g_deletedovlloaded;
extern bool					g_lockedovlloaded;
extern bool					g_addedovlloaded;

extern	void				LoadLangDll();
extern  CComCriticalSection	g_csCacheGuard;
typedef CComCritSecLock<CComCriticalSection> AutoLocker;

// The actual OLE Shell context menu handler
/**
 * \ingroup TortoiseShell
 * The main class of our COM object / Shell Extension.
 * It contains all Interfaces we implement for the shell to use.
 * \remark The implementations of the different interfaces are
 * split into several *.cpp files to keep them in a reasonable size.
 *
 * \par requirements
 * Version 4.00 or later of Shell32.dll (WinNT or later, Win95 or later)
 * Internet Explorer Version 5.0 or higher
 * Note: some functions are disabled and/or reduced for older systems.
 *
 * \version 1.0
 * first version
 *
 * \date 10-12-2002
 *
 * \author Stefan Kueng, Tim Kemp
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 *
 */
class CShellExt : public IContextMenu3,
							IPersistFile,
							IColumnProvider,
							IShellExtInit,
							IShellIconOverlayIdentifier,
							IShellPropSheetExt

// COMPILER ERROR? You need the latest version of the
// platform SDK which has references to IColumnProvider 
// in the header files.  Download it here:
// http://www.microsoft.com/msdownload/platformsdk/sdkupdate/
{
protected:

	enum SVNCommands
	{
		SubMenu = 1,
		Checkout,
		Update,
		Commit,
		Add,
		AddAsReplacement,
		Revert,
		Cleanup,
		Resolve,
		Switch,
		Import,
		Export,
		About,
		CreateRepos,
		Copy,
		Merge,
		Settings,
		Remove,
		Rename,
		UpdateExt,
		Diff,
		DropCopyAdd,
		DropMoveAdd,
		DropMove,
		DropCopy,
		DropCopyRename,
		DropExport,
		DropExportExtended,
		Log,
		ConflictEditor,
		Relocate,
		Help,
		ShowChanged,
		IgnoreSub,
		Ignore,
		IgnoreFile,
		IgnoreCaseSensitive,
		IgnoreCaseInsensitive,
		RepoBrowse,
		Blame,
		ApplyPatch,
		CreatePatch,
		RevisionGraph,
		UnIgnoreSub,
		UnIgnoreCaseSensitive,
		UnIgnore,
		Lock,
		Unlock,
		UnlockForce
	};

	FileState m_State;
	ULONG	m_cRef;
	//std::map<int,std::string> verbMap;
	std::map<UINT_PTR, int>	myIDMap;
	std::map<UINT_PTR, int>	mySubMenuMap;
	std::map<stdstring, int> myVerbsMap;
	std::map<UINT_PTR, stdstring> myVerbsIDMap;
	stdstring	folder_;
	std::vector<stdstring> files_;
	bool isOnlyOneItemSelected;
	bool isInSVN;
	bool isIgnored;
	bool isConflicted;
	bool isFolder;
	bool isInVersionedFolder;
	bool isFolderInSVN;
	bool isNormal;
	bool isAdded;
	bool isDeleted;
	bool isLocked;
	bool isPatchFile;
	int space;
	TCHAR stringtablebuffer[255];
	stdstring columnfilepath;		///< holds the last file/dir path for the column provider
	stdstring columnauthor;			///< holds the corresponding author of the file/dir above
	stdstring itemurl;
	stdstring itemshorturl;
	stdstring ignoredprops;
	stdstring owner;
	svn_revnum_t columnrev;			///< holds the corresponding revision to the file/dir above
	svn_wc_status_kind	filestatus;
	std::map<UINT, HBITMAP> bitmaps;
#define MAKESTRING(ID) LoadStringEx(g_hResInst, ID, stringtablebuffer, sizeof(stringtablebuffer)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)))
//#define MAKESTRING(ID) LoadString(g_hResInst, ID, stringtablebuffer, sizeof(stringtablebuffer))
private:
	void InsertSVNMenu(BOOL ownerdrawn, BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, SVNCommands com);
	stdstring WriteFileListToTempFile();
	LPCTSTR GetMenuTextFromResource(int id);
	void GetColumnStatus(const TCHAR * path, BOOL bIsDir);
	HBITMAP IconToBitmap(UINT hIcon, COLORREF transparentColor);
	int GetInstalledOverlays();		///< returns the maximum number of overlays TSVN shall use
public:
	CShellExt(FileState state);
	virtual ~CShellExt();

	/** \name IUnknown 
	 * IUnknown members
	 */
	//@{
	STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	//@}

	/** \name IContextMenu2 
	 * IContextMenu2 members
	 */
	//@{
	STDMETHODIMP	QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHODIMP	InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
	STDMETHODIMP	GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
	STDMETHODIMP	HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	//@}

    /** \name IContextMenu3 
	 * IContextMenu3 members
	 */
	//@{
	STDMETHODIMP	HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	//@}

	/** \name IColumnProvider
	 * IColumnProvider members
	 */
	//@{
	STDMETHODIMP	GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci);
	STDMETHODIMP	GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);
	STDMETHODIMP	Initialize(LPCSHCOLUMNINIT psci);
	//@}

	/** \name IShellExtInit
	 * IShellExtInit methods
	 */
	//@{
	STDMETHODIMP	Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
	//@}

    /** \name IPersistFile
	 * IPersistFile methods
	 */
	//@{
    STDMETHODIMP GetClassID(CLSID *pclsid);
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP IsDirty(void) { return S_OK; };
    STDMETHODIMP Save(LPCOLESTR /*pszFileName*/, BOOL /*fRemember*/) { return S_OK; };
    STDMETHODIMP SaveCompleted(LPCOLESTR /*pszFileName*/) { return S_OK; };
    STDMETHODIMP GetCurFile(LPOLESTR * /*ppszFileName*/) { return S_OK; };
	//@}

	/** \name IShellIconOverlayIdentifier 
	 * IShellIconOverlayIdentifier methods
	 */
	//@{
	STDMETHODIMP	GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
	STDMETHODIMP	GetPriority(int *pPriority); 
	STDMETHODIMP	IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);
	//@}

	/** \name IShellPropSheetExt 
	 * IShellPropSheetExt methods
	 */
	//@{
	STDMETHODIMP	AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
	STDMETHODIMP	ReplacePage (UINT, LPFNADDPROPSHEETPAGE, LPARAM);
	//@}
};
