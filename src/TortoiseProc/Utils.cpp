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
//
#include "StdAfx.h"
#include "utils.h"

CUtils::CUtils(void)
{
}

CUtils::~CUtils(void)
{
}

CString CUtils::GetDiffPath()
{
	CString diffpath;
	//now get the path to the diff program
	CRegString diffexe(_T("Software\\TortoiseSVN\\Diff"));
	if (diffexe == _T(""))
	{
		//no registry entry for a diff program
		//sad but true so let's search for one

		//first check if there's a WinSDK
		CRegString sdk(_T("Software\\Microsoft\\Win32SDK\\Directories\\Install Dir"));
		if (sdk == _T(""))
		{
			//shit, no SDK installed
			//try VisualStudio
			CRegString vs(_T("Software\\Microsoft\\VisualStudio\\6.0\\InstallDir"));
			if (vs == _T(""))
			{
				//try VisualStudio 7
				vs = CRegString(_T("Software\\Microsoft\\VisualStudio\\7.0\\InstallDir"));
				if (vs == _T(""))
					vs = CRegString(_T("Software\\Microsoft\\VisualStudio\\7.1\\InstallDir"));
				if (vs != _T(""))
				{
					//the visual studio install dir looks like C:\programs\Microsoft Visual Studio\Common\IDE
					//so we have to remove the last part of the path
					diffpath = vs;
					diffpath.TrimRight('\\');		//remove trailing slash
					diffpath.Left(diffpath.ReverseFind('\\'));
					diffpath += _T("\\Tools\\Bin\\WinDiff.exe");
				}
			} // if (vs == "")
		}
		else
		{
			//SDK found, windiff is in its bin-directory (I hope)
			diffpath = sdk;
			diffpath += _T("bin\\WinDiff.exe");
		}
		if (!PathFileExists((LPCTSTR)diffpath))
		{
			diffpath = _T("");
		}
	} // if (diffexe == "")
	else
	{
		diffpath = diffexe;
		//even the our own registry entry could point to a nonexistent file
		if (!PathFileExists((LPCTSTR)diffpath))
			diffpath = "";
	}
	return diffpath;
}