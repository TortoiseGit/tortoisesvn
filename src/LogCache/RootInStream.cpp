#include "StdAfx.h"
#include "RootInStream.h"

// construction / destruction: manage file buffer

CRootInStream::CRootInStream (const std::wstring& fileName)
	: CHierachicalInStreamBase()
	, buffer (fileName)
{
	ReadSubStreams (&buffer, buffer.GetLastStream());
}

CRootInStream::~CRootInStream()
{
}
