// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN

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

#ifdef _MFC_VER
#include "SVNPrompt.h"
#endif
#include "TSVNPath.h"
#include "SVNRev.h"
#include "SVNBase.h"

#include <deque>

class SVNConflictData
{
public:
    SVNConflictData();

    svn_wc_conflict_kind_t kind;

    CString             conflict_old;
    CString             conflict_new;
    CString             conflict_wrk;
    CString             prejfile;

    // tree conflict data
    CString             treeconflict_path;
    svn_node_kind_t     treeconflict_nodekind;
    CString             treeconflict_propertyname;
    bool                treeconflict_binary;
    CString             treeconflict_mimetype;
    svn_wc_conflict_action_t treeconflict_action;
    svn_wc_conflict_reason_t treeconflict_reason;
    CString             treeconflict_basefile;
    CString             treeconflict_theirfile;
    CString             treeconflict_myfile;
    CString             treeconflict_mergedfile;
    svn_wc_operation_t  treeconflict_operation;

    CString             src_right_version_url;
    CString             src_right_version_path;
    SVNRev              src_right_version_rev;
    svn_node_kind_t     src_right_version_kind;
    CString             src_left_version_url;
    CString             src_left_version_path;
    SVNRev              src_left_version_rev;
    svn_node_kind_t     src_left_version_kind;
};

/**
 * \ingroup SVN
 * data object which holds all information returned from an svn_client_info() call.
 */
class SVNInfoData
{
public:
    SVNInfoData();

    CTSVNPath           path;
    CString             url;
    SVNRev              rev;
    svn_node_kind_t     kind;
    CString             reposRoot;
    CString             reposUUID;
    SVNRev              lastchangedrev;
    __time64_t          lastchangedtime;
    CString             author;
    CString             wcroot;

    CString             lock_path;
    CString             lock_token;
    CString             lock_owner;
    CString             lock_comment;
    bool                lock_davcomment;
    __time64_t          lock_createtime;
    __time64_t          lock_expirationtime;
    svn_filesize_t      size64;

    bool                hasWCInfo;
    svn_wc_schedule_t   schedule;
    CString             copyfromurl;
    SVNRev              copyfromrev;
    __time64_t          texttime;
    CString             checksum;

    CString             changelist;
    svn_depth_t         depth;
    svn_filesize_t      working_size64;

    CString             moved_to_abspath;
    CString             moved_from_abspath;

    std::deque<SVNConflictData> conflicts;
    // convenience methods:

    bool IsValid() {return rev.IsValid() != FALSE;}

};


/**
 * \ingroup SVN
 * Wrapper for the svn_client_info() API.
 */
class SVNInfo : public SVNBase
{
private:
    SVNInfo(const SVNInfo&);
    SVNInfo& operator=(SVNInfo&);
public:
    SVNInfo(bool suppressUI = false);
    ~SVNInfo(void);

    /**
     * returns the info for the \a path.
     * \param path a path or an url
     * \param pegrev the peg revision to use
     * \param revision the revision to get the info for
     * \param depth how deep to fetch the info
     * \param fetchExcluded if true, also also fetch excluded nodes in the working copy
     * \param fetchActualOnly if true, also fetch nodes that don't exist as versioned but are still tree conflicted
     * for all children of \a path.
     */
    const SVNInfoData * GetFirstFileInfo(const CTSVNPath& path, SVNRev pegrev, SVNRev revision, svn_depth_t depth = svn_depth_empty, bool fetchExcluded  = true , bool fetchActualOnly  = true );
    size_t GetFileCount() const {return m_arInfo.size();}
    /**
     * Returns the info of the next file in the file list. If no more files are in the list then NULL is returned.
     * See GetFirstFileInfo() for details.
     */
    const SVNInfoData * GetNextFileInfo();

    virtual BOOL Cancel();
    virtual void Receiver(SVNInfoData * data);

    /// convenience methods

    static bool IsFile (const CTSVNPath& path, const SVNRev& revision);

    friend class SVN;   // So that SVN can get to our m_err
protected:
    apr_pool_t *                m_pool;         ///< the memory pool
    std::vector<SVNInfoData>    m_arInfo;       ///< contains all gathered info structs.
private:

    unsigned int                m_pos;          ///< the current position of the vector

#ifdef _MFC_VER
    SVNPrompt                   m_prompt;
#endif
    static svn_error_t *        cancel(void *baton);
    static svn_error_t *        infoReceiver(void* baton, const char * path, const svn_client_info2_t* info, apr_pool_t * pool);

};

