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

#define MENUCHECKOUT		0x00000001
#define MENUUPDATE			0x00000002
#define MENUCOMMIT			0x00000004
#define MENUADD				0x00000008
#define MENUREVERT			0x00000010
#define MENUCLEANUP			0x00000020
#define MENURESOLVE			0x00000040
#define MENUSWITCH			0x00000080
#define MENUIMPORT			0x00000100
#define MENUEXPORT			0x00000200
#define MENUCREATEREPOS		0x00000400
#define MENUCOPY			0x00000800
#define MENUMERGE			0x00001000
#define MENUREMOVE			0x00002000
#define MENURENAME			0x00004000
#define MENUUPDATEEXT		0x00008000
#define MENUDIFF			0x00010000
#define MENULOG				0x00020000
#define MENUCONFLICTEDITOR	0x00040000
#define MENURELOCATE		0x00080000
#define MENUSHOWCHANGED		0x00100000
#define MENUIGNORE			0x00200000
#define MENUREPOBROWSE		0x00400000
#define MENUBLAME			0x00800000
#define MENUCREATEPATCH		0x01000000
#define MENUAPPLYPATCH		0x02000000
#define MENUREVISIONGRAPH	0x04000000
#define MENULOCK			0x08000000
#define MENUUNLOCK			0x10000000

/**
 * Since we need an own COM-object for every different
 * Icon-Overlay implemented this enum defines which class
 * is used.
 */
enum FileState
{
    Uncontrolled,
    Versioned,
    Modified,
    Conflict,
	Deleted,
	ReadOnly,
	LockedOverlay,
	AddedOverlay,
	DropHandler,
	Invalid
};


