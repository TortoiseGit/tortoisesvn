// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013 - TortoiseSVN

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
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "PathUtils.h"

#define GetPIDLFolder(pida) (PIDLIST_ABSOLUTE)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (PCUITEMID_CHILD)(((LPBYTE)pida)+(pida)->aoffset[i+1])

int g_shellidlist=RegisterClipboardFormat(CFSTR_SHELLIDLIST);


// conditions:
// (all flags set) && (none of the flags set) || (all flags set) && (none of the flags set) || ...
CShellExt::MenuInfo CShellExt::menuInfo[] =
{
    { ShellMenuCheckout,                    MENUCHECKOUT,       IDI_CHECKOUT,           IDS_MENUCHECKOUT,           IDS_MENUDESCCHECKOUT,
        {ITEMIS_FOLDER|ITEMIS_ONLYONE, ITEMIS_INSVN|ITEMIS_FOLDERINSVN}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_checkout") },

    { ShellMenuUpgradeWC,                       MENUUPGRADE,    IDI_CLEANUP,            IDS_MENUUPGRADE,            IDS_MENUDESCUPGRADE,
        {ITEMIS_UNSUPPORTEDFORMAT, 0}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_upgradewc") },

    { ShellMenuUpdate,                      MENUUPDATE,         IDI_UPDATE,             IDS_MENUUPDATE,             IDS_MENUDESCUPDATE,
        {ITEMIS_INSVN,  ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_update") },

    { ShellMenuCommit,                      MENUCOMMIT,         IDI_COMMIT,             IDS_MENUCOMMIT,             IDS_MENUDESCCOMMIT,
        {ITEMIS_INSVN, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_commit") },

    { ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}},

    { ShellMenuDiff,                        MENUDIFF,           IDI_DIFF,               IDS_MENUDIFF,               IDS_MENUDESCDIFF,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_NORMAL|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_TWO, 0}, {ITEMIS_PROPMODIFIED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, _T("tsvn_diff") },

    { ShellMenuPrevDiff,                    MENUPREVDIFF,           IDI_DIFF,               IDS_MENUPREVDIFF,           IDS_MENUDESCPREVDIFF,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_prevdiff") },

    { ShellMenuUrlDiff,                     MENUURLDIFF,        IDI_DIFF,               IDS_MENUURLDIFF,            IDS_MENUDESCURLDIFF,
        {ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_EXTENDED|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_urldiff") },

    { ShellMenuLog,                         MENULOG,            IDI_LOG,                IDS_MENULOG,                IDS_MENUDESCLOG,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_ADDEDWITHHISTORY|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, _T("tsvn_log") },

    { ShellMenuRepoBrowse,                  MENUREPOBROWSE,     IDI_REPOBROWSE,         IDS_MENUREPOBROWSE,         IDS_MENUDESCREPOBROWSE,
        {ITEMIS_ONLYONE, 0}, {ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_repobrowse") },

    { ShellMenuShowChanged,                 MENUSHOWCHANGED,    IDI_SHOWCHANGED,        IDS_MENUSHOWCHANGED,        IDS_MENUDESCSHOWCHANGED,
        {ITEMIS_INSVN, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_showchanged")},

    { ShellMenuRevisionGraph,               MENUREVISIONGRAPH,  IDI_REVISIONGRAPH,      IDS_MENUREVISIONGRAPH,      IDS_MENUDESCREVISIONGRAPH,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_revisiongraph")},

    { ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("")},

    { ShellMenuConflictEditor,              MENUCONFLICTEDITOR, IDI_CONFLICT,           IDS_MENUCONFLICT,           IDS_MENUDESCCONFLICT,
        {ITEMIS_CONFLICTED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_conflicteditor") },

    { ShellMenuResolve,                     MENURESOLVE,        IDI_RESOLVE,            IDS_MENURESOLVE,            IDS_MENUDESCRESOLVE,
        {ITEMIS_CONFLICTED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_resolve") },

    { ShellMenuUpdateExt,                   MENUUPDATEEXT,      IDI_UPDATE,             IDS_MENUUPDATEEXT,          IDS_MENUDESCUPDATEEXT,
        {ITEMIS_INSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_updateext") },

    { ShellMenuRename,                      MENURENAME,         IDI_RENAME,             IDS_MENURENAME,             IDS_MENUDESCRENAME,
        {ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_INVERSIONEDFOLDER, ITEMIS_FILEEXTERNAL|ITEMIS_WCROOT|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_rename") },

    { ShellMenuRemove,                      MENUREMOVE,         IDI_DELETE,             IDS_MENUREMOVE,             IDS_MENUDESCREMOVE,
        {ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER, ITEMIS_ADDED|ITEMIS_FILEEXTERNAL|ITEMIS_WCROOT|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_remove") },

    { ShellMenuRemoveKeep,                  MENUREMOVE,         IDI_DELETE,             IDS_MENUREMOVEKEEP,         IDS_MENUDESCREMOVEKEEP,
        {ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER|ITEMIS_EXTENDED, ITEMIS_ADDED|ITEMIS_WCROOT|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_removekeep") },

    { ShellMenuRevert,                      MENUREVERT,         IDI_REVERT,             IDS_MENUREVERT,             IDS_MENUDESCREVERT,
        {ITEMIS_INSVN, ITEMIS_NORMAL|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_revert") },

    { ShellMenuRevert,                      MENUREVERT,         IDI_REVERT,             IDS_MENUUNDOADD,            IDS_MENUDESCUNDOADD,
        {ITEMIS_ADDED, ITEMIS_NORMAL|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_ADDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_revert") },

    { ShellMenuDelUnversioned,              MENUDELUNVERSIONED, IDI_DELUNVERSIONED,     IDS_MENUDELUNVERSIONED,     IDS_MENUDESCDELUNVERSIONED,
        {ITEMIS_FOLDER|ITEMIS_INSVN|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_delunversioned") },

    { ShellMenuCleanup,                     MENUCLEANUP,        IDI_CLEANUP,            IDS_MENUCLEANUP,            IDS_MENUDESCCLEANUP,
        {ITEMIS_INSVN|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_cleanup") },

    { ShellMenuLock,                        MENULOCK,           IDI_LOCK,               IDS_MENU_LOCK,              IDS_MENUDESC_LOCK,
        {ITEMIS_INSVN, ITEMIS_LOCKED|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_LOCKED|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_lock") },

    { ShellMenuUnlock,                      MENUUNLOCK,         IDI_UNLOCK,             IDS_MENU_UNLOCK,            IDS_MENUDESC_UNLOCK,
        {ITEMIS_INSVN|ITEMIS_LOCKED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_INSVN, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, _T("tsvn_unlock") },

    { ShellMenuUnlockForce,                 MENUUNLOCK,         IDI_UNLOCK,             IDS_MENU_UNLOCKFORCE,       IDS_MENUDESC_UNLOCKFORCE,
        {ITEMIS_INSVN|ITEMIS_LOCKED|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_INSVN|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_unlockforce") },

    { ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("")},

    { ShellMenuCopy,                        MENUCOPY,           IDI_COPY,               IDS_MENUBRANCH,             IDS_MENUDESCCOPY,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_copy") },

    { ShellMenuSwitch,                      MENUSWITCH,         IDI_SWITCH,             IDS_MENUSWITCH,             IDS_MENUDESCSWITCH,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_switch") },

    { ShellMenuMerge,                       MENUMERGE,          IDI_MERGE,              IDS_MENUMERGE,              IDS_MENUDESCMERGE,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_merge") },
    { ShellMenuMergeAll,                    MENUMERGEALL,       IDI_MERGE,              IDS_MENUMERGEALL,           IDS_MENUDESCMERGEALL,
        {ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_mergeall") },

    { ShellMenuExport,                      MENUEXPORT,         IDI_EXPORT,             IDS_MENUEXPORT,             IDS_MENUDESCEXPORT,
        {ITEMIS_FOLDER|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_export") },

    { ShellMenuRelocate,                    MENURELOCATE,       IDI_RELOCATE,           IDS_MENURELOCATE,           IDS_MENUDESCRELOCATE,
        {ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE|ITEMIS_WCROOT, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE|ITEMIS_WCROOT, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_relocate") },

    { ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("")},

    { ShellMenuCreateRepos,                 MENUCREATEREPOS,    IDI_CREATEREPOS,        IDS_MENUCREATEREPOS,        IDS_MENUDESCCREATEREPOS,
        {ITEMIS_FOLDER|ITEMIS_ONLYONE, ITEMIS_INSVN|ITEMIS_FOLDERINSVN}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_createrepo") },

    { ShellMenuAdd,                         MENUADD,            IDI_ADD,                IDS_MENUADD,                IDS_MENUDESCADD,
        {ITEMIS_INVERSIONEDFOLDER|ITEMIS_FOLDER, ITEMIS_INSVN|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_INSVN|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_IGNORED|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_DELETED, ITEMIS_FOLDER|ITEMIS_ONLYONE|ITEMIS_UNSUPPORTEDFORMAT}, _T("tsvn_add") },

    { ShellMenuAdd,                         MENUADD,            IDI_ADD,                IDS_MENUADDIMMEDIATE,       IDS_MENUDESCADD,
        {ITEMIS_INVERSIONEDFOLDER, ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {ITEMIS_IGNORED, ITEMIS_FOLDER|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_DELETED, ITEMIS_FOLDER|ITEMIS_ONLYONE|ITEMIS_UNSUPPORTEDFORMAT}, _T("tsvn_add") },

    { ShellMenuAddAsReplacement,            MENUADD,            IDI_ADD,                IDS_MENUADDASREPLACEMENT,   IDS_MENUADDASREPLACEMENT,
        {ITEMIS_DELETED|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_addasreplacement") },

    { ShellMenuImport,                      MENUIMPORT,         IDI_IMPORT,             IDS_MENUIMPORT,             IDS_MENUDESCIMPORT,
        {ITEMIS_FOLDER, ITEMIS_INSVN}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_import") },

    { ShellMenuBlame,                       MENUBLAME,          IDI_BLAME,              IDS_MENUBLAME,              IDS_MENUDESCBLAME,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_blame") },

    { ShellMenuIgnoreSub,                   MENUIGNORE,         IDI_IGNORE,             IDS_MENUIGNORE,             IDS_MENUDESCIGNORE,
        {ITEMIS_INVERSIONEDFOLDER, ITEMIS_IGNORED|ITEMIS_INSVN|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_ignoresub") },

    { ShellMenuDeleteIgnoreSub,             MENUIGNORE,         IDI_IGNORE,             IDS_MENUDELETEIGNORE,       IDS_MENUDESCDELETEIGNORE,
        {ITEMIS_INVERSIONEDFOLDER|ITEMIS_INSVN, ITEMIS_IGNORED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_deleteignoresub") },

    { ShellMenuUnIgnoreSub,                 MENUIGNORE,         IDI_IGNORE,             IDS_MENUUNIGNORE,           IDS_MENUDESCUNIGNORE,
        {ITEMIS_IGNORED, 0}, {0, 0}, {0, 0}, {0, 0} , _T("tsvn_unignoresub")},

    { ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("")},

    { ShellMenuCreatePatch,                 MENUCREATEPATCH,    IDI_CREATEPATCH,        IDS_MENUCREATEPATCH,        IDS_MENUDESCCREATEPATCH,
        {ITEMIS_INSVN, ITEMIS_NORMAL|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, _T("tsvn_createpatch") },

    { ShellMenuApplyPatch,                  MENUAPPLYPATCH,     IDI_PATCH,              IDS_MENUAPPLYPATCH,         IDS_MENUDESCAPPLYPATCH,
        {ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_ONLYONE|ITEMIS_PATCHFILE, 0}, {ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, _T("tsvn_applypatch") },

    { ShellMenuProperties,                  MENUPROPERTIES,     IDI_PROPERTIES,         IDS_MENUPROPERTIES,         IDS_MENUDESCPROPERTIES,
        {ITEMIS_INSVN, 0}, {ITEMIS_FOLDERINSVN, 0}, {0, 0}, {0, 0}, _T("tsvn_properties") },

    { ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("")},
    { ShellMenuClipPaste,                   MENUCLIPPASTE,      IDI_CLIPPASTE,          IDS_MENUCLIPPASTE,          IDS_MENUDESCCLIPPASTE,
        {ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_PATHINCLIPBOARD, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_clippaste") },

    { ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("")},

    { ShellMenuSettings,                    MENUSETTINGS,       IDI_SETTINGS,           IDS_MENUSETTINGS,           IDS_MENUDESCSETTINGS,
        {ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0}, _T("tsvn_settings") },
    { ShellMenuHelp,                        MENUHELP,           IDI_HELP,               IDS_MENUHELP,               IDS_MENUDESCHELP,
        {ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0}, _T("tsvn_help") },
    { ShellMenuAbout,                       MENUABOUT,          IDI_ABOUT,              IDS_MENUABOUT,              IDS_MENUDESCABOUT,
        {ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0}, _T("tsvn_about") },

    // the sub menus - they're not added like the the commands, therefore the menu ID is zero
    // but they still need to be in here, because we use the icon and string information anyway.
    { ShellSubMenu,                         NULL,               IDI_APP,                IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_submenu") },
    { ShellSubMenuFile,                     NULL,               IDI_MENUFILE,           IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_submenufile") },
    { ShellSubMenuFolder,                   NULL,               IDI_MENUFOLDER,         IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_submenufolder") },
    { ShellSubMenuLink,                     NULL,               IDI_MENULINK,           IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_submenulink") },
    { ShellSubMenuMultiple,                 NULL,               IDI_MENUMULTIPLE,       IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("tsvn_submenumultiple") },
    // mark the last entry to tell the loop where to stop iterating over this array
    { ShellMenuLastEntry,                   0,                  0,                      0,                          0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, _T("") },
};

STDMETHODIMP CShellExt::Initialize(PCIDLIST_ABSOLUTE pIDFolder,
                                        LPDATAOBJECT pDataObj,
                                        HKEY  hRegKey)
{
    __try
    {
        return Initialize_Wrap(pIDFolder, pDataObj, hRegKey);
    }
    __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return E_FAIL;
}

STDMETHODIMP CShellExt::Initialize_Wrap(PCIDLIST_ABSOLUTE pIDFolder,
                                        LPDATAOBJECT pDataObj,
                                        HKEY /* hRegKey */)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: Initialize\n");
    PreserveChdir preserveChdir;
    files_.clear();
    folder_.clear();
    uuidSource.clear();
    uuidTarget.clear();
    itemStates = 0;
    itemStatesFolder = 0;
    tstring statuspath;
    svn_wc_status_kind fetchedstatus = svn_wc_status_none;
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
                {
                    ReleaseStgMedium ( &medium );
                    return E_INVALIDARG;
                }


                HDROP drop = (HDROP)GlobalLock(stg.hGlobal);
                if ( NULL == drop )
                {
                    ReleaseStgMedium ( &stg );
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
                    std::unique_ptr<TCHAR[]> szFileName(new TCHAR[len+1]);
                    if (0 == DragQueryFile(drop, i, szFileName.get(), len+1))
                    {
                        continue;
                    }
                    tstring str = tstring(szFileName.get());
                    if (str.empty()||(!g_ShellCache.IsContextPathAllowed(szFileName.get())))
                        continue;
                    CTSVNPath strpath;
                    strpath.SetFromWin(CPathUtils::GetLongPathname(str.c_str()));
                    if (itemStates & ITEMIS_ONLYONE)
                    {
                        itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".diff"))==0) ? ITEMIS_PATCHFILE : 0;
                        itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".patch"))==0) ? ITEMIS_PATCHFILE : 0;
                    }
                    files_.push_back(strpath.GetWinPath());
                    if (i != 0)
                        continue;
                    //get the Subversion status of the item
                    svn_wc_status_kind status = svn_wc_status_none;
                    try
                    {
                        SVNStatus stat;
                        stat.GetStatus(strpath, false, true, true);
                        if (stat.status)
                        {
                            statuspath = str;
                            status = stat.status->node_status;
                            itemStates |= stat.status->prop_status > svn_wc_status_normal ? ITEMIS_PROPMODIFIED : 0;
                            fetchedstatus = status;
                            if (stat.status->lock && stat.status->lock->token)
                                itemStates |= (stat.status->lock->token[0] != 0) ? ITEMIS_LOCKED : 0;
                            if (stat.status->kind == svn_node_dir)
                            {
                                itemStates |= ITEMIS_FOLDER;
                                if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
                                    itemStates |= ITEMIS_FOLDERINSVN;
                                if (strpath.IsWCRoot())
                                    itemStates |= ITEMIS_WCROOT;
                            }
                            if (stat.status->repos_uuid)
                                uuidSource = CUnicodeUtils::StdGetUnicode(stat.status->repos_uuid);
                            if (stat.status->file_external)
                                itemStates |= ITEMIS_FILEEXTERNAL;
                            if (stat.status->conflicted)
                                itemStates |= ITEMIS_CONFLICTED;
                            if (stat.status->copied)
                                itemStates |= ITEMIS_ADDEDWITHHISTORY;
                        }
                        else
                        {
                            if (stat.IsUnsupportedFormat())
                            {
                                itemStates |= ITEMIS_UNSUPPORTEDFORMAT;
                            }
                            if (stat.GetSVNError())
                                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": GetStatus Failed with error:\"%ws\"\n", (LPCTSTR)stat.GetLastErrorMessage());
                            // sometimes, svn_client_status() returns with an error.
                            // in that case, we have to check if the working copy is versioned
                            // anyway to show the 'correct' context menu
                            if (g_ShellCache.IsVersioned(str.c_str(), true, false))
                                status = svn_wc_status_normal;
                        }
                    }
                    catch ( ... )
                    {
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
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
                } // for (int i = 0; i < count; i++)
                GlobalUnlock ( drop );
                ReleaseStgMedium ( &stg );
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
                    tstring str = child.toString();
                    if (str.empty()||(!g_ShellCache.IsContextPathAllowed(str.c_str())))
                        continue;
                    //check if our menu is requested for a subversion admin directory
                    if (g_SVNAdminDir.IsAdminDirPath(str.c_str()))
                        continue;

                    CTSVNPath strpath;
                    strpath.SetFromWin(CPathUtils::GetLongPathname(str.c_str()));
                    files_.push_back(strpath.GetWinPath());
                    itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".diff"))==0) ? ITEMIS_PATCHFILE : 0;
                    itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".patch"))==0) ? ITEMIS_PATCHFILE : 0;
                    if (statfetched)
                        continue;
                    //get the Subversion status of the item
                    svn_wc_status_kind status = svn_wc_status_none;
                    try
                    {
                        SVNStatus stat;
                        stat.GetStatus(strpath, false, true, true);
                        statuspath = str;
                        if (stat.status)
                        {
                            status = stat.status->node_status;
                            itemStates |= stat.status->prop_status > svn_wc_status_normal ? ITEMIS_PROPMODIFIED : 0;
                            fetchedstatus = status;
                            if (stat.status->lock && stat.status->lock->token)
                                itemStates |= (stat.status->lock->token[0] != 0) ? ITEMIS_LOCKED : 0;
                            if (stat.status->conflicted)
                                itemStates |= ITEMIS_CONFLICTED;
                            if (stat.status->repos_uuid)
                                uuidSource = CUnicodeUtils::StdGetUnicode(stat.status->repos_uuid);
                            if (stat.status->file_external)
                                itemStates |= ITEMIS_FILEEXTERNAL;
                            if (stat.status->copied)
                                itemStates |= ITEMIS_ADDEDWITHHISTORY;
                            if (stat.status->kind == svn_node_dir)
                            {
                                itemStates |= ITEMIS_FOLDER;
                                if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
                                    itemStates |= ITEMIS_FOLDERINSVN;
                                if (strpath.IsWCRoot())
                                    itemStates |= ITEMIS_WCROOT;
                            }
                            else
                            {
                                if ( (status == svn_wc_status_normal) &&
                                    g_ShellCache.IsGetLockTop() )
                                {
                                    SVNProperties props(strpath, false, false);
                                    if (props.HasProperty("svn:needs-lock"))
                                        itemStates |= ITEMIS_NEEDSLOCK;
                                }
                            }
                        }
                        else
                        {
                            if (stat.IsUnsupportedFormat())
                            {
                                itemStates |= ITEMIS_UNSUPPORTEDFORMAT;
                            }
                            if (stat.GetSVNError())
                                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": GetStatus Failed with error:\"%ws\"\n", (LPCTSTR)stat.GetLastErrorMessage());
                            // sometimes, svn_client_status() returns with an error.
                            // in that case, we have to check if the working copy is versioned
                            // anyway to show the 'correct' context menu
                            if (g_ShellCache.IsVersioned(strpath.GetWinPath(), strpath.IsDirectory(), false))
                            {
                                status = svn_wc_status_normal;
                                fetchedstatus = status;
                            }
                        }
                        statfetched = TRUE;
                    }
                    catch ( ... )
                    {
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
                    }
                    if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
                        itemStates |= ITEMIS_INSVN;
                    if (status == svn_wc_status_ignored)
                    {
                        itemStates |= ITEMIS_IGNORED;
                        // the item is ignored. Get the svn:ignored properties so we can (maybe) later
                        // offer a 'remove from ignored list' entry
                        SVNProperties props(strpath.GetContainingDirectory(), false, false);
                        ignoredprops.clear();
                        ignoredglobalprops.clear();
                        for (int p=0; p<props.GetCount(); ++p)
                        {
                            if (props.GetItemName(p).compare(SVN_PROP_IGNORE)==0)
                            {
                                std::string st = props.GetItemValue(p);
                                ignoredprops = CUnicodeUtils::StdGetUnicode(st);
                                // remove all escape chars ('\\')
                                ignoredprops.erase(std::remove(ignoredprops.begin(), ignoredprops.end(), '\\'), ignoredprops.end());
                            }
                            if (props.GetItemName(p).compare(SVN_PROP_INHERITABLE_IGNORES)==0)
                            {
                                std::string st = props.GetItemValue(p);
                                ignoredglobalprops = CUnicodeUtils::StdGetUnicode(st);
                                // remove all escape chars ('\\')
                                ignoredglobalprops.erase(std::remove(ignoredglobalprops.begin(), ignoredglobalprops.end(), '\\'), ignoredglobalprops.end());
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
                } // for (int i = 0; i < count; ++i)
                ItemIDList child (GetPIDLItem (cida, 0), &parent);
                if (g_ShellCache.IsVersioned(child.toString().c_str(), (itemStates & ITEMIS_FOLDER) != 0, false))
                    itemStates |= ITEMIS_INVERSIONEDFOLDER;
                GlobalUnlock(medium.hGlobal);

                // if the item is a versioned folder, check if there's a patch file
                // in the clipboard to be used in "Apply Patch"
                UINT cFormatDiff = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
                if (cFormatDiff)
                {
                    if (IsClipboardFormatAvailable(cFormatDiff))
                        itemStates |= ITEMIS_PATCHINCLIPBOARD;
                }
                if (IsClipboardFormatAvailable(CF_HDROP))
                    itemStates |= ITEMIS_PATHINCLIPBOARD;
            }

            ReleaseStgMedium ( &medium );
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
        svn_wc_status_kind status = svn_wc_status_none;
        if (IsClipboardFormatAvailable(CF_HDROP))
            itemStatesFolder |= ITEMIS_PATHINCLIPBOARD;
        if (g_ShellCache.IsContextPathAllowed(folder_.c_str()))
        {
            if (folder_.compare(statuspath)!=0)
            {
                try
                {
                    CTSVNPath strpath;
                    strpath.SetFromWin(CPathUtils::GetLongPathname(folder_.c_str()));
                    SVNStatus stat;
                    stat.GetStatus(strpath, false, true, true);
                    if (stat.status)
                    {
                        status = stat.status->node_status;
                        itemStatesFolder |= stat.status->prop_status > svn_wc_status_normal ? ITEMIS_PROPMODIFIED : 0;
                        if (stat.status->lock && stat.status->lock->token)
                            itemStatesFolder |= (stat.status->lock->token[0] != 0) ? ITEMIS_LOCKED : 0;
                        if (stat.status->repos_uuid)
                            uuidTarget = CUnicodeUtils::StdGetUnicode(stat.status->repos_uuid);
                        if (stat.status->conflicted)
                            itemStates |= ITEMIS_CONFLICTED;
                        if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
                            itemStatesFolder |= ITEMIS_INSVN;
                        if (status == svn_wc_status_normal)
                            itemStatesFolder |= ITEMIS_NORMAL;
                        if (status == svn_wc_status_conflicted)
                            itemStatesFolder |= ITEMIS_CONFLICTED;
                        if (status == svn_wc_status_added)
                            itemStatesFolder |= ITEMIS_ADDED;
                        if (status == svn_wc_status_deleted)
                            itemStatesFolder |= ITEMIS_DELETED;
                        if (stat.status->copied)
                            itemStates |= ITEMIS_ADDEDWITHHISTORY;
                        if (strpath.IsWCRoot())
                            itemStates |= ITEMIS_WCROOT;
                    }
                    else
                    {
                        if (stat.IsUnsupportedFormat())
                        {
                            itemStates |= ITEMIS_UNSUPPORTEDFORMAT;
                        }
                        if (stat.GetSVNError())
                            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": GetStatus Failed with error:\"%ws\"\n", (LPCTSTR)stat.GetLastErrorMessage());
                        // sometimes, svn_client_status() returns with an error.
                        // in that case, we have to check if the working copy is versioned
                        // anyway to show the 'correct' context menu
                        if (g_ShellCache.IsVersioned(folder_.c_str(), true, false))
                            status = svn_wc_status_normal;
                    }
                }
                catch ( ... )
                {
                    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
                }
            }
            else
            {
                status = fetchedstatus;
                itemStatesFolder = itemStates;
            }
        }
        else
        {
            folder_.clear();
            status = fetchedstatus;
        }
        if ((status != svn_wc_status_unversioned)&&(status != svn_wc_status_ignored)&&(status != svn_wc_status_none))
        {
            itemStatesFolder |= ITEMIS_FOLDERINSVN;
        }
        if (status == svn_wc_status_ignored)
            itemStatesFolder |= ITEMIS_IGNORED;
        itemStatesFolder |= ITEMIS_FOLDER;
        if (files_.empty())
            itemStates |= ITEMIS_ONLYONE;
        if (m_State != FileStateDropHandler)
            itemStates |= itemStatesFolder;
    }
    if (files_.size() == 2)
        itemStates |= ITEMIS_TWO;
    if ((files_.size() == 1)&&(g_ShellCache.IsContextPathAllowed(files_.front().c_str())))
    {
        itemStates |= ITEMIS_ONLYONE;
        if (m_State != FileStateDropHandler)
        {
            if (PathIsDirectory(files_.front().c_str()))
            {
                folder_ = files_.front();
                svn_wc_status_kind status = svn_wc_status_none;
                if (folder_.compare(statuspath)!=0)
                {
                    try
                    {
                        CTSVNPath strpath;
                        strpath.SetFromWin(CPathUtils::GetLongPathname(folder_.c_str()));
                        if (strpath.IsWCRoot())
                            itemStates |= ITEMIS_WCROOT;
                        SVNStatus stat;
                        stat.GetStatus(strpath, false, true, true);
                        if (stat.status)
                        {
                            status = stat.status->node_status;
                            itemStates |= stat.status->prop_status > svn_wc_status_normal ? ITEMIS_PROPMODIFIED : 0;
                            if (stat.status->lock && stat.status->lock->token)
                                itemStates |= (stat.status->lock->token[0] != 0) ? ITEMIS_LOCKED : 0;
                            if (stat.status->repos_uuid)
                                uuidTarget = CUnicodeUtils::StdGetUnicode(stat.status->repos_uuid);
                            if (stat.status->copied)
                                itemStates |= ITEMIS_ADDEDWITHHISTORY;
                        }
                        else
                        {
                            if (stat.IsUnsupportedFormat())
                            {
                                itemStates |= ITEMIS_UNSUPPORTEDFORMAT;
                            }
                        }
                    }
                    catch ( ... )
                    {
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
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
    }

    return S_OK;
}

void CShellExt::InsertSVNMenu(BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, SVNCommands com, const tstring& verb)
{
    TCHAR menutextbuffer[255] = {0};
    MAKESTRING(stringid);

    if (istop)
    {
        //menu entry for the top context menu, so append an "SVN " before
        //the menu text to indicate where the entry comes from
        _tcscpy_s(menutextbuffer, _T("SVN "));
        if (!g_ShellCache.HasShellMenuAccelerators())
        {
            // remove the accelerators
            tstring temp = stringtablebuffer;
            temp.erase(std::remove(temp.begin(), temp.end(), '&'), temp.end());
            _tcscpy_s(stringtablebuffer, temp.c_str());
        }
    }
    _tcscat_s(menutextbuffer, stringtablebuffer);

    MENUITEMINFO menuiteminfo = {0};
    menuiteminfo.cbSize = sizeof(menuiteminfo);
    menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_BITMAP | MIIM_STRING;
    menuiteminfo.fType = MFT_STRING;
    menuiteminfo.dwTypeData = menutextbuffer;
    if (icon)
        menuiteminfo.hbmpItem = SysInfo::Instance().IsVistaOrLater() ? m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, icon) : HBMMENU_CALLBACK;
    menuiteminfo.wID = (UINT)id;
    InsertMenuItem(menu, pos, TRUE, &menuiteminfo);

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


bool CShellExt::WriteClipboardPathsToTempFile(tstring& tempfile)
{
    tempfile = tstring();
    //write all selected files and paths to a temporary file
    //for TortoiseProc.exe to read out again.
    DWORD pathlength = GetTempPath(0, NULL);
    std::unique_ptr<TCHAR[]> path(new TCHAR[pathlength+1]);
    std::unique_ptr<TCHAR[]> tempFile(new TCHAR[pathlength + 100]);
    GetTempPath (pathlength+1, path.get());
    GetTempFileName (path.get(), _T("svn"), 0, tempFile.get());
    tempfile = tstring(tempFile.get());

    CAutoFile file = ::CreateFile (tempFile.get(),
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ,
                                   0,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_TEMPORARY,
                                   0);

    if (!file)
        return false;

    if (!IsClipboardFormatAvailable(CF_HDROP))
    {
        return false;
    }
    if (!OpenClipboard(NULL))
    {
        return false;
    }

    HGLOBAL hglb = GetClipboardData(CF_HDROP);
    HDROP hDrop = (HDROP)GlobalLock(hglb);
    const bool bRet = (hDrop != NULL);
    if(bRet)
    {
        TCHAR szFileName[MAX_PATH + 1];
        DWORD bytesWritten = 0;
        UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        for(UINT i = 0; i < cFiles; ++i)
        {
            if (DragQueryFile(hDrop, i, szFileName, _countof(szFileName)))
            {
                ::WriteFile (file, szFileName, (DWORD)(_tcslen(szFileName))*sizeof(TCHAR), &bytesWritten, 0);
                ::WriteFile (file, _T("\n"), 2, &bytesWritten, 0);
            }
        }
        GlobalUnlock(hDrop);
    }
    GlobalUnlock(hglb);

    CloseClipboard();

    return bRet;
}

tstring CShellExt::WriteFileListToTempFile()
{
    //write all selected files and paths to a temporary file
    //for TortoiseProc.exe to read out again.
    DWORD pathlength = GetTempPath(0, NULL);
    std::unique_ptr<TCHAR[]> path(new TCHAR[pathlength+1]);
    std::unique_ptr<TCHAR[]> tempFile(new TCHAR[pathlength + 100]);
    GetTempPath (pathlength+1, path.get());
    GetTempFileName (path.get(), _T("svn"), 0, tempFile.get());
    tstring retFilePath = tstring(tempFile.get());

    CAutoFile file = ::CreateFile (tempFile.get(),
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ,
                                   0,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_TEMPORARY,
                                   0);

    if (!file)
        return tstring();

    DWORD written = 0;
    if (files_.empty())
    {
        ::WriteFile (file, folder_.c_str(), (DWORD)folder_.size()*sizeof(TCHAR), &written, 0);
        ::WriteFile (file, _T("\n"), 2, &written, 0);
    }

    for (std::vector<tstring>::iterator I = files_.begin(); I != files_.end(); ++I)
    {
        ::WriteFile (file, I->c_str(), (DWORD)I->size()*sizeof(TCHAR), &written, 0);
        ::WriteFile (file, _T("\n"), 2, &written, 0);
    }
    return retFilePath;
}

STDMETHODIMP CShellExt::QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT &indexMenu)
{
    PreserveChdir preserveChdir;
    LoadLangDll();

    if ((uFlags & CMF_DEFAULTONLY)!=0)
        return S_OK;                    //we don't change the default action

    if (files_.empty()||folder_.empty())
        return S_OK;

    if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
        return S_OK;

    bool bSourceAndTargetFromSameRepository = (uuidSource.compare(uuidTarget) == 0) || uuidSource.empty() || uuidTarget.empty();

    //the drop handler only has eight commands, but not all are visible at the same time:
    //if the source file(s) are under version control then those files can be moved
    //to the new location or they can be moved with a rename,
    //if they are unversioned then they can be added to the working copy
    //if they are versioned, they also can be exported to an unversioned location
    UINT idCmd = idCmdFirst;

    // SVN move here
    // available if source is versioned, target is versioned, source and target from same repository or target folder is added
    // and the item is not a file external
    if (((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_INSVN))&&((~itemStates) & ITEMIS_FILEEXTERNAL))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVEMENU, 0, idCmdFirst, ShellMenuDropMove, _T("tsvn_dropmove"));

    // SVN move and rename here
    // available if source is a single, versioned item, target is versioned, source and target from same repository or target folder is added
    // and the item is not a file external
    if (((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_ONLYONE))&&((~itemStates) & ITEMIS_FILEEXTERNAL))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVERENAMEMENU, 0, idCmdFirst, ShellMenuDropMoveRename, _T("tsvn_dropmoverename"));

    // SVN copy here
    // available if source is versioned, target is versioned, source and target from same repository or target folder is added
    if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_INSVN))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYMENU, 0, idCmdFirst, ShellMenuDropCopy, _T("tsvn_dropcopy"));

    // SVN copy and rename here, source and target from same repository
    // available if source is a single, versioned item, target is versioned or target folder is added
    if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_ONLYONE))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYRENAMEMENU, 0, idCmdFirst, ShellMenuDropCopyRename, _T("tsvn_dropcopyrename"));

    // SVN add here
    // available if target is versioned and source is either unversioned or from another repository
    if ((itemStatesFolder & ITEMIS_FOLDERINSVN)&&(((~itemStates) & ITEMIS_INSVN)||!bSourceAndTargetFromSameRepository))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYADDMENU, 0, idCmdFirst, ShellMenuDropCopyAdd, _T("tsvn_dropcopyadd"));

    // SVN export here
    // available if source is versioned and a folder
    if ((itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_FOLDER))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTMENU, 0, idCmdFirst, ShellMenuDropExport, _T("tsvn_dropexport"));

    // SVN export all here
    // available if source is versioned and a folder
    if ((itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_FOLDER)&&(itemStates & ITEMIS_WCROOT))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTEXTENDEDMENU, 0, idCmdFirst, ShellMenuDropExportExtended, _T("tsvn_dropexportextended"));

    // SVN export changed here
    // available if source is versioned and a folder
    if ((itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_FOLDER))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTCHANGEDMENU, 0, idCmdFirst, ShellMenuDropExportChanged, _T("tsvn_dropexportchanged"));

    // SVN vendorbranch here
    // available if target is versioned and source is either unversioned or from another repository
    if ((itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_FOLDER)&&(((~itemStates) & ITEMIS_INSVN)||!bSourceAndTargetFromSameRepository))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPVENDORMENU, 0, idCmdFirst, ShellMenuDropVendor, _T("tsvn_dropvendor"));

    // apply patch
    // available if source is a patchfile
    if (itemStates & ITEMIS_PATCHFILE)
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_MENUAPPLYPATCH, 0, idCmdFirst, ShellMenuApplyPatch, _T("tsvn_applypatch"));

    // separator
    if (idCmd != idCmdFirst)
        InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

    TweakMenu(hMenu);

    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT idCmdLast,
                                         UINT uFlags)
{
    __try
    {
        return QueryContextMenu_Wrap(hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }
    __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return E_FAIL;
}


STDMETHODIMP CShellExt::QueryContextMenu_Wrap(HMENU hMenu,
                                              UINT indexMenu,
                                              UINT idCmdFirst,
                                              UINT /*idCmdLast*/,
                                              UINT uFlags)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: QueryContextMenu itemStates=%ld\n", itemStates);
    PreserveChdir preserveChdir;

    //first check if our drop handler is called
    //and then (if true) provide the context menu for the
    //drop handler
    if (m_State == FileStateDropHandler)
    {
        return QueryDropContext(uFlags, idCmdFirst, hMenu, indexMenu);
    }

    if ((uFlags & CMF_DEFAULTONLY)!=0)
        return S_OK;                    //we don't change the default action

    if (files_.empty()&&folder_.empty())
        return S_OK;

    if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
        return S_OK;

    int csidlarray[] =
    {
        CSIDL_BITBUCKET,
        CSIDL_CDBURN_AREA,
        CSIDL_COMMON_STARTMENU,
        CSIDL_COMPUTERSNEARME,
        CSIDL_CONNECTIONS,
        CSIDL_CONTROLS,
        CSIDL_COOKIES,
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
        return S_OK;

    if (folder_.empty())
    {
        // folder is empty, but maybe files are selected
        if (files_.empty())
            return S_OK;    // nothing selected - we don't have a menu to show
        // check whether a selected entry is an UID - those are namespace extensions
        // which we can't handle
        for (std::vector<tstring>::const_iterator it = files_.begin(); it != files_.end(); ++it)
        {
            if (_tcsncmp(it->c_str(), _T("::{"), 3)==0)
                return S_OK;
        }
    }
    else
    {
        if (_tcsncmp(folder_.c_str(), _T("::{"), 3)==0)
            return S_OK;
    }

    if (((uFlags & CMF_EXTENDEDVERBS) == 0) && g_ShellCache.HideMenusForUnversionedItems())
    {
        if ((itemStates & (ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER|ITEMIS_FOLDERINSVN|ITEMIS_TWO))==0)
            return S_OK;
    }
    //check if our menu is requested for a subversion admin directory
    if (g_SVNAdminDir.IsAdminDirPath(folder_.c_str()))
        return S_OK;

    if ((uFlags & CMF_EXTENDEDVERBS)||(g_ShellCache.AlwaysExtended()))
        itemStates |= ITEMIS_EXTENDED;

    const BOOL bShortcut = !!(uFlags & CMF_VERBSONLY);
    if ( bShortcut && (files_.size()==1))
    {
        // Don't show the context menu for a link if the
        // destination is not part of a working copy.
        // It would only show the standard menu items
        // which are already shown for the lnk-file.
        CString path = files_.front().c_str();
        if ( !g_ShellCache.IsVersioned(path, !!PathIsDirectory(path), false) )
        {
            return S_OK;
        }
    }

    //check if we already added our menu entry for a folder.
    //we check that by iterating through all menu entries and check if
    //the dwItemData member points to our global ID string. That string is set
    //by our shell extension when the folder menu is inserted.
    TCHAR menubuf[MAX_PATH];
    int count = GetMenuItemCount(hMenu);
    for (int i=0; i<count; ++i)
    {
        MENUITEMINFO miif;
        SecureZeroMemory(&miif, sizeof(MENUITEMINFO));
        miif.cbSize = sizeof(MENUITEMINFO);
        miif.fMask = MIIM_DATA;
        miif.dwTypeData = menubuf;
        miif.cch = _countof(menubuf);
        GetMenuItemInfo(hMenu, i, TRUE, &miif);
        if (miif.dwItemData == (ULONG_PTR)g_MenuIDString)
            return S_OK;
    }

    LoadLangDll();
    UINT idCmd = idCmdFirst;

    //create the sub menu
    HMENU subMenu = CreateMenu();
    int indexSubMenu = 0;

    unsigned __int64 topmenu = g_ShellCache.GetMenuLayout();
    unsigned __int64 menumask = g_ShellCache.GetMenuMask();

    bool bAddSeparator = false;
    bool bMenuEntryAdded = false;
    // insert separator at start
    InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;
    bool bShowIcons = !!DWORD(CRegStdDWORD(_T("Software\\TortoiseSVN\\ShowContextMenuIcons"), TRUE));
    for (int menuIndex = 0; menuInfo[menuIndex].command != ShellMenuLastEntry; menuIndex++)
    {
        MenuInfo& menuItem = menuInfo[menuIndex];
        if (menuItem.command == ShellSeparator)
        {
            // we don't add a separator immediately. Because there might not be
            // another 'normal' menu entry after we insert a separator.
            // we simply set a flag here, indicating that before the next
            // 'normal' menu entry, a separator should be added.
            bAddSeparator = true;
            continue;
        }
        // check the conditions whether to show the menu entry or not
        if ((menuItem.menuID & (~menumask)) == 0)
            continue;

        const bool bInsertMenu = ShouldInsertItem(menuItem);
        if (!bInsertMenu)
            continue;
        // insert a separator
        if ((bMenuEntryAdded)&&(bAddSeparator))
        {
            bAddSeparator = false;
            bMenuEntryAdded = false;
            InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
            idCmd++;
        }

        // handle special cases (sub menus)
        if ((menuItem.command == ShellMenuIgnoreSub)||
            (menuItem.command == ShellMenuDeleteIgnoreSub)||
            (menuItem.command == ShellMenuUnIgnoreSub))
        {
            InsertIgnoreSubmenus(idCmd, idCmdFirst, hMenu, subMenu, indexMenu, indexSubMenu, topmenu, bShowIcons);
            bMenuEntryAdded = true;
        }
        else
        {
            // the 'get lock' command is special
            bool bIsTop = ((topmenu & menuItem.menuID) != 0);
            if (menuItem.command == ShellMenuLock)
            {
                if ((itemStates & ITEMIS_NEEDSLOCK) && g_ShellCache.IsGetLockTop())
                    bIsTop = true;
            }
            // the 'upgrade wc' command always has to on the top menu
            if (menuItem.command == ShellMenuUpgradeWC)
                bIsTop = true;
            // insert the menu entry
            InsertSVNMenu(  bIsTop,
                            bIsTop ? hMenu : subMenu,
                            bIsTop ? indexMenu++ : indexSubMenu++,
                            idCmd++,
                            menuItem.menuTextID,
                            bShowIcons ? menuItem.iconID : 0,
                            idCmdFirst,
                            menuItem.command,
                            menuItem.verb);
            if (!bIsTop)
                bMenuEntryAdded = true;
        }
    }

    //add sub menu to main context menu
    //don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
    //see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
    MAKESTRING(IDS_MENUSUBMENU);
    if (!g_ShellCache.HasShellMenuAccelerators())
    {
        // remove the accelerators
        tstring temp = stringtablebuffer;
        temp.erase(std::remove(temp.begin(), temp.end(), '&'), temp.end());
        _tcscpy_s(stringtablebuffer, temp.c_str());
    }
    MENUITEMINFO menuiteminfo;
    SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
    menuiteminfo.cbSize = sizeof(menuiteminfo);
    menuiteminfo.fType = MFT_STRING;
    menuiteminfo.dwTypeData = stringtablebuffer;

    UINT uIcon = bShowIcons ? IDI_APP : 0;
    if (folder_.size())
    {
        uIcon = bShowIcons ? IDI_MENUFOLDER : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuFolder;
        myIDMap[idCmd] = ShellSubMenuFolder;
        menuiteminfo.dwItemData = (ULONG_PTR)g_MenuIDString;
    }
    else if (!bShortcut && (files_.size()==1))
    {
        uIcon = bShowIcons ? IDI_MENUFILE : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuFile;
        myIDMap[idCmd] = ShellSubMenuFile;
    }
    else if (bShortcut && (files_.size()==1))
    {
        uIcon = bShowIcons ? IDI_MENULINK : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuLink;
        myIDMap[idCmd] = ShellSubMenuLink;
    }
    else if (files_.size() > 1)
    {
        uIcon = bShowIcons ? IDI_MENUMULTIPLE : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuMultiple;
        myIDMap[idCmd] = ShellSubMenuMultiple;
    }
    else
    {
        myIDMap[idCmd - idCmdFirst] = ShellSubMenu;
        myIDMap[idCmd] = ShellSubMenu;
    }
    HBITMAP bmp = NULL;

    menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_BITMAP | MIIM_STRING;
    if (bShowIcons)
        menuiteminfo.hbmpItem = SysInfo::Instance().IsVistaOrLater() ? m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, uIcon) : HBMMENU_CALLBACK;
    menuiteminfo.hbmpChecked = bmp;
    menuiteminfo.hbmpUnchecked = bmp;
    menuiteminfo.hSubMenu = subMenu;
    menuiteminfo.wID = idCmd++;
    InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);

    //separator after
    InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

    TweakMenu(hMenu);

    //return number of menu items added
    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
}

void CShellExt::TweakMenu(HMENU hMenu)
{
    MENUINFO MenuInfo = {};
    MenuInfo.cbSize  = sizeof(MenuInfo);
    MenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
    MenuInfo.dwStyle = MNS_CHECKORBMP;

    SetMenuInfo(hMenu, &MenuInfo);
}

void CShellExt::AddPathCommand(tstring& svnCmd, LPCTSTR command, bool bFilesAllowed)
{
    svnCmd += command;
    svnCmd += _T(" /path:\"");
    if ((bFilesAllowed) && !files_.empty())
        svnCmd += files_.front();
    else
        svnCmd += folder_;
    svnCmd += _T("\"");
}

void CShellExt::AddPathFileCommand(tstring& svnCmd, LPCTSTR command)
{
    tstring tempfile = WriteFileListToTempFile();
    svnCmd += command;
    svnCmd += _T(" /pathfile:\"");
    svnCmd += tempfile;
    svnCmd += _T("\"");
    svnCmd += _T(" /deletepathfile");
}

void CShellExt::AddPathFileDropCommand(tstring& svnCmd, LPCTSTR command)
{
    tstring tempfile = WriteFileListToTempFile();
    svnCmd += command;
    svnCmd += _T(" /pathfile:\"");
    svnCmd += tempfile;
    svnCmd += _T("\"");
    svnCmd += _T(" /deletepathfile");
    svnCmd += _T(" /droptarget:\"");
    svnCmd += folder_;
    svnCmd += _T("\"");
}

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    __try
    {
        return InvokeCommand_Wrap(lpcmi);
    }
    __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return E_FAIL;
}

// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi)
{
    PreserveChdir preserveChdir;
    const HRESULT hr = E_INVALIDARG;
    if (lpcmi == NULL)
        return hr;

    if(files_.empty()&&folder_.empty())
        return hr;

    UINT_PTR idCmd = LOWORD(lpcmi->lpVerb);

    if (HIWORD(lpcmi->lpVerb))
    {
        tstring verb = CUnicodeUtils::StdGetUnicode(lpcmi->lpVerb);
        std::map<tstring, UINT_PTR>::const_iterator verb_it = myVerbsMap.lower_bound(verb);
        if (verb_it != myVerbsMap.end() && verb_it->first == verb)
            idCmd = verb_it->second;
        else
            return hr;
    }

    // See if we have a handler interface for this id
    std::map<UINT_PTR, UINT_PTR>::const_iterator id_it = myIDMap.lower_bound(idCmd);
    if (id_it != myIDMap.end() && id_it->first == idCmd)
    {
        tstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
        tstring tortoiseMergePath = GetAppDirectory() + _T("TortoiseMerge.exe");
        tstring cwdFolder;
        if (folder_.empty() && files_.empty())
        {
            // use the users desktop path as the CWD
            TCHAR desktopDir[MAX_PATH] = {0};
            SHGetSpecialFolderPath(lpcmi->hwnd, desktopDir, CSIDL_DESKTOPDIRECTORY, TRUE);
            cwdFolder = desktopDir;
        }
        else
        {
            cwdFolder = folder_.empty() ? files_[0] : folder_;
            if (!PathIsDirectory(cwdFolder.c_str()))
            {
                cwdFolder = cwdFolder.substr(0, cwdFolder.rfind('\\'));
            }
        }

        //TortoiseProc expects a command line of the form:
        //"/command:<commandname> /pathfile:<path> /startrev:<startrevision> /endrev:<endrevision> /deletepathfile
        // or
        //"/command:<commandname> /path:<path> /startrev:<startrevision> /endrev:<endrevision>
        //
        //* path is a path to a single file/directory for commands which only act on single items (log, checkout, ...)
        //* pathfile is a path to a temporary file which contains a list of file paths
        tstring svnCmd = _T(" /command:");
        switch (id_it->second)
        {
        case ShellMenuUpgradeWC:
            AddPathFileCommand(svnCmd, L"wcupgrade");
            break;
        case ShellMenuCheckout:
            AddPathCommand(svnCmd, L"checkout", false);
            break;
        case ShellMenuUpdate:
            AddPathFileCommand(svnCmd, L"update");
            break;
        case ShellMenuUpdateExt:
            AddPathFileCommand(svnCmd, L"update");
            svnCmd += _T(" /rev");
            break;
        case ShellMenuCommit:
            AddPathFileCommand(svnCmd, L"commit");
            break;
        case ShellMenuAdd:
        case ShellMenuAddAsReplacement:
            AddPathFileCommand(svnCmd, L"add");
            break;
        case ShellMenuIgnore:
            AddPathFileCommand(svnCmd, L"ignore");
            break;
        case ShellMenuIgnoreGlobal:
            AddPathFileCommand(svnCmd, L"ignore");
            svnCmd += _T(" /recursive");
            break;
        case ShellMenuIgnoreCaseSensitive:
            AddPathFileCommand(svnCmd, L"ignore");
            svnCmd += _T(" /onlymask");
            break;
        case ShellMenuIgnoreCaseSensitiveGlobal:
            AddPathFileCommand(svnCmd, L"ignore");
            svnCmd += _T(" /onlymask /recursive");
            break;
        case ShellMenuDeleteIgnore:
            AddPathFileCommand(svnCmd, L"ignore");
            svnCmd += _T(" /delete");
            break;
        case ShellMenuDeleteIgnoreGlobal:
            AddPathFileCommand(svnCmd, L"ignore");
            svnCmd += _T(" /delete /recursive");
            break;
        case ShellMenuDeleteIgnoreCaseSensitive:
            AddPathFileCommand(svnCmd, L"ignore");
            svnCmd += _T(" /delete");
            svnCmd += _T(" /onlymask");
            break;
        case ShellMenuDeleteIgnoreCaseSensitiveGlobal:
            AddPathFileCommand(svnCmd, L"ignore");
            svnCmd += _T(" /delete");
            svnCmd += _T(" /onlymask");
            svnCmd += _T(" /recursive");
            break;
        case ShellMenuUnIgnore:
            AddPathFileCommand(svnCmd, L"unignore");
            break;
        case ShellMenuUnIgnoreGlobal:
            AddPathFileCommand(svnCmd, L"unignore");
            svnCmd += _T(" /recursive");
            break;
        case ShellMenuUnIgnoreCaseSensitive:
            AddPathFileCommand(svnCmd, L"unignore");
            svnCmd += _T(" /onlymask");
            break;
        case ShellMenuUnIgnoreCaseSensitiveGlobal:
            AddPathFileCommand(svnCmd, L"unignore");
            svnCmd += _T(" /onlymask /recursive");
            break;
        case ShellMenuRevert:
            AddPathFileCommand(svnCmd, L"revert");
            break;
        case ShellMenuDelUnversioned:
            AddPathFileCommand(svnCmd, L"delunversioned");
            break;
        case ShellMenuCleanup:
            AddPathFileCommand(svnCmd, L"cleanup");
            break;
        case ShellMenuResolve:
            AddPathFileCommand(svnCmd, L"resolve");
            break;
        case ShellMenuSwitch:
            AddPathCommand(svnCmd, L"switch", true);
            break;
        case ShellMenuImport:
            AddPathCommand(svnCmd, L"import", false);
            break;
        case ShellMenuExport:
            AddPathCommand(svnCmd, L"export", false);
            break;
        case ShellMenuAbout:
            svnCmd += _T("about");
            break;
        case ShellMenuCreateRepos:
            AddPathCommand(svnCmd, L"repocreate", false);
            break;
        case ShellMenuMerge:
            AddPathCommand(svnCmd, L"merge", true);
            break;
        case ShellMenuMergeAll:
            AddPathCommand(svnCmd, L"mergeall", true);
            break;
        case ShellMenuCopy:
            AddPathCommand(svnCmd, L"copy", true);
            break;
        case ShellMenuSettings:
            svnCmd += _T("settings");
            break;
        case ShellMenuHelp:
            svnCmd += _T("help");
            break;
        case ShellMenuRename:
            AddPathCommand(svnCmd, L"rename", true);
            break;
        case ShellMenuRemove:
            AddPathFileCommand(svnCmd, L"remove");
            break;
        case ShellMenuRemoveKeep:
            AddPathFileCommand(svnCmd, L"remove");
            svnCmd += _T(" /keep");
            break;
        case ShellMenuDiff:
            svnCmd += _T("diff /path:\"");
            if (files_.size() == 1)
                svnCmd += files_.front();
            else if (files_.size() == 2)
            {
                std::vector<tstring>::iterator I = files_.begin();
                svnCmd += *I;
                ++I;
                svnCmd += _T("\" /path2:\"");
                svnCmd += *I;
            }
            else
                svnCmd += folder_;
            svnCmd += _T("\"");
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                svnCmd += _T(" /alternative");
            break;
        case ShellMenuPrevDiff:
            AddPathCommand(svnCmd, L"prevdiff", true);
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                svnCmd += _T(" /alternative");
            break;
        case ShellMenuUrlDiff:
            AddPathCommand(svnCmd, L"urldiff", true);
            break;
        case ShellMenuDropCopyAdd:
            AddPathFileDropCommand(svnCmd, L"dropcopyadd");
            break;
        case ShellMenuDropCopy:
            AddPathFileDropCommand(svnCmd, L"dropcopy");
            break;
        case ShellMenuDropCopyRename:
            AddPathFileDropCommand(svnCmd, L"dropcopy");
            svnCmd += _T(" /rename");
            break;
        case ShellMenuDropMove:
            AddPathFileDropCommand(svnCmd, L"dropmove");
            break;
        case ShellMenuDropMoveRename:
            AddPathFileDropCommand(svnCmd, L"dropmove");
            svnCmd += _T(" /rename");
            break;
        case ShellMenuDropExport:
            AddPathFileDropCommand(svnCmd, L"dropexport");
            break;
        case ShellMenuDropExportExtended:
            AddPathFileDropCommand(svnCmd, L"dropexport");
            svnCmd += _T(" /extended:unversioned");
            break;
        case ShellMenuDropExportChanged:
            AddPathFileDropCommand(svnCmd, L"dropexport");
            svnCmd += _T(" /extended:localchanges");
            break;
        case ShellMenuDropVendor:
            AddPathFileDropCommand(svnCmd, L"dropvendor");
            break;
        case ShellMenuLog:
            AddPathCommand(svnCmd, L"log", true);
            break;
        case ShellMenuConflictEditor:
            AddPathCommand(svnCmd, L"conflicteditor", true);
            break;
        case ShellMenuRelocate:
            AddPathCommand(svnCmd, L"relocate", false);
            break;
        case ShellMenuShowChanged:
            if (files_.size() > 1)
            {
                AddPathFileCommand(svnCmd, L"repostatus");
            }
            else
            {
                AddPathCommand(svnCmd, L"repostatus", true);
            }
            break;
        case ShellMenuRepoBrowse:
            AddPathCommand(svnCmd, L"repobrowser", true);
            break;
        case ShellMenuBlame:
            AddPathCommand(svnCmd, L"blame", true);
            break;
        case ShellMenuCreatePatch:
            AddPathFileCommand(svnCmd, L"createpatch");
            break;
        case ShellMenuApplyPatch:
            if ((itemStates & ITEMIS_PATCHINCLIPBOARD) && ((~itemStates) & ITEMIS_PATCHFILE))
            {
                // if there's a patch file in the clipboard, we save it
                // to a temporary file and tell TortoiseMerge to use that one
                UINT cFormat = RegisterClipboardFormat(_T("TSVN_UNIFIEDDIFF"));
                if ((cFormat)&&(OpenClipboard(NULL)))
                {
                    HGLOBAL hglb = GetClipboardData(cFormat);
                    LPCSTR lpstr = (LPCSTR)GlobalLock(hglb);

                    DWORD len = GetTempPath(0, NULL);
                    std::unique_ptr<TCHAR[]> path(new TCHAR[len+1]);
                    std::unique_ptr<TCHAR[]> tempF(new TCHAR[len+100]);
                    GetTempPath (len+1, path.get());
                    GetTempFileName (path.get(), _T("svn"), 0, tempF.get());
                    std::wstring sTempFile = std::wstring(tempF.get());

                    FILE * outFile;
                    size_t patchlen = strlen(lpstr);
                    _tfopen_s(&outFile, sTempFile.c_str(), _T("wb"));
                    if(outFile)
                    {
                        size_t size = fwrite(lpstr, sizeof(char), patchlen, outFile);
                        if (size == patchlen)
                        {
                            itemStates |= ITEMIS_PATCHFILE;
                            if ((folder_.size()==0)&&(files_.size()))
                            {
                                folder_ = files_[0];
                            }
                            itemStatesFolder |= ITEMIS_FOLDERINSVN;
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
                if (!files_.empty())
                {
                    svnCmd += files_.front();
                    if (itemStatesFolder & ITEMIS_FOLDERINSVN)
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
                if (!files_.empty())
                    svnCmd += files_.front();
                else
                    svnCmd += folder_;
                svnCmd += _T("\"");
            }
            if (!uuidSource.empty())
            {
                CRegStdDWORD groupSetting = CRegStdDWORD(_T("Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo"), 3);
                switch (DWORD(groupSetting))
                {
                case 1:
                case 2:
                    {
                        svnCmd += _T(" /groupuuid:");
                        svnCmd += uuidSource;
                    }
                    break;
                }
            }
            myIDMap.clear();
            myVerbsIDMap.clear();
            myVerbsMap.clear();
            RunCommand(tortoiseMergePath, svnCmd, cwdFolder, _T("TortoiseMerge launch failed") );
            return S_OK;
        case ShellMenuRevisionGraph:
            AddPathCommand(svnCmd, L"revisiongraph", true);
            break;
        case ShellMenuLock:
            AddPathFileCommand(svnCmd, L"lock");
            break;
        case ShellMenuUnlock:
            AddPathFileCommand(svnCmd, L"unlock");
            break;
        case ShellMenuUnlockForce:
            AddPathFileCommand(svnCmd, L"unlock");
            svnCmd += _T(" /force");
            break;
        case ShellMenuProperties:
            AddPathFileCommand(svnCmd, L"properties");
            break;
        case ShellMenuClipPaste:
            {
                tstring tempfile;
                if (WriteClipboardPathsToTempFile(tempfile))
                {
                    bool bCopy = true;
                    UINT cPrefDropFormat = RegisterClipboardFormat(_T("Preferred DropEffect"));
                    if (cPrefDropFormat)
                    {
                        if (OpenClipboard(lpcmi->hwnd))
                        {
                            HGLOBAL hglb = GetClipboardData(cPrefDropFormat);
                            if (hglb)
                            {
                                DWORD* effect = (DWORD*) GlobalLock(hglb);
                                if (*effect == DROPEFFECT_MOVE)
                                    bCopy = false;
                                GlobalUnlock(hglb);
                            }
                            CloseClipboard();
                        }
                    }

                    if (bCopy)
                        svnCmd += _T("pastecopy /pathfile:\"");
                    else
                        svnCmd += _T("pastemove /pathfile:\"");
                    svnCmd += tempfile;
                    svnCmd += _T("\"");
                    svnCmd += _T(" /deletepathfile");
                    svnCmd += _T(" /droptarget:\"");
                    svnCmd += folder_;
                    svnCmd += _T("\"");
                }
                else
                    return S_OK;
            }
            break;
        default:
            break;
            //#endregion
        } // switch (id_it->second)
        svnCmd += _T(" /hwnd:");
        TCHAR buf[30];
        _stprintf_s(buf, _T("%ld"), (LONG_PTR)lpcmi->hwnd);
        svnCmd += buf;
        if (!uuidSource.empty())
        {
            CRegStdDWORD groupSetting = CRegStdDWORD(_T("Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo"), 3);
            switch (DWORD(groupSetting))
            {
            case 1:
            case 2:
                {
                    svnCmd += _T(" /groupuuid:");
                    svnCmd += uuidSource;
                }
                break;
            }
        }
        myIDMap.clear();
        myVerbsIDMap.clear();
        myVerbsMap.clear();
        RunCommand(tortoiseProcPath, svnCmd, cwdFolder.c_str(), _T("TortoiseProc Launch failed"));
        return S_OK;
    } // if (id_it != myIDMap.end() && id_it->first == idCmd)
    return hr;
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT uFlags,
                                         UINT FAR * reserved,
                                         LPSTR pszName,
                                         UINT cchMax)
{
    __try
    {
        return GetCommandString_Wrap(idCmd, uFlags, reserved, pszName, cchMax);
    }
    __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return E_FAIL;
}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString_Wrap(UINT_PTR idCmd,
                                              UINT uFlags,
                                              UINT FAR * /*reserved*/,
                                              LPSTR pszName,
                                              UINT cchMax)
{
    PreserveChdir preserveChdir;
    //do we know the id?
    std::map<UINT_PTR, UINT_PTR>::const_iterator id_it = myIDMap.lower_bound(idCmd);
    if (id_it == myIDMap.end() || id_it->first != idCmd)
    {
        return E_INVALIDARG;        //no, we don't
    }

    LoadLangDll();

    MAKESTRING(IDS_MENUDESCDEFAULT);
    for (int menuIndex = 0; menuInfo[menuIndex].command != ShellMenuLastEntry; menuIndex++)
    {
        if (menuInfo[menuIndex].command == (SVNCommands)id_it->second)
        {
            MAKESTRING(menuInfo[menuIndex].menuDescID);
            break;
        }
    }

    const TCHAR * desc = stringtablebuffer;
    HRESULT hr = E_INVALIDARG;
    switch(uFlags)
    {
    case GCS_HELPTEXTA:
        {
            std::string help = CUnicodeUtils::StdGetUTF8(desc);
            lstrcpynA(pszName, help.c_str(), cchMax);
            hr = S_OK;
            break;
        }
    case GCS_HELPTEXTW:
        {
            std::wstring help = desc;
            lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax);
            hr = S_OK;
            break;
        }
    case GCS_VERBA:
        {
            std::map<UINT_PTR, tstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
            if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
            {
                std::string help = CUnicodeUtils::StdGetUTF8(verb_id_it->second);
                lstrcpynA(pszName, help.c_str(), cchMax);
                hr = S_OK;
            }
        }
        break;
    case GCS_VERBW:
        {
            std::map<UINT_PTR, tstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
            if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
            {
                std::wstring help = verb_id_it->second;
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": verb : %ws\n", help.c_str());
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
    __try
    {
        return HandleMenuMsg_Wrap(uMsg, wParam, lParam);
    }
    __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return E_FAIL;
}

STDMETHODIMP CShellExt::HandleMenuMsg_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
    __try
    {
        return HandleMenuMsg2_Wrap(uMsg, wParam, lParam, pResult);
    }
    __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return E_FAIL;
}

STDMETHODIMP CShellExt::HandleMenuMsg2_Wrap(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
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
            lpmis->itemWidth = 16;
            lpmis->itemHeight = 16;
            *pResult = TRUE;
        }
        break;
    case WM_DRAWITEM:
        {
            LPCTSTR resource;
            DRAWITEMSTRUCT* lpdis = (DRAWITEMSTRUCT*)lParam;
            if ((lpdis==NULL)||(lpdis->CtlType != ODT_MENU))
                return S_OK;        //not for a menu
            resource = GetMenuTextFromResource((int)myIDMap[lpdis->itemID]);
            if (resource == NULL)
                return S_OK;
            HICON hIcon = (HICON)LoadImage(g_hResInst, resource, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
            if (hIcon == NULL)
                return S_OK;
            DrawIconEx(lpdis->hDC,
                lpdis->rcItem.left,
                lpdis->rcItem.top + (lpdis->rcItem.bottom - lpdis->rcItem.top - 16) / 2,
                hIcon, 16, 16,
                0, NULL, DI_NORMAL);
            DestroyIcon(hIcon);
            *pResult = TRUE;
        }
        break;
    case WM_MENUCHAR:
        {
            LPCTSTR resource;
            TCHAR *szItem;
            if (HIWORD(wParam) != MF_POPUP)
                return S_OK;
            int nChar = LOWORD(wParam);
            if (_istascii((wint_t)nChar) && _istupper((wint_t)nChar))
                nChar = tolower(nChar);
            // we have the char the user pressed, now search that char in all our
            // menu items
            std::vector<UINT_PTR> accmenus;
            for (std::map<UINT_PTR, UINT_PTR>::iterator It = mySubMenuMap.begin(); It != mySubMenuMap.end(); ++It)
            {
                resource = GetMenuTextFromResource((int)mySubMenuMap[It->first]);
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
            if (accmenus.empty())
            {
                // no menu with that accelerator key.
                *pResult = MAKELONG(0, MNC_IGNORE);
                return S_OK;
            }
            if (accmenus.size() == 1)
            {
                // Only one menu with that accelerator key. We're lucky!
                // So just execute that menu entry.
                *pResult = MAKELONG(accmenus[0], MNC_EXECUTE);
                return S_OK;
            }
            if (accmenus.size() > 1)
            {
                // we have more than one menu item with this accelerator key!
                MENUITEMINFO mif;
                mif.cbSize = sizeof(MENUITEMINFO);
                mif.fMask = MIIM_STATE;
                for (std::vector<UINT_PTR>::iterator it = accmenus.begin(); it != accmenus.end(); ++it)
                {
                    GetMenuItemInfo((HMENU)lParam, (UINT)*it, TRUE, &mif);
                    if (mif.fState == MFS_HILITE)
                    {
                        // this is the selected item, so select the next one
                        ++it;
                        if (it == accmenus.end())
                            *pResult = MAKELONG(accmenus[0], MNC_SELECT);
                        else
                            *pResult = MAKELONG(*it, MNC_SELECT);
                        return S_OK;
                    }
                }
                *pResult = MAKELONG(accmenus[0], MNC_SELECT);
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
    TCHAR textbuf[255];
    LPCTSTR resource = NULL;
    unsigned __int64 layout = g_ShellCache.GetMenuLayout();
    space = 6;

    for (int menuIndex = 0; menuInfo[menuIndex].command != ShellMenuLastEntry; menuIndex++)
    {
        MenuInfo& menuItem = menuInfo[menuIndex];
        if (menuItem.command != id)
            continue;
        MAKESTRING(menuItem.menuTextID);
        resource = MAKEINTRESOURCE(menuItem.iconID);
        switch (id)
        {
        case ShellMenuLock:
            {
                // menu lock is special because it can be set to the top
                // with a separate option in the registry
                const bool lock = (layout & MENULOCK) || ((itemStates & ITEMIS_NEEDSLOCK) && g_ShellCache.IsGetLockTop());
                space = lock ? 0 : 6;
                if (lock)
                {
                    _tcscpy_s(textbuf, _T("SVN "));
                    _tcscat_s(textbuf, stringtablebuffer);
                    _tcscpy_s(stringtablebuffer, textbuf);
                }
                break;
            }
            // the sub menu entries are special because they're *always* on the top level menu
        case ShellSubMenuMultiple:
        case ShellSubMenuLink:
        case ShellSubMenuFolder:
        case ShellSubMenuFile:
        case ShellSubMenu:
            space = 0;
            break;
        default:
            space = layout & menuItem.menuID ? 0 : 6;
            if (layout & (menuItem.menuID))
            {
                _tcscpy_s(textbuf, _T("SVN "));
                _tcscat_s(textbuf, stringtablebuffer);
                _tcscpy_s(stringtablebuffer, textbuf);
            }
            break;
        }
        return resource;
    }
    return NULL;
}

bool CShellExt::IsIllegalFolder(std::wstring folder, int * csidlarray)
{
    TCHAR buf[MAX_PATH];    //MAX_PATH ok, since SHGetSpecialFolderPath doesn't return the required buffer length!
    PIDLIST_ABSOLUTE pidl = NULL;
    for (int i = 0; csidlarray[i]; i++)
    {
        pidl = NULL;
        if (SHGetFolderLocation(NULL, csidlarray[i], NULL, 0, &pidl)!=S_OK)
            continue;
        if (!SHGetPathFromIDList(pidl, buf))
        {
            // not a file system path, definitely illegal for our use
            CoTaskMemFree(pidl);
            continue;
        }
        CoTaskMemFree(pidl);
        if (buf[0]==0)
            continue;
        if (_tcscmp(buf, folder.c_str())==0)
            return true;
    }
    return false;
}

void CShellExt::InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst,
                                     HMENU hMenu, HMENU subMenu,
                                     UINT &indexMenu, int &indexSubMenu,
                                     unsigned __int64 topmenu,
                                     bool bShowIcons)
{
    HMENU ignoresubmenu = NULL;
    int indexignoresub = 0;
    bool bShowIgnoreMenu = false;
    TCHAR maskbuf[MAX_PATH];        // MAX_PATH is ok, since this only holds a filename
    TCHAR ignorepath[MAX_PATH];     // MAX_PATH is ok, since this only holds a filename
    if (files_.empty())
        return;
    UINT icon = bShowIcons ? IDI_IGNORE : 0;

    std::vector<tstring>::iterator I = files_.begin();
    if (_tcsrchr(I->c_str(), '\\'))
        _tcscpy_s(ignorepath, _tcsrchr(I->c_str(), '\\')+1);
    else
        _tcscpy_s(ignorepath, I->c_str());
    if ((itemStates & ITEMIS_IGNORED)&&((ignoredprops.size() > 0)||(ignoredglobalprops.size() > 0)))
    {
        // check if the item name is ignored or the mask
        size_t p = 0;
        while ( (p=ignoredprops.find( ignorepath,p )) != -1 )
        {
            if ( (p==0 || ignoredprops[p-1]==TCHAR('\n')) )
            {
                const size_t pathLength = _tcslen(ignorepath);
                if ( ((p + pathLength)==ignoredprops.length()) || (ignoredprops[p + pathLength]==TCHAR('\n')) || (ignoredprops[p + pathLength]==0) )
                {
                    break;
                }
            }
            p++;
        }
        if (p!=-1)
        {
            ignoresubmenu = CreateMenu();
            InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
            tstring verb = _T("tsvn_") + tstring(ignorepath);
            myVerbsMap[verb] = idCmd - idCmdFirst;
            myVerbsMap[verb] = idCmd;
            myVerbsIDMap[idCmd - idCmdFirst] = verb;
            myVerbsIDMap[idCmd] = verb;
            myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnore;
            myIDMap[idCmd++] = ShellMenuUnIgnore;
            bShowIgnoreMenu = true;
        }

        p = 0;
        while ( (p=ignoredglobalprops.find(ignoredglobalprops, p)) != -1 )
        {
            if ( (p==0 || ignoredglobalprops[p-1]==TCHAR('\n')) )
            {
                const size_t pathLength = _tcslen(ignorepath);
                if ( ((p + pathLength)==ignoredglobalprops.length()) || (ignoredglobalprops[p + pathLength]==TCHAR('\n')) || (ignoredglobalprops[p + pathLength]==0) )
                {
                    break;
                }
            }
            p++;
        }
        if (p!=-1)
        {
            CString temp;
            temp.Format(IDS_MENUIGNOREGLOBAL, ignorepath);
            InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
            tstring verb = _T("tsvn_") + tstring(temp);
            myVerbsMap[verb] = idCmd - idCmdFirst;
            myVerbsMap[verb] = idCmd;
            myVerbsIDMap[idCmd - idCmdFirst] = verb;
            myVerbsIDMap[idCmd] = verb;
            myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreGlobal;
            myIDMap[idCmd++] = ShellMenuUnIgnoreGlobal;
        }
        _tcscpy_s(maskbuf, _T("*"));
        if (_tcsrchr(ignorepath, '.'))
        {
            _tcscat_s(maskbuf, _tcsrchr(ignorepath, '.'));
            p = ignoredprops.find(maskbuf);
            if ((p!=-1) &&
                ((ignoredprops.compare(maskbuf)==0) || (ignoredprops.find('\n', p)==p+_tcslen(maskbuf)+1) || (ignoredprops.rfind('\n', p)==p-1)))
            {
                if (ignoresubmenu==NULL)
                    ignoresubmenu = CreateMenu();

                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
                tstring verb = _T("tsvn_") + tstring(maskbuf);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreCaseSensitive;
                myIDMap[idCmd++] = ShellMenuUnIgnoreCaseSensitive;
                bShowIgnoreMenu = true;
            }
            p = ignoredglobalprops.find(maskbuf);
            if ((p!=-1) &&
                ((ignoredglobalprops.compare(maskbuf)==0) || (ignoredglobalprops.find('\n', p)==p+_tcslen(maskbuf)+1) || (ignoredglobalprops.rfind('\n', p)==p-1)))
            {
                if (ignoresubmenu==NULL)
                    ignoresubmenu = CreateMenu();

                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, maskbuf);
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
                tstring verb = _T("tsvn_") + tstring(temp);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreCaseSensitiveGlobal;
                myIDMap[idCmd++] = ShellMenuUnIgnoreCaseSensitiveGlobal;
                bShowIgnoreMenu = true;
            }
        }
    }
    else if ((itemStates & ITEMIS_IGNORED) == 0)
    {
        bShowIgnoreMenu = true;
        ignoresubmenu = CreateMenu();
        if (itemStates & ITEMIS_ONLYONE)
        {
            if (itemStates & ITEMIS_INSVN)
            {
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
                myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnore;
                myIDMap[idCmd++] = ShellMenuDeleteIgnore;

                _tcscpy_s(maskbuf, _T("*"));
                if (_tcsrchr(ignorepath, '.'))
                {
                    _tcscat_s(maskbuf, _tcsrchr(ignorepath, '.'));
                    InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
                    tstring verb = _T("tsvn_") + tstring(maskbuf);
                    myVerbsMap[verb] = idCmd - idCmdFirst;
                    myVerbsMap[verb] = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd] = verb;
                    myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreCaseSensitive;
                    myIDMap[idCmd++] = ShellMenuDeleteIgnoreCaseSensitive;
                }

                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorepath);
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
                myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreGlobal;
                myIDMap[idCmd++] = ShellMenuDeleteIgnoreGlobal;

                _tcscpy_s(maskbuf, _T("*"));
                if (_tcsrchr(ignorepath, '.'))
                {
                    _tcscat_s(maskbuf, _tcsrchr(ignorepath, '.'));
                    temp.Format(IDS_MENUIGNOREGLOBAL, maskbuf);
                    InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
                    tstring verb = _T("tsvn_") + tstring(temp);
                    myVerbsMap[verb] = idCmd - idCmdFirst;
                    myVerbsMap[verb] = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd] = verb;
                    myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
                    myIDMap[idCmd++] = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
                }

            }
            else
            {
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
                myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
                myIDMap[idCmd++] = ShellMenuIgnore;

                _tcscpy_s(maskbuf, _T("*"));
                if (_tcsrchr(ignorepath, '.'))
                {
                    _tcscat_s(maskbuf, _tcsrchr(ignorepath, '.'));
                    InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
                    tstring verb = _T("tsvn_") + tstring(maskbuf);
                    myVerbsMap[verb] = idCmd - idCmdFirst;
                    myVerbsMap[verb] = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd] = verb;
                    myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitive;
                    myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitive;
                }

                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorepath);
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
                myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreGlobal;
                myIDMap[idCmd++] = ShellMenuIgnoreGlobal;

                _tcscpy_s(maskbuf, _T("*"));
                if (_tcsrchr(ignorepath, '.'))
                {
                    _tcscat_s(maskbuf, _tcsrchr(ignorepath, '.'));
                    temp.Format(IDS_MENUIGNOREGLOBAL, maskbuf);
                    InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
                    tstring verb = _T("tsvn_") + tstring(temp);
                    myVerbsMap[verb] = idCmd - idCmdFirst;
                    myVerbsMap[verb] = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd] = verb;
                    myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitiveGlobal;
                    myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitiveGlobal;
                }
            }
        }
        else
        {
            if (itemStates & ITEMIS_INSVN)
            {
                MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLE);
                _stprintf_s(ignorepath, stringtablebuffer, files_.size());
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
                tstring verb = _T("tsvn_") + tstring(ignorepath);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnore;
                myIDMap[idCmd++] = ShellMenuDeleteIgnore;

                MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK);
                _stprintf_s(ignorepath, stringtablebuffer, files_.size());
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
                verb = tstring(ignorepath);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreCaseSensitive;
                myIDMap[idCmd++] = ShellMenuDeleteIgnoreCaseSensitive;

                MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK);
                _stprintf_s(ignorepath, stringtablebuffer, files_.size());
                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorepath);
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
                verb = tstring(temp);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
                myIDMap[idCmd++] = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
            }
            else
            {
                MAKESTRING(IDS_MENUIGNOREMULTIPLE);
                _stprintf_s(ignorepath, stringtablebuffer, files_.size());
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
                tstring verb = _T("tsvn_") + tstring(ignorepath);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
                myIDMap[idCmd++] = ShellMenuIgnore;

                MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK);
                _stprintf_s(ignorepath, stringtablebuffer, files_.size());
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
                verb = tstring(ignorepath);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitive;
                myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitive;

                MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK);
                _stprintf_s(ignorepath, stringtablebuffer, files_.size());
                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorepath);
                InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, temp);
                verb = tstring(temp);
                myVerbsMap[verb] = idCmd - idCmdFirst;
                myVerbsMap[verb] = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd] = verb;
                myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitiveGlobal;
                myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitiveGlobal;
            }
        }
    }

    if (bShowIgnoreMenu)
    {
        MENUITEMINFO menuiteminfo;
        SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
        menuiteminfo.cbSize = sizeof(menuiteminfo);
        menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_BITMAP | MIIM_STRING;
        menuiteminfo.fType = MFT_STRING;
        HBITMAP bmp = SysInfo::Instance().IsVistaOrLater() ? m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, icon) : m_iconBitmapUtils.IconToBitmap(g_hResInst, icon);
        if (icon)
            menuiteminfo.hbmpItem = SysInfo::Instance().IsVistaOrLater() ? bmp : HBMMENU_CALLBACK;
        menuiteminfo.hbmpChecked = bmp;
        menuiteminfo.hbmpUnchecked = bmp;
        menuiteminfo.hSubMenu = ignoresubmenu;
        menuiteminfo.wID = idCmd;
        SecureZeroMemory(stringtablebuffer, sizeof(stringtablebuffer));
        if (itemStates & ITEMIS_IGNORED)
            GetMenuTextFromResource(ShellMenuUnIgnoreSub);
        else if (itemStates & ITEMIS_INSVN)
            GetMenuTextFromResource(ShellMenuDeleteIgnoreSub);
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

void CShellExt::RunCommand(const tstring& path, const tstring& command,
    const tstring& folder, LPCTSTR errorMessage)
{
    if (CCreateProcessHelper::CreateProcessDetached(path.c_str(),
        const_cast<TCHAR*>(command.c_str()), folder.c_str()))
    {
        // process started - exit
        return;
    }

    CFormatMessageWrapper errorDetails;
    MessageBox( NULL, errorDetails, errorMessage, MB_OK | MB_ICONINFORMATION );
}

bool CShellExt::ShouldInsertItem(const MenuInfo& item) const
{
    return ShouldEnableMenu(item.first) || ShouldEnableMenu(item.second) ||
        ShouldEnableMenu(item.third) || ShouldEnableMenu(item.fourth);
}

bool CShellExt::ShouldEnableMenu(const YesNoPair& pair) const
{
    if (pair.yes && pair.no)
    {
        if (((pair.yes & itemStates) == pair.yes) && ((pair.no & (~itemStates)) == pair.no))
            return true;
    }
    else if ((pair.yes)&&((pair.yes & itemStates) == pair.yes))
        return true;
    else if ((pair.no)&&((pair.no & (~itemStates)) == pair.no))
        return true;
    return false;
}
