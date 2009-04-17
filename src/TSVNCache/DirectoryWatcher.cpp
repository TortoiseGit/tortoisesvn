// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "Dbt.h"
#include "SVNStatusCache.h"
#include ".\directorywatcher.h"

extern HWND hWnd;

CDirectoryWatcher::CDirectoryWatcher(void) 
    : m_hCompPort(NULL)
	, m_bRunning(TRUE)
	, m_FolderCrawler(NULL)
	, blockTickCount(0)
{
	// enable the required privileges for this process
	
	LPCTSTR arPrivelegeNames[] = {	SE_BACKUP_NAME,
									SE_RESTORE_NAME,
									SE_CHANGE_NOTIFY_NAME
								 };

	for (int i=0; i<(sizeof(arPrivelegeNames)/sizeof(LPCTSTR)); ++i)
	{
		HANDLE hToken;    
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) 	
		{        
			TOKEN_PRIVILEGES tp = { 1 };        

			if (LookupPrivilegeValue(NULL, arPrivelegeNames[i],  &tp.Privileges[0].Luid))
			{
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
			}
			CloseHandle(hToken);	
		}	
	}

	unsigned int threadId = 0;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
}

CDirectoryWatcher::~CDirectoryWatcher(void)
{
	InterlockedExchange(&m_bRunning, FALSE);
	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
	}
	AutoLocker lock(m_critSec);
	ClearInfoMap();
    CleanupWatchInfo();
}

void CDirectoryWatcher::CloseCompletionPort()
{
	if (m_hCompPort != INVALID_HANDLE_VALUE)
		if (CloseHandle (m_hCompPort) == FALSE)
            ATLTRACE (_T("Closing completion port failed\n"));

	m_hCompPort = INVALID_HANDLE_VALUE;
}

void CDirectoryWatcher::ScheduleForDeletion (CDirWatchInfo* info)
{
    infoToDelete.push_back (info);
}

void CDirectoryWatcher::CleanupWatchInfo()
{
	AutoLocker lock(m_critSec);
    while (!infoToDelete.empty())
    {
        CDirWatchInfo* info = *infoToDelete.rbegin();
        infoToDelete.pop_back();
        delete info;
    }
}

void CDirectoryWatcher::Stop()
{
	InterlockedExchange(&m_bRunning, FALSE);
	if (m_hThread != INVALID_HANDLE_VALUE)
		CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;

    CloseWatchHandles();
}

void CDirectoryWatcher::SetFolderCrawler(CFolderCrawler * crawler)
{
	m_FolderCrawler = crawler;
}

bool CDirectoryWatcher::RemovePathAndChildren(const CTSVNPath& path)
{
	bool bRemoved = false;
	AutoLocker lock(m_critSec);
repeat:
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		if (path.IsAncestorOf(watchedPaths[i]))
		{
			watchedPaths.RemovePath(watchedPaths[i]);
			bRemoved = true;
			goto repeat;
		}
	}
	return bRemoved;
}

void CDirectoryWatcher::BlockPath(const CTSVNPath& path)
{
	blockedPath = path;
	// block the path from being watched for 4 seconds
	blockTickCount = GetTickCount()+4000;
	ATLTRACE(_T("Blocking path: %s\n"), path.GetWinPath());
}

