// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
    __time64_t          proptime;
    CString             checksum;
    CString             conflict_old;
    CString             conflict_new;
    CString             conflict_wrk;
    CString             prejfile;

    CString             changelist;
    svn_depth_t         depth;
    svn_filesize_t      working_size64;

    // tree conflict data
    CString             treeconflict_path;
    svn_node_kind_t     treeconflict_nodekind;
    svn_wc_conflict_kind_t treeconflict_kind;
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

    // convenience methods:

    bool IsValid() {return rev.IsValid() != FALSE;}

};


/**
 * \ingroup SVN
 * Wrapper for the svn_client_info() API.
 */
class SVNInfo
{
private:
    SVNInfo(const SVNInfo&){}
    SVNInfo& operator=(SVNInfo&){};
public:
    SVNInfo(void);
    ~SVNInfo(void);

    /**
     * returns the info for the \a path.
     * \param path a path or an url
     * \param pegrev the peg revision to use
     * \param revision the revision to get the info for
     * \param recurse if TRUE, then GetNextFileInfo() returns the info also
     * for all children of \a path.
     */
    const SVNInfoData * GetFirstFileInfo(const CTSVNPath& path, SVNRev pegrev, SVNRev revision, svn_depth_t depth = svn_depth_empty);
    size_t GetFileCount() const {return m_arInfo.size();}
    /**
     * Returns the info of the next file in the file list. If no more files are in the list then NULL is returned.
     * See GetFirstFileInfo() for details.
     */
    const SVNInfoData * GetNextFileInfo();

    friend class SVN;   // So that SVN can get to our m_err
#ifdef _MFC_VER
    /**
     * Returns the last error message as a CString object.
     */
    CString GetLastErrorMsg();
#endif
    const svn_error_t * GetError() const { return m_err; }

    virtual BOOL Cancel();
    virtual void Receiver(SVNInfoData * data);

    /// convenience methods

    static bool IsFile (const CTSVNPath& path, const SVNRev& revision);

protected:
    apr_pool_t *                m_pool;         ///< the memory pool
    std::vector<SVNInfoData>    m_arInfo;       ///< contains all gathered info structs.
private:
    svn_client_ctx_t *          m_pctx;
    svn_error_t *               m_err;          ///< Subversion error baton

    unsigned int                m_pos;          ///< the current position of the vector

#ifdef _MFC_VER
    SVNPrompt                   m_prompt;
#endif
    static svn_error_t *        cancel(void *baton);
    static svn_error_t *        infoReceiver(void* baton, const char * path, const svn_info_t* info, apr_pool_t * pool);

};

