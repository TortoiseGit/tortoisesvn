#include "StdAfx.h"
#include "Remotecachelink.h"
#include "ShellExt.h"

CRemoteCacheLink::CRemoteCacheLink(void) :
	m_hPipe(INVALID_HANDLE_VALUE)
{
	ZeroMemory(&m_dummyStatus, sizeof(m_dummyStatus));
	m_dummyStatus.text_status = svn_wc_status_none;
	m_dummyStatus.prop_status = svn_wc_status_none;
	m_dummyStatus.repos_text_status = svn_wc_status_none;
	m_dummyStatus.repos_prop_status = svn_wc_status_none;

	m_critSec.Init();
}

CRemoteCacheLink::~CRemoteCacheLink(void)
{
	m_critSec.Term();
}



bool CRemoteCacheLink::EnsurePipeOpen()
{
	AutoLocker lock(m_critSec);
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		return true;
	}

	m_hPipe = CreateFile( 
		_T("\\\\.\\pipe\\TSVNCache"),   // pipe name 
		GENERIC_READ |  // read and write access 
		GENERIC_WRITE, 
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 


	if (m_hPipe != INVALID_HANDLE_VALUE) 
	{
		// The pipe connected; change to message-read mode. 
		DWORD dwMode; 

		dwMode = PIPE_READMODE_MESSAGE; 
		if(!SetNamedPipeHandleState( 
			m_hPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL))    // don't set maximum time 
		{
			ATLTRACE("SetNamedPipeHandleState failed"); 
			CloseHandle(m_hPipe);
			m_hPipe = INVALID_HANDLE_VALUE;
			return false;
		}

		return true;
	}

	return false;
}

bool CRemoteCacheLink::GetStatusFromRemoteCache(LPCTSTR pPath, svn_wc_status_t* pReturnedStatus)
{
	if(!EnsurePipeOpen())
	{
		// We've failed to open the pipe - try and start the cache
		STARTUPINFO startup;
		PROCESS_INFORMATION process;
		memset(&startup, 0, sizeof(startup));
		startup.cb = sizeof(startup);
		memset(&process, 0, sizeof(process));

		CRegStdString cachePath(_T("Software\\TortoiseSVN\\CachePath"), _T("TSVNCache.exe"), false, HKEY_LOCAL_MACHINE);
		if (CreateProcess(cachePath, _T(""), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
		{
			// It's not appropriate to do a message box here, because there may be hundreds of calls
			ATLTRACE("Failed to start cache\n");
		} 

		// Wait for the cache to open
		long endTime = (long)GetTickCount()+1000;
		while(!EnsurePipeOpen())
		{
			if(((long)GetTickCount() - endTime) > 0)
			{
				return false;
			}
		}
	}

	// Send a message to the pipe server. 
	DWORD nBytesWritten;
	if (!WriteFile(m_hPipe,pPath,(lstrlen(pPath)+1)*sizeof(TCHAR),&nBytesWritten,NULL)) 
	{
		OutputDebugStringA("Pipe WriteFile failed\n"); 
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
		return false;
	}

	BOOL bSuccess;
	do 
	{ 
		// Read from the pipe. 
		DWORD nBytesRead; 
		bSuccess = ReadFile(m_hPipe,pReturnedStatus,sizeof(svn_wc_status_t),&nBytesRead,NULL);
		if (!bSuccess && GetLastError() != ERROR_MORE_DATA) 
		{
			OutputDebugStringA("Pipe ReadFile failed\n");
			CloseHandle(m_hPipe);
			m_hPipe = INVALID_HANDLE_VALUE;
			break; 
		}
	} while (!bSuccess);

	if(!bSuccess)
	{
		OutputDebugStringA("Pipe GSRC failed\n");
	}

	return !!bSuccess;
}
