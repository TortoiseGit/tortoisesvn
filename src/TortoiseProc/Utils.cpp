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

CString CUtils::GetDiffPath()
{
	CString diffpath;
	//now get the path to the diff program
	CRegString diffexe(_T("Software\\TortoiseSVN\\Diff"));
	if (static_cast<CString>(diffexe).IsEmpty())
	{
		//no registry entry for a diff program
		//sad but true so let's search for one

		//first check if there's a WinSDK
		CRegString sdk(_T("Software\\Microsoft\\Win32SDK\\Directories\\Install Dir"));
		if (static_cast<CString>(sdk).IsEmpty())
		{
			//shit, no SDK installed
			//try VisualStudio
			CRegString vs(_T("Software\\Microsoft\\VisualStudio\\6.0\\InstallDir"));
			if (static_cast<CString>(vs).IsEmpty())
			{
				//try VisualStudio 7
				vs = CRegString(_T("Software\\Microsoft\\VisualStudio\\7.0\\InstallDir"));
				if (static_cast<CString>(vs).IsEmpty())
					vs = CRegString(_T("Software\\Microsoft\\VisualStudio\\7.1\\InstallDir"));
				if (!static_cast<CString>(vs).IsEmpty())
				{
					//the visual studio install dir looks like C:\programs\Microsoft Visual Studio\Common\IDE
					//so we have to remove the last part of the path
					diffpath = vs;
					diffpath.TrimRight('\\');		//remove trailing slash
					diffpath.Left(diffpath.ReverseFind('\\'));
					diffpath += _T("\\Tools\\Bin\\WinDiff.exe");
				}
			} // if (static_cast<CString>(vs).IsEmpty())
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
		//even our own registry entry could point to a nonexistent file
		if (!PathFileExists((LPCTSTR)diffpath))
			diffpath = "";
	}
	if (static_cast<CString>(diffpath).IsEmpty())
	{
		//no diff program found, so tell this to the user and
		//ask for one
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
		temp.LoadString(IDS_SETTINGS_SELECTDIFF);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			diffpath = CString(ofn.lpstrFile);
			diffexe = diffpath;
		}

	}
	return diffpath;
}

BOOL CUtils::StartExtMerge(CString basefile, CString theirfile, CString yourfile, CString mergedfile)
{
	CString com;
	CRegString regCom = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	com = regCom;
	
	if (com.IsEmpty())
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
		temp.LoadString(IDS_SETTINGS_SELECTMERGE);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			com = CString(ofn.lpstrFile);
			regCom = com;
		}
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

BOOL CUtils::StartDiffViewer(CString file)
{
	CRegString v = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
	CString viewer = v;
	if (viewer.IsEmpty())
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
		}
	} // if (viewer.IsEmpty())
	if (viewer.IsEmpty())
		return FALSE;

	if (viewer.Find(_T("%1")) >= 0)
	{
		viewer.Replace(_T("%1"), file);
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

void CUtils::Unescape(LPTSTR psz)
{
	LPTSTR pszSource = psz;
	LPTSTR pszDest = psz;

	static const TCHAR szHex[] = _T("0123456789ABCDEF");

	// Unescape special characters. The number of characters
	// in the *pszDest is assumed to be <= the number of characters
	// in pszSource (they are both the same string anyway)

	while (*pszSource != '\0' && *pszDest != '\0')
	{
		if (*pszSource == '+')
			*pszDest++ = ' ';
		else if (*pszSource == '%')
		{
			// The next two chars following '%' should be digits
			if ( *(pszSource + 1) == '\0' ||
				 *(pszSource + 2) == '\0' )
			{
				// nothing left to do
				break;
			}

			TCHAR nValue = '?';
			LPCTSTR pszLow = NULL;
			LPCTSTR pszHigh = NULL;
			pszSource++;

			*pszSource = (TCHAR) _totupper(*pszSource);
			pszHigh = _tcschr(szHex, *pszSource);

			if (pszHigh != NULL)
			{
				pszSource++;
				*pszSource = (TCHAR) _totupper(*pszSource);
				pszLow = _tcschr(szHex, *pszSource);

				if (pszLow != NULL)
				{
					nValue = (TCHAR) (((pszHigh - szHex) << 4) +
									(pszLow - szHex));
				}
			}
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
	ret.Replace(_T(" "), _T("%20"));
	ret.Replace(_T("^"), _T("%5E"));
	ret.Replace(_T("&"), _T("%26"));
	ret.Replace(_T("`"), _T("%60"));
	ret.Replace(_T("{"), _T("%7B"));
	ret.Replace(_T("}"), _T("%7D"));
	ret.Replace(_T("|"), _T("%7C"));
	ret.Replace(_T("]"), _T("%5D"));
	ret.Replace(_T("["), _T("%5B"));
	ret.Replace(_T("\""), _T("%22"));
	ret.Replace(_T("<"), _T("%3C"));
	ret.Replace(_T(">"), _T("%3E"));
	ret.Replace(_T("\\"), _T("%5C"));
	ret.Replace(_T("#"), _T("%23"));
	ret.Replace(_T("?"), _T("%3F"));
	return ret;
}

