#include "StdAfx.h"
#include "debughelpers.h"

CString GetLastErrorMessageString(int err)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	CString temp = CString((LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return temp;
};

CString GetLastErrorMessageString()
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	CString temp = CString((LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return temp;
};

void SetThreadName(DWORD dwThreadID, LPCTSTR szThreadName)
{
#ifdef _UNICODE
	char narrow[_MAX_PATH * 3];
	BOOL defaultCharUsed;
	int ret = WideCharToMultiByte(CP_ACP, 0, szThreadName, (int)_tcslen(szThreadName), narrow, _MAX_PATH*3 - 1, ".", &defaultCharUsed);
	narrow[ret] = 0;
#endif
	THREADNAME_INFO info;
	info.dwType = 0x1000;
#ifdef _UNICODE
	info.szName = narrow;
#else
	info.szName = szThreadName;
#endif
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD),
			(DWORD *)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

