// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2012 - TortoiseSVN

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
#include "CacheFileOutBuffer.h"

///////////////////////////////////////////////////////////////
//
// IHierarchicalOutStream
//
//      the generic write stream interface.
//      Streams form a tree.
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

    // for simplified access
    // The last parameter is only present for technical reasons
    // (different type overloads must have different signatures)
    // and neither needs to be specified nor will it not be used.

    template<class S>
    S* OpenSubStream (SUB_STREAM_ID subStreamID, S* = NULL);

    // close the stream (returns a globally unique index)

    virtual STREAM_INDEX AutoClose() = 0;

    // required for proper destruction of sub-class instances

    virtual ~IHierarchicalOutStream() {};
};

///////////////////////////////////////////////////////////////
// for simplified access
///////////////////////////////////////////////////////////////

template<class S>
S* IHierarchicalOutStream::OpenSubStream (SUB_STREAM_ID subStreamID, S*)
{
    return dynamic_cast<S*>(OpenSubStream (subStreamID, S::TYPE_ID));
}

///////////////////////////////////////////////////////////////
//
// CHierachicalOutStreamBase
//
//      implements IHierarchicalOutStream except for GetTypeID().
//      It mainly manages the sub-streams.
//
//      Stream format:
//
//      * number N of sub-streams (4 bytes)
//      * sub-stream list: N triples (N * 3 * 4 bytes)
//        (index, id, type)
//      * M bytes of local stream content
//
//      The local stream content is as follows:
//
//      * one or more Huffman compressed blocks
//        (block size depending on the chunk size
//         chosen by the respective sub-class)
//      * cumulative size of the decoded local stream
//        content (4 bytes)
//
///////////////////////////////////////////////////////////////

class CHierachicalOutStreamBase : public IHierarchicalOutStream
{
private:

    // our logical ID within the parent stream

    SUB_STREAM_ID id;

    CCacheFileOutBuffer* buffer;
    STREAM_INDEX index; // (-1) while stream is open
    bool isOpen;

    // cumulated stream size *before* huffman-encoding it

    size_t decodedSize;

    // sub-streams

    std::vector<IHierarchicalOutStream*> subStreams;

    // overwrite this in your class

    virtual const unsigned char* GetStreamData() = 0;
    virtual size_t GetStreamSize() = 0;
    virtual void ReleaseStreamData() {};
    virtual void FlushData() {};

    // close (write) stream

    void AutoOpen();
    void CloseLatestSubStream();
    void WriteSubStreamList();
    void Close();

protected:

    void WriteThisStream();

public:

    // construction / destruction (Close() must have been called before)

    CHierachicalOutStreamBase ( CCacheFileOutBuffer* aBuffer
                              , SUB_STREAM_ID anID);
    virtual ~CHierachicalOutStreamBase(void);

    // implement most of IHierarchicalOutStream

    virtual SUB_STREAM_ID GetID() const override;
    virtual IHierarchicalOutStream* OpenSubStream ( SUB_STREAM_ID subStreamID
                                                  , STREAM_TYPE_ID type);
    template<class S>
    S* OpenSubStream (SUB_STREAM_ID subStreamID, S* = NULL);

    virtual STREAM_INDEX AutoClose() override;
};

template<class S>
inline S* CHierachicalOutStreamBase::OpenSubStream (SUB_STREAM_ID subStreamID, S*)
{
    return IHierarchicalOutStream::OpenSubStream<S> (subStreamID);
}

///////////////////////////////////////////////////////////////
//
// COutStreamImplBase<>
//
//      implements a write stream class based upon the non-
//      creatable base class B. T is the actual stream class
//      to create and type is the desired stream type id.
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

    // for reference in other templates

    enum {TYPE_ID = type};

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

    virtual STREAM_TYPE_ID GetTypeID() const override
    {
        return TYPE_ID;
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
//      enhances CInStreamImplBase<> for the case that there
//      is no further sub-class.
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
