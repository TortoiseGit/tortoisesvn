#include "StdAfx.h"
#include "BLOBOutStream.h"

// write the (possible NULL) data we just got through Add()

void CBLOBOutStreamBase::WriteThisStream (CCacheFileOutBuffer* buffer)
{
	if ((data != NULL) && (size > 0))
		buffer->Add (data, size);
}

// construction: nothing special to do

CBLOBOutStreamBase::CBLOBOutStreamBase ( CCacheFileOutBuffer* aBuffer
									   , SUB_STREAM_ID anID)
	: CHierachicalOutStreamBase (aBuffer, anID)
	, data (NULL)
	, size (0)
{
}

// write local stream data and close the stream

void CBLOBOutStreamBase::Add (const unsigned char* source, size_t byteCount)
{
	assert (data == NULL);

	// this may fail under x64

	if (byteCount > (DWORD)(-1))
		throw std::exception ("BLOB to large for stream");

	// remember the buffer & size just for a few moments

	data = source;
	size = (DWORD)byteCount;

	// write them (and all sub-streams) to file

	AutoClose();
}
