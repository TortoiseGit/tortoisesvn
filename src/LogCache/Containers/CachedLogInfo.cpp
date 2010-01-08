// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

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
#include "CachedLogInfo.h"
//#include "LogCacheSettings.h"

#include "../Streams/RootInStream.h"
#include "../Streams/RootOutStream.h"

// begin namespace LogCache

namespace LogCache
{

// .lock file mechanism is only available under Windows

#ifdef WIN32

// "in use" (hidden flag) file flag handling

bool CCachedLogInfo::CCacheFileManager::IsMarked (const TFileName& name) const
{
    DWORD attributes = GetFileAttributes (name.c_str());
    return    (attributes != INVALID_FILE_ATTRIBUTES) 
           && ((attributes & FILE_ATTRIBUTE_HIDDEN) != 0);
}

void CCachedLogInfo::CCacheFileManager::SetMark (const TFileName& name)
{
    fileName = name;

    DWORD attributes = GetFileAttributes (fileName.c_str());
    attributes |= FILE_ATTRIBUTE_HIDDEN;
    SetFileAttributes (fileName.c_str(), attributes);
}

void CCachedLogInfo::CCacheFileManager::ResetMark()
{
    DWORD attributes = GetFileAttributes (fileName.c_str());
    attributes &= ~FILE_ATTRIBUTE_HIDDEN;
    SetFileAttributes (fileName.c_str(), attributes);
}

// allow for multiple failures 

bool CCachedLogInfo::CCacheFileManager::ShouldDrop 
    ( const TFileName& name
    , int maxFailures)
{
    // no mark -> no crash -> no drop here

    if (!IsMarked (name))
    {
        failureCount = NO_FAILURE;
        return false;
    }

    // can we open it?
    // If not, somebody else owns the lock -> don't touch it.

    HANDLE tempHandle = CreateFile ( name.c_str()
                                   , GENERIC_READ
                                   , 0
                                   , 0
                                   , OPEN_ALWAYS
                                   , FILE_ATTRIBUTE_NORMAL
                                   , NULL);
    if (tempHandle == INVALID_HANDLE_VALUE)
        return false;

    try
    {
        // any failure count so far?

        CFile file (tempHandle);
        if (file.GetLength() != 0)
        {
            CArchive stream (&file, CArchive::load);
            stream >> failureCount;
        }
        else
        {
            failureCount = 0;
        }

        // to many of them?

        CloseHandle (tempHandle);
        return failureCount >= maxFailures;
    }
    catch (CException* e)
    {
		e->Delete();
    }

    // could not access the file or similar problem 
    // -> remove it if it's no longer in use

    CloseHandle (tempHandle);

    return true;
}

void CCachedLogInfo::CCacheFileManager::UpdateMark (const TFileName& name)
{
    assert (OwnsFile());

    // mark as "in use"

    SetMark (name);

    // failed before?
    // If so, keep track of the number of failures.

    if (++failureCount > 0)
    {
        try
        {
            CFile file (fileHandle);
            CArchive stream (&file, CArchive::store);
            stream << failureCount;
        }
        catch (CException* e)
        {
			e->Delete();
        }
    }
}

// default construction / destruction

CCachedLogInfo::CCacheFileManager::CCacheFileManager()
    : fileHandle (INVALID_HANDLE_VALUE)
    , failureCount (NO_FAILURE)
{
}

CCachedLogInfo::CCacheFileManager::~CCacheFileManager()
{
    // cache file shall not be open anymore

    assert (!OwnsFile());
}

/// call this *before* opening the file
/// (will auto-drop crashed files etc.)

void CCachedLogInfo::CCacheFileManager::AutoAcquire 
    ( const TFileName& fileName
    , int maxFailures)
{
    assert (!OwnsFile());

    // remove existing files when they crashed before
    // (DeleteFile() will fail for open files)

    std::wstring lockFileName = fileName + L".lock";
    if (ShouldDrop (lockFileName, maxFailures))
    {
        if (DeleteFile (lockFileName.c_str()) == TRUE)
        {
            DeleteFile (fileName.c_str());
            failureCount = NO_FAILURE;
        }
    }

    // auto-create file and acquire lock

    fileHandle = CreateFile ( lockFileName.c_str()
                            , GENERIC_READ | GENERIC_WRITE
                            , 0
                            , 0
                            , OPEN_ALWAYS
                            , FILE_ATTRIBUTE_NORMAL
                            , NULL);
    if (OwnsFile())
    {
        // we are the first to open that file -> we own it.
        // Mark it as "in use" until being closed by AutoRelease().
        // Also, increment failure counter.

        UpdateMark (lockFileName);
    }
}

// call this *after* releasing a cache file
// (resets the "hidden" flag and closes the handle
// -- no-op if file is not owned)

void CCachedLogInfo::CCacheFileManager::AutoRelease()
{
    if (OwnsFile())
    {
        // reset the mark 
        // (AutoAcquire() will no longer try to delete the file)
        
        ResetMark();

        // now, we may close the file
        // -> DeleteFile() would succeed afterwards 
        //    but AutoAcquire() would no longer attempt it

        CloseHandle (fileHandle);
        fileHandle = INVALID_HANDLE_VALUE;

        // remove lock file
        // (may fail if somebody else already acquired the lock)

        DeleteFile (fileName.c_str());
    }
}

// If this returns false, you should not write the file.

bool CCachedLogInfo::CCacheFileManager::OwnsFile() const
{
    return fileHandle != INVALID_HANDLE_VALUE;
}

#else

// dummy implementation for non-Windows OS

bool CCachedLogInfo::CCacheFileManager::IsMarked (const TFileName&) const
{
    return false;
}

void CCachedLogInfo::CCacheFileManager::SetMark (const TFileName&)
{
}

void CCachedLogInfo::CCacheFileManager::ResetMark()
{
}

bool CCachedLogInfo::CCacheFileManager::ShouldDrop (const TFileName&, int)
{
    return false;
}

void CCachedLogInfo::CCacheFileManager::UpdateMark (const TFileName&)
{
}

// default construction / destruction

CCachedLogInfo::CCacheFileManager::CCacheFileManager()
{
}

CCachedLogInfo::CCacheFileManager::~CCacheFileManager()
{
}

void CCachedLogInfo::CCacheFileManager::AutoAcquire (const TFileName&, int)
{
}

void CCachedLogInfo::CCacheFileManager::AutoRelease()
{
}

bool CCachedLogInfo::CCacheFileManager::OwnsFile() const
{
    return true;
}

#endif

// construction / destruction (nothing to do)

CCachedLogInfo::CCachedLogInfo()
	: fileName()
	, revisions()
	, logInfo()
	, skippedRevisions (logInfo.GetPaths(), revisions, logInfo)
	, modified (false)
	, revisionAdded (false)
{
}

CCachedLogInfo::CCachedLogInfo (const TFileName& aFileName)
	: fileName (aFileName)
	, revisions()
	, logInfo()
	, skippedRevisions (logInfo.GetPaths(), revisions, logInfo)
	, modified (false)
	, revisionAdded (false)
{
}

CCachedLogInfo::~CCachedLogInfo (void)
{
    fileManager.AutoRelease();
}

// cache persistence

void CCachedLogInfo::Load (int maxFailures)
{
	assert (revisions.GetLastRevision() == 0);

	try
	{
		// handle crashes and lock file
	
		fileManager.AutoAcquire (fileName, maxFailures);
	
		// does log cache file exist?
	
	#ifdef WIN32
		if (GetFileAttributes (fileName.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
	#else
		FILE* file = fopen (fileName.c_str(), "r+");
		if (file != NULL)
		{
			fclose (file);
	#endif
			// read the data
	
			CRootInStream stream (fileName);
	
			IHierarchicalInStream* revisionsStream
				= stream.GetSubStream (REVISIONS_STREAM_ID);
			*revisionsStream >> revisions;
	
			IHierarchicalInStream* logInfoStream
				= stream.GetSubStream (LOG_INFO_STREAM_ID);
			*logInfoStream >> logInfo;
	
			IHierarchicalInStream* skipRevisionsStream
				= stream.GetSubStream (SKIP_REVISIONS_STREAM_ID);
			*skipRevisionsStream >> skippedRevisions;
		}
	}
	catch (...)
	{
		// if there was a problem, the cache file is probably corrupt
		// -> don't use its data
	}
}

bool CCachedLogInfo::IsEmpty() const
{
    return revisions.GetFirstCachedRevision() == NO_REVISION;
}

void CCachedLogInfo::Save (const TFileName& newFileName)
{
    // switch crash and lock management to new file name

    if (fileName != newFileName)
    {
        fileManager.AutoRelease();
        fileManager.AutoAcquire (newFileName, 0);
    }

	// write the data file, if we were the first to open it

    if (fileManager.OwnsFile())
    {
	    CRootOutStream stream (newFileName);

	    IHierarchicalOutStream* revisionsStream
			= stream.OpenSubStream<CCompositeOutStream> (REVISIONS_STREAM_ID);
	    *revisionsStream << revisions;

	    IHierarchicalOutStream* logInfoStream
		    = stream.OpenSubStream<CCompositeOutStream> (LOG_INFO_STREAM_ID);
	    *logInfoStream << logInfo;

	    IHierarchicalOutStream* skipRevisionsStream
		    = stream.OpenSubStream<CCompositeOutStream> (SKIP_REVISIONS_STREAM_ID);
	    *skipRevisionsStream << skippedRevisions;
    }

	// all fine -> connect to the new file name

	fileName = newFileName;

    // the data is no longer "modified"

    modified = false;
}

// find the highest revision not exceeding the given timestamp

revision_t CCachedLogInfo::FindRevisionByDate (__time64_t maxTimeStamp) const
{
    revision_t first = revisions.GetFirstCachedRevision();
    revision_t last = revisions.GetLastCachedRevision();

    revision_t result = (revision_t)NO_REVISION;
    for (revision_t revision = first; revision != last; ++revision)
    {
        index_t index = revisions[revision];
        if (index != NO_INDEX)
        {
            __time64_t revisionTime = logInfo.GetTimeStamp (index);
            if (revisionTime > maxTimeStamp)
                return result;

            result = revision;
        }
    }

    return result;
}

// data modification (mirrors CRevisionInfoContainer)

void CCachedLogInfo::Insert ( revision_t revision
							 , const std::string& author
							 , const std::string& comment
							 , __time64_t timeStamp
                             , char flags)
{
	// there will be a modification

	modified = true;

	// add entry to cache and update the revision index

	index_t index = logInfo.Insert (author, comment, timeStamp, flags);
	revisions.SetRevisionIndex (revision, index);

	// you may call AddChange() now

	revisionAdded = true;
}

void CCachedLogInfo::AddSkipRange ( const CDictionaryBasedPath& path
								  , revision_t startRevision
								  , revision_t count)
{
	modified = true;

	skippedRevisions.Add (path, startRevision, count);
}

void CCachedLogInfo::Clear()
{
	modified = revisions.GetLastRevision() != 0;
	revisionAdded = false;

	revisions.Clear();
	logInfo.Clear();
}

// return false if concurrent read accesses
// would potentially access invalid data.

bool CCachedLogInfo::CanInsertThreadSafely
    ( revision_t revision
    , const std::string& author
    , const std::string& comment
    , __time64_t timeStamp) const
{
    // check feasibility & add revision index

    return logInfo.CanInsertThreadSafely (author, comment, timeStamp)
        && revisions.CanSetRevisionIndexThreadSafely (revision);
}

// update / modify existing data

void CCachedLogInfo::Update ( const CCachedLogInfo& newData
							, char flags
                            , bool keepOldDataForMissingNew)
{
    // newData is often empty -> don't copy existing data around
    // (e.g. when we received known revision only)

    if (newData.IsEmpty() && keepOldDataForMissingNew)
        return;

	// build revision index map

	index_mapping_t indexMap;

	index_t newIndex = logInfo.size();
	for ( revision_t i = newData.revisions.GetFirstRevision()
		, last = newData.revisions.GetLastRevision()
		; i < last
		; ++i)
	{
		index_t sourceIndex = newData.revisions[i];
		if (sourceIndex != NO_INDEX)
		{
			index_t destIndex = revisions[i];
			if (destIndex == NO_INDEX)
			{
				destIndex = newIndex++;
				revisions.SetRevisionIndex (i, destIndex);
			}

			indexMap.insert (destIndex, sourceIndex);
		}
	}

	// update our log info

	logInfo.Update ( newData.logInfo
				   , indexMap
				   , flags
                   , keepOldDataForMissingNew);

	// our skip ranges should still be valid
	// but we check them anyway

	skippedRevisions.Compress();

    // this cache has been touched

    modified = true;
}

// end namespace LogCache

}

