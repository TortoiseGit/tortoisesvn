// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2021-2022 - TortoiseSVN

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

#include <comutil.h>
#include <winrt/base.h>
#include <wrl/client.h>

#include "ShellExt.h"
#include "ItemIDList.h"
#include "PreserveChdir.h"
#include "UnicodeUtils.h"
#include "SVNProperties.h"
#include "SVNStatus.h"
#include "SVNAdminDir.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "PathUtils.h"
#include "LoadIconEx.h"
#include "resource.h"

// ReSharper disable CppInconsistentNaming
#define GetPIDLFolder(pida)  (PIDLIST_ABSOLUTE)(((LPBYTE)pida) + (pida)->aoffset[0])
#define GetPIDLItem(pida, i) (PCUITEMID_CHILD)(((LPBYTE)pida) + (pida)->aoffset[i + 1])
// ReSharper restore CppInconsistentNaming

int          g_shellIdList = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

// clang-format off
// conditions:
// (all flags set) && (none of the flags set) || (all flags set) && (none of the flags set) || ...
CShellExt::MenuInfo CShellExt::menuInfo[] =
{
    { ShellMenuCheckout,                    TSVNContextMenuEntries::Checkout,       IDI_CHECKOUT,           IDS_MENUCHECKOUT,           IDS_MENUDESCCHECKOUT,
    { ITEMIS_FOLDER | ITEMIS_ONLYONE, ITEMIS_INSVN | ITEMIS_FOLDERINSVN }, { ITEMIS_FOLDER | ITEMIS_ONLYONE | ITEMIS_EXTENDED | ITEMIS_INSVN | ITEMIS_FOLDERINSVN, 0 }, { 0, 0 }, { 0, 0 }, L"tsvn_checkout" },

    { ShellMenuUpgradeWC,                   TSVNContextMenuEntries::Upgrade,        IDI_CLEANUP,            IDS_MENUUPGRADE,            IDS_MENUDESCUPGRADE,
        {ITEMIS_UNSUPPORTEDFORMAT, 0}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_upgradewc" },

    { ShellMenuUpdate,                      TSVNContextMenuEntries::Update,         IDI_UPDATE,             IDS_MENUUPDATE,             IDS_MENUDESCUPDATE,
        {ITEMIS_INSVN,  ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_update" },

    { ShellMenuCommit,                      TSVNContextMenuEntries::Commit,         IDI_COMMIT,             IDS_MENUCOMMIT,             IDS_MENUDESCCOMMIT,
        {ITEMIS_INSVN, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_commit" },

    { ShellSeparator, TSVNContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}},

    { ShellMenuDiff,                        TSVNContextMenuEntries::Diff,           IDI_DIFF,               IDS_MENUDIFF,               IDS_MENUDESCDIFF,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_NORMAL|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_TWO, 0}, {ITEMIS_PROPMODIFIED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, L"tsvn_diff" },

    { ShellMenuDiffLater,                   TSVNContextMenuEntries::Difflater,      IDI_DIFF,               IDS_MENUDIFFLATER,          IDS_MENUDESCDIFFLATER,
        {ITEMIS_ONLYONE, ITEMIS_FOLDER}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_difflater" },

    { ShellMenuDiffNow,                     TSVNContextMenuEntries::Diffnow,        IDI_DIFF,               IDS_MENUDIFFNOW,            IDS_MENUDESCDIFFNOW,
        {ITEMIS_ONLYONE|ITEMIS_HASDIFFLATER, ITEMIS_FOLDER}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_diffnow" },

    { ShellMenuPrevDiff,                    TSVNContextMenuEntries::Prevdiff,       IDI_DIFF,               IDS_MENUPREVDIFF,           IDS_MENUDESCPREVDIFF,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_prevdiff" },

    { ShellMenuUrlDiff,                     TSVNContextMenuEntries::Urldiff,        IDI_DIFF,               IDS_MENUURLDIFF,            IDS_MENUDESCURLDIFF,
        {ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_EXTENDED|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_urldiff" },

    { ShellMenuUnifiedDiff,                 TSVNContextMenuEntries::Unidiff,        IDI_DIFF,               IDS_MENUUNIDIFF,            IDS_MENUUNIDESCDIFF,
        {ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_unifieddiff" },

    { ShellMenuLog,                         TSVNContextMenuEntries::Log,            IDI_LOG,                IDS_MENULOG,                IDS_MENUDESCLOG,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_ADDEDWITHHISTORY|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, L"tsvn_log" },

    { ShellMenuRepoBrowse,                  TSVNContextMenuEntries::Repobrowse,     IDI_REPOBROWSE,         IDS_MENUREPOBROWSE,         IDS_MENUDESCREPOBROWSE,
        {ITEMIS_ONLYONE, 0}, {ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_repobrowse" },

    { ShellMenuShowChanged,                 TSVNContextMenuEntries::Showchanged,    IDI_SHOWCHANGED,        IDS_MENUSHOWCHANGED,        IDS_MENUDESCSHOWCHANGED,
        {ITEMIS_INSVN, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_showchanged"},

    { ShellMenuRevisionGraph,               TSVNContextMenuEntries::Revisiongraph,  IDI_REVISIONGRAPH,      IDS_MENUREVISIONGRAPH,      IDS_MENUDESCREVISIONGRAPH,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_revisiongraph"},

    { ShellSeparator, TSVNContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, L""},

    { ShellMenuConflictEditor,              TSVNContextMenuEntries::Conflicteditor, IDI_CONFLICT,           IDS_MENUCONFLICT,           IDS_MENUDESCCONFLICT,
        {ITEMIS_CONFLICTED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_conflicteditor" },

    { ShellMenuResolve,                     TSVNContextMenuEntries::Resolve,        IDI_RESOLVE,            IDS_MENURESOLVE,            IDS_MENUDESCRESOLVE,
        {ITEMIS_CONFLICTED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_resolve" },

    { ShellMenuUpdateExt,                   TSVNContextMenuEntries::Updateext,      IDI_UPDATE,             IDS_MENUUPDATEEXT,          IDS_MENUDESCUPDATEEXT,
        {ITEMIS_INSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_updateext" },

    { ShellMenuRename,                      TSVNContextMenuEntries::Rename,         IDI_RENAME,             IDS_MENURENAME,             IDS_MENUDESCRENAME,
        {ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_INVERSIONEDFOLDER, ITEMIS_FILEEXTERNAL|ITEMIS_WCROOT|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_rename" },

    { ShellMenuRemove,                      TSVNContextMenuEntries::Remove,         IDI_DELETE,             IDS_MENUREMOVE,             IDS_MENUDESCREMOVE,
        {ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER, ITEMIS_ADDED|ITEMIS_FILEEXTERNAL|ITEMIS_WCROOT|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_remove" },

    { ShellMenuRemoveKeep,                  TSVNContextMenuEntries::Remove,         IDI_DELETE,             IDS_MENUREMOVEKEEP,         IDS_MENUDESCREMOVEKEEP,
        {ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER|ITEMIS_EXTENDED, ITEMIS_ADDED|ITEMIS_WCROOT|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_removekeep" },

    { ShellMenuRevert,                      TSVNContextMenuEntries::Revert,         IDI_REVERT,             IDS_MENUREVERT,             IDS_MENUDESCREVERT,
        {ITEMIS_INSVN, ITEMIS_NORMAL|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_revert" },

    { ShellMenuRevert,                      TSVNContextMenuEntries::Revert,         IDI_REVERT,             IDS_MENUUNDOADD,            IDS_MENUDESCUNDOADD,
        {ITEMIS_ADDED, ITEMIS_NORMAL|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_ADDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_revert" },

    { ShellMenuDelUnversioned,              TSVNContextMenuEntries::Delunversioned, IDI_DELUNVERSIONED,     IDS_MENUDELUNVERSIONED,     IDS_MENUDESCDELUNVERSIONED,
        {ITEMIS_FOLDER|ITEMIS_INSVN|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_delunversioned" },

    { ShellMenuCleanup,                     TSVNContextMenuEntries::Cleanup,        IDI_CLEANUP,            IDS_MENUCLEANUP,            IDS_MENUDESCCLEANUP,
        {ITEMIS_INSVN|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_cleanup" },

    { ShellMenuLock,                        TSVNContextMenuEntries::Lock,           IDI_LOCK,               IDS_MENU_LOCK,              IDS_MENUDESC_LOCK,
        {ITEMIS_INSVN, ITEMIS_LOCKED|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_LOCKED|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_lock" },

    { ShellMenuUnlock,                      TSVNContextMenuEntries::Unlock,         IDI_UNLOCK,             IDS_MENU_UNLOCK,            IDS_MENUDESC_UNLOCK,
        {ITEMIS_INSVN|ITEMIS_LOCKED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_INSVN, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, L"tsvn_unlock" },

    { ShellMenuUnlockForce,                 TSVNContextMenuEntries::Unlock,         IDI_UNLOCK,             IDS_MENU_UNLOCKFORCE,       IDS_MENUDESC_UNLOCKFORCE,
        {ITEMIS_INSVN|ITEMIS_LOCKED|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_INSVN|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_unlockforce" },

    { ShellSeparator, TSVNContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, L""},

    { ShellMenuCopy,                        TSVNContextMenuEntries::Copy,           IDI_COPY,               IDS_MENUBRANCH,             IDS_MENUDESCCOPY,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_copy" },

    { ShellMenuSwitch,                      TSVNContextMenuEntries::Switch,         IDI_SWITCH,             IDS_MENUSWITCH,             IDS_MENUDESCSWITCH,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_switch" },

    { ShellMenuMerge,                       TSVNContextMenuEntries::Merge,          IDI_MERGE,              IDS_MENUMERGE,              IDS_MENUDESCMERGE,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_merge" },
    { ShellMenuMergeAll,                    TSVNContextMenuEntries::Mergeall,       IDI_MERGE,              IDS_MENUMERGEALL,           IDS_MENUDESCMERGEALL,
        {ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_mergeall" },

    { ShellMenuExport,                      TSVNContextMenuEntries::Export,         IDI_EXPORT,             IDS_MENUEXPORT,             IDS_MENUDESCEXPORT,
        {ITEMIS_FOLDER|ITEMIS_ONLYONE, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_export" },

    { ShellMenuRelocate,                    TSVNContextMenuEntries::Relocate,       IDI_RELOCATE,           IDS_MENURELOCATE,           IDS_MENUDESCRELOCATE,
        {ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE|ITEMIS_WCROOT, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE|ITEMIS_WCROOT, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_relocate" },

    { ShellSeparator, TSVNContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, L""},

    { ShellMenuCreateRepos,                 TSVNContextMenuEntries::Createrepos,    IDI_CREATEREPOS,        IDS_MENUCREATEREPOS,        IDS_MENUDESCCREATEREPOS,
        {ITEMIS_FOLDER|ITEMIS_ONLYONE, ITEMIS_INSVN|ITEMIS_FOLDERINSVN}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_createrepo" },

    { ShellMenuAdd,                         TSVNContextMenuEntries::Add,            IDI_ADD,                IDS_MENUADD,                IDS_MENUDESCADD,
        {ITEMIS_INVERSIONEDFOLDER|ITEMIS_FOLDER, ITEMIS_INSVN|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_INSVN|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_IGNORED|ITEMIS_FOLDER, ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_DELETED, ITEMIS_FOLDER|ITEMIS_ONLYONE|ITEMIS_UNSUPPORTEDFORMAT}, L"tsvn_add" },

    { ShellMenuAdd,                         TSVNContextMenuEntries::Add,            IDI_ADD,                IDS_MENUADDIMMEDIATE,       IDS_MENUDESCADD,
        {ITEMIS_INVERSIONEDFOLDER, ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {ITEMIS_IGNORED, ITEMIS_FOLDER|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_DELETED, ITEMIS_FOLDER|ITEMIS_ONLYONE|ITEMIS_UNSUPPORTEDFORMAT}, L"tsvn_add" },

    { ShellMenuAddAsReplacement,            TSVNContextMenuEntries::Add,            IDI_ADD,                IDS_MENUADDASREPLACEMENT,   IDS_MENUADDASREPLACEMENT,
        {ITEMIS_DELETED|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_addasreplacement" },

    { ShellMenuImport,                      TSVNContextMenuEntries::Import,         IDI_IMPORT,             IDS_MENUIMPORT,             IDS_MENUDESCIMPORT,
        {ITEMIS_FOLDER, ITEMIS_INSVN}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_import" },

    { ShellMenuBlame,                       TSVNContextMenuEntries::Blame,          IDI_BLAME,              IDS_MENUBLAME,              IDS_MENUDESCBLAME,
        {ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_blame" },

    { ShellMenuCopyUrl,                     TSVNContextMenuEntries::Copyurl,        IDI_COPYURL,            IDS_MENUCOPYURL,            IDS_MENUDESCCOPYURL,
        { ITEMIS_INSVN, ITEMIS_UNSUPPORTEDFORMAT }, { 0, 0 }, { 0, 0 }, { 0, 0 }, L"tsvn_copyurl" },

    { ShellMenuIgnoreSub,                   TSVNContextMenuEntries::Ignore,         IDI_IGNORE,             IDS_MENUIGNORE,             IDS_MENUDESCIGNORE,
        {ITEMIS_INVERSIONEDFOLDER, ITEMIS_IGNORED|ITEMIS_INSVN|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_ignoresub" },

    { ShellMenuDeleteIgnoreSub,             TSVNContextMenuEntries::Ignore,         IDI_IGNORE,             IDS_MENUDELETEIGNORE,       IDS_MENUDESCDELETEIGNORE,
        {ITEMIS_INVERSIONEDFOLDER|ITEMIS_INSVN, ITEMIS_IGNORED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_deleteignoresub" },

    { ShellMenuUnIgnoreSub,                 TSVNContextMenuEntries::Ignore,         IDI_IGNORE,             IDS_MENUUNIGNORE,           IDS_MENUDESCUNIGNORE,
        {ITEMIS_IGNORED, 0}, {0, 0}, {0, 0}, {0, 0} , L"tsvn_unignoresub"},

    { ShellSeparator, TSVNContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, L""},

    { ShellMenuShelve,                      TSVNContextMenuEntries::Shelve,         IDI_SHELVE,             IDS_MENUSHELVE,             IDS_MENUDESCSHELVE,
        {ITEMIS_INSVN, ITEMIS_NORMAL|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_shelve" },

    { ShellMenuUnshelve,                    TSVNContextMenuEntries::Unshelve,       IDI_UNSHELVE,           IDS_MENUUNSHELVE,           IDS_MENUDESCUNSHELVE,
        {ITEMIS_INSVN, 0}, { 0, 0 }, { 0, 0 }, {0, 0}, L"tsvn_unshelve" },

    { ShellMenuCreatePatch,                 TSVNContextMenuEntries::Createpatch,    IDI_CREATEPATCH,        IDS_MENUCREATEPATCH,        IDS_MENUDESCCREATEPATCH,
        {ITEMIS_INSVN, ITEMIS_NORMAL|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_FOLDERINSVN, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, L"tsvn_createpatch" },

    { ShellMenuApplyPatch,                  TSVNContextMenuEntries::Applypatch,     IDI_PATCH,              IDS_MENUAPPLYPATCH,         IDS_MENUDESCAPPLYPATCH,
        {ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {ITEMIS_ONLYONE|ITEMIS_PATCHFILE, 0}, {ITEMIS_FOLDERINSVN, ITEMIS_ADDED|ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, L"tsvn_applypatch" },

    { ShellMenuProperties,                  TSVNContextMenuEntries::Properties,     IDI_PROPERTIES,         IDS_MENUPROPERTIES,         IDS_MENUDESCPROPERTIES,
        {ITEMIS_INSVN, 0}, {ITEMIS_FOLDERINSVN, 0}, {0, 0}, {0, 0}, L"tsvn_properties" },

    { ShellSeparator, TSVNContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, L""},
    { ShellMenuClipPaste,                   TSVNContextMenuEntries::Clippaste,      IDI_CLIPPASTE,          IDS_MENUCLIPPASTE,          IDS_MENUDESCCLIPPASTE,
        {ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_PATHINCLIPBOARD, ITEMIS_UNSUPPORTEDFORMAT}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_clippaste" },

    { ShellSeparator, TSVNContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, L""},

    { ShellMenuSettings,                    TSVNContextMenuEntries::Settings,       IDI_SETTINGS,           IDS_MENUSETTINGS,           IDS_MENUDESCSETTINGS,
        {ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0}, L"tsvn_settings" },
    { ShellMenuHelp,                        TSVNContextMenuEntries::Help,           IDI_HELP,               IDS_MENUHELP,               IDS_MENUDESCHELP,
        {ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0}, L"tsvn_help" },
    { ShellMenuAbout,                       TSVNContextMenuEntries::About,          IDI_ABOUT,              IDS_MENUABOUT,              IDS_MENUDESCABOUT,
        {ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0}, L"tsvn_about" },

    // the sub menus - they're not added like the the commands, therefore the menu ID is zero
    // but they still need to be in here, because we use the icon and string information anyway.
    { ShellSubMenu,                         TSVNContextMenuEntries::None,           IDI_APP,                IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_submenu" },
    { ShellSubMenuFile,                     TSVNContextMenuEntries::None,           IDI_MENUFILE,           IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_submenufile" },
    { ShellSubMenuFolder,                   TSVNContextMenuEntries::None,           IDI_MENUFOLDER,         IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_submenufolder" },
    { ShellSubMenuLink,                     TSVNContextMenuEntries::None,           IDI_MENULINK,           IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_submenulink" },
    { ShellSubMenuMultiple,                 TSVNContextMenuEntries::None,           IDI_MENUMULTIPLE,       IDS_MENUSUBMENU,            0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, L"tsvn_submenumultiple" },
    // mark the last entry to tell the loop where to stop iterating over this array
    { ShellMenuLastEntry,                   TSVNContextMenuEntries::None,           0,                      0,                          0,
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, L"" },
};
// clang-format on

STDMETHODIMP CShellExt::Initialize(PCIDLIST_ABSOLUTE pIDFolder,
                                   LPDATAOBJECT      pDataObj,
                                   HKEY /* hRegKey */)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: Initialize\n");
    PreserveChdir preserveChdir;
    m_files.clear();
    m_urls.clear();
    m_folder.clear();
    uuidSource.clear();
    uuidTarget.clear();
    itemStates       = 0;
    itemStatesFolder = 0;
    std::wstring       statusPath;
    svn_wc_status_kind fetchedStatus = svn_wc_status_none;

    // get selected files/folders
    if (pDataObj)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {static_cast<CLIPFORMAT>(g_shellIdList),
                          static_cast<DVTARGETDEVICE*>(nullptr),
                          DVASPECT_CONTENT,
                          -1,
                          TYMED_HGLOBAL};
        HRESULT   hRes = pDataObj->GetData(&fmte, &medium);

        if (SUCCEEDED(hRes) && medium.hGlobal)
        {
            if (m_state == FileStateDropHandler)
            {
                if (!CRegStdDWORD(L"Software\\TortoiseSVN\\EnableDragContextMenu", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY))
                {
                    ReleaseStgMedium(&medium);
                    return S_OK;
                }

                FORMATETC etc = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM stg = {TYMED_HGLOBAL};
                if (FAILED(pDataObj->GetData(&etc, &stg)))
                {
                    ReleaseStgMedium(&medium);
                    return E_INVALIDARG;
                }

                HDROP drop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
                if (nullptr == drop)
                {
                    ReleaseStgMedium(&stg);
                    ReleaseStgMedium(&medium);
                    return E_INVALIDARG;
                }

                int count = DragQueryFile(drop, static_cast<UINT>(-1), nullptr, 0);
                if (count == 1)
                    itemStates |= ITEMIS_ONLYONE;
                for (int i = 0; i < count; i++)
                {
                    // find the path length in chars
                    UINT len = DragQueryFile(drop, i, nullptr, 0);
                    if (len == 0)
                        continue;
                    auto szFileName = std::make_unique<wchar_t[]>(len + 1LL);
                    if (0 == DragQueryFile(drop, i, szFileName.get(), len + 1))
                    {
                        continue;
                    }
                    std::wstring str = std::wstring(szFileName.get());
                    if (str.empty() || (!g_shellCache.IsContextPathAllowed(szFileName.get())))
                        continue;
                    CTSVNPath strPath;
                    // only use GetLongPathname for the first item, since we only get the status for
                    // that first item. TortoiseProc later converts the file names before using them too.
                    strPath.SetFromWin(i != 0 ? str.c_str() : CPathUtils::GetLongPathname(str).c_str());
                    if (itemStates & ITEMIS_ONLYONE)
                    {
                        itemStates |= (strPath.GetFileExtension().CompareNoCase(L".diff") == 0) ? ITEMIS_PATCHFILE : 0;
                        itemStates |= (strPath.GetFileExtension().CompareNoCase(L".patch") == 0) ? ITEMIS_PATCHFILE : 0;
                    }
                    m_files.push_back(strPath.GetWinPath());
                    if (i != 0)
                        continue;
                    //get the Subversion status of the item
                    svn_wc_status_kind status = svn_wc_status_none;
                    try
                    {
                        SVNStatus stat;
                        stat.GetStatus(strPath, false, true, true);
                        if (stat.status)
                        {
                            statusPath = str;
                            status     = stat.status->node_status;
                            itemStates |= stat.status->prop_status > svn_wc_status_normal ? ITEMIS_PROPMODIFIED : 0;
                            fetchedStatus = status;
                            if (stat.status->lock && stat.status->lock->token)
                                itemStates |= (stat.status->lock->token[0] != 0) ? ITEMIS_LOCKED : 0;
                            if (stat.status->kind == svn_node_dir)
                            {
                                itemStates |= ITEMIS_FOLDER;
                                if ((status != svn_wc_status_unversioned) && (status != svn_wc_status_ignored) && (status != svn_wc_status_none))
                                    itemStates |= ITEMIS_FOLDERINSVN;
                                if (strPath.IsWCRoot())
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
                            if (stat.status->repos_relpath && stat.status->repos_root_url)
                            {
                                size_t sLen = strlen(stat.status->repos_relpath) + strlen(stat.status->repos_root_url);
                                sLen += 2;
                                auto url = std::make_unique<char[]>(sLen);
                                strcpy_s(url.get(), sLen, stat.status->repos_root_url);
                                strcat_s(url.get(), sLen, "/");
                                strcat_s(url.get(), sLen, stat.status->repos_relpath);
                                m_urls.push_back(CUnicodeUtils::StdGetUnicode(url.get()));
                            }
                        }
                        else
                        {
                            if (stat.IsUnsupportedFormat())
                            {
                                itemStates |= ITEMIS_UNSUPPORTEDFORMAT;
                            }
                            if (stat.GetSVNError())
                                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": GetStatus Failed with error:\"%ws\"\n", static_cast<LPCWSTR>(stat.GetLastErrorMessage()));
                            // sometimes, svn_client_status() returns with an error.
                            // in that case, we have to check if the working copy is versioned
                            // anyway to show the 'correct' context menu
                            if (g_shellCache.IsVersioned(str.c_str(), true, false))
                                status = svn_wc_status_normal;
                        }
                    }
                    catch (...)
                    {
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
                    }
                    if ((status != svn_wc_status_unversioned) && (status != svn_wc_status_ignored) && (status != svn_wc_status_none))
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
                GlobalUnlock(drop);
                ReleaseStgMedium(&stg);
            } // if (m_State == FileStateDropHandler)
            else
            {
                //Enumerate PIDLs which the user has selected
                CIDA*      cida = static_cast<CIDA*>(GlobalLock(medium.hGlobal));
                ItemIDList parent(GetPIDLFolder(cida));

                int        count       = cida->cidl;
                BOOL       statFetched = FALSE;
                for (int i = 0; i < count; ++i)
                {
                    ItemIDList child(GetPIDLItem(cida, i), &parent);
                    if (i == 0)
                    {
                        if (g_shellCache.IsVersioned(child.toString().c_str(), (itemStates & ITEMIS_FOLDER) != 0, false))
                            itemStates |= ITEMIS_INVERSIONEDFOLDER;
                    }

                    std::wstring str = child.toString();
                    // resolve shortcuts if it's a multi-selection (single selections are resolved by the shell!)
                    // and only if the parent directory is not versioned: a shortcut inside a working copy
                    // should not be resolved so it can be added to version control and handled as versioned later
                    if ((count > 1) && ((itemStates & ITEMIS_INVERSIONEDFOLDER) == 0))
                    {
                        // test if the item is a shortcut
                        SHFILEINFO fi     = {nullptr};
                        fi.dwAttributes   = SFGAO_LINK;
                        auto pidlAbsolute = ILCombine(GetPIDLFolder(cida), GetPIDLItem(cida, i));
                        if (pidlAbsolute)
                        {
                            DWORD_PTR dwResult = SHGetFileInfo(reinterpret_cast<LPCWSTR>(pidlAbsolute), 0, &fi, sizeof(fi),
                                                               SHGFI_ATTR_SPECIFIED | SHGFI_ATTRIBUTES | SHGFI_PIDL);
                            if (dwResult != 0)
                            {
                                if (fi.dwAttributes & SFGAO_LINK)
                                {
                                    // it is a shortcut, so resolve that shortcut
                                    IShellLink* pShLink = nullptr;
                                    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                                                   IID_IShellLink, reinterpret_cast<LPVOID*>(&pShLink))))
                                    {
                                        IPersistFile* ppf = nullptr;
                                        if (SUCCEEDED(pShLink->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&ppf))))
                                        {
                                            if (SUCCEEDED(ppf->Load(str.c_str(), STGM_READ)))
                                            {
                                                if (SUCCEEDED(pShLink->Resolve(NULL, SLR_NO_UI)))
                                                {
                                                    wchar_t shortcutpath[MAX_PATH] = {0};
                                                    if (SUCCEEDED(pShLink->GetPath(shortcutpath, MAX_PATH, NULL, 0)))
                                                    {
                                                        str = shortcutpath;
                                                    }
                                                }
                                            }
                                            ppf->Release();
                                        }
                                        pShLink->Release();
                                    }
                                }
                            }
                            ILFree(pidlAbsolute);
                        }
                    }
                    if (str.empty() || (!g_shellCache.IsContextPathAllowed(str.c_str())))
                        continue;
                    //check if our menu is requested for a subversion admin directory
                    if (g_SVNAdminDir.IsAdminDirPath(str.c_str()))
                        continue;

                    CTSVNPath strPath;
                    strPath.SetFromWin(CPathUtils::GetLongPathname(str).c_str());
                    m_files.push_back(strPath.GetWinPath());
                    itemStates |= (strPath.GetFileExtension().CompareNoCase(L".diff") == 0) ? ITEMIS_PATCHFILE : 0;
                    itemStates |= (strPath.GetFileExtension().CompareNoCase(L".patch") == 0) ? ITEMIS_PATCHFILE : 0;
                    if (statFetched)
                        continue;
                    //get the Subversion status of the item
                    svn_wc_status_kind status = svn_wc_status_none;
                    try
                    {
                        SVNStatus stat;
                        stat.GetStatus(strPath, false, true, true);
                        statusPath = str;
                        if (stat.status)
                        {
                            status = stat.status->node_status;
                            itemStates |= stat.status->prop_status > svn_wc_status_normal ? ITEMIS_PROPMODIFIED : 0;
                            fetchedStatus = status;
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
                                if ((status != svn_wc_status_unversioned) && (status != svn_wc_status_ignored) && (status != svn_wc_status_none))
                                    itemStates |= ITEMIS_FOLDERINSVN;
                                if (strPath.IsWCRoot())
                                    itemStates |= ITEMIS_WCROOT;
                            }
                            else
                            {
                                if ((status == svn_wc_status_normal) &&
                                    g_shellCache.IsGetLockTop())
                                {
                                    SVNProperties props(strPath, false, false);
                                    if (props.HasProperty("svn:needs-lock"))
                                        itemStates |= ITEMIS_NEEDSLOCK;
                                }
                            }
                            if (stat.status->repos_relpath && stat.status->repos_root_url)
                            {
                                size_t len = strlen(stat.status->repos_relpath) + strlen(stat.status->repos_root_url);
                                len += 2;
                                auto url = std::make_unique<char[]>(len);
                                strcpy_s(url.get(), len, stat.status->repos_root_url);
                                strcat_s(url.get(), len, "/");
                                strcat_s(url.get(), len, stat.status->repos_relpath);
                                m_urls.push_back(CUnicodeUtils::StdGetUnicode(url.get()));
                            }
                        }
                        else
                        {
                            if (stat.IsUnsupportedFormat())
                            {
                                itemStates |= ITEMIS_UNSUPPORTEDFORMAT;
                            }
                            if (stat.GetSVNError())
                                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": GetStatus Failed with error:\"%ws\"\n", static_cast<LPCWSTR>(stat.GetLastErrorMessage()));
                            // sometimes, svn_client_status() returns with an error.
                            // in that case, we have to check if the working copy is versioned
                            // anyway to show the 'correct' context menu
                            if (g_shellCache.IsVersioned(strPath.GetWinPath(), strPath.IsDirectory(), false))
                            {
                                status        = svn_wc_status_normal;
                                fetchedStatus = status;
                            }
                        }
                        statFetched = TRUE;
                    }
                    catch (...)
                    {
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
                    }
                    if ((status != svn_wc_status_unversioned) && (status != svn_wc_status_ignored) && (status != svn_wc_status_none))
                        itemStates |= ITEMIS_INSVN;
                    if (status == svn_wc_status_ignored)
                    {
                        itemStates |= ITEMIS_IGNORED;
                        // the item is ignored. Get the svn:ignored properties so we can (maybe) later
                        // offer a 'remove from ignored list' entry
                        SVNProperties props(strPath.GetContainingDirectory(), false, false);
                        ignoredProps.clear();
                        ignoredGlobalProps.clear();
                        for (int p = 0; p < props.GetCount(); ++p)
                        {
                            if (props.GetItemName(p).compare(SVN_PROP_IGNORE) == 0)
                            {
                                std::string st = props.GetItemValue(p);
                                ignoredProps   = CUnicodeUtils::StdGetUnicode(st);
                                // remove all escape chars ('\\')
                                ignoredProps.erase(std::remove(ignoredProps.begin(), ignoredProps.end(), '\\'), ignoredProps.end());
                            }
                            if (props.GetItemName(p).compare(SVN_PROP_INHERITABLE_IGNORES) == 0)
                            {
                                std::string st     = props.GetItemValue(p);
                                ignoredGlobalProps = CUnicodeUtils::StdGetUnicode(st);
                                // remove all escape chars ('\\')
                                ignoredGlobalProps.erase(std::remove(ignoredGlobalProps.begin(), ignoredGlobalProps.end(), '\\'), ignoredGlobalProps.end());
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
                GlobalUnlock(medium.hGlobal);

                // if the item is a versioned folder, check if there's a patch file
                // in the clipboard to be used in "Apply Patch"
                UINT cFormatDiff = RegisterClipboardFormat(L"TSVN_UNIFIEDDIFF");
                if (cFormatDiff)
                {
                    if (IsClipboardFormatAvailable(cFormatDiff))
                        itemStates |= ITEMIS_PATCHINCLIPBOARD;
                }
                if (IsClipboardFormatAvailable(CF_HDROP))
                    itemStates |= ITEMIS_PATHINCLIPBOARD;
            }

            ReleaseStgMedium(&medium);
            if (medium.pUnkForRelease)
            {
                IUnknown* relInterface = static_cast<IUnknown*>(medium.pUnkForRelease);
                relInterface->Release();
            }
        }
    }

    // get folder background
    if (pIDFolder)
    {
        ItemIDList list(pIDFolder);
        m_folder                  = list.toString();
        svn_wc_status_kind status = svn_wc_status_none;
        if (IsClipboardFormatAvailable(CF_HDROP))
            itemStatesFolder |= ITEMIS_PATHINCLIPBOARD;
        if (g_shellCache.IsContextPathAllowed(m_folder.c_str()))
        {
            if (m_folder.compare(statusPath) != 0)
            {
                try
                {
                    CTSVNPath strpath;
                    strpath.SetFromWin(CPathUtils::GetLongPathname(m_folder).c_str());
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
                        if ((status != svn_wc_status_unversioned) && (status != svn_wc_status_ignored) && (status != svn_wc_status_none))
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
                        if (m_urls.empty() && stat.status->repos_relpath && stat.status->repos_root_url)
                        {
                            size_t len = strlen(stat.status->repos_relpath) + strlen(stat.status->repos_root_url);
                            len += 2;
                            auto url = std::make_unique<char[]>(len);
                            strcpy_s(url.get(), len, stat.status->repos_root_url);
                            strcat_s(url.get(), len, "/");
                            strcat_s(url.get(), len, stat.status->repos_relpath);
                            m_urls.push_back(CUnicodeUtils::StdGetUnicode(url.get()));
                        }
                    }
                    else
                    {
                        if (stat.IsUnsupportedFormat())
                        {
                            itemStates |= ITEMIS_UNSUPPORTEDFORMAT;
                        }
                        if (stat.GetSVNError())
                            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": GetStatus Failed with error:\"%ws\"\n", static_cast<LPCWSTR>(stat.GetLastErrorMessage()));
                        // sometimes, svn_client_status() returns with an error.
                        // in that case, we have to check if the working copy is versioned
                        // anyway to show the 'correct' context menu
                        if (g_shellCache.IsVersioned(m_folder.c_str(), true, false))
                            status = svn_wc_status_normal;
                    }
                }
                catch (...)
                {
                    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
                }
            }
            else
            {
                status           = fetchedStatus;
                itemStatesFolder = itemStates;
            }
        }
        else
        {
            m_folder.clear();
            status = fetchedStatus;
        }
        if ((status != svn_wc_status_unversioned) && (status != svn_wc_status_ignored) && (status != svn_wc_status_none))
        {
            itemStatesFolder |= ITEMIS_FOLDERINSVN;
        }
        if (status == svn_wc_status_ignored)
            itemStatesFolder |= ITEMIS_IGNORED;
        itemStatesFolder |= ITEMIS_FOLDER;
        if (m_files.empty())
            itemStates |= ITEMIS_ONLYONE;
        if (m_state != FileStateDropHandler)
            itemStates |= itemStatesFolder;
    }
    if (m_files.size() == 2)
        itemStates |= ITEMIS_TWO;
    if ((m_files.size() == 1) && (g_shellCache.IsContextPathAllowed(m_files.front().c_str())))
    {
        itemStates |= ITEMIS_ONLYONE;
        if (m_state != FileStateDropHandler)
        {
            if (PathIsDirectory(m_files.front().c_str()))
            {
                m_folder                  = m_files.front();
                svn_wc_status_kind status = svn_wc_status_none;
                if (m_folder.compare(statusPath) != 0)
                {
                    try
                    {
                        CTSVNPath strPath;
                        strPath.SetFromWin(CPathUtils::GetLongPathname(m_folder).c_str());
                        if (strPath.IsWCRoot())
                            itemStates |= ITEMIS_WCROOT;
                        SVNStatus stat;
                        stat.GetStatus(strPath, false, true, true);
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
                    catch (...)
                    {
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in SVNStatus::GetAllStatus()\n");
                    }
                }
                else
                {
                    status = fetchedStatus;
                }
                if ((status != svn_wc_status_unversioned) && (status != svn_wc_status_ignored) && (status != svn_wc_status_none))
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

void CShellExt::InsertSVNMenu(BOOL isTop, HMENU menu, UINT pos, UINT_PTR id, UINT stringId, UINT icon, UINT idCmdFirst, SVNCommands com, const std::wstring& verb)
{
    wchar_t menuTextBuffer[255] = {0};
    MAKESTRING(stringId);

    if (isTop && menu)
    {
        //menu entry for the top context menu, so append an "SVN " before
        //the menu text to indicate where the entry comes from
        wcscpy_s(menuTextBuffer, L"SVN ");
        if (!g_shellCache.HasShellMenuAccelerators())
        {
            // remove the accelerators
            std::wstring temp = stringTableBuffer;
            temp.erase(std::remove(temp.begin(), temp.end(), '&'), temp.end());
            wcscpy_s(stringTableBuffer, temp.c_str());
        }
    }
    if (com == ShellMenuDiffNow)
    {
        // get the path of the saved file and compact it
        wchar_t      compact[40] = {0};
        std::wstring sPath       = regDiffLater;
        PathCompactPathEx(compact, sPath.c_str(), _countof(compact) - 1, 0);

        CString sMenu;
        sMenu.Format(IDS_MENUDIFFNOW, compact);

        wcscat_s(menuTextBuffer, static_cast<LPCWSTR>(sMenu));
    }
    else
        wcscat_s(menuTextBuffer, stringTableBuffer);

    MENUITEMINFO menuItemInfo = {0};
    menuItemInfo.cbSize       = sizeof(menuItemInfo);
    menuItemInfo.fMask        = MIIM_FTYPE | MIIM_ID | MIIM_BITMAP | MIIM_STRING;
    menuItemInfo.fType        = MFT_STRING;
    menuItemInfo.dwTypeData   = menuTextBuffer;
    if (icon)
        menuItemInfo.hbmpItem = m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, icon);
    menuItemInfo.wID = static_cast<UINT>(id);
    if (menu)
        InsertMenuItem(menu, pos, TRUE, &menuItemInfo);
    else
        m_explorerCommands.push_back(CExplorerCommand(menuTextBuffer, icon, com, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));

    myVerbsMap[verb]              = id - idCmdFirst;
    myVerbsMap[verb]              = id;
    myVerbsIDMap[id - idCmdFirst] = verb;
    myVerbsIDMap[id]              = verb;
    // We store the relative and absolute diameter
    // (drawitem callback uses absolute, others relative)
    myIDMap[id - idCmdFirst]      = com;
    myIDMap[id]                   = com;
    if (!isTop)
        mySubMenuMap[pos] = com;
}

bool CShellExt::WriteClipboardPathsToTempFile(std::wstring& tempFile)
{
    tempFile          = std::wstring();
    //write all selected files and paths to a temporary file
    //for TortoiseProc.exe to read out again.
    DWORD pathLength  = GetTempPath(0, nullptr);
    auto  path        = std::make_unique<wchar_t[]>(pathLength + 1LL);
    auto  tempFileBuf = std::make_unique<wchar_t[]>(pathLength + 100LL);
    GetTempPath(pathLength + 1, path.get());
    GetTempFileName(path.get(), L"svn", 0, tempFileBuf.get());
    tempFile       = std::wstring(tempFileBuf.get());

    CAutoFile file = ::CreateFile(tempFileBuf.get(),
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  nullptr,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_TEMPORARY,
                                  nullptr);

    if (!file)
        return false;

    if (!IsClipboardFormatAvailable(CF_HDROP))
    {
        return false;
    }
    if (!OpenClipboard(nullptr))
    {
        return false;
    }

    HGLOBAL    hGlb  = GetClipboardData(CF_HDROP);
    HDROP      hDrop = static_cast<HDROP>(GlobalLock(hGlb));
    const bool bRet  = (hDrop != nullptr);
    if (bRet)
    {
        wchar_t szFileName[MAX_PATH + 1]{};
        DWORD   bytesWritten = 0;
        UINT    cFiles       = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < cFiles; ++i)
        {
            if (DragQueryFile(hDrop, i, szFileName, _countof(szFileName)))
            {
                ::WriteFile(file, szFileName, static_cast<DWORD>(wcslen(szFileName)) * sizeof(wchar_t), &bytesWritten, nullptr);
                ::WriteFile(file, L"\n", 2, &bytesWritten, nullptr);
            }
        }
        GlobalUnlock(hDrop);
    }
    GlobalUnlock(hGlb);

    CloseClipboard();

    return bRet;
}

std::wstring CShellExt::WriteFileListToTempFile(const std::vector<std::wstring>& files, const std::wstring folder)
{
    //write all selected files and paths to a temporary file
    //for TortoiseProc.exe to read out again.
    DWORD pathLength = GetTempPath(0, nullptr);
    auto  path       = std::make_unique<wchar_t[]>(pathLength + 1);
    auto  tempFile   = std::make_unique<wchar_t[]>(pathLength + 100);
    GetTempPath(pathLength + 1, path.get());
    GetTempFileName(path.get(), L"svn", 0, tempFile.get());
    std::wstring retFilePath = std::wstring(tempFile.get());

    CAutoFile    file        = ::CreateFile(tempFile.get(),
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  nullptr,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_TEMPORARY,
                                  nullptr);

    if (!file)
        return std::wstring();

    DWORD written = 0;
    if (files.empty())
    {
        ::WriteFile(file, folder.c_str(), static_cast<DWORD>(folder.size()) * sizeof(wchar_t), &written, nullptr);
        ::WriteFile(file, L"\n", 2, &written, nullptr);
    }

    for (auto I = files.begin(); I != files.end(); ++I)
    {
        ::WriteFile(file, I->c_str(), static_cast<DWORD>(I->size()) * sizeof(wchar_t), &written, nullptr);
        ::WriteFile(file, L"\n", 2, &written, nullptr);
    }
    return retFilePath;
}

STDMETHODIMP CShellExt::QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT& indexMenu)
{
    if (!CRegStdDWORD(L"Software\\TortoiseSVN\\EnableDragContextMenu", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY))
        return S_OK;

    PreserveChdir preserveChdir;
    LoadLangDll();

    if ((uFlags & CMF_DEFAULTONLY) != 0)
        return S_OK; //we don't change the default action

    if (m_files.empty() || m_folder.empty())
        return S_OK;

    if (((uFlags & 0x000f) != CMF_NORMAL) && (!(uFlags & CMF_EXPLORE)) && (!(uFlags & CMF_VERBSONLY)))
        return S_OK;

    bool bSourceAndTargetFromSameRepository = (uuidSource.compare(uuidTarget) == 0) || uuidSource.empty() || uuidTarget.empty();

    //the drop handler only has eight commands, but not all are visible at the same time:
    //if the source file(s) are under version control then those files can be moved
    //to the new location or they can be moved with a rename,
    //if they are unversioned then they can be added to the working copy
    //if they are versioned, they also can be exported to an unversioned location
    UINT idCmd                              = idCmdFirst;

    // SVN move here
    // available if source is versioned, target is versioned, source and target from same repository or target folder is added
    // and the item is not a file external
    if (((bSourceAndTargetFromSameRepository || (itemStatesFolder & ITEMIS_ADDED)) && (itemStatesFolder & ITEMIS_FOLDERINSVN) && (itemStates & ITEMIS_INSVN)) && ((~itemStates) & ITEMIS_FILEEXTERNAL))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVEMENU, 0, idCmdFirst, ShellMenuDropMove, L"tsvn_dropmove");

    // SVN move and rename here
    // available if source is a single, versioned item, target is versioned, source and target from same repository or target folder is added
    // and the item is not a file external
    if (((bSourceAndTargetFromSameRepository || (itemStatesFolder & ITEMIS_ADDED)) && (itemStatesFolder & ITEMIS_FOLDERINSVN) && (itemStates & ITEMIS_INSVN) && (itemStates & ITEMIS_ONLYONE)) && ((~itemStates) & ITEMIS_FILEEXTERNAL))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVERENAMEMENU, 0, idCmdFirst, ShellMenuDropMoveRename, L"tsvn_dropmoverename");

    // SVN copy here
    // available if source is versioned, target is versioned, source and target from same repository or target folder is added
    if ((bSourceAndTargetFromSameRepository || (itemStatesFolder & ITEMIS_ADDED)) && (itemStatesFolder & ITEMIS_FOLDERINSVN) && (itemStates & ITEMIS_INSVN))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYMENU, 0, idCmdFirst, ShellMenuDropCopy, L"tsvn_dropcopy");

    // SVN copy and rename here, source and target from same repository
    // available if source is a single, versioned item, target is versioned or target folder is added
    if ((bSourceAndTargetFromSameRepository || (itemStatesFolder & ITEMIS_ADDED)) && (itemStatesFolder & ITEMIS_FOLDERINSVN) && (itemStates & ITEMIS_INSVN) && (itemStates & ITEMIS_ONLYONE))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYRENAMEMENU, 0, idCmdFirst, ShellMenuDropCopyRename, L"tsvn_dropcopyrename");

    // SVN add here
    // available if target is versioned and source is either unversioned or from another repository
    if ((itemStatesFolder & ITEMIS_FOLDERINSVN) && (((~itemStates) & ITEMIS_INSVN) || !bSourceAndTargetFromSameRepository))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYADDMENU, 0, idCmdFirst, ShellMenuDropCopyAdd, L"tsvn_dropcopyadd");

    // SVN export here
    // available if source is versioned and a folder
    if ((itemStates & ITEMIS_INSVN) && (itemStates & ITEMIS_FOLDER))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTMENU, 0, idCmdFirst, ShellMenuDropExport, L"tsvn_dropexport");

    // SVN export all here
    // available if source is versioned and a folder
    if ((itemStates & ITEMIS_INSVN) && (itemStates & ITEMIS_FOLDER) && (itemStates & ITEMIS_WCROOT))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTEXTENDEDMENU, 0, idCmdFirst, ShellMenuDropExportExtended, L"tsvn_dropexportextended");

    // SVN export changed here
    // available if source is versioned and a folder
    if ((itemStates & ITEMIS_INSVN) && (itemStates & ITEMIS_FOLDER))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTCHANGEDMENU, 0, idCmdFirst, ShellMenuDropExportChanged, L"tsvn_dropexportchanged");

    // SVN vendorbranch here
    // available if target is versioned and source is either unversioned or from another repository
    if ((itemStatesFolder & ITEMIS_FOLDERINSVN) && (((~itemStates) & ITEMIS_INSVN) || !bSourceAndTargetFromSameRepository))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPVENDORMENU, 0, idCmdFirst, ShellMenuDropVendor, L"tsvn_dropvendor");

    // SVN Add as external here
    // available if source is versioned, target is versioned
    if ((itemStatesFolder & ITEMIS_FOLDERINSVN) && (itemStates & ITEMIS_INSVN))
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXTERNAL, 0, idCmdFirst, ShellMenuDropExternals, L"tsvn_dropexternal");

    // apply patch
    // available if source is a patchfile
    if (itemStates & ITEMIS_PATCHFILE)
        InsertSVNMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_MENUAPPLYPATCH, 0, idCmdFirst, ShellMenuApplyPatch, L"tsvn_applypatch");

    // separator
    if (idCmd != idCmdFirst)
        InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);

    TweakMenu(hMenu);

    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, static_cast<USHORT>(idCmd - idCmdFirst)));
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT  indexMenu,
                                         UINT  idCmdFirst,
                                         UINT /*idCmdLast*/,
                                         UINT uFlags)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: QueryContextMenu itemStates=%ld\n", itemStates);
    PreserveChdir preserveChdir;

    //first check if our drop handler is called
    //and then (if true) provide the context menu for the
    //drop handler
    if (m_state == FileStateDropHandler)
    {
        return QueryDropContext(uFlags, idCmdFirst, hMenu, indexMenu);
    }

    if ((uFlags & CMF_DEFAULTONLY) != 0)
        return S_OK; //we don't change the default action

    if (m_files.empty() && m_folder.empty())
        return S_OK;

    if (((uFlags & 0x000f) != CMF_NORMAL) && (!(uFlags & CMF_EXPLORE)) && (!(uFlags & CMF_VERBSONLY)))
        return S_OK;

    int csidlArray[] =
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
            0};
    if (IsIllegalFolder(m_folder, csidlArray))
        return S_OK;

    if (m_folder.empty())
    {
        // folder is empty, but maybe files are selected
        if (m_files.empty())
            return S_OK; // nothing selected - we don't have a menu to show
        // check whether a selected entry is an UID - those are namespace extensions
        // which we can't handle
        for (std::vector<std::wstring>::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
        {
            if (wcsncmp(it->c_str(), L"::{", 3) == 0)
                return S_OK;
        }
    }
    else
    {
        if (wcsncmp(m_folder.c_str(), L"::{", 3) == 0)
            return S_OK;
    }

    if (((uFlags & CMF_EXTENDEDVERBS) == 0) && g_shellCache.HideMenusForUnversionedItems())
    {
        if ((itemStates & (ITEMIS_INSVN | ITEMIS_INVERSIONEDFOLDER | ITEMIS_FOLDERINSVN)) == 0)
            return S_OK;
    }
    //check if our menu is requested for a subversion admin directory
    if (g_SVNAdminDir.IsAdminDirPath(m_folder.c_str()))
        return S_OK;

    if ((uFlags & CMF_EXTENDEDVERBS) || (g_shellCache.AlwaysExtended()))
        itemStates |= ITEMIS_EXTENDED;

    regDiffLater.read();
    if (!std::wstring(regDiffLater).empty())
        itemStates |= ITEMIS_HASDIFFLATER;

    const BOOL bShortcut = !!(uFlags & CMF_VERBSONLY);
    if (bShortcut && (m_files.size() == 1))
    {
        // Don't show the context menu for a link if the
        // destination is not part of a working copy.
        // It would only show the standard menu items
        // which are already shown for the lnk-file.
        CString path = m_files.front().c_str();
        if (!g_shellCache.IsVersioned(path, !!PathIsDirectory(path), false))
        {
            return S_OK;
        }
    }

    //check if we already added our menu entry for a folder.
    //we check that by iterating through all menu entries and check if
    //the dwItemData member points to our global ID string. That string is set
    //by our shell extension when the folder menu is inserted.
    if (hMenu)
    {
        wchar_t menuBuf[MAX_PATH] = {0};
        int     count             = GetMenuItemCount(hMenu);
        for (int i = 0; i < count; ++i)
        {
            MENUITEMINFO miif = {0};
            miif.cbSize       = sizeof(MENUITEMINFO);
            miif.fMask        = MIIM_DATA;
            miif.dwTypeData   = menuBuf;
            miif.cch          = _countof(menuBuf);
            GetMenuItemInfo(hMenu, i, TRUE, &miif);
            if (miif.dwItemData == reinterpret_cast<ULONG_PTR>(g_menuIDString))
                return S_OK;
        }
    }

    LoadLangDll();
    UINT                   idCmd           = idCmdFirst;

    //create the sub menu
    HMENU                  subMenu         = hMenu ? CreateMenu() : nullptr;
    int                    indexSubMenu    = 0;

    TSVNContextMenuEntries topMenu11       = g_shellCache.GetMenuLayout11();
    unsigned __int64       topMenu         = g_shellCache.GetMenuLayout();
    unsigned __int64       menuMask        = g_shellCache.GetMenuMask();

    bool                   bAddSeparator   = false;
    bool                   bMenuEntryAdded = false;
    // insert separator at start
    if (hMenu)
        InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);
    idCmd++;
    bool bShowIcons = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\ShowContextMenuIcons", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY));
    for (int menuIndex = 0; menuInfo[menuIndex].command != ShellMenuLastEntry; menuIndex++)
    {
        MenuInfo& menuItem = menuInfo[menuIndex];
        if (menuItem.command == ShellSeparator)
        {
            // we don't add a separator immediately. Because there might not be
            // another 'normal' menu entry after we insert a separator.
            // we simply set a flag here, indicating that before the next
            // 'normal' menu entry, a separator should be added.
            if (bMenuEntryAdded)
                bAddSeparator = true;
            continue;
        }
        // check the conditions whether to show the menu entry or not
        if ((static_cast<QWORD>(menuItem.menuID) & (~menuMask)) == 0)
            continue;

        const bool bInsertMenu = ShouldInsertItem(menuItem);
        if (!bInsertMenu)
            continue;
        // insert a separator
        if ((bMenuEntryAdded) && (bAddSeparator))
        {
            bAddSeparator   = false;
            bMenuEntryAdded = false;
            if (subMenu)
                InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);
            else
                m_explorerCommands.push_back(CExplorerCommand(L"", 0, 0, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
            idCmd++;
        }

        bool isMenu11 = (topMenu11 & menuItem.menuID) != TSVNContextMenuEntries::None;
        // handle special cases (sub menus)
        if ((menuItem.command == ShellMenuIgnoreSub) ||
            (menuItem.command == ShellMenuDeleteIgnoreSub) ||
            (menuItem.command == ShellMenuUnIgnoreSub))
        {
            if (hMenu || isMenu11)
            {
                InsertIgnoreSubmenus(idCmd, idCmdFirst, hMenu, subMenu, indexMenu, indexSubMenu, topMenu, bShowIcons);
                bMenuEntryAdded = true;
            }
        }
        else
        {
            // the 'get lock' command is special
            bool bIsTop = ((topMenu & static_cast<QWORD>(menuItem.menuID)) != 0);
            if (menuItem.command == ShellMenuLock)
            {
                if ((itemStates & ITEMIS_NEEDSLOCK) && g_shellCache.IsGetLockTop())
                    bIsTop = true;
            }
            // the 'upgrade wc' command always has to on the top menu
            if (menuItem.command == ShellMenuUpgradeWC)
            {
                bIsTop   = true;
                isMenu11 = true;
            }
            if (hMenu || isMenu11)
            {
                // insert the menu entry
                InsertSVNMenu(bIsTop,
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
    }

    //add sub menu to main context menu
    //don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
    //see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
    MAKESTRING(IDS_MENUSUBMENU);
    if (!g_shellCache.HasShellMenuAccelerators())
    {
        // remove the accelerators
        std::wstring temp = stringTableBuffer;
        temp.erase(std::remove(temp.begin(), temp.end(), '&'), temp.end());
        wcscpy_s(stringTableBuffer, temp.c_str());
    }
    MENUITEMINFO menuItemInfo = {0};
    menuItemInfo.cbSize       = sizeof(menuItemInfo);
    menuItemInfo.fType        = MFT_STRING;
    menuItemInfo.dwTypeData   = stringTableBuffer;

    UINT uIcon                = bShowIcons ? IDI_APP : 0;
    if (m_folder.size())
    {
        uIcon                       = bShowIcons ? IDI_MENUFOLDER : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuFolder;
        myIDMap[idCmd]              = ShellSubMenuFolder;
        menuItemInfo.dwItemData     = reinterpret_cast<ULONG_PTR>(g_menuIDString);
    }
    else if (!bShortcut && (m_files.size() == 1))
    {
        uIcon                       = bShowIcons ? IDI_MENUFILE : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuFile;
        myIDMap[idCmd]              = ShellSubMenuFile;
    }
    else if (bShortcut && (m_files.size() == 1))
    {
        uIcon                       = bShowIcons ? IDI_MENULINK : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuLink;
        myIDMap[idCmd]              = ShellSubMenuLink;
    }
    else if (m_files.size() > 1)
    {
        uIcon                       = bShowIcons ? IDI_MENUMULTIPLE : 0;
        myIDMap[idCmd - idCmdFirst] = ShellSubMenuMultiple;
        myIDMap[idCmd]              = ShellSubMenuMultiple;
    }
    else
    {
        myIDMap[idCmd - idCmdFirst] = ShellSubMenu;
        myIDMap[idCmd]              = ShellSubMenu;
    }
    HBITMAP bmp        = nullptr;

    menuItemInfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_BITMAP | MIIM_STRING;
    if (bShowIcons)
        menuItemInfo.hbmpItem = m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, uIcon);
    menuItemInfo.hbmpChecked   = bmp;
    menuItemInfo.hbmpUnchecked = bmp;
    menuItemInfo.hSubMenu      = subMenu;
    menuItemInfo.wID           = idCmd++;
    if (hMenu)
    {
        InsertMenuItem(hMenu, indexMenu++, TRUE, &menuItemInfo);

        //separator after
        InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);
        TweakMenu(hMenu);
    }
    idCmd++;

    //return number of menu items added
    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, static_cast<USHORT>(idCmd - idCmdFirst)));
}

void CShellExt::TweakMenu(HMENU hMenu)
{
    MENUINFO mInfo = {};
    mInfo.cbSize   = sizeof(mInfo);
    mInfo.fMask    = MIM_STYLE | MIM_APPLYTOSUBMENUS;
    mInfo.dwStyle  = MNS_CHECKORBMP;

    SetMenuInfo(hMenu, &mInfo);
}

void CShellExt::AddPathCommand(std::wstring& svnCmd, LPCWSTR command, bool bFilesAllowed, const std::vector<std::wstring>& files, const std::wstring folder)
{
    svnCmd += command;
    svnCmd += L" /path:\"";
    if ((bFilesAllowed) && !files.empty())
        svnCmd += files.front();
    else
        svnCmd += folder;
    svnCmd += L"\"";
}

void CShellExt::AddPathFileCommand(std::wstring& svnCmd, LPCWSTR command, const std::vector<std::wstring>& files, const std::wstring folder)
{
    std::wstring tempFile = WriteFileListToTempFile(files, folder);
    svnCmd += command;
    svnCmd += L" /pathfile:\"";
    svnCmd += tempFile;
    svnCmd += L"\"";
    svnCmd += L" /deletepathfile";
}

void CShellExt::AddPathFileDropCommand(std::wstring& svnCmd, LPCWSTR command, const std::vector<std::wstring>& files, const std::wstring folder)
{
    std::wstring tempFile = WriteFileListToTempFile(files, folder);
    svnCmd += command;
    svnCmd += L" /pathfile:\"";
    svnCmd += tempFile;
    svnCmd += L"\"";
    svnCmd += L" /deletepathfile";
    svnCmd += L" /droptarget:\"";
    svnCmd += folder;
    svnCmd += L"\"";
}

// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    PreserveChdir preserveChdir;
    const HRESULT hr = E_INVALIDARG;
    if (lpcmi == nullptr)
        return hr;

    if (m_files.empty() && m_folder.empty())
        return hr;

    UINT_PTR idCmd = LOWORD(lpcmi->lpVerb);

    if (HIWORD(lpcmi->lpVerb))
    {
        std::wstring verb   = CUnicodeUtils::StdGetUnicode(lpcmi->lpVerb);
        auto         verbIt = myVerbsMap.lower_bound(verb);
        if (verbIt != myVerbsMap.end() && verbIt->first == verb)
            idCmd = verbIt->second;
        else
            return hr;
    }

    // See if we have a handler interface for this id
    auto idIt = myIDMap.lower_bound(idCmd);
    if (idIt != myIDMap.end() && idIt->first == idCmd)
    {
        std::wstring tortoiseProcPath  = GetAppDirectory() + L"TortoiseProc.exe";
        std::wstring tortoiseMergePath = GetAppDirectory() + L"TortoiseMerge.exe";
        std::wstring cwdFolder;
        if (m_folder.empty() && m_files.empty())
        {
            // use the users desktop path as the CWD
            wchar_t desktopDir[MAX_PATH] = {0};
            SHGetSpecialFolderPath(lpcmi->hwnd, desktopDir, CSIDL_DESKTOPDIRECTORY, TRUE);
            cwdFolder = desktopDir;
        }
        else
        {
            cwdFolder = m_folder.empty() ? m_files[0] : m_folder;
            if (!PathIsDirectory(cwdFolder.c_str()))
            {
                cwdFolder = cwdFolder.substr(0, cwdFolder.rfind('\\'));
            }
        }

        InvokeCommand(static_cast<SVNCommands>(idIt->second),
                      cwdFolder,
                      GetAppDirectory(),
                      uuidSource,
                      lpcmi->hwnd,
                      itemStates,
                      itemStatesFolder,
                      m_files,
                      m_folder,
                      regDiffLater);
        myIDMap.clear();
        myVerbsIDMap.clear();
        myVerbsMap.clear();

        return S_OK;
    }
    return hr;
}

void CShellExt::InvokeCommand(int cmd, const std::wstring& cwd, const std::wstring& appDir, const std::wstring uuidSource, HWND hParent, DWORD itemStates, DWORD itemStatesFolder, const std::vector<std::wstring>& paths, const std::wstring& folder, CRegStdString& regDiffLater)
{
    //TortoiseProc expects a command line of the form:
    //"/command:<commandname> /pathfile:<path> /startrev:<startrevision> /endrev:<endrevision> /deletepathfile
    // or
    //"/command:<commandname> /path:<path> /startrev:<startrevision> /endrev:<endrevision>
    //
    //* path is a path to a single file/directory for commands which only act on single items (log, checkout, ...)
    //* pathfile is a path to a temporary file which contains a list of file paths
    std::wstring svnCmd = L" /command:";
    switch (cmd)
    {
        case ShellMenuUpgradeWC:
            AddPathFileCommand(svnCmd, L"wcupgrade", paths, folder);
            break;
        case ShellMenuCheckout:
            AddPathCommand(svnCmd, L"checkout", false, paths, folder);
            break;
        case ShellMenuUpdate:
            AddPathFileCommand(svnCmd, L"update", paths, folder);
            break;
        case ShellMenuUpdateExt:
            AddPathFileCommand(svnCmd, L"update", paths, folder);
            svnCmd += L" /rev";
            break;
        case ShellMenuCommit:
            AddPathFileCommand(svnCmd, L"commit", paths, folder);
            break;
        case ShellMenuAdd:
        case ShellMenuAddAsReplacement:
            AddPathFileCommand(svnCmd, L"add", paths, folder);
            break;
        case ShellMenuIgnore:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            break;
        case ShellMenuIgnoreGlobal:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            svnCmd += L" /recursive";
            break;
        case ShellMenuIgnoreCaseSensitive:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            svnCmd += L" /onlymask";
            break;
        case ShellMenuIgnoreCaseSensitiveGlobal:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            svnCmd += L" /onlymask /recursive";
            break;
        case ShellMenuDeleteIgnore:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            svnCmd += L" /delete";
            break;
        case ShellMenuDeleteIgnoreGlobal:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            svnCmd += L" /delete /recursive";
            break;
        case ShellMenuDeleteIgnoreCaseSensitive:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            svnCmd += L" /delete";
            svnCmd += L" /onlymask";
            break;
        case ShellMenuDeleteIgnoreCaseSensitiveGlobal:
            AddPathFileCommand(svnCmd, L"ignore", paths, folder);
            svnCmd += L" /delete";
            svnCmd += L" /onlymask";
            svnCmd += L" /recursive";
            break;
        case ShellMenuUnIgnore:
            AddPathFileCommand(svnCmd, L"unignore", paths, folder);
            break;
        case ShellMenuUnIgnoreGlobal:
            AddPathFileCommand(svnCmd, L"unignore", paths, folder);
            svnCmd += L" /recursive";
            break;
        case ShellMenuUnIgnoreCaseSensitive:
            AddPathFileCommand(svnCmd, L"unignore", paths, folder);
            svnCmd += L" /onlymask";
            break;
        case ShellMenuUnIgnoreCaseSensitiveGlobal:
            AddPathFileCommand(svnCmd, L"unignore", paths, folder);
            svnCmd += L" /onlymask /recursive";
            break;
        case ShellMenuRevert:
            AddPathFileCommand(svnCmd, L"revert", paths, folder);
            break;
        case ShellMenuDelUnversioned:
            AddPathFileCommand(svnCmd, L"delunversioned", paths, folder);
            break;
        case ShellMenuCleanup:
            AddPathFileCommand(svnCmd, L"cleanup", paths, folder);
            break;
        case ShellMenuResolve:
            AddPathFileCommand(svnCmd, L"resolve", paths, folder);
            break;
        case ShellMenuSwitch:
            AddPathCommand(svnCmd, L"switch", true, paths, folder);
            break;
        case ShellMenuImport:
            AddPathCommand(svnCmd, L"import", false, paths, folder);
            break;
        case ShellMenuExport:
            AddPathCommand(svnCmd, L"export", false, paths, folder);
            break;
        case ShellMenuAbout:
            svnCmd += L"about";
            break;
        case ShellMenuCreateRepos:
            AddPathCommand(svnCmd, L"repocreate", false, paths, folder);
            break;
        case ShellMenuMerge:
            AddPathCommand(svnCmd, L"merge", true, paths, folder);
            break;
        case ShellMenuMergeAll:
            AddPathCommand(svnCmd, L"mergeall", true, paths, folder);
            break;
        case ShellMenuCopy:
            AddPathCommand(svnCmd, L"copy", true, paths, folder);
            break;
        case ShellMenuSettings:
            svnCmd += L"settings";
            break;
        case ShellMenuHelp:
            svnCmd += L"help";
            break;
        case ShellMenuRename:
            AddPathCommand(svnCmd, L"rename", true, paths, folder);
            break;
        case ShellMenuRemove:
            AddPathFileCommand(svnCmd, L"remove", paths, folder);
            break;
        case ShellMenuRemoveKeep:
            AddPathFileCommand(svnCmd, L"remove", paths, folder);
            svnCmd += L" /keep";
            break;
        case ShellMenuDiff:
            svnCmd += L"diff /path:\"";
            if (paths.size() == 1)
                svnCmd += paths.front();
            else if (paths.size() == 2)
            {
                auto I = paths.cbegin();
                svnCmd += *I;
                ++I;
                svnCmd += L"\" /path2:\"";
                svnCmd += *I;
            }
            else
                svnCmd += folder;
            svnCmd += L"\"";
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                svnCmd += L" /alternative";
            break;
        case ShellMenuDiffLater:
            svnCmd.clear();
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                regDiffLater.removeValue();
            }
            else if (paths.size() == 1)
            {
                regDiffLater = paths[0];
            }
            break;
        case ShellMenuDiffNow:
            AddPathCommand(svnCmd, L"diff", true, paths, folder);
            svnCmd += L"\" /path2:\"";
            svnCmd += std::wstring(regDiffLater);
            svnCmd += L"\"";
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                svnCmd += L" /alternative";
            break;
        case ShellMenuPrevDiff:
            AddPathCommand(svnCmd, L"prevdiff", true, paths, folder);
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                svnCmd += L" /alternative";
            break;
        case ShellMenuUrlDiff:
            AddPathCommand(svnCmd, L"urldiff", true, paths, folder);
            break;
        case ShellMenuUnifiedDiff:
            AddPathCommand(svnCmd, L"diff", true, paths, folder);
            svnCmd += L" /unified";
            break;
        case ShellMenuDropCopyAdd:
            AddPathFileDropCommand(svnCmd, L"dropcopyadd", paths, folder);
            break;
        case ShellMenuDropCopy:
            AddPathFileDropCommand(svnCmd, L"dropcopy", paths, folder);
            break;
        case ShellMenuDropCopyRename:
            AddPathFileDropCommand(svnCmd, L"dropcopy", paths, folder);
            svnCmd += L" /rename";
            break;
        case ShellMenuDropMove:
            AddPathFileDropCommand(svnCmd, L"dropmove", paths, folder);
            break;
        case ShellMenuDropMoveRename:
            AddPathFileDropCommand(svnCmd, L"dropmove", paths, folder);
            svnCmd += L" /rename";
            break;
        case ShellMenuDropExport:
            AddPathFileDropCommand(svnCmd, L"dropexport", paths, folder);
            break;
        case ShellMenuDropExportExtended:
            AddPathFileDropCommand(svnCmd, L"dropexport", paths, folder);
            svnCmd += L" /extended:unversioned";
            break;
        case ShellMenuDropExportChanged:
            AddPathFileDropCommand(svnCmd, L"dropexport", paths, folder);
            svnCmd += L" /extended:localchanges";
            break;
        case ShellMenuDropExternals:
            AddPathFileDropCommand(svnCmd, L"dropexternals", paths, folder);
            break;
        case ShellMenuDropVendor:
            AddPathFileDropCommand(svnCmd, L"dropvendor", paths, folder);
            break;
        case ShellMenuLog:
            AddPathCommand(svnCmd, L"log", true, paths, folder);
            break;
        case ShellMenuConflictEditor:
            AddPathCommand(svnCmd, L"conflicteditor", true, paths, folder);
            break;
        case ShellMenuRelocate:
            AddPathCommand(svnCmd, L"relocate", false, paths, folder);
            break;
        case ShellMenuShowChanged:
            if (paths.size() > 1)
            {
                AddPathFileCommand(svnCmd, L"repostatus", paths, folder);
            }
            else
            {
                AddPathCommand(svnCmd, L"repostatus", true, paths, folder);
            }
            break;
        case ShellMenuRepoBrowse:
            AddPathCommand(svnCmd, L"repobrowser", true, paths, folder);
            break;
        case ShellMenuBlame:
            AddPathCommand(svnCmd, L"blame", true, paths, folder);
            break;
        case ShellMenuCopyUrl:
            AddPathFileCommand(svnCmd, L"copyurls", paths, folder);
            break;
        case ShellMenuShelve:
            AddPathFileCommand(svnCmd, L"shelve", paths, folder);
            break;
        case ShellMenuUnshelve:
            AddPathFileCommand(svnCmd, L"unshelve", paths, folder);
            break;
        case ShellMenuCreatePatch:
            AddPathFileCommand(svnCmd, L"createpatch", paths, folder);
            break;
        case ShellMenuApplyPatch:
        {
            auto localPaths  = paths;
            auto localFolder = folder;
            if ((itemStates & ITEMIS_PATCHINCLIPBOARD) && ((~itemStates) & ITEMIS_PATCHFILE))
            {
                // if there's a patch file in the clipboard, we save it
                // to a temporary file and tell TortoiseMerge to use that one
                UINT cFormat = RegisterClipboardFormat(L"TSVN_UNIFIEDDIFF");
                if ((cFormat) && (OpenClipboard(nullptr)))
                {
                    HGLOBAL hGlb  = GetClipboardData(cFormat);
                    LPCSTR  lpStr = static_cast<LPCSTR>(GlobalLock(hGlb));

                    DWORD   len   = GetTempPath(0, nullptr);
                    auto    path  = std::make_unique<wchar_t[]>(len + 1LL);
                    auto    tempF = std::make_unique<wchar_t[]>(len + 100LL);
                    GetTempPath(len + 1, path.get());
                    GetTempFileName(path.get(), L"svn", 0, tempF.get());
                    std::wstring sTempFile = std::wstring(tempF.get());

                    FILE*        outFile;
                    size_t       patchLen = strlen(lpStr);
                    _tfopen_s(&outFile, sTempFile.c_str(), L"wb");
                    if (outFile)
                    {
                        size_t size = fwrite(lpStr, sizeof(char), patchLen, outFile);
                        if (size == patchLen)
                        {
                            itemStates |= ITEMIS_PATCHFILE;
                            if ((folder.empty()) && (!paths.empty()))
                            {
                                localFolder = paths[0];
                            }
                            itemStatesFolder |= ITEMIS_FOLDERINSVN;
                            localPaths.clear();
                            localPaths.push_back(sTempFile);
                        }
                        fclose(outFile);
                    }
                    GlobalUnlock(hGlb);
                    CloseClipboard();
                }
            }
            if (itemStates & ITEMIS_PATCHFILE)
            {
                svnCmd = L" /diff:\"";
                if (!localPaths.empty())
                {
                    svnCmd += localPaths.front();
                    if (itemStatesFolder & ITEMIS_FOLDERINSVN)
                    {
                        svnCmd += L"\" /patchpath:\"";
                        svnCmd += localFolder;
                    }
                }
                else
                    svnCmd += localFolder;
                if (itemStates & ITEMIS_INVERSIONEDFOLDER)
                    svnCmd += L"\" /wc";
                else
                    svnCmd += L"\"";
            }
            else
            {
                svnCmd = L" /patchpath:\"";
                if (!localPaths.empty())
                    svnCmd += localPaths.front();
                else
                    svnCmd += localFolder;
                svnCmd += L"\"";
            }
            if (!uuidSource.empty())
            {
                CRegStdDWORD groupSetting = CRegStdDWORD(L"Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo", 3, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
                switch (static_cast<DWORD>(groupSetting))
                {
                    case 1:
                    case 2:
                    {
                        svnCmd += L" /groupuuid:";
                        svnCmd += uuidSource;
                    }
                    break;
                }
            }
            RunCommand(appDir + L"TortoiseMerge.exe", svnCmd, cwd, L"TortoiseMerge launch failed");
        }
            return;
        case ShellMenuRevisionGraph:
            AddPathCommand(svnCmd, L"revisiongraph", true, paths, folder);
            break;
        case ShellMenuLock:
            AddPathFileCommand(svnCmd, L"lock", paths, folder);
            break;
        case ShellMenuUnlock:
            AddPathFileCommand(svnCmd, L"unlock", paths, folder);
            break;
        case ShellMenuUnlockForce:
            AddPathFileCommand(svnCmd, L"unlock", paths, folder);
            svnCmd += L" /force";
            break;
        case ShellMenuProperties:
            AddPathFileCommand(svnCmd, L"properties", paths, folder);
            break;
        case ShellMenuClipPaste:
        {
            std::wstring tempFile;
            if (WriteClipboardPathsToTempFile(tempFile))
            {
                bool bCopy           = true;
                UINT cPrefDropFormat = RegisterClipboardFormat(L"Preferred DropEffect");
                if (cPrefDropFormat)
                {
                    if (OpenClipboard(hParent))
                    {
                        HGLOBAL hGlb = GetClipboardData(cPrefDropFormat);
                        if (hGlb)
                        {
                            DWORD* effect = static_cast<DWORD*>(GlobalLock(hGlb));
                            if (*effect == DROPEFFECT_MOVE)
                                bCopy = false;
                            GlobalUnlock(hGlb);
                        }
                        CloseClipboard();
                    }
                }

                if (bCopy)
                    svnCmd += L"pastecopy /pathfile:\"";
                else
                    svnCmd += L"pastemove /pathfile:\"";
                svnCmd += tempFile;
                svnCmd += L"\"";
                svnCmd += L" /deletepathfile";
                svnCmd += L" /droptarget:\"";
                svnCmd += folder;
                svnCmd += L"\"";
            }
            else
                return;
        }
        break;
        default:
            break;
            //#endregion
    } // switch (id_it->second)
    if (!svnCmd.empty())
    {
        svnCmd += L" /hwnd:";
        wchar_t buf[30] = {0};
        swprintf_s(buf, L"%p", static_cast<void*>(hParent));
        svnCmd += buf;
        if (!uuidSource.empty())
        {
            CRegStdDWORD groupSetting = CRegStdDWORD(L"Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo", 3, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
            switch (static_cast<DWORD>(groupSetting))
            {
                case 1:
                case 2:
                {
                    svnCmd += L" /groupuuid:";
                    svnCmd += uuidSource;
                }
                break;
            }
        }
        RunCommand(appDir + L"TortoiseProc.exe", svnCmd, cwd, L"TortoiseProc Launch failed");
    }
}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT     uFlags,
                                         UINT     FAR* /*reserved*/,
                                         LPSTR    pszName,
                                         UINT     cchMax)
{
    PreserveChdir preserveChdir;
    //do we know the id?
    auto          idIt = myIDMap.lower_bound(idCmd);
    if (idIt == myIDMap.end() || idIt->first != idCmd)
    {
        return E_INVALIDARG; //no, we don't
    }

    LoadLangDll();

    MAKESTRING(IDS_MENUDESCDEFAULT);
    for (int menuIndex = 0; menuInfo[menuIndex].command != ShellMenuLastEntry; menuIndex++)
    {
        if (menuInfo[menuIndex].command == static_cast<SVNCommands>(idIt->second))
        {
            MAKESTRING(menuInfo[menuIndex].menuDescID);
            break;
        }
    }

    const wchar_t* desc = stringTableBuffer;
    HRESULT        hr   = E_INVALIDARG;
    switch (uFlags)
    {
        case GCS_HELPTEXTA:
        {
            std::string help = CUnicodeUtils::StdGetUTF8(desc);
            lstrcpynA(pszName, help.c_str(), cchMax - 1);
            hr = S_OK;
            break;
        }
        case GCS_HELPTEXTW:
        {
            std::wstring help = desc;
            lstrcpynW(reinterpret_cast<LPWSTR>(pszName), help.c_str(), cchMax - 1);
            hr = S_OK;
            break;
        }
        case GCS_VERBA:
        {
            std::map<UINT_PTR, std::wstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
            if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
            {
                std::string help = CUnicodeUtils::StdGetUTF8(verb_id_it->second);
                lstrcpynA(pszName, help.c_str(), cchMax - 1);
                hr = S_OK;
            }
        }
        break;
        case GCS_VERBW:
        {
            std::map<UINT_PTR, std::wstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
            if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
            {
                std::wstring help = verb_id_it->second;
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": verb : %ws\n", help.c_str());
                lstrcpynW(reinterpret_cast<LPWSTR>(pszName), help.c_str(), cchMax - 1);
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

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    PreserveChdir preserveChdir;

    LRESULT       res;
    if (pResult == nullptr)
        pResult = &res;
    *pResult = FALSE;

    LoadLangDll();
    switch (uMsg)
    {
        case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT* lpmis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            if (lpmis == nullptr)
                break;
            lpmis->itemWidth  = 16;
            lpmis->itemHeight = 16;
            *pResult          = TRUE;
        }
        break;
        case WM_DRAWITEM:
        {
            LPCWSTR         resource;
            DRAWITEMSTRUCT* lpdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if ((lpdis == nullptr) || (lpdis->CtlType != ODT_MENU))
                return S_OK; //not for a menu
            resource = GetMenuTextFromResource(static_cast<int>(myIDMap[lpdis->itemID]));
            if (resource == nullptr)
                return S_OK;
            int  iconWidth  = GetSystemMetrics(SM_CXSMICON);
            int  iconHeight = GetSystemMetrics(SM_CYSMICON);
            auto hIcon      = LoadIconEx(g_hResInst, resource, iconWidth, iconHeight);
            if (hIcon == nullptr)
                return S_OK;
            DrawIconEx(lpdis->hDC,
                       lpdis->rcItem.left,
                       lpdis->rcItem.top + (lpdis->rcItem.bottom - lpdis->rcItem.top - iconHeight) / 2,
                       hIcon, iconWidth, iconHeight,
                       0, nullptr, DI_NORMAL);
            DestroyIcon(hIcon);
            *pResult = TRUE;
        }
        break;
        case WM_MENUCHAR:
        {
            wchar_t* szItem = nullptr;
            if (HIWORD(wParam) != MF_POPUP)
                return S_OK;
            int nChar = LOWORD(wParam);
            if (_istascii(static_cast<wint_t>(nChar)) && _istupper(static_cast<wint_t>(nChar)))
                nChar = tolower(nChar);
            // we have the char the user pressed, now search that char in all our
            // menu items
            std::vector<UINT_PTR> accMenus;
            for (auto it = mySubMenuMap.begin(); it != mySubMenuMap.end(); ++it)
            {
                LPCWSTR resource = GetMenuTextFromResource(static_cast<int>(mySubMenuMap[it->first]));
                if (resource == nullptr)
                    continue;
                szItem       = stringTableBuffer;
                wchar_t* amp = wcschr(szItem, '&');
                if (amp == nullptr)
                    continue;
                amp++;
                int ampChar = LOWORD(*amp);
                if (_istascii(static_cast<wint_t>(ampChar)) && _istupper(static_cast<wint_t>(ampChar)))
                    ampChar = tolower(ampChar);
                if (ampChar == nChar)
                {
                    // yep, we found a menu which has the pressed key
                    // as an accelerator. Add that menu to the list to
                    // process later.
                    accMenus.push_back(it->first);
                }
            }
            if (accMenus.empty())
            {
                // no menu with that accelerator key.
                *pResult = MAKELONG(0, MNC_IGNORE);
                return S_OK;
            }
            if (accMenus.size() == 1)
            {
                // Only one menu with that accelerator key. We're lucky!
                // So just execute that menu entry.
                *pResult = MAKELONG(accMenus[0], MNC_EXECUTE);
                return S_OK;
            }
            if (accMenus.size() > 1)
            {
                // we have more than one menu item with this accelerator key!
                MENUITEMINFO mif;
                mif.cbSize = sizeof(MENUITEMINFO);
                mif.fMask  = MIIM_STATE;
                for (std::vector<UINT_PTR>::iterator it = accMenus.begin(); it != accMenus.end(); ++it)
                {
                    GetMenuItemInfo(reinterpret_cast<HMENU>(lParam), static_cast<UINT>(*it), TRUE, &mif);
                    if (mif.fState == MFS_HILITE)
                    {
                        // this is the selected item, so select the next one
                        ++it;
                        if (it == accMenus.end())
                            *pResult = MAKELONG(accMenus[0], MNC_SELECT);
                        else
                            *pResult = MAKELONG(*it, MNC_SELECT);
                        return S_OK;
                    }
                }
                *pResult = MAKELONG(accMenus[0], MNC_SELECT);
            }
        }
        break;
        default:
            return S_OK;
    }

    return S_OK;
}

LPCWSTR CShellExt::GetMenuTextFromResource(int id)
{
    wchar_t          textBuf[255] = {0};
    LPCWSTR          resource     = nullptr;
    unsigned __int64 layout       = g_shellCache.GetMenuLayout();
    space                         = 6;

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
                const bool lock = (layout & static_cast<QWORD>(TSVNContextMenuEntries::Lock)) || ((itemStates & ITEMIS_NEEDSLOCK) && g_shellCache.IsGetLockTop());
                space           = lock ? 0 : 6;
                if (lock)
                {
                    wcscpy_s(textBuf, L"SVN ");
                    wcscat_s(textBuf, stringTableBuffer);
                    wcscpy_s(stringTableBuffer, textBuf);
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
                space = (layout & static_cast<QWORD>(menuItem.menuID)) ? 0 : 6;
                if (layout & static_cast<QWORD>(menuItem.menuID))
                {
                    wcscpy_s(textBuf, L"SVN ");
                    wcscat_s(textBuf, stringTableBuffer);
                    wcscpy_s(stringTableBuffer, textBuf);
                }
                break;
        }
        return resource;
    }
    return nullptr;
}

UINT CShellExt::IconIdForCommand(int id)
{
    for (int menuIndex = 0; menuInfo[menuIndex].command != ShellMenuLastEntry; menuIndex++)
    {
        auto& menuItem = menuInfo[menuIndex];
        if (menuItem.command != id)
            continue;
        MAKESTRING(menuItem.menuTextID);
        return menuItem.iconID;
    }
    return 0;
}

bool CShellExt::IsIllegalFolder(const std::wstring& folder, int* csidlarray)
{
    wchar_t          buf[MAX_PATH] = {0}; //MAX_PATH ok, since SHGetSpecialFolderPath doesn't return the required buffer length!
    PIDLIST_ABSOLUTE pidl          = nullptr;
    for (int i = 0; csidlarray[i]; i++)
    {
        pidl = nullptr;
        if (SHGetFolderLocation(nullptr, csidlarray[i], nullptr, 0, &pidl) != S_OK)
            continue;
        if (!SHGetPathFromIDList(pidl, buf))
        {
            // not a file system path, definitely illegal for our use
            CoTaskMemFree(pidl);
            continue;
        }
        CoTaskMemFree(pidl);
        if (buf[0] == 0)
            continue;
        if (wcscmp(buf, folder.c_str()) == 0)
            return true;
    }
    return false;
}

void CShellExt::InsertIgnoreSubmenus(UINT& idCmd, UINT idCmdFirst,
                                     HMENU hMenu, HMENU subMenu,
                                     UINT& indexMenu, int& indexSubMenu,
                                     unsigned __int64 topMenu,
                                     bool             bShowIcons)
{
    HMENU                         ignoreSubMenu        = nullptr;
    int                           indexIgnoreSub       = 0;
    bool                          bShowIgnoreMenu      = false;
    wchar_t                       maskBuf[MAX_PATH]    = {0}; // MAX_PATH is ok, since this only holds a filename
    wchar_t                       ignorePath[MAX_PATH] = {0}; // MAX_PATH is ok, since this only holds a filename
    std::vector<CExplorerCommand> exCmds;

    if (m_files.empty())
        return;
    UINT                                icon = bShowIcons ? IDI_IGNORE : 0;

    std::vector<std::wstring>::iterator I    = m_files.begin();
    if (wcsrchr(I->c_str(), '\\'))
        wcscpy_s(ignorePath, wcsrchr(I->c_str(), '\\') + 1);
    else
        wcscpy_s(ignorePath, I->c_str());
    if ((itemStates & ITEMIS_IGNORED) && ((!ignoredProps.empty()) || (!ignoredGlobalProps.empty())))
    {
        // check if the item name is ignored or the mask
        size_t       p          = 0;
        const size_t pathLength = wcslen(ignorePath);
        while ((p = ignoredProps.find(ignorePath, p)) != -1)
        {
            if ((p == 0 || ignoredProps[p - 1] == static_cast<wchar_t>('\n')))
            {
                if (((p + pathLength) == ignoredProps.length()) || (ignoredProps[p + pathLength] == static_cast<wchar_t>('\n')) || (ignoredProps[p + pathLength] == 0))
                {
                    break;
                }
            }
            p++;
        }
        if (p != -1)
        {
            if (hMenu)
            {
                ignoreSubMenu = CreateMenu();
                InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, ignorePath);
            }
            else
                exCmds.push_back(CExplorerCommand(ignorePath, 0, ShellMenuUnIgnore, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
            std::wstring verb                = L"tsvn_" + std::wstring(ignorePath);
            myVerbsMap[verb]                 = idCmd - idCmdFirst;
            myVerbsMap[verb]                 = idCmd;
            myVerbsIDMap[idCmd - idCmdFirst] = verb;
            myVerbsIDMap[idCmd]              = verb;
            myIDMap[idCmd - idCmdFirst]      = ShellMenuUnIgnore;
            myIDMap[idCmd++]                 = ShellMenuUnIgnore;
            bShowIgnoreMenu                  = true;
        }

        p = 0;
        // cppcheck-suppress uselessCallsCompare
        while ((p = ignoredGlobalProps.find(ignoredGlobalProps, p)) != -1)
        {
            if ((p == 0 || ignoredGlobalProps[p - 1] == static_cast<wchar_t>('\n')))
            {
                if (((p + pathLength) == ignoredGlobalProps.length()) || (ignoredGlobalProps[p + pathLength] == static_cast<wchar_t>('\n')) || (ignoredGlobalProps[p + pathLength] == 0))
                {
                    break;
                }
            }
            p++;
        }
        if (p != -1)
        {
            CString temp;
            temp.Format(IDS_MENUIGNOREGLOBAL, ignorePath);
            if (hMenu)
                InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
            else
                exCmds.push_back(CExplorerCommand(static_cast<LPCWSTR>(temp), 0, ShellMenuUnIgnoreGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
            std::wstring verb                = L"tsvn_" + std::wstring(temp);
            myVerbsMap[verb]                 = idCmd - idCmdFirst;
            myVerbsMap[verb]                 = idCmd;
            myVerbsIDMap[idCmd - idCmdFirst] = verb;
            myVerbsIDMap[idCmd]              = verb;
            myIDMap[idCmd - idCmdFirst]      = ShellMenuUnIgnoreGlobal;
            myIDMap[idCmd++]                 = ShellMenuUnIgnoreGlobal;
        }
        wcscpy_s(maskBuf, L"*");
        if (wcsrchr(ignorePath, '.'))
        {
            wcscat_s(maskBuf, wcsrchr(ignorePath, '.'));
            p = ignoredProps.find(maskBuf);
            if ((p != -1) &&
                ((ignoredProps.compare(maskBuf) == 0) || (ignoredProps.find('\n', p) == p + wcslen(maskBuf) + 1) || (ignoredProps.rfind('\n', p) == p - 1)))
            {
                if (hMenu)
                {
                    if (ignoreSubMenu == nullptr)
                        ignoreSubMenu = CreateMenu();

                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, maskBuf);
                }
                else
                    exCmds.push_back(CExplorerCommand(maskBuf, 0, ShellMenuUnIgnoreCaseSensitive, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                std::wstring verb                = L"tsvn_" + std::wstring(maskBuf);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuUnIgnoreCaseSensitive;
                myIDMap[idCmd++]                 = ShellMenuUnIgnoreCaseSensitive;
                bShowIgnoreMenu                  = true;
            }
            p = ignoredGlobalProps.find(maskBuf);
            if ((p != -1) &&
                ((ignoredGlobalProps.compare(maskBuf) == 0) || (ignoredGlobalProps.find('\n', p) == p + wcslen(maskBuf) + 1) || (ignoredGlobalProps.rfind('\n', p) == p - 1)))
            {
                if (ignoreSubMenu == nullptr)
                    ignoreSubMenu = CreateMenu();

                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, maskBuf);
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
                else
                    exCmds.push_back(CExplorerCommand(static_cast<LPCWSTR>(temp), 0, ShellMenuUnIgnoreCaseSensitiveGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                std::wstring verb                = L"tsvn_" + std::wstring(temp);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuUnIgnoreCaseSensitiveGlobal;
                myIDMap[idCmd++]                 = ShellMenuUnIgnoreCaseSensitiveGlobal;
                bShowIgnoreMenu                  = true;
            }
        }
    }
    else if ((itemStates & ITEMIS_IGNORED) == 0)
    {
        bShowIgnoreMenu = true;
        ignoreSubMenu   = CreateMenu();
        if (itemStates & ITEMIS_ONLYONE)
        {
            if (itemStates & ITEMIS_INSVN)
            {
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, ignorePath);
                else
                    exCmds.push_back(CExplorerCommand(ignorePath, 0, ShellMenuDeleteIgnore, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnore;
                myIDMap[idCmd++]            = ShellMenuDeleteIgnore;

                wcscpy_s(maskBuf, L"*");
                if (wcsrchr(ignorePath, '.'))
                {
                    wcscat_s(maskBuf, wcsrchr(ignorePath, '.'));
                    if (hMenu)
                        InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, maskBuf);
                    else
                        exCmds.push_back(CExplorerCommand(maskBuf, 0, ShellMenuDeleteIgnoreCaseSensitive, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                    std::wstring verb                = L"tsvn_" + std::wstring(maskBuf);
                    myVerbsMap[verb]                 = idCmd - idCmdFirst;
                    myVerbsMap[verb]                 = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd]              = verb;
                    myIDMap[idCmd - idCmdFirst]      = ShellMenuDeleteIgnoreCaseSensitive;
                    myIDMap[idCmd++]                 = ShellMenuDeleteIgnoreCaseSensitive;
                }

                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorePath);
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
                else
                    exCmds.push_back(CExplorerCommand(static_cast<LPCWSTR>(temp), 0, ShellMenuDeleteIgnoreGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));

                myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreGlobal;
                myIDMap[idCmd++]            = ShellMenuDeleteIgnoreGlobal;

                wcscpy_s(maskBuf, L"*");
                if (wcsrchr(ignorePath, '.'))
                {
                    wcscat_s(maskBuf, wcsrchr(ignorePath, '.'));
                    temp.Format(IDS_MENUIGNOREGLOBAL, maskBuf);
                    if (hMenu)
                        InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
                    else
                        exCmds.push_back(CExplorerCommand(maskBuf, 0, ShellMenuDeleteIgnoreCaseSensitiveGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                    std::wstring verb                = L"tsvn_" + std::wstring(temp);
                    myVerbsMap[verb]                 = idCmd - idCmdFirst;
                    myVerbsMap[verb]                 = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd]              = verb;
                    myIDMap[idCmd - idCmdFirst]      = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
                    myIDMap[idCmd++]                 = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
                }
            }
            else
            {
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, ignorePath);
                else
                    exCmds.push_back(CExplorerCommand(ignorePath, 0, ShellMenuIgnore, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
                myIDMap[idCmd++]            = ShellMenuIgnore;

                wcscpy_s(maskBuf, L"*");
                if (wcsrchr(ignorePath, '.'))
                {
                    wcscat_s(maskBuf, wcsrchr(ignorePath, '.'));
                    if (hMenu)
                        InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, maskBuf);
                    else
                        exCmds.push_back(CExplorerCommand(maskBuf, 0, ShellMenuIgnoreCaseSensitive, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                    std::wstring verb                = L"tsvn_" + std::wstring(maskBuf);
                    myVerbsMap[verb]                 = idCmd - idCmdFirst;
                    myVerbsMap[verb]                 = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd]              = verb;
                    myIDMap[idCmd - idCmdFirst]      = ShellMenuIgnoreCaseSensitive;
                    myIDMap[idCmd++]                 = ShellMenuIgnoreCaseSensitive;
                }

                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorePath);
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
                else
                    exCmds.push_back(CExplorerCommand(static_cast<LPCWSTR>(temp), 0, ShellMenuIgnoreGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreGlobal;
                myIDMap[idCmd++]            = ShellMenuIgnoreGlobal;

                wcscpy_s(maskBuf, L"*");
                if (wcsrchr(ignorePath, '.'))
                {
                    wcscat_s(maskBuf, wcsrchr(ignorePath, '.'));
                    temp.Format(IDS_MENUIGNOREGLOBAL, maskBuf);
                    if (hMenu)
                        InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
                    else
                        exCmds.push_back(CExplorerCommand(static_cast<LPCWSTR>(temp), 0, ShellMenuIgnoreCaseSensitiveGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                    std::wstring verb                = L"tsvn_" + std::wstring(temp);
                    myVerbsMap[verb]                 = idCmd - idCmdFirst;
                    myVerbsMap[verb]                 = idCmd;
                    myVerbsIDMap[idCmd - idCmdFirst] = verb;
                    myVerbsIDMap[idCmd]              = verb;
                    myIDMap[idCmd - idCmdFirst]      = ShellMenuIgnoreCaseSensitiveGlobal;
                    myIDMap[idCmd++]                 = ShellMenuIgnoreCaseSensitiveGlobal;
                }
            }
        }
        else
        {
            // note: as of Windows 7, the shell does not pass more than 16 items from a multiselection
            // in the Initialize() call before the QueryContextMenu() call. Which means even if the user
            // has selected more than 16 files, we won't know about that here.
            // Note: after QueryContextMenu() exits, Initialize() is called again with all selected files.
            if (itemStates & ITEMIS_INSVN)
            {
                if (m_files.size() >= 16)
                {
                    MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLE2);
                    wcscpy_s(ignorePath, stringTableBuffer);
                }
                else
                {
                    MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLE);
                    swprintf_s(ignorePath, stringTableBuffer, m_files.size());
                }
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, ignorePath);
                else
                    exCmds.push_back(CExplorerCommand(ignorePath, 0, ShellMenuDeleteIgnore, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                std::wstring verb                = L"tsvn_" + std::wstring(ignorePath);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuDeleteIgnore;
                myIDMap[idCmd++]                 = ShellMenuDeleteIgnore;

                if (m_files.size() >= 16)
                {
                    MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK2);
                    wcscpy_s(ignorePath, stringTableBuffer);
                }
                else
                {
                    MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK);
                    swprintf_s(ignorePath, stringTableBuffer, m_files.size());
                }
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, ignorePath);
                else
                    exCmds.push_back(CExplorerCommand(ignorePath, 0, ShellMenuDeleteIgnoreCaseSensitive, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                verb                             = std::wstring(ignorePath);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuDeleteIgnoreCaseSensitive;
                myIDMap[idCmd++]                 = ShellMenuDeleteIgnoreCaseSensitive;

                if (m_files.size() >= 16)
                {
                    MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK2);
                    wcscpy_s(ignorePath, stringTableBuffer);
                }
                else
                {
                    MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK);
                    swprintf_s(ignorePath, stringTableBuffer, m_files.size());
                }
                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorePath);
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
                else
                    exCmds.push_back(CExplorerCommand(static_cast<LPCWSTR>(temp), 0, ShellMenuDeleteIgnoreCaseSensitiveGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                verb                             = std::wstring(temp);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
                myIDMap[idCmd++]                 = ShellMenuDeleteIgnoreCaseSensitiveGlobal;
            }
            else
            {
                if (m_files.size() >= 16)
                {
                    MAKESTRING(IDS_MENUIGNOREMULTIPLE2);
                    wcscpy_s(ignorePath, stringTableBuffer);
                }
                else
                {
                    MAKESTRING(IDS_MENUIGNOREMULTIPLE);
                    swprintf_s(ignorePath, stringTableBuffer, m_files.size());
                }
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, ignorePath);
                else
                    exCmds.push_back(CExplorerCommand(ignorePath, 0, ShellMenuIgnore, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                std::wstring verb                = L"tsvn_" + std::wstring(ignorePath);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuIgnore;
                myIDMap[idCmd++]                 = ShellMenuIgnore;

                if (m_files.size() >= 16)
                {
                    MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK2);
                    wcscpy_s(ignorePath, stringTableBuffer);
                }
                else
                {
                    MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK);
                    swprintf_s(ignorePath, stringTableBuffer, m_files.size());
                }
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, ignorePath);
                else
                    exCmds.push_back(CExplorerCommand(ignorePath, 0, ShellMenuIgnoreCaseSensitive, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                verb                             = std::wstring(ignorePath);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuIgnoreCaseSensitive;
                myIDMap[idCmd++]                 = ShellMenuIgnoreCaseSensitive;

                if (m_files.size() >= 16)
                {
                    MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK2);
                    wcscpy_s(ignorePath, stringTableBuffer);
                }
                else
                {
                    MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK);
                    swprintf_s(ignorePath, stringTableBuffer, m_files.size());
                }
                CString temp;
                temp.Format(IDS_MENUIGNOREGLOBAL, ignorePath);
                if (hMenu)
                    InsertMenu(ignoreSubMenu, indexIgnoreSub++, MF_BYPOSITION | MF_STRING, idCmd, temp);
                else
                    exCmds.push_back(CExplorerCommand(static_cast<LPCWSTR>(temp), 0, ShellMenuIgnoreCaseSensitiveGlobal, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
                verb                             = std::wstring(temp);
                myVerbsMap[verb]                 = idCmd - idCmdFirst;
                myVerbsMap[verb]                 = idCmd;
                myVerbsIDMap[idCmd - idCmdFirst] = verb;
                myVerbsIDMap[idCmd]              = verb;
                myIDMap[idCmd - idCmdFirst]      = ShellMenuIgnoreCaseSensitiveGlobal;
                myIDMap[idCmd++]                 = ShellMenuIgnoreCaseSensitiveGlobal;
            }
        }
    }

    if (bShowIgnoreMenu)
    {
        MENUITEMINFO menuiteminfo = {0};
        menuiteminfo.cbSize       = sizeof(menuiteminfo);
        menuiteminfo.fMask        = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_BITMAP | MIIM_STRING;
        menuiteminfo.fType        = MFT_STRING;
        HBITMAP bmp               = m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, icon);
        if (icon)
            menuiteminfo.hbmpItem = bmp;
        menuiteminfo.hbmpChecked   = bmp;
        menuiteminfo.hbmpUnchecked = bmp;
        menuiteminfo.hSubMenu      = ignoreSubMenu;
        menuiteminfo.wID           = idCmd;
        SecureZeroMemory(stringTableBuffer, sizeof(stringTableBuffer));
        if (itemStates & ITEMIS_IGNORED)
            GetMenuTextFromResource(ShellMenuUnIgnoreSub);
        else if (itemStates & ITEMIS_INSVN)
            GetMenuTextFromResource(ShellMenuDeleteIgnoreSub);
        else
            GetMenuTextFromResource(ShellMenuIgnoreSub);
        menuiteminfo.dwTypeData = stringTableBuffer;
        menuiteminfo.cch        = static_cast<UINT>(std::min(static_cast<UINT>(wcslen(menuiteminfo.dwTypeData)), UINT_MAX));
        if (hMenu)
        {
            InsertMenuItem((topMenu & static_cast<QWORD>(TSVNContextMenuEntries::Ignore)) ? hMenu : subMenu, (topMenu & static_cast<QWORD>(TSVNContextMenuEntries::Ignore)) ? indexMenu++ : indexSubMenu++, TRUE, &menuiteminfo);
        }
        else
        {
            m_explorerCommands.push_back(CExplorerCommand(L"", 0, ShellSeparator, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, {}));
            for (const auto& cmd : exCmds)
            {
                m_explorerCommands.push_back(cmd);
                std::wstring prep = stringTableBuffer;
                prep += L": ";
                m_explorerCommands.back().PrependTitleWith(prep);
            }
            // currently, explorer does not support subcommands which their own subcommands. Once it does,
            // use the line below instead of the ones above
            //m_explorerCommands.push_back(CExplorerCommand(stringTableBuffer, icon, ShellMenuUnIgnoreSub, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, exCmds));
        }
        if (itemStates & ITEMIS_IGNORED)
        {
            myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreSub;
            myIDMap[idCmd++]            = ShellMenuUnIgnoreSub;
        }
        else
        {
            myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreSub;
            myIDMap[idCmd++]            = ShellMenuIgnoreSub;
        }
    }
}

void CShellExt::RunCommand(const std::wstring& path, const std::wstring& command,
                           const std::wstring& folder, LPCWSTR errorMessage)
{
    if (CCreateProcessHelper::CreateProcessDetached(path.c_str(),
                                                    command.c_str(), folder.c_str()))
    {
        // process started - exit
        return;
    }

    CFormatMessageWrapper errorDetails;
    MessageBox(nullptr, errorDetails, errorMessage, MB_OK | MB_ICONINFORMATION);
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
    else if ((pair.yes) && ((pair.yes & itemStates) == pair.yes))
        return true;
    else if ((pair.no) && ((pair.no & (~itemStates)) == pair.no))
        return true;
    return false;
}

// IExplorerCommand
HRESULT __stdcall CShellExt::GetTitle(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszName)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetTitle\n");
    SHStrDupW(L"TortoiseSVN", ppszName);
    return S_OK;
}

HRESULT __stdcall CShellExt::GetIcon(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszIcon)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetIcon\n");
    std::wstring iconPath = GetAppDirectory() + L"TortoiseProc.exe,-";
    iconPath += std::to_wstring(IDI_APP);
    SHStrDupW(iconPath.c_str(), ppszIcon);
    return S_OK;
}

HRESULT __stdcall CShellExt::GetToolTip(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszInfotip)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetToolTip\n");
    *ppszInfotip = nullptr;
    return E_NOTIMPL;
}

HRESULT __stdcall CShellExt::GetCanonicalName(GUID* pguidCommandName)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetCanonicalName\n");
    *pguidCommandName = GUID_NULL;
    return S_OK;
}

HRESULT __stdcall CShellExt::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetState\n");
    *pCmdState = ECS_ENABLED;
    if (m_site)
    {
        Microsoft::WRL::ComPtr<IOleWindow> oleWindow;
        m_site.As(&oleWindow);
        if (oleWindow)
        {
            // We don't want to show the menu on the classic context menu.
            // The classic menu provides an IOleWindow, but the main context
            // menu of the left treeview in explorer does too.
            // So we check the window class name: if it's "NamespaceTreeControl",
            // then we're dealing with the main context menu of the tree view.
            // If it's not, then we're dealing with the classic context menu
            // and there we hide this menu entry.
            HWND hWnd = nullptr;
            oleWindow->GetWindow(&hWnd);
            wchar_t szWndClassName[MAX_PATH] = {0};
            GetClassName(hWnd, szWndClassName, _countof(szWndClassName));
            if (wcscmp(szWndClassName, L"NamespaceTreeControl"))
            {
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetState - hidden\n");
                *pCmdState = ECS_HIDDEN;
                return S_OK;
            }
            else
            {
                // tree view
                if (psiItemArray == nullptr)
                {
                    // just in case Win11 one day provides us with the item that's right-clicked on...
                    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetState - hidden\n");
                    *pCmdState = ECS_HIDDEN;
                    return S_OK;
                }
            }
        }
    }

    if (!fOkToBeSlow)
        return E_PENDING;

    Initialize(nullptr, nullptr, nullptr);
    Microsoft::WRL::ComPtr<IShellItemArray> itemArray;
    if (psiItemArray == nullptr)
    {
        // context menu for a folder background (no selection),
        // so try to get the current path of the explorer window instead
        auto path = ExplorerViewPath();
        if (path.empty())
        {
            *pCmdState = ECS_HIDDEN;
            return S_OK;
        }
        PIDLIST_ABSOLUTE pidl{};
        if (SUCCEEDED(SHParseDisplayName(path.c_str(), nullptr, &pidl, 0, nullptr)))
        {
            if (SUCCEEDED(SHCreateShellItemArrayFromIDLists(1, &pidl, itemArray.GetAddressOf())))
            {
                if (itemArray)
                {
                    psiItemArray = itemArray.Get();
                }
            }
        }
    }
    if (psiItemArray)
    {
        IDataObject* pDataObj = nullptr;
        if (SUCCEEDED(psiItemArray->BindToHandler(nullptr, BHID_DataObject, IID_IDataObject, reinterpret_cast<void**>(&pDataObj))))
        {
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: Initialize from GetState()\n");

            Initialize(nullptr, pDataObj, nullptr);
            pDataObj->Release();
            pDataObj = nullptr;
        }
        else
            *pCmdState = ECS_HIDDEN;
    }

    if (g_shellCache.HideMenusForUnversionedItems() && (GetKeyState(VK_SHIFT) & 0x8000) == 0)
    {
        if ((itemStates & (ITEMIS_INSVN | ITEMIS_INVERSIONEDFOLDER | ITEMIS_FOLDERINSVN)) == 0)
            *pCmdState = ECS_HIDDEN;
    }
    if (*pCmdState != ECS_HIDDEN)
    {
        m_explorerCommands.clear();
        QueryContextMenu(nullptr, 0, 0, 0, CMF_EXTENDEDVERBS | CMF_NORMAL);
        if (m_explorerCommands.empty())
            *pCmdState = ECS_HIDDEN;
    }

    return S_OK;
}

HRESULT __stdcall CShellExt::Invoke(IShellItemArray* /*psiItemArray*/, IBindCtx* /*pbc*/)
{
    return E_NOTIMPL;
}

HRESULT __stdcall CShellExt::GetFlags(EXPCMDFLAGS* pFlags)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetFlags\n");
    *pFlags = ECF_HASSUBCOMMANDS;
    return S_OK;
}

HRESULT __stdcall CShellExt::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: EnumSubCommands\n");
    m_explorerCommands.clear();
    QueryContextMenu(nullptr, 0, 0, 0, CMF_EXTENDEDVERBS | CMF_NORMAL);
    *ppEnum = new CExplorerCommandEnum(m_explorerCommands);
    (*ppEnum)->AddRef();
    return S_OK;
}

std::wstring CShellExt::ExplorerViewPath()
{
    std::wstring path;
    HRESULT      hr   = NOERROR;

    // the top context menu in Win11 does not
    // provide an IOleWindow with the SetSite() object,
    // so we have to use a trick to get it: since the
    // context menu must always be the top window, we
    // just grab the foreground window and assume that
    // this is the explorer window.
    auto         hwnd = ::GetForegroundWindow();
    if (hwnd == nullptr)
        return path;

    wchar_t szName[1024] = {0};
    ::GetClassName(hwnd, szName, _countof(szName));
    if (StrCmp(szName, L"WorkerW") == 0 ||
        StrCmp(szName, L"Progman") == 0)
    {
        //special folder: desktop
        hr = ::SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, szName);
        if (FAILED(hr))
            return path;

        path = szName;
        return path;
    }

    if (StrCmp(szName, L"CabinetWClass") != 0)
        return path;

    // get the shell windows object to enumerate all active explorer
    // instances. We use those to compare the foreground hwnd to it.
    Microsoft::WRL::ComPtr<IShellWindows> shell;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL,
                                IID_IShellWindows, reinterpret_cast<LPVOID*>(shell.GetAddressOf()))))
        return path;

    if (shell == nullptr)
        return path;

    Microsoft::WRL::ComPtr<IDispatch> disp;
    VARIANT                           variant{};
    variant.vt = VT_I4;

    Microsoft::WRL::ComPtr<IWebBrowserApp> browser;
    // look for correct explorer window
    for (variant.intVal = 0; shell->Item(variant, disp.GetAddressOf()) == S_OK; variant.intVal++)
    {
        Microsoft::WRL::ComPtr<IWebBrowserApp> tmp;
        if (FAILED(disp->QueryInterface(tmp.GetAddressOf())))
            continue;

        HWND tmpHwnd = nullptr;
        hr           = tmp->get_HWND(reinterpret_cast<SHANDLE_PTR*>(&tmpHwnd));
        if (hwnd == tmpHwnd)
        {
            browser = tmp;
            break; // found it!
        }
    }

    if (browser != nullptr)
    {
        BSTR url;
        hr = browser->get_LocationURL(&url);
        if (FAILED(hr))
            return path;

        std::wstring sUrl(url, SysStringLen(url));
        SysFreeString(url);
        DWORD size = _countof(szName);
        hr         = ::PathCreateFromUrl(sUrl.c_str(), szName, &size, NULL);
        if (SUCCEEDED(hr))
            path = szName;
    }

    return path;
}
