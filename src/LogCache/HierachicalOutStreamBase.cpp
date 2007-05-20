#include "StdAfx.h"
#include "HierachicalOutStreamBase.h"

// close (write) stream

void CHierachicalOutStreamBase::CloseSubStreams()
{
	std::for_each (subStreams.begin(), subStreams.end(), 
		std::mem_fun (&IHierarchicalOutStream::AutoClose));
}

void CHierachicalOutStreamBase::WriteSubStreamList()
{
	// first, the count

	buffer->Add ((DWORD)subStreams.size());

	// for each sub-stream: (index, id, type)

	for (size_t i = 0, count = subStreams.size(); i < count; ++i)
	{
		IHierarchicalOutStream* subStream = subStreams[i];
		buffer->Add (subStream->AutoClose());
		buffer->Add (subStream->GetID());
		buffer->Add (subStream->GetTypeID());
	}
}

void CHierachicalOutStreamBase::Close()
{
	CloseSubStreams();

	index = buffer->OpenStream();
	WriteSubStreamList();
	WriteThisStream (buffer);
	buffer->CloseStream();
}

// construction / destruction (Close() must have been called before)

CHierachicalOutStreamBase::CHierachicalOutStreamBase ( CCacheFileOutBuffer* aBuffer
													 , SUB_STREAM_ID anID)
	: id (anID)
	, buffer (aBuffer)
	, index (-1)
{
}

CHierachicalOutStreamBase::~CHierachicalOutStreamBase()
{
	assert (index != -1);

	for (size_t i = 0, count = subStreams.size(); i < count; ++i)
		delete subStreams[i];
}

// implement IHierarchicalOutStream

SUB_STREAM_ID CHierachicalOutStreamBase::GetID() const
{
	return id;
}

IHierarchicalOutStream* 
CHierachicalOutStreamBase::OpenSubStream ( SUB_STREAM_ID subStreamID
										 , STREAM_TYPE_ID type)
{
	const IOutStreamFactory* factory 
		= COutStreamFactoryPool::GetInstance()->GetFactory(type);
	IHierarchicalOutStream* subStream 
		= factory->CreateStream (buffer, subStreamID);

	subStreams.push_back (subStream);

	return subStream;
}

STREAM_INDEX CHierachicalOutStreamBase::AutoClose()
{
	if (index == -1)
		Close();

	return index;
}
