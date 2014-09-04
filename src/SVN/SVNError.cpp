// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010, 2014 - TortoiseSVN

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
#include "stdafx.h"
#include "SVNError.h"
#include "FormatMessageWrapper.h"

///////////////////////////////////////////////////////////////
// construction
///////////////////////////////////////////////////////////////

SVNError::SVNError (svn_errno_t errcode, const CStringA& errmessage)
    : std::exception()
    , code (errcode)
    , message (errmessage)
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

///////////////////////////////////////////////////////////////
// frequently used
///////////////////////////////////////////////////////////////

void SVNError::ThrowLastError (DWORD lastError)
{
    // get formatted system error message

    CFormatMessageWrapper errorMessage(lastError);
    CStringA errorText ((LPCTSTR)errorMessage);
    throw SVNError (SVN_ERR_BASE, errorText);
}
