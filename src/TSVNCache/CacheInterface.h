// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - Will Dean

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#pragma once
#include "wininet.h"

// The name of the named-pipe for the cache
#define TSVN_CACHE_PIPE_NAME _T("\\\\.\\pipe\\TSVNCache")


// A structure passed as a request from the shell (or other client) to the external cache
struct TSVNCacheRequest
{
	DWORD flags;
	WCHAR path[MAX_PATH+1];
};

// The structure returned as a response
struct TSVNCacheResponse
{
	svn_wc_status_t m_status;
	svn_wc_entry_t m_entry;
	char m_url[INTERNET_MAX_URL_LENGTH+1];
};


/// Set this flag if you already know whether or not the item is a folder
#define TSVNCACHE_FLAGS_FOLDERISKNOWN		0x01
/// Set this flag if the item is a folder
#define TSVNCACHE_FLAGS_ISFOLDER			0x02
/// Set this flag if you want recursive folder status (safely ignored for file paths)
#define TSVNCACHE_FLAGS_RECUSIVE_STATUS		0x04

