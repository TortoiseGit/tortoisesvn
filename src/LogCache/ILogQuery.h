// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CTSVNPathList;
class SVNRev;

class ILogReceiver;

/**
 * A container type for (user-defined) revision property names.
 */

typedef std::vector<CString> TRevPropNames;

/**
 * Interface for log queries. It mimics svn_client_log4.
 *
 * Errors are reported by throwing SVNError exceptions.
 */

class ILogQuery
{
public:

    /** query a section from log for multiple paths
     * (special revisions, like "HEAD", supported)
     *
     * userRevProps will be ignored for if includeUserRevProps
     * is not set. If it is set and userRevProps is empty
     * all user-defined revprops will be returned.
     *
     * userRevProps must not overlap with the standard revprops
     * (svn:log, svn:date and svn:author).
     */

    virtual void Log ( const CTSVNPathList& targets
                     , const SVNRev& peg_revision
                     , const SVNRev& start
                     , const SVNRev& end
                     , int limit
                     , bool strictNodeHistory
                     , ILogReceiver* receiver
                     , bool includeChanges
                     , bool includeMerges
                     , bool includeStandardRevProps
                     , bool includeUserRevProps
                     , const TRevPropNames& userRevProps) = 0;
};
