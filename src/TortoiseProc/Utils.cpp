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
//
#include "StdAfx.h"
#include "resource.h"
#include "utils.h"
#include "afxdlgs.h"
#include "MessageBox.h"

CUtils::CUtils(void)
{
}

CUtils::~CUtils(void)
{
}

CString CUtils::GetTempFile()
{
	TCHAR path[MAX_PATH];
	TCHAR tempF[MAX_PATH];
	DWORD len = ::GetTempPath (MAX_PATH, path);
	UINT unique = ::GetTempFileName (path, TEXT("svn"), 0, tempF);
	CString tempfile = CString(tempF);
	return tempfile;
}

BOOL CUtils::StartExtMerge(CString basefile, CString theirfile, CString yourfile, CString mergedfile)
{
	CString com;
	CRegString regCom = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	com = regCom;
	
	if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseMerge
		CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
		com = tortoiseMergePath;
		com = com + _T(" /base:%base /theirs:%theirs /yours:%mine /merged:%merged");
	}

	TCHAR buf[MAX_PATH];
	_tcscpy(buf, basefile);
	PathQuoteSpaces(buf);
	basefile = CString(buf);
	_tcscpy(buf, theirfile);
	PathQuoteSpaces(buf);
	theirfile = CString(buf);
	_tcscpy(buf, yourfile);
	PathQuoteSpaces(buf);
	yourfile = CString(buf);
	_tcscpy(buf, mergedfile);
	PathQuoteSpaces(buf);
	mergedfile = CString(buf);

	com.Replace(_T("%base"), basefile);
	com.Replace(_T("%theirs"), theirfile);
	com.Replace(_T("%mine"), yourfile);
	com.Replace(_T("%merged"), mergedfile);

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	if (CreateProcess(NULL /*(LPCTSTR)diffpath*/, const_cast<TCHAR*>((LPCTSTR)com), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		CString temp;
		temp.Format(IDS_ERR_EXTMERGESTART, lpMsgBuf);
		CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
		LocalFree( lpMsgBuf );
	} // if (CreateProcess(NULL /*(LPCTSTR)diffpath*/, const_cast<TCHAR*>((LPCTSTR)com), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 

	return TRUE;
}

BOOL CUtils::StartDiffViewer(CString file, CString dir, BOOL bWait)
{
	// if "dir" is actually a file, then don't start the unified diff viewer
	// but the file diff application (e.g. TortoiseMerge, WinMerge, WinDiff, P4Diff, ...)
	CString viewer;
	if ((PathIsDirectory(dir)) || (dir.IsEmpty()))
	{
		CRegString v = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
		viewer = v;
		if (viewer.IsEmpty() && dir.IsEmpty())
		{
			//first try the default app which is associated with diff files
			CRegString diff = CRegString(_T(".diff\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
			viewer = diff;
			viewer = viewer + _T("\\Shell\\Open\\Command\\");
			CRegString diffexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
			viewer = diffexe;
			if (viewer.IsEmpty())
			{
				CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
				viewer = txt;
				viewer = viewer + _T("\\Shell\\Open\\Command\\");
				CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
				viewer = txtexe;
			} // if (viewer.IsEmpty()) 
			TCHAR buf[MAX_PATH+1];
			ExpandEnvironmentStrings(viewer, buf, MAX_PATH);
			viewer = buf;
		} // if (viewer.IsEmpty())
		if (viewer.IsEmpty() && !dir.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
		{
			// use TortoiseMerge
			CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
			viewer = tortoiseMergePath;
			viewer = viewer + _T(" /patchpath:%path /diff:%base");
		} // if (viewer.IsEmpty() && !dir.IsEmpty())
		if (viewer.IsEmpty())
			return FALSE;
		if (viewer.Find(_T("%base")) >= 0)
		{
			viewer.Replace(_T("%base"), _T("\"")+file+_T("\""));
		}
		else
		{
			viewer += _T(" ");
			viewer += file;
		}
		if (viewer.Find(_T("%path")) >= 0)
		{
			viewer.Replace(_T("%path"), dir);
		}
	} // if ((PathIsDirectory(dir)) || (dir.IsEmpty())) 
	else
	{
		// not a unified diff
		CRegString diffexe(_T("Software\\TortoiseSVN\\Diff"));
		viewer = diffexe;
		if (viewer.IsEmpty()||(viewer.Left(1).Compare(_T("#"))==0))
		{
			//no registry entry for a diff program
			//use TortoiseMerge
			CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
			viewer = tortoiseMergePath;
			viewer = viewer + _T(" /base:%base /yours:%mine");
		} // if (diffexe == "")
		if (viewer.Find(_T("%base")) >= 0)
		{
			viewer.Replace(_T("%base"),  _T("\"")+file+_T("\""));
		}
		else
		{
			viewer += _T(" ");
			viewer += file;
		}
		if (viewer.Find(_T("%mine")) >= 0)
		{
			viewer.Replace(_T("%mine"),  _T("\"")+dir+_T("\""));
		}
		else
		{
			viewer += _T(" ");
			viewer += dir;
		}
	}

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)viewer), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		CString temp;
		if ((PathIsDirectory(dir)) || (dir.IsEmpty()))
			temp.Format(IDS_ERR_DIFFVIEWSTART, lpMsgBuf);
		else
			temp.Format(IDS_ERR_EXTDIFFSTART, lpMsgBuf);
		CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
		LocalFree( lpMsgBuf );
		return FALSE;
	} // if (CreateProcess(NULL /*(LPCTSTR)diffpath*/, const_cast<TCHAR*>((LPCTSTR)viewer), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 

	if (bWait)
	{
		WaitForInputIdle(process.hProcess, 10000);
	}

	return TRUE;
}

