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

BOOL CUtils::StartExtMerge(CString basefile, CString theirfile, CString yourfile, CString mergedfile,
						   		CString basename, CString theirname, CString yourname, CString mergedname)
{

	CRegString regCom = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	CString ext = GetFileExtFromPath(mergedfile);
	CString com = regCom;

	if (ext != "")
	{
		// is there an extension specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + ext.MakeLower());
		if (CString(mergetool) != "")
		{
			com = mergetool;
			com = _T("\"") + com + _T("\"");
		}
	}
	
	if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseMerge
		CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
		com = tortoiseMergePath;
		if (com.IsEmpty())
		{
			TCHAR tmerge[MAX_PATH];
			GetModuleFileName(NULL, tmerge, MAX_PATH);
			com = tmerge;
			com.Replace(_T("TortoiseProc.exe"), _T("TortoiseMerge.exe"));
		}
		com = _T("\"") + com + _T("\"");
		com = com + _T(" /base:%base /theirs:%theirs /yours:%mine /merged:%merged");
		com = com + _T(" /basename:%bname /theirsname:%tname /yoursname:%yname /mergedname:%mname");
	}

	com.Replace(_T("%base"), _T("\"") + basefile + _T("\""));
	com.Replace(_T("%theirs"), _T("\"") + theirfile + _T("\""));
	com.Replace(_T("%mine"), _T("\"") + yourfile + _T("\""));
	com.Replace(_T("%merged"), _T("\"") + mergedfile + _T("\""));
	com.Replace(_T("%bname"), _T("\"") + basename + _T("\""));
	com.Replace(_T("%tname"), _T("\"") + theirname + _T("\""));
	com.Replace(_T("%yname"), _T("\"") + yourname + _T("\""));
	com.Replace(_T("%mname"), _T("\"") + mergedname + _T("\""));

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