bool CDirectoryWatcher::AddPath(const CTSVNPath& path)
{
	if (!CSVNStatusCache::Instance().IsPathAllowed(path))
		return false;
	if ((!blockedPath.IsEmpty())&&(blockedPath.IsAncestorOf(path)))
	{
		if (GetTickCount() < blockTickCount)
		{
			ATLTRACE(_T("Path %s prevented from being watched\n"), path.GetWinPath());
			return false;
		}
	}
	AutoLocker lock(m_critSec);
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		if (watchedPaths[i].IsAncestorOf(path))
			return false;		// already watched (recursively)
	}

	// now check if with the new path we might have a new root
	CTSVNPath newroot;
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		const CString& watched = watchedPaths[i].GetWinPathString();
		const CString& sPath = path.GetWinPathString();
		int minlen = min(sPath.GetLength(), watched.GetLength());
		int len = 0;
		for (len = 0; len < minlen; ++len)
		{
			if (watched.GetAt(len) != sPath.GetAt(len))
			{
				if ((len > 1)&&(len < minlen))
				{
					if (sPath.GetAt(len)=='\\')
					{
						newroot = CTSVNPath(sPath.Left(len));
					}
					else if (watched.GetAt(len)=='\\')
					{
						newroot = CTSVNPath(watched.Left(len));
					}
				}
				break;
			}
		}
		if (len == minlen)
		{
			if (sPath.GetLength() == minlen)
			{
				if (watched.GetLength() > minlen)
				{
					if (watched.GetAt(len)=='\\')
					{
						newroot = path;
					}
					else if (sPath.GetLength() == 3 && sPath[1] == ':')
					{
						newroot = path;
					}
				}
			}
			else
			{
				if (sPath.GetLength() > minlen)
				{
					if (sPath.GetAt(len)=='\\')
					{
						newroot = CTSVNPath(watched);
					}
					else if (watched.GetLength() == 3 && watched[1] == ':')
					{
						newroot = CTSVNPath(watched);
					}
				}
			}
		}
	}
	if (!newroot.IsEmpty())
	{
		ATLTRACE(_T("add path to watch %s\n"), newroot.GetWinPath());
		watchedPaths.AddPath(newroot);
		watchedPaths.RemoveChildren();
		CloseInfoMap();

		return true;
	}
	ATLTRACE(_T("add path to watch %s\n"), path.GetWinPath());
	watchedPaths.AddPath(path);
	CloseInfoMap();

	return true;
}

bool CDirectoryWatcher::IsPathWatched(const CTSVNPath& path)
{
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		if (watchedPaths[i].IsAncestorOf(path))
			return true;
	}
	return false;
}

unsigned int CDirectoryWatcher::ThreadEntry(void* pContext)
{
	((CDirectoryWatcher*)pContext)->WorkerThread();
	return 0;
}

