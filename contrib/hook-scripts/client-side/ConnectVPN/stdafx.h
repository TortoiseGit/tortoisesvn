// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT                // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600         // Change this to the appropriate value to target other versions of Windows.
#endif

#define WIN32_LEAN_AND_MEAN         // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
