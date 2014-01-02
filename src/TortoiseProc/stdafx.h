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

#include "MessageBox.h"

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

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <deque>
#include <regex>

#include <vfw.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include <shlguid.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <dlgs.h>
#include <wininet.h>
#include <assert.h>
#include <math.h>
#include <gdiplus.h>

#include "apr_version.h"
#include "apu_version.h"
#ifdef _WIN64
#include "openssl/opensslv.h"
#else
#include "openssl/opensslv.h"
#endif
#include "../../ext/zlib/zlib.h"

#define __WIN32__
#include "boost/pool/object_pool.hpp"

#define USE_GDI_GRADIENT
#define HISTORYCOMBO_WITH_SYSIMAGELIST

#include "ProfilingInfo.h"
#include "DebugOutput.h"
#include "CrashReport.h"

#ifdef _WIN64
#   define APP_X64_STRING   "x64"
#else
#   define APP_X64_STRING ""
#endif

#define HAVE_APPUTILS
