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
#include "stdafx.h"
#include "RevisionInfoContainer.h"

// begin namespace LogCache

namespace LogCache
{

// update / modify utilities

void CRevisionInfoContainer::UpdateAuthors
    ( const CRevisionInfoContainer& newData
    , const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
    index_mapping_t idMapping = authorPool.Merge (newData.authorPool);

    for (index_mapping_t::const_iterator iter = indexMap.begin()
            , end = indexMap.end()
                    ; iter != end
            ; ++iter)
    {
        if (!keepOldDataForMissingNew
                || (newData.presenceFlags[iter->value] & HAS_AUTHOR))
        {
            authors[iter->key] = *idMapping.find (newData.authors[iter->value]);
        }
    }
}

void CRevisionInfoContainer::UpdateTimeStamps
    ( const CRevisionInfoContainer& newData
    , const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
    for (index_mapping_t::const_iterator iter = indexMap.begin()
            , end = indexMap.end()
                    ; iter != end
            ; ++iter)
    {
        if (!keepOldDataForMissingNew
                || (newData.presenceFlags[iter->value] & HAS_TIME_STAMP))
        {
            timeStamps[iter->key] = newData.timeStamps[iter->value];
        }
    }
}

void CRevisionInfoContainer::UpdateComments
    ( const CRevisionInfoContainer& newData
    , const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
    index_mapping_t toReplace;

    for (index_mapping_t::const_iterator iter = indexMap.begin()
            , end = indexMap.end()
                    ; iter != end
            ; ++iter)
    {
        if (!keepOldDataForMissingNew
                || (newData.presenceFlags[iter->value] & HAS_COMMENT))
        {
            toReplace.insert (iter->key, iter->value);
        }
    }

    comments.Replace (newData.comments, toReplace);
}

void CRevisionInfoContainer::UpdateChanges
    ( const CRevisionInfoContainer& newData
    , const index_mapping_t& indexMap
    , const index_mapping_t& pathIDMapping
    , bool keepOldDataForMissingNew)
{
    // save & remove old data

    std::vector<unsigned char> oldChanges;
    changes.swap (oldChanges);
    changes.reserve (oldChanges.size());

    std::vector<index_t> oldChangedPaths;
    changedPaths.swap (oldChangedPaths);
    changedPaths.reserve (oldChangedPaths.size());

    std::vector<node_kind_t> oldChangedPathTypes;
    changedPathTypes.swap (oldChangedPathTypes);
    changedPathTypes.reserve (oldChangedPathTypes.size());

    std::vector<index_t> oldCopyFromPaths;
    copyFromPaths.swap (oldCopyFromPaths);
    copyFromPaths.reserve (oldCopyFromPaths.size());

    std::vector<revision_t> oldCopyFromRevisions;
    copyFromRevisions.swap (oldCopyFromRevisions);
    copyFromRevisions.reserve (oldCopyFromRevisions.size());

    std::vector<unsigned char> oldTextModifies;
    textModifies.swap (oldTextModifies);
    textModifies.reserve (oldTextModifies.size());

    std::vector<unsigned char> oldPropsModifies;
    propsModifies.swap (oldPropsModifies);
    propsModifies.reserve (oldPropsModifies.size());


    // the container sizes must match

    assert (changesOffsets.size() == size() +1);
    assert (copyFromOffsets.size() == size() +1);

    // splice

    index_t firstChange = 0;
    index_t firstCopy = 0;

    index_mapping_t::const_iterator mapEnd = indexMap.end();
    for (index_t i = 0, count = size(); i < count; ++i)
    {
        index_mapping_t::const_iterator iter = indexMap.find (i);
        if ( (iter != mapEnd)
                && (!keepOldDataForMissingNew
                    || (newData.presenceFlags[iter->value] & HAS_CHANGEDPATHS)))
        {
            // copy & translate

            index_t sourceIndex = iter->value;
            for (index_t k = newData.changesOffsets [sourceIndex]
                , last = newData.changesOffsets [sourceIndex+1]
                ; k != last
                ; ++k)
            {
                changes.push_back (newData.changes[k]);
                changedPathTypes.push_back (newData.changedPathTypes[k]);
                changedPaths.push_back (*pathIDMapping.find (newData.changedPaths[k]));
                textModifies.push_back(newData.textModifies[k]);
                propsModifies.push_back(newData.propsModifies[k]);
            }

            for (index_t k = newData.copyFromOffsets [sourceIndex]
                , last = newData.copyFromOffsets [sourceIndex+1]
                ; k != last
                ; ++k)
            {
                copyFromRevisions.push_back (newData.copyFromRevisions[k]);
                copyFromPaths.push_back (*pathIDMapping.find (newData.copyFromPaths[k]));
            }

            rootPaths[i] = *pathIDMapping.find (newData.rootPaths[sourceIndex]);
            sumChanges[i] = newData.sumChanges[sourceIndex];
        }
        else
        {
            // keep existing data

            index_t lastChange = changesOffsets[i+1];
            index_t lastCopy = copyFromOffsets[i+1];

            // standard per-path info

            changes.insert (changes.end()
                            , oldChanges.begin() + firstChange
                            , oldChanges.begin() + lastChange);
            changedPathTypes.insert (changedPathTypes.end()
                                     , oldChangedPathTypes.begin() + firstChange
                                     , oldChangedPathTypes.begin() + lastChange);
            changedPaths.insert (changedPaths.end()
                                 , oldChangedPaths.begin() + firstChange
                                 , oldChangedPaths.begin() + lastChange);

            textModifies.insert (textModifies.end()
                                , oldTextModifies.begin() + firstChange
                                , oldTextModifies.begin() + lastChange);

            propsModifies.insert (propsModifies.end()
                                , oldPropsModifies.begin() + firstChange
                                , oldPropsModifies.begin() + lastChange);

            // copy-from info, if available

            if (firstCopy != lastCopy)
            {
                copyFromPaths.insert (copyFromPaths.end()
                                      , oldCopyFromPaths.begin() + firstCopy
                                      , oldCopyFromPaths.begin() + lastCopy);
                copyFromRevisions.insert (copyFromRevisions.end()
                                          , oldCopyFromRevisions.begin() + firstCopy
                                          , oldCopyFromRevisions.begin() + lastCopy);
            }
        }

        // update positions

        firstChange = changesOffsets[i+1];
        firstCopy = copyFromOffsets[i+1];

        changesOffsets[i+1] = static_cast<index_t> (changes.size());
        copyFromOffsets[i+1] = static_cast<index_t> (copyFromPaths.size());
    }
}

void CRevisionInfoContainer::UpdateMergers
    ( const CRevisionInfoContainer& newData
    , const index_mapping_t& indexMap
    , const index_mapping_t& pathIDMapping
    , bool keepOldDataForMissingNew)
{
    // save & remove old data

    std::vector<index_t> oldMergedFromPaths;
    mergedFromPaths.swap (oldMergedFromPaths);
    mergedFromPaths.reserve (oldMergedFromPaths.size());

    std::vector<index_t> oldMergedToPaths;
    mergedToPaths.swap (oldMergedToPaths);
    mergedToPaths.reserve (oldMergedToPaths.size());

    std::vector<revision_t> oldMergedRangeStarts;
    mergedRangeStarts.swap (oldMergedRangeStarts);
    mergedRangeStarts.reserve (oldMergedRangeStarts.size());

    std::vector<revision_t> oldMergedRangeDeltas;
    mergedRangeDeltas.swap (oldMergedRangeDeltas);
    mergedRangeDeltas.reserve (oldMergedRangeDeltas.size());

    // the container sizes must match

    assert (mergedRevisionsOffsets.size() == size() +1);

    // splice

    index_t firstMerge = 0;
    index_mapping_t::const_iterator mapEnd = indexMap.end();
    for (index_t i = 0, count = size(); i < count; ++i)
    {
        index_mapping_t::const_iterator iter = indexMap.find (i);
        if ( (iter != mapEnd)
                && (!keepOldDataForMissingNew
                    || (newData.presenceFlags[iter->value] & HAS_MERGEINFO)))
        {
            // copy & translate

            index_t sourceIndex = iter->value;
            for (index_t k = newData.mergedRevisionsOffsets [sourceIndex]
                , last = newData.mergedRevisionsOffsets [sourceIndex+1]
                ; k != last
                ; ++k)
            {
                mergedFromPaths.push_back (*pathIDMapping.find (newData.mergedFromPaths[k]));
                mergedToPaths.push_back (*pathIDMapping.find (newData.mergedToPaths[k]));

                mergedRangeStarts.push_back (newData.mergedRangeStarts[k]);
                mergedRangeDeltas.push_back (newData.mergedRangeDeltas[k]);
            }
        }
        else
        {
            // keep existing data

            index_t lastMerge = mergedRevisionsOffsets[i+1];

            mergedFromPaths.insert (mergedFromPaths.end()
                                    , oldMergedFromPaths.begin() + firstMerge
                                    , oldMergedFromPaths.begin() + lastMerge);
            mergedToPaths.insert (mergedToPaths.end()
                                  , oldMergedToPaths.begin() + firstMerge
                                  , oldMergedToPaths.begin() + lastMerge);
            mergedRangeStarts.insert (mergedRangeStarts.end()
                                      , oldMergedRangeStarts.begin() + firstMerge
                                      , oldMergedRangeStarts.begin() + lastMerge);
            mergedRangeDeltas.insert (mergedRangeDeltas.end()
                                      , oldMergedRangeDeltas.begin() + firstMerge
                                      , oldMergedRangeDeltas.begin() + lastMerge);
        }

        // update positions

        firstMerge = mergedRevisionsOffsets[i+1];
        mergedRevisionsOffsets[i+1] = static_cast<index_t> (mergedFromPaths.size());
    }
}

void CRevisionInfoContainer::UpdateUserRevProps
    ( const CRevisionInfoContainer& newData
    , const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
    index_mapping_t propIDMapping
    = userRevPropsPool.Merge (newData.userRevPropsPool);

    index_mapping_t oldValueMapping;
    index_mapping_t newValueMapping;

    // save & remove old data

    std::vector<index_t> oldUserRevPropNames;
    userRevPropNames.swap (oldUserRevPropNames);
    userRevPropNames.reserve (oldUserRevPropNames.size());

    CTokenizedStringContainer oldUserRevPropValues;
    userRevPropValues.swap (oldUserRevPropValues);

    static const std::string emptyValue;
    userRevPropValues.Insert (emptyValue,  userRevPropValues.size()
                              + newData.userRevPropValues.size());

    // the container sizes must match

    assert (userRevPropOffsets.size() == size() +1);

    // splice

    index_t firstProp = 0;
    index_t valueIndex = 0;

    index_mapping_t::const_iterator mapEnd = indexMap.end();
    for (index_t i = 0, count = size(); i < count; ++i)
    {
        index_mapping_t::const_iterator iter = indexMap.find (i);
        if ( (iter != mapEnd)
                && (!keepOldDataForMissingNew
                    || (newData.presenceFlags[iter->value] & HAS_USERREVPROPS)))
        {
            // copy & translate

            if (newData.userRevPropNames.size())
            {
                index_t sourceIndex = iter->value;
                for (index_t k = newData.userRevPropOffsets [sourceIndex]
                                 , last = newData.userRevPropOffsets [sourceIndex+1]
                                          ; k != last
                        ; ++k)
                {
                    userRevPropNames.push_back (*propIDMapping.find (newData.userRevPropNames [k]));
                    newValueMapping.insert (valueIndex++, k);
                }
            }
        }
        else
        {
            // keep existing data

            index_t lastProp = userRevPropOffsets[i+1];

            userRevPropNames.insert (userRevPropNames.end()
                                     , oldUserRevPropNames.begin() + firstProp
                                     , oldUserRevPropNames.begin() + lastProp);

            for (index_t k = firstProp; k != lastProp; ++k)
                oldValueMapping.insert (valueIndex++, k);
        }

        // update positions

        firstProp = userRevPropOffsets[i+1];
        userRevPropOffsets[i+1] = static_cast<index_t> (userRevPropNames.size());
    }

    // actually write the combined rev prop values

    userRevPropValues.Replace (oldUserRevPropValues, oldValueMapping);
    userRevPropValues.Replace (newData.userRevPropValues, newValueMapping);
}

void CRevisionInfoContainer::UpdatePresenceFlags
    ( const CRevisionInfoContainer& newData
    , const index_mapping_t& indexMap
    , char flags
    , bool keepOldDataForMissingNew)
{
    for ( index_mapping_t::const_iterator iter = indexMap.begin()
        , end = indexMap.end()
        ; iter != end
        ; ++iter)
    {
        // values that have been copied

        char setValues = newData.presenceFlags[iter->value] & flags;

        if (keepOldDataForMissingNew)
            presenceFlags[iter->key] |= setValues;
        else
            presenceFlags[iter->key] = (presenceFlags[iter->key] & ~flags)
                                       | setValues;
    }
}

void CRevisionInfoContainer::AppendNewEntries
(const index_mapping_t& indexMap)
{
    // count the number of new entries required

    size_t oldCount = size();
    size_t toAppend = 0;

    for ( index_mapping_t::const_iterator iter = indexMap.begin()
        , end = indexMap.end()
        ; iter != end
        ; ++iter)
    {
        if (iter->key >= oldCount)
            ++toAppend;
    }

    // append new entries

    if (toAppend > 0)
    {
        // append a new revision info (no info available)

        authors.insert (authors.end(), toAppend, 0);
        timeStamps.insert (timeStamps.end(), toAppend, 0);
        presenceFlags.insert (presenceFlags.end(), toAppend, 0);
        rootPaths.insert (rootPaths.end(), toAppend, (index_t) NO_INDEX);
        sumChanges.insert (sumChanges.end(), toAppend, 0);

        static const std::string emptyComment;
        comments.Insert (emptyComment, toAppend);

        // all changes, revprops and merge info is empty as well

        changesOffsets.insert (changesOffsets.end(), toAppend, changesOffsets.back());
        copyFromOffsets.insert (copyFromOffsets.end(), toAppend, copyFromOffsets.back());
        mergedRevisionsOffsets.insert (mergedRevisionsOffsets.end(), toAppend, mergedRevisionsOffsets.back());
        userRevPropOffsets.insert (userRevPropOffsets.end(), toAppend, userRevPropOffsets.back());
    }
}

// sort authors by frequency
// (reduces average diff between authors -> less disk space
// and clusters author access -> cache friendly)

namespace
{

