// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - Will Dean, Stefan Kueng

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

#include "stdafx.h"
#include "SVNStatusCache.h"
#include "CacheInterface.h"
#include "..\crashrpt\CrashReport.h"

#include "..\version.h"

#define BUFSIZE 4096

CCrashReport crasher("crashreports@tortoisesvn.tigris.org", "Crash Report for TSVNCache : " STRPRODUCTVER);// crash


VOID				InstanceThread(LPVOID); 
VOID				PipeThread(LPVOID);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
bool bRun = true;
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*cmdShow*/)
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

	DWORD dwThreadId; 
	HANDLE hPipeThread; 
	MSG msg;
	TCHAR szWindowClass[] = {_T("TSVNCacheWindow")};

	// create a hidden window to receive window messages.
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= 0;
	wcex.hCursor		= 0;
	wcex.hbrBackground	= 0;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= 0;
	RegisterClassEx(&wcex);
	HWND hWnd = CreateWindow(_T("TSVNCacheWindow"), _T("TSVNCacheWindow"), WS_CAPTION, 0, 0, 0, 0, NULL, 0, hInstance, 0);
	msg.hwnd = hWnd;	
	
	// Create a thread which waits for incoming pipe connections 
	hPipeThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		(LPTHREAD_START_ROUTINE) PipeThread, 
		(LPVOID) &bRun,    // thread parameter 
		0,                 // not suspended 
		&dwThreadId);      // returns thread ID 

	if (hPipeThread == NULL) 
	{
		printf("CreateThread failed"); 
		return 0;
	}
	else CloseHandle(hPipeThread); 

	// loop to handle window messages.
	while (bRun)
	{
		GetMessage(&msg, NULL, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CSVNStatusCache::Destroy();
	apr_terminate();

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_ENDSESSION:
	case WM_DESTROY:
	case WM_QUIT:
	case WM_CLOSE:
		{
			bRun = false;
			// now connect to the pipe so that the running thread can exit
			HANDLE hPipe = CreateFile( 
				TSVN_CACHE_PIPE_NAME,   // pipe name 
				GENERIC_READ |  // read and write access 
				GENERIC_WRITE, 
				0,              // no sharing 
				NULL,           // default security attributes
				OPEN_EXISTING,  // opens existing pipe 
				0,              // default attributes 
				NULL);          // no template file 

			CloseHandle(hPipe);
			Sleep(100);
			PostQuitMessage(0);
			ATLTRACE("WM_CLOSE/QUIT/DESTROY/ENDSESSION\n");
			return 0;
		}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////

VOID GetAnswerToRequest(const TSVNCacheRequest* pRequest, TSVNCacheResponse* pReply, DWORD* pResponseLength)
{
	CTSVNPath path;
	if(pRequest->flags & TSVNCACHE_FLAGS_FOLDERISKNOWN)
	{
		path.SetFromWin(pRequest->path, !!(pRequest->flags & TSVNCACHE_FLAGS_ISFOLDER));
	}
	else
	{
		path.SetFromWin(pRequest->path);
	}

	CSVNStatusCache::Instance().GetStatusForPath(path, pRequest->flags).BuildCacheResponse(*pReply, *pResponseLength);
}

VOID PipeThread(LPVOID lpvParam)
{
	bool * bRun = (bool *)lpvParam;
	// The main loop creates an instance of the named pipe and 
	// then waits for a client to connect to it. When the client 
	// connects, a thread is created to handle communications 
	// with that client, and the loop is repeated. 
	DWORD dwThreadId; 
	BOOL fConnected;
	HANDLE hPipe, hInstanceThread;

	while (*bRun) 
	{ 
		hPipe = CreateNamedPipe( 
			TSVN_CACHE_PIPE_NAME,
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

		if (WaitNamedPipe(TSVN_CACHE_PIPE_NAME, 100))
		{
			// Wait for the client to connect; if it succeeds, 
			// the function returns a nonzero value. If the function returns 
			// zero, GetLastError returns ERROR_PIPE_CONNECTED. 
			fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
			if (fConnected) 
			{ 
				// Create a thread for this client. 
				hInstanceThread = CreateThread( 
					NULL,              // no security attribute 
					0,                 // default stack size 
					(LPTHREAD_START_ROUTINE) InstanceThread, 
					(LPVOID) hPipe,    // thread parameter 
					0,                 // not suspended 
					&dwThreadId);      // returns thread ID 

				if (hInstanceThread == NULL) 
				{
					printf("CreateThread failed"); 
					return;
				}
				else CloseHandle(hInstanceThread); 
			} 
			else 
				// The client could not connect, so close the pipe. 
				CloseHandle(hPipe); 
		}
	}
	ATLTRACE("Pipe thread exited\n");
}

VOID InstanceThread(LPVOID lpvParam) 
{ 
	TSVNCacheResponse response; 
	DWORD cbBytesRead, cbWritten; 
	BOOL fSuccess; 
	HANDLE hPipe; 

	// The thread's parameter is a handle to a pipe instance. 

	hPipe = (HANDLE) lpvParam; 

	while (bRun) 
	{ 
		// Read client requests from the pipe. 
		TSVNCacheRequest request;
		fSuccess = ReadFile( 
			hPipe,        // handle to pipe 
			&request,    // buffer to receive data 
			sizeof(request), // size of buffer 
			&cbBytesRead, // number of bytes read 
			NULL);        // not overlapped I/O 

		if (! fSuccess || cbBytesRead == 0) 
			break; 

		DWORD responseLength;
		GetAnswerToRequest(&request, &response, &responseLength); 

		// Write the reply to the pipe. 
		fSuccess = WriteFile( 
			hPipe,        // handle to pipe 
			&response,      // buffer to write from 
			responseLength, // number of bytes to write 
			&cbWritten,   // number of bytes written 
			NULL);        // not overlapped I/O 

		if (! fSuccess || responseLength != cbWritten) break; 
	} 

	// Flush the pipe to allow the client to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 
	ATLTRACE("Instance thread exited\n");
}

