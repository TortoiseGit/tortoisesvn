#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "BufferedOutFile.h"

///////////////////////////////////////////////////////////////
// index type used to address a certain stream within the file.
///////////////////////////////////////////////////////////////

typedef int STREAM_INDEX;

///////////////////////////////////////////////////////////////
// our current version identifier and the minimum version
// required to read this file
// (just in case we want to change / extend the format later on)
///////////////////////////////////////////////////////////////

enum
{
	OUR_LOG_CACHE_FILE_VERSION = 0x20060701,
	MIN_LOG_CACHE_FILE_VERSION = 0x20060701
};

///////////////////////////////////////////////////////////////
//
// CCacheFileOutBuffer
//
//		class that allows arbitrary numbers of streams to be
//		written sequentially to a single file. Usually, stream
//		write there data during their "Close()" operation.
//
//		The purpose	of this class is to provide a directory for 
//		all streams so they may be found again when reading the 
//		file.
//
//		Streams can be written only one at a time. OpenStream()
//		will return a unique stream index to be used to address
//		the stream within the file. Add will then put arbitrary
//		chunks of data to the stream. Smaller data chunks will 
//		be buffered. 
//
//		The number of streams and the size of each stream is
//		limited to 2^32-1 (bytes). As the data will be mapped
//		to memory, this is only an issue on 64 bit machines.
//
//		File format:
//
//		* our version ID (4 bytes. current value: 0x20060701)
//		* min. version ID (4 bytes. current value: 0x20060701)
//		* stream(s) (N BLOBs)
//		* stream sizes (N 4 byte integers)
//		* N (4 bytes unsigned integer)
//
///////////////////////////////////////////////////////////////

class CCacheFileOutBuffer : public CBufferedOutFile
{
private:

	// offsets of all streams

	std::vector<size_t> streamOffsets;

	// check that we don't open two streams at once

	bool streamIsOpen;

public:

	// construction / destruction: auto- open/close

	CCacheFileOutBuffer (const std::wstring& fileName);
	virtual ~CCacheFileOutBuffer();

	// write data to file

	STREAM_INDEX OpenStream();
	void CloseStream();
};

