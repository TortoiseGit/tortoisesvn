#pragma once

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CTSVNPathList;
class SVNRev;

class ILogReceiver;

///////////////////////////////////////////////////////////////
//
// ILogQuery
//
//		This interface will be implemented by all log 
//		providers (mimics svn_client_log3). 
//
//		Errors will be reported back by SVNError exceptions
//		being thrown.
//
///////////////////////////////////////////////////////////////

class ILogQuery
{
public:

	// query a section from log for multiple paths
	// (special revisions, like "HEAD", supported)

	virtual void Log ( const CTSVNPathList& targets
					 , const SVNRev& peg_revision
					 , const SVNRev& start
					 , const SVNRev& end
					 , int limit
					 , bool strictNodeHistory
					 , ILogReceiver* receiver) = 0;
};
