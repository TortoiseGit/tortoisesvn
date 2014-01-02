// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxcoll.h>        // MFC Collection templates and classes
#include <shlwapi.h>        // Shell API

#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#include <afxdlgs.h>
#include <afxctl.h>
#include <afxext.h>             // MFC extensions
#include <afxcontrolbars.h>     // MFC support for ribbons and control bars
#include <afxtaskdialog.h>

#include <vfw.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

#pragma warning(push)
#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_config.h"
#include "svn_error_codes.h"
#include "svn_subst.h"
#include "svn_repos.h"
#include "svn_time.h"
#include "svn_props.h"
#pragma warning(pop)

#define USE_GDI_GRADIENT

#define XMESSAGEBOX_APPREGPATH "Software\\TortoiseMerge\\"

#include "ProfilingInfo.h"
#include "DebugOutput.h"
#include "CrashReport.h"

#ifdef _WIN64
#   define APP_X64_STRING   "x64"
#else
#   define APP_X64_STRING   ""
#endif
