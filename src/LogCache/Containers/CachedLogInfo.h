// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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
#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "../Streams/FileName.h"
#include "RevisionIndex.h"
#include "RevisionInfoContainer.h"
#include "SkipRevisionInfo.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

/**
 * Contains the whole cached log information:
 * It just combines the existing logInfo structure with the revision index
 * (lookup is revision=>revIndex=>revData).
 *
 * The interface is that of the logInfo component, except that it uses proper
 * revision numbers.
 *
 * As the cache root object, it is associated with a fileName. You have to load
 * and save the data explicitly.
 *
 * It also maintains a "modified" flag to check whether new data has been
 * added or removed. You don't need to call Save() if there was no change.
 */
class CCachedLogInfo
{
private:

    /**
     * Utility class that manages a lock file for this log cache.
     *
     * If TSVN fails to close it properly (e.g. due to a segfault),
     * the stale lock will be detected the next time TSVN tries to
     * aquire the lock.
     *
     * Management is as follows:
     *
     *    * {repoCacheFile}.lock will be opened exclusively by the
     *      "first" TSVN instance -> OwnsFile() is true
     *    * write cache file only if OwnsFile() is true
     *      (conflict resolution when multiple TSVN instances use
     *      the same cache)
     *    * set Hidden flag when OwnsFile() is true and reset it
     *      as soon as the CCachedLogInfo gets destroyed
     *      (detects a failed TSVN session)
     *
     *    * drop cache, if Hidden flag is set and the file can be
     *      deleted (i.e. handle is no longer hold by the responsible
     *      TSVN instance).
     */

    class CCacheFileManager
    {
    private:

#ifdef WIN32
        /// only this value means "no crash" because "0" means
        /// TSVN didn't crash *before* the .lock file was set.

        enum {NO_FAILURE = -1};

        /// if we own the file, we will keep it open

        HANDLE fileHandle;

        /// we need that info to manipulate the file attributes

        TFileName fileName;

        /// number of times this cache was not released propertly

        int failureCount;
#endif

        /// "in use" (hidden flag) file flag handling

        bool IsMarked (const TFileName& name) const;
        void SetMark (const TFileName& name);
        void ResetMark();

        /// allow for multiple failures

        bool ShouldDrop (const TFileName& name, int maxFailures);
        void UpdateMark (const TFileName& name);

        /// copying is not supported

        CCacheFileManager(const CCacheFileManager&);
        CCacheFileManager& operator=(const CCacheFileManager&);

    public:

        /// default construction / destruction

        CCacheFileManager();
        ~CCacheFileManager();

        /// call this *before* opening the file
        /// (will auto-drop crashed files etc.)

        void AutoAcquire (const TFileName& fileName, int maxFailures);

        /// call this *after* releasing a cache file
        /// (resets the "hidden" flag and closes the handle
        /// -- no-op if file is not owned)

        void AutoRelease();

        /// If this returns false, you should not write the file.

        bool OwnsFile() const;
    };

    /// where we load / save our cached data

    TFileName fileName;

    /// crash detection.

    CCacheFileManager fileManager;

    /// revision index and the log info itself

    CRevisionIndex revisions;
    CRevisionInfoContainer logInfo;
    CSkipRevisionInfo skippedRevisions;

    /// revision has been added or Clear() has been called

    bool modified;

    /// revision has been added (otherwise, AddChange is forbidden)

    bool revisionAdded;

    /// stream IDs

    enum
    {
        REVISIONS_STREAM_ID = 1,
        LOG_INFO_STREAM_ID = 2,
        SKIP_REVISIONS_STREAM_ID = 3
    };

public:

    /// for convenience

    typedef CRevisionInfoContainer::TChangeAction TChangeAction;

    /// construction / destruction (nothing to do)

    CCachedLogInfo();
    CCachedLogInfo (const TFileName& aFileName);
    ~CCachedLogInfo (void);

    /// cache persistence

    void Load (int maxFailures);
    bool IsModified() const;
    bool IsEmpty() const;
    void Save();
    void Save (const TFileName& newFileName);

    /// data access

    const TFileName& GetFileName() const;
    const CRevisionIndex& GetRevisions() const;
    const CRevisionInfoContainer& GetLogInfo() const;
    const CSkipRevisionInfo& GetSkippedRevisions() const;

    /// find the highest revision not exceeding the given timestamp

