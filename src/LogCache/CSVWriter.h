#pragma once

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CStringDictionary;
class CPathDictionary;
class CCachedLogInfo;

/**
 * A simple exporter class that writes (most of) the
 * cached log info to a series of CSV files. 
 */

class CCSVWriter
{
private:

	/// utilities

	void Escape (std::string& value);

	/// write container content as CSV
	/// every method writes exactly one file

	void WriteStringList (std::ostream& os, const CStringDictionary& strings);
	void WritePathList (std::ostream& os, const CPathDictionary& dictionary);

	void WriteChanges (std::ostream& os, const CCachedLogInfo& cache);
	void WriteMerges (std::ostream& os, const CCachedLogInfo& cache);
	void WriteRevProps (std::ostream& os, const CCachedLogInfo& cache);

	void WriteRevisions (std::ostream& os, const CCachedLogInfo& cache);

public:

	/// construction / destruction (nothing to do)

	CCSVWriter(void);
	~CCSVWriter(void);

	/// write cache content as CSV files (overwrite existing ones)
	/// file names are extensions to the given fileName

	void Write (const CCachedLogInfo& cache, const std::wstring& fileName);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

