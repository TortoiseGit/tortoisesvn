// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#pragma once

#include "Globals.h"
#include "registry.h"
#include "resource.h"
#include "ShellCache.h"
#include "RemoteCacheLink.h"
#include "SVNFolderStatus.h"
#include "IconBitmapUtils.h"
#include "CrashReport.h"

extern  volatile LONG       g_cRefThisDll;          // Reference count of this DLL.
extern  HINSTANCE           g_hmodThisDll;          // Instance handle for this DLL
extern  ShellCache          g_ShellCache;           // caching of registry entries, ...
extern  DWORD               g_langid;
extern  ULONGLONG           g_langTimeout;
extern  HINSTANCE           g_hResInst;
extern  tstring             g_filepath;
extern  svn_wc_status_kind  g_filestatus;           ///< holds the corresponding status to the file/dir above
extern  bool                g_readonlyoverlay;      ///< whether to show the read only overlay or not
extern  bool                g_lockedoverlay;        ///< whether to show the locked overlay or not

extern bool                 g_normalovlloaded;
extern bool                 g_modifiedovlloaded;
extern bool                 g_conflictedovlloaded;
extern bool                 g_readonlyovlloaded;
extern bool                 g_deletedovlloaded;
extern bool                 g_lockedovlloaded;
extern bool                 g_addedovlloaded;
extern bool                 g_ignoredovlloaded;
extern bool                 g_unversionedovlloaded;
extern LPCTSTR              g_MenuIDString;

extern  void                LoadLangDll();
extern  tstring             GetAppDirectory();
extern  CComCriticalSection g_csGlobalCOMGuard;
typedef CComCritSecLock<CComCriticalSection> AutoLocker;

typedef DWORD ARGB;

// The actual OLE Shell context menu handler
/**
 * \ingroup TortoiseShell
 * The main class of our COM object / Shell Extension.
 * It contains all Interfaces we implement for the shell to use.
 * \remark The implementations of the different interfaces are
 * split into several *.cpp files to keep them in a reasonable size.
 */
class CShellExt : public IContextMenu3,
                         IPersistFile,
                         IColumnProvider,
                         IShellExtInit,
                         IShellIconOverlayIdentifier,
                         IShellPropSheetExt,
                         ICopyHookW

