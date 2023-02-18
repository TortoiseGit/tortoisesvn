// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2021-2023 - TortoiseSVN

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

#include <wrl/client.h>

#include "Globals.h"
#include "registry.h"
#include "ShellCache.h"
#include "RemoteCacheLink.h"
#include "SVNFolderStatus.h"
#include "IconBitmapUtils.h"
#include "ExplorerCommand.h"

class CExplorerCommand;
extern volatile LONG       g_cRefThisDll; // Reference count of this DLL.
extern HINSTANCE           g_hModThisDll; // Instance handle for this DLL
extern ShellCache          g_shellCache;  // caching of registry entries, ...
extern DWORD               g_langId;
extern ULONGLONG           g_langTimeout;
extern HINSTANCE           g_hResInst;
extern std::wstring        g_filePath;
extern svn_wc_status_kind  g_fileStatus;      ///< holds the corresponding status to the file/dir above
extern bool                g_readOnlyOverlay; ///< whether to show the read only overlay or not
extern bool                g_lockedOverlay;   ///< whether to show the locked overlay or not

extern bool                g_normalOvlLoaded;
extern bool                g_modifiedOvlLoaded;
extern bool                g_conflictedOvlLoaded;
extern bool                g_readonlyOvlLoaded;
extern bool                g_deletedOvlLoaded;
extern bool                g_lockedOvlLoaded;
extern bool                g_addedOvlLoaded;
extern bool                g_ignoredOvlLoaded;
extern bool                g_unversionedOvlLoaded;
extern LPCWSTR             g_menuIDString;

extern void                LoadLangDll();
extern std::wstring        GetAppDirectory();
extern CComCriticalSection g_csGlobalComGuard;
using AutoLocker = CComCritSecLock<CComCriticalSection>;

using ARGB       = DWORD;

// The actual OLE Shell context menu handler
/**
 * \ingroup TortoiseShell
 * The main class of our COM object / Shell Extension.
 * It contains all Interfaces we implement for the shell to use.
 * \remark The implementations of the different interfaces are
 * split into several *.cpp files to keep them in a reasonable size.
 */
