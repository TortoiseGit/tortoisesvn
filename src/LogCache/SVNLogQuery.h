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

#include "ILogQuery.h"
#include "svn_client.h"

class CSVNLogQuery : public ILogQuery
{
private:

	// our client context info

	svn_client_ctx_t *context;

	// the memory pool to use

	apr_pool_t *pool;

	// SVN callback. Route data to receiver

	static svn_error_t* LogReceiver ( void* baton
									, apr_hash_t* ch_paths
									, svn_revnum_t revision
									, const char* author
									, const char* date
									, const char* msg
									, apr_pool_t* pool);

public:

	// construction / destruction

	CSVNLogQuery ( svn_client_ctx_t *context
				 , apr_pool_t *pool);
	virtual ~CSVNLogQuery(void);

	// query a section from log for multiple paths
	// (special revisions, like "HEAD", supported)

	virtual void Log ( const CTSVNPathList& targets
					 , const SVNRev& peg_revision
					 , const SVNRev& start
					 , const SVNRev& end
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver);
};
