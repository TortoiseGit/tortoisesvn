line   rev    date                           path                                                         author                         content 

     0   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // TortoiseSVN - a Windows shell extension for easy version control
     1   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
     2   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng
     3   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
     4   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // This program is free software; you can redistribute it and/or
     5   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // modify it under the terms of the GNU General Public License
     6   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // as published by the Free Software Foundation; either version 2
     7   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // of the License, or (at your option) any later version.
     8   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
     9   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // This program is distributed in the hope that it will be useful,
    10   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // but WITHOUT ANY WARRANTY; without even the implied warranty of
    11   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    12   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // GNU General Public License for more details.
    13   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
    14   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // You should have received a copy of the GNU General Public License
    15   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // along with this program; if not, write to the Free Software
    16   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      // Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
    17   5310 31.12.2005 16:30:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      //
    18   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          #include "StdAfx.h"
    19   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          #include ".\cacheddirectory.h"
    20   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          #include "SVNHelpers.h"
    21   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          #include "SVNStatusCache.h"
    22   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          #include "SVNStatus.h"
    23   4877 05.11.2005 10:01:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      #include <set>
    24   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
    25   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CCachedDirectory::CCachedDirectory(void)
    26   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
    27   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	m_entriesFileTime = 0;
    28   4309 03.09.2005 10:27:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	m_propsFileTime = 0;
    29   3275 08.05.2005 11:59:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
    30   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
    31   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
    32   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CCachedDirectory::~CCachedDirectory(void)
    33   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
    34   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
    35   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
    36   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CCachedDirectory::CCachedDirectory(const CTSVNPath& directoryPath)
    37   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
    38   2561 27.01.2005 23:03:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	ATLASSERT(directoryPath.IsDirectory() || !PathFileExists(directoryPath.GetWinPath()));
    39   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
    40   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	m_directoryPath = directoryPath;
    41   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	m_entriesFileTime = 0;
    42   4309 03.09.2005 10:27:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	m_propsFileTime = 0;
    43   3275 08.05.2005 11:59:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
    44   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
    45   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
    46   3817 01.07.2005 19:27:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      BOOL CCachedDirectory::SaveToDisk(FILE * pFile)
    47   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      {
    48   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	AutoLocker lock(m_critSec);
    49   3818 02.07.2005 14:24:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      #define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;
    50   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
    51   4953 15.11.2005 19:24:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	unsigned int value = 1;
    52   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(value);	// 'version' of this save-format
    53   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	value = (int)m_entryCache.size();
    54   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(value);	// size of the cache map
    55   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	// now iterate through the maps and save every entry.
    56   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	for (CacheEntryMap::iterator I = m_entryCache.begin(); I != m_entryCache.end(); ++I)
    57   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
    58   3762 22.06.2005 17:32:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		const CString& key = I->first;
    59   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		value = key.GetLength();
    60   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		WRITEVALUETOFILE(value);
    61   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (value)
    62   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
    63   6343 20.04.2006 20:00:05            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (fwrite((LPCTSTR)key, sizeof(TCHAR), value, pFile)!=value)
    64   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return false;
    65   3817 01.07.2005 19:27:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (!I->second.SaveToDisk(pFile))
    66   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return false;
    67   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
    68   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
    69   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	value = (int)m_childDirectories.size();
    70   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(value);
    71   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	for (ChildDirStatus::iterator I = m_childDirectories.begin(); I != m_childDirectories.end(); ++I)
    72   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
    73   3762 22.06.2005 17:32:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		const CString& path = I->first.GetWinPathString();
    74   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		value = path.GetLength();
    75   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		WRITEVALUETOFILE(value);
    76   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (value)
    77   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
    78   6343 20.04.2006 20:00:05            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (fwrite((LPCTSTR)path, sizeof(TCHAR), value, pFile)!=value)
    79   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return false;
    80   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			svn_wc_status_kind status = I->second;
    81   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			WRITEVALUETOFILE(status);
    82   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
    83   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
    84   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(m_entriesFileTime);
    85   4309 03.09.2005 10:27:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(m_propsFileTime);
    86   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	value = m_directoryPath.GetWinPathString().GetLength();
    87   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(value);
    88   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if (value)
    89   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
    90   6343 20.04.2006 20:00:05            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (fwrite(m_directoryPath.GetWinPath(), sizeof(TCHAR), value, pFile)!=value)
    91   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			return false;
    92   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
    93   3817 01.07.2005 19:27:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if (!m_ownStatus.SaveToDisk(pFile))
    94   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		return false;
    95   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(m_currentFullStatus);
    96   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	WRITEVALUETOFILE(m_mostImportantFileStatus);
    97   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	return true;
    98   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      }
    99   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   100   3817 01.07.2005 19:27:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      BOOL CCachedDirectory::LoadFromDisk(FILE * pFile)
   101   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      {
   102   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	AutoLocker lock(m_critSec);
   103   3818 02.07.2005 14:24:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      #define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) return false;
   104   9674 05.06.2007 21:36:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            schaefer                       	try
   105   9674 05.06.2007 21:36:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            schaefer                       	{
   106   3835 06.07.2005 13:56:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		unsigned int value = 0;
   107   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(value);
   108   4953 15.11.2005 19:24:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (value != 1)
   109   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			return false;		// not the correct version
   110   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		int mapsize = 0;
   111   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(mapsize);
   112   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		for (int i=0; i<mapsize; ++i)
   113   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   114   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			LOADVALUEFROMFILE(value);
   115   4973 18.11.2005 22:14:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (value > MAX_PATH)
   116   4973 18.11.2005 22:14:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return false;
   117   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (value)
   118   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   119   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				CString sKey;
   120   7089 22.07.2006 08:56:52            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (fread(sKey.GetBuffer(value+1), sizeof(TCHAR), value, pFile)!=value)
   121   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   122   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					sKey.ReleaseBuffer(0);
   123   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					return false;
   124   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				}
   125   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				sKey.ReleaseBuffer(value);
   126   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				CStatusCacheEntry entry;
   127   3817 01.07.2005 19:27:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (!entry.LoadFromDisk(pFile))
   128   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					return false;
   129   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_entryCache[sKey] = entry;
   130   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   131   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   132   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(mapsize);
   133   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		for (int i=0; i<mapsize; ++i)
   134   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   135   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			LOADVALUEFROMFILE(value);
   136   4973 18.11.2005 22:14:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (value > MAX_PATH)
   137   4973 18.11.2005 22:14:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return false;
   138   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (value)
   139   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   140   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				CString sPath;
   141   3818 02.07.2005 14:24:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (fread(sPath.GetBuffer(value), sizeof(TCHAR), value, pFile)!=value)
   142   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   143   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					sPath.ReleaseBuffer(0);
   144   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					return false;
   145   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				}
   146   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				sPath.ReleaseBuffer(value);
   147   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				svn_wc_status_kind status;
   148   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				LOADVALUEFROMFILE(status);
   149   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_childDirectories[CTSVNPath(sPath)] = status;
   150   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   151   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   152   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(m_entriesFileTime);
   153   4309 03.09.2005 10:27:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(m_propsFileTime);
   154   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(value);
   155   4973 18.11.2005 22:14:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (value > MAX_PATH)
   156   4973 18.11.2005 22:14:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			return false;
   157   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (value)
   158   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   159   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			CString sPath;
   160   7089 22.07.2006 08:56:52            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (fread(sPath.GetBuffer(value+1), sizeof(TCHAR), value, pFile)!=value)
   161   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   162   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				sPath.ReleaseBuffer(0);
   163   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return false;
   164   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   165   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			sPath.ReleaseBuffer(value);
   166   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			m_directoryPath.SetFromWin(sPath);
   167   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   168   3817 01.07.2005 19:27:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (!m_ownStatus.LoadFromDisk(pFile))
   169   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			return false;
   170   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   171   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(m_currentFullStatus);
   172   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		LOADVALUEFROMFILE(m_mostImportantFileStatus);
   173   9674 05.06.2007 21:36:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            schaefer                       	}
   174   9674 05.06.2007 21:36:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            schaefer                       	catch ( CAtlException )
   175   9674 05.06.2007 21:36:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            schaefer                       	{
   176   9674 05.06.2007 21:36:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            schaefer                       		return false;
   177   9674 05.06.2007 21:36:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            schaefer                       	}
   178   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	return true;
   179   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   180   2745 27.02.2005 11:48:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      }
   181   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   182   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      CStatusCacheEntry CCachedDirectory::GetStatusForMember(const CTSVNPath& path, bool bRecursive,  bool bFetch /* = true */)
   183   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   184   2563 27.01.2005 23:52:19            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	CString strCacheKey;
   185   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	bool bThisDirectoryIsUnversioned = false;
   186   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	bool bRequestForSelf = false;
   187   8157 29.11.2006 16:46:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      	if(path.IsEquivalentToWithoutCase(m_directoryPath))
   188   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   189   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		bRequestForSelf = true;
   190   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   191   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   192   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// In all most circumstances, we ask for the status of a member of this directory.
   193   8157 29.11.2006 16:46:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      	ATLASSERT(m_directoryPath.IsEquivalentToWithoutCase(path.GetContainingDirectory()) || bRequestForSelf);
   194   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   195   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// Check if the entries file has been changed
   196   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	CTSVNPath entriesFilePath(m_directoryPath);
   197   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	CTSVNPath propsDirPath(m_directoryPath);
   198   4517 29.09.2005 21:07:47            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if (g_SVNAdminDir.IsVSNETHackActive())
   199   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   200   4664 16.10.2005 17:26:29            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		entriesFilePath.AppendPathString(g_SVNAdminDir.GetVSNETAdminDirName() + _T("\\entries"));
   201   4664 16.10.2005 17:26:29            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		propsDirPath.AppendPathString(g_SVNAdminDir.GetVSNETAdminDirName() + _T("\\dir-props"));
   202   4517 29.09.2005 21:07:47            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   203   4664 16.10.2005 17:26:29            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	else
   204   4517 29.09.2005 21:07:47            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
   205   4664 16.10.2005 17:26:29            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		entriesFilePath.AppendPathString(g_SVNAdminDir.GetAdminDirName() + _T("\\entries"));
   206   4664 16.10.2005 17:26:29            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		propsDirPath.AppendPathString(g_SVNAdminDir.GetAdminDirName() + _T("\\dir-props"));
   207   4664 16.10.2005 17:26:29            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   208  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      	if ( (m_entriesFileTime == entriesFilePath.GetLastWriteTime()) && ((entriesFilePath.GetLastWriteTime() == 0) || (m_propsFileTime == propsDirPath.GetLastWriteTime())) )
   209   4664 16.10.2005 17:26:29            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
   210  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		m_entriesFileTime = entriesFilePath.GetLastWriteTime();
   211  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		if (m_entriesFileTime)
   212  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			m_propsFileTime = propsDirPath.GetLastWriteTime();
   213  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      
   214   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		if(m_entriesFileTime == 0)
   215   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   216   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// We are a folder which is not in a working copy
   217   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			bThisDirectoryIsUnversioned = true;
   218   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			m_ownStatus.SetStatus(NULL);
   219   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   220   5668 09.02.2006 17:58:36            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			// If a user removes the .svn directory, we get here with m_entryCache
   221   5668 09.02.2006 17:58:36            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			// not being empty, but still us being unversioned
   222   5668 09.02.2006 17:58:36            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (!m_entryCache.empty())
   223   5668 09.02.2006 17:58:36            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   224   5668 09.02.2006 17:58:36            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_entryCache.clear();
   225   5668 09.02.2006 17:58:36            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   226   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			ATLASSERT(m_entryCache.empty());
   227   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			
   228   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// However, a member *DIRECTORY* might be the top of WC
   229   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// so we need to ask them to get their own status
   230   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			if(!path.IsDirectory())
   231   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			{
   232   2790 07.03.2005 17:40:16            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if ((PathFileExists(path.GetWinPath()))||(bRequestForSelf))
   233   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					return CStatusCacheEntry();
   234   3322 12.05.2005 17:44:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// the entry doesn't exist anymore! 
   235   5889 02.03.2006 16:55:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// but we can't remove it from the cache here:
   236   5889 02.03.2006 16:55:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// the GetStatusForMember() method is called only with a read
   237   5889 02.03.2006 16:55:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// lock and not a write lock!
   238   5889 02.03.2006 16:55:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// So mark it for crawling, and let the crawler remove it
   239   5889 02.03.2006 16:55:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// later
   240  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				CSVNStatusCache::Instance().AddFolderForCrawling(path.GetContainingDirectory());
   241   2829 14.03.2005 18:49:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return CStatusCacheEntry();
   242   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			}
   243   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			else
   244   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			{
   245   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// If we're in the special case of a directory being asked for its own status
   246   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// and this directory is unversioned, then we should just return that here
   247   2790 07.03.2005 17:40:16            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if(bRequestForSelf)
   248   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				{
   249   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					return CStatusCacheEntry();
   250   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   251   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			}
   252   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   253   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   254   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		if(path.IsDirectory())
   255   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   256   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// We don't have directory status in our cache
   257   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// Ask the directory if it knows its own status
   258   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			CCachedDirectory * dirEntry = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
   259   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if ((dirEntry)&&(dirEntry->IsOwnStatusValid()))
   260   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			{
   261   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// This directory knows its own status
   262   3111 23.04.2005 10:44:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// but is it still versioned?
   263   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   264   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				// CTSVNPath has a default copy constructor, but we *don't* want to use
   265   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				// that one here, because some internal data (here: the m_bHasAdminDirKnown flag)
   266   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				// must be not used. It could have been changed and must be checked again.
   267   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				// That's why we use the CString constructor, which resets all those flags.
   268   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				CTSVNPath svnFilePath(dirEntry->m_directoryPath.GetWinPathString());
   269   3111 23.04.2005 10:44:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (!svnFilePath.HasAdminDir())
   270   3111 23.04.2005 10:44:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   271   3322 12.05.2005 17:44:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					CSVNStatusCache::Instance().AddFolderForCrawling(svnFilePath);
   272   3111 23.04.2005 10:44:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					return CStatusCacheEntry();
   273   3111 23.04.2005 10:44:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				}
   274   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   275   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// To keep recursive status up to date, we'll request that children are all crawled again
   276   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// This will be very quick if nothing's changed, because it will all be cache hits
   277   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (bRecursive)
   278   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   279  10469 31.08.2007 19:21:04            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					AutoLocker lock(dirEntry->m_critSec);
   280   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					ChildDirStatus::const_iterator it;
   281   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					for(it = dirEntry->m_childDirectories.begin(); it != dirEntry->m_childDirectories.end(); ++it)
   282   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					{
   283   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          						CSVNStatusCache::Instance().AddFolderForCrawling(it->first);
   284   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					}
   285   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				}
   286   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   287   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return dirEntry->GetOwnStatus(bRecursive);
   288   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			}
   289   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   290   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		else
   291   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   292   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   293   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// if we currently are fetching the status of the directory
   294   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// we want the status for, we just return an empty entry here
   295   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// and don't wait for that fetching to finish.
   296   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// That's because fetching the status can take a *really* long
   297   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// time (e.g. if a commit is also in progress on that same
   298   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// directory), and we don't want to make the explorer appear
   299   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// to hang.
   300   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				AutoLocker pathlock(m_critSecPath);
   301   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if ((!bFetch)&&(!m_currentStatusFetchingPath.IsEmpty()))
   302   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   303   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					if ((m_currentStatusFetchingPath.IsAncestorOf(path))&&((m_currentStatusFetchingPathTicks + 1000)<GetTickCount()))
   304   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					{
   305   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      						ATLTRACE("returning empty status (status fetch in progress) for %ws\n", path.GetWinPath());
   306   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      						m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
   307   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      						return CStatusCacheEntry();
   308   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					}
   309   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				}
   310   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   311   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// Look up a file in our own cache
   312   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			AutoLocker lock(m_critSec);
   313   2563 27.01.2005 23:52:19            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			strCacheKey = GetCacheKey(path);
   314   2563 27.01.2005 23:52:19            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
   315   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			if(itMap != m_entryCache.end())
   316   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			{
   317   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// We've hit the cache - check for timeout
   318   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				if(!itMap->second.HasExpired((long)GetTickCount()))
   319   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				{
   320   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					if(itMap->second.DoesFileTimeMatch(path.GetLastWriteTime()))
   321   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					{
   322   6027 15.03.2006 19:30:43            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      						if ((itMap->second.GetEffectiveStatus()!=svn_wc_status_missing)||(!PathFileExists(path.GetWinPath())))
   323   6027 15.03.2006 19:30:43            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      						{
   324   3110 23.04.2005 09:51:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      							// Note: the filetime matches after a modified has been committed too.
   325   3110 23.04.2005 09:51:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      							// So in that case, we would return a wrong status (e.g. 'modified' instead
   326   3110 23.04.2005 09:51:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      							// of 'normal') here.
   327   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          							return itMap->second;
   328   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          						}
   329   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					}
   330   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   331   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			}
   332   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   333   6027 15.03.2006 19:30:43            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   334   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	else
   335   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   336   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		AutoLocker pathlock(m_critSecPath);
   337   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if ((!bFetch)&&(!m_currentStatusFetchingPath.IsEmpty()))
   338   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   339   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if ((m_currentStatusFetchingPath.IsAncestorOf(path))&&((m_currentStatusFetchingPathTicks + 1000)<GetTickCount()))
   340   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   341   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				ATLTRACE("returning empty status (status fetch in progress) for %ws\n", path.GetWinPath());
   342   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
   343   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return CStatusCacheEntry();
   344   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   345   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   346   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// if we're fetching the status for the explorer,
   347   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// we don't refresh the status but use the one
   348   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// we already have (to save time and make the explorer
   349   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// more responsive in stress conditions).
   350   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// We leave the refreshing to the crawler.
   351   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if ((!bFetch)&&(m_entriesFileTime))
   352   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   353   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			CSVNStatusCache::Instance().AddFolderForCrawling(path.GetDirectory());
   354   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			return CStatusCacheEntry();
   355   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   356   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		AutoLocker lock(m_critSec);
   357   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		m_entriesFileTime = entriesFilePath.GetLastWriteTime();
   358   4309 03.09.2005 10:27:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		m_propsFileTime = propsDirPath.GetLastWriteTime();
   359   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		m_entryCache.clear();
   360   2624 05.02.2005 18:08:04            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		strCacheKey = GetCacheKey(path);
   361   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   362   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   363   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	svn_opt_revision_t revision;
   364   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	revision.kind = svn_opt_revision_unspecified;
   365   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   366   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// We've not got this item in the cache - let's add it
   367   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// We never bother asking SVN for the status of just one file, always for its containing directory
   368   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   369   4517 29.09.2005 21:07:47            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if (g_SVNAdminDir.IsAdminDirPath(path.GetWinPathString()))
   370   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   371   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		// We're being asked for the status of an .SVN directory
   372   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		// It's not worth asking for this
   373   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		return CStatusCacheEntry();
   374   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   375   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   376   3319 11.05.2005 22:57:24            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
   377   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		AutoLocker pathlock(m_critSecPath);
   378   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if ((!bFetch)&&(!m_currentStatusFetchingPath.IsEmpty()))
   379   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   380   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if ((m_currentStatusFetchingPath.IsAncestorOf(path))&&((m_currentStatusFetchingPathTicks + 1000)<GetTickCount()))
   381   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   382   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				ATLTRACE("returning empty status (status fetch in progress) for %ws\n", path.GetWinPath());
   383   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
   384   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				return CStatusCacheEntry();
   385   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   386   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   387   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		AutoLocker lock(m_critSec);
   388   7089 22.07.2006 08:56:52            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		SVNPool subPool(CSVNStatusCache::Instance().m_svnHelp.Pool());
   389   3275 08.05.2005 11:59:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		m_mostImportantFileStatus = svn_wc_status_none;
   390   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		m_childDirectories.clear();
   391   2834 15.03.2005 13:08:54            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		m_entryCache.clear();
   392   3111 23.04.2005 10:44:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		m_ownStatus.SetStatus(NULL);
   393   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		m_bRecursive = bRecursive;
   394   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		if(!bThisDirectoryIsUnversioned)
   395   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   396   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   397   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				AutoLocker pathlock(m_critSecPath);
   398   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_currentStatusFetchingPath = m_directoryPath;
   399   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_currentStatusFetchingPathTicks = GetTickCount();
   400   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   401   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			ATLTRACE(_T("svn_cli_stat for '%s' (req %s)\n"), m_directoryPath.GetWinPath(), path.GetWinPath());
   402   2983 06.04.2005 18:49:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			svn_error_t* pErr = svn_client_status2 (
   403   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				NULL,
   404   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				m_directoryPath.GetSVNApiPath(),
   405   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				&revision,
   406   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				GetStatusCallback,
   407   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				this,
   408   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				FALSE,
   409   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				TRUE,									//getall
   410   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				FALSE,
   411   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				TRUE,									//noignore
   412   2983 06.04.2005 18:49:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				FALSE,									//ignore externals
   413   7089 22.07.2006 08:56:52            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				CSVNStatusCache::Instance().m_svnHelp.ClientContext(),
   414   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				subPool
   415   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				);
   416   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   417   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				AutoLocker pathlock(m_critSecPath);
   418   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				m_currentStatusFetchingPath.Reset();
   419   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   420   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			ATLTRACE(_T("svn_cli_stat finished for '%s'\n"), m_directoryPath.GetWinPath(), path.GetWinPath());
   421   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			if(pErr)
   422   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			{
   423   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// Handle an error
   424   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// The most likely error on a folder is that it's not part of a WC
   425   2562 27.01.2005 23:38:04            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// In most circumstances, this will have been caught earlier,
   426   2562 27.01.2005 23:38:04            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// but in some situations, we'll get this error.
   427   2562 27.01.2005 23:38:04            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// If we allow ourselves to fall on through, then folders will be asked
   428   2562 27.01.2005 23:38:04            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// for their own status, and will set themselves as unversioned, for the 
   429   2562 27.01.2005 23:38:04            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// benefit of future requests
   430   2562 27.01.2005 23:38:04            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				ATLTRACE("svn_cli_stat err: '%s'\n", pErr->message);
   431   2830 14.03.2005 19:03:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				svn_error_clear(pErr);
   432   2649 10.02.2005 17:36:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// No assert here! Since we _can_ get here, an assertion is not an option!
   433   2649 10.02.2005 17:36:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// Reasons to get here: 
   434   2649 10.02.2005 17:36:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// - renaming a folder with many subfolders --> results in "not a working copy" if the revert
   435   2649 10.02.2005 17:36:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				//   happens between our checks and the svn_client_status() call.
   436   2649 10.02.2005 17:36:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// - reverting a move/copy --> results in "not a working copy" (as above)
   437   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (!m_directoryPath.HasAdminDir())
   438   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   439   3275 08.05.2005 11:59:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
   440   2929 27.03.2005 19:01:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					return CStatusCacheEntry();
   441   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   442   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				else
   443   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   444   7883 29.10.2006 12:13:03            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					ATLTRACE("svn_cli_stat error, assume none status\n");
   445   7883 29.10.2006 12:13:03            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					// Since we only assume a none status here due to svn_client_status()
   446   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// returning an error, make sure that this status times out soon.
   447   7958 07.11.2006 18:41:29            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					CSVNStatusCache::Instance().m_folderCrawler.BlockPath(m_directoryPath, 2000);
   448   7883 29.10.2006 12:13:03            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					CSVNStatusCache::Instance().AddFolderForCrawling(m_directoryPath);
   449   7883 29.10.2006 12:13:03            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					return CStatusCacheEntry();
   450   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   451   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   452   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   453   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		else
   454   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   455   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			ATLTRACE("Skipped SVN status for unversioned folder\n");
   456   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   457   3319 11.05.2005 22:57:24            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   458   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// Now that we've refreshed our SVN status, we can see if it's 
   459   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// changed the 'most important' status value for this directory.
   460   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// If it has, then we should tell our parent
   461   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	UpdateCurrentStatus();
   462   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   463   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	if (path.IsDirectory())
   464   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   465   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		CCachedDirectory * dirEntry = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
   466   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if ((dirEntry)&&(dirEntry->IsOwnStatusValid()))
   467   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   468   4064 04.08.2005 18:51:42            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			CSVNStatusCache::Instance().AddFolderForCrawling(path);
   469   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			return dirEntry->GetOwnStatus(bRecursive);
   470   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   471   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   472   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		// If the status *still* isn't valid here, it means that 
   473   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		// the current directory is unversioned, and we shall need to ask its children for info about themselves
   474   5129 11.12.2005 17:57:52            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (dirEntry)
   475   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			return dirEntry->GetStatusForMember(path,bRecursive);
   476   5129 11.12.2005 17:57:52            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		CSVNStatusCache::Instance().AddFolderForCrawling(path);
   477   5129 11.12.2005 17:57:52            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		return CStatusCacheEntry();
   478   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   479   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	else
   480   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   481   2563 27.01.2005 23:52:19            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
   482   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		if(itMap != m_entryCache.end())
   483   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   484   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			return itMap->second;
   485   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   486   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   487   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   488   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	AddEntry(path, NULL);
   489   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	return CStatusCacheEntry();
   490   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   491   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   492   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          void 
   493   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      CCachedDirectory::AddEntry(const CTSVNPath& path, const svn_wc_status2_t* pSVNStatus, DWORD validuntil /* = 0*/)
   494   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   495   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	AutoLocker lock(m_critSec);
   496   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	if(path.IsDirectory())
   497   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   498   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		CCachedDirectory * childDir = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
   499   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (childDir)
   500   7439 10.09.2006 16:17:46            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		{
   501   3202 03.05.2005 18:05:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			childDir->m_ownStatus.SetStatus(pSVNStatus);
   502   7439 10.09.2006 16:17:46            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			childDir->m_ownStatus.SetKind(svn_node_dir);
   503   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   504   7439 10.09.2006 16:17:46            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      	}
   505   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	else
   506   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   507   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		CString cachekey = GetCacheKey(path);
   508   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		CacheEntryMap::iterator entry_it = m_entryCache.lower_bound(cachekey);
   509   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (entry_it != m_entryCache.end() && entry_it->first == cachekey)
   510   4967 17.11.2005 19:27:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   511   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (pSVNStatus)
   512   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   513   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (entry_it->second.GetEffectiveStatus() > svn_wc_status_unversioned &&
   514   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					entry_it->second.GetEffectiveStatus() != SVNStatus::GetMoreImportant(pSVNStatus->prop_status, pSVNStatus->text_status))
   515   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   516   4966 17.11.2005 18:53:21            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					CSVNStatusCache::Instance().UpdateShell(path);
   517   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					ATLTRACE("shell update for %ws\n", path.GetWinPath());
   518   4967 17.11.2005 19:27:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				}
   519   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   520   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   521   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		else
   522   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   523   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			entry_it = m_entryCache.insert(entry_it, std::make_pair(cachekey, CStatusCacheEntry()));
   524   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   525   6702 03.06.2006 10:19:27            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		entry_it->second = CStatusCacheEntry(pSVNStatus, path.GetLastWriteTime(), path.IsReadOnly(), validuntil);
   526   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   527   6311 17.04.2006 12:11:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      }
   528   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   529   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   530   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CString 
   531   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CCachedDirectory::GetCacheKey(const CTSVNPath& path)
   532   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   533   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// All we put into the cache as a key is just the end portion of the pathname
   534   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// There's no point storing the path of the containing directory for every item
   535   4876 05.11.2005 09:42:28            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	return path.GetWinPathString().Mid(m_directoryPath.GetWinPathString().GetLength());
   536   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   537   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   538   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CString 
   539   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CCachedDirectory::GetFullPathString(const CString& cacheKey)
   540   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   541   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	return m_directoryPath.GetWinPathString() + _T("\\") + cacheKey;
   542   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   543   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   544   2983 06.04.2005 18:49:35            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      void CCachedDirectory::GetStatusCallback(void *baton, const char *path, svn_wc_status2_t *status)
   545   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   546   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	CCachedDirectory* pThis = (CCachedDirectory*)baton;
   547   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   548   2949 01.04.2005 21:34:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if (path == NULL)
   549   2949 01.04.2005 21:34:58            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		return;
   550   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		
   551   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	CTSVNPath svnPath;
   552   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   553   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	if(status->entry)
   554   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   555  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		if ((status->text_status != svn_wc_status_none)&&(status->text_status != svn_wc_status_ignored))
   556   8766 21.02.2007 18:56:04            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			svnPath.SetFromSVN(path, (status->entry->kind == svn_node_dir));
   557  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		else
   558  10403 22.08.2007 22:23:58            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			svnPath.SetFromSVN(path);
   559   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   560   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		if(svnPath.IsDirectory())
   561   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   562   8157 29.11.2006 16:46:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			if(!svnPath.IsEquivalentToWithoutCase(pThis->m_directoryPath))
   563   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			{
   564   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (pThis->m_bRecursive)
   565   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   566   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					// Add any versioned directory, which is not our 'self' entry, to the list for having its status updated
   567   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					CSVNStatusCache::Instance().AddFolderForCrawling(svnPath);
   568   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				}
   569   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   570   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// Make sure we know about this child directory
   571   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				// This initial status value is likely to be overwritten from below at some point
   572   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				svn_wc_status_kind s = SVNStatus::GetMoreImportant(status->text_status, status->prop_status);
   573   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				CCachedDirectory * cdir = CSVNStatusCache::Instance().GetDirectoryCacheEntryNoCreate(svnPath);
   574   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if (cdir)
   575   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   576   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// This child directory is already in our cache!
   577   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// So ask this dir about its recursive status
   578   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					pThis->m_childDirectories[svnPath] = SVNStatus::GetMoreImportant(s, cdir->GetCurrentFullStatus());
   579   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   580   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				else
   581   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				{
   582   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					// the child directory is not in the cache. Create a new entry for it in the cache which is
   583   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					// initially 'unversioned'. But we added that directory to the crawling list above, which
   584   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					// means the cache will be updated soon.
   585   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      					CSVNStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
   586   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					pThis->m_childDirectories[svnPath] = s;
   587   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   588   5101 07.12.2005 19:10:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   589   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		}
   590   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		else
   591   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		{
   592   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// Keep track of the most important status of all the files in this directory
   593   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// Don't include subdirectories in this figure, because they need to provide their 
   594   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			// own 'most important' value
   595   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			pThis->m_mostImportantFileStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status->text_status);
   596   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			pThis->m_mostImportantFileStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status->prop_status);
   597   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			if (((status->text_status == svn_wc_status_unversioned)||(status->text_status == svn_wc_status_none))
   598   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				&&(CSVNStatusCache::Instance().IsUnversionedAsModified()))
   599   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   600   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// treat unversioned files as modified
   601   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				pThis->m_mostImportantFileStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, svn_wc_status_modified);
   602   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			}
   603   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   604   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   605   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	else
   606   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   607   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		svnPath.SetFromSVN(path);
   608   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// Subversion returns no 'entry' field for versioned folders if they're
   609   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// part of another working copy (nested layouts).
   610   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// So we have to make sure that such an 'unversioned' folder really
   611   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		// is unversioned.
   612   8157 29.11.2006 16:46:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		if ((status->text_status == svn_wc_status_unversioned)&&(!svnPath.IsEquivalentToWithoutCase(pThis->m_directoryPath))&&(svnPath.IsDirectory()))
   613   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   614   2740 25.02.2005 22:01:17            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if (svnPath.HasAdminDir())
   615   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   616   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				CSVNStatusCache::Instance().AddFolderForCrawling(svnPath);
   617   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// Mark the directory as 'versioned' (status 'normal' for now).
   618   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// This initial value will be overwritten from below some time later
   619   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				pThis->m_childDirectories[svnPath] = svn_wc_status_normal;
   620   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				// Make sure the entry is also in the cache
   621   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				CSVNStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
   622   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				// also mark the status in the status object as normal
   623   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				status->text_status = svn_wc_status_normal;
   624   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			}
   625   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   626   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		else if (status->text_status == svn_wc_status_external)
   627   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   628   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			CSVNStatusCache::Instance().AddFolderForCrawling(svnPath);
   629   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			// Mark the directory as 'versioned' (status 'normal' for now).
   630   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			// This initial value will be overwritten from below some time later
   631   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			pThis->m_childDirectories[svnPath] = svn_wc_status_normal;
   632   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			// we have added a directory to the child-directory list of this
   633   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			// directory. We now must make sure that this directory also has
   634   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			// an entry in the cache.
   635   8047 16.11.2006 21:57:52            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			CSVNStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
   636   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			// also mark the status in the status object as normal
   637   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			status->text_status = svn_wc_status_normal;
   638   2640 08.02.2005 18:19:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   639   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		else
   640   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		{
   641   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			if (svnPath.IsDirectory())
   642   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				pThis->m_childDirectories[svnPath] = SVNStatus::GetMoreImportant(status->text_status, status->prop_status);
   643   9735 12.06.2007 19:08:51            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			else if ((CSVNStatusCache::Instance().IsUnversionedAsModified())&&(status->text_status != svn_wc_status_ignored))
   644   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			{
   645   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				// make this unversioned item change the most important status of this
   646   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				// folder to modified if it doesn't already have another status
   647   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				pThis->m_mostImportantFileStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, svn_wc_status_modified);
   648   3274 08.05.2005 10:58:20            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			}
   649   8140 27.11.2006 23:01:32            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		}
   650   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      	}
   651   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   652   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	pThis->AddEntry(svnPath, status);
   653   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   654   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   655   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          bool 
   656   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          CCachedDirectory::IsOwnStatusValid() const
   657   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   658   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	return m_ownStatus.HasBeenSet() && 
   659   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		   !m_ownStatus.HasExpired(GetTickCount()) &&
   660   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		   // 'external' isn't a valid status. That just
   661   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		   // means the folder is not part of the current working
   662   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		   // copy but it still has its own 'real' status
   663   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		   m_ownStatus.GetEffectiveStatus()!=svn_wc_status_external &&
   664   5771 18.02.2006 19:11:49            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		   m_ownStatus.IsKindKnown();
   665   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   666   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   667   4260 26.08.2005 19:01:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      void CCachedDirectory::Invalidate()
   668   4260 26.08.2005 19:01:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      {
   669   4260 26.08.2005 19:01:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	m_ownStatus.Invalidate();
   670   4260 26.08.2005 19:01:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      }
   671   4260 26.08.2005 19:01:22            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   672   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          svn_wc_status_kind CCachedDirectory::CalculateRecursiveStatus() const
   673   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   674   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// Combine our OWN folder status with the most important of our *FILES'* status.
   675   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	svn_wc_status_kind retVal = SVNStatus::GetMoreImportant(m_mostImportantFileStatus, m_ownStatus.GetEffectiveStatus());
   676   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   677   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if ((retVal != svn_wc_status_modified)&&(retVal != m_ownStatus.GetEffectiveStatus()))
   678   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
   679   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if ((retVal == svn_wc_status_added)||(retVal == svn_wc_status_deleted)||(retVal == svn_wc_status_missing))
   680   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			retVal = svn_wc_status_modified;
   681   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   682   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   683   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// Now combine all our child-directorie's status
   684   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	
   685   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	ChildDirStatus::const_iterator it;
   686   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	for(it = m_childDirectories.begin(); it != m_childDirectories.end(); ++it)
   687   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   688   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		retVal = SVNStatus::GetMoreImportant(retVal, it->second);
   689   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if ((retVal != svn_wc_status_modified)&&(retVal != m_ownStatus.GetEffectiveStatus()))
   690   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   691   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			if ((retVal == svn_wc_status_added)||(retVal == svn_wc_status_deleted)||(retVal == svn_wc_status_missing))
   692   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				retVal = svn_wc_status_modified;
   693   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		}
   694   2862 18.03.2005 15:26:09            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   695   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	
   696   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	return retVal;
   697   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   698   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   699   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          // Update our composite status and deal with things if it's changed
   700   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      void CCachedDirectory::UpdateCurrentStatus()
   701   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   702   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	svn_wc_status_kind newStatus = CalculateRecursiveStatus();
   703   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   704   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if ((newStatus != m_currentFullStatus)&&(m_ownStatus.IsVersioned()))
   705   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
   706   4959 16.11.2005 17:25:33            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (m_currentFullStatus != svn_wc_status_none)
   707   4959 16.11.2005 17:25:33            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   708   4953 15.11.2005 19:24:41            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			// Our status has changed - tell the shell
   709   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			ATLTRACE("Dir %ws, status change from %d to %d, send shell notification\n", m_directoryPath.GetWinPath(), m_currentFullStatus, newStatus);		
   710   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			CSVNStatusCache::Instance().UpdateShell(m_directoryPath);
   711   4959 16.11.2005 17:25:33            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   712   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		m_currentFullStatus = newStatus;
   713   4967 17.11.2005 19:27:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   714   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// And tell our parent, if we've got one...
   715   4967 17.11.2005 19:27:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	// we tell our parent *always* about our status, even if it hasn't
   716   4967 17.11.2005 19:27:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	// changed. This is to make sure that the parent has really our current
   717   4967 17.11.2005 19:27:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	// status - the parent can decide itself if our status has changed
   718   4967 17.11.2005 19:27:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	// or not.
   719   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	CTSVNPath parentPath = m_directoryPath.GetContainingDirectory();
   720   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	if(!parentPath.IsEmpty())
   721   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   722   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		// We have a parent
   723   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		CCachedDirectory * cachedDir = CSVNStatusCache::Instance().GetDirectoryCacheEntry(parentPath);
   724   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (cachedDir)
   725   5058 02.12.2005 23:38:31            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			cachedDir->UpdateChildDirectoryStatus(m_directoryPath, m_currentFullStatus);
   726   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   727   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   728   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   729   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   730   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          // Receive a notification from a child that its status has changed
   731   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      void CCachedDirectory::UpdateChildDirectoryStatus(const CTSVNPath& childDir, svn_wc_status_kind childStatus)
   732   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   733   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	svn_wc_status_kind currentStatus = m_childDirectories[childDir];
   734   2852 17.03.2005 20:38:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if ((currentStatus != childStatus)||(!IsOwnStatusValid()))
   735   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   736   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   737   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			AutoLocker lock(m_critSec);
   738   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			m_childDirectories[childDir] = childStatus;
   739   3256 06.05.2005 15:11:45            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   740   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		UpdateCurrentStatus();
   741   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   742   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   743   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   744   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      CStatusCacheEntry CCachedDirectory::GetOwnStatus(bool bRecursive)
   745   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   746   2600 01.02.2005 09:59:44            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// Don't return recursive status if we're unversioned ourselves.
   747   2600 01.02.2005 09:59:44            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	if(bRecursive && m_ownStatus.GetEffectiveStatus() > svn_wc_status_unversioned)
   748   2561 27.01.2005 23:03:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   749   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		CStatusCacheEntry recursiveStatus(m_ownStatus);
   750   3263 06.05.2005 20:40:11            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		UpdateCurrentStatus();
   751   2638 08.02.2005 13:41:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		recursiveStatus.ForceStatus(m_currentFullStatus);
   752   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		return recursiveStatus;				
   753   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   754   2561 27.01.2005 23:03:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	else
   755   2561 27.01.2005 23:03:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   756   2561 27.01.2005 23:03:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          		return m_ownStatus;
   757   2561 27.01.2005 23:03:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	}
   758   2561 27.01.2005 23:03:18            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          }
   759   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   760   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      void CCachedDirectory::RefreshStatus(bool bRecursive)
   761   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          {
   762   3611 02.06.2005 18:25:07            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	AutoLocker lock(m_critSec);
   763   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// Make sure that our own status is up-to-date
   764   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	GetStatusForMember(m_directoryPath,bRecursive);
   765   3982 24.07.2005 14:06:25            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   766   2607 02.02.2005 18:27:56            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	// We also need to check if all our file members have the right date on them
   767   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	CacheEntryMap::iterator itMembers;
   768   4877 05.11.2005 10:01:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	std::set<CTSVNPath> refreshedpaths;
   769   2852 17.03.2005 20:38:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	DWORD now = GetTickCount();
   770   5439 17.01.2006 20:35:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if (m_entryCache.size() == 0)
   771   5439 17.01.2006 20:35:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		return;
   772   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	for (itMembers = m_entryCache.begin(); itMembers != m_entryCache.end(); ++itMembers)
   773   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          	{
   774   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		if (itMembers->first)
   775   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   776   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			CTSVNPath filePath(m_directoryPath);
   777   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			filePath.AppendPathString(itMembers->first);
   778   6320 18.04.2006 17:19:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			std::set<CTSVNPath>::iterator refr_it;
   779   8157 29.11.2006 16:46:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			if ((!filePath.IsEquivalentToWithoutCase(m_directoryPath))&&
   780   8157 29.11.2006 16:46:37            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      				(((refr_it = refreshedpaths.lower_bound(filePath)) == refreshedpaths.end()) || !filePath.IsEquivalentToWithoutCase(*refr_it)))
   781   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			{
   782   2852 17.03.2005 20:38:55            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				if ((itMembers->second.HasExpired(now))||(!itMembers->second.DoesFileTimeMatch(filePath.GetLastWriteTime())))
   783   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				{
   784   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          					// We need to request this item as well
   785   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					GetStatusForMember(filePath,bRecursive);
   786   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// GetStatusForMember now has recreated the m_entryCache map.
   787   4877 05.11.2005 10:01:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// So start the loop again, but add this path to the refreshedpaths set
   788   4877 05.11.2005 10:01:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// to make sure we don't refresh this path again. This is to make sure
   789   4877 05.11.2005 10:01:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// that we don't end up in an endless loop.
   790   6320 18.04.2006 17:19:12            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					refreshedpaths.insert(refr_it, filePath);
   791   4877 05.11.2005 10:01:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					itMembers = m_entryCache.begin();
   792   5172 16.12.2005 22:33:53            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					if (m_entryCache.size()==0)
   793   5172 16.12.2005 22:33:53            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      						return;
   794   4877 05.11.2005 10:01:10            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					continue;
   795   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   796   3534 28.05.2005 14:46:02            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				else if ((bRecursive)&&(itMembers->second.IsDirectory()))
   797   3135 25.04.2005 20:02:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      				{
   798   3135 25.04.2005 20:02:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// crawl all subfolders too! Otherwise a change deep inside the
   799   3135 25.04.2005 20:02:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					// tree which has changed won't get propagated up the tree.
   800   5404 12.01.2006 19:04:57            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      					CSVNStatusCache::Instance().AddFolderForCrawling(filePath);
   801   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          				}
   802   2553 27.01.2005 00:28:26            /trunk/src/TSVNCache/CachedDirectory.cpp                     wdean                          			}
   803   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   804   2856 18.03.2005 10:22:50            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   805   3135 25.04.2005 20:02:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      }
   806   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      
   807   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      void CCachedDirectory::RefreshMostImportant()
   808   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      {
   809   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	CacheEntryMap::iterator itMembers;
   810   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	svn_wc_status_kind newStatus = m_ownStatus.GetEffectiveStatus();
   811   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	for (itMembers = m_entryCache.begin(); itMembers != m_entryCache.end(); ++itMembers)
   812   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
   813   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		newStatus = SVNStatus::GetMoreImportant(newStatus, itMembers->second.GetEffectiveStatus());
   814   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      		if (((itMembers->second.GetEffectiveStatus() == svn_wc_status_unversioned)||(itMembers->second.GetEffectiveStatus() == svn_wc_status_none))
   815   9084 29.03.2007 22:50:47            /branches/1.4.x/src/TSVNCache/CachedDirectory.cpp            steveking                      			&&(CSVNStatusCache::Instance().IsUnversionedAsModified()))
   816   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		{
   817   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			// treat unversioned files as modified
   818   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      			newStatus = SVNStatus::GetMoreImportant(newStatus, svn_wc_status_modified);
   819   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		}
   820   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   821   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	if (newStatus != m_mostImportantFileStatus)
   822   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	{
   823   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		ATLTRACE("status change of path %ws\n", m_directoryPath.GetWinPath());
   824   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      		CSVNStatusCache::Instance().UpdateShell(m_directoryPath);
   825   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	}
   826   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      	m_mostImportantFileStatus = newStatus;
   827   6769 12.06.2006 19:32:00            /trunk/src/TSVNCache/CachedDirectory.cpp                     steveking                      }
