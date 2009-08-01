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
 * A very efficient storage for text strings. Every string gets tokenized into 
 * "words" (actually, word + delimiter). While the words are stored in a 
 * dictionary, the stringData contains the token indices (see below). A string 
 * 'i' is the part of stringData at indices offsets[i] .. offsets[i+1]-1.
 *
 * The token sequences themselves are further compressed by replacing common 
 * pairs of tokens with special "pair" tokens. Those pairs can be combined with 
 * further tokens and so on. Only pairs that have been found more than twice 
 * will be replaced by a pair token.
 *
 * Thus, the tokens are interpreted as follows:
 *
 * -1				empty token (allowed only temporarily)
 * -2 .. MIN_INT	word dictionary index 1 .. MAX_INT-1
 * 0 .. MAX_INT		pair dictionary index 0 .. MAX_INT
 *
 * A word token overflow is not possible since the word dictionary can hold 
 * only 4G chars, i.e. less than 1G words.
 * For a pair token overflow you need 6G (original) pairs, i.e. at least 6G 
 * uncompressed words. So, that is impossible as well.
 *
 * Strings newly added to the container will automatically be compressed using 
 * the existing pair tokens. Compress() should be called before writing the 
 * data to disk since the new strings may allow for further compression.
 */
class CTokenizedStringContainer
{
private:

	/**
	 * A utility class that encapsulates the compression
	 * algorithm. Note, that CTokenizedStringContainer::Insert()
	 * will not introduce new pairs but only use existing ones.
	 * 
	 * This class will be instantiated temporarily. A number
	 * of compression rounds are run until there is little or
	 * no further improvement.
	 * 
	 * Although the algorithm is quite straightforward, there
	 * is a lot of temporary data involved that can best be
	 * handled in this separate class.
	 * 
	 * Please note, that the container is always "maximally"
	 * compressed, i.e. all entries in pairs have actually
	 * been applied to all strings.
	 */

	class CPairPacker
	{
	private:

		typedef std::vector<index_t>::iterator IT;

		/// the container we operate on and the indices
		/// of all strings that might be compressed

		CTokenizedStringContainer* container;
		std::vector<index_t> strings;

		/// the pairs we found and the number of places they occur in

		CIndexPairDictionary newPairs;
		std::vector<index_t> counts;

		/// tokens smaller than that cannot be compressed
		/// (internal optimization as after the initial round 
		/// only combinations with the latest pairs may yield
		/// further pairs)

		index_t minimumToken;

		/// total number of replacements performed so far
		/// (i.e. number of tokens saved)

		index_t replacements;

		/// find all strings that consist of more than one token

		void Initialize();

		/// efficiently determine the iterator range that spans our string

		void GetStringRange (index_t stringIndex, IT& first, IT& last);

		/// add token pairs of one string to our counters

		void AccumulatePairs (index_t stringIndex);
		void AddCompressablePairs();
		bool CompressString (index_t stringIndex);

	public:

		/// construction / destruction

		CPairPacker (CTokenizedStringContainer* aContainer);
		~CPairPacker();

		/// perform one round of compression and return the number of tokens saved

		size_t OneRound();

		/// call this after the last round to remove any empty entries

		void Compact();
	};

	friend class CPairPacker;

    /**
     * Hash function over tokenized strings.
     * Index and value types refer to the string index.
     */

	class CHashFunction
	{
	private:

		/// the dictionary we index with the hash
		/// (used to map index -> value)

		CTokenizedStringContainer* parent;

	public:

		/// simple construction

		CHashFunction (CTokenizedStringContainer* parent);

		/// required typedefs and constants

		typedef index_t value_type;
		typedef index_t index_type;

		enum {NO_INDEX = LogCache::NO_INDEX};

		/// the actual hash function

		size_t operator() (value_type value) const;

		/// dictionary lookup

		value_type value (index_type index) const;

		/// lookup and comparison

		bool equal (value_type value, index_type index) const;
	};

	friend class CHashFunction;

    /// useful typedefs

	typedef std::vector<index_t>::const_iterator TSDIterator;

	typedef std::vector<index_t>::iterator IT;
	typedef std::vector<index_t>::const_iterator CIT;

	/// the token contents: words and pairs

	CStringDictionary words;
	CIndexPairDictionary pairs;

	/// container for all tokens of all strings

	std::vector<index_t> stringData;

	/// marks the ranges within stringData that form the strings

	std::vector<index_t> offsets;

	/// sub-stream IDs

	enum
	{
		WORDS_STREAM_ID = 1,
		PAIRS_STREAM_ID = 2,
		STRINGS_STREAM_ID = 3,
		OFFSETS_STREAM_ID = 4
	};

	/// token coding

	enum 
	{
		EMPTY_TOKEN = NO_INDEX,
		LAST_PAIR_TOKEN = (index_t)NO_INDEX / (index_t)2
	};

	bool IsToken (index_t token) const
	{
		return token != EMPTY_TOKEN;
	}
	bool IsDictionaryWord (index_t token) const
	{
		return token > LAST_PAIR_TOKEN;
	}

	index_t GetWordIndex (index_t token) const
	{
		return EMPTY_TOKEN - token;
	}
	index_t GetPairIndex (index_t token) const
	{
		return token;
	}

	index_t GetWordToken (index_t wordIndex) const
	{
		return EMPTY_TOKEN - wordIndex;
	}
	index_t GetPairToken (index_t pairIndex) const
	{
		return pairIndex;
	}

	/// data access utility

	void AppendToken (std::string& target, index_t token) const;

	/// insertion utilities

	void Append (index_t token);
	void Append (const std::string& s);

	/// range check

	void CheckIndex (index_t index) const;

	/// call this to re-assign indices in an attempt to reduce file size

	void OptimizePairs();

public:

	/// construction / destruction

	CTokenizedStringContainer(void);
	~CTokenizedStringContainer(void);

	/// data access

	index_t size() const
	{
		return (index_t)offsets.size() -1;
	}

	std::string operator[] (index_t index) const;

    /// STL-like behavior

    void swap (CTokenizedStringContainer& rhs);

	/// modification

	index_t Insert (const std::string& s);
    index_t Insert (const std::string& s, size_t count);
	void Remove (index_t index);

	void Compress();
	void AutoCompress();

    /// return false if concurrent read accesses
    /// would potentially access invalid data.

    bool CanInsertThreadSafely (const std::string& s) const;

	/// reset content

	void Clear();

	/// statistics

	size_t UncompressedWordCount() const;
	size_t CompressedWordCount() const;
    size_t WorkTokenCount() const;
    size_t PairTokenCount() const;
    size_t WorkTokenSize() const;

	/// batch modifications
	/// indexes must be in ascending order

	/// indexMap maps target index -> source index, 
	/// entries with target index >= size() will be appended

	void Remove (const std::vector<index_t>& indexes);
	void Replace ( const CTokenizedStringContainer& source
				 , const index_mapping_t& indexMap);

    /// Make sure, every string sequence occurs only once.
    /// Return the new index values in \ref newIndices.

    void Unify (std::vector<index_t>& newIndexes);

	/// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CTokenizedStringContainer& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CTokenizedStringContainer& container);

	/// for statistics

	friend class CLogCacheStatistics;
};

/// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CTokenizedStringContainer& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CTokenizedStringContainer& container);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