    revision_t FindRevisionByDate (__time64_t maxTimeStamp) const;

    /// data modification
    /// (mirrors CRevisionInfoContainer and CSkipRevisionInfo)

    void Insert ( revision_t revision
                , const std::string& author
                , const std::string& comment
                , __time64_t timeStamp
                , char flags = CRevisionInfoContainer::HAS_STANDARD_INFO);

    void AddChange ( TChangeAction action
                   , node_kind_t pathType
                   , const std::string& path
                   , const std::string& fromPath
                   , revision_t fromRevision
                   , unsigned char text_modified
                   , unsigned char props_modified);

    void AddMergedRevision ( const std::string& fromPath
                           , const std::string& toPath
                           , revision_t revisionStart
                           , revision_t revisionDelta);

    void AddRevProp ( const std::string& revProp
                    , const std::string& value);

    void AddSkipRange ( const CDictionaryBasedPath& path
                      , revision_t startRevision
                      , revision_t count);

    void Clear();

    /// return false if concurrent read accesses
    /// would potentially access invalid data.

    bool CanInsertThreadSafely ( revision_t revision
                               , const std::string& author
                               , const std::string& comment
                               , __time64_t timeStamp) const;

    bool CanAddChangeThreadSafely ( TChangeAction action
                                  , node_kind_t pathType
                                  , const std::string& path
                                  , const std::string& fromPath
                                  , revision_t fromRevision) const;

    bool CanAddRevPropThreadSafely ( const std::string& revProp
                                   , const std::string& value) const;

    /// update / modify existing data

    void Update ( const CCachedLogInfo& newData
                , char flags = CRevisionInfoContainer::HAS_ALL
                , bool keepOldDataForMissingNew = true);

    /// for statistics

    friend class CLogCacheStatistics;
};

///////////////////////////////////////////////////////////////
// cache persistence
///////////////////////////////////////////////////////////////

inline bool CCachedLogInfo::IsModified() const
{
    return modified;
}

inline void CCachedLogInfo::Save()
{
    Save (fileName);
}

///////////////////////////////////////////////////////////////
// data access
///////////////////////////////////////////////////////////////

inline const TFileName& CCachedLogInfo::GetFileName() const
{
    return fileName;
}

inline const CRevisionIndex& CCachedLogInfo::GetRevisions() const
{
    return revisions;
}

inline const CRevisionInfoContainer& CCachedLogInfo::GetLogInfo() const
{
    return logInfo;
}

inline const CSkipRevisionInfo& CCachedLogInfo::GetSkippedRevisions() const
{
    return skippedRevisions;
}

///////////////////////////////////////////////////////////////
// data modification (mirrors CRevisionInfoContainer)
///////////////////////////////////////////////////////////////

inline void CCachedLogInfo::AddChange ( TChangeAction action
                                      , node_kind_t pathType
                                      , const std::string& path
                                      , const std::string& fromPath
                                      , revision_t fromRevision
                                      , unsigned char text_modified
                                      , unsigned char props_modified)
{
    assert (revisionAdded);
    logInfo.AddChange (action, pathType, path, fromPath, fromRevision, text_modified, props_modified);
}

inline void CCachedLogInfo::AddMergedRevision ( const std::string& fromPath
                                              , const std::string& toPath
                                              , revision_t revisionStart
                                              , revision_t revisionDelta)
{
    assert (revisionAdded);
    logInfo.AddMergedRevision (fromPath, toPath, revisionStart, revisionDelta);
}

inline void CCachedLogInfo::AddRevProp ( const std::string& revProp
                                       , const std::string& value)
{
    assert (revisionAdded);
    logInfo.AddRevProp (revProp, value);
}

///////////////////////////////////////////////////////////////
// 'pseudo-thread-safe' variants of the above:
///////////////////////////////////////////////////////////////

inline bool
CCachedLogInfo::CanAddChangeThreadSafely
    ( TChangeAction action
    , node_kind_t pathType
    , const std::string& path
    , const std::string& fromPath
    , revision_t fromRevision) const
{
    assert (revisionAdded);
    return logInfo.CanAddChangeThreadSafely
        (action, pathType, path, fromPath, fromRevision);
}

inline bool
CCachedLogInfo::CanAddRevPropThreadSafely
    ( const std::string& revProp
    , const std::string& value) const
{
    assert (revisionAdded);
    return logInfo.CanAddRevPropThreadSafely (revProp, value);
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