BOOL CUtils::StartDiffViewer(CString file, CString dir, BOOL bWait,	CString name1, CString name2, CString ext)
{
	// change all paths to have backslashes instead of forward slashes
	file.Replace('/', '\\');
	dir.Replace('/', '\\');
	// if "dir" is actually a file, then don't start the unified diff viewer
	// but the file diff application (e.g. TortoiseMerge, WinMerge, WinDiff, P4Diff, ...)
	CString viewer;
	if ((PathIsDirectory(dir)) || (dir.IsEmpty()))
	{
		CRegString v = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
		viewer = v;
		if ((viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0)) && dir.IsEmpty())
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
			TCHAR buf[32*1024];
			ExpandEnvironmentStrings(viewer, buf, MAX_PATH);
			viewer = buf;
		} // if (viewer.IsEmpty())
		if ((viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0)) && !dir.IsEmpty())
		{
			// use TortoiseMerge
			CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
			viewer = tortoiseMergePath;
			if (viewer.IsEmpty())
			{
				TCHAR tmerge[MAX_PATH];
				GetModuleFileName(NULL, tmerge, MAX_PATH);
				viewer = tmerge;
				viewer.Replace(_T("TortoiseProc.exe"), _T("TortoiseMerge.exe"));
			}
			viewer = _T("\"") + viewer + _T("\"");
			viewer = viewer + _T(" /patchpath:%path /diff:%base");
		} // if (viewer.IsEmpty() && !dir.IsEmpty())
		if (viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
			return FALSE;
		if (viewer.Find(_T("%base")) >= 0)
		{
			viewer.Replace(_T("%base"), _T("\"")+file+_T("\""));
		}
		else if (viewer.Find(_T("%1")) >= 0)
		{
			viewer.Replace(_T("%1"), _T("\"")+file+_T("\""));
		}
		else
		{
			viewer += _T(" ");
			viewer += _T("\"") + file + _T("\"");
		}
		if (viewer.Find(_T("%path")) >= 0)
		{
			viewer.Replace(_T("%path"), _T("\"") + dir + _T("\""));
		}
	} // if ((PathIsDirectory(dir)) || (dir.IsEmpty())) 
	else
	{
		// not a unified diff
		CRegString diffexe(_T("Software\\TortoiseSVN\\Diff"));
		viewer = diffexe;
		if (ext != "")
		{
			// is there an extension specific diff tool?
			CRegString difftool(_T("Software\\TortoiseSVN\\DiffTools\\") + ext.MakeLower());
			if (CString(difftool) != "")
				viewer = difftool;
		}
		if (viewer.IsEmpty()||(viewer.Left(1).Compare(_T("#"))==0))
		{
			//no registry entry for a diff program
			//use TortoiseMerge
			CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
			viewer = tortoiseMergePath;
			if (viewer.IsEmpty())
			{
				TCHAR tmerge[MAX_PATH];
				GetModuleFileName(NULL, tmerge, MAX_PATH);
				viewer = tmerge;
				viewer.Replace(_T("TortoiseProc.exe"), _T("TortoiseMerge.exe"));
			}
			viewer = _T("\"") + viewer + _T("\"");
			viewer = viewer + _T(" /base:%base /yours:%mine /basename:%bname /yoursname:%yname");
		} // if (diffexe == "")
		if (viewer.Find(_T("%base")) >= 0)
		{
			viewer.Replace(_T("%base"),  _T("\"")+file+_T("\""));
		}
		else
		{
			viewer += _T(" ");
			viewer += _T("\"") +file + _T("\"");
		}
		if (viewer.Find(_T("%mine")) >= 0)
		{
			viewer.Replace(_T("%mine"),  _T("\"")+dir+_T("\""));
		}
		else
		{
			viewer += _T(" ");
			viewer += _T("\"") + dir + _T("\"");
		}
		viewer.Replace(_T("%bname"), _T("\"") + name1 + _T("\""));
		viewer.Replace(_T("%yname"), _T("\"") + name2 + _T("\""));
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
	TCHAR buf[32*1024];
	ExpandEnvironmentStrings(viewer, buf, MAX_PATH);
	viewer = buf;
	ExpandEnvironmentStrings(file, buf, MAX_PATH);
	file = buf;
	file = _T("\"")+file+_T("\"");
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
		CString sFilter;
		sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy (pszFilters, sFilter);
		// Replace '|' delimeters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		} // while (ptr != pszFilters) 
		ofn.lpstrFilter = pszFilters;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		CString temp;
		temp.LoadString(IDS_UTILS_SELECTTEXTVIEWER);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			delete [] pszFilters;
			viewer = CString(ofn.lpstrFile);
		} // if (GetOpenFileName(&ofn)==TRUE)
		else
		{
			delete [] pszFilters;
			return FALSE;
		}
	}
	if (viewer.Find(_T("%1")) >= 0)
	{
		viewer.Replace(_T("%1"),  _T("\"")+file+_T("\""));
		viewer.Replace(_T("\"\""), _T("\""));
	}
	else
	{
		viewer += _T(" ");
		viewer += _T("\"")+file+_T("\"");
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

CStringA CUtils::PathEscape(CStringA path)
{
	CStringA ret = path;
	ret.Replace((" "), ("%20"));
	ret.Replace(("^"), ("%5E"));
	ret.Replace(("&"), ("%26"));
	ret.Replace(("`"), ("%60"));
	ret.Replace(("{"), ("%7B"));
	ret.Replace(("}"), ("%7D"));
	ret.Replace(("|"), ("%7C"));
	ret.Replace(("]"), ("%5D"));
	ret.Replace(("["), ("%5B"));
	ret.Replace(("\""), ("%22"));
	ret.Replace(("<"), ("%3C"));
	ret.Replace((">"), ("%3E"));
	ret.Replace(("\\"), ("%5C"));
	ret.Replace(("#"), ("%23"));
	ret.Replace(("?"), ("%3F"));
	return ret;
}

BOOL CUtils::IsEscaped(CStringA path)
{
	if (path.Find("%20")>=0)
		return TRUE;
	if (path.Find("%%5E")>=0)
		return TRUE;
	if (path.Find("%26")>=0)
		return TRUE;
	if (path.Find("%60")>=0)
		return TRUE;
	if (path.Find("%7B")>=0)
		return TRUE;
	if (path.Find("%7D")>=0)
		return TRUE;
	if (path.Find("%7C")>=0)
		return TRUE;
	if (path.Find("%5D")>=0)
		return TRUE;
	if (path.Find("%5B")>=0)
		return TRUE;
	if (path.Find("%22")>=0)
		return TRUE;
	if (path.Find("%3C")>=0)
		return TRUE;
	if (path.Find("%3E")>=0)
		return TRUE;
	if (path.Find("%5C")>=0)
		return TRUE;
	if (path.Find("%23")>=0)
		return TRUE;
	if (path.Find("%3F")>=0)
		return TRUE;
	return FALSE;
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

CString CUtils::GetFileNameFromPath(CString sPath)
{
	CString ret;
	sPath.Replace(_T("/"), _T("\\"));
	ret = sPath.Mid(sPath.ReverseFind('\\') + 1);
	return ret;
}

CString CUtils::GetFileExtFromPath(CString sPath)
{
	CString filename = GetFileNameFromPath(sPath);
	int pos = filename.ReverseFind('.');
	if (pos >= 0)
		return filename.Mid(pos);
	return _T("");
}

BOOL CUtils::PathIsParent(CString sPath1, CString sPath2)
{
	sPath1.Replace('\\', '/');
	sPath2.Replace('\\', '/');
	if (sPath1.Right(1).Compare(_T("/"))==0)
		sPath1 = sPath1.Left(sPath1.GetLength()-1);
	int pos = 0;
	do
	{
		pos = sPath2.ReverseFind('/');
		if (pos >= 0)
			sPath2 = sPath2.Left(pos);
		if (sPath2.CompareNoCase(sPath1)==0)
			return TRUE;
	} while (pos >= 0);
	return FALSE;
}

CString CUtils::WritePathsToTempFile(CString paths)
{
	CString tempfile = CUtils::GetTempFile();
	try
	{
		CStdioFile file(tempfile, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
		int pos = -1;
		CString temp;
		do
		{
			pos = paths.Find('*');
			if (pos>=0)
				temp = paths.Left(pos);
			else
				temp = paths;
			temp = CUtils::GetLongPathName(temp);
			file.WriteString(temp + _T("\n"));
			paths = paths.Mid(pos+1);
		} while (pos >= 0);
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE(_T("CFileException in Commit!\n"));
		pE->Delete();
	}
	return tempfile;
}

CString CUtils::GetLongPathName(CString path)
{
	TCHAR pathbuf[MAX_PATH];
	DWORD ret = ::GetLongPathName(path, pathbuf, MAX_PATH);
	if ((ret == 0)||(ret > MAX_PATH))
		return path;
	return CString(pathbuf);
}

BOOL CUtils::FileCopy(CString srcPath, CString destPath, BOOL force)
{
	srcPath.Replace('/', '\\');
	destPath.Replace('/', '\\');
	// now make sure that the destination directory exists
	int ind = 0;
	while (destPath.Find('\\', ind)>=0)
	{
		if (!PathIsDirectory(destPath.Left(destPath.Find('\\', ind))))
		{
			if (!CreateDirectory(destPath.Left(destPath.Find('\\', ind)), NULL))
				return FALSE;
		}
		ind = destPath.Find('\\', ind)+1;
	}
	if (PathIsDirectory(srcPath))
	{
		if (!PathIsDirectory(destPath))
			return CreateDirectory(destPath, NULL);
		return TRUE;
	}
	return (CopyFile(srcPath, destPath, !force));
}
