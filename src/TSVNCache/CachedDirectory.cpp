#include "StdAfx.h"
#include ".\cacheddirectory.h"
#include "SVNHelpers.h"
#include "SVNStatusCache.h"
#include "SVNStatus.h"
#include <ShlObj.h>

CCachedDirectory::CCachedDirectory(void)
{
	m_entriesFileTime = 0;
	m_propsDirTime = 0;
	m_mostImportantFileStatus = svn_wc_status_unversioned;
}

CCachedDirectory::~CCachedDirectory(void)
{
}

CCachedDirectory::CCachedDirectory(const CTSVNPath& directoryPath)
{
	ATLASSERT(directoryPath.IsDirectory() || !PathFileExists(directoryPath.GetWinPath()));

	m_directoryPath = directoryPath;
	m_entriesFileTime = 0;
	m_propsDirTime = 0;
	m_mostImportantFileStatus = svn_wc_status_unversioned;
}

CStatusCacheEntry CCachedDirectory::GetStatusForMember(const CTSVNPath& path, bool bRecursive)
{
	CString strCacheKey;
	bool bThisDirectoryIsUnversioned = false;
	bool bRequestForSelf = false;
	if(path.IsEquivalentTo(m_directoryPath))
	{
		bRequestForSelf = true;
	}

	if(path.IsEquivalentTo(CTSVNPath(_T("C:\\keil"))))
	{
		ATLTRACE("");
	}

	// In all most circumstances, we ask for the status of a member of this directory.
	ATLASSERT(m_directoryPath.IsEquivalentTo(path.GetContainingDirectory()) || bRequestForSelf);

	// Check if the entries file has been changed
	CTSVNPath entriesFilePath(m_directoryPath);
	entriesFilePath.AppendPathString(_T(SVN_WC_ADM_DIR_NAME) _T("\\entries"));
	CTSVNPath propsDirPath(m_directoryPath);
	propsDirPath.AppendPathString(_T(SVN_WC_ADM_DIR_NAME) _T("\\props"));
	if(m_entriesFileTime == entriesFilePath.GetLastWriteTime() && m_propsDirTime == propsDirPath.GetLastWriteTime())
	{
		if(m_entriesFileTime == 0)
		{
			// We are a folder which is not in a working copy
			bThisDirectoryIsUnversioned = true;
			m_ownStatus.SetStatus(NULL);

			// We should never have anything in our file item cache if we're unversioned
			ATLASSERT(m_entryCache.empty());

			// However, a member *DIRECTORY* might be the top of WC
			// so we need to ask them to get their own status
			if(!path.IsDirectory())
			{
				return CStatusCacheEntry();
			}
			else
			{
				// If we're in the special case of a directory being asked for its own status
				// and this directory is unversioned, then we should just return that here
				if(path.IsEquivalentTo(m_directoryPath))
				{
					return CStatusCacheEntry();
				}
			}
		}

		if(path.IsDirectory())
		{
			// We don't have directory status in our cache
			// Ask the directory if it knows its own status
			CCachedDirectory& dirEntry = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
			if(dirEntry.IsOwnStatusValid())
			{
				// This directory knows its own status

				// To keep recursive status up to date, we'll request that children are all crawled again
				// This will be very quick if nothing's changed, because it will all be cache hits
				ChildDirStatus::const_iterator it;
				for(it = dirEntry.m_childDirectories.begin(); it != dirEntry.m_childDirectories.end(); ++it)
				{
					CTSVNPath childPath = it->first;
					CSVNStatusCache::Instance().AddFolderForCrawling(it->first);
				}


				return dirEntry.GetOwnStatus(bRecursive);
			}
		}
		else
		{
			// Look up a file in our own cache
			strCacheKey = GetCacheKey(path);
			CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
			if(itMap != m_entryCache.end())
			{
				// We've hit the cache - check for timeout
				if(!itMap->second.HasExpired((long)GetTickCount()))
				{
					if(itMap->second.DoesFileTimeMatch(path.GetLastWriteTime()))
					{
						return itMap->second;
					}
					else
					{
						ATLTRACE("Filetime change on file %ws\n", path.GetWinPath());
					}
				}
				else
				{
					ATLTRACE("Cache timeout on file %ws\n", path.GetWinPath());
				}
			}
		}
	}
	else
	{
		if(m_entriesFileTime != 0)
		{
			ATLTRACE("Entries file change seen for %ws\n", path.GetWinPath());
		}
		m_entriesFileTime = entriesFilePath.GetLastWriteTime();
		m_propsDirTime = propsDirPath.GetLastWriteTime();
		m_entryCache.clear();
	}

	CSVNStatusCache& mainCache = CSVNStatusCache::Instance();
	SVNPool subPool(mainCache.m_svnHelp.Pool());
	svn_opt_revision_t revision;
	revision.kind = svn_opt_revision_unspecified;

	// We've not got this item in the cache - let's add it
	// We never bother asking SVN for the status of just one file, always for its containing directory

	if(path.IsDirectory() && path.GetFileOrDirectoryName().CompareNoCase(_T(SVN_WC_ADM_DIR_NAME)) == 0)
	{
		// We're being asked for the status of an .SVN directory
		// It's not worth asking for this
		AddEntry(path, NULL);
		return CStatusCacheEntry();
	}


	svn_wc_status_kind preUpdateStatus = m_mostImportantFileStatus;
	m_mostImportantFileStatus = svn_wc_status_unversioned;
	m_childDirectories.clear();

	if(!bThisDirectoryIsUnversioned)
	{
		ATLTRACE("svn_cli_stat for '%ws' (req %ws)\n", m_directoryPath.GetWinPath(), path.GetWinPath());

		svn_error_t* pErr = svn_client_status (
			NULL,
			m_directoryPath.GetSVNApiPath(),
			&revision,
			GetStatusCallback,
			this,
			mainCache.m_bDoRecursiveFetches,		//descend
			TRUE,									//getall
			mainCache.m_bGetRemoteStatus,			//update
			TRUE,									//noignore
			mainCache.m_svnHelp.ClientContext(),
			subPool
			);

		if(pErr)
		{
			// Handle an error
			// The most likely error on a folder is that it's not part of a WC
			// In most circumstances, this will have been caught earlier,
			// but in some situations, we'll get this error.
			// If we allow ourselves to fall on through, then folders will be asked
			// for their own status, and will set themselves as unversioned, for the 
			// benefit of future requests
			ATLTRACE("svn_cli_stat err: '%s'\n", pErr->message);
			ATLASSERT(FALSE);
		}
	}
	else
	{
		ATLTRACE("Skipped SVN status for unversioned folder\n");
	}

	// Now that we've refreshed our SVN status, we can see if it's 
	// changed the 'most important' status value for this directory.
	// If it has, then we should tell our parent
	if(m_mostImportantFileStatus != preUpdateStatus)
	{
		PushOurStatusToParent();
	}

	if(path.IsDirectory())
	{
		CCachedDirectory& dirEntry = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
		if(dirEntry.IsOwnStatusValid())
		{
			return dirEntry.GetOwnStatus(bRecursive);
		}

		// If the status *still* isn't valid here, it means that 
		// the current directory is unversioned, and we shall need to ask its children for info about themselves
		return dirEntry.GetStatusForMember(path,bRecursive);
	}
	else
	{
		CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
		if(itMap != m_entryCache.end())
		{
			return itMap->second;
		}
	}

	AddEntry(path, NULL);
	return CStatusCacheEntry();
}

