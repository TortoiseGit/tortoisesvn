// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "ShellExt.h"
#include "SVNFolderStatus.h"

extern ShellCache g_ShellCache;

// get / auto-alloc a string "copy"

const char* StringPool::GetString (const char* value)
{
	// special case: NULL pointer
	
	if (value == NULL)
	{
		return emptyString;
	}

	// do we already have a string with the desired value?

	pool_type::const_iterator iter = pool.find (value);
	if (iter != pool.end())
	{
		// yes -> return it
		return *iter;
	}
	
	// no -> add one	
	
	const char* newString =  strdup (value);
	if (newString)
	{
		pool.insert (newString);
	}
	else
		return emptyString;
	
	// .. and return it
	
	return newString;
}

// clear internal pool

void StringPool::clear()
{
	// delete all strings

	for (pool_type::iterator iter = pool.begin(), end = pool.end(); iter != end; ++iter)
	{
		free((void*)*iter);
	}
		
	// remove pointers from pool
		
	pool.clear();
}
	

SVNFolderStatus::SVNFolderStatus(void)
{
	m_TimeStamp = 0;
	emptyString[0] = 0;
	invalidstatus.author = emptyString;
	invalidstatus.askedcounter = -1;
	invalidstatus.status = svn_wc_status_unversioned;
	invalidstatus.url = emptyString;
	invalidstatus.rev = -1;
	m_nCounter = 0;
	sCacheKey.reserve(MAX_PATH);
	hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, _T("TortoiseSVNStatuscache"));
	if (hMutex == NULL)
	{
		hMutex = CreateMutex(NULL, NULL,  _T("TortoiseSVNStatuscache"));
		SetSecurityInfo(hMutex,SE_KERNEL_OBJECT,DACL_SECURITY_INFORMATION,0,0,0,0);
		ATLTRACE2(_T("created mutex\n"));
	}
	else
	{
		ATLTRACE2(_T("opened mutex\n"));
	}
}

SVNFolderStatus::~SVNFolderStatus(void)
{
	CloseHandle(hMutex);
}

filestatuscache * SVNFolderStatus::BuildCache(LPCTSTR filepath, BOOL bIsFolder)
{
	svn_client_ctx_t *			ctx;
	apr_hash_t *				statushash;
	apr_pool_t *				pool;
	svn_error_t *				err = NULL; // If svn_client_status comes out through catch(...), err would else be unassigned
	const char *				internalpath;

	//dont' build the cache if an instance of TortoiseProc is running
	//since this could interfere with svn commands running (concurrent
	//access of the .svn directory).
	if (g_ShellCache.BlockStatus())
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

	pool = svn_pool_create (NULL);				// create the memory pool
	svn_utf_initialize(pool);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", pool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", pool);

	svn_client_create_context(&ctx, pool);

	svn_pool_clear(this->m_pool);

	m_cache.clear();
	
	// strings pools are unused, now -> we may clear them
	
	authors.clear();
	urls.clear();
	
	ATLTRACE2(_T("building cache for %s\n"), filepath);
	if (bIsFolder)
	{
		if (g_ShellCache.IsRecursive())
		{
			// initialize record members
			dirstat.rev = -1;
			dirstat.status = svn_wc_status_unversioned;
			dirstat.author = authors.GetString(NULL);
			dirstat.url = urls.GetString(NULL);
			dirstat.askedcounter = SVNFOLDERSTATUS_CACHETIMES;
			try
			{
				GetStatus(filepath);
			}
			catch ( ... )
			{
				status = NULL;
			}
			if (status)
			{
				if (status->entry)
				{
					dirstat.author = authors.GetString (status->entry->cmt_author);
					dirstat.url = authors.GetString (status->entry->url);
					dirstat.rev = status->entry->cmt_rev;
				} // if (status.status->entry)
				dirstat.status = SVNStatus::GetAllStatusRecursive(filepath);
			} // if (status)
			m_cache[filepath] = dirstat;
			m_TimeStamp = GetTickCount();
			svn_pool_destroy (pool);				//free allocated memory
			ClearPool();
			return &dirstat;
		} // if (g_ShellCache.IsRecursive())
	} // if (bIsFolder) 
	
	m_nCounter = 0;
	
	//Fill in the cache with
	//all files inside the same folder as the asked file/folder is
	//since subversion can do this in one step
	TCHAR pathbuf[MAX_PATH+4];
	_tcscpy(pathbuf, filepath);
	const TCHAR * p = _tcsrchr(filepath, '\\');
	if (p)
		pathbuf[p-filepath] = '\0';
	if ((bIsFolder)&&(!g_ShellCache.HasSVNAdminDir(pathbuf, bIsFolder)))
	{
		_tcscpy(pathbuf, filepath);
	}

	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(pathbuf).c_str(), pool);
	ctx->auth_baton = NULL;

	statushash = apr_hash_make(pool);
	svn_revnum_t youngest = SVN_INVALID_REVNUM;
	svn_opt_revision_t rev;
	rev.kind = svn_opt_revision_unspecified;
	try
	{
		err = svn_client_status (&youngest,
			internalpath,
			&rev,
			fillstatusmap,
			this,
			FALSE,		//descend
			TRUE,		//getall
			FALSE,		//update
			TRUE,		//noignore
			ctx,
			pool);
	}
	catch ( ... )
	{
	}

	// Error present if function is not under version control
	if (err != NULL)
	{
		svn_pool_destroy (pool);				//free allocated memory
		ClearPool();
		return &invalidstatus;	
	}

	svn_pool_destroy (pool);				//free allocated memory
	m_TimeStamp = GetTickCount();
	filestatuscache * ret = NULL;
	std::map<stdstring, filestatuscache>::iterator iter;
	if ((iter = m_cache.find(filepath)) != m_cache.end())
	{
		ret = (filestatuscache *)&iter->second;
	}
	ClearPool();
	if (ret)
		return ret;
	return &invalidstatus;
}

