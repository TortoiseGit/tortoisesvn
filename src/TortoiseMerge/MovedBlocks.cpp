// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2010-2014, 2020-2021 - TortoiseSVN

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
#include "stdafx.h"
#include "diff.h"
#include "MovedBlocks.h"
#include "DiffData.h"
#include <set>
#include <map>

// This file implements moved blocks detection algorithm, based
// on WinMerges(http:\\winmerge.org) one

class IntSet
{
public:
    void Add(int val);
    void Remove(int val);
    int  Count() const;
    bool IsPresent(int val) const;
    int  GetSingle() const;

private:
    std::set<int> m_set;
};

struct EquivalencyGroup
{
    IntSet linesLeft;  // equivalent lines on left pane
    IntSet linesRight; // equivalent lines on right pane

    bool IsPerfectMatch() const;
};

class LineToGroupMap : public std::map<CString, EquivalencyGroup*>
{
public:
    void              Add(int lineNo, const CString& line, int nSide);
    EquivalencyGroup* find(const CString& line) const;
    ~LineToGroupMap();
};

void IntSet::Add(int val)
{
    m_set.insert(val);
}

void IntSet::Remove(int val)
{
    m_set.erase(val);
}

int IntSet::Count() const
{
    return static_cast<int>(m_set.size());
}

bool IntSet::IsPresent(int val) const
{
    return m_set.find(val) != m_set.end();
}

int IntSet::GetSingle() const
{
    if (!m_set.empty())
    {
        return *m_set.cbegin();
    }
    return 0;
}

bool EquivalencyGroup::IsPerfectMatch() const
{
    return (linesLeft.Count() == 1) && (linesRight.Count() == 1);
}

void LineToGroupMap::Add(int lineNo, const CString& line, int nSide)
{
    EquivalencyGroup* pGroup = nullptr;
    auto              it     = __super::find(line);
    if (it == cend())
    {
        pGroup = new EquivalencyGroup;
        insert(std::pair<CString, EquivalencyGroup*>(line, pGroup));
    }
    else
        pGroup = it->second;
    if (nSide)
    {
        pGroup->linesRight.Add(lineNo);
    }
    else
    {
        pGroup->linesLeft.Add(lineNo);
    }
}

EquivalencyGroup* LineToGroupMap::find(const CString& line) const
{
    EquivalencyGroup* pGroup = nullptr;
    auto              it     = __super::find(line);
    if (it != cend())
        pGroup = it->second;
    return pGroup;
}

LineToGroupMap::~LineToGroupMap()
{
    for (auto it = cbegin(); it != cend(); ++it)
    {
        delete it->second;
    }
}

tsvn_svn_diff_t_extension* CreateDiffExtension(svn_diff_t* base, apr_pool_t* pool)
{
    tsvn_svn_diff_t_extension* ext = static_cast<tsvn_svn_diff_t_extension*>(apr_palloc(pool, sizeof(tsvn_svn_diff_t_extension)));
    ext->next                      = nullptr;
    ext->movedTo                   = -1;
    ext->movedFrom                 = -1;
    ext->base                      = base;
    return ext;
}

void AdjustExistingAndTail(svn_diff_t* tempDiff, tsvn_svn_diff_t_extension*& existing, tsvn_svn_diff_t_extension*& tail)
{
    if (existing && existing->base == tempDiff)
    {
        if (tail && tail != existing)
        {
            tail->next = existing;
        }
        tail     = existing;
        existing = existing->next;
    }
}

CString GetTrimmedString(const CString& s1, IgnoreWS ignoreWs)
{
    if (ignoreWs == IgnoreWS::AllWhiteSpaces)
    {
        CString s2 = s1;
        s2.Remove(' ');
        s2.Remove('\t');
        return s2;
    }
    else if (ignoreWs == IgnoreWS::WhiteSpaces)
        return CString(s1).TrimLeft(L" \t");
    return CString(s1).TrimRight(L" \t");
}

EquivalencyGroup* ExtractGroup(const LineToGroupMap& map, const CString& line, IgnoreWS ignoreWS)
{
    if (ignoreWS != IgnoreWS::None)
        return map.find(GetTrimmedString(line, ignoreWS));
    return map.find(line);
}

