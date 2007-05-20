#pragma once

///////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////

#include "StreamFactory.h"
#include "StreamTypeIDs.h"
#include "CacheFileInBuffer.h"

///////////////////////////////////////////////////////////////
//
// IHierarchicalInStream
//
//		the generic read stream interface. 
//		Streams form a tree.
//
///////////////////////////////////////////////////////////////

class IHierarchicalInStream
{
public:

	// add a sub-stream

	virtual IHierarchicalInStream* GetSubStream (SUB_STREAM_ID subStreamID) = 0;
};

///////////////////////////////////////////////////////////////
//
// CHierachicalInStreamBase
//
//		implements IHierarchicalInStream except for GetTypeID().
//		It mainly manages the sub-streams. 
//
//		For details of the stream format see CHierachicalOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CHierachicalInStreamBase : public IHierarchicalInStream
{
private:

	// the sub-streams (in no particular order)

	typedef std::map<SUB_STREAM_ID, IHierarchicalInStream*> TSubStreams;
	TSubStreams subStreams;

protected:

	// stream content
	// (sub-stream info will be excluded after ReadSubStreams()) 

	const unsigned char* first;
	const unsigned char* last;

	// for usage with CRootInStream

	CHierachicalInStreamBase();

	void ReadSubStreams ( CCacheFileInBuffer* buffer
				        , STREAM_INDEX index);

public:

	// construction / destruction

	CHierachicalInStreamBase ( CCacheFileInBuffer* buffer
							 , STREAM_INDEX index);
	virtual ~CHierachicalInStreamBase(void);

	// implement IHierarchicalOutStream

	virtual IHierarchicalInStream* GetSubStream (SUB_STREAM_ID subStreamID);
};

///////////////////////////////////////////////////////////////
//
// CInStreamImplBase<>
//
//		implements a read stream class based upon the non-
//		creatable base class B. T is the actual stream class
//		to create and type is the desired stream type id.
//
///////////////////////////////////////////////////////////////

template<class T, class B, STREAM_TYPE_ID type> 
class CInStreamImplBase : public B
{
public:

	// create our stream factory

	typedef CStreamFactory< T
						  , IHierarchicalInStream
						  , CCacheFileInBuffer
						  , type> TFactory;
	static typename TFactory::CCreator factoryCreator;

public:

	// construction / destruction: nothing to do here

	CInStreamImplBase ( CCacheFileInBuffer* buffer
					  , STREAM_INDEX index)
		: B (buffer, index)
	{
		// trick the compiler: 
		// use a dummy reference to factoryCreator
		// to force its creation

		&factoryCreator;
	}

	virtual ~CInStreamImplBase() {};
};

// stream factory creator

template<class T, class B, STREAM_TYPE_ID type> 
typename CInStreamImplBase<T, B, type>::TFactory::CCreator 
	CInStreamImplBase<T, B, type>::factoryCreator;

///////////////////////////////////////////////////////////////
//
// CInStreamImpl<>
//
//		enhances CInStreamImplBase<> for the case that there 
//		is no further sub-class.
//
///////////////////////////////////////////////////////////////

template<class B, STREAM_TYPE_ID type> 
class CInStreamImpl : public CInStreamImplBase< CInStreamImpl<B, type>
											  , B
											  , type>
{
public:

	typedef CInStreamImplBase<CInStreamImpl<B, type>, B, type> TBase;

	// construction / destruction: nothing to do here

	CInStreamImpl ( CCacheFileInBuffer* buffer
				  , STREAM_INDEX index)
		: TBase (buffer, index)
	{
	}

	virtual ~CInStreamImpl() {};
};
