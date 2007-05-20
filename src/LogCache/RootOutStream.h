#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalOutStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CRootOutStream
//
//		The mother of all streams: represents the root of the
//		stream hierarchy but has no content of its own. It also
//		opens and closes the write buffer.
//
//		In contrast to all other stream types, there is no 
//		factory to create an instance of this class.
//
///////////////////////////////////////////////////////////////

class CRootOutStream : public CHierachicalOutStreamBase
{
private:

	CCacheFileOutBuffer buffer;

	enum {ROOT_STREAM_ID = 0};

	// the root does not have local stream data

	virtual void WriteThisStream (CCacheFileOutBuffer* buffer) {};

public:

	// construction / destruction: manage file buffer

	CRootOutStream (const std::wstring& fileName);
	virtual ~CRootOutStream();

	// implement the rest of IHierarchicalOutStream

	virtual STREAM_TYPE_ID GetTypeID() const;
};
