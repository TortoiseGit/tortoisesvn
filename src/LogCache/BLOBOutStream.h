#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalOutStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CBLOBOutStreamBase
//
//		Base class for write streams that contain a single 
//		binary chunk of data.
//
//		Add() must be called at most once. Since it will close
//		the stream implicitly, there is no need to buffer the
//		data within the stream. It will be written directly
//		to the file buffer instead.
//
//		That means, all sub-streams will be written as well!
//
///////////////////////////////////////////////////////////////

class CBLOBOutStreamBase : public CHierachicalOutStreamBase
{
private:

	// data to write (may be NULL)

	const unsigned char* data;
	DWORD size;

	// write the (possible NULL) data we just got through Add()

	virtual void WriteThisStream (CCacheFileOutBuffer* buffer);

public:

	// construction / destruction: nothing special to do

	CBLOBOutStreamBase ( CCacheFileOutBuffer* aBuffer
					   , SUB_STREAM_ID anID);
	virtual ~CBLOBOutStreamBase() {};

	// write local stream data and close the stream

	void Add (const unsigned char* source, size_t byteCount);
};

///////////////////////////////////////////////////////////////
//
// CBLOBOutStream
//
//		instantiable sub-class of CBLOBOutStreamBase.
//
///////////////////////////////////////////////////////////////

template COutStreamImpl< CBLOBOutStreamBase
					   , BLOB_STREAM_TYPE_ID>;
typedef COutStreamImpl< CBLOBOutStreamBase
					  , BLOB_STREAM_TYPE_ID> CBLOBOutStream;