void 
CCachedDirectory::AddEntry(const CTSVNPath& path, const svn_wc_status_t* pSVNStatus)
{
	if(path.IsDirectory())
	{
		CSVNStatusCache::Instance().GetDirectoryCacheEntry(path).m_ownStatus.SetStatus(pSVNStatus);
	}
	else
	{
		m_entryCache[GetCacheKey(path)] = CStatusCacheEntry(pSVNStatus,path.GetLastWriteTime());
	}
}


CString 
CCachedDirectory::GetCacheKey(const CTSVNPath& path)
{
	// All we put into the cache as a key is just the end portion of the pathname
	// There's no point storing the path of the containing directory for every item
	return path.GetWinPathString().Mid(m_directoryPath.GetWinPathString().GetLength()).MakeLower();
}

CString 
CCachedDirectory::GetFullPathString(const CString& cacheKey)
{
	return m_directoryPath.GetWinPathString() + _T("\\") + cacheKey;
}

void CCachedDirectory::GetStatusCallback(void *baton, const char *path, svn_wc_status_t *status)
{
	CCachedDirectory* pThis = (CCachedDirectory*)baton;

	CTSVNPath svnPath;

	if(status->entry)
	{
		svnPath.SetFromSVN(path, (status->entry->kind == svn_node_dir));

		if(svnPath.IsDirectory())
		{
			if(!svnPath.IsEquivalentTo(pThis->m_directoryPath))
			{
				// Add any versioned directory, which is not our 'self' entry, to the list for having its status updated
				CSVNStatusCache::Instance().AddFolderForCrawling(svnPath);

				// Make sure we know about this child directory
				// This initial status value is likely to be overwritten from below at some point
				pThis->m_childDirectories[svnPath] = SVNStatus::GetMoreImportant(status->text_status, status->prop_status);
			}
		}
		else
		{
			// Keep track of the most important status of all the files in this directory
			// Don't include subdirectories in this figure, because they need to provide their 
			// own 'most important' value
			pThis->m_mostImportantFileStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status->text_status);
			pThis->m_mostImportantFileStatus = SVNStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status->prop_status);
		}
	}
	else
	{
		svnPath.SetFromSVN(path);
	}

	pThis->AddEntry(svnPath, status);
}

