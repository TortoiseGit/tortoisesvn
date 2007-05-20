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
