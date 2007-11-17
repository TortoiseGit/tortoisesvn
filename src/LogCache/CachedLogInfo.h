// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

	/// where we load / save our cached data

	std::wstring fileName;

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
	CCachedLogInfo (const std::wstring& aFileName);
	~CCachedLogInfo (void);

	/// cache persistence

	void Load();
	bool IsModified() const;
	void Save();
	void Save (const std::wstring& newFileName);

	/// data access

	const std::wstring& GetFileName() const;
	const CRevisionIndex& GetRevisions() const;
	const CRevisionInfoContainer& GetLogInfo() const;
	const CSkipRevisionInfo& GetSkippedRevisions() const;

	/// data modification 
	/// (mirrors CRevisionInfoContainer and CSkipRevisionInfo)

	void Insert ( revision_t revision
				, const std::string& author
				, const std::string& comment
				, __time64_t timeStamp
                , char flags = CRevisionInfoContainer::HAS_STANDARD_INFO);

	void AddChange ( TChangeAction action
				   , const std::string& path
				   , const std::string& fromPath
				   , revision_t fromRevision);

	void AddMergedRevision ( const std::string& fromPath
				           , const std::string& toPath
				           , revision_t revisionStart
				           , revision_t revisionDelta);

	void AddUserRevProp ( const std::string& revProp
				        , const std::string& value);

    void AddSkipRange ( const CDictionaryBasedPath& path
					  , revision_t startRevision
					  , revision_t count);

	void Clear();

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

inline const std::wstring& CCachedLogInfo::GetFileName() const
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
								      , const std::string& path
									  , const std::string& fromPath
									  , revision_t fromRevision)
{
	assert (revisionAdded);

	logInfo.AddChange (action, path, fromPath, fromRevision);
}

inline void CCachedLogInfo::AddMergedRevision ( const std::string& fromPath
				                              , const std::string& toPath
				                              , revision_t revisionStart
				                              , revision_t revisionDelta)
{
	assert (revisionAdded);

	logInfo.AddMergedRevision (fromPath, toPath, revisionStart, revisionDelta);
}

inline void CCachedLogInfo::AddUserRevProp ( const std::string& revProp
				                           , const std::string& value)
{
	assert (revisionAdded);

	logInfo.AddUserRevProp (revProp, value);
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

