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

#include "stdafx.h"
#include "resource.h"

#include "SVNFolderStatus.h"
#include "SVNStatus.h"
#include "UnicodeUtils.h"


SVNFolderStatus::SVNFolderStatus(void)
{
	m_pStatusCache = NULL;
	m_TimeStamp = 0;
	invalidstatus.author[0] = 0;
	invalidstatus.askedcounter = -1;
	invalidstatus.filename[0] = 0;
	invalidstatus.status = svn_wc_status_unversioned;
	invalidstatus.url[0] = 0;
	invalidstatus.rev = -1;
}

SVNFolderStatus::~SVNFolderStatus(void)
{
	if (m_pStatusCache != NULL)
		delete[] m_pStatusCache;
}

filestatuscache * SVNFolderStatus::BuildCache(LPCTSTR filepath)
{
	svn_client_ctx_t 			ctx;
	apr_hash_t *				statushash;
	apr_pool_t *				pool;
	svn_error_t *				err;
	const char *				internalpath;
	apr_initialize();
	pool = svn_pool_create (NULL);				// create the memory pool
	memset (&ctx, 0, sizeof (ctx));


	TCHAR pathbuf[MAX_PATH+4];
	_tcscpy(pathbuf, filepath);
	PathRemoveFileSpec(pathbuf);
	PathAddBackslash(pathbuf);
	UINT drivetype = GetDriveType(pathbuf);
	if ((drivetype == DRIVE_REMOVABLE)&&(!shellCache.IsRemovable()))
		return &invalidstatus;
	if ((drivetype == DRIVE_FIXED)&&(!shellCache.IsFixed()))
		return &invalidstatus;
	if ((drivetype == DRIVE_REMOTE)&&(!shellCache.IsRemote()))
		return &invalidstatus;
	if ((drivetype == DRIVE_CDROM)&&(!shellCache.IsCDRom()))
		return &invalidstatus;

	//ATLTRACE2(_T("building cache for %s\n"), filepath);
	BOOL isFolder = PathIsDirectory(filepath);

	if (isFolder)
	{
		SVNStatus status;
		status.GetStatus(filepath);
		int index = FindFirstInvalidFolder();
		m_FolderCache[index].author[0] = 0;
		m_FolderCache[index].url[0] = 0;
		m_FolderCache[index].rev = -1;
		m_FolderCache[index].status = svn_wc_status_unversioned;
		_tcsncpy(m_FolderCache[index].filename, filepath, MAX_PATH);
		if (status.status)
		{
			if (status.status->entry)
			{
				if (status.status->entry->cmt_author)
					strncpy(m_FolderCache[index].author, status.status->entry->cmt_author, MAX_PATH);
				else
					m_FolderCache[index].author[0] = 0;
				if (status.status->entry->url)
					strncpy(m_FolderCache[index].url, status.status->entry->url, MAX_PATH);
				else
					m_FolderCache[index].url[0] = 0;
				m_FolderCache[index].rev = status.status->entry->cmt_rev;
			} // if (status.status->entry)
			m_FolderCache[index].status = SVNStatus::GetMoreImportant(svn_wc_status_unversioned, status.status->text_status);
			m_FolderCache[index].status = SVNStatus::GetMoreImportant(m_FolderCache[index].status, status.status->prop_status);
			if (shellCache.IsRecursive())
			{
				m_FolderCache[index].status = SVNStatus::GetAllStatusRecursive(filepath);
			}
			m_FolderCache[index].askedcounter = SVNFOLDERSTATUS_CACHETIMES;
		} // if (status.status)
		m_TimeStamp = GetTickCount();
		return &m_FolderCache[index];
	}

	//it's a file, not a folder. So fill in the cache with
	//all files inside the same folder as the asked file is
	//since subversion can do this in one step

	_tcscpy(pathbuf, filepath);
	if (!isFolder)
	{
		TCHAR * p = _tcsrchr(filepath, '/');
		if (p)
			pathbuf[p-filepath] = '\0';
	} // if (!isFolder)
	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'

	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(pathbuf).c_str(), pool);

	ctx.auth_baton = NULL;

	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision
	err = svn_client_status (&statushash,
							&youngest,
							internalpath,
							FALSE,				//don't descend to subitems
							1,
							false,				//don't update with repository
							1,
							&ctx,
							pool);

	// Error present if function is not under version control
	if ((err != NULL) || (apr_hash_count(statushash) == 0))
	{
		svn_pool_destroy (pool);				//free allocated memory
		return &invalidstatus;	
	}

    apr_hash_index_t *hi;
	if (m_pStatusCache != NULL)
		delete[] m_pStatusCache;
	m_nCacheCount = apr_hash_count(statushash);
	m_pStatusCache = new filestatuscache[m_nCacheCount];
	if (!m_pStatusCache)
		return &invalidstatus;
	int i=0;
	for (hi = apr_hash_first (pool, statushash); hi; hi = apr_hash_next (hi))
	{
		svn_wc_status_t * tempstatus;
		apr_hash_this(hi, NULL, NULL, (void **)&tempstatus);
		if (tempstatus->entry)
		{
			if (tempstatus->entry->cmt_author)
				strncpy(m_pStatusCache[i].author, tempstatus->entry->cmt_author, MAX_PATH-1);
			else
				m_pStatusCache[i].author[0] = 0;
			if (tempstatus->entry->url)
				strncpy(m_pStatusCache[i].url, tempstatus->entry->url, MAX_PATH-1);
			else
				m_pStatusCache[i].url[0] = 0;
			m_pStatusCache[i].rev = tempstatus->entry->cmt_rev;
			if (tempstatus->entry->kind == svn_node_dir)
			{
				if (tempstatus->entry->url)
					_tcscpy(m_pStatusCache[i].filename, _tcsrchr(CUnicodeUtils::StdGetUnicode(tempstatus->entry->url).c_str(), _T('/')) + 1);
				else
				{
					_tcscpy(m_pStatusCache[i].filename, _T(" "));
					m_pStatusCache[i].status = svn_wc_status_unversioned;
				}
			}
			else
			{
				if (tempstatus->entry->name)
					_tcscpy(m_pStatusCache[i].filename, CUnicodeUtils::StdGetUnicode(tempstatus->entry->name).c_str());
				else
				{
					_tcscpy(m_pStatusCache[i].filename, _T(" "));
					m_pStatusCache[i].status = svn_wc_status_unversioned;
				}
			}
		} // if (tempstatus->entry)
		else
		{
			m_pStatusCache[i].filename[0] = 0;
			m_pStatusCache[i].author[0] = 0;
			m_pStatusCache[i].url[0] = 0;
			m_pStatusCache[i].rev = -1;
		}
		m_pStatusCache[i].status = svn_wc_status_unversioned;
		m_pStatusCache[i].status = SVNStatus::GetMoreImportant(m_pStatusCache[i].status, tempstatus->text_status);
		m_pStatusCache[i].status = SVNStatus::GetMoreImportant(m_pStatusCache[i].status, tempstatus->prop_status);
		m_pStatusCache[i].askedcounter = SVNFOLDERSTATUS_CACHETIMES;
		i++;
	} // for (hi = apr_hash_first (pool, statushash); hi; hi = apr_hash_next (hi)) 
	svn_pool_destroy (pool);				//free allocated memory
	apr_terminate();
	m_TimeStamp = GetTickCount();
	int index = IsCacheValid(filepath);
	if (index >= 0)
	{
		if (index >= SVNFOLDERSTATUS_FOLDER)
		{
			m_pStatusCache[index-SVNFOLDERSTATUS_FOLDER].askedcounter--;
			return &m_pStatusCache[index-SVNFOLDERSTATUS_FOLDER];
		} // if (index >= SVNFOLDERSTATUS_FOLDER)
		m_FolderCache[index].askedcounter--;
		m_TimeStamp = GetTickCount();
		return &m_FolderCache[index];
	} // if (index >= 0)
	return &invalidstatus;
}