class CShellExt : public IContextMenu3
    , IPersistFile
    , IColumnProvider
    , IShellExtInit
    , IShellIconOverlayIdentifier
    , IShellPropSheetExt
    , ICopyHookW
    , IExplorerCommand
    , IObjectWithSite
{
public:
    enum SVNCommands : int
    {
        ShellSeparator = 0,
        ShellSubMenu   = 1,
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
        ShellMenuShelve,
        ShellMenuUnshelve,
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
        ShellMenuLogExt,
        ShellMenuLastEntry // used to mark the menu array end
    };

protected:
    // helper struct for context menu entries
    struct YesNoPair
    {
        DWORD yes;
        DWORD no;
    };
    struct MenuInfo
    {
        SVNCommands            command;    ///< the command which gets executed for this menu entry
        TSVNContextMenuEntries menuID;     ///< the menu ID to recognize the entry. NULL if it shouldn't be added to the context menu automatically
        UINT                   iconID;     ///< the icon to show for the menu entry
        UINT                   menuTextID; ///< the text of the menu entry
        UINT                   menuDescID; ///< the description text for the menu entry
        /// the following 8 params are for checking whether the menu entry should
        /// be added automatically, based on states of the selected item(s).
        /// The 'yes' states must be set, the 'no' states must not be set
        /// the four pairs are OR'ed together, the 'yes'/'no' states are AND'ed together.
        YesNoPair              first;
        YesNoPair              second;
        YesNoPair              third;
        YesNoPair              fourth;
        std::wstring           verb;
    };

    static MenuInfo                                       menuInfo[];
    FileState                                             m_state;
    volatile ULONG                                        m_cRef;
    // std::map<int,std::string> verbMap;
    std::map<UINT_PTR, UINT_PTR>                          myIDMap;
    std::map<UINT_PTR, UINT_PTR>                          mySubMenuMap;
    std::map<std::wstring, UINT_PTR>                      myVerbsMap;
    std::map<UINT_PTR, std::wstring>                      myVerbsIDMap;
    std::wstring                                          m_folder;
    std::vector<std::wstring>                             m_files;
    std::vector<std::wstring>                             m_urls;
    DWORD                                                 itemStates;       ///< see the globals.h file for the ITEMIS_* defines
    DWORD                                                 itemStatesFolder; ///< used for states of the folder_ (folder background and/or drop target folder)
    std::wstring                                          uuidSource;
    std::wstring                                          uuidTarget;
    int                                                   space;
    wchar_t                                               stringTableBuffer[255];
    std::wstring                                          mainColumnFilePath;  ///< holds the last file/dir path for the column provider
    std::wstring                                          extraColumnFilePath; ///< holds the last file/dir path for the column provider
    std::wstring                                          columnAuthor;        ///< holds the corresponding author of the file/dir above
    std::wstring                                          itemUrl;
    std::wstring                                          itemShortUrl;
    std::wstring                                          ignoredProps;
    std::wstring                                          ignoredGlobalProps;
    std::wstring                                          owner;
    svn_revnum_t                                          columnRev; ///< holds the corresponding revision to the file/dir above
    svn_wc_status_kind                                    fileStatus;
    CRegStdString                                         regDiffLater;
    Microsoft::WRL::ComPtr<IUnknown>                      m_site;

    SVNFolderStatus                                       m_cachedStatus; // status cache
    CRemoteCacheLink                                      m_remoteCacheLink;
    IconBitmapUtils                                       m_iconBitmapUtils;

    CString                                               columnFolder;    ///< current folder of ColumnProvider
    std::vector<std::pair<std::wstring, std::string>>     columnUserProps; ///< user properties of ColumnProvider
    std::vector<Microsoft::WRL::ComPtr<CExplorerCommand>> m_explorerCommands;

#define MAKESTRING(ID) LoadStringEx(g_hResInst, ID, stringTableBuffer, _countof(stringTableBuffer), (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)))
private:
    void                InsertSVNMenu(BOOL isTop, HMENU menu, UINT pos, UINT_PTR id, UINT stringId, UINT icon, UINT idCmdFirst, SVNCommands com, const std::wstring& verb);
    void                InsertIgnoreSubmenus(UINT& idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT& indexMenu, int& indexSubMenu, unsigned __int64 topMenu, bool bShowIcons);
    static std::wstring WriteFileListToTempFile(const std::vector<std::wstring>& files, const std::wstring folder);
    static bool         WriteClipboardPathsToTempFile(std::wstring& tempFile);
    LPCWSTR             GetMenuTextFromResource(int id);
    UINT                IconIdForCommand(int id);
    void                SetExtraColumnStatus(const wchar_t* path, const FileStatusCacheEntry* status);
    void                GetExtraColumnStatus(const wchar_t* path, BOOL bIsDir);
    void                GetMainColumnStatus(const wchar_t* path, BOOL bIsDir);
    STDMETHODIMP        QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT& indexMenu);
    static bool         IsIllegalFolder(const std::wstring& folder, int* csidlarray);
    static void         RunCommand(const std::wstring& path, const std::wstring& command, const std::wstring& folder, LPCWSTR errorMessage, Microsoft::WRL::ComPtr<IUnknown> site);
    bool                ShouldInsertItem(const MenuInfo& pair) const;
    bool                ShouldEnableMenu(const YesNoPair& pair) const;
    void                GetColumnInfo(SHCOLUMNINFO* to, DWORD index, UINT charactersCount, UINT titleId, UINT descriptionId);
    static void         TweakMenu(HMENU menu);
    static void         ExtractProperty(const wchar_t* path, const char* propertyName, std::wstring& to);
    static void         AddPathCommand(std::wstring& svnCmd, LPCWSTR command, bool bFilesAllowed, const std::vector<std::wstring>& files, const std::wstring folder);
    static void         AddPathFileCommand(std::wstring& svnCmd, LPCWSTR command, const std::vector<std::wstring>& files, const std::wstring folder);
    static void         AddPathFileDropCommand(std::wstring& svnCmd, LPCWSTR command, const std::vector<std::wstring>& files, const std::wstring folder);
    static std::wstring ExplorerViewPath(const Microsoft::WRL::ComPtr<IUnknown>& site);

