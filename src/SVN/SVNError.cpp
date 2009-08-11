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

SVNError::SVNError (const svn_error_t* error)
    : code (error ? static_cast<svn_errno_t>(error->apr_err) : (svn_errno_t)0)
    , message (error ? (error->message != NULL ? error->message : error->file) : 0)
{
}

SVNError::SVNError (svn_error_t* error)
    : code (error ? static_cast<svn_errno_t>(error->apr_err) : (svn_errno_t)0)
    , message (error ? (error->message != NULL ? error->message : error->file) : 0)
{
    svn_error_clear (error);
}
