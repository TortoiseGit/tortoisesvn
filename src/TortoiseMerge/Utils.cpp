#include "StdAfx.h"
#include "Registry.h"
#include ".\utils.h"

CUtils::CUtils(void)
{
}

CUtils::~CUtils(void)
{
}

BOOL CUtils::GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CProgressDlg * progDlg, HWND hWnd /*=NULL*/)
{
	CString sSCMPath = CRegString(_T("TortoiseMerge\\SCMPath"),
		_T("F:\\Development\\SVN\\TortoiseSVN\\bin\\Release\\TortoiseProc.exe /command:cat /path:\"%1\" /revision:%2 /savepath:\"%3\" /hwnd:%4"));
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
	} // if (CreateProcess(NULL /*(LPCTSTR)diffpath*/, const_cast<TCHAR*>((LPCTSTR)com), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	DWORD ret = 0;
	DWORD i = 0;
	do
	{
		ret = WaitForSingleObject(process.hProcess, 100);
		progDlg->SetProgress(i++, (DWORD)10000);
	} while ((ret == WAIT_TIMEOUT) && (!progDlg->HasUserCancelled()));
	
	if (progDlg->HasUserCancelled())
	{
		return FALSE;
	}
	if (!PathFileExists(sSavePath))
		return FALSE;
	return TRUE;
}








