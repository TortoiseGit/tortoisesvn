// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>


// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <shlwapi.h>
#include <richedit.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <map>
#include <fstream>

#ifdef _WIN64
#   define APP_X64_STRING   "x64"
#else
#   define APP_X64_STRING   ""
#endif
