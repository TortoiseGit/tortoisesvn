#include "StdAfx.h"
#include "RootOutStream.h"

// construction / destruction: manage file buffer

CRootOutStream::CRootOutStream (const std::wstring& fileName)
	: CHierachicalOutStreamBase (&buffer, ROOT_STREAM_ID)
	, buffer (fileName)
{
}

CRootOutStream::~CRootOutStream()
{
	AutoClose();
}

// implement the rest of IHierarchicalOutStream

STREAM_TYPE_ID CRootOutStream::GetTypeID() const
{
	return ROOT_STREAM_TYPE_ID;
};