    struct SPerAuthorInfo
    {
        index_t frequency;
        index_t author;

        SPerAuthorInfo (index_t author)
            : frequency (0)
            , author (author)
        {
        }

        bool operator< (const SPerAuthorInfo& rhs) const
        {
            return (frequency > rhs.frequency)
                   || ( (frequency == rhs.frequency)
                        && (author < rhs.author));
        }
    };
}

void CRevisionInfoContainer::OptimizeAuthors()
{
    // initialize the counter array: counter[author].author == author

    std::vector<SPerAuthorInfo> counter;
    counter.reserve (authorPool.size());

    for (index_t i = 0, count = authorPool.size(); i < count; ++i)
        counter.push_back (i);

    // count occurrences

    for ( std::vector<index_t>::const_iterator iter = authors.begin()
        , end = authors.end()
        ; iter != end
        ; ++iter)
    {
        ++counter[*iter].frequency;
    }

    // sort by frequency and "age", but keep empty string untouched

    std::sort (++counter.begin(), counter.end());

    // re-arrange authors according to this new order

    std::vector<index_t> indices;
    indices.reserve (authorPool.size());

    for (index_t i = 0, count = authorPool.size(); i < count; ++i)
        indices.push_back (counter[i].author);

    authorPool.Reorder (indices);

    // re-map the author indices

    for (index_t i = 0, count = authorPool.size(); i < count; ++i)
        indices [counter[i].author] = i;

    for (std::vector<index_t>::iterator iter = authors.begin()
        , end = authors.end()
        ; iter != end
        ; ++iter)
    {
        *iter = indices[*iter];
    }
}

// sort changes by pathID; parent change still before leave change
// (reduces average diff between changes -> less disk space
// and sorts path access patterns -> cache friendly)

namespace
{
    struct SPerChangeInfo
    {
        index_t changedPath;
        node_kind_t changedPathType;
        unsigned char change;
        index_t copyFromPath;
        revision_t copyFromRevision;
        unsigned char textModifies;
        unsigned char propsModifies;

