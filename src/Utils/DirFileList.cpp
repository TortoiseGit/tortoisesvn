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
#include "StdAfx.h"
#include "dirfilelist.h"


CDirFileList::CDirFileList()
{
   SetSize(0,50);
}

void CDirFileList::BuildList(const CString dirName, const BOOL recurse, const BOOL includeDirs)
{
	CFileFind finder;
	BOOL working = finder.FindFile(dirName+_T("\\*.*"));
	while (working)
	{
		working = finder.FindNextFile();
		//skip "." and ".."
		if (finder.IsDots())
			continue;

		//if its a directory then recurse
		if (finder.IsDirectory())
		{
			if (includeDirs)
			{
				Add(finder.GetFilePath());
			}
			if (recurse)
			{
				BuildList(finder.GetFilePath(), recurse, includeDirs);
			}
		} // if (finder.IsDirectory())
		else
		{
			Add(finder.GetFilePath());
		}
	} // while (working)
	finder.Close();
}

