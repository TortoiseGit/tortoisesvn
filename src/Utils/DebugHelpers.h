#pragma once

#ifdef _DEBUG
#	define BEGIN_TICK   { DWORD dwTickMeasureBegin = ::GetTickCount();
#	define END_TICK(s) DWORD dwTickMeasureEnd = ::GetTickCount(); TRACE("%s: tick count = %d\n", s, dwTickMeasureEnd-dwTickMeasureBegin); }
#else
#	define BEGIN_TICK
#	define END_TICK(s)
#endif

/**
 * \ingroup CommonClasses
 * returns the string to the given error number.
 * \param err the error number, obtained with GetLastError() or WSAGetLastError() or ...
 */
CString GetLastErrorMessageString(int err);
/*
 * \ingroup CommonClasses
 * returns the string to the GetLastError() function.
 */
CString GetLastErrorMessageString();

#define MS_VC_EXCEPTION 0x406d1388

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;        // must be 0x1000
	LPCSTR szName;       // pointer to name (in same addr space)
	DWORD dwThreadID;    // thread ID (-1 caller thread)
	DWORD dwFlags;       // reserved for future use, most be zero
} THREADNAME_INFO;

/**
 * Sets a name for a thread. The Thread name must not exceed 9 characters!
 * Inside the current thread you can use -1 for dwThreadID.
 * \param dwThreadID The Thread ID
 * \param szThreadName A name for the thread.
 */   
void SetThreadName(DWORD dwThreadID, LPCTSTR szThreadName);
