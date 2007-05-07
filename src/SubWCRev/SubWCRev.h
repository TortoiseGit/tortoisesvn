// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#pragma once
#include <vector>

#include <apr_pools.h>
#include "svn_error.h"
#include "svn_client.h"
#include "svn_path.h"

#define URL_BUF	2048

// This structure is used as the status baton for WC crawling
// and contains all the information we are collecting.
typedef struct SubWCRev_t
{
	svn_revnum_t MinRev;	// Lowest update revision found
	svn_revnum_t MaxRev;	// Highest update revision found
	svn_revnum_t CmtRev;	// Highest commit revision found
	apr_time_t CmtDate;		// Date of highest commit revision
	BOOL HasMods;			// True if local modifications found
	BOOL bFolders;			// If TRUE, status of folders is included
	BOOL bExternals;		// If TRUE, status of externals is included
	char Url[URL_BUF];		// URL of working copy
	char UUID[1024];	// The repository UUID of the working copy
} SubWCRev_t;

typedef struct SubWCRev_StatusBaton_t
{
	SubWCRev_t * SubStat;
	std::vector<const char *> * extarray;
	apr_pool_t *pool;
} SubWCRev_StatusBaton_t;

svn_error_t *
svn_status (       const char *path,
                   void *status_baton,
                   svn_boolean_t no_ignore,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

char *Utf16ToUtf8(const WCHAR *pszUtf16, apr_pool_t *pool);

void AutomationMain();
