// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "pathutils.h"

BOOL CPathUtils::MakeSureDirectoryPathExists(LPCTSTR path)
{
	size_t len = _tcslen(path);
	TCHAR * buf = new TCHAR[len+10];
	TCHAR * internalpathbuf = new TCHAR[len+10];
	TCHAR * pPath = internalpathbuf;
	SECURITY_ATTRIBUTES attribs;

	ZeroMemory(&attribs, sizeof(SECURITY_ATTRIBUTES));

	attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
	attribs.bInheritHandle = FALSE;

	ConvertToBackslash(internalpathbuf, path);
	do
	{
		ZeroMemory(buf, (len+10)*sizeof(TCHAR));
		_tcsncpy(buf, internalpathbuf, _tcschr(pPath, '\\') - internalpathbuf);
		CreateDirectory(buf, &attribs);
		pPath = _tcschr(pPath, '\\');
		pPath++;
	} while (_tcschr(pPath, '\\'));
	
	BOOL bRet = CreateDirectory(internalpathbuf, &attribs);
	delete buf;
	delete internalpathbuf;
	return bRet;
}

void CPathUtils::ConvertToBackslash(LPTSTR dest, LPCTSTR src)
{
	_tcscpy(dest, src);
	TCHAR * p = dest;
	for (; *p != '\0'; ++p)
		if (*p == '/')
			*p = '\\';
}

