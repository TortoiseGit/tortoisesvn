// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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

#pragma once
#include "atlsimpstr.h"
#include "afxtempl.h"

#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_sorts.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_string.h"
#include "svn_config.h"
#include "svn_time.h"
#include "svn_subst.h"

class SVNConfig
{
public:
	SVNConfig(void);
	~SVNConfig(void);

	BOOL	MatchIgnorePattern(CString sFilepath);
private:
	apr_pool_t *				parentpool;
	apr_pool_t *				pool;			///< memory pool
	svn_client_ctx_t 			ctx;

};
