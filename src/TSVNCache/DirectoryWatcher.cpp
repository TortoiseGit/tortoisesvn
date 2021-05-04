// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008, 2011-2012, 2014-2015, 2021 - TortoiseSVN

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
#include "stdafx.h"
#include "Dbt.h"
#include "SVNStatusCache.h"
#include "DirectoryWatcher.h"
#include "SmartHandle.h"

#include <list>

extern HWND hWndHidden;

CDirectoryWatcher::CDirectoryWatcher()
    : m_bRunning(TRUE)
    , m_bCleaned(FALSE)
    , m_folderCrawler(nullptr)
    , blockTickCount(0)
{
    // enable the required privileges for this process

    LPCWSTR arPrivelegeNames[] = {SE_BACKUP_NAME,
                                  SE_RESTORE_NAME,
                                  SE_CHANGE_NOTIFY_NAME};

    for (int i = 0; i < (sizeof(arPrivelegeNames) / sizeof(LPCWSTR)); ++i)
    {
        CAutoGeneralHandle hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, hToken.GetPointer()))
        {
            TOKEN_PRIVILEGES tp = {1};

            if (LookupPrivilegeValue(nullptr, arPrivelegeNames[i], &tp.Privileges[0].Luid))
            {
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
            }
        }
    }

    unsigned int threadId = 0;
    m_hThread             = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadEntry, this, 0, &threadId));
}

CDirectoryWatcher::~CDirectoryWatcher()
{
    Stop();
    AutoLocker lock(m_critSec);
    ClearInfoMap();
    CleanupWatchInfo();
}

void CDirectoryWatcher::CloseCompletionPort()
{
    m_hCompPort.CloseHandle();
}

void CDirectoryWatcher::ScheduleForDeletion(CDirWatchInfo* info)
{
    infoToDelete.push_back(info);
}

void CDirectoryWatcher::CleanupWatchInfo()
{
    AutoLocker lock(m_critSec);
    InterlockedExchange(&m_bCleaned, TRUE);
    while (!infoToDelete.empty())
    {
        CDirWatchInfo* info = infoToDelete.back();
        infoToDelete.pop_back();
        delete info;
    }
}

void CDirectoryWatcher::Stop()
{
    InterlockedExchange(&m_bRunning, FALSE);
    CloseWatchHandles();
    WaitForSingleObject(m_hThread, 4000);
    m_hThread.CloseHandle();
}

void CDirectoryWatcher::SetFolderCrawler(CFolderCrawler* crawler)
{
    m_folderCrawler = crawler;
}

bool CDirectoryWatcher::RemovePathAndChildren(const CTSVNPath& path)
{
    bool       bRemoved = false;
    AutoLocker lock(m_critSec);
repeat:
    for (int i = 0; i < watchedPaths.GetCount(); ++i)
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
    blockTickCount = GetTickCount64() + 4000;
}

