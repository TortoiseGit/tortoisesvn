// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2010 - TortoiseSVN

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

#pragma warning(push)
#include "svn_client.h"
#include "apr_pools.h"
#pragma warning(pop)
#include "TSVNPath.h"


class SVNPatch
{
public:
    SVNPatch();
    ~SVNPatch();

    /**
     * Does a dry run of the patching, fills in all the arrays.
     * Call this function first.
     * \return the number of files affected by the patchfile, 0 in case of an error
     */
    int                     Init(const CString& patchfile, const CString& targetpath);

    /**
     * Sets the target path. Use this after getting a new path from CheckPatchPath()
     */
    void                    SetTargetPath(const CString& targetpath) { m_targetpath = targetpath; m_targetpath.Replace('\\', '/'); }
    CString                 GetTargetPath() { return m_targetpath; }

    /**
     * Finds the best path to apply the patch file. Starting from the targetpath
     * specified in Init() first upwards, then downwards.
     * \remark this function shows a progress dialog and also shows a dialog asking
     * the user to accept a possible better target path.
     * \return the best path to apply the patch to.
     */
    CString                 CheckPatchPath(const CString& path);

    /**
     * Applies the patch to the file specified in \c sPath and saves the result
     * to \c sSavePath. If \c sSavePath is empty, the patch is applied but the result
     * is not saved, useful to test whether a patch can be applied.
     * \return TRUE if the patch can be applied.
     */
    bool                    PatchFile(const CString& sPath, bool dryrun, CString& sSavePath);

    /**
     * returns the number of files that are affected by the patchfile.
     */
    int                     GetNumberOfFiles() const { return (int)m_filePaths.size(); }

    /**
     * Returns the path of the affected file
     */
    CString                 GetFilePath(int index) const { return m_filePaths[index]; }

    /**
     * Returns the path of the affected file, stripped by m_nStrip.
     */
    CString                 GetStrippedPath(int nIndex) const;

    /**
     * Returns a string containing the last error message.
     */
    CString                 GetErrorMessage() const { return m_errorStr; }

private:
    int                     CountMatches(const CString& path) const;
    int                     CountDirMatches(const CString& path) const;
    /**
     * Strips the filename by removing m_nStrip prefixes.
     */
    CString                 Strip(const CString& filename) const;
    CString                 GetErrorMessage(svn_error_t * Err) const;

    static int              abort_on_pool_failure (int retcode);
    static svn_error_t *    patch_func( void *baton, svn_boolean_t * filtered, const char * canon_path_from_patchfile, 
                                        const char *patch_abspath, 
                                        const char *reject_abspath, 
                                        apr_pool_t * scratch_pool );
    static void             notify(void *baton,
                                   const svn_wc_notify_t *notify,
                                   apr_pool_t *pool);

    apr_pool_t * m_pool;
    std::vector<CString>    m_filePaths;
    int                     m_nStrip;
    bool                    m_bInit;
    bool                    m_bSuccessfullyPatched;
    bool                    m_bDryRun;
    int                     m_nRejected;
    CString                 m_filterPath;
    CString                 m_patchfile;
    CString                 m_targetpath;
    CString                 m_testPath;
    CString                 m_patchedPath;
    CString                 m_errorStr;

};