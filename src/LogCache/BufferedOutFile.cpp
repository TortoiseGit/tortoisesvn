#include "StdAfx.h"
#include "BufferedOutFile.h"

// write buffer content to disk

void CBufferedOutFile::Flush()
{
	if (used > 0)
	{
		DWORD written = 0;
		WriteFile (file, buffer.get(), used, &written, NULL);
		used = 0;
	}
}

// construction / destruction: auto- open/close

CBufferedOutFile::CBufferedOutFile (const std::wstring& fileName)
	: file (INVALID_HANDLE_VALUE)
	, buffer (new unsigned char [BUFFER_SIZE])
	, used (0)
	, fileSize (0)
{
	file = CreateFile ( fileName.c_str()
					  , GENERIC_WRITE
					  , 0
					  , NULL
					  , CREATE_ALWAYS
					  , FILE_ATTRIBUTE_NORMAL
					  , NULL);
	if (file == INVALID_HANDLE_VALUE)
		throw std::exception ("can't create log cache file");
}

CBufferedOutFile::~CBufferedOutFile()
{
	if (IsOpen())
	{
		Flush();
		CloseHandle (file);
	}
}

// write data to file

void CBufferedOutFile::Add (const unsigned char* data, DWORD bytes)
{
	// test for buffer overflow

	if (used + bytes > BUFFER_SIZE)
	{
		Flush();

		// don't buffer large chunks of data

		if (bytes >= BUFFER_SIZE)
		{
			DWORD written = 0;
			WriteFile (file, data, bytes, &written, NULL);
			fileSize += written;

			return;
		}
	}

	// buffer small chunks of data

	memcpy (buffer.get() + used, data, bytes);

	used += bytes;
	fileSize += bytes;
}

///////////////////////////////////////////////////////////////
// file stream operation
///////////////////////////////////////////////////////////////

CBufferedOutFile& operator<< (CBufferedOutFile& dest, int value)
{
	enum {BUFFER_SIZE = 11};
	char buffer [BUFFER_SIZE];
	_itoa_s (value, buffer, 10);

	return operator<< (dest, buffer);
}
