#include "StdAfx.h"
#include "HierachicalInStreamBase.h"

void CHierachicalInStreamBase::ReadSubStreams ( CCacheFileInBuffer* buffer
											  , STREAM_INDEX index)
{
	// get the raw stream data

	buffer->GetStreamBuffer (index, first, last);

	// get the number of sub-streams

	size_t size = last - first;
	if (sizeof (DWORD) > size)
		throw std::exception ("stream too short");

	const DWORD *source = reinterpret_cast<const DWORD*>(first);
	DWORD subStreamCount = *source;
	size_t directorySize = (subStreamCount + 1) * sizeof (DWORD);
	if (directorySize > size)
		throw std::exception ("stream too short for sub-stream list");

	// read the sub-streams

	for (DWORD i = 0; i < subStreamCount; ++i)
	{
		STREAM_INDEX subStreamIndex = *++source;
		SUB_STREAM_ID subStreamID = *++source;
		STREAM_TYPE_ID subStreamType = *++source;

		const IInStreamFactory* factory 
			= CInStreamFactoryPool::GetInstance()->GetFactory (subStreamType);
		IHierarchicalInStream* subStream 
			= factory->CreateStream (buffer, subStreamIndex);

		subStreams.insert (std::make_pair (subStreamID, subStream));
	}

	// adjust buffer pointer

	first += directorySize;
}

// construction / destruction

CHierachicalInStreamBase::CHierachicalInStreamBase()
	: first (NULL)
	, last (NULL)
{
}

CHierachicalInStreamBase::CHierachicalInStreamBase ( CCacheFileInBuffer* buffer
												   , STREAM_INDEX index)
	: first (NULL)
	, last (NULL)
{
	ReadSubStreams (buffer, index);
}

CHierachicalInStreamBase::~CHierachicalInStreamBase()
{
	for ( TSubStreams::iterator iter = subStreams.begin()
		, end = subStreams.end()
		; iter != end
		; ++iter)
		delete iter->second;
}

// implement IHierarchicalOutStream

IHierarchicalInStream* 
CHierachicalInStreamBase::GetSubStream (SUB_STREAM_ID subStreamID)
{
	TSubStreams::const_iterator iter = subStreams.find (subStreamID);
	if (iter == subStreams.end())
		throw std::exception ("no such sub-stream");

	return iter->second;
}
