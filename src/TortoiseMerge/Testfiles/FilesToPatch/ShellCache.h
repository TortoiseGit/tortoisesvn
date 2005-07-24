// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "atltrace.h"
#include "globals.h"
#include <tchar.h>
#include <string>
#include "registry.h"

#define REGISTRYTIMEOUT 2000
#define DRIVETYPETIMEOUT 300000		// 5 min
class ShellCache
{
public:
	ShellCache()
	{
		showrecursive = CRegStdWORD(_T("Software\\TortoiseSVN\\RecursiveOverlay"));
		driveremote = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskRemote"));
		drivefixed = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskFixed"));
		drivecdrom = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskCDROM"));
		driveremove = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskRemovable"));
		driveram = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskRAM"));
		driveunknown = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskUnknown"));
		recursiveticker = GetTickCount();
		driveticker = recursiveticker;
		drivetypeticker = recursiveticker;
		langticker = recursiveticker;
		menulayout = CRegStdWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
		langid = CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
		blockstatus = CRegStdWORD(_T("Software\\TortoiseSVN\\BlockStatus"), 0);
		for (int i=0; i<27; i++)
		{
			drivetypecache[i] = -1;
		}
	}
	DWORD BlockStatus()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > blockstatusticker)
		{
			blockstatusticker = GetTickCount();
			blockstatus.read();
		} // if ((GetTickCount() - REGISTRYTIMEOUT) > blockstatusticker)
		return (blockstatus);
	}
	DWORD GetMenuLayout()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > layoutticker)
		{
			layoutticker = GetTickCount();
			menulayout.read();
		} // if ((GetTickCount() - REGISTRYTIMEOUT) > layoutticker)
		return (menulayout);
	}
	BOOL IsRecursive()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>recursiveticker)
		{
			recursiveticker = GetTickCount();
			showrecursive.read();
		} // if ((GetTickCount() - REGISTRYTIMEOUT)>recursiveticker)
		return (showrecursive);
	}
	BOOL IsRemote()
	{
		DriveValid();
		return (driveremote);
	}
	BOOL IsFixed()
	{
		DriveValid();
		return (drivefixed);
	}
	BOOL IsCDRom()
	{
		DriveValid();
		return (drivecdrom);
	}
	BOOL IsRemovable()
	{
		DriveValid();
		return (driveremove);
	}
	BOOL IsRAM()
	{
		DriveValid();
		return (driveram);
	}
	BOOL IsUnknown()
	{
		DriveValid();
		return (driveunknown);
	}
	BOOL IsPathAllowed(LPCTSTR path)
	{
		UINT drivetype = 0;
		int drivenumber = PathGetDriveNumber(path);
		if ((drivenumber >=0)&&(drivenumber < 25))
		{
			drivetype = drivetypecache[drivenumber];
			if ((drivetype == -1)||((GetTickCount() - DRIVETYPETIMEOUT)>drivetypeticker))
			{
				drivetypeticker = GetTickCount();
				TCHAR pathbuf[MAX_PATH+4];
				_tcscpy(pathbuf, path);
				PathRemoveFileSpec(pathbuf);
				PathAddBackslash(pathbuf);
				ATLTRACE2(_T("GetDriveType for %s, Drive %d\n"), pathbuf, drivenumber);
				drivetype = GetDriveType(pathbuf);
				drivetypecache[drivenumber] = drivetype;
			} // if (drivetype == -1)
		} // if ((drivenumber >=0)&&(drivenumber < 25)) 
		else
		{
			TCHAR pathbuf[MAX_PATH+4];
			_tcscpy(pathbuf, path);
			PathRemoveFileSpec(pathbuf);
			PathAddBackslash(pathbuf);
			if (_tcsncmp(pathbuf, drivetypepathcache, MAX_PATH-1)==0)
				drivetype = drivetypecache[26];
			else
			{
				ATLTRACE2(_T("GetDriveType for %s\n"), pathbuf);
				drivetype = GetDriveType(pathbuf);
				drivetypecache[26] = drivetype;
				_tcsncpy(drivetypepathcache, pathbuf, MAX_PATH);
			} 
		}
		if ((drivetype == DRIVE_REMOVABLE)&&(!IsRemovable()))
			return FALSE;
		if ((drivetype == DRIVE_FIXED)&&(!IsFixed()))
			return FALSE;
		if ((drivetype == DRIVE_REMOTE)&&(!IsRemote()))
			return FALSE;
		if ((drivetype == DRIVE_CDROM)&&(!IsCDRom()))
			return FALSE;
		if ((drivetype == DRIVE_RAMDISK)&&(!IsRAM()))
			return FALSE;
		if ((drivetype == DRIVE_UNKNOWN)&&(IsUnknown()))
			return FALSE;
		return TRUE;
	}
	DWORD GetLangID()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > langticker)
		{
			langticker = GetTickCount();
			langid.read();
		} // if ((GetTickCount() - REGISTRYTIMEOUT) > layoutticker)
		return (langid);
	}

private:
	void DriveValid()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>driveticker)
		{
			driveticker = GetTickCount();
			driveremote.read();
			drivefixed.read();
			drivecdrom.read();
			driveremove.read();
		}
	}
	CRegStdWORD blockstatus;
	CRegStdWORD langid;
	CRegStdWORD showrecursive;
	CRegStdWORD driveremote;
	CRegStdWORD drivefixed;
	CRegStdWORD drivecdrom;
	CRegStdWORD driveremove;
	CRegStdWORD driveram;
	CRegStdWORD driveunknown;
	CRegStdWORD menulayout;
	DWORD recursiveticker;
	DWORD driveticker;
	DWORD drivetypeticker;
	DWORD layoutticker;
	DWORD langticker;
	DWORD blockstatusticker;
	UINT  drivetypecache[27];
	TCHAR drivetypepathcache[MAX_PATH];
};