#include "StdAfx.h"
#include "Shellupdater.h"
#include "MessageBox.h"
#include "TortoiseProc.h"
#include "Registry.h"

CShellUpdater::CShellUpdater(void)
{
	m_hInvalidationEvent = CreateEvent(NULL, FALSE, FALSE, _T("TortoiseSVNCacheInvalidationEvent"));

	// We need to start our worker thread
	DWORD threadId;
	if((m_hThread = CreateThread(NULL, 0, UpdateThreadEntry, this, 0, &threadId)) == NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

CShellUpdater::~CShellUpdater(void)
{
	Flush();

	// Stop the worker thread
	m_terminationEvent.SetEvent();
	if(WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0)
	{
		TRACE("Error shutting-down shell update worker thread\n");
	}

	CloseHandle(m_hInvalidationEvent);
	CloseHandle(m_hThread);
}

CShellUpdater& CShellUpdater::Instance()
{
	static CShellUpdater instance;
	return instance;
}

/** 
* Add a single path for updating.
* The update will happen at some suitable time in the future
*/
void CShellUpdater::AddPathForUpdate(const CTSVNPath& path)
{
	CSingleLock lock(&m_critSec, TRUE);

	// Tell the shell extension to purge its cache - we'll redo this when 
	// we actually do the shell-updates, but sometimes there's an earlier update, which
	// might benefit from cache invalidation
	SetEvent(m_hInvalidationEvent);

	m_pathsForUpdating.AddPath(path);
	m_updateAt = (long)GetTickCount() + 5000;
}
/** 
* Add a list of paths for updating.
* The update will happen at some suitable time in the future
*/
void CShellUpdater::AddPathsForUpdate(const CTSVNPathList& pathList)
{
	for(int nPath=0; nPath < pathList.GetCount(); nPath++)
	{
		AddPathForUpdate(pathList[nPath]);
	}
}

// This thread will kick-off the update, when it's a suitable time
DWORD CShellUpdater::UpdateThreadEntry(LPVOID pVoid)
{
	((CShellUpdater*)pVoid)->UpdateThread();
	return 0;
}
void CShellUpdater::UpdateThread()
{
	while(WaitForSingleObject(m_terminationEvent.m_hObject, 1000) == WAIT_TIMEOUT)
	{
		CSingleLock lock(&m_critSec, TRUE);
		if(((long)GetTickCount() - m_updateAt) > 0)
		{
			// It's time to update
			Flush();
		}
	}
}

void CShellUpdater::Flush()
{
	CSingleLock lock(&m_critSec, TRUE);

	if(m_pathsForUpdating.GetCount() > 0)
	{
		TRACE("Flushing shell update list\n");

		UpdateShell();
		m_pathsForUpdating.Clear();
	}
}

void CShellUpdater::UpdateShell()
{
	// Tell the shell extension to purge its cache
	TRACE("Setting cache invalidation event %d\n", GetTickCount());
	SetEvent(m_hInvalidationEvent);

	// We use the SVN 'notify' call-back to add items to the list
	// Because this might call-back more than once per file (for example, when committing)
	// it's possible that there may be duplicates in the list.
	// There's no point asking the shell to do more than it has to, so we remove the duplicates before
	// passing the list on
	m_pathsForUpdating.RemoveDuplicates();

	DWORD brute = CRegDWORD(_T("Software\\TortoiseSVN\\ForceShellUpdate"), 0);
	if (brute)
	{
		//this method actually works, i.e. the icon overlays are updated as they
		//should. The problem is that _every_ icon is refreshed, even those
		//located on a server share. And this can block the explorer for about 5
		//seconds on my computer in the office. So use this function only if the
		//user requests it!
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSHNOWAIT, 0, 0);
	}
	else
	{
		//updating the left pane (tree view) of the explorer
		//is more difficult (if not impossible) than I thought.
		//Using SHChangeNotify() doesn't work at all. I found that
		//the shell receives the message, but then checks the files/folders
		//itself for changes. And since the folders which are shown
		//in the tree view haven't changed the icon-overlay is
		//not updated!
		//a workaround for this problem would be if this method would
		//rename the folders, do a SHChangeNotify(SHCNE_RMDIR, ...),
		//rename the folders back and do an SHChangeNotify(SHCNE_UPDATEDIR, ...)
		//
		//But I'm not sure if that is really a good workaround - it'll possibly
		//slows down the explorer and also causes more HD usage.
		//
		//So this method only updates the files and folders in the normal
		//explorer view by telling the explorer that the folder icon itself
		//has changed.

		for(int nPath = 0; nPath < m_pathsForUpdating.GetCount(); nPath++)
		{
			// WGD: There seems to be a great disparity between the documentation for SHChangeNotify 
			// and the reality.  We used to make one call, with (SHCNE_UPDATEITEM | SHCNE_UPDATEDIR) as
			// the first parameter.  Personally, I've *never* found that to work, at all.
			// Very careful experimentation lead me to believe that one should call with *just*
			// SHCNE_UPDATEITEM, even if the item which has changed is a folder.
			// Anyway, I can think of no logical reason why making two calls should be *worse*
			// than making one with the two flags combined, I think splitting the flags into 
			// two calls is better
			// It has the additional merit of actually working on my XP machines...
			if(m_pathsForUpdating[nPath].IsDirectory())
			{
				TRACE("Shell Dir Update for %ws (%d)\n", m_pathsForUpdating[nPath].GetWinPathString(), GetTickCount());
				SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, m_pathsForUpdating[nPath].GetWinPath(), NULL);
			}
			else
			{
				TRACE("Shell Item Update for %ws (%d)\n", m_pathsForUpdating[nPath].GetWinPathString(), GetTickCount());
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSH, m_pathsForUpdating[nPath].GetWinPath(), NULL);
			}
		}
	}
}
