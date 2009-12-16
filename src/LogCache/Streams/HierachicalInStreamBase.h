// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
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

    // prepare / finish data access

    virtual void AutoOpen() = 0;
    virtual void AutoClose() = 0;
    virtual void Prefetch() = 0;

	// access a sub-stream

    virtual bool HasSubStream (SUB_STREAM_ID subStreamID) const = 0;
	virtual IHierarchicalInStream* GetSubStream ( SUB_STREAM_ID subStreamID
                                                , bool autoOpen = true) = 0;

	// required for proper destruction of sub-class instances

	virtual ~IHierarchicalInStream() {};
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

	typedef unsigned char BYTE;
	const BYTE* first;
	const BYTE* last;

    // original stream data. NULL while not open

    const BYTE* packedLast;
    const BYTE* packedFirst;

	// for usage with CRootInStream

	CHierachicalInStreamBase();

	// read stream content

	void ReadSubStreams ( CCacheFileInBuffer* buffer
				        , STREAM_INDEX index);
	void DecodeThisStream();

public:

	// construction / destruction

	CHierachicalInStreamBase ( CCacheFileInBuffer* buffer
							 , STREAM_INDEX index);
	virtual ~CHierachicalInStreamBase(void);

	// implement IHierarchicalInStream

    virtual void AutoOpen();
    virtual void AutoClose();
    virtual void Prefetch();

    virtual bool HasSubStream (SUB_STREAM_ID subStreamID) const;
	virtual IHierarchicalInStream* GetSubStream ( SUB_STREAM_ID subStreamID
                                                , bool autoOpen = true);
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
