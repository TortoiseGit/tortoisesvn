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

// required includes

#include "DictionaryBasedTempPath.h"

/**
 * Efficiently classifies a path as "trunk", "branches" or "tags".
 * For every class, a list of wild card patterns separated by semicolon 
 * can be specified. Example "tags;qa ?;qa ?.*". String comparison
 * is case-insensitive for latin chars ('a' .. 'z').
 * A given path may be assigned to multiple classes.
 */

class CPathClassificator
{
public:

    /// classification for a revision graph tree node.
    /// Only the first section will be used by the CPathClassificator.
    /// All others are added by the revision graph.

    enum 
    {
        IS_TRUNK           = 0x01,
        IS_BRANCH          = 0x02,
        IS_TAG             = 0x04,
        IS_OTHER           = 0x08,
        IS_MASK            = 0x0f,

        COPIES_TO_TRUNK    = 0x10,
        COPIES_TO_BRANCH   = 0x20,
        COPIES_TO_TAG      = 0x40,
        COPIES_TO_OTHER    = 0x80,
        COPIES_TO_MASK     = 0xf0,

        IS_DELETED         = 0x100,     // only propagated along a single line
        ALL_COPIES_DELETED = 0x200,
        SUBTREE_DELETED    = IS_DELETED | ALL_COPIES_DELETED,

        IS_MODIFIED        = 0x1000
    };

private:

    class CWildCardPattern
    {
    private:

        /// pattern to match against

        std::string pattern;

        /// algorithm selection

        bool hasWildCards;

        /// different matching algorithms

        bool WildCardMatch (const char* s, const char* pattern) const;
        bool StraightMatch (const char* s, size_t length) const;

        /// construction utility

        void NormalizePattern();

    public:

        /// construction

        CWildCardPattern (const std::string& pattern);

        /// match s against the pattern

        bool Match (const char* s, size_t length) const;
    };

    /// the patterns to detect trunk, branches and tags

    typedef std::vector<CWildCardPattern> TPatterns;

    TPatterns trunkPatterns;
    TPatterns branchesPatterns;
    TPatterns tagsPatterns;

    /// path classification

    std::vector<unsigned char> pathElementClassification;
    std::vector<unsigned char> pathClassification;

    /// construction utilities

    TPatterns ParsePatterns (const std::string& patterns) const;

    bool Matches ( TPatterns::const_iterator firstPattern
                 , TPatterns::const_iterator lastPattern
                 , const char* element
                 , size_t length) const;
    void ClassifyPathElements ( const LogCache::CStringDictionary& elements
                              , const TPatterns& patterns
                              , unsigned char classification);
    void ClassifyPathElements (const LogCache::CStringDictionary& elements);
    void ClassifyPaths (const LogCache::CPathDictionary& paths);

public:

    /// construction / destruction

    CPathClassificator ( const LogCache::CPathDictionary& paths
                       , const std::string& trunkPatterns = "trunk"
                       , const std::string& branchesPatterns = "branches"
                       , const std::string& tagsPatterns = "tags");
    ~CPathClassificator(void);

    /// get the classification for a given path

    unsigned char GetClassification (const LogCache::index_t pathID) const;
    unsigned char operator[](const LogCache::CDictionaryBasedPath& path) const;

    unsigned char GetClassification (const LogCache::CDictionaryBasedTempPath& path) const;
    unsigned char operator[](const LogCache::CDictionaryBasedTempPath& path) const;
};

/// get the classification for a given path

inline unsigned char 
CPathClassificator::GetClassification (const LogCache::index_t pathID) const
{
    return pathClassification[pathID];
}

inline unsigned char 
CPathClassificator::operator[](const LogCache::CDictionaryBasedPath& path) const
{
    return GetClassification (path.GetIndex());
}

inline unsigned char 
CPathClassificator::operator[](const LogCache::CDictionaryBasedTempPath& path) const
{
    return GetClassification (path);
}
