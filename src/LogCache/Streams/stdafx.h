// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER
#	define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0501
#endif						
#ifndef _WIN32_WINDOWS
#	define _WIN32_WINDOWS 0x0501
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

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

#include <windows.h>
#include <assert.h>

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