bool 
CCachedDirectory::IsOwnStatusValid() const
{
	return m_ownStatus.HasBeenSet() && !m_ownStatus.HasExpired(GetTickCount());

}

svn_wc_status_kind CCachedDirectory::GetMostImportantStatus() const
{
	svn_wc_status_kind retVal = SVNStatus::GetMoreImportant(m_mostImportantFileStatus, m_ownStatus.GetEffectiveStatus());

	ChildDirStatus::const_iterator it;
	for(it = m_childDirectories.begin(); it != m_childDirectories.end(); ++it)
	{
//		CTSVNPath path = it->first;
		retVal = SVNStatus::GetMoreImportant(retVal, it->second);
	}
	
	return retVal;
}

void CCachedDirectory::PushOurStatusToParent()
{
	// See if we've got a parent
	CTSVNPath parentPath = m_directoryPath.GetContainingDirectory();
	if(!parentPath.IsEmpty())
	{
		// We have a parent
		CSVNStatusCache::Instance().GetDirectoryCacheEntry(parentPath).UpdateChildDirectoryStatus(m_directoryPath, GetMostImportantStatus());
	}
}

void CCachedDirectory::UpdateChildDirectoryStatus(const CTSVNPath& childDir, svn_wc_status_kind childStatus)
{
	svn_wc_status_kind currentStatus = m_childDirectories[childDir];
	if(currentStatus != childStatus)
	{
		// This status has changed - we need to push it up again
		SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, childDir.GetWinPath(), NULL);

		m_childDirectories[childDir] = childStatus;
		PushOurStatusToParent();
	}
}

CStatusCacheEntry CCachedDirectory::GetOwnStatus(bool bRecursive) const
{
	// Don't return recursive status if we're unversioned ourselves.
	if(bRecursive && m_ownStatus.GetEffectiveStatus() > svn_wc_status_unversioned)
	{
		CStatusCacheEntry recursiveStatus(m_ownStatus);
		recursiveStatus.ForceStatus(GetMostImportantStatus());
		return recursiveStatus;				
	}
	else
	{
		return m_ownStatus;
	}
}

void CCachedDirectory::RefreshStatus()
{
	// Make sure that our own status is up-to-date
	GetStatusForMember(m_directoryPath,true);

	// We also need to check if all our file members have the right date on them
	CacheEntryMap::iterator itMembers;
	for(itMembers = m_entryCache.begin(); itMembers != m_entryCache.end(); ++itMembers)
	{
		CTSVNPath filePath(m_directoryPath);
		filePath.AppendPathString(itMembers->first);
		if(!itMembers->second.DoesFileTimeMatch(filePath.GetLastWriteTime()))
		{
			// We need to request this item as well
			GetStatusForMember(filePath,false);
		}
	}

	// Make sure that its parent knows its status
	PushOurStatusToParent();
}