bool CDirectoryWatcher::AddPath(const CTSVNPath& path, bool bCloseInfoMap)
{
    if (!CSVNStatusCache::Instance().IsPathAllowed(path))
        return false;
    if ((!blockedPath.IsEmpty()) && (blockedPath.IsAncestorOf(path)))
    {
        if (GetTickCount64() < blockTickCount)
        {
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Path %s prevented from being watched\n", path.GetWinPath());
            return false;
        }
    }

    // ignore the recycle bin
    PTSTR pFound = StrStrI(path.GetWinPath(), L":\\RECYCLER");
    if (pFound != nullptr)
    {
        if ((*(pFound + 10) == '\0') || (*(pFound + 10) == '\\'))
            return false;
    }
    pFound = StrStrI(path.GetWinPath(), L":\\$Recycle.Bin");
    if (pFound != nullptr)
    {
        if ((*(pFound + 14) == '\0') || (*(pFound + 14) == '\\'))
            return false;
    }

    AutoLocker lock(m_critSec);
    for (int i = 0; i < watchedPaths.GetCount(); ++i)
    {
        if (watchedPaths[i].IsAncestorOf(path))
            return false; // already watched (recursively)
    }

    // now check if with the new path we might have a new root
    CTSVNPath newRoot;
    for (int i = 0; i < watchedPaths.GetCount(); ++i)
    {
        const CString& watched = watchedPaths[i].GetWinPathString();
        const CString& sPath   = path.GetWinPathString();
        int            minLen  = min(sPath.GetLength(), watched.GetLength());
        int            len     = 0;
        for (len = 0; len < minLen; ++len)
        {
            if (watched.GetAt(len) != sPath.GetAt(len))
            {
                if ((len > 1) && (len < minLen))
                {
                    if (sPath.GetAt(len) == '\\')
                    {
                        newRoot = CTSVNPath(sPath.Left(len));
                    }
                    else if (watched.GetAt(len) == '\\')
                    {
                        newRoot = CTSVNPath(watched.Left(len));
                    }
                }
                break;
            }
        }
        if (len == minLen)
        {
            if (sPath.GetLength() == minLen)
            {
                if (watched.GetLength() > minLen)
                {
                    if (watched.GetAt(len) == '\\')
                    {
                        newRoot = path;
                    }
                    else if (sPath.GetLength() == 3 && sPath[1] == ':')
                    {
                        newRoot = path;
                    }
                }
            }
            else
            {
                if (sPath.GetLength() > minLen)
                {
                    if (sPath.GetAt(len) == '\\')
                    {
                        newRoot = CTSVNPath(watched);
                    }
                    else if (watched.GetLength() == 3 && watched[1] == ':')
                    {
                        newRoot = CTSVNPath(watched);
                    }
                }
            }
        }
    }
    if (!newRoot.IsEmpty() && SVNHelper::IsVersioned(newRoot, false))
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": AddPath for %s\n", newRoot.GetWinPath());
        watchedPaths.AddPath(newRoot);
        watchedPaths.RemoveChildren();
        if (bCloseInfoMap)
            ClearInfoMap();

        return true;
    }
    if (!SVNHelper::IsVersioned(path, false))
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Path %s prevented from being watched: not versioned\n", path.GetWinPath());
        return false;
    }
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": AddPath for %s\n", path.GetWinPath());
    watchedPaths.AddPath(path);
    if (bCloseInfoMap)
        ClearInfoMap();

    return true;
}

bool CDirectoryWatcher::IsPathWatched(const CTSVNPath& path)
{
    for (int i = 0; i < watchedPaths.GetCount(); ++i)
    {
        if (watchedPaths[i].IsAncestorOf(path))
            return true;
    }
    return false;
}

unsigned int CDirectoryWatcher::ThreadEntry(void* pContext)
{
    CCrashReportThread crashThread;
    static_cast<CDirectoryWatcher*>(pContext)->WorkerThread();
    return 0;
}