int SVNFolderStatus::FindFile(LPCTSTR filename)
{
	LPCTSTR file;
	file = filename;
	if (!PathIsDirectory(filename))
	{
		// files are searched only by the filename, not the full path
		file = _tcsrchr(filename, '/')+1;
	}
	for (int i=0; i<m_nCacheCount; i++)
	{
		if (_tcsicmp(file, m_pStatusCache[i].filename) == 0)
			return i+SVNFOLDERSTATUS_FOLDER;
	} // for (int i=0; i<m_nCacheCount; i++)
	for (i=0; i<m_nFolderCount; i++)
	{
		if (_tcsicmp(file, m_FolderCache[i].filename) == 0)
			return i;
	}
	return -1;		//not found!
}

int SVNFolderStatus::IsCacheValid(LPCTSTR filename)
{
	int i = -1;
	if ((GetTickCount() - m_TimeStamp) < GetTimeoutValue())
	{
		i = FindFile(filename);
	}
	else
	{
		ATLTRACE2(_T("cache timeout\n"));
	}
	if (i>=SVNFOLDERSTATUS_FOLDER)
	{
		if ((m_pStatusCache[i-SVNFOLDERSTATUS_FOLDER].askedcounter > 0)&&((GetTickCount() - m_TimeStamp) < GetTimeoutValue()))
			return i;
	} // if (i>=0)
	if ((i>=0)&&(m_FolderCache[i].askedcounter > 0)&&((GetTickCount() - m_TimeStamp) < GetTimeoutValue()))
		return i;
	//ATLTRACE2(_T("file %s not found\n"), filename);
	return -1;
}