DWORD SVNFolderStatus::GetTimeoutValue()
{
	DWORD timeout = SVNFOLDERSTATUS_CACHETIMEOUT;
	if (g_ShellCache.IsRecursive())
		timeout = SVNFOLDERSTATUS_RECURSIVECACHETIMEOUT;
	DWORD factor = m_cache.size()/200;
	if (factor==0)
		factor = 1;
	return factor*timeout;
}

filestatuscache * SVNFolderStatus::GetFullStatus(LPCTSTR filepath, BOOL bIsFolder, BOOL bColumnProvider)
{
	filestatuscache * ret = NULL;
	m_bColumnProvider = bColumnProvider;
	BOOL bHasAdminDir = g_ShellCache.HasSVNAdminDir(filepath, bIsFolder);
	
	//no overlay for unversioned folders
	if ((!bColumnProvider)&&(!bHasAdminDir))
		return &invalidstatus;
	//for the SVNStatus column, we have to check the cache to see
	//if it's not just unversioned but ignored
	ret = GetCachedItem(filepath);
	if (ret)
		return ret;

	//if it's not in the cache and has no admin dir, then we assume
	//it's not ignored too
	if ((bColumnProvider)&&(!bHasAdminDir))
		return &invalidstatus;
	DWORD dwWaitResult = WaitForSingleObject(hMutex, 1000);
	if (dwWaitResult == WAIT_OBJECT_0)
		ret = BuildCache(filepath, bIsFolder);
	ReleaseMutex(hMutex);
	if (ret)
		return ret;
	else
		return &invalidstatus;
}

filestatuscache * SVNFolderStatus::GetCachedItem(LPCTSTR filepath)
{
	DWORD dwWaitResult = WaitForSingleObject(hMutex, 1000);
	if (dwWaitResult == WAIT_OBJECT_0)
	{
		sCacheKey.assign(filepath);
		std::map<stdstring, filestatuscache>::iterator iter;
		if ((iter = m_cache.find(sCacheKey)) != m_cache.end())
		{
			ATLTRACE2(_T("cache found for %s\n"), filepath);
			DWORD now = GetTickCount();
			ReleaseMutex(hMutex);
			if ((now >= m_TimeStamp)&&((now - m_TimeStamp) > GetTimeoutValue()))
			{
				// Cache is timeed-out
				return NULL;
			}
			return (filestatuscache *)&iter->second;
		}
	}
	ReleaseMutex(hMutex);
	return NULL;
}

void SVNFolderStatus::fillstatusmap(void * baton, const char * path, svn_wc_status_t * status)
{
	SVNFolderStatus * Stat = (SVNFolderStatus *)baton;
	if ((status->entry)&&(g_ShellCache.IsRecursive())&&(status->entry->kind == svn_node_dir))
		return;

	std::map<stdstring, filestatuscache> * cache = (std::map<stdstring, filestatuscache> *)(&Stat->m_cache);
	filestatuscache s;
	if ((status)&&(status->entry))
	{
		s.author = Stat->authors.GetString(status->entry->cmt_author);
		s.url = Stat->urls.GetString(status->entry->url);
		s.rev = status->entry->cmt_rev;
	} // if (status->entry) 
	else
	{
		s.author = Stat->authors.GetString(NULL);
		s.url = Stat->urls.GetString(NULL);
		s.rev = -1;
	}
	s.status = svn_wc_status_unversioned;
	if (status)
	{
		s.status = SVNStatus::GetMoreImportant(s.status, status->text_status);
		s.status = SVNStatus::GetMoreImportant(s.status, status->prop_status);
	}
	s.askedcounter = SVNFOLDERSTATUS_CACHETIMES;
	stdstring str;
	if (path)
	{
		char osPath[MAX_PATH+1];
		UINT len = strlen(path);
		for (UINT i=0; i<len; i++)
		{
			if (path[i] =='/')
				osPath[i] = '\\';
			else
				osPath[i] = path[i];
		}
		osPath[i] = 0;
		str = CUnicodeUtils::StdGetUnicode(osPath);
	}
	else
		str = _T(" ");
	(*cache)[str] = s;
}