void CDirectoryWatcher::WorkerThread()
{
    DWORD          numBytes;
    CDirWatchInfo* pdi = nullptr;
    LPOVERLAPPED   lpOverlapped;
    WCHAR          buf[READ_DIR_CHANGE_BUFFER_SIZE] = {0};
    WCHAR*         pFound                           = nullptr;
    while (m_bRunning)
    {
        CleanupWatchInfo();
        if (watchedPaths.GetCount())
        {
            // Any incoming notifications?

            pdi      = nullptr;
            numBytes = 0;
            InterlockedExchange(&m_bCleaned, FALSE);
            if ((!m_hCompPort) || !GetQueuedCompletionStatus(m_hCompPort,
                                                             &numBytes,
                                                             reinterpret_cast<PULONG_PTR>(&pdi),
                                                             &lpOverlapped,
                                                             600000 /*10 minutes*/))
            {
                // No. Still trying?

                if (!m_bRunning)
                    return;

                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": restarting watcher\n");
                m_hCompPort.CloseHandle();

                // We must sync the whole section because other threads may
                // receive "AddPath" calls that will delete the completion
                // port *while* we are adding references to it .

                AutoLocker lock(m_critSec);

                // Clear the list of watched objects and recreate that list.
                // This will also delete the old completion port

                ClearInfoMap();
                CleanupWatchInfo();

                for (int i = 0; i < watchedPaths.GetCount(); ++i)
                {
                    CTSVNPath watchedPath = watchedPaths[i];

                    CAutoFile hDir = CreateFile(watchedPath.GetWinPath(),
                                                FILE_LIST_DIRECTORY,
                                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                nullptr, //security attributes
                                                OPEN_EXISTING,
                                                FILE_FLAG_BACKUP_SEMANTICS | //required privileges: SE_BACKUP_NAME and SE_RESTORE_NAME.
                                                    FILE_FLAG_OVERLAPPED,
                                                nullptr);
                    if (!hDir)
                    {
                        // this could happen if a watched folder has been removed/renamed
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": CreateFile failed. Can't watch directory %s\n", watchedPaths[i].GetWinPath());
                        watchedPaths.RemovePath(watchedPath);
                        break;
                    }

                    DEV_BROADCAST_HANDLE notificationFilter = {0};
                    notificationFilter.dbch_size            = sizeof(DEV_BROADCAST_HANDLE);
                    notificationFilter.dbch_devicetype      = DBT_DEVTYP_HANDLE;
                    notificationFilter.dbch_handle          = hDir;
                    // RegisterDeviceNotification sends a message to the UI thread:
                    // make sure we *can* send it and that the UI thread isn't waiting on a lock
                    int    numPaths = watchedPaths.GetCount();
                    size_t numWatch = watchInfoMap.size();
                    lock.Unlock();
                    notificationFilter.dbch_hdevnotify = RegisterDeviceNotification(hWndHidden, &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
                    lock.Lock();
                    // since we released the lock to prevent a deadlock with the UI thread,
                    // it could happen that new paths were added to watch, or another thread
                    // could have cleared our info map.
                    // if that happened, we have to restart watching all paths again.
                    if ((numPaths != watchedPaths.GetCount()) || (numWatch != watchInfoMap.size()))
                    {
                        ClearInfoMap();
                        CleanupWatchInfo();
                        Sleep(200);
                        break;
                    }

                    CDirWatchInfo* pDirInfo = new CDirWatchInfo(hDir.Detach(), watchedPath); // the new CDirWatchInfo object owns the handle now
                    pDirInfo->m_hDevNotify  = notificationFilter.dbch_hdevnotify;

                    CAutoGeneralHandle port = CreateIoCompletionPort(pDirInfo->m_hDir, m_hCompPort, reinterpret_cast<ULONG_PTR>(pDirInfo), 0);
                    if (!port)
                    {
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": CreateIoCompletionPort failed. Can't watch directory %s\n", watchedPath.GetWinPath());

                        // we must close the directory handle to allow ClearInfoMap()
                        // to close the completion port properly
                        pDirInfo->CloseDirectoryHandle();

                        ClearInfoMap();
                        CleanupWatchInfo();
                        delete pDirInfo;
                        pDirInfo = nullptr;

                        watchedPaths.RemovePath(watchedPath);
                        break;
                    }
                    m_hCompPort = std::move(port);

                    if (!ReadDirectoryChangesW(pDirInfo->m_hDir,
                                               pDirInfo->m_buffer,
                                               READ_DIR_CHANGE_BUFFER_SIZE,
                                               TRUE,
                                               FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                               &numBytes, // not used
                                               &pDirInfo->m_overlapped,
                                               nullptr)) //no completion routine!
                    {
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": ReadDirectoryChangesW failed. Can't watch directory %s\n", watchedPath.GetWinPath());

                        // we must close the directory handle to allow ClearInfoMap()
                        // to close the completion port properly
                        pDirInfo->CloseDirectoryHandle();

                        ClearInfoMap();
                        CleanupWatchInfo();
                        delete pDirInfo;
                        pDirInfo = nullptr;
                        watchedPaths.RemovePath(watchedPath);
                        break;
                    }

                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": watching path %s\n", pDirInfo->m_dirName.GetWinPath());
                    watchInfoMap[pDirInfo->m_hDir] = pDirInfo;
                }
            }
            else
            {
                if (!m_bRunning)
                    return;
                if (watchInfoMap.empty())
                    continue;

                // NOTE: the longer this code takes to execute until ReadDirectoryChangesW
                // is called again, the higher the chance that we miss some
                // changes in the file system!
                if (pdi)
                {
                    BOOL                 bRet = false;
                    std::list<CTSVNPath> notifyPaths;
                    {
                        AutoLocker lock(m_critSec);
                        // in case the CDirectoryWatcher objects have been cleaned,
                        // the m_bCleaned variable will be set to true here. If the
                        // objects haven't been cleared, we can access them here.
                        if (InterlockedExchange(&m_bCleaned, FALSE))
                            continue;
                        if ((!pdi->m_hDir) || watchInfoMap.empty() || (watchInfoMap.find(pdi->m_hDir) == watchInfoMap.end()))
                        {
                            continue;
                        }
                        PFILE_NOTIFY_INFORMATION pnotify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(pdi->m_buffer);
                        DWORD                    nOffset = 0;

                        do
                        {
                            pnotify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(reinterpret_cast<LPBYTE>(pnotify) + nOffset);

                            if (reinterpret_cast<ULONG_PTR>(pnotify) - reinterpret_cast<ULONG_PTR>(pdi->m_buffer) > READ_DIR_CHANGE_BUFFER_SIZE)
                                break;

                            nOffset = pnotify->NextEntryOffset;

                            if (pnotify->FileNameLength >= (READ_DIR_CHANGE_BUFFER_SIZE * sizeof(wchar_t)))
                                continue;

                            SecureZeroMemory(buf, READ_DIR_CHANGE_BUFFER_SIZE * sizeof(wchar_t));
                            wcsncpy_s(buf, pdi->m_dirPath, _countof(buf) - 1);
                            errno_t err = wcsncat_s(buf + pdi->m_dirPath.GetLength(), READ_DIR_CHANGE_BUFFER_SIZE - pdi->m_dirPath.GetLength(), pnotify->FileName, min(READ_DIR_CHANGE_BUFFER_SIZE - pdi->m_dirPath.GetLength(), static_cast<int>(pnotify->FileNameLength / sizeof(wchar_t))));
                            if (err == STRUNCATE)
                            {
                                continue;
                            }
                            buf[(pnotify->FileNameLength / sizeof(wchar_t)) + pdi->m_dirPath.GetLength()] = 0;

                            if (m_folderCrawler)
                            {
                                if ((pFound = StrStrI(buf, L"\\tmp")) != nullptr)
                                {
                                    pFound += 4;
                                    if (((*pFound) == '\\') || ((*pFound) == '\0'))
                                    {
                                        continue;
                                    }
                                }
                                if ((pFound = StrStrI(buf, L":\\RECYCLER")) != nullptr)
                                {
                                    if ((*(pFound + 10) == '\0') || (*(pFound + 10) == '\\'))
                                        continue;
                                }
                                if ((pFound = StrStrI(buf, L":\\$Recycle.Bin")) != nullptr)
                                {
                                    if ((*(pFound + 14) == '\0') || (*(pFound + 14) == '\\'))
                                        continue;
                                }

                                if (StrStrI(buf, L".tmp") != nullptr)
                                {
                                    // assume files with a .tmp extension are not versioned and interesting,
                                    // so ignore them.
                                    continue;
                                }
                                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": change notification for %s\n", buf);
                                notifyPaths.emplace_back(buf);
                            }
                        } while ((nOffset > 0) && (nOffset < READ_DIR_CHANGE_BUFFER_SIZE));

                        // setup next notification cycle

                        SecureZeroMemory(pdi->m_buffer, sizeof(pdi->m_buffer));
                        SecureZeroMemory(&pdi->m_overlapped, sizeof(OVERLAPPED));
                        bRet = ReadDirectoryChangesW(pdi->m_hDir,
                                                     pdi->m_buffer,
                                                     READ_DIR_CHANGE_BUFFER_SIZE,
                                                     TRUE,
                                                     FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                                     &numBytes, // not used
                                                     &pdi->m_overlapped,
                                                     nullptr); //no completion routine!
                    }
                    if (!notifyPaths.empty())
                    {
                        for (auto nit = notifyPaths.cbegin(); nit != notifyPaths.cend(); ++nit)
                        {
                            m_folderCrawler->AddPathForUpdate(*nit);
                        }
                    }

                    // any clean-up to do?

                    CleanupWatchInfo();

                    if (!bRet)
                    {
                        // Since the call to ReadDirectoryChangesW failed, just
                        // wait a while. We don't want to have this thread
                        // running using 100% CPU if something goes completely
                        // wrong.
                        pdi->CloseDirectoryHandle();
                        watchedPaths.RemovePath(pdi->m_dirName);
                        Sleep(200);
                        CloseCompletionPort();
                    }
                }
            }
        } // if (watchedPaths.GetCount())
        else
            Sleep(200);
    } // while (m_bRunning)
}

// call this before destroying async I/O structures:

void CDirectoryWatcher::CloseWatchHandles()
{
    AutoLocker lock(m_critSec);

    for (auto I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
        I->second->CloseDirectoryHandle();

    CloseCompletionPort();
}

void CDirectoryWatcher::ClearInfoMap()
{
    CloseWatchHandles();
    if (!watchInfoMap.empty())
    {
        AutoLocker lock(m_critSec);
        for (auto I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
        {
            auto info = I->second;
            I->second = nullptr;
            ScheduleForDeletion(info);
        }
        watchInfoMap.clear();
    }
}

CTSVNPath CDirectoryWatcher::CloseInfoMap(HANDLE hDir)
{
    AutoLocker lock(m_critSec);
    auto       d = watchInfoMap.find(hDir);
    if (d != watchInfoMap.end())
    {
        CTSVNPath root = CTSVNPath(CTSVNPath(d->second->m_dirPath).GetRootPathString());
        RemovePathAndChildren(root);
        BlockPath(root);
    }
    CloseWatchHandles();

    CTSVNPath path;
    if (watchInfoMap.empty())
        return path;

    for (auto I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
    {
        auto info = I->second;

        ScheduleForDeletion(info);
    }
    watchInfoMap.clear();

    return path;
}

bool CDirectoryWatcher::CloseHandlesForPath(const CTSVNPath& path)
{
    AutoLocker lock(m_critSec);
    CloseWatchHandles();

    if (watchInfoMap.empty())
        return false;

    for (auto I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
    {
        CDirectoryWatcher::CDirWatchInfo* info = I->second;
        I->second                              = nullptr;
        CTSVNPath p                            = CTSVNPath(info->m_dirPath);
        if (path.IsAncestorOf(p))
        {
            RemovePathAndChildren(p);
            BlockPath(p);
        }
        ScheduleForDeletion(info);
    }
    watchInfoMap.clear();
    return true;
}

CDirectoryWatcher::CDirWatchInfo::CDirWatchInfo(CAutoFile&& hDir, const CTSVNPath& directoryName)
    : m_hDir(std::move(hDir))
    , m_dirName(directoryName)
{
    ATLASSERT(m_hDir && !directoryName.IsEmpty());
    m_buffer[0] = 0;
    SecureZeroMemory(&m_overlapped, sizeof(m_overlapped));
    m_dirPath = m_dirName.GetWinPathString();
    if (m_dirPath.GetAt(m_dirPath.GetLength() - 1) != '\\')
        m_dirPath += L"\\";
    m_hDevNotify = nullptr;
}

CDirectoryWatcher::CDirWatchInfo::~CDirWatchInfo()
{
    CloseDirectoryHandle();
}

bool CDirectoryWatcher::CDirWatchInfo::CloseDirectoryHandle()
{
    bool b = m_hDir.CloseHandle();

    if (m_hDevNotify)
    {
        UnregisterDeviceNotification(m_hDevNotify);
        m_hDevNotify = nullptr;
    }
    return b;
}
