// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

// all currently available stream types

enum 
{
	ROOT_STREAM_TYPE_ID             = 0,
	
	COMPOSITE_STREAM_TYPE_ID        = 0x10,

	BLOB_STREAM_TYPE_ID				= 0x20,

	BINARY_STREAM_TYPE_ID			= 0x30,
	PACKED_DWORD_STREAM_TYPE_ID		= 0x31,
	PACKED_INTEGER_STREAM_TYPE_ID	= 0x32,
	DIFF_INTEGER_STREAM_TYPE_ID     = 0x33,
	DIFF_DWORD_STREAM_TYPE_ID       = 0x34,
	PACKED_TIME64_STREAM_TYPE_ID	= 0x35
};