// COMPILER ERROR? You need the latest version of the
// platform SDK which has references to IColumnProvider
// in the header files.  Download it here:
// http://www.microsoft.com/msdownload/platformsdk/sdkupdate/
{
protected:

    enum SVNCommands
    {
        ShellSeparator = 0,
        ShellSubMenu = 1,
        ShellSubMenuFolder,
        ShellSubMenuFile,
        ShellSubMenuLink,
        ShellSubMenuMultiple,
        ShellMenuCheckout,
        ShellMenuUpdate,
        ShellMenuCommit,
        ShellMenuAdd,
        ShellMenuAddAsReplacement,
        ShellMenuRevert,
        ShellMenuCleanup,
        ShellMenuCleanupRefresh,
        ShellMenuResolve,
        ShellMenuSwitch,
        ShellMenuImport,
        ShellMenuExport,
        ShellMenuAbout,
        ShellMenuCreateRepos,
        ShellMenuCopy,
        ShellMenuMerge,
        ShellMenuMergeAll,
        ShellMenuSettings,
        ShellMenuRemove,
        ShellMenuRemoveKeep,
        ShellMenuRename,
        ShellMenuUpdateExt,
        ShellMenuDiff,
        ShellMenuDiffLater,
        ShellMenuDiffNow,
        ShellMenuPrevDiff,
        ShellMenuUrlDiff,
        ShellMenuUnifiedDiff,
        ShellMenuDropCopyAdd,
        ShellMenuDropMoveAdd,
        ShellMenuDropMove,
        ShellMenuDropMoveRename,
        ShellMenuDropCopy,
        ShellMenuDropCopyRename,
        ShellMenuDropExport,
        ShellMenuDropExportExtended,
        ShellMenuDropExportChanged,
        ShellMenuDropExternals,
        ShellMenuDropVendor,
        ShellMenuLog,
        ShellMenuConflictEditor,
        ShellMenuRelocate,
        ShellMenuHelp,
        ShellMenuShowChanged,
        ShellMenuIgnoreSub,
        ShellMenuDeleteIgnoreSub,
        ShellMenuIgnore,
        ShellMenuIgnoreGlobal,
        ShellMenuDeleteIgnore,
        ShellMenuDeleteIgnoreGlobal,
        ShellMenuIgnoreCaseSensitive,
        ShellMenuIgnoreCaseSensitiveGlobal,
        ShellMenuDeleteIgnoreCaseSensitive,
        ShellMenuDeleteIgnoreCaseSensitiveGlobal,
        ShellMenuRepoBrowse,
        ShellMenuBlame,
        ShellMenuCopyUrl,
        ShellMenuApplyPatch,
        ShellMenuCreatePatch,
        ShellMenuRevisionGraph,
        ShellMenuUnIgnoreSub,
        ShellMenuUnIgnoreCaseSensitive,
        ShellMenuUnIgnoreCaseSensitiveGlobal,
        ShellMenuUnIgnore,
        ShellMenuUnIgnoreGlobal,
        ShellMenuLock,
        ShellMenuUnlock,
        ShellMenuUnlockForce,
        ShellMenuProperties,
        ShellMenuDelUnversioned,
        ShellMenuClipPaste,
        ShellMenuUpgradeWC,
        ShellMenuLastEntry          // used to mark the menu array end
    };

    // helper struct for context menu entries
    typedef struct YesNoPair
    {
        DWORD               yes;
        DWORD               no;
    };
    typedef struct MenuInfo
    {
        SVNCommands         command;        ///< the command which gets executed for this menu entry
        unsigned __int64    menuID;         ///< the menu ID to recognize the entry. NULL if it shouldn't be added to the context menu automatically
        UINT                iconID;         ///< the icon to show for the menu entry
        UINT                menuTextID;     ///< the text of the menu entry
        UINT                menuDescID;     ///< the description text for the menu entry
        /// the following 8 params are for checking whether the menu entry should
        /// be added automatically, based on states of the selected item(s).
        /// The 'yes' states must be set, the 'no' states must not be set
        /// the four pairs are OR'ed together, the 'yes'/'no' states are AND'ed together.
        YesNoPair           first;
        YesNoPair           second;
        YesNoPair           third;
        YesNoPair           fourth;
        std::wstring        verb;
    };

    static MenuInfo                 menuInfo[];
    FileState                       m_State;
    ULONG                           m_cRef;
    //std::map<int,std::string> verbMap;
    std::map<UINT_PTR, UINT_PTR>    myIDMap;
    std::map<UINT_PTR, UINT_PTR>    mySubMenuMap;
    std::map<tstring, UINT_PTR>     myVerbsMap;
    std::map<UINT_PTR, tstring>     myVerbsIDMap;
    tstring                         folder_;
    std::vector<tstring>            files_;
    std::vector<tstring>            urls_;
    DWORD                           itemStates;         ///< see the globals.h file for the ITEMIS_* defines
    DWORD                           itemStatesFolder;   ///< used for states of the folder_ (folder background and/or drop target folder)
    tstring                         uuidSource;
    tstring                         uuidTarget;
    int                             space;
    TCHAR                           stringtablebuffer[255];
    tstring                         maincolumnfilepath; ///< holds the last file/dir path for the column provider
    tstring                         extracolumnfilepath;///< holds the last file/dir path for the column provider
    tstring                         columnauthor;       ///< holds the corresponding author of the file/dir above
    tstring                         itemurl;
    tstring                         itemshorturl;
    tstring                         ignoredprops;
    tstring                         ignoredglobalprops;
    tstring                         owner;
    svn_revnum_t                    columnrev;          ///< holds the corresponding revision to the file/dir above
    svn_wc_status_kind              filestatus;
    CRegStdString                   regDiffLater;

    SVNFolderStatus                 m_CachedStatus;     // status cache
    CRemoteCacheLink                m_remoteCacheLink;
    IconBitmapUtils                 m_iconBitmapUtils;

    CString                         columnfolder;       ///< current folder of ColumnProvider
    typedef std::pair<std::wstring, std::string> columnuserprop; ///< type of user property of ColumnProvider
    std::vector<columnuserprop>     columnuserprops;    ///< user properties of ColumnProvider
    CCrashReportTSVN                m_crasher;

#define MAKESTRING(ID) LoadStringEx(g_hResInst, ID, stringtablebuffer, _countof(stringtablebuffer), (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)))
private:
    void            InsertSVNMenu(BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, SVNCommands com, const tstring& verb);
    void            InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT &indexMenu, int &indexSubMenu, unsigned __int64 topmenu, bool bShowIcons);
    tstring         WriteFileListToTempFile();
    bool            WriteClipboardPathsToTempFile(tstring& tempfile);
    LPCTSTR         GetMenuTextFromResource(int id);
    void            SetExtraColumnStatus(const TCHAR * path, const FileStatusCacheEntry * status);
    void            GetExtraColumnStatus(const TCHAR * path, BOOL bIsDir);
    void            GetMainColumnStatus(const TCHAR * path, BOOL bIsDir);
    STDMETHODIMP    QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT &indexMenu);
    bool            IsIllegalFolder(std::wstring folder, int * csidlarray);
    static void     RunCommand( const tstring& path, const tstring& command, const tstring& folder, LPCTSTR errorMessage );
    bool            ShouldInsertItem(const MenuInfo& pair) const;
    bool            ShouldEnableMenu(const YesNoPair& pair) const;
    void            GetColumnInfo(SHCOLUMNINFO* to, DWORD index, UINT charactersCount, UINT titleId, UINT descriptionId);
    void            TweakMenu(HMENU menu);
    void            ExtractProperty(const TCHAR* path, const char* propertyName, tstring& to);
    void            AddPathCommand(tstring& svnCmd, LPCTSTR command, bool bFilesAllowed);
    void            AddPathFileCommand(tstring& svnCmd, LPCTSTR command);
    void            AddPathFileDropCommand(tstring& svnCmd, LPCTSTR command);

    /** \name IContextMenu2 wrappers
     * IContextMenu2 wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    QueryContextMenu_Wrap(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP    InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP    GetCommandString_Wrap(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
    STDMETHODIMP    HandleMenuMsg_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam);
    //@}

    /** \name IContextMenu3 wrappers
     * IContextMenu3 wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    HandleMenuMsg2_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
    //@}

    /** \name IColumnProvider wrappers
     * IColumnProvider wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    GetColumnInfo_Wrap(DWORD dwIndex, SHCOLUMNINFO *psci);
    STDMETHODIMP    GetItemData_Wrap(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);
    STDMETHODIMP    Initialize_Wrap(LPCSHCOLUMNINIT psci);
    //@}

    /** \name IShellExtInit wrappers
     * IShellExtInit wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    Initialize_Wrap(PCIDLIST_ABSOLUTE pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
    //@}

    /** \name IShellIconOverlayIdentifier wrappers
     * IShellIconOverlayIdentifier wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    GetOverlayInfo_Wrap(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP    GetPriority_Wrap(int *pPriority);
    STDMETHODIMP    IsMemberOf_Wrap(LPCWSTR pwszPath, DWORD dwAttrib);
    //@}

    /** \name IShellPropSheetExt wrappers
     * IShellPropSheetExt wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP    AddPages_Wrap(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    //STDMETHODIMP    ReplacePage_Wrap(UINT, LPFNADDPROPSHEETPAGE, LPARAM);
    //@}

    /** \name ICopyHook wrapper
     * ICopyHook wrapper functions to catch exceptions and send crash reports
     */
    //@{
    STDMETHODIMP_(UINT) CopyCallback_Wrap(HWND hWnd, UINT wFunc, UINT wFlags, LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs);
    //@}

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
    STDMETHODIMP    QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP    InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP    GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
    STDMETHODIMP    HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    //@}

    /** \name IContextMenu3
     * IContextMenu3 members
     */
    //@{
    STDMETHODIMP    HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
    //@}

    /** \name IColumnProvider
     * IColumnProvider members
     */
    //@{
    STDMETHODIMP    GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci);
    STDMETHODIMP    GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);
    STDMETHODIMP    Initialize(LPCSHCOLUMNINIT psci);
    //@}

    /** \name IShellExtInit
     * IShellExtInit methods
     */
    //@{
    STDMETHODIMP    Initialize(PCIDLIST_ABSOLUTE pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
    //@}

    /** \name IPersistFile
     * IPersistFile methods
     */
    //@{
    STDMETHODIMP    GetClassID(CLSID *pclsid);
    STDMETHODIMP    Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP    IsDirty(void) { return S_OK; };
    STDMETHODIMP    Save(LPCOLESTR /*pszFileName*/, BOOL /*fRemember*/) { return S_OK; };
    STDMETHODIMP    SaveCompleted(LPCOLESTR /*pszFileName*/) { return S_OK; };
    STDMETHODIMP    GetCurFile(LPOLESTR * /*ppszFileName*/) { return S_OK; };
    //@}

    /** \name IShellIconOverlayIdentifier
     * IShellIconOverlayIdentifier methods
     */
    //@{
    STDMETHODIMP    GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    STDMETHODIMP    GetPriority(int *pPriority);
    STDMETHODIMP    IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);
    //@}

    /** \name IShellPropSheetExt
     * IShellPropSheetExt methods
     */
    //@{
    STDMETHODIMP    AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHODIMP    ReplacePage (UINT, LPFNADDPROPSHEETPAGE, LPARAM);
    //@}

    /** \name ICopyHook
     * ICopyHook members
     */
    //@{
    STDMETHODIMP_(UINT) CopyCallback(HWND hWnd, UINT wFunc, UINT wFlags, LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs);
    //@}

};
