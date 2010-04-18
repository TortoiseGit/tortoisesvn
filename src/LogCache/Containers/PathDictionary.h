// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
// necessary includes
///////////////////////////////////////////////////////////////

#include "StringDictonary.h"
#include "IndexPairDictionary.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{


/**
 * A very efficient storage for file paths. Every path is decomposed into its
 * elements (separated by '/').
 * Those elements are uniquely stored in a string dictionary.
 * Paths are stored as (parentPathID, elementID) pairs with index/ID 0
 * designating the root path.
 *
 * Parent(root) == root.
 */
class CPathDictionary
{
private:

    /// sub-stream IDs

    enum
    {
        ELEMENTS_STREAM_ID = 1,
        PATHS_STREAM_ID = 2
    };

    /// string pool containing all used path elements

    CStringDictionary pathElements;

    /// the paths as (parent, sub-path) pairs
    /// index 0 is always the root

    CIndexPairDictionary paths;

    /// index check utility

    void CheckParentIndex (index_t index) const;

    /// construction utility

    void Initialize();

public:

    /// construction (create root path) / destruction

    CPathDictionary();
    virtual ~CPathDictionary(void);

    /// member access

    const CStringDictionary& GetPathElements() const
    {
        return pathElements;
    }

    /// dictionary operations

    index_t size() const
    {
        return (index_t)paths.size();
    }

    index_t GetParent (index_t index) const;
    const char* GetPathElement (index_t index) const;
    index_t GetPathElementSize (index_t index) const;
    index_t GetPathElementID (index_t index) const;

    index_t Find (index_t parent, const char* pathElement) const;
    index_t Insert (index_t parent, const char* pathElement);
    index_t AutoInsert (index_t parent, const char* pathElement);

    /// return false if concurrent read accesses
    /// would potentially access invalid data.

    bool CanInsertThreadSafely (index_t elements, size_t chars) const;

    /// reset content

    void Clear();

    /// "merge" with another container:
    /// add new entries and return ID mapping for source container

    index_mapping_t Merge (const CPathDictionary& source);

    /// stream I/O

    friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
                                             , CPathDictionary& dictionary);
    friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
                                              , const CPathDictionary& dictionary);

    /// for statistics

    friend class CLogCacheStatistics;
};

/// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
                                  , CPathDictionary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
                                   , const CPathDictionary& dictionary);

/**
 * A path whose structure is (fully) represented in a @a dictionary.
 * Basically, this is a (@a dictionary, @a index) pair that identifies
 * the path unambiguously. All path operations are implemented using
 * the path dictionary.
 */

class CDictionaryBasedPath
{
private:

    /// our dictionary and position within it

    const CPathDictionary* dictionary;
    index_t index;

#ifdef _DEBUG
    /// the path expanded into a string - for easier debugging only

    std::string _path;
#endif

protected:

    /// index manipulation

    void SetIndex (index_t newIndex)
    {
        index = newIndex;
    #ifdef _DEBUG
        _path = GetPath();
    #endif
    }

    /// construction utility: lookup and optionally auto-insert

    void ParsePath ( const std::string& path
                   , CPathDictionary* writableDictionary = NULL
                   , std::vector<std::string>* relPath = NULL);

    /// comparison utility

    bool IsSameOrParentOf (index_t lhsIndex, index_t rhsIndex) const;

    bool Intersects (index_t lhsIndex, index_t rhsIndex) const
    {
        return lhsIndex < rhsIndex
            ? IsSameOrParentOf (lhsIndex, rhsIndex)
            : IsSameOrParentOf (rhsIndex, lhsIndex);
    }

    /// element access

    std::string ReverseAt (size_t reverseIndex) const;

public:

    /// construction / destruction

    CDictionaryBasedPath (const CPathDictionary* aDictionary, index_t anIndex)
        : dictionary (aDictionary)
        , index (anIndex)
    {
    #ifdef _DEBUG
        _path = GetPath();
    #endif
    }

    CDictionaryBasedPath ( const CPathDictionary* aDictionary
                         , const std::string& path);

    CDictionaryBasedPath ( CPathDictionary* aDictionary
                         , const std::string& path
                         , bool nextParent);

    ~CDictionaryBasedPath()
    {
    }

    /// return false if concurrent read accesses
    /// would potentially access invalid data.

    static bool CanParsePathThreadSafely ( const CPathDictionary* dictionary
                                         , const std::string& path);

    /// data access

    index_t GetIndex() const
    {
        return index;
    }

    const CPathDictionary* GetDictionary() const
    {
        return dictionary;
    }

    /// path operations

    bool IsRoot() const
    {
        return index == 0;
    }

    index_t GetDepth() const;

    std::string operator[](size_t index) const
    {
        return ReverseAt (GetDepth() - index - 1);
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

    bool IsSameOrParentOf (index_t rhsIndex) const
    {
        return IsSameOrParentOf (index, rhsIndex);
    }

    bool IsSameOrParentOf (const CDictionaryBasedPath& rhs) const
    {
        return IsSameOrParentOf (index, rhs.index);
    }

    bool IsSameOrChildOf (index_t rhsIndex) const
    {
        return IsSameOrParentOf (rhsIndex, index);
    }

    bool IsSameOrChildOf (const CDictionaryBasedPath& rhs) const
    {
        return IsSameOrParentOf (rhs.index, index);
    }

    bool Intersects (index_t rhsIndex) const
    {
        return Intersects (rhsIndex, index);
    }

    bool Intersects (const CDictionaryBasedPath& rhs) const
    {
        return Intersects (rhs.index, index);
    }

    bool Contains (index_t pathElementID) const;

    /// general comparison

    bool operator==(const CDictionaryBasedPath& rhs) const
    {
        assert (dictionary == rhs.dictionary);
        return index == rhs.index;
    }

    bool operator!=(const CDictionaryBasedPath& rhs) const
    {
        return !operator==(rhs);
    }

    /// convert to string

    std::string GetPath() const;
    void GetPath (std::string& result) const;
};

/// standard operator used by STL containers
/// Note: This is not lexicographical order!

inline bool operator< ( const CDictionaryBasedPath& lhs
                      , const CDictionaryBasedPath& rhs)
{
    // quick compare: indices and counters

    return lhs.GetIndex() < rhs.GetIndex();
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

