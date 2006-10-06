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
#include "registry.h"
#include "Globals.h"
#include "SVNAdminDir.h"

#define REGISTRYTIMEOUT 2000
#define EXCLUDELISTTIMEOUT 5000
#define ADMINDIRTIMEOUT 10000
#define DRIVETYPETIMEOUT 300000		// 5 min
#define NUMBERFMTTIMEOUT 300000
#define MENUTIMEOUT 100

typedef CComCritSecLock<CComCriticalSection> Locker;

/**
 * \ingroup TortoiseShell
 * Helper class which caches access to the registry. Also provides helper methods
 * for checks against the settings stored in the registry.
 */
class ShellCache
{
public:
	enum CacheType
	{
		none,
		exe,
		dll
	};
	ShellCache()
	{
		cachetype = CRegStdWORD(_T("Software\\TortoiseSVN\\CacheType"), exe);
		showrecursive = CRegStdWORD(_T("Software\\TortoiseSVN\\RecursiveOverlay"), TRUE);
		folderoverlay = CRegStdWORD(_T("Software\\TortoiseSVN\\FolderOverlay"), TRUE);
		driveremote = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskRemote"));
		drivefixed = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskFixed"), TRUE);
		drivecdrom = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskCDROM"));
		driveremove = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskRemovable"));
		drivefloppy = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskFloppy"));
		driveram = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskRAM"));
		driveunknown = CRegStdWORD(_T("Software\\TortoiseSVN\\DriveMaskUnknown"));
		excludelist = CRegStdString(_T("Software\\TortoiseSVN\\OverlayExcludeList"));
		includelist = CRegStdString(_T("Software\\TortoiseSVN\\OverlayIncludeList"));
		simplecontext = CRegStdWORD(_T("Software\\TortoiseSVN\\SimpleContext"), TRUE);
		unversionedasmodified = CRegStdWORD(_T("Software\\TortoiseSVN\\UnversionedAsModified"), FALSE);
		getlocktop = CRegStdWORD(_T("Software\\TortoiseSVN\\GetLockTop"), TRUE);
		cachetypeticker = GetTickCount();
		recursiveticker = cachetypeticker;
		folderoverlayticker = cachetypeticker;
		driveticker = cachetypeticker;
		drivetypeticker = cachetypeticker;
		langticker = cachetypeticker;
		columnrevformatticker = cachetypeticker;
		excludelistticker = cachetypeticker;
		includelistticker = cachetypeticker;
		simplecontextticker = cachetypeticker;
		unversionedasmodifiedticker = cachetypeticker;
		admindirticker = cachetypeticker;
		columnseverywhereticker = cachetypeticker;
		getlocktopticker = cachetypeticker;
		menulayout = CRegStdWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
		langid = CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
		blockstatus = CRegStdWORD(_T("Software\\TortoiseSVN\\BlockStatus"), 0);
		columnseverywhere = CRegStdWORD(_T("Software\\TortoiseSVN\\ColumnsEveryWhere"), FALSE);
		for (int i=0; i<27; i++)
		{
			drivetypecache[i] = (UINT)-1;
		}
		TCHAR szBuffer[5];
		columnrevformatticker = GetTickCount();
		ZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], sizeof(szDecSep));
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], sizeof(szThousandsSep));
		columnrevformat.lpDecimalSep = szDecSep;
		columnrevformat.lpThousandSep = szThousandsSep;
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], sizeof(szBuffer));
		columnrevformat.Grouping = _ttoi(szBuffer);
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], sizeof(szBuffer));
		columnrevformat.NegativeOrder = _ttoi(szBuffer);
		sAdminDirCacheKey.reserve(MAX_PATH);		// MAX_PATH as buffer reservation ok.
		m_critSec.Init();
	}
	void ForceRefresh()
	{
		cachetype.read();
		showrecursive.read();
		folderoverlay.read();
		driveremote.read();
		drivefixed.read();
		drivecdrom.read();
		driveremove.read();
		drivefloppy.read();
		driveram.read();
		driveunknown.read();
		excludelist.read();
		includelist.read();
		simplecontext.read();
		unversionedasmodified.read();
		menulayout.read();
		langid.read();
		blockstatus.read();
		columnseverywhere.read();
		getlocktop.read();
	}
	CacheType GetCacheType()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > cachetypeticker)
		{
			cachetypeticker = GetTickCount();
			cachetype.read();
		}
		return CacheType(DWORD((cachetype)));
	}
	DWORD BlockStatus()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > blockstatusticker)
		{
			blockstatusticker = GetTickCount();
			blockstatus.read();
		}
		return (blockstatus);
	}
	DWORD GetMenuLayout()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > layoutticker)
		{
			layoutticker = GetTickCount();
			menulayout.read();
		}
		return (menulayout);
	}
	BOOL IsRecursive()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>recursiveticker)
		{
			recursiveticker = GetTickCount();
			showrecursive.read();
		}
		return (showrecursive);
	}
	BOOL IsFolderOverlay()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>folderoverlayticker)
		{
			folderoverlayticker = GetTickCount();
			folderoverlay.read();
		}
		return (folderoverlay);
	}
	BOOL IsSimpleContext()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>simplecontextticker)
		{
			simplecontextticker = GetTickCount();
			simplecontext.read();
		}
		return (simplecontext==0);
	}
	BOOL IsUnversionedAsModified()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>unversionedasmodifiedticker)
		{
			unversionedasmodifiedticker = GetTickCount();
			unversionedasmodified.read();
		}
		return (unversionedasmodified);
	}
	BOOL IsGetLockTop()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>getlocktopticker)
		{
			getlocktopticker = GetTickCount();
			getlocktop.read();
		}
		return (getlocktop);
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
		Locker lock(m_critSec);
		IncludeListValid();
		for (std::vector<stdstring>::iterator I = invector.begin(); I != invector.end(); ++I)
		{
			if (I->empty())
				continue;
			if (I->at(I->size()-1)=='*')
			{
				stdstring str = I->substr(0, I->size()-1);
				if (_tcsnicmp(str.c_str(), path, str.size())==0)
					return TRUE;
			}
			else if (_tcsicmp(I->c_str(), path)==0)
				return TRUE;
		}
		UINT drivetype = 0;
		int drivenumber = PathGetDriveNumber(path);
		if ((drivenumber >=0)&&(drivenumber < 25))
		{
			drivetype = drivetypecache[drivenumber];
			if ((drivetype == -1)||((GetTickCount() - DRIVETYPETIMEOUT)>drivetypeticker))
			{
				drivetypeticker = GetTickCount();
				TCHAR pathbuf[MAX_PATH+4];		// MAX_PATH ok here. PathStripToRoot works with partial paths too.
				_tcsncpy_s(pathbuf, MAX_PATH+4, path, MAX_PATH+3);
				PathStripToRoot(pathbuf);
				PathAddBackslash(pathbuf);
				ATLTRACE2(_T("GetDriveType for %s, Drive %d\n"), pathbuf, drivenumber);
				drivetype = GetDriveType(pathbuf);
				drivetypecache[drivenumber] = drivetype;
			}
		}
		else
		{
			TCHAR pathbuf[MAX_PATH+4];		// MAX_PATH ok here. PathIsUNCServer works with partial paths too.
			_tcsncpy_s(pathbuf, MAX_PATH+4, path, MAX_PATH+3);
			if (PathIsUNCServer(pathbuf))
				drivetype = DRIVE_REMOTE;
			else
			{
				PathStripToRoot(pathbuf);
				PathAddBackslash(pathbuf);
				if (_tcsncmp(pathbuf, drivetypepathcache, MAX_PATH-1)==0)		// MAX_PATH ok.
					drivetype = drivetypecache[26];
				else
				{
					ATLTRACE2(_T("GetDriveType for %s\n"), pathbuf);
					drivetype = GetDriveType(pathbuf);
					drivetypecache[26] = drivetype;
					_tcsncpy_s(drivetypepathcache, MAX_PATH, pathbuf, MAX_PATH);			// MAX_PATH ok.
				} 
			}
		}
		if ((drivetype == DRIVE_REMOVABLE)&&(!IsRemovable()))
			return FALSE;
		if ((drivetype == DRIVE_REMOVABLE)&&(drivefloppy == 0)&&((drivenumber==0)||(drivenumber==1)))
			return FALSE;
		if ((drivetype == DRIVE_FIXED)&&(!IsFixed()))
			return FALSE;
		if (((drivetype == DRIVE_REMOTE)||(drivetype == DRIVE_NO_ROOT_DIR))&&(!IsRemote()))
			return FALSE;
		if ((drivetype == DRIVE_CDROM)&&(!IsCDRom()))
			return FALSE;
		if ((drivetype == DRIVE_RAMDISK)&&(!IsRAM()))
			return FALSE;
		if ((drivetype == DRIVE_UNKNOWN)&&(IsUnknown()))
			return FALSE;

		ExcludeListValid();
		for (std::vector<stdstring>::iterator I = exvector.begin(); I != exvector.end(); ++I)
		{
			if (I->empty())
				continue;
			if (I->at(I->size()-1)=='*')
			{
				stdstring str = I->substr(0, I->size()-1);
				if (_tcsnicmp(str.c_str(), path, str.size())==0)
					return FALSE;
			}
			else if (_tcsicmp(I->c_str(), path)==0)
				return FALSE;
		}
		return TRUE;
	}
	DWORD GetLangID()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > langticker)
		{
			langticker = GetTickCount();
			langid.read();
		}
		return (langid);
	}
	NUMBERFMT * GetNumberFmt()
	{
		if ((GetTickCount() - NUMBERFMTTIMEOUT) > columnrevformatticker)
		{
			TCHAR szBuffer[5];
			columnrevformatticker = GetTickCount();
			ZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], sizeof(szDecSep));
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], sizeof(szThousandsSep));
			columnrevformat.lpDecimalSep = szDecSep;
			columnrevformat.lpThousandSep = szThousandsSep;
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], sizeof(szBuffer));
			columnrevformat.Grouping = _ttoi(szBuffer);
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], sizeof(szBuffer));
			columnrevformat.NegativeOrder = _ttoi(szBuffer);
		}
		return &columnrevformat;
	}
	BOOL HasSVNAdminDir(LPCTSTR path, BOOL bIsDir)
	{
		size_t len = _tcslen(path);
		TCHAR * buf = new TCHAR[len+1];
		_tcscpy_s(buf, len+1, path);
		if (! bIsDir)
		{
			TCHAR * ptr = _tcsrchr(buf, '\\');
			if (ptr != 0)
			{
				*ptr = 0;
			}
		}
		if ((GetTickCount() - ADMINDIRTIMEOUT) < admindirticker)
		{
			std::map<stdstring, BOOL>::iterator iter;
			sAdminDirCacheKey.assign(buf);
			if ((iter = admindircache.find(sAdminDirCacheKey)) != admindircache.end())
			{
				delete buf;
				return iter->second;
			}
		}
		BOOL hasAdminDir = g_SVNAdminDir.HasAdminDir(buf, true);
		admindirticker = GetTickCount();
		Locker lock(m_critSec);
		admindircache[buf] = hasAdminDir;
		delete buf;
		return hasAdminDir;
	}
	bool IsColumnsEveryWhere()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > columnseverywhereticker)
		{
			columnseverywhereticker = GetTickCount();
			columnseverywhere.read();
		} 
		return !!(DWORD)columnseverywhere;
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
			drivefloppy.read();
		}
	}
	void ExcludeListValid()
	{
		if ((GetTickCount() - EXCLUDELISTTIMEOUT)>excludelistticker)
		{
			Locker lock(m_critSec);
			excludelistticker = GetTickCount();
			excludelist.read();
			if (excludeliststr.compare((stdstring)excludelist)==0)
				return;
			excludeliststr = (stdstring)excludelist;
			exvector.clear();
			size_t pos = 0, pos_ant = 0;
			pos = excludeliststr.find(_T("\n"), pos_ant);
			while (pos != stdstring::npos)
			{
				stdstring token = excludeliststr.substr(pos_ant, pos-pos_ant);
				exvector.push_back(token);
				pos_ant = pos+1;
				pos = excludeliststr.find(_T("\n"), pos_ant);
			}
			if (!excludeliststr.empty())
			{
				exvector.push_back(excludeliststr.substr(pos_ant, excludeliststr.size()-1));
			}
			excludeliststr = (stdstring)excludelist;
		}
	}
	void IncludeListValid()
	{
		if ((GetTickCount() - EXCLUDELISTTIMEOUT)>includelistticker)
		{
			Locker lock(m_critSec);
			includelistticker = GetTickCount();
			includelist.read();
			if (includeliststr.compare((stdstring)includelist)==0)
				return;
			includeliststr = (stdstring)includelist;
			invector.clear();
			size_t pos = 0, pos_ant = 0;
			pos = includeliststr.find(_T("\n"), pos_ant);
			while (pos != stdstring::npos)
			{
				stdstring token = includeliststr.substr(pos_ant, pos-pos_ant);
				invector.push_back(token);
				pos_ant = pos+1;
				pos = includeliststr.find(_T("\n"), pos_ant);
			}
			if (!includeliststr.empty())
			{
				invector.push_back(includeliststr.substr(pos_ant, includeliststr.size()-1));
			}
			includeliststr = (stdstring)includelist;
		}
	}
	CRegStdWORD cachetype;
	CRegStdWORD blockstatus;
	CRegStdWORD langid;
	CRegStdWORD showrecursive;
	CRegStdWORD folderoverlay;
	CRegStdWORD getlocktop;
	CRegStdWORD driveremote;
	CRegStdWORD drivefixed;
	CRegStdWORD drivecdrom;
	CRegStdWORD driveremove;
	CRegStdWORD drivefloppy;
	CRegStdWORD driveram;
	CRegStdWORD driveunknown;
	CRegStdWORD menulayout;
	CRegStdWORD simplecontext;
	CRegStdWORD unversionedasmodified;
	CRegStdString excludelist;
	CRegStdWORD columnseverywhere;
	stdstring excludeliststr;
	std::vector<stdstring> exvector;
	CRegStdString includelist;
	stdstring includeliststr;
	std::vector<stdstring> invector;
	DWORD cachetypeticker;
	DWORD recursiveticker;
	DWORD folderoverlayticker;
	DWORD getlocktopticker;
	DWORD driveticker;
	DWORD drivetypeticker;
	DWORD layoutticker;
	DWORD langticker;
	DWORD blockstatusticker;
	DWORD columnrevformatticker;
	DWORD excludelistticker;
	DWORD includelistticker;
	DWORD simplecontextticker;
	DWORD unversionedasmodifiedticker;
	DWORD columnseverywhereticker;
	UINT  drivetypecache[27];
	TCHAR drivetypepathcache[MAX_PATH];		// MAX_PATH ok.
	NUMBERFMT columnrevformat;
	TCHAR szDecSep[5];
	TCHAR szThousandsSep[5];
	std::map<stdstring, BOOL> admindircache;
	stdstring sAdminDirCacheKey;
	DWORD admindirticker;
	CComCriticalSection m_critSec;
};
