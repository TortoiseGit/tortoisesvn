// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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

#include "SVNPrompt.h"
#include "TSVNPath.h"
#include "SVNRev.h"

class SVNInfoData
{
public:
	SVNInfoData(){}

	CString				url;
	SVNRev				rev;
	svn_node_kind_t		kind;
	CString				reposRoot;
	CString				reposUUID;
	SVNRev				lastchangedrev;
	__time64_t			lastchangedtime;
	CString				author;

	CString				lock_path;
	CString				lock_token;
	CString				lock_owner;
	CString				lock_comment;
	bool				lock_davcomment;
	__time64_t			lock_createtime;
	__time64_t			lock_expirationtime;

	bool				hasWCInfo;
	svn_wc_schedule_t	schedule;
	CString				copyfromurl;
	SVNRev				copyfromrev;
	__time64_t			texttime;
	__time64_t			proptime;
	CString				checksum;
	CString				conflict_old;
	CString				conflict_new;
	CString				conflict_wrk;
	CString				prejfile;
};


/**
 * \ingroup SVN
 * Wrapper for the svn_client_info() API.
 */
class SVNInfo
{
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
	const SVNInfoData * GetFirstFileInfo(const CTSVNPath& path, SVNRev pegrev, SVNRev revision, bool recurse = false);
	size_t GetFileCount() {return m_arInfo.size();}
	/**
	 * Returns the info of the next file in the filelist. If no more files are in the list then NULL is returned.
	 * See GetFirstFileInfo() for details.
	 */
	const SVNInfoData * GetNextFileInfo();

friend class SVN;	// So that SVN can get to our m_err
	/**
	 * Returns the last error message as a CString object.
	 */
	CString GetLastErrorMsg();

	virtual BOOL Cancel();
	virtual void Receiver(SVNInfoData * data);

protected:
	apr_pool_t *				m_pool;			///< the memory pool
	std::vector<SVNInfoData>	m_arInfo;		///< contains all gathered info structs.
private:
	svn_client_ctx_t * 			m_pctx;
	svn_error_t *				m_err;			///< Subversion error baton

	unsigned int				m_pos;			///< the current position of the vector

	SVNPrompt					m_prompt;
	
	static svn_error_t *		cancel(void *baton);
	static svn_error_t *		infoReceiver(void* baton, const char * path, const svn_info_t* info, apr_pool_t * pool);
	
};

