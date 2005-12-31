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

	ConvertToBackslash(internalpathbuf, path, len+10);
	do
	{
		ZeroMemory(buf, (len+10)*sizeof(TCHAR));
		TCHAR * slashpos = _tcschr(pPath, '\\');
		if (slashpos)
			_tcsncpy_s(buf, len+10, internalpathbuf, slashpos - internalpathbuf);
		else
			_tcsncpy_s(buf, len+10, internalpathbuf, len+10);
		CreateDirectory(buf, &attribs);
		pPath = _tcschr(pPath, '\\');
	} while ((pPath++)&&(_tcschr(pPath, '\\')));
	
	BOOL bRet = CreateDirectory(internalpathbuf, &attribs);
	delete buf;
	delete internalpathbuf;
	return bRet;
}

void CPathUtils::ConvertToBackslash(LPTSTR dest, LPCTSTR src, size_t len)
{
	_tcscpy_s(dest, len, src);
	TCHAR * p = dest;
	for (; *p != '\0'; ++p)
		if (*p == '/')
			*p = '\\';
}

