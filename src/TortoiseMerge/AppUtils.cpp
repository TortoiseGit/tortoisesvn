// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "Registry.h"
#include "AppUtils.h"
#include "ProgressDlg.h"

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

BOOL CAppUtils::GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CProgressDlg * progDlg, HWND hWnd /*=NULL*/)
{
	CString sSCMPath = CRegString(_T("Software\\TortoiseMerge\\SCMPath"), _T(""));
	if (sSCMPath.IsEmpty())
	{
		// no path set, so use TortoiseSVN as default
		sSCMPath = CRegString(_T("Software\\TortoiseSVN\\ProcPath"), _T(""), false, HKEY_LOCAL_MACHINE);
		if (sSCMPath.IsEmpty())
		{
			TCHAR pszSCMPath[MAX_PATH];
			GetModuleFileName(NULL, pszSCMPath, MAX_PATH);
			sSCMPath = pszSCMPath;
			sSCMPath = sSCMPath.Left(sSCMPath.ReverseFind('\\'));
			sSCMPath += _T("\\TortoiseProc.exe");
		}
		sSCMPath += _T(" /command:cat /path:\"%1\" /revision:%2 /savepath:\"%3\" /hwnd:%4");
	} // if (sSCMPath.IsEmpty()) 
	CString sTemp;
	sTemp.Format(_T("%d"), hWnd);
	sSCMPath.Replace(_T("%1"), sPath);
	sSCMPath.Replace(_T("%2"), sVersion);
	sSCMPath.Replace(_T("%3"), sSavePath);
	sSCMPath.Replace(_T("%4"), sTemp);
	// start the external SCM program to fetch the specific version of the file
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	if (CreateProcess(NULL, (LPTSTR)(LPCTSTR)sSCMPath, NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
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
		MessageBox(NULL, (LPCTSTR)lpMsgBuf, _T("TortoiseMerge"), MB_OK | MB_ICONERROR);
		LocalFree( lpMsgBuf );
	}
	DWORD ret = 0;
	do
	{
		ret = WaitForSingleObject(process.hProcess, 100);
	} while ((ret == WAIT_TIMEOUT) && (!progDlg->HasUserCancelled()));
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	
	if (progDlg->HasUserCancelled())
	{
		return FALSE;
	}
	if (!PathFileExists(sSavePath))
		return FALSE;
	return TRUE;
}







