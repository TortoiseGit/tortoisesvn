// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once
#define XMESSAGEBOX_APPREGPATH "Software\\TortoiseSVN\\"

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifdef UNICODE
#	ifndef WINVER
#		define WINVER 0x0501
#	endif
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0501
#	endif						
#	ifndef _WIN32_WINDOWS
#		define _WIN32_WINDOWS 0x0501
#	endif
#else
#	ifndef WINVER
#		define WINVER 0x0410
#	endif
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0500
#	endif						
#	ifndef _WIN32_WINDOWS
#		define _WIN32_WINDOWS 0x0410
#	endif
// some defines which are missing if WINVER < 0x0500
#define MB_CANCELTRYCONTINUE		0x00000006L
#define DFCS_TRANSPARENT			0x0800
#define DFCS_HOT					0x1000
#define IDC_HAND					MAKEINTRESOURCE(32649)
#define IDTRYAGAIN					10
#define IDCONTINUE					11
#endif



#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>

#ifndef UNICODE
#include "multimon.h"
#endif

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif
