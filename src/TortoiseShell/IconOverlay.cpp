// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2017-2018, 2021 - TortoiseSVN

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
#include "ShellExt.h"
#include "PreserveChdir.h"
#include "UnicodeUtils.h"
#include "SVNStatus.h"
#include "../TSVNCache/CacheInterface.h"

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."

STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags)
{
    if (pwszIconFile == nullptr)
        return E_POINTER;
    if (pIndex == nullptr)
        return E_POINTER;
    if (pdwFlags == nullptr)
        return E_POINTER;
    if (cchMax < 1)
        return E_INVALIDARG;

    // Set "out parameters" since we return S_OK later.
    *pwszIconFile = 0;
    *pIndex       = 0;
    *pdwFlags     = 0;

    // Now here's where we can find out if due to lack of enough overlay
    // slots some of our overlays won't be shown.
    // To do that we have to mark every overlay handler that's successfully
    // loaded, so we can later check if some are missing
    switch (m_state)
    {
        case FileStateVersioned:
            g_normalOvlLoaded = true;
            break;
        case FileStateModified:
            g_modifiedOvlLoaded = true;
            break;
        case FileStateConflict:
            g_conflictedOvlLoaded = true;
            break;
        case FileStateDeleted:
            g_deletedOvlLoaded = true;
            break;
        case FileStateReadOnly:
            g_readonlyOvlLoaded = true;
            break;
        case FileStateLockedOverlay:
            g_lockedOvlLoaded = true;
            break;
        case FileStateAddedOverlay:
            g_addedOvlLoaded = true;
            break;
        case FileStateIgnoredOverlay:
            g_ignoredOvlLoaded = true;
            break;
        case FileStateUnversionedOverlay:
            g_unversionedOvlLoaded = true;
            break;
        default:
            break;
    }

    // we don't have to set the icon file and/or the index here:
    // the icons are handled by the TortoiseOverlays dll.
    return S_OK;
}

STDMETHODIMP CShellExt::GetPriority(int* pPriority)
{
    if (pPriority == nullptr)
        return E_POINTER;

    switch (m_state)
    {
        case FileStateConflict:
            *pPriority = 0;
            break;
        case FileStateModified:
            *pPriority = 1;
            break;
        case FileStateDeleted:
            *pPriority = 2;
            break;
        case FileStateAddedOverlay:
            *pPriority = 3;
            break;
        case FileStateVersioned:
            *pPriority = 4;
            break;
        case FileStateUncontrolled:
            *pPriority = 5;
            break;
        case FileStateReadOnly:
            *pPriority = 6;
            break;
        case FileStateIgnoredOverlay:
            *pPriority = 7;
            break;
        case FileStateLockedOverlay:
            *pPriority = 8;
            break;
        default:
            *pPriority = 100;
            return S_FALSE;
    }
    return S_OK;
}

// "Before painting an object's icon, the Shell passes the object's name to
//  each icon overlay handler's IShellIconOverlayIdentifier::IsMemberOf
//  method. If a handler wants to have its icon overlay displayed,
//  it returns S_OK. The Shell then calls the handler's
//  IShellIconOverlayIdentifier::GetOverlayInfo method to determine which icon
//  to display."

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD /*dwAttrib*/)
{
    if (pwszPath == nullptr)
        return E_INVALIDARG;
    const wchar_t* pPath = pwszPath;
    // the shell sometimes asks overlays for invalid paths, e.g. for network
    // printers (in that case the path is "0", at least for me here).
    if (wcslen(pPath) < 2)
        return S_FALSE;

    PreserveChdir      preserveChdir;
    svn_wc_status_kind status          = svn_wc_status_none;
    bool               readonlyOverlay = false;
    bool               lockedOverlay   = false;

    // since the shell calls each and every overlay handler with the same filepath
    // we use a small 'fast' cache of just one path here.
    // To make sure that cache expires, clear it as soon as one handler is used.

    AutoLocker lock(g_csGlobalComGuard);
    if (wcscmp(pPath, g_filePath.c_str()) == 0)
    {
        status          = g_fileStatus;
        readonlyOverlay = g_readOnlyOverlay;
        lockedOverlay   = g_lockedOverlay;
    }
    else
    {
        g_filePath.clear();
        if (!g_shellCache.IsPathAllowed(pPath))
        {
            if ((m_state == FileStateVersioned) && g_shellCache.ShowExcludedAsNormal() &&
                (PathGetDriveNumber(pPath) > 1) &&
                PathIsDirectory(pPath) && CTSVNPath(pPath).IsWCRoot())
            {
                return S_OK;
            }
            return S_FALSE;
        }
        auto cacheType = g_shellCache.GetCacheType();
        if (g_shellCache.IsOnlyNonElevated() && g_shellCache.IsProcessElevated())
        {
            cacheType = ShellCache::None;
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": elevated overlays blocked\n");
        }
        switch (cacheType)
        {
            case ShellCache::Exe:
            {
                TSVNCacheResponse itemStatus = {0};
                if (m_remoteCacheLink.GetStatusFromRemoteCache(CTSVNPath(pPath), &itemStatus, true))
                {
                    status = static_cast<svn_wc_status_kind>(itemStatus.m_status);
                    if (itemStatus.m_hasLockOwner)
                        lockedOverlay = true;
                    else if ((itemStatus.m_kind == svn_node_file) && (status == svn_wc_status_normal) && (itemStatus.m_needsLock))
                        readonlyOverlay = true;

                    if (itemStatus.m_treeConflict)
                        status = SVNStatus::GetMoreImportant(status, svn_wc_status_conflicted);
                }
            }
            break;
            case ShellCache::Dll:
            {
                // Look in our caches for this item
                const FileStatusCacheEntry* s = m_cachedStatus.GetCachedItem(CTSVNPath(pPath));
                if (s)
                {
                    if (s->treeConflict)
                        status = svn_wc_status_conflicted;
                    else
                        status = s->status;
                }
                else
                {
                    // No cached status available

                    // since the dwAttrib param of the IsMemberOf() function does not
                    // have the SFGAO_FOLDER flag set at all (it's 0 for files and folders!)
                    // we have to check if the path is a folder ourselves :(
                    if (PathIsDirectory(pPath))
                    {
                        if (g_shellCache.IsVersioned(pPath, true, true))
                        {
                            if ((!g_shellCache.IsRecursive()) && (!g_shellCache.IsFolderOverlay()))
                            {
                                status = svn_wc_status_normal;
                            }
                            else
                            {
                                const FileStatusCacheEntry* se = m_cachedStatus.GetFullStatus(CTSVNPath(pPath), TRUE);
                                if (se->treeConflict)
                                    status = svn_wc_status_conflicted;
                                else
                                    status = se->status;
                                status = SVNStatus::GetMoreImportant(svn_wc_status_normal, status);
                            }
                        }
                        else
                        {
                            status = svn_wc_status_none;
                        }
                    }
                    else
                    {
                        const FileStatusCacheEntry* se = m_cachedStatus.GetFullStatus(CTSVNPath(pPath), FALSE);
                        if (se->treeConflict)
                            status = svn_wc_status_conflicted;
                        else
                            status = se->status;
                    }
                }
                if (s != nullptr)
                {
                    const bool isOwnerEmpty = ((s->owner == nullptr) || (s->owner[0] == 0));
                    if (isOwnerEmpty)
                    {
                        if ((status == svn_wc_status_normal) && (s->needsLock))
                            readonlyOverlay = true;
                    }
                    else
                        lockedOverlay = true;
                }
            }
            break;
            default:
            case ShellCache::None:
            {
                // no cache means we only show a 'versioned' overlay on folders
                // with an admin directory
                if (PathIsDirectory(pPath))
                {
                    if (g_shellCache.IsVersioned(pPath, true, true))
                    {
                        status = svn_wc_status_normal;
                    }
                    else
                    {
                        status = svn_wc_status_none;
                    }
                }
                else
                {
                    status = svn_wc_status_none;
                }
            }
            break;
        }
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Status %d for file %s\n", status, pwszPath);
    }
    g_filePath.clear();
    g_filePath        = pPath;
    g_fileStatus      = status;
    g_readOnlyOverlay = readonlyOverlay;
    g_lockedOverlay   = lockedOverlay;

    //the priority system of the shell doesn't seem to work as expected (or as I expected):
    //as it seems that if one handler returns S_OK then that handler is used, no matter
    //if other handlers would return S_OK too (they're never called on my machine!)
    //So we return S_OK for ONLY ONE handler!
    switch (status)
    {
        // note: we can show other overlays if due to lack of enough free overlay
        // slots some of our overlays aren't loaded. But we assume that
        // at least the 'normal' overlay is available.
        // if the 'modified' overlay isn't available, we show the 'normal' overlay,
        // but in this case the overlays don't really provide anything useful anymore.
        case svn_wc_status_none:
            return S_FALSE;

        case svn_wc_status_unversioned:
            if (!g_unversionedOvlLoaded || (m_state != FileStateUnversionedOverlay))
                return S_FALSE;
            break;

        case svn_wc_status_ignored:
            if (!g_ignoredOvlLoaded || (m_state != FileStateIgnoredOverlay))
                return S_FALSE;
            break;

        case svn_wc_status_normal:
        case svn_wc_status_external:
        case svn_wc_status_incomplete:
            if ((readonlyOverlay) && (g_readonlyOvlLoaded))
            {
                if (m_state != FileStateReadOnly)
                    return S_FALSE;
            }
            else if ((lockedOverlay) && (g_lockedOvlLoaded))
            {
                if (m_state != FileStateLockedOverlay)
                    return S_FALSE;
            }
            else if (m_state != FileStateVersioned)
                return S_FALSE;
            break;

        case svn_wc_status_missing:
        case svn_wc_status_deleted:
            if (g_deletedOvlLoaded)
            {
                if (m_state != FileStateDeleted)
                    return S_FALSE;
            }
            else
            {
                // the 'deleted' overlay isn't available (due to lack of enough
                // overlay slots). So just show the 'modified' overlay instead.

                if (m_state != FileStateModified)
                    return S_FALSE;
            }
            break;

        case svn_wc_status_replaced:
        case svn_wc_status_modified:
        case svn_wc_status_merged:
            if (g_modifiedOvlLoaded)
            {
                if (m_state != FileStateModified)
                    return S_FALSE;
            }
            else
            {
                if (m_state != FileStateVersioned)
                    return S_FALSE;
            }
            break;

        case svn_wc_status_added:
            if (g_addedOvlLoaded)
            {
                if (m_state != FileStateAddedOverlay)
                    return S_FALSE;
            }
            else
            {
                // the 'added' overlay isn't available (due to lack of enough
                // overlay slots). So just show the 'modified' overlay instead.
                if (m_state != FileStateModified)
                    return S_FALSE;
            }
            break;

        case svn_wc_status_conflicted:
        case svn_wc_status_obstructed:
            if (g_conflictedOvlLoaded)
            {
                if (m_state != FileStateConflict)
                    return S_FALSE;
            }
            else
            {
                // the 'conflicted' overlay isn't available (due to lack of enough
                // overlay slots). So just show the 'modified' overlay instead.

                if (m_state != FileStateModified)
                    return S_FALSE;
            }
            break;

        default:
            return S_FALSE;
    } // switch (status)
    //return S_FALSE;

    // we want to show the overlay icon specified in m_State

    g_filePath.clear();
    return S_OK;
}
