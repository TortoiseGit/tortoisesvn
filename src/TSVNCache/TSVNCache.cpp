// TSVNCache.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SVNStatusCache.h"
#include <conio.h>

#define BUFSIZE 4096

VOID InstanceThread(LPVOID); 
VOID GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD); 

int __stdcall WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*cmdShow*/)
{
	HANDLE hReloadProtection = ::CreateSemaphore(NULL, 0, 1, _T("TSVNCacheReloadProtection"));
	if(hReloadProtection != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// An instance of TSVNCache is already running
		ATLTRACE("TSVNCache ignoring restart\n");
		return 0;
	}

	apr_initialize();
	CSVNStatusCache::Create();

	BOOL fConnected; 
	DWORD dwThreadId; 
	HANDLE hPipe, hThread; 
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\TSVNCache"); 

	// The main loop creates an instance of the named pipe and 
	// then waits for a client to connect to it. When the client 
	// connects, a thread is created to handle communications 
	// with that client, and the loop is repeated. 

	for (;;) 
	{ 
		hPipe = CreateNamedPipe( 
			lpszPipename,             // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_TYPE_MESSAGE |       // message type pipe 
			PIPE_READMODE_MESSAGE |   // message-read mode 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			BUFSIZE,                  // output buffer size 
			BUFSIZE,                  // input buffer size 
			NMPWAIT_USE_DEFAULT_WAIT, // client time-out 
			NULL);                    // default security attribute 

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			printf("CreatePipe failed"); 
			break;
		}

		// Wait for the client to connect; if it succeeds, 
		// the function returns a nonzero value. If the function returns 
		// zero, GetLastError returns ERROR_PIPE_CONNECTED. 

		fConnected = ConnectNamedPipe(hPipe, NULL) ? 
TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

		if (fConnected) 
		{ 
			// Create a thread for this client. 
			hThread = CreateThread( 
				NULL,              // no security attribute 
				0,                 // default stack size 
				(LPTHREAD_START_ROUTINE) InstanceThread, 
				(LPVOID) hPipe,    // thread parameter 
				0,                 // not suspended 
				&dwThreadId);      // returns thread ID 

			if (hThread == NULL) 
			{
				printf("CreateThread failed"); 
				return 0;
			}
			else CloseHandle(hThread); 
		} 
		else 
			// The client could not connect, so close the pipe. 
			CloseHandle(hPipe); 
	} 


	CSVNStatusCache::Destroy();
	apr_terminate();

	return 0;
}


VOID InstanceThread(LPVOID lpvParam) 
{ 
	TCHAR chRequest[BUFSIZE]; 
	TCHAR chReply[BUFSIZE]; 
	DWORD cbBytesRead, cbReplyBytes, cbWritten; 
	BOOL fSuccess; 
	HANDLE hPipe; 

	// The thread's parameter is a handle to a pipe instance. 

	hPipe = (HANDLE) lpvParam; 

	while (1) 
	{ 
		// Read client requests from the pipe. 
		fSuccess = ReadFile( 
			hPipe,        // handle to pipe 
			chRequest,    // buffer to receive data 
			BUFSIZE*sizeof(TCHAR), // size of buffer 
			&cbBytesRead, // number of bytes read 
			NULL);        // not overlapped I/O 

		if (! fSuccess || cbBytesRead == 0) 
			break; 
		GetAnswerToRequest(chRequest, chReply, &cbReplyBytes); 

		// Write the reply to the pipe. 
		fSuccess = WriteFile( 
			hPipe,        // handle to pipe 
			chReply,      // buffer to write from 
			cbReplyBytes, // number of bytes to write 
			&cbWritten,   // number of bytes written 
			NULL);        // not overlapped I/O 

		if (! fSuccess || cbReplyBytes != cbWritten) break; 
	} 

	// Flush the pipe to allow the client to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 
}

VOID GetAnswerToRequest(LPTSTR chRequest, 
						LPTSTR chReply, LPDWORD pchBytes)
{

//	_cprintf("Requested: %ws\n", chRequest);

	CTSVNPath path;
	path.SetFromWin(chRequest);
	const svn_wc_status_t* pStatus = CSVNStatusCache::Instance().GetStatusForPath(path).Status();

	memcpy(chReply, pStatus, sizeof(svn_wc_status_t));
	*pchBytes = sizeof(svn_wc_status_t);
}
