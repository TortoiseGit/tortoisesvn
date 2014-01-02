// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2014 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

// Exclude rarely-used stuff from Windows headers

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#define NOSERVICE
#define NOMCX
#define NOIME
#define NOSOUND
#define NOCOMM
#define NORPC
#define NOMINMAX

// streamline APIs

#ifdef _UNICODE
#define UNICODE_ONLY
#else
#define ANSI_ONLY
#endif

// include commonly used headers

#include <afxwin.h>         // MFC core and standard components
#include <afxtempl.h>       // CArray and friends

#include <assert.h>
#include <time.h>

#include <algorithm>

#include <string>
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <memory>
#include <algorithm>

// Commonly used log cache streams
// (others may be included locally)

#include "../Streams/DiffIntegerInStream.h"
#include "../Streams/DiffIntegerOutStream.h"
#include "../Streams/PackedTime64InStream.h"
#include "../Streams/PackedTime64OutStream.h"
#include "../Streams/CompositeInStream.h"
#include "../Streams/CompositeOutStream.h"