BOOL CUtils::StartTextViewer(CString file)
{
	CString viewer;
	CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txt;
	viewer = viewer + _T("\\Shell\\Open\\Command\\");
	CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txtexe;
	TCHAR buf[MAX_PATH+1];
	ExpandEnvironmentStrings(viewer, buf, MAX_PATH);
	viewer = buf;
	if (viewer.IsEmpty())
	{
		OPENFILENAME ofn;		// common dialog box structure
		TCHAR szFile[MAX_PATH];  // buffer for file name
		ZeroMemory(szFile, sizeof(szFile));
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		//ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		ofn.lpstrFilter = _T("Programs\0*.exe\0All\0*.*\0");
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		CString temp;
		temp.LoadString(IDS_UTILS_SELECTTEXTVIEWER);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			viewer = CString(ofn.lpstrFile);
		} // if (GetOpenFileName(&ofn)==TRUE)
		else
			return FALSE;
	}
	if (viewer.Find(_T("%base")) >= 0)
	{
		viewer.Replace(_T("%base"),  _T("\"")+file+_T("\""));
	}
	else
	{
		viewer += _T(" ");
		viewer += file;
	}

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)viewer), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		CString temp;
		temp.Format(IDS_ERR_DIFFVIEWSTART, lpMsgBuf);
		CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
		LocalFree( lpMsgBuf );
		return FALSE;
	} // if (CreateProcess(NULL /*(LPCTSTR)diffpath*/, const_cast<TCHAR*>((LPCTSTR)viewer), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0) 

	return TRUE;
}

void CUtils::Unescape(char * psz)
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
			char * pszLow = NULL;
			char * pszHigh = NULL;
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

CString CUtils::PathEscape(CString path)
{
	CString ret = path;
	ret.Replace(_T(" "), _T("%mine0"));
	ret.Replace(_T("^"), _T("%5E"));
	ret.Replace(_T("&"), _T("%mine6"));
	ret.Replace(_T("`"), _T("%60"));
	ret.Replace(_T("{"), _T("%7B"));
	ret.Replace(_T("}"), _T("%7D"));
	ret.Replace(_T("|"), _T("%7C"));
	ret.Replace(_T("]"), _T("%5D"));
	ret.Replace(_T("["), _T("%5B"));
	ret.Replace(_T("\""), _T("%mine2"));
	ret.Replace(_T("<"), _T("%3C"));
	ret.Replace(_T(">"), _T("%3E"));
	ret.Replace(_T("\\"), _T("%5C"));
	ret.Replace(_T("#"), _T("%mine3"));
	ret.Replace(_T("?"), _T("%3F"));
	return ret;
}

CString CUtils::GetVersionFromFile(const CString & p_strDateiname)
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
			//int iPos;
			//do
			//{
			//	iPos = strReturn.Find(_T(", "));
			//	if (iPos > -1)
			//	{
			//		strReturn = strReturn.Left(iPos) + _T(".")\
			//		+ strReturn.Right(strReturn.GetLength()-iPos-2);
			//	}
			//} while (iPos != -1);

			free(pBuffer);
		}
	} 

	return strReturn;
}