tsvn_svn_diff_t_extension* CDiffData::MovedBlocksDetect(svn_diff_t* diffYourBase, IgnoreWS ignoreWs, apr_pool_t* pool) const
{
    LineToGroupMap             map;
    tsvn_svn_diff_t_extension* head     = nullptr;
    tsvn_svn_diff_t_extension* tail     = nullptr;
    svn_diff_t*                tempDiff = diffYourBase;
    LONG                       baseLine = 0;
    LONG                       yourLine = 0;
    for (; tempDiff; tempDiff = tempDiff->next) // fill map
    {
        if (tempDiff->type != svn_diff__type_diff_modified)
            continue;

        baseLine = static_cast<LONG>(tempDiff->original_start);
        if (m_arBaseFile.GetCount() <= (baseLine + tempDiff->original_length))
            return nullptr;
        for (int i = 0; i < tempDiff->original_length; ++i, ++baseLine)
        {
            const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseLine);
            if (ignoreWs != IgnoreWS::None)
                map.Add(baseLine, GetTrimmedString(sCurrentBaseLine, ignoreWs), 0);
            else
                map.Add(baseLine, sCurrentBaseLine, 0);
        }
        yourLine = static_cast<LONG>(tempDiff->modified_start);
        if (m_arYourFile.GetCount() <= (yourLine + tempDiff->modified_length))
            return nullptr;
        for (int i = 0; i < tempDiff->modified_length; ++i, ++yourLine)
        {
            const CString& sCurrentYourLine = m_arYourFile.GetAt(yourLine);
            if (ignoreWs != IgnoreWS::None)
                map.Add(yourLine, GetTrimmedString(sCurrentYourLine, ignoreWs), 1);
            else
                map.Add(yourLine, sCurrentYourLine, 1);
        }
    }
    for (tempDiff = diffYourBase; tempDiff; tempDiff = tempDiff->next)
    {
        // Scan through diff blocks, finding moved sections from left side
        // and splitting them out
        // That is, we actually fragment diff blocks as we find moved sections
        if (tempDiff->type != svn_diff__type_diff_modified)
            continue;

        EquivalencyGroup* pGroup = nullptr;

        int i;
        for (i = static_cast<int>(tempDiff->original_start); (i - tempDiff->original_start) < tempDiff->original_length; ++i)
        {
            EquivalencyGroup* group = ExtractGroup(map, m_arBaseFile.GetAt(i), ignoreWs);
            if (group->IsPerfectMatch())
            {
                pGroup = group;
                break;
            }
        }
        if (!pGroup) // if no match
            continue;
        // found a match
        int j = pGroup->linesRight.GetSingle();
        // Ok, now our moved block is the single line (i, j)

        // extend moved block upward as far as possible
        int i1 = i - 1;
        int j1 = j - 1;
        for (; (i1 >= tempDiff->original_start) && (j1 >= 0) && (i1 >= 0); --i1, --j1)
        {
            EquivalencyGroup* pGroup0 = ExtractGroup(map, m_arBaseFile.GetAt(i1), ignoreWs);
            EquivalencyGroup* pGroup1 = ExtractGroup(map, m_arYourFile.GetAt(j1), ignoreWs);
            if (pGroup1 != pGroup0)
                break;
            pGroup0->linesLeft.Remove(i1);
            pGroup1->linesRight.Remove(j1);
        }
        ++i1;
        ++j1;
        // Ok, now our moved block is (i1..i, j1..j)

        // extend moved block downward as far as possible

        int i2 = i + 1;
        int j2 = j + 1;
        for (; ((i2 - tempDiff->original_start) < tempDiff->original_length) && (j2 >= 0); ++i2, ++j2)
        {
            if (i2 >= m_arBaseFile.GetCount() || j2 >= m_arYourFile.GetCount())
                break;
            EquivalencyGroup* pGroup0 = ExtractGroup(map, m_arBaseFile.GetAt(i2), ignoreWs);
            EquivalencyGroup* pGroup1 = ExtractGroup(map, m_arYourFile.GetAt(j2), ignoreWs);
            if (pGroup1 != pGroup0)
                break;
            pGroup0->linesLeft.Remove(i2);
            pGroup1->linesRight.Remove(j2);
        }
        --i2;
        --j2;
        // Ok, now our moved block is (i1..i2,j1..j2)
        tsvn_svn_diff_t_extension* newTail = CreateDiffExtension(tempDiff, pool);
        if (!head)
        {
            head = newTail;
            tail = head;
        }
        else
        {
            tail->next = newTail;
            tail       = newTail;
        }

        int prefix = i1 - static_cast<int>(tempDiff->original_start);
        if (prefix)
        {
            // break tempdiff (current change) into two pieces
            // first part is the prefix, before the moved part
            // that stays in tempdiff
            // second part is the moved part & anything after it
            // that goes in newob
            // leave the left side (tempdiff->original_length) on tempdiff
            // so no right side on newob
            // newob will be the moved part only, later after we split off any suffix from it
            svn_diff_t* newOb = static_cast<svn_diff_t*>(apr_palloc(pool, sizeof(svn_diff_t)));
            SecureZeroMemory(newOb, sizeof(*newOb));

            tail->base             = newOb;
            newOb->type            = svn_diff__type_diff_modified;
            newOb->original_start  = i1;
            newOb->modified_start  = tempDiff->modified_start + tempDiff->modified_length;
            newOb->modified_length = 0;
            newOb->original_length = tempDiff->original_length - prefix;
            newOb->next            = tempDiff->next;

            tempDiff->original_length = prefix;
            tempDiff->next            = newOb;

            // now make tempdiff point to the moved part (& any suffix)
            tempDiff = newOb;
        }

        tail->movedTo = j1;

        apr_off_t suffix = (tempDiff->original_length) - (i2 - (tempDiff->original_start)) - 1;
        if (suffix)
        {
            // break off any suffix from tempdiff
            // newob will be the suffix, and will get all the right side
            svn_diff_t* newOb = static_cast<svn_diff_t*>(apr_palloc(pool, sizeof(*newOb)));
            SecureZeroMemory(newOb, sizeof(*newOb));
            newOb->type = svn_diff__type_diff_modified;

            newOb->original_start  = i2 + 1LL;
            newOb->modified_start  = tempDiff->modified_start;
            newOb->modified_length = tempDiff->modified_length;
            newOb->original_length = suffix;
            newOb->next            = tempDiff->next;

            tempDiff->modified_length = 0;
            tempDiff->original_length -= suffix;
            tempDiff->next = newOb;
        }
    }
    // Scan through diff blocks, finding moved sections from right side
    // and splitting them out
    // That is, we actually fragment diff blocks as we find moved sections
    tsvn_svn_diff_t_extension* existing = head;
    tail                                = nullptr;
    for (tempDiff = diffYourBase; tempDiff; tempDiff = tempDiff->next)
    {
        // scan down block for a match
        if (tempDiff->type != svn_diff__type_diff_modified)
            continue;

        EquivalencyGroup* pGroup = nullptr;
        int               j      = 0;
        for (j = static_cast<int>(tempDiff->modified_start); (j - tempDiff->modified_start) < tempDiff->modified_length; ++j)
        {
            EquivalencyGroup* group = ExtractGroup(map, m_arYourFile.GetAt(j), ignoreWs);
            if (group->IsPerfectMatch())
            {
                pGroup = group;
                break;
            }
        }

        // if no match, go to next diff block
        if (!pGroup)
        {
            AdjustExistingAndTail(tempDiff, existing, tail);
            continue;
        }

        // found a match
        int i = pGroup->linesLeft.GetSingle();
        if (i == 0)
            continue;
        // Ok, now our moved block is the single line (i,j)

        // extend moved block upward as far as possible
        int i1 = i - 1;
        int j1 = j - 1;
        for (; (j1 >= tempDiff->modified_start) && (j1 >= 0) && (i1 >= 0); --i1, --j1)
        {
            EquivalencyGroup* pGroup0 = ExtractGroup(map, m_arBaseFile.GetAt(i1), ignoreWs);
            EquivalencyGroup* pGroup1 = ExtractGroup(map, m_arYourFile.GetAt(j1), ignoreWs);
            if (pGroup0 != pGroup1)
                break;
            pGroup0->linesLeft.Remove(i1);
            pGroup1->linesRight.Remove(j1);
        }
        ++i1;
        ++j1;
        // Ok, now our moved block is (i1..i,j1..j)

        // extend moved block downward as far as possible
        int i2 = i + 1;
        int j2 = j + 1;
        for (; (j2 - (tempDiff->modified_start) < tempDiff->modified_length) && (i2 >= 0); ++i2, ++j2)
        {
            if (i2 >= m_arBaseFile.GetCount() || j2 >= m_arYourFile.GetCount())
                break;
            EquivalencyGroup* pGroup0 = ExtractGroup(map, m_arBaseFile.GetAt(i2), ignoreWs);
            EquivalencyGroup* pGroup1 = ExtractGroup(map, m_arYourFile.GetAt(j2), ignoreWs);
            if (pGroup0 != pGroup1)
                break;
            pGroup0->linesLeft.Remove(i2);
            pGroup1->linesRight.Remove(j2);
        }
        --i2;
        --j2;
        // Ok, now our moved block is (i1..i2,j1..j2)
        tsvn_svn_diff_t_extension* newTail = nullptr;
        if (existing && existing->base == tempDiff)
        {
            newTail = existing;
        }
        else
        {
            newTail = CreateDiffExtension(tempDiff, pool);
            if (!head)
                head = newTail;
            else if (tail)
            {
                newTail->next = tail->next;
                tail->next    = newTail;
            }
        }
        tail = newTail;

        apr_off_t prefix = j1 - (tempDiff->modified_start);
        if (prefix)
        {
            // break tempdiff (current change) into two pieces
            // first part is the prefix, before the moved part
            // that stays in tempdiff
            // second part is the moved part & anything after it
            // that goes in newob
            // leave the left side (tempdiff->original_length) on tempdiff
            // so no right side on newob
            // newob will be the moved part only, later after we split off any suffix from it
            svn_diff_t* newOb = static_cast<svn_diff_t*>(apr_palloc(pool, sizeof(*newOb)));
            SecureZeroMemory(newOb, sizeof(*newOb));
            newOb->type = svn_diff__type_diff_modified;

            if (existing == newTail)
            {
                newTail       = CreateDiffExtension(newOb, pool);
                newTail->next = tail->next;
                tail->next    = newTail;
                tail          = newTail;
            }
            tail->base             = newOb;
            newOb->original_start  = tempDiff->original_start + tempDiff->original_length;
            newOb->modified_start  = j1;
            newOb->modified_length = tempDiff->modified_length - prefix;
            newOb->original_length = 0;
            newOb->next            = tempDiff->next;

            tempDiff->modified_length = prefix;
            tempDiff->next            = newOb;

            // now make tempdiff point to the moved part (& any suffix)
            tempDiff = newOb;
        }
        // now tempdiff points to a moved diff chunk with no prefix, but maybe a suffix

        tail->movedFrom = i1;

        apr_off_t suffix = (tempDiff->modified_length) - (j2 - (tempDiff->modified_start)) - 1;
        if (suffix)
        {
            // break off any suffix from tempdiff
            // newob will be the suffix, and will get all the left side
            svn_diff_t* newOb = static_cast<svn_diff_t*>(apr_palloc(pool, sizeof(*newOb)));
            SecureZeroMemory(newOb, sizeof(*newOb));
            tsvn_svn_diff_t_extension* eNewOb = CreateDiffExtension(newOb, pool);

            newOb->type            = svn_diff__type_diff_modified;
            newOb->original_start  = tempDiff->original_start;
            newOb->modified_start  = j2 + 1;
            newOb->modified_length = suffix;
            newOb->original_length = tempDiff->original_length;
            newOb->next            = tempDiff->next;
            eNewOb->movedFrom      = -1;
            eNewOb->movedTo        = tail->movedTo;

            tempDiff->modified_length -= suffix;
            tempDiff->original_length = 0;
            tail->movedTo             = -1;
            tempDiff->next            = newOb;
            eNewOb->next              = tail->next;
            tail->next                = eNewOb;
            existing = tail = eNewOb;
        }
        AdjustExistingAndTail(tempDiff, existing, tail);
    }
    return head;
}
