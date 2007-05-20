#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalInStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CRootInStream
//
//		The mother of all streams: represents the root of the
//		stream hierarchy but has no content of its own. It also
//		opens and closes the read buffer.
//
//		In contrast to all other stream types, there is no 
//		factory to create an instance of this class.
//
///////////////////////////////////////////////////////////////

class CRootInStream : public CHierachicalInStreamBase
{
private:

	CCacheFileInBuffer buffer;

public:

	// construction / destruction: manage file buffer

	CRootInStream (const std::wstring& fileName);
	virtual ~CRootInStream();
};