        SPerChangeInfo (const CRevisionInfoContainer::CChangesIterator& iter)
            : changedPath (iter->GetPathID())
            , changedPathType (iter->GetPathType())
            , change ( (unsigned char) iter->GetRawChange() )
            , textModifies ( (unsigned char) iter->GetTextModifies() )
            , propsModifies ( (unsigned char) iter->GetPropsModifies() )
        {
            if (iter->HasFromPath())
            {
                copyFromPath = iter->GetFromPathID();
                copyFromRevision = iter->GetFromRevision();
            }
        }

        bool operator< (const SPerChangeInfo& rhs) const
        {
            // report deletes of a given path *before* any other change
            // (probably an "add") to this path

            return (changedPath < rhs.changedPath)
                   || ( (changedPath == rhs.changedPath)
                        && (change > rhs.change));
        }
    };
}

void CRevisionInfoContainer::OptimizeChangeOrder()
{
    std::vector<SPerChangeInfo> revisionChanges;
    for (index_t index = 0, count = size(); index < count; ++index)
    {
        // collect all changes of this revision

        revisionChanges.clear();

        for (CChangesIterator iter = GetChangesBegin (index)
            , end = GetChangesEnd (index)
            ; iter != end
            ; ++iter)
        {
            revisionChanges.push_back (iter);
        }

        // sort them by path

        std::sort (revisionChanges.begin(), revisionChanges.end());

        // overwrite old data with new ordering

        index_t changeOffset = changesOffsets[index];
        index_t copyFromOffset = copyFromOffsets[index];

        for (std::vector<SPerChangeInfo>::iterator iter = revisionChanges.begin()
            , end = revisionChanges.end()
            ; iter != end
            ; ++iter)
        {
            changedPaths[changeOffset] = iter->changedPath;
            changedPathTypes[changeOffset] = iter->changedPathType;
            changes[changeOffset] = iter->change;
            textModifies[changeOffset] = iter->textModifies;
            propsModifies[changeOffset] = iter->propsModifies;
            ++changeOffset;

            if (iter->change & HAS_COPY_FROM)
            {
                copyFromPaths[copyFromOffset] = iter->copyFromPath;
                copyFromRevisions[copyFromOffset] = iter->copyFromRevision;
                ++copyFromOffset;
            }
        }

        // we must have used the exact same memory

        assert (changeOffset == changesOffsets[index+1]);
        assert (copyFromOffset == copyFromOffsets[index+1]);
    }
}

// construction / destruction

CRevisionInfoContainer::CRevisionInfoContainer (void)
    : storedSize (0)
{
    // [lastIndex+1] must point to [size()]

    changesOffsets.push_back (0);
    copyFromOffsets.push_back (0);
    mergedRevisionsOffsets.push_back (0);
    userRevPropOffsets.push_back (0);
}

CRevisionInfoContainer::~CRevisionInfoContainer (void)
{
}

// add information
// AddChange() always adds to the last revision

index_t CRevisionInfoContainer::Insert ( const std::string& author
                                       , const std::string& comment
                                       , __time64_t timeStamp
                                       , char flags)
{
    // this should newer throw as there are usually more
    // changes than revisions. But you never know ...

    if (authors.size() == NO_INDEX)
        throw CContainerException ("revision container overflow");

    // store the given revision info

    authors.push_back (authorPool.AutoInsert (author.c_str()));
    timeStamps.push_back (timeStamp);
    comments.Insert (comment);
    presenceFlags.push_back (flags);

    // no changes yet -> no common root path info

    rootPaths.push_back ( (index_t) NO_INDEX);
    sumChanges.push_back (0);

    // empty range for changes

    changesOffsets.push_back (changesOffsets.back());
    copyFromOffsets.push_back (copyFromOffsets.back());
    mergedRevisionsOffsets.push_back (mergedRevisionsOffsets.back());
    userRevPropOffsets.push_back (userRevPropOffsets.back());

    // ready

    return size()-1;
}

void CRevisionInfoContainer::AddChange ( TChangeAction action
                                       , node_kind_t pathType
                                       , const std::string& path
                                       , const std::string& fromPath
                                       , revision_t fromRevision
                                       , unsigned char text_modified
                                       , unsigned char props_modified)
{
    assert (presenceFlags.back() & HAS_CHANGEDPATHS);

    // under x64, there might actually be an overflow

    if (changes.size() == NO_INDEX)
        throw CContainerException ("revision container change list overflow");

    // add change path index and update the root path
    // of all changes in this revision

    CDictionaryBasedPath parsedPath (&paths, path, false);
    changedPaths.push_back (parsedPath.GetIndex());

    index_t& rootPathIndex = rootPaths.back();
    rootPathIndex = rootPathIndex == NO_INDEX
                    ? parsedPath.GetIndex()
                    : parsedPath.GetCommonRoot (rootPathIndex).GetIndex();

    // store the node kind as well

    changedPathTypes.push_back (pathType);

    // add changes info (flags), and indicate presence of fromPath (if given)

    char flags = fromPath.empty()
                 ? (char) action
                 : (char) action | HAS_COPY_FROM;

    sumChanges.back() |= flags;
    changes.push_back (flags);

    if (!fromPath.empty())
    {
        // add fromPath info

        CDictionaryBasedPath parsedFromPath (&paths, fromPath, false);
        copyFromPaths.push_back (parsedFromPath.GetIndex());
        copyFromRevisions.push_back (fromRevision);

        ++copyFromOffsets.back();
    }

    textModifies.push_back(text_modified);
    propsModifies.push_back(props_modified);
    // another change

    ++changesOffsets.back();
}

void CRevisionInfoContainer::AddMergedRevision ( const std::string& fromPath
                                               , const std::string& toPath
                                               , revision_t revisionStart
                                               , revision_t revisionDelta)
{
    assert (revisionDelta != 0);
    assert (presenceFlags.back() & HAS_MERGEINFO);

    // under x64, there might actually be an overflow

    if (mergedRangeStarts.size() == NO_INDEX)
        throw CContainerException ("revision container change list overflow");

    // another merge

    ++mergedRevisionsOffsets.back();

    // add merged revision range and path info

    CDictionaryBasedPath parsedFromPath (&paths, fromPath, false);
    mergedFromPaths.push_back (parsedFromPath.GetIndex());
    CDictionaryBasedPath parsedToPath (&paths, toPath, false);
    mergedToPaths.push_back (parsedToPath.GetIndex());

    mergedRangeStarts.push_back (revisionStart);
    mergedRangeDeltas.push_back (revisionDelta);
}

void CRevisionInfoContainer::AddRevProp ( const std::string& revProp
                                        , const std::string& value)
{
    static const std::string svnAuthor ("svn:author");
    static const std::string svnDate ("svn:date");
    static const std::string svnLog ("svn:log");

    // store standard rev-props somewhere else

    if (revProp == svnAuthor)
    {
        // update the revision info

        assert (presenceFlags.back() & HAS_AUTHOR);
        authors.back() = authorPool.AutoInsert (value.c_str());
    }
    else if (revProp == svnDate)
    {
        // update the revision info

        assert (presenceFlags.back() & HAS_TIME_STAMP);

        __time64_t timeStamp = 0;
        if (!value.empty())
        {
            tm time = {0,0,0, 0,0,0, 0,0,0};
            int musecs = 0;
            sscanf_s ( value.c_str()
                     , "%04d-%02d-%02dT%02d:%02d:%02d.%06d"
                     , &time.tm_year
                     , &time.tm_mon
                     , &time.tm_mday
                     , &time.tm_hour
                     , &time.tm_min
                     , &time.tm_sec
                     , &musecs);
            time.tm_isdst = 0;
            time.tm_year -= 1900;
            time.tm_mon -= 1;

        #ifdef _WIN32
            timeStamp = _mkgmtime64 (&time) *1000000 + musecs;
        #else
            timeStamp = mktime (&time);
            timeStamp = timeStamp * 1000000 + musecs;
        #endif
        }

        timeStamps.back() = timeStamp;
    }
    else if (revProp == svnLog)
    {
        // update the revision info

        assert (presenceFlags.back() & HAS_COMMENT);
        comments.Remove (comments.size()-1);
        comments.Insert (value);
    }
    else
    {
        // it's a user rev prop

        assert (presenceFlags.back() & HAS_USERREVPROPS);

        // under x64, there might actually be an overflow

        if (userRevPropNames.size() == NO_INDEX)
            throw CContainerException ("revision container change list overflow");

        // another revProp

        ++userRevPropOffsets.back();

        // add (revPropName, value) pair

        userRevPropNames.push_back (userRevPropsPool.AutoInsert (revProp.c_str()));
        userRevPropValues.Insert (value);
    }
}

// return false if concurrent read accesses
// would potentially access invalid data.

bool CRevisionInfoContainer::CanInsertThreadSafely
    ( const std::string& author
    , const std::string& comment
    , __time64_t) const
{
    // will any of our containers need to be re-allocated?

    if (   (authorPool.Find (author.c_str()) == NO_INDEX)
        || !comments.CanInsertThreadSafely (comment)
        || (authors.capacity() == authors.size())
        || (timeStamps.capacity() == timeStamps.size())

        || (presenceFlags.capacity() == presenceFlags.size())
        || (rootPaths.capacity() == rootPaths.size())
        || (sumChanges.capacity() == sumChanges.size())

        || (changesOffsets.capacity() == changesOffsets.size())
        || (copyFromOffsets.capacity() == copyFromOffsets.size())
        || (mergedRevisionsOffsets.capacity() == mergedRevisionsOffsets.size())
        || (userRevPropOffsets.capacity() == userRevPropOffsets.size()))
    {
        return false;
    }

    // all fine

    return true;
}

bool CRevisionInfoContainer::CanAddChangeThreadSafely
    ( TChangeAction
    , node_kind_t
    , const std::string& path
    , const std::string& fromPath
    , revision_t) const
{
    // will any of our containers need to be re-allocated?

    if (   !CDictionaryBasedPath::CanParsePathThreadSafely (&paths, path)
        || (changedPaths.capacity() == changedPaths.size())
        || (changedPathTypes.capacity() == changedPathTypes.size())
        || (changes.capacity() == changes.size()))
    {
        return false;
    }

    // add changes info (flags), and indicate presence of fromPath (if given)

    if (!fromPath.empty())
        if (   !CDictionaryBasedPath::CanParsePathThreadSafely (&paths, fromPath)
            || (copyFromPaths.capacity() == copyFromPaths.size())
            || (copyFromRevisions.capacity() == copyFromRevisions.size()))
        {
            return false;
        }

    // all fine

    return true;
}

bool CRevisionInfoContainer::CanAddMergedRevisionThreadSafely
    ( const std::string& fromPath
    , const std::string& toPath
    , revision_t
    , revision_t) const
{
    // will any of our containers need to be re-allocated?

    if (   !CDictionaryBasedPath::CanParsePathThreadSafely (&paths, fromPath)
        || !CDictionaryBasedPath::CanParsePathThreadSafely (&paths, toPath)
        || (mergedFromPaths.capacity() == mergedFromPaths.size())
        || (mergedToPaths.capacity() == mergedToPaths.size())
        || (mergedRangeStarts.capacity() == mergedRangeStarts.size())
        || (mergedRangeDeltas.capacity() == mergedRangeDeltas.size()))
    {
        return false;
    }

    // all fine

    return true;
}

bool CRevisionInfoContainer::CanAddRevPropThreadSafely
    ( const std::string& revProp
    , const std::string& value) const
{
    static const std::string svnAuthor ("svn:author");
    static const std::string svnDate ("svn:date");
    static const std::string svnLog ("svn:log");

    // store standard rev-props somewhere else

    if (revProp == svnAuthor)
    {
        return authorPool.Find (value.c_str()) != NO_INDEX;
    }
    else if (revProp == svnDate)
    {
        return true;
    }
    else if (revProp == svnLog)
    {
        return comments.CanInsertThreadSafely (value);
    }
    else
    {
        // it's a user rev prop

        return (userRevPropNames.capacity() > userRevPropNames.size())
            && (userRevPropsPool.Find (revProp.c_str()) != NO_INDEX)
            && (userRevPropValues.CanInsertThreadSafely (value));
    }
}

// reset content

void CRevisionInfoContainer::Clear()
{
    authorPool.Clear();
    paths.Clear();
    comments.Clear();

    presenceFlags.clear();
    authors.clear();
    timeStamps.clear();

    rootPaths.clear();
    sumChanges.clear();

    changesOffsets.erase (changesOffsets.begin() +1, changesOffsets.end());
    copyFromOffsets.erase (copyFromOffsets.begin() +1, copyFromOffsets.end());
    mergedRevisionsOffsets.erase (mergedRevisionsOffsets.begin() +1, mergedRevisionsOffsets.end());
    userRevPropOffsets.erase (userRevPropOffsets.begin() +1, userRevPropOffsets.end());

    changes.clear();
    changedPaths.clear();
    changedPathTypes.clear();
    copyFromPaths.clear();
    copyFromRevisions.clear();
    textModifies.clear();
    propsModifies.clear();

    mergedFromPaths.clear();
    mergedToPaths.clear();
    mergedRangeStarts.clear();
    mergedRangeDeltas.clear();

    userRevPropsPool.Clear();
    userRevPropValues.Clear();
    userRevPropNames.clear();
}

// update / modify existing data
// indexes must be in ascending order
// indexes[] may be size() -> results in an Append()

void CRevisionInfoContainer::Update (const CRevisionInfoContainer& newData
                                     , const index_mapping_t& indexMap
                                     , char flags
                                     , bool keepOldDataForMissingNew)
{
    // first, create new indices where necessary

    AppendNewEntries (indexMap);

    // make new paths available

    index_mapping_t pathIDMapping = paths.Merge (newData.paths);

    // replace existing data

    if (flags & HAS_AUTHOR)
        UpdateAuthors (newData, indexMap, keepOldDataForMissingNew);
    if (flags & HAS_TIME_STAMP)
        UpdateTimeStamps (newData, indexMap, keepOldDataForMissingNew);
    if (flags & HAS_COMMENT)
        UpdateComments (newData, indexMap, keepOldDataForMissingNew);

    if (flags & HAS_CHANGEDPATHS)
        UpdateChanges (newData, indexMap, pathIDMapping, keepOldDataForMissingNew);
    if (flags & HAS_MERGEINFO)
        UpdateMergers (newData, indexMap, pathIDMapping, keepOldDataForMissingNew);
    if (flags & HAS_USERREVPROPS)
        UpdateUserRevProps (newData, indexMap, keepOldDataForMissingNew);

    UpdatePresenceFlags (newData, indexMap, flags, keepOldDataForMissingNew);
}

// rearrange the data to minimize disk and cache footprint

void CRevisionInfoContainer::Optimize()
{
    OptimizeAuthors();
    OptimizeChangeOrder();
}

// AutoOptimize() will call Optimize() when size() crossed 2^n boundaries.

void CRevisionInfoContainer::AutoOptimize()
{
    // bitwise XOR old and new size

    index_t currentSize = size();
    index_t diffBits = currentSize ^ storedSize;

    // if the position of highest bit set is equal (no 2^n boundary crossed),
    // diffBits will have that bit reset -> smaller than both input values

    if ( (currentSize <= diffBits) || (storedSize <= diffBits))
    {
        // shrink or growth cross a 2^n boundary

        Optimize();
    }
}

// reconstruct derived data

void CRevisionInfoContainer::CalculateSumChanges()
{
    // initialize all sums with "0"

    sumChanges.clear();
    sumChanges.resize (rootPaths.size());

    if (changes.empty())
        return;

    // fold all changes
    // (hand-tuned code because vector iterators do expensive checking
    //  ~6 ticks vs. ~20 ticks per change)

    const index_t *iter = &changesOffsets.at (1);
    unsigned char *target = &sumChanges.front();
    const unsigned char *source = &changes.front();

    unsigned char sum = 0;
    for (size_t i = 0, count = changes.size(); i < count; ++i)
    {
        while (*iter <= i)
        {
            *target = sum;
            ++iter;
            ++target;
            sum = 0;
        }

        sum |= * (source+i);
    }

    *target = sum;
}

// stream I/O

template<class T>
void ReadOrDefault ( IHierarchicalInStream& stream
                   , SUB_STREAM_ID id
                   , typename std::vector<T>& container
                   , size_t size
                   , T defaultValue)
{
    if (stream.HasSubStream (id))
    {
        *stream.GetSubStream<CDiffIntegerInStream>(id) >> container;
    }
    else
    {
        container.clear();
        container.insert (container.begin(), size, defaultValue);
    }
}

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
                                  , CRevisionInfoContainer& container)
{
    // read the pools

    IHierarchicalInStream* authorPoolStream
        = stream.GetSubStream (CRevisionInfoContainer::AUTHOR_POOL_STREAM_ID);
    *authorPoolStream >> container.authorPool;

    IHierarchicalInStream* commentsStream
        = stream.GetSubStream (CRevisionInfoContainer::COMMENTS_STREAM_ID);
    *commentsStream >> container.comments;

    IHierarchicalInStream* pathsStream
        = stream.GetSubStream (CRevisionInfoContainer::PATHS_STREAM_ID);
    *pathsStream >> container.paths;

    // read the revision info

    CDiffIntegerInStream* authorsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::AUTHORS_STREAM_ID);
    *authorsStream >> container.authors;

    CPackedTime64InStream* timeStampsStream
        = stream.GetSubStream<CPackedTime64InStream>
            (CRevisionInfoContainer::TIMESTAMPS_STREAM_ID);
    *timeStampsStream >> container.timeStamps;

    CDiffIntegerInStream* rootPathsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::ROOTPATHS_STREAM_ID);
    *rootPathsStream >> container.rootPaths;

    CDiffDWORDInStream* changesOffsetsStream
        = stream.GetSubStream<CDiffDWORDInStream>
            (CRevisionInfoContainer::CHANGES_OFFSETS_STREAM_ID);
    *changesOffsetsStream >> container.changesOffsets;

    CDiffDWORDInStream* copyFromOffsetsStream
        = stream.GetSubStream<CDiffDWORDInStream>
            (CRevisionInfoContainer::COPYFROM_OFFSETS_STREAM_ID);
    *copyFromOffsetsStream >> container.copyFromOffsets;

    CDiffDWORDInStream* mergedRevisionsOffsetsStream
        = stream.GetSubStream<CDiffDWORDInStream>
            (CRevisionInfoContainer::MERGEDREVISION_OFFSETS_STREAM_ID);
    *mergedRevisionsOffsetsStream >> container.mergedRevisionsOffsets;

    // read the changes info

    CPackedDWORDInStream* changesStream
        = stream.GetSubStream<CPackedDWORDInStream>
            (CRevisionInfoContainer::CHANGES_STREAM_ID);
    *changesStream >> container.changes;

    CDiffIntegerInStream* changedPathsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::CHANGED_PATHS_STREAM_ID);
    *changedPathsStream >> container.changedPaths;

    CDiffIntegerInStream* copyFromPathsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::COPYFROM_PATHS_STREAM_ID);
    *copyFromPathsStream >> container.copyFromPaths;

    CDiffIntegerInStream* copyFromRevisionsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::COPYFROM_REVISIONS_STREAM_ID);
    *copyFromRevisionsStream >> container.copyFromRevisions;

    // merged revisions

    CDiffIntegerInStream* mergedFromPathsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::MERGED_FROM_PATHS_STREAM_ID);
    *mergedFromPathsStream >> container.mergedFromPaths;

    CDiffIntegerInStream* mergedToPathsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::MERGED_TO_PATHS_STREAM_ID);
    *mergedToPathsStream >> container.mergedToPaths;

    CDiffIntegerInStream* mergedRangeStartsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::MERGED_RANGE_STARTS_STREAM_ID);
    *mergedRangeStartsStream >> container.mergedRangeStarts;

    CDiffIntegerInStream* mergedRangeDeltasStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::MERGED_RANGE_DELTAS_STREAM_ID);
    *mergedRangeDeltasStream >> container.mergedRangeDeltas;

    // user-defined revision properties

    CDiffIntegerInStream* userRevPropsOffsetsStream
        = stream.GetSubStream<CDiffIntegerInStream>
            (CRevisionInfoContainer::USER_REVPROPS_OFFSETS_STREAM_ID);
    *userRevPropsOffsetsStream >> container.userRevPropOffsets;

    IHierarchicalInStream* userRevPropsPoolStream
        = stream.GetSubStream (CRevisionInfoContainer::USER_REVPROPS_POOL_STREAM_ID);
    *userRevPropsPoolStream >> container.userRevPropsPool;

    CPackedDWORDInStream* userRevPropsNameStream
        = stream.GetSubStream<CPackedDWORDInStream>
            (CRevisionInfoContainer::USER_REVPROPS_NAME_STREAM_ID);
    *userRevPropsNameStream >> container.userRevPropNames;

    IHierarchicalInStream* userRevPropsValuesStream
        = stream.GetSubStream (CRevisionInfoContainer::USER_REVPROPS_VALUE_STREAM_ID);
    *userRevPropsValuesStream >> container.userRevPropValues;

    // data presence flags

    CPackedDWORDInStream* dataPresenceStream
        = stream.GetSubStream<CPackedDWORDInStream>
            (CRevisionInfoContainer::DATA_PRESENCE_STREAM_ID);
    *dataPresenceStream >> container.presenceFlags;

    // latest additions:
    // path type info (auto-add for legacy caches)
    // and detailed path modification info

    ReadOrDefault ( stream
                  , CRevisionInfoContainer::CHANGED_PATHS_TYPES_STREAM_ID
                  , container.changedPathTypes
                  , container.changedPaths.size()
                  , node_unknown);

    ReadOrDefault ( stream
                  , CRevisionInfoContainer::TEXTMODIFIES_STREAM_ID
                  , container.textModifies
                  , container.changedPaths.size()
                  , (unsigned char)0);

    ReadOrDefault ( stream
                  , CRevisionInfoContainer::PROPSMODIFIES_STREAM_ID
                  , container.propsModifies
                  , container.changedPaths.size()
                  , (unsigned char)0);

    // update size info

    container.storedSize = container.size();

    // reconstruct derived data

    container.CalculateSumChanges();

    // ready

    return stream;
}

