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

#ifdef _MFC_VER
void CPathUtils::ConvertToBackslash(LPTSTR dest, LPCTSTR src, size_t len)
{
	_tcscpy_s(dest, len, src);
	TCHAR * p = dest;
	for (; *p != '\0'; ++p)
		if (*p == '/')
			*p = '\\';
}

CString CPathUtils::GetFileNameFromPath(CString sPath)
{
	CString ret;
	sPath.Replace(_T("/"), _T("\\"));
	ret = sPath.Mid(sPath.ReverseFind('\\') + 1);
	return ret;
}

CString CPathUtils::GetFileExtFromPath(const CString& sPath)
{
	int dotPos = sPath.ReverseFind('.');
	int slashPos = sPath.ReverseFind('\\');
	if (slashPos < 0)
		slashPos = sPath.ReverseFind('/');
	if (dotPos > slashPos)
		return sPath.Mid(dotPos);
	return CString();
}

CString CPathUtils::GetLongPathname(const CString& path)
{
	if (path.IsEmpty())
		return path;
	TCHAR pathbufcanonicalized[MAX_PATH]; // MAX_PATH ok.
	DWORD ret = 0;
	CString sRet;
	if (PathCanonicalize(pathbufcanonicalized, path))
	{
		ret = ::GetLongPathName(pathbufcanonicalized, NULL, 0);
		TCHAR * pathbuf = new TCHAR[ret+2];	
		ret = ::GetLongPathName(pathbufcanonicalized, pathbuf, ret+1);
		sRet = CString(pathbuf, ret);
		delete pathbuf;
	}
	else
	{
		ret = ::GetLongPathName(path, NULL, 0);
		TCHAR * pathbuf = new TCHAR[ret+2];
		ret = ::GetLongPathName(path, pathbuf, ret+1);
		sRet = CString(pathbuf, ret);
		delete pathbuf;
	}
	if (ret == 0)
		return path;
	return sRet;
}

BOOL CPathUtils::FileCopy(CString srcPath, CString destPath, BOOL force)
{
	srcPath.Replace('/', '\\');
	destPath.Replace('/', '\\');
	CString destFolder = destPath.Left(destPath.ReverseFind('\\'));
	MakeSureDirectoryPathExists(destFolder);
	return (CopyFile(srcPath, destPath, !force));
}

CString CPathUtils::ParsePathInString(const CString& Str)
{
	CString sToken;
	int curPos = 0;
	sToken = Str.Tokenize(_T(" \t\r\n"), curPos);
	while (!sToken.IsEmpty())
	{
		if ((sToken.Find('/')>=0)||(sToken.Find('\\')>=0))
		{
			sToken.Trim(_T("'\""));
			return sToken;
		}
		sToken = Str.Tokenize(_T(" \t\r\n"), curPos);
	}
	sToken.Empty();
	return sToken;
}

CString CPathUtils::GetAppDirectory()
{
	CString path;
	DWORD len = 0;
	DWORD bufferlen = MAX_PATH;		// MAX_PATH is not the limit here!
	path.GetBuffer(bufferlen);
	do 
	{
		bufferlen += MAX_PATH;		// MAX_PATH is not the limit here!
		path.ReleaseBuffer(0);
		len = GetModuleFileName(NULL, path.GetBuffer(bufferlen+1), bufferlen);				
	} while(len == bufferlen);
	path.ReleaseBuffer();
	path = path.Left(path.ReverseFind('\\')+1);
	return path;
}

CString CPathUtils::GetAppParentDirectory()
{
	CString path = GetAppDirectory();
	path = path.Left(path.ReverseFind('\\'));
	path = path.Left(path.ReverseFind('\\')+1);
	return path;
}



#endif
