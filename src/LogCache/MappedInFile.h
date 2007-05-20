#pragma once

///////////////////////////////////////////////////////////////
//
// CMappedInFile
//
//		class that maps arbitrary files into memory.
//
//		The file must be small enough to fit into your address
//		space (i.e. less than about 1 GB under win32).
//
///////////////////////////////////////////////////////////////

class CMappedInFile
{
private:

	// the file

	HANDLE file;

	// the memory mapping

	HANDLE mapping;

	// file content memory address

	const unsigned char* buffer;

	// physical file size (== file size)

	size_t size;

	// construction utilities

	void MapToMemory (const std::wstring& fileName);

	// destruction / exception utility: close all handles

	void UnMap();

public:

	// construction / destruction: auto- open/close

	CMappedInFile (const std::wstring& fileName);
	virtual ~CMappedInFile();

	// access streams

	const unsigned char* GetBuffer() const;
	size_t GetSize() const;
};

///////////////////////////////////////////////////////////////
// access streams
///////////////////////////////////////////////////////////////

inline const unsigned char* CMappedInFile::GetBuffer() const
{
	return buffer;
}

inline size_t CMappedInFile::GetSize() const
{
	return size;
}
