// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "stdafx.h"
#ifdef _MFC_VER
#include "PromptDlg.h"
#endif
#include <windows.h>
//#include "resource.h"
#include <tchar.h>
#include <Shlwapi.h>
#include "ShellCache.h"

#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_sorts.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_config.h"

#include "SVNStatus.h"
#include <map>

typedef struct filestatuscache
{
	//TCHAR					filename[MAX_PATH];
	svn_wc_status_kind		status;
	char					author[MAX_PATH];
	char					url[MAX_PATH];
	svn_revnum_t			rev;
	int						askedcounter;
} filestatuscache;

#define SVNFOLDERSTATUS_CACHETIMES				10
#define SVNFOLDERSTATUS_CACHETIMEOUT			500
#define SVNFOLDERSTATUS_RECURSIVECACHETIMEOUT	1000
#define SVNFOLDERSTATUS_FOLDER					500
/**
 * \ingroup TortoiseShell
 * This class represents a caching mechanism for the
 * subversion statuses. Once a status for a versioned
 * file is requested (GetFileStatus()) first its checked
 * if that status is already in the cache. If it is not
 * then the subversion statuses for ALL files in the same
 * directory is fetched and cached. This is because subversion
 * needs almost the same time to get one or all status (in
 * the same directory).
 * To prevent a cache flush for the explorer folder view
 * the cache is only fetched for versioned files and
 * not for folders.
 *
 * \par requirements
 * win95 or later\n
 * winNT4 or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 04-10-2003
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class SVNFolderStatus : public SVNStatus
{
public:
	SVNFolderStatus(void);
	~SVNFolderStatus(void);
	filestatuscache *	GetFullStatus(LPCTSTR filepath);

private:
	filestatuscache *	BuildCache(LPCTSTR filepath);
	DWORD				GetTimeoutValue();
	static void			fillstatusmap (void *baton, const char *path, svn_wc_status_t *status);
	
	std::map<stdstring, filestatuscache> m_cache;
	DWORD				m_TimeStamp;
	filestatuscache		invalidstatus;
	ShellCache			shellCache;
	filestatuscache		dirstat;
	filestatuscache		filestat;
};

