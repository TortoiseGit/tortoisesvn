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

#pragma once
#include "stdafx.h"
#include <windows.h>
#include "registry.h"
#include "SVNStatus.h"
#include "SVNFolderStatus.h"
#include "ShellCache.h"

extern	UINT				g_cRefThisDll;			// Reference count of this DLL.
extern	HINSTANCE			g_hmodThisDll;			// Instance handle for this DLL
extern	SVNFolderStatus		g_CachedStatus;			// status cache
extern	ShellCache			g_ShellCache;			// caching of registry entries, ...
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
	Added,
	DropHandler,
	Invalid
};

