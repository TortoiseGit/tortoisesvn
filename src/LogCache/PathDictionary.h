#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "StringDictonary.h"
#include "IndexPairDictionary.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
//
// CPathDictionary
//
//		A very efficient storage for file paths. Every path
//		is decomposed into its elements (separented by '/').
//		Those elements are uniquely stored in a string dictionary.
//		Paths are stored as (parentPathID, elementID) pairs
//		with index/ID 0 designating the root path.
//
//		Parent(root) == root.
//
///////////////////////////////////////////////////////////////

class CPathDictionary
{
private:

	// sub-stream IDs

	enum
	{
		ELEMENTS_STREAM_ID = 1,
		PATHS_STREAM_ID = 2
	};

	// string pool containing all used path elements

	CStringDictionary pathElements;

	// the paths as (parent, sub-path) pairs
	// index 0 is always the root

	CIndexPairDictionary paths;

	// index check utility

	void CheckParentIndex (index_t index) const;

	// construction utility

	void Initialize();

public:

	// construction (create root path) / destruction

	CPathDictionary();
	virtual ~CPathDictionary(void);

	// dictionary operations

	index_t size() const
	{
		return (index_t)paths.size();
	}

	index_t GetParent (index_t index) const;
	const char* GetPathElement (index_t index) const;

	index_t Find (index_t parent, const char* pathElement) const;
	index_t Insert (index_t parent, const char* pathElement);
	index_t AutoInsert (index_t parent, const char* pathElement);

	// reset content

	void Clear();

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CPathDictionary& dictionary);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CPathDictionary& dictionary);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CPathDictionary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CPathDictionary& dictionary);

///////////////////////////////////////////////////////////////
//
// CDictionaryBasedPath
//
///////////////////////////////////////////////////////////////

class CDictionaryBasedPath
{
private:

	// our dictionary and position within it

	const CPathDictionary* dictionary;
	index_t index;

protected:

	// index manipulation

	void SetIndex (index_t newIndex) 
	{
		index = newIndex;
	}

	// construction utility: lookup and optionally auto-insert

	void ParsePath ( const std::string& path
				   , CPathDictionary* writableDictionary = NULL
				   , std::vector<std::string>* relPath = NULL);

public:

	// construction / destruction

	CDictionaryBasedPath (const CPathDictionary* aDictionary, index_t anIndex)
		: dictionary (aDictionary)
		, index (anIndex)
	{
	}

	CDictionaryBasedPath ( const CPathDictionary* aDictionary
						 , const std::string& path);

	CDictionaryBasedPath ( CPathDictionary* aDictionary
						 , const std::string& path
						 , bool nextParent);

	~CDictionaryBasedPath() 
	{
	}

	// data access

	index_t GetIndex() const
	{
		return index;
	}

	const CPathDictionary* GetDictionary() const
	{
		return dictionary;
	}

	// path operations

	bool IsRoot() const
	{
		return index == 0;
	}

	bool IsValid() const
	{
		return index != NO_INDEX;
	}

	CDictionaryBasedPath GetParent() const
	{
		return CDictionaryBasedPath ( dictionary
									, dictionary->GetParent (index));
	}

	CDictionaryBasedPath GetCommonRoot (const CDictionaryBasedPath& rhs) const
	{
		return GetCommonRoot (rhs.index);
	}

	CDictionaryBasedPath GetCommonRoot (index_t rhsIndex) const;

	bool IsSameOrParentOf (const CDictionaryBasedPath& rhs) const;

	// convert to string

	std::string GetPath() const;
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

