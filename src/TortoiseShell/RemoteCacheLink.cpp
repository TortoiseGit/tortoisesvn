#include "StdAfx.h"
#include "Remotecachelink.h"
#include "ShellExt.h"
#include "..\TSVNCache\CacheInterface.h"

CRemoteCacheLink::CRemoteCacheLink(void) :
	m_hPipe(INVALID_HANDLE_VALUE)
{
	ZeroMemory(&m_dummyStatus, sizeof(m_dummyStatus));
	m_dummyStatus.text_status = svn_wc_status_none;
	m_dummyStatus.prop_status = svn_wc_status_none;
	m_dummyStatus.repos_text_status = svn_wc_status_none;
	m_dummyStatus.repos_prop_status = svn_wc_status_none;
	m_lastTimeout = 0;
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

bool CRemoteCacheLink::GetStatusFromRemoteCache(LPCTSTR pPath, TSVNCacheResponse* pReturnedStatus, bool bRecursive)
{
	if(!EnsurePipeOpen())
	{
		// We've failed to open the pipe - try and start the cache
		// but only if the last try to start the cache was a certain time
		// ago. If we just try over and over again without a small pause
		// in between, the explorer is rendered unusable!
		// Failing to start the cache can have different reasons: missing exe,
		// missing registry key, corrupt exe, ...
		if (((long)GetTickCount() - m_lastTimeout) < 0)
			return false;
		STARTUPINFO startup;
		PROCESS_INFORMATION process;
		memset(&startup, 0, sizeof(startup));
		startup.cb = sizeof(startup);
		memset(&process, 0, sizeof(process));

		CRegStdString cachePath(_T("Software\\TortoiseSVN\\CachePath"), _T("TSVNCache.exe"), false, HKEY_LOCAL_MACHINE);
		CString sCachePath = cachePath;
		if (CreateProcess(sCachePath.GetBuffer(MAX_PATH), _T(""), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
		{
			// It's not appropriate to do a message box here, because there may be hundreds of calls
			sCachePath.ReleaseBuffer();
			ATLTRACE("Failed to start cache\n");
			return false;
		} 
		sCachePath.ReleaseBuffer();

		// Wait for the cache to open
		long endTime = (long)GetTickCount()+1000;
		while(!EnsurePipeOpen())
		{
			if(((long)GetTickCount() - endTime) > 0)
			{
				m_lastTimeout = (long)GetTickCount()+10000;
				return false;
			}
		}
	}

	// Send a message to the pipe server. 
	DWORD nBytesWritten;
	TSVNCacheRequest request;
	request.flags = 0;
	if(bRecursive)
	{
		request.flags |= TSVNCACHE_FLAGS_RECUSIVE_STATUS;
	}
	wcscpy(request.path, pPath);
	if (!WriteFile(m_hPipe,&request,sizeof(request),&nBytesWritten,NULL)) 
	{
		OutputDebugStringA("Pipe WriteFile failed\n"); 
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
		return false;
	}

	// Read from the pipe. 
	DWORD nBytesRead; 
	if(!ReadFile(m_hPipe,pReturnedStatus,sizeof(*pReturnedStatus),&nBytesRead,NULL))
	{
		OutputDebugStringA("Pipe ReadFile failed\n");
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
		return false;
	}

	if(nBytesRead == sizeof(TSVNCacheResponse))
	{
		// This is a full response - we need to fix-up some pointers
		pReturnedStatus->m_status.entry = &pReturnedStatus->m_entry;
		pReturnedStatus->m_entry.url = pReturnedStatus->m_url;
	}
	else
	{
		pReturnedStatus->m_status.entry = NULL;
	}

	return true;
}
