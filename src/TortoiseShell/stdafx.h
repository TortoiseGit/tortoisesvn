// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include <SDKDDKVer.h>

#define ISOLATION_AWARE_ENABLED 1
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#include <windows.h>

#include <commctrl.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <wininet.h>
#include <Aclapi.h>

#include <atlbase.h>
#include <atlexcept.h>
#include <atlstr.h>

#include <string>
#include <set>
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
#include "svn_subst.h"
#include "svn_props.h"
#pragma warning(pop)

#include "SysInfo.h"
#include "DebugOutput.h"

#define CSTRING_AVAILABLE
