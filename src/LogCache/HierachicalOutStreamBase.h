#pragma once

///////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////

#include "StreamFactory.h"
#include "StreamTypeIDs.h"
#include "CacheFileOutBuffer.h"

///////////////////////////////////////////////////////////////
//
// IHierarchicalOutStream
//
//		the generic write stream interface. 
//		Streams form a tree.
//
///////////////////////////////////////////////////////////////

class IHierarchicalOutStream
{
public:

	// id, unique within the parent stream

	virtual SUB_STREAM_ID GetID() const = 0;

	// stream type (identifies the factory to use)

	virtual STREAM_TYPE_ID GetTypeID() const = 0;

	// add a sub-stream

	virtual IHierarchicalOutStream* OpenSubStream ( SUB_STREAM_ID subStreamID
												  , STREAM_TYPE_ID type) = 0;

	// close the stream (returns a globally unique index)

	virtual STREAM_INDEX AutoClose() = 0;
};

///////////////////////////////////////////////////////////////
//
// CHierachicalOutStreamBase
//
//		implements IHierarchicalOutStream except for GetTypeID().
//		It mainly manages the sub-streams. 
//
//		Stream format:
//
//		* number N of sub-streams (4 bytes)
//		* sub-stream list: N triples (N * 3 * 4 bytes)
//		  (index, id, type)
//		* M bytes of local stream content
//
///////////////////////////////////////////////////////////////

class CHierachicalOutStreamBase : public IHierarchicalOutStream
{
private:

	// our logical ID within the parent stream

	SUB_STREAM_ID id;

	CCacheFileOutBuffer* buffer;
	STREAM_INDEX index;	// (-1) while stream is open

	std::vector<IHierarchicalOutStream*> subStreams;

	// overwrite this in your class

	virtual void WriteThisStream (CCacheFileOutBuffer* buffer) = 0;

	// close (write) stream

	void CloseSubStreams();
	void WriteSubStreamList();
	void Close();

public:

	// construction / destruction (Close() must have been called before)

	CHierachicalOutStreamBase ( CCacheFileOutBuffer* aBuffer
							  , SUB_STREAM_ID anID);
	virtual ~CHierachicalOutStreamBase(void);

	// implement most of IHierarchicalOutStream

	virtual SUB_STREAM_ID GetID() const;
	virtual IHierarchicalOutStream* OpenSubStream ( SUB_STREAM_ID subStreamID
												  , STREAM_TYPE_ID type);
	virtual STREAM_INDEX AutoClose();
};

///////////////////////////////////////////////////////////////
//
// COutStreamImplBase<>
//
//		implements a write stream class based upon the non-
//		creatable base class B. T is the actual stream class
//		to create and type is the desired stream type id.
//
///////////////////////////////////////////////////////////////

template<class T, class B, STREAM_TYPE_ID type> 
class COutStreamImplBase : public B
{
private:

	// create our stream factory

	typedef CStreamFactory< T
						  , IHierarchicalOutStream
						  , CCacheFileOutBuffer
						  , type> TFactory;
	static typename TFactory::CCreator factoryCreator;

public:

	// construction / destruction: nothing to do here

	COutStreamImplBase ( CCacheFileOutBuffer* aBuffer
					   , SUB_STREAM_ID anID)
		: B (aBuffer, anID)
	{
		// trick the compiler: 
		// use a dummy reference to factoryCreator
		// to force its creation

		&factoryCreator;
	}

	virtual ~COutStreamImplBase() {};

	// implement the rest of IHierarchicalOutStream

	virtual STREAM_TYPE_ID GetTypeID() const
	{
		return TFactory::GetInstance()->GetTypeID();
	}
};

// stream factory creator

template<class T, class B, STREAM_TYPE_ID type> 
typename COutStreamImplBase<T, B, type>::TFactory::CCreator 
	COutStreamImplBase<T, B, type>::factoryCreator;

///////////////////////////////////////////////////////////////
//
// CInStreamImpl<>
//
//		enhances CInStreamImplBase<> for the case that there 
//		is no further sub-class.
//
///////////////////////////////////////////////////////////////

template<class B, STREAM_TYPE_ID type> 
class COutStreamImpl : public COutStreamImplBase< COutStreamImpl<B, type>
												, B
												, type>
{
public:

	typedef COutStreamImplBase<COutStreamImpl<B, type>, B, type> TBase;

	// construction / destruction: nothing to do here

	COutStreamImpl ( CCacheFileOutBuffer* buffer
				   , STREAM_INDEX index)
		: TBase (buffer, index)
	{
	}

	virtual ~COutStreamImpl() {};
};
