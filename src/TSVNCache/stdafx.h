// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#include <atlbase.h>
#include <atlstr.h>

#include <conio.h>

#define CSTRING_AVAILABLE


using namespace ATL;

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <deque>

#pragma warning(push)
#include "svn_wc.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_pools.h"
#include "svn_utf.h"
#include "svn_props.h"
#pragma warning(pop)

#include "DebugOutput.h"

typedef CComCritSecLock<CComAutoCriticalSection> AutoLocker;

#ifdef _WIN64
#   define APP_X64_STRING   "x64"
#else
#   define APP_X64_STRING   ""
#endif

#include "ProfilingInfo.h"
#include "CrashReport.h"
