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

void CPathUtils::Unescape(char * psz)
{
	char * pszSource = psz;
	char * pszDest = psz;

	static const char szHex[] = "0123456789ABCDEF";

	// Unescape special characters. The number of characters
	// in the *pszDest is assumed to be <= the number of characters
	// in pszSource (they are both the same string anyway)

	while (*pszSource != '\0' && *pszDest != '\0')
	{
		if (*pszSource == '%')
		{
			// The next two chars following '%' should be digits
			if ( *(pszSource + 1) == '\0' ||
				*(pszSource + 2) == '\0' )
			{
				// nothing left to do
				break;
			}

			char nValue = '?';
			const char * pszLow = NULL;
			const char * pszHigh = NULL;
			pszSource++;

			*pszSource = (char) toupper(*pszSource);
			pszHigh = strchr(szHex, *pszSource);

			if (pszHigh != NULL)
			{
				pszSource++;
				*pszSource = (char) toupper(*pszSource);
				pszLow = strchr(szHex, *pszSource);

				if (pszLow != NULL)
				{
					nValue = (char) (((pszHigh - szHex) << 4) +
						(pszLow - szHex));
				}
			} // if (pszHigh != NULL) 
			*pszDest++ = nValue;
		} 
		else
			*pszDest++ = *pszSource;

		pszSource++;
	}

	*pszDest = '\0';
}

static const char iri_escape_chars[256] = {
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,

	/* 128 */
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
};

const char uri_autoescape_chars[256] = {
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

	/* 64 */
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
	0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

	/* 128 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

	/* 192 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

static const char uri_char_validity[256] = {
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 1, 0, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

	/* 64 */
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
	0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

	/* 128 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

	/* 192 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

CStringA CPathUtils::PathEscape(const CStringA& path)
{
	CStringA ret2;
	int c;
	int i;
	for (i=0; path[i]; ++i)
	{
		c = (unsigned char)path[i];
		if (iri_escape_chars[c])
		{
			// no escaping needed for that char
			ret2 += (unsigned char)path[i];
		}
		else
		{
			// char needs escaping
			CStringA temp;
			temp.Format("%%%02X", (unsigned char)c);
			ret2 += temp;
		}
	}
	CStringA ret;
	for (i=0; ret2[i]; ++i)
	{
		c = (unsigned char)ret2[i];
		if (uri_autoescape_chars[c])
		{
			// no escaping needed for that char
			ret += (unsigned char)ret2[i];
		}
		else
		{
			// char needs escaping
			CStringA temp;
			temp.Format("%%%02X", (unsigned char)c);
			ret += temp;
		}
	}

	ret.Replace(("file:///%5C"), ("file:///\\"));
	ret.Replace(("file:////%5C"), ("file:////\\"));

	return ret;
}

CString CPathUtils::GetVersionFromFile(const CString & p_strDateiname)
{
	struct TRANSARRAY
	{
		WORD wLanguageID;
		WORD wCharacterSet;
	};

	CString strReturn;
	DWORD dwReserved,dwBufferSize;
	dwBufferSize = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)p_strDateiname,&dwReserved);

	if (dwBufferSize > 0)
	{
		LPVOID pBuffer = (void*) malloc(dwBufferSize);

		if (pBuffer != (void*) NULL)
		{
			UINT        nInfoSize = 0,
				nFixedLength = 0;
			LPSTR       lpVersion = NULL;
			VOID*       lpFixedPointer;
			TRANSARRAY* lpTransArray;
			CString     strLangProduktVersion;

			GetFileVersionInfo((LPTSTR)(LPCTSTR)p_strDateiname,
				dwReserved,
				dwBufferSize,
				pBuffer);

			// Abfragen der aktuellen Sprache
			VerQueryValue(	pBuffer,
				_T("\\VarFileInfo\\Translation"),
				&lpFixedPointer,
				&nFixedLength);
			lpTransArray = (TRANSARRAY*) lpFixedPointer;

			strLangProduktVersion.Format(_T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
				lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

			VerQueryValue(pBuffer,
				(LPTSTR)(LPCTSTR)strLangProduktVersion,
				(LPVOID *)&lpVersion,
				&nInfoSize);
			strReturn = (LPCTSTR)lpVersion;
			free(pBuffer);
		}
	} 

	return strReturn;
}


#endif
