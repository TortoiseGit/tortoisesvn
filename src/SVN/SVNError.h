// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010 - TortoiseSVN 

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


/**
 * \ingroup SVN
 * handles svn errors
 */
class SVNError : public std::exception
{
private:

    // parameters required for svn_error_create

    svn_errno_t code;
    CStringA message;

public:

    // construction

    SVNError (svn_errno_t code, const CStringA& message);
    explicit SVNError (svn_error_t* error);
    explicit SVNError (const svn_error_t* error);

    // access internal info

    svn_errno_t GetCode() const;
    const CStringA& GetMessage() const;

    // frequently used

    static void ThrowLastError (DWORD lastError = GetLastError());
};

inline svn_errno_t SVNError::GetCode() const
{
    return code;
}

inline const CStringA& SVNError::GetMessage() const
{
    return message;
}

