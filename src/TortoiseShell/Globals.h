// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2013-2017, 2021, 2023 - TortoiseSVN

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
#include "resource.h"

enum class TSVNContextMenuEntries : unsigned __int64
{
    None           = 0x0000000000000000,
    Checkout       = 0x0000000000000001,
    Update         = 0x0000000000000002,
    Commit         = 0x0000000000000004,
    Add            = 0x0000000000000008,
    Revert         = 0x0000000000000010,
    Cleanup        = 0x0000000000000020,
    Resolve        = 0x0000000000000040,
    Switch         = 0x0000000000000080,
    Import         = 0x0000000000000100,
    Export         = 0x0000000000000200,
    Createrepos    = 0x0000000000000400,
    Copy           = 0x0000000000000800,
    Merge          = 0x0000000000001000,
    Remove         = 0x0000000000002000,
    Rename         = 0x0000000000004000,
    Updateext      = 0x0000000000008000,
    Diff           = 0x0000000000010000,
    Log            = 0x0000000000020000,
    Conflicteditor = 0x0000000000040000,
    Relocate       = 0x0000000000080000,
    Showchanged    = 0x0000000000100000,
    Ignore         = 0x0000000000200000,
    Repobrowse     = 0x0000000000400000,
    Blame          = 0x0000000000800000,
    Createpatch    = 0x0000000001000000,
    Applypatch     = 0x0000000002000000,
    Revisiongraph  = 0x0000000004000000,
    Lock           = 0x0000000008000000,
    Unlock         = 0x0000000010000000,
    Properties     = 0x0000000020000000,
    Urldiff        = 0x0000000040000000,
    Delunversioned = 0x0000000080000000,
    Mergeall       = 0x0000000100000000,
    Prevdiff       = 0x0000000200000000,
    Clippaste      = 0x0000000400000000,
    Upgrade        = 0x0000000800000000,
    Difflater      = 0x0000001000000000,
    Diffnow        = 0x0000002000000000,
    Unidiff        = 0x0000004000000000,
    Copyurl        = 0x0000008000000000,
    Shelve         = 0x0000010000000000,
    Unshelve       = 0x0000020000000000,
    Logext         = 0x0000040000000000,

    Settings = 0x2000000000000000,
    Help     = 0x4000000000000000,
    About    = 0x8000000000000000,
};

constexpr TSVNContextMenuEntries operator|(TSVNContextMenuEntries a, TSVNContextMenuEntries b) noexcept
{
    return static_cast<TSVNContextMenuEntries>(static_cast<unsigned __int64>(a) | static_cast<unsigned __int64>(b));
}

constexpr TSVNContextMenuEntries operator&(TSVNContextMenuEntries a, TSVNContextMenuEntries b) noexcept
{
    return static_cast<TSVNContextMenuEntries>(static_cast<unsigned __int64>(a) & static_cast<unsigned __int64>(b));
}

inline TSVNContextMenuEntries &operator|=(TSVNContextMenuEntries &self, TSVNContextMenuEntries a) noexcept
{
    self = self | a;
    return self;
}

constexpr TSVNContextMenuEntries defaultWin11TopMenuEntries = TSVNContextMenuEntries::Checkout |
                                                              TSVNContextMenuEntries::Update |
                                                              TSVNContextMenuEntries::Commit |
                                                              TSVNContextMenuEntries::Diff |
                                                              TSVNContextMenuEntries::Revert |
                                                              TSVNContextMenuEntries::Log |
                                                              TSVNContextMenuEntries::Rename |
                                                              TSVNContextMenuEntries::Switch |
                                                              TSVNContextMenuEntries::Copy |
                                                              TSVNContextMenuEntries::Conflicteditor |
                                                              TSVNContextMenuEntries::Repobrowse |
                                                              TSVNContextMenuEntries::Showchanged |
                                                              TSVNContextMenuEntries::Blame |
                                                              TSVNContextMenuEntries::Properties |
                                                              TSVNContextMenuEntries::Revisiongraph |
                                                              TSVNContextMenuEntries::Merge |
                                                              TSVNContextMenuEntries::Settings;

