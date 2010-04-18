// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2008-2009 - TortoiseSVN

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
#pragma once

#pragma warning(push)
#include "svn_pools.h"
#include "svn_wc.h"
#pragma warning(pop)

/**
 * \ingroup SVN
 * This class implements one global object, handling the Subversion admin
 * directory and functions used with that.
 *
 * The global object is g_SVNAdminDir, defined as an external in this header
 * file, and declared once in the cpp file.
 *
 * When the global object is created, apr is initialized, a pool is created
 * and the Subversion admin directory name is set. That name then stays the same
 * as long as the object exists.
 *
 * The name of the admin dir is determined by reading the environment variable
 * SVN_ASP_DOT_NET_HACK. If that variable exists, the admin dir name is set
 * to "_svn", otherwise it is the default ".svn".
 *
 * Since some applications (like TortoiseProc.exe) clear *all* pools at the end,
 * this object has a Close() method to make it clean up the pool at a defined
 * time. Otherwise it will clean up the pool when the object is destroyed, which
 * isn't done at a defined time for global objects.
 */
class SVNAdminDir
{
private:
    SVNAdminDir(const SVNAdminDir&){}
    SVNAdminDir& operator=(SVNAdminDir&){};
public:
    SVNAdminDir();
    ~SVNAdminDir();
    /**
     * Initializes the global object. Call this after apr is initialized but
     * before using any other methods of this class.
     */
    bool Init();
    /**
     * Clears the memory pool. Call this before you clear *all* pools
     * with apr_pool_terminate(). If you don't use apr_pool_terminate(), then
     * this method doesn't need to be called, because the deconstructor will
     * do the same too.
     */
    bool Close();

    /// Returns true if \a name is the admin dir name
    bool IsAdminDirName(const CString& name) const;

    /// Returns true if the path points to or below an admin directory
    bool IsAdminDirPath(const CString& path) const;

    /// Returns true if the path (file or folder) has an admin directory
    /// associated, i.e. if the path is in a working copy.
    bool HasAdminDir(const CString& path) const;
    bool HasAdminDir(const CString& path, bool bDir) const;

    /// Returns true if the admin dir name is set to "_svn".
    bool IsVSNETHackActive() const {return m_bVSNETHack;}

    CString GetAdminDirName() const {return m_bVSNETHack ? _T("_svn") : _T(".svn");}
private:
    apr_pool_t* m_pool;
    bool m_bVSNETHack;
    int m_nInit;
};

extern SVNAdminDir g_SVNAdminDir;