int SVNFolderStatus::FindFirstInvalidFolder()
{
	DWORD tick = GetTickCount();
	if ((tick - GetTimeoutValue()) > m_TimeStamp)
	{
		m_nFolderCount = 0;
		return 0;
	} // if ((tick - SVNFOLDERSTATUS_CACHETIMEOUT) > m_TimeStamp)
	m_nFolderCount++;
	if (m_nFolderCount < SVNFOLDERSTATUS_FOLDER)
		return m_nFolderCount;
	m_nFolderCount = 0;
	return 0;
}

DWORD SVNFolderStatus::GetTimeoutValue()
{
	if (shellCache.IsRecursive())
		return SVNFOLDERSTATUS_RECURSIVECACHETIMEOUT;
	return SVNFOLDERSTATUS_CACHETIMEOUT;
}

filestatuscache * SVNFolderStatus::GetFullStatus(LPCTSTR filepath)
{
	TCHAR * filename;
	TCHAR * filepathnonconst = (LPTSTR)filepath;
	BOOL isFolder = PathIsDirectory(filepath);

	//first change the filename to 'internal' format
	for (UINT i=0; i<_tcsclen(filepath); i++)
	{
		if (filepath[i] == _T('\\'))
			filepathnonconst[i] = _T('/');
	} // for (int i=0; i<_tcsclen(filename); i++)
	filename = _tcsrchr(filepath, _T('/')) + 1;


	TCHAR pathbuf[MAX_PATH];
	_tcscpy(pathbuf, filepath);
	if (!isFolder)
	{
		TCHAR * ptr = _tcsrchr(pathbuf, '/');
		if (ptr != 0)
		{
			*ptr = 0;
			_tcscat(pathbuf, _T("/.svn"));
			if (!PathFileExists(pathbuf))
				return &invalidstatus;
		}
	} // if (!isFolder)

	int index = IsCacheValid(filepath);
	if (index >= 0)
	{
		if (index >= SVNFOLDERSTATUS_FOLDER)
		{
			m_pStatusCache[index-SVNFOLDERSTATUS_FOLDER].askedcounter--;
			ATLTRACE2(_T("cache found for %s\n"), filepath);
			return &m_pStatusCache[index-SVNFOLDERSTATUS_FOLDER];
		} // if (index >= SVNFOLDERSTATUS_FOLDER)
		m_FolderCache[index].askedcounter--;
		ATLTRACE2(_T("cache found for %s\n"), filepath);
		return &m_FolderCache[index];
	}

	return BuildCache(filepath);
}
