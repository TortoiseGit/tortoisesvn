// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define NOMINMAX

#define WINVER 0x0600           // Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#define _WIN32_IE 0x0600        // Change this to the appropriate value to target other versions of IE.

#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers

#include <windows.h>

#include <assert.h>
#include <process.h>

#include <vector>
#include <algorithm>

// TODO: reference additional headers your program requires here
