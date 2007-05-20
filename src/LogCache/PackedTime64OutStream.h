#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "BinaryOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedTime64OutStreamBase
//
//		Base class for write streams that store __time64_t-sized
//		data in a space efficient format. CBinaryOutStreamBase
//		is used as a base class for stream data handling.
//
//		Data is stored as follows:
//
//		* calculate the difference to the last value (init: 0)
//		  and store only that difference
//		* every entry starts with a header byte:
//		  bit 0 .. 3: abs (value diff.) bit 0 .. 3
//		  bit 4 .. 6: number of data bytes following the header -1
//		              (even 0 values use at least one data byte)
//		  bit 7:      sign (value diff.)
//		* data bytes following the header contain the abs. diff.
//	      value bits, lsb first, starting with bit 4.
//
//		[sgn s2 s1 s0 v3 v2 v1 v0][v11 .. v4][v19 .. v12] ...
//
///////////////////////////////////////////////////////////////

class CPackedTime64OutStreamBase : public CBinaryOutStreamBase
{
protected:

	__time64_t lastValue;

	// add data to the stream

	void Add (__time64_t value);

public:

	// construction / destruction: nothing special to do

	CPackedTime64OutStreamBase ( CCacheFileOutBuffer* aBuffer
						        , SUB_STREAM_ID anID);
	virtual ~CPackedTime64OutStreamBase() {};

	// plain data access

	void AddSizeValue (size_t value);
};

// plain data access

inline void CPackedTime64OutStreamBase::AddSizeValue (size_t value)
{
	Add (static_cast<DWORD>(value));
}

///////////////////////////////////////////////////////////////
//
// CPackedTime64OutStream
//
//		instantiable sub-class of CPackedTime64OutStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedTime64OutStream 
	: public COutStreamImplBase< CPackedTime64OutStream
							   , CPackedTime64OutStreamBase
		                       , PACKED_TIME64_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CPackedTime64OutStream
							  , CPackedTime64OutStreamBase
							  , PACKED_TIME64_STREAM_TYPE_ID> TBase;

	typedef __time64_t value_type;

	// construction / destruction: nothing special to do

	CPackedTime64OutStream ( CCacheFileOutBuffer* aBuffer
						   , SUB_STREAM_ID anID);
	virtual ~CPackedTime64OutStream() {};

	// public Add() methods

	using TBase::Add;
};
