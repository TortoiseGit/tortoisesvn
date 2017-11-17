// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once
#define XMESSAGEBOX_APPREGPATH "Software\\TortoiseSVN\\"

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>

#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>
#include <afxctl.h>
#include <afxtempl.h>
#include <afxmt.h>
#include <afxext.h>         // MFC extensions
#include <afxcontrolbars.h>     // MFC support for ribbons and control bars
#include <afxtaskdialog.h>

#include <atlbase.h>

#pragma warning(push)
// allow use of experimental svn functions (such as shelving)
#define SVN_EXPERIMENTAL
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

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <deque>
#include <regex>

#include <vfw.h>
#pragma warning(push)
#pragma warning(disable: 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <shlobj.h>
#pragma warning(pop)
#include <Shlwapi.h>
#include <shlguid.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <dlgs.h>
#include <wininet.h>
#include <assert.h>
#include <math.h>
#pragma warning(push)
#pragma warning(disable: 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>
#pragma warning(pop)
#include <VersionHelpers.h>
// SDKs prior to Win10 don't have the IsWindows10OrGreater API in the versionhelpers header, so
// we define it here just in case:
#ifndef _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10 0x0A00
#define _WIN32_WINNT_WINTHRESHOLD 0x0A00
#define  IsWindows10OrGreater() (IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), 0))
#endif

#include "apr_version.h"
#include "apu_version.h"
#include "../../ext/zlib/zlib.h"

#define __WIN32__
#include "boost/pool/object_pool.hpp"

#define USE_GDI_GRADIENT
#define HISTORYCOMBO_WITH_SYSIMAGELIST

#include "ProfilingInfo.h"
#include "DebugOutput.h"
#include "CrashReport.h"
#include "OnOutOfScope.h"

#ifdef _WIN64
#   define APP_X64_STRING   "x64"
#else
#   define APP_X64_STRING ""
#endif

#define HAVE_APPUTILS
