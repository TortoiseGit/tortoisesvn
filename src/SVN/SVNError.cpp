#include "StdAfx.h"
#include "SVNError.h"

///////////////////////////////////////////////////////////////
// construction
///////////////////////////////////////////////////////////////

SVNError::SVNError (svn_errno_t code, const CStringA& message)
	: std::exception()
	, code (code)
	, message (message)
{
}

SVNError::SVNError (svn_error_t* error)
	: code (static_cast<svn_errno_t>(error->apr_err))
    , message (error->message != NULL ? error->message : error->file)
{
}
