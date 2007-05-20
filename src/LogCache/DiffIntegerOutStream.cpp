#include "StdAfx.h"
#include "DiffIntegerOutStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffDWORDOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CDiffDWORDOutStream::CDiffDWORDOutStream ( CCacheFileOutBuffer* aBuffer
										 , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}

///////////////////////////////////////////////////////////////
//
// CDiffIntegerOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CDiffIntegerOutStream::CDiffIntegerOutStream ( CCacheFileOutBuffer* aBuffer
										     , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