const std::vector<std::tuple<TSVNContextMenuEntries, UINT, UINT>> TSVNContextMenuEntriesVec{
    {TSVNContextMenuEntries::Checkout, IDS_MENUCHECKOUT, IDI_CHECKOUT},
    {TSVNContextMenuEntries::Update, IDS_MENUUPDATE, IDI_UPDATE},
    {TSVNContextMenuEntries::Commit, IDS_MENUCOMMIT, IDI_COMMIT},
    {TSVNContextMenuEntries::Add, IDS_MENUADD, IDI_ADD},
    {TSVNContextMenuEntries::Revert, IDS_MENUREVERT, IDI_REVERT},
    {TSVNContextMenuEntries::Cleanup, IDS_MENUCLEANUP, IDI_CLEANUP},
    {TSVNContextMenuEntries::Resolve, IDS_MENURESOLVE, IDI_RESOLVE},
    {TSVNContextMenuEntries::Switch, IDS_MENUSWITCH, IDI_SWITCH},
    {TSVNContextMenuEntries::Import, IDS_MENUIMPORT, IDI_IMPORT},
    {TSVNContextMenuEntries::Export, IDS_MENUEXPORT, IDI_EXPORT},
    {TSVNContextMenuEntries::Createrepos, IDS_MENUCREATEREPOS, IDI_CREATEREPOS},
    {TSVNContextMenuEntries::Copy, IDS_MENUBRANCH, IDI_COPY},
    {TSVNContextMenuEntries::Merge, IDS_MENUMERGE, IDI_MERGE},
    {TSVNContextMenuEntries::Remove, IDS_MENUREMOVE, IDI_DELETE},
    {TSVNContextMenuEntries::Rename, IDS_MENURENAME, IDI_RENAME},
    {TSVNContextMenuEntries::Updateext, IDS_MENUUPDATEEXT, IDI_UPDATE},
    {TSVNContextMenuEntries::Diff, IDS_MENUDIFF, IDI_DIFF},
    {TSVNContextMenuEntries::Log, IDS_MENULOG, IDI_LOG},
    {TSVNContextMenuEntries::Conflicteditor, IDS_MENUCONFLICT, IDI_CONFLICT},
    {TSVNContextMenuEntries::Relocate, IDS_MENURELOCATE, IDI_RELOCATE},
    {TSVNContextMenuEntries::Showchanged, IDS_MENUSHOWCHANGED, IDI_SHOWCHANGED},
    {TSVNContextMenuEntries::Ignore, IDS_MENUIGNORE, IDI_IGNORE},
    {TSVNContextMenuEntries::Repobrowse, IDS_MENUREPOBROWSE, IDI_REPOBROWSE},
    {TSVNContextMenuEntries::Blame, IDS_MENUBLAME, IDI_BLAME},
    {TSVNContextMenuEntries::Createpatch, IDS_MENUCREATEPATCH, IDI_CREATEPATCH},
    {TSVNContextMenuEntries::Applypatch, IDS_MENUAPPLYPATCH, IDI_PATCH},
    {TSVNContextMenuEntries::Revisiongraph, IDS_MENUREVISIONGRAPH, IDI_REVISIONGRAPH},
    {TSVNContextMenuEntries::Lock, IDS_MENU_LOCK, IDI_LOCK},
    {TSVNContextMenuEntries::Unlock, IDS_MENUDESC_UNLOCK, IDI_UNLOCK},
    {TSVNContextMenuEntries::Properties, IDS_MENUPROPERTIES, IDI_PROPERTIES},
    {TSVNContextMenuEntries::Urldiff, IDS_MENUURLDIFF, IDI_DIFF},
    {TSVNContextMenuEntries::Delunversioned, IDS_MENUDELUNVERSIONED, IDI_DELUNVERSIONED},
    {TSVNContextMenuEntries::Mergeall, IDS_MENUMERGEALL, IDI_MERGE},
    {TSVNContextMenuEntries::Prevdiff, IDS_MENUPREVDIFF, IDI_DIFF},
    {TSVNContextMenuEntries::Clippaste, IDS_MENUCLIPPASTE, IDI_CLIPPASTE},
    {TSVNContextMenuEntries::Upgrade, IDS_MENUUPGRADE, IDI_CLEANUP},
    {TSVNContextMenuEntries::Difflater, IDS_MENUDIFFLATER, IDI_DIFF},
    {TSVNContextMenuEntries::Diffnow, IDS_MENUDIFFNOW, IDI_DIFF},
    {TSVNContextMenuEntries::Unidiff, IDS_MENUUNIDIFF, IDI_DIFF},
    {TSVNContextMenuEntries::Copyurl, IDS_MENUCOPYURL, IDI_COPYURL},
    {TSVNContextMenuEntries::Shelve, IDS_MENUSHELVE, IDI_SHELVE},
    {TSVNContextMenuEntries::Unshelve, IDS_MENUUNSHELVE, IDI_UNSHELVE},
    {TSVNContextMenuEntries::Logext, IDS_MENULOGEXT, IDI_LOG},

    {TSVNContextMenuEntries::Settings, IDS_MENUSETTINGS, IDI_SETTINGS},
    {TSVNContextMenuEntries::Help, IDS_MENUHELP, IDI_HELP},
    {TSVNContextMenuEntries::About, IDS_MENUABOUT, IDI_ABOUT}};

/**
 * \ingroup TortoiseShell
 * Since we need an own COM-object for every different
 * Icon-Overlay implemented this enum defines which class
 * is used.
 */
enum FileState
{
    FileStateUncontrolled,
    FileStateVersioned,
    FileStateModified,
    FileStateConflict,
    FileStateDeleted,
    FileStateReadOnly,
    FileStateLockedOverlay,
    FileStateAddedOverlay,
    FileStateIgnoredOverlay,
    FileStateUnversionedOverlay,
    FileStateDropHandler,
    FileStateInvalid
};

#define ITEMIS_ONLYONE           0x00000001
#define ITEMIS_EXTENDED          0x00000002
#define ITEMIS_INSVN             0x00000004
#define ITEMIS_CONFLICTED        0x00000008
#define ITEMIS_FOLDER            0x00000010
#define ITEMIS_FOLDERINSVN       0x00000020
#define ITEMIS_NORMAL            0x00000040
#define ITEMIS_IGNORED           0x00000080
#define ITEMIS_INVERSIONEDFOLDER 0x00000100
#define ITEMIS_ADDED             0x00000200
#define ITEMIS_DELETED           0x00000400
#define ITEMIS_LOCKED            0x00000800
#define ITEMIS_PATCHFILE         0x00001000
#define ITEMIS_PROPMODIFIED      0x00002000
#define ITEMIS_NEEDSLOCK         0x00004000
#define ITEMIS_PATCHINCLIPBOARD  0x00008000
#define ITEMIS_PATHINCLIPBOARD   0x00010000
#define ITEMIS_TWO               0x00020000
#define ITEMIS_FILEEXTERNAL      0x00040000
#define ITEMIS_ADDEDWITHHISTORY  0x00080000
#define ITEMIS_UNSUPPORTEDFORMAT 0x00100000
#define ITEMIS_WCROOT            0x00200000
#define ITEMIS_HASDIFFLATER      0x00400000