IHierarchicalOutStream& operator<< (IHierarchicalOutStream& stream
                                    , const CRevisionInfoContainer& container)
{
    const_cast<CRevisionInfoContainer*> (&container)->AutoOptimize();

    // write the pools

    IHierarchicalOutStream* authorPoolStream
        = stream.OpenSubStream<CCompositeOutStream>
            (CRevisionInfoContainer::AUTHOR_POOL_STREAM_ID);
    *authorPoolStream << container.authorPool;

    IHierarchicalOutStream* commentsStream
        = stream.OpenSubStream<CCompositeOutStream>
            (CRevisionInfoContainer::COMMENTS_STREAM_ID);
    *commentsStream << container.comments;

    IHierarchicalOutStream* pathsStream
        = stream.OpenSubStream<CCompositeOutStream>
            (CRevisionInfoContainer::PATHS_STREAM_ID);
    *pathsStream << container.paths;

    // write the revision info

    CDiffIntegerOutStream* authorsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::AUTHORS_STREAM_ID);
    *authorsStream << container.authors;

    CPackedTime64OutStream* timeStampsStream
        = stream.OpenSubStream<CPackedTime64OutStream>
            (CRevisionInfoContainer::TIMESTAMPS_STREAM_ID);
    *timeStampsStream << container.timeStamps;

    CDiffIntegerOutStream* rootPathsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::ROOTPATHS_STREAM_ID);
    *rootPathsStream << container.rootPaths;

    CDiffDWORDOutStream* changesOffsetsStream
        = stream.OpenSubStream<CDiffDWORDOutStream>
            (CRevisionInfoContainer::CHANGES_OFFSETS_STREAM_ID);
    *changesOffsetsStream << container.changesOffsets;

    CDiffDWORDOutStream* copyFromOffsetsStream
        = stream.OpenSubStream<CDiffDWORDOutStream>
            (CRevisionInfoContainer::COPYFROM_OFFSETS_STREAM_ID);
    *copyFromOffsetsStream << container.copyFromOffsets;

    CDiffDWORDOutStream* mergedRevisionsOffsetsStream
        = stream.OpenSubStream<CDiffDWORDOutStream>
            (CRevisionInfoContainer::MERGEDREVISION_OFFSETS_STREAM_ID);
    *mergedRevisionsOffsetsStream << container.mergedRevisionsOffsets;

    // write the changes info

    CPackedDWORDOutStream* changesStream
        = stream.OpenSubStream<CPackedDWORDOutStream>
            (CRevisionInfoContainer::CHANGES_STREAM_ID);
    *changesStream << container.changes;

    CDiffIntegerOutStream* changedPathsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::CHANGED_PATHS_STREAM_ID);
    *changedPathsStream << container.changedPaths;

    CDiffIntegerOutStream* changedPathTypesStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::CHANGED_PATHS_TYPES_STREAM_ID);
    *changedPathTypesStream << container.changedPathTypes;

    CDiffIntegerOutStream* copyFromPathsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::COPYFROM_PATHS_STREAM_ID);
    *copyFromPathsStream << container.copyFromPaths;

    CDiffIntegerOutStream* copyFromRevisionsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::COPYFROM_REVISIONS_STREAM_ID);
    *copyFromRevisionsStream << container.copyFromRevisions;

    CDiffIntegerOutStream* textModifiesStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
        (CRevisionInfoContainer::TEXTMODIFIES_STREAM_ID);
    *textModifiesStream << container.textModifies;

    CDiffIntegerOutStream* propsModifiesStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
        (CRevisionInfoContainer::PROPSMODIFIES_STREAM_ID);
    *propsModifiesStream << container.propsModifies;

    // merged revisions

    CDiffIntegerOutStream* mergedFromPathsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::MERGED_FROM_PATHS_STREAM_ID);
    *mergedFromPathsStream << container.mergedFromPaths;

    CDiffIntegerOutStream* mergedToPathsStream
         = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::MERGED_TO_PATHS_STREAM_ID);
    *mergedToPathsStream << container.mergedToPaths;

    CDiffIntegerOutStream* mergedRangeStartsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::MERGED_RANGE_STARTS_STREAM_ID);
    *mergedRangeStartsStream << container.mergedRangeStarts;

    CDiffIntegerOutStream* mergedRangeDeltasStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::MERGED_RANGE_DELTAS_STREAM_ID);
    *mergedRangeDeltasStream << container.mergedRangeDeltas;

    // user-defined revision properties

    CDiffIntegerOutStream* userRevPropsOffsetsStream
        = stream.OpenSubStream<CDiffIntegerOutStream>
            (CRevisionInfoContainer::USER_REVPROPS_OFFSETS_STREAM_ID);
    *userRevPropsOffsetsStream << container.userRevPropOffsets;

    IHierarchicalOutStream* userRevPropsPoolStream
        = stream.OpenSubStream<CCompositeOutStream>
            (CRevisionInfoContainer::USER_REVPROPS_POOL_STREAM_ID);
    *userRevPropsPoolStream << container.userRevPropsPool;

    CPackedDWORDOutStream* userRevPropsNameStream
        = stream.OpenSubStream<CPackedDWORDOutStream>
            (CRevisionInfoContainer::USER_REVPROPS_NAME_STREAM_ID);
    *userRevPropsNameStream << container.userRevPropNames;

    IHierarchicalOutStream* userRevPropsValuesStream
        = stream.OpenSubStream<CCompositeOutStream>
            (CRevisionInfoContainer::USER_REVPROPS_VALUE_STREAM_ID);
    *userRevPropsValuesStream << container.userRevPropValues;

    // data presence flags

    CPackedDWORDOutStream* dataPresenceStream
        = stream.OpenSubStream<CPackedDWORDOutStream>
            (CRevisionInfoContainer::DATA_PRESENCE_STREAM_ID);
    *dataPresenceStream << container.presenceFlags;

    // update size info

    container.storedSize = container.size();

    // ready

    return stream;
}

// end namespace LogCache

}
