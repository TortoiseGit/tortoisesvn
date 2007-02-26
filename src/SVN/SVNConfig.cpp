// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Tim Kemp and Stefan Kueng

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

#include "StdAfx.h"
#include "SVNConfig.h"
#include "UnicodeUtils.h"

SVNConfig::SVNConfig(void)
{
	svn_error_t * err;
	memset (&ctx, 0, sizeof (ctx));
	parentpool = svn_pool_create(NULL);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", parentpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", parentpool);

	err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	if (err == 0)
		err = svn_config_get_config (&(ctx.config), g_pConfigDir, pool);

	patterns = NULL;

	if (err != 0)
	{
		svn_pool_destroy (pool);
		svn_pool_destroy (parentpool);
		exit(-1);
	}
}

SVNConfig::~SVNConfig(void)
{
	svn_pool_destroy (pool);
	svn_pool_destroy (parentpool);
}

BOOL SVNConfig::GetDefaultIgnores()
{
	svn_error_t * err;
	patterns = NULL;
	err = svn_wc_get_default_ignores (&(patterns), ctx.config, pool);
	if (err)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SVNConfig::MatchIgnorePattern(const CString& name)
{
	if (patterns == NULL)
		return FALSE;
	return svn_wc_match_ignore_list(CUnicodeUtils::GetUTF8(name), patterns, pool);
}