void CDirectoryWatcher::WorkerThread()
{
	DWORD numBytes;
	CDirWatchInfo * pdi = NULL;
	LPOVERLAPPED lpOverlapped;
	WCHAR buf[READ_DIR_CHANGE_BUFFER_SIZE] = {0};
	WCHAR * pFound = NULL;
	while (m_bRunning)
	{
		if (watchedPaths.GetCount())
		{
            // Any incomming notifications?

            if (   (m_hCompPort == INVALID_HANDLE_VALUE)
                || !GetQueuedCompletionStatus(m_hCompPort,
											  &numBytes,
											  (PULONG_PTR) &pdi,
											  &lpOverlapped,
											  INFINITE))
			{
                // No. Still trying?

				if (!m_bRunning)
					return;

                // We must sync the whole section because other threads may
                // receive "AddPath" calls that will delete the completion
                // port *while* we are adding references to it .

                AutoLocker lock(m_critSec);

				// Clear the list of watched objects and recreate that list.
                // This will also delete the old completion port

				ClearInfoMap();

                for (int i=0; i<watchedPaths.GetCount(); ++i)
				{
					CTSVNPath watchedPath = watchedPaths[i];

					HANDLE hDir = CreateFile(watchedPath.GetWinPath(), 
											FILE_LIST_DIRECTORY, 
											FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
											NULL, //security attributes
											OPEN_EXISTING,
											FILE_FLAG_BACKUP_SEMANTICS | //required privileges: SE_BACKUP_NAME and SE_RESTORE_NAME.
											FILE_FLAG_OVERLAPPED,
											NULL);
					if (hDir == INVALID_HANDLE_VALUE)
					{
						// this could happen if a watched folder has been removed/renamed
						ATLTRACE(_T("CDirectoryWatcher: CreateFile failed. Can't watch directory %s\n"), watchedPaths[i].GetWinPath());
						watchedPaths.RemovePath(watchedPath);
						break;
					}
					
					DEV_BROADCAST_HANDLE NotificationFilter;
					SecureZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
					NotificationFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
					NotificationFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
					NotificationFilter.dbch_handle = hDir;
					NotificationFilter.dbch_hdevnotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

					CDirWatchInfo * pDirInfo = new CDirWatchInfo(hDir, watchedPath);
					pDirInfo->m_hDevNotify = NotificationFilter.dbch_hdevnotify;

                    // Since we pass m_hCompPort to CreateIoCompletionPort, we
				    // have to set this to NULL to have that API create a new
				    // handle.
                    if (m_hCompPort == INVALID_HANDLE_VALUE)
    				    m_hCompPort = NULL;

                    HANDLE port = CreateIoCompletionPort(hDir, m_hCompPort, (ULONG_PTR)pDirInfo, 0);
					if (port == NULL)
					{
						ATLTRACE(_T("CDirectoryWatcher: CreateIoCompletionPort failed. Can't watch directory %s\n"), watchedPath.GetWinPath());

                        // we must close the directory handle to allow ClearInfoMap()
                        // to close the completion port properly
                        pDirInfo->CloseDirectoryHandle();

						ClearInfoMap();
						delete pDirInfo;
						pDirInfo = NULL;

                        watchedPaths.RemovePath(watchedPath);
						break;
					}
                    m_hCompPort = port;

					if (!ReadDirectoryChangesW(pDirInfo->m_hDir,
												pDirInfo->m_Buffer,
												READ_DIR_CHANGE_BUFFER_SIZE,
												TRUE,
												FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
												&numBytes,// not used
												&pDirInfo->m_Overlapped,
												NULL))	//no completion routine!
					{
						ATLTRACE(_T("CDirectoryWatcher: ReadDirectoryChangesW failed. Can't watch directory %s\n"), watchedPath.GetWinPath());

                        // we must close the directory handle to allow ClearInfoMap()
                        // to close the completion port properly
                        pDirInfo->CloseDirectoryHandle();

						ClearInfoMap();
						delete pDirInfo;
						pDirInfo = NULL;
						watchedPaths.RemovePath(watchedPath);
						break;
					}

					ATLTRACE(_T("watching path %s\n"), pDirInfo->m_DirName.GetWinPath());
                    watchInfoMap[pDirInfo->m_hDir] = pDirInfo;
				}
			}
			else
			{
				if (!m_bRunning)
					return;

				// NOTE: the longer this code takes to execute until ReadDirectoryChangesW
				// is called again, the higher the chance that we miss some
				// changes in the file system! 
				if (pdi)
				{
					BOOL bRet = false;

					{
						AutoLocker lock(m_critSec);
                        if (   (pdi->m_hDir == INVALID_HANDLE_VALUE)
                            || (watchInfoMap.find(pdi->m_hDir) == watchInfoMap.end()))
                        {
							continue;
                        }
					}
					PFILE_NOTIFY_INFORMATION pnotify = (PFILE_NOTIFY_INFORMATION)pdi->m_Buffer;
					DWORD nOffset = numBytes == 0 
						? 0 
						: pnotify->NextEntryOffset;

					while (nOffset > 0)
					{
						nOffset = pnotify->NextEntryOffset;
						if (pnotify->FileNameLength >= (READ_DIR_CHANGE_BUFFER_SIZE*sizeof(TCHAR)))
							continue;

						SecureZeroMemory(buf, READ_DIR_CHANGE_BUFFER_SIZE*sizeof(TCHAR));
						_tcsncpy_s(buf, READ_DIR_CHANGE_BUFFER_SIZE, pdi->m_DirPath, READ_DIR_CHANGE_BUFFER_SIZE);
						errno_t err = _tcsncat_s(buf+pdi->m_DirPath.GetLength(), READ_DIR_CHANGE_BUFFER_SIZE-pdi->m_DirPath.GetLength(), pnotify->FileName, _TRUNCATE);
						if (err == STRUNCATE)
						{
							pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
							continue;
						}
						buf[(pnotify->FileNameLength/sizeof(TCHAR))+pdi->m_DirPath.GetLength()] = 0;
						pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
						if (m_FolderCrawler)
						{
							if ((pFound = wcsstr(buf, L"\\tmp"))!=NULL)
							{
								pFound += 4;
								if (((*pFound)=='\\')||((*pFound)=='\0'))
								{
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
							}
							if ((pFound = wcsstr(buf, L":\\RECYCLER\\"))!=NULL)
							{
								if ((pFound-buf) < 5)
								{
									// a notification for the recycle bin - ignore it
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
							}
							if ((pFound = wcsstr(buf, L":\\$Recycle.Bin\\"))!=NULL)
							{
								if ((pFound-buf) < 5)
								{
									// a notification for the recycle bin - ignore it
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
							}
							if ((pFound = wcsstr(buf, L".tmp"))!=NULL)
							{
								// assume files with a .tmp extension are not versioned and interesting,
								// so ignore them.
								if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
									break;
								continue;
							}
							ATLTRACE(_T("change notification: %s\n"), buf);
							m_FolderCrawler->AddPathForUpdate(CTSVNPath(buf));
						}
						if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
							break;
					};

					// setup next notification cycle

					SecureZeroMemory (pdi->m_Buffer, sizeof(pdi->m_Buffer));
					SecureZeroMemory (&pdi->m_Overlapped, sizeof(OVERLAPPED));
					bRet = ReadDirectoryChangesW (pdi->m_hDir,
													pdi->m_Buffer,
													READ_DIR_CHANGE_BUFFER_SIZE,
													TRUE,
													FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
													&numBytes,// not used
													&pdi->m_Overlapped,
													NULL);	//no completion routine!

					// any clean-up to do?

					CleanupWatchInfo();

					if (!bRet)
					{
						// Since the call to ReadDirectoryChangesW failed, just
						// wait a while. We don't want to have this thread
						// running using 100% CPU if something goes completely
						// wrong.
						Sleep(200);
					}
				}
			}
		}// if (watchedPaths.GetCount())
		else
			Sleep(200);
	}// while (m_bRunning)
}

// call this before destroying async I/O structures:

void CDirectoryWatcher::CloseWatchHandles()
{
    AutoLocker lock(m_critSec);

    for (TInfoMap::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
        I->second->CloseDirectoryHandle();

    CloseCompletionPort();
}

void CDirectoryWatcher::ClearInfoMap()
{
    CloseWatchHandles();
	if (watchInfoMap.size() > 0)
	{
		AutoLocker lock(m_critSec);
		for (TInfoMap::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
		{
			CDirectoryWatcher::CDirWatchInfo * info = I->second;
            I->second = NULL;
			ScheduleForDeletion (info);
		}
		watchInfoMap.clear();
	}
}

CTSVNPath CDirectoryWatcher::CloseInfoMap(HDEVNOTIFY hdev)
{
    CloseWatchHandles();

    CTSVNPath path;
	if (watchInfoMap.empty())
		return path;

	AutoLocker lock(m_critSec);
	for (TInfoMap::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;
		I->second = NULL;
		if (info->m_hDevNotify == hdev)
		{
			path = info->m_DirName;
			RemovePathAndChildren(path);
			BlockPath(path);
		}

        ScheduleForDeletion (info);
	}
	watchInfoMap.clear();

	return path;
}

bool CDirectoryWatcher::CloseHandlesForPath(const CTSVNPath& path)
{
    CloseWatchHandles();

	if (watchInfoMap.empty())
		return false;

    AutoLocker lock(m_critSec);
	for (TInfoMap::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;
        I->second = NULL;
		CTSVNPath p = CTSVNPath(info->m_DirPath);
		if (path.IsAncestorOf(p))
		{
			RemovePathAndChildren(p);
			BlockPath(p);
		}
		ScheduleForDeletion (info);
	}
	watchInfoMap.clear();
	return true;
}

CDirectoryWatcher::CDirWatchInfo::CDirWatchInfo(HANDLE hDir, const CTSVNPath& DirectoryName) :
	m_hDir(hDir),
	m_DirName(DirectoryName)
{
	ATLASSERT( hDir != INVALID_HANDLE_VALUE 
		&& !DirectoryName.IsEmpty());
	SecureZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
	m_DirPath = m_DirName.GetWinPathString();
	if (m_DirPath.GetAt(m_DirPath.GetLength()-1) != '\\')
		m_DirPath += _T("\\");
	m_hDevNotify = INVALID_HANDLE_VALUE;
}

CDirectoryWatcher::CDirWatchInfo::~CDirWatchInfo()
{
	CloseDirectoryHandle();
}

bool CDirectoryWatcher::CDirWatchInfo::CloseDirectoryHandle()
{
	bool b = TRUE;
	if( m_hDir != INVALID_HANDLE_VALUE )
	{
		b = !!CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}
	if (m_hDevNotify != INVALID_HANDLE_VALUE)
	{
		UnregisterDeviceNotification(m_hDevNotify);
	}
	return b;
}

