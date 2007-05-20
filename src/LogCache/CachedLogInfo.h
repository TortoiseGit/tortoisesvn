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

///////////////////////////////////////////////////////////////
//
// CCachedLogInfo
//
//		contains the whole cached log information: It just
//		combines the exisiting logInfo structure with the
//		revision index (lookup is revision=>revIndex=>revData).
//
//		The interface is that of the logInfo component, except
//		that it uses proper revision numbers.
//
//		As the cache root object, it is associated with a
//		fileName. You have to load and save the data explicitly.
//
//		It also maintains a "modified" flag to check whether
//		new data has been added / removed at all. You don't 
//		need to call Save() if there was no change.
//
///////////////////////////////////////////////////////////////

class CCachedLogInfo
{
private:

	// where we load / save our cached data

	std::wstring fileName;

	// revision index and the log info itself

	CRevisionIndex revisions;
	CRevisionInfoContainer logInfo;
	CSkipRevisionInfo skippedRevisions;

	// revision has been added or Clear() has been called

	bool modified;

	// revision has been added (otherwise, AddChange is forbidden)

	bool revisionAdded;

	// stream IDs

	enum
	{
		REVISIONS_STREAM_ID = 1,
		LOG_INFO_STREAM_ID = 2,
		SKIP_REVISIONS_STREAM_ID = 3
	};

public:

	// for convenience

	typedef CRevisionInfoContainer::TChangeAction TChangeAction;

	// construction / destruction (nothing to do)

	CCachedLogInfo (const std::wstring& aFileName);
	~CCachedLogInfo (void);

	// cache persistency

	void Load();
	bool IsModified();
	void Save();
	void Save (const std::wstring& newFileName);

	// data access

	const std::wstring& GetFileName() const;
	const CRevisionIndex& GetRevisions() const;
	const CRevisionInfoContainer& GetLogInfo() const;
	const CSkipRevisionInfo& GetSkippedRevisions() const;

	// data modification 
	// (mirrors CRevisionInfoContainer and CSkipRevisionInfo)

	void Insert ( revision_t revision
				, const std::string& author
				, const std::string& comment
				, __time64_t timeStamp);

	void AddChange ( TChangeAction action
				   , const std::string& path
				   , const std::string& fromPath
				   , revision_t fromRevision);

	void AddSkipRange ( const CDictionaryBasedPath& path
					  , revision_t startRevision
					  , revision_t count);

	void Clear();
};

///////////////////////////////////////////////////////////////
// cache persistency
///////////////////////////////////////////////////////////////

inline bool CCachedLogInfo::IsModified()
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

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

