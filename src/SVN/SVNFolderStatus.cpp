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
#include "UnicodeUtils.h"


SVNFolderStatus::SVNFolderStatus(void)
{
	m_TimeStamp = 0;
	invalidstatus.author[0] = 0;
	invalidstatus.askedcounter = -1;
	invalidstatus.status = svn_wc_status_unversioned;
	invalidstatus.url[0] = 0;
	invalidstatus.rev = -1;
}

SVNFolderStatus::~SVNFolderStatus(void)
{
}

filestatuscache * SVNFolderStatus::BuildCache(LPCTSTR filepath)
{
	svn_client_ctx_t 			ctx;
	apr_hash_t *				statushash;
	apr_pool_t *				pool;
	svn_error_t *				err;
	const char *				internalpath;

	//dont' build the cache if an instance of TortoiseProc is running
	//since this could interfere with svn commands running (concurrent
	//access of the .svn directory).
	if (shellCache.BlockStatus())
	{
		HANDLE TSVNMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseProc.exe"));	
		if (TSVNMutex != NULL)
		{
			if (::GetLastError() == ERROR_ALREADY_EXISTS)
			{
				::CloseHandle(TSVNMutex);
				return &invalidstatus;
			} // if (::GetLastError() == ERROR_ALREADY_EXISTS) 
		} // if (TSVNMutex != NULL)
		::CloseHandle(TSVNMutex);
	}

	apr_initialize();
	pool = svn_pool_create (NULL);				// create the memory pool
	memset (&ctx, 0, sizeof (ctx));

	svn_pool_clear(this->m_pool);

	m_cache.clear();
	ATLTRACE2(_T("building cache for %s\n"), filepath);
	BOOL isFolder = PathIsDirectory(filepath);

	if (isFolder)
	{
		GetStatus(filepath);
		dirstat.rev = -1;
		dirstat.status = svn_wc_status_unversioned;

		if (status)
		{
			//if (status.status->entry)
			if (status->entry)
			{
				if (status->entry->cmt_author)
					strncpy(dirstat.author, status->entry->cmt_author, MAX_PATH);
				else
					dirstat.author[0] = 0;
				if (status->entry->url)
					strncpy(dirstat.url, status->entry->url, MAX_PATH);
				else
					dirstat.url[0] = 0;
				dirstat.rev = status->entry->cmt_rev;
			} // if (status.status->entry)
			dirstat.status = SVNStatus::GetMoreImportant(svn_wc_status_unversioned, status->text_status);
			dirstat.status = SVNStatus::GetMoreImportant(dirstat.status, status->prop_status);
			if (shellCache.IsRecursive())
			{
				dirstat.status = SVNStatus::GetAllStatusRecursive(filepath);
			}
		} // if (status.status)
		dirstat.askedcounter = SVNFOLDERSTATUS_CACHETIMES;
		m_cache[filepath] = dirstat;
		m_TimeStamp = GetTickCount();
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return &dirstat;
	}

	//it's a file, not a folder. So fill in the cache with
	//all files inside the same folder as the asked file is
	//since subversion can do this in one step
	TCHAR pathbuf[MAX_PATH+4];
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

	statushash = apr_hash_make(pool);
	svn_revnum_t youngest = SVN_INVALID_REVNUM;
	svn_opt_revision_t rev;
	rev.kind = svn_opt_revision_unspecified;
	err = svn_client_status (&youngest,
							internalpath,
							&rev,
							fillstatusmap,
							&m_cache,
							FALSE,		//descend
							TRUE,		//getall
							FALSE,		//update
							TRUE,		//noignore
							&ctx,
							pool);

	// Error present if function is not under version control
	if (err != NULL)
	{
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return &invalidstatus;	
	}

	svn_pool_destroy (pool);				//free allocated memory
	apr_terminate();
	m_TimeStamp = GetTickCount();
	filestatuscache * ret = NULL;
	std::map<stdstring, filestatuscache>::iterator iter;
	if ((iter = m_cache.find(filepath)) != m_cache.end())
	{
		ret = (filestatuscache *)&iter->second;
	}
	if (ret)
		return ret;
	return &invalidstatus;
}

DWORD SVNFolderStatus::GetTimeoutValue()
{
	if (shellCache.IsRecursive())
		return SVNFOLDERSTATUS_RECURSIVECACHETIMEOUT;
	return SVNFOLDERSTATUS_CACHETIMEOUT;
}

filestatuscache * SVNFolderStatus::GetFullStatus(LPCTSTR filepath)
{
	TCHAR * filepathnonconst = (LPTSTR)filepath;

	//first change the filename to 'internal' format
	for (UINT i=0; i<_tcsclen(filepath); i++)
	{
		if (filepath[i] == _T('\\'))
			filepathnonconst[i] = _T('/');
	} // for (int i=0; i<_tcsclen(filename); i++)

	if (! shellCache.IsPathAllowed(filepath))
		return &invalidstatus;

	BOOL isFolder = PathIsDirectory(filepath);
	TCHAR pathbuf[MAX_PATH];
	_tcscpy(pathbuf, filepath);
	if (!isFolder)
	{
		TCHAR * ptr = _tcsrchr(pathbuf, '/');
		if (ptr != 0)
		{
			*ptr = 0;
			_tcscat(pathbuf, _T("/"));
			_tcscat(pathbuf, _T(SVN_WC_ADM_DIR_NAME));
			if (!PathFileExists(pathbuf))
				return &invalidstatus;
		}
	} // if (!isFolder)
	else
	{
		_tcscat(pathbuf, _T("/"));
		_tcscat(pathbuf, _T(SVN_WC_ADM_DIR_NAME));
		if (!PathFileExists(pathbuf))
			return &invalidstatus;
	}
	filestatuscache * ret = NULL;
	std::map<stdstring, filestatuscache>::iterator iter;
	if ((iter = m_cache.find(filepath)) != m_cache.end())
	{
		ATLTRACE2(_T("cache found for %s - %s\n"), filepath, pathbuf);
		ret = (filestatuscache *)&iter->second;
		DWORD now = GetTickCount();
		if ((now >= m_TimeStamp)&&((now - m_TimeStamp) > GetTimeoutValue()))
			ret = NULL;
	}
	if (ret)
		return ret;

	return BuildCache(filepath);
}

void SVNFolderStatus::fillstatusmap(void * baton, const char * path, svn_wc_status_t * status)
{
	std::map<stdstring, filestatuscache> * cache = (std::map<stdstring, filestatuscache> *)baton;
	filestatuscache s;
	TCHAR * key = NULL;
	if (status->entry)
	{
		if (status->entry->cmt_author)
			strncpy(s.author, status->entry->cmt_author, MAX_PATH-1);
		if (status->entry->url)
			strncpy(s.url, status->entry->url, MAX_PATH-1);
		s.rev = status->entry->cmt_rev;
	} // if (status->entry) 
	else
	{
		s.author[0] = 0;
		s.url[0] = 0;
		s.rev = -1;
	}
	s.status = svn_wc_status_unversioned;
	s.status = SVNStatus::GetMoreImportant(s.status, status->text_status);
	s.status = SVNStatus::GetMoreImportant(s.status, status->prop_status);
	s.askedcounter = SVNFOLDERSTATUS_CACHETIMES;
	stdstring str;
	if (path)
		str = CUnicodeUtils::StdGetUnicode(path);
	else
		str = _T(" ");

	(*cache)[str] = s;
	ATLTRACE2(_T("%s\n"), str.c_str());
}