public:
    explicit CShellExt(FileState state);
    virtual ~CShellExt();

    /** \name IUnknown
     * IUnknown members
     */
    //@{
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR*) override;
    ULONG STDMETHODCALLTYPE   AddRef() override;
    ULONG STDMETHODCALLTYPE   Release() override;
    //@}

    /** \name IContextMenu2
     * IContextMenu2 members
     */
    //@{
    HRESULT STDMETHODCALLTYPE QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    HRESULT STDMETHODCALLTYPE InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) override;
    HRESULT STDMETHODCALLTYPE GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR* reserved, LPSTR pszName, UINT cchMax) override;
    HRESULT STDMETHODCALLTYPE HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    //@}

    /** \name IContextMenu3
     * IContextMenu3 members
     */
    //@{
    HRESULT STDMETHODCALLTYPE HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
    //@}

    /** \name IColumnProvider
     * IColumnProvider members
     */
    //@{
    HRESULT STDMETHODCALLTYPE GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO* psci) override;
    HRESULT STDMETHODCALLTYPE GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData) override;
    HRESULT STDMETHODCALLTYPE Initialize(LPCSHCOLUMNINIT psci) override;
    //@}

    /** \name IShellExtInit
     * IShellExtInit methods
     */
    //@{
    STDMETHODIMP              Initialize(PCIDLIST_ABSOLUTE pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID) override;
    //@}

    /** \name IPersistFile
     * IPersistFile methods
     */
    //@{
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID* pclsid) override;
    HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszFileName, DWORD dwMode) override;
    HRESULT STDMETHODCALLTYPE IsDirty() override { return S_OK; };
    HRESULT STDMETHODCALLTYPE Save(LPCOLESTR /*pszFileName*/, BOOL /*fRemember*/) override { return S_OK; };
    HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR /*pszFileName*/) override { return S_OK; };
    HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR* /*ppszFileName*/) override { return S_OK; };
    //@}

    /** \name IShellIconOverlayIdentifier
     * IShellIconOverlayIdentifier methods
     */
    //@{
    HRESULT STDMETHODCALLTYPE GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags) override;
    HRESULT STDMETHODCALLTYPE GetPriority(int* pPriority) override;
    HRESULT STDMETHODCALLTYPE IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib) override;
    //@}

    /** \name IShellPropSheetExt
     * IShellPropSheetExt methods
     */
    //@{
    HRESULT STDMETHODCALLTYPE AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) override;
    HRESULT STDMETHODCALLTYPE ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM) override;
    //@}

    /** \name ICopyHook
     * ICopyHook members
     */
    //@{
    UINT STDMETHODCALLTYPE    CopyCallback(HWND hWnd, UINT wFunc, UINT wFlags, LPCWSTR pszSrcFile, DWORD dwSrcAttribs, LPCWSTR pszDestFile, DWORD dwDestAttribs) override;
    //@}

    /** \name IExplorerCommand
     * IExplorerCommand members
     */
    //@{
    HRESULT STDMETHODCALLTYPE GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName) override;
    HRESULT STDMETHODCALLTYPE GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon) override;
    HRESULT STDMETHODCALLTYPE GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip) override;
    HRESULT STDMETHODCALLTYPE GetCanonicalName(GUID* pguidCommandName) override;
    HRESULT STDMETHODCALLTYPE GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState) override;
    HRESULT STDMETHODCALLTYPE Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc) override;
    HRESULT STDMETHODCALLTYPE GetFlags(EXPCMDFLAGS* pFlags) override;
    HRESULT STDMETHODCALLTYPE EnumSubCommands(IEnumExplorerCommand** ppEnum) override;
    //@}

    /** \name IObjectWithSite
     * IObjectWithSite members
     */
    //@{
    HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite) override;
    HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppvSite) override;
    //@}

    static void               InvokeCommand(int                 cmd,
                                            const std::wstring& cwd,
                                            const std::wstring& appDir,
                                            const std::wstring  uuidSource,
                                            HWND                hParent,
                                            DWORD itemStates, DWORD itemStatesFolder,
                                            const std::vector<std::wstring>& paths,
                                            const std::wstring&              folder,
                                            CRegStdString&                   regDiffLater,
                                            Microsoft::WRL::ComPtr<IUnknown> site);
};
