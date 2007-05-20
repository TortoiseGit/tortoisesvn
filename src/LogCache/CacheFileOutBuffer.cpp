#include "StdAfx.h"
#include "CacheFileOutBuffer.h"

// construction / destruction: auto- open/close

CCacheFileOutBuffer::CCacheFileOutBuffer (const std::wstring& fileName)
	: CBufferedOutFile (fileName)
	, streamIsOpen (false)
{
	// write the version ids

	Add (OUR_LOG_CACHE_FILE_VERSION);
	Add (MIN_LOG_CACHE_FILE_VERSION);
}

CCacheFileOutBuffer::~CCacheFileOutBuffer()
{
	if (IsOpen())
	{
		streamOffsets.push_back (GetFileSize());

		size_t lastOffset = streamOffsets[0];
		for (size_t i = 1, count = streamOffsets.size(); i < count; ++i)
		{
			size_t offset = streamOffsets[i];
			size_t size = offset - lastOffset;

			if (size >= (DWORD)(-1))
				throw std::exception ("stream too large");

			Add ((DWORD)size);
			lastOffset = offset;
		}

		Add ((DWORD)(streamOffsets.size()-1));
	}
}

// write data to file

STREAM_INDEX CCacheFileOutBuffer::OpenStream()
{
	// don't overflow (2^32 streams is quite unlikely, though)

	assert ((STREAM_INDEX)streamOffsets.size() != (-1));

	assert (streamIsOpen == false);
	streamIsOpen = true;

	streamOffsets.push_back (GetFileSize());
	return (STREAM_INDEX)streamOffsets.size()-1;
}

void CCacheFileOutBuffer::CloseStream()
{
	assert (streamIsOpen == true);
	streamIsOpen = false;
}


