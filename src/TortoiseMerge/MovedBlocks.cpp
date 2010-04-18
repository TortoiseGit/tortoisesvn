// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2010 - TortoiseSVN

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
#include "StdAfx.h"
#include "diff.h"
#include "MovedBlocks.h"
#include "DiffData.h"

// This file implements moved blocks detection algorithm, based
// on WinMerges(http:\\winmerge.org) one

class IntSet
{
public:
    void Add(int val);
    void Remove(int val);
    int Count() const;
    bool IsPresent(int val) const;
    int GetSingle() const;
private:
    CMap<int, int, int, int> m_map;
};                   

struct EquivalencyGroup
{
    IntSet m_LinesLeft;  // equivalent lines on left pane
    IntSet m_LinesRight; // equivalent lines on right pane

    bool IsPerfectMatch() const;
};

class LineToGroupMap : public CTypedPtrMap<CMapStringToPtr, CString, EquivalencyGroup *>
{
public:
    void Add(int lineno, const CString &line, int nside);
    EquivalencyGroup *find(const CString &line);
    ~LineToGroupMap();
};

void IntSet::Add(int val)
{ 
    m_map.SetAt(val, 1);
}

void IntSet::Remove(int val)
{ 
    m_map.RemoveKey(val);
}

int IntSet::Count() const 
{ 
    return (int)m_map.GetCount();
}

bool IntSet::IsPresent(int val) const 
{ 
    int parm;
    return !!m_map.Lookup(val, parm);
}

int IntSet::GetSingle() const 
{
    int val, parm;
    POSITION pos = m_map.GetStartPosition();
    m_map.GetNextAssoc(pos, val, parm); 
    return val; 
}

bool EquivalencyGroup::IsPerfectMatch() const
{
    return (m_LinesLeft.Count() == 1)&&(m_LinesRight.Count() == 1); 
}

void LineToGroupMap::Add(int lineno, const CString &line, int nside)
{
    EquivalencyGroup *pGroup = NULL;
    if ( !Lookup(line, pGroup) )
    {
        pGroup = new EquivalencyGroup;
        SetAt(line, pGroup);
    }
    if(nside)
    {
        pGroup->m_LinesRight.Add(lineno);
    }
    else
    {
        pGroup->m_LinesLeft.Add(lineno);
    }
}

EquivalencyGroup *LineToGroupMap::find(const CString &line)
{
    EquivalencyGroup *pGroup = NULL;
    Lookup(line, pGroup);
    return pGroup;
}

LineToGroupMap::~LineToGroupMap()
{
    for (POSITION pos = GetStartPosition(); pos; )
    {
        CString str;
        EquivalencyGroup *pGroup = NULL;
        GetNextAssoc(pos, str, pGroup);
        delete pGroup;
    }
}

tsvn_svn_diff_t_extension * CreateDiffExtension(svn_diff_t * base, apr_pool_t * pool)
{
    tsvn_svn_diff_t_extension * ext = (tsvn_svn_diff_t_extension *)apr_palloc(pool, sizeof(tsvn_svn_diff_t_extension));
    ext->next = NULL;
    ext->moved_to = -1;
    ext->moved_from = -1;
    ext->base = base;
    return ext;
}

void AdjustExistingAndTail(svn_diff_t * tempdiff, tsvn_svn_diff_t_extension *& existing, tsvn_svn_diff_t_extension *& tail)
{
    if(existing && existing->base == tempdiff)
    {
        if(tail && tail != existing)
        {
            tail->next = existing;
        }
        tail = existing;
        existing = existing->next;
    }
}

tsvn_svn_diff_t_extension * CDiffData::MovedBlocksDetect(svn_diff_t * diffYourBase, apr_pool_t * pool)
{
    LineToGroupMap map;
    tsvn_svn_diff_t_extension * head = NULL;
    tsvn_svn_diff_t_extension * tail = NULL;
    svn_diff_t * tempdiff = diffYourBase;
    LONG baseLine = 0;
    LONG yourLine = 0;
    for(;tempdiff; tempdiff = tempdiff->next) // fill map
    {
        if(tempdiff->type != svn_diff__type_diff_modified)
            continue;
        
        baseLine = (LONG)tempdiff->original_start;
        for(int i = 0; i < tempdiff->original_length; ++i, ++baseLine)
        {
            map.Add(baseLine, m_arBaseFile.GetAt(baseLine), 0);            
        }
        yourLine = (LONG)tempdiff->modified_start;
        for(int i = 0; i < tempdiff->modified_length; ++i, ++yourLine)
        {
            map.Add(yourLine, m_arYourFile.GetAt(yourLine), 1);            
        }                                
    } 
    for(tempdiff = diffYourBase; tempdiff; tempdiff = tempdiff->next)
    {
    // Scan through diff blocks, finding moved sections from left side
    // and splitting them out
    // That is, we actually fragment diff blocks as we find moved sections
        if(tempdiff->type != svn_diff__type_diff_modified)
            continue;

        EquivalencyGroup * pGroup = NULL;                  

        int i;
        for(i = (int)tempdiff->original_start; (i - tempdiff->original_start)< tempdiff->original_length; ++i)
        {
            EquivalencyGroup * group = map.find(m_arBaseFile.GetAt(i));
            if(group->IsPerfectMatch())
            {
                pGroup = group;
                break;
            }
        }
        if(!pGroup) // if no match
            continue;
        // found a match
        int j = pGroup->m_LinesRight.GetSingle();
        // Ok, now our moved block is the single line (i, j)

        // extend moved block upward as far as possible
        int i1 = i - 1;
        int j1 = j - 1;
        for(; i1 >= tempdiff->original_start; --i1, --j1)
        {
            EquivalencyGroup * pGroup0 = map.find(m_arBaseFile.GetAt(i1));
            EquivalencyGroup * pGroup1 = map.find(m_arYourFile.GetAt(j1));
            if(pGroup1 != pGroup0)
                break;
            pGroup0->m_LinesLeft.Remove(i1);
            pGroup1->m_LinesRight.Remove(j1);
        }
        ++i1;
        ++j1;
        // Ok, now our moved block is (i1..i, j1..j)

        // extend moved block downward as far as possible

        int i2 = i + 1;
        int j2 = j + 1;
        for(; (i2-tempdiff->original_start) < tempdiff->original_length; ++i2, ++j2)
        {
            if(i2 >= m_arBaseFile.GetCount() || j2 >= m_arYourFile.GetCount())
                break;
            EquivalencyGroup * pGroup0 = map.find(m_arBaseFile.GetAt(i2));
            EquivalencyGroup * pGroup1 = map.find(m_arYourFile.GetAt(j2));
            if(pGroup1 != pGroup0)
                break;
            pGroup0->m_LinesLeft.Remove(i2);
            pGroup1->m_LinesRight.Remove(j2);
        }
        --i2;
        --j2;
        // Ok, now our moved block is (i1..i2,j1..j2)
        tsvn_svn_diff_t_extension * newTail = CreateDiffExtension(tempdiff, pool);
        if(head == NULL)
        {
            head = newTail;
            tail = head;
        }
        else
        {
            tail->next = newTail;
            tail = newTail;
        }

        int prefix = i1 - (int)tempdiff->original_start;
        if(prefix)
        {
            // break tempdiff (current change) into two pieces
            // first part is the prefix, before the moved part
            // that stays in tempdiff
            // second part is the moved part & anything after it
            // that goes in newob
            // leave the left side (tempdiff->original_length) on tempdiff
            // so no right side on newob
            // newob will be the moved part only, later after we split off any suffix from it
            svn_diff_t * newob = (svn_diff_t *)apr_palloc(pool, sizeof(svn_diff_t));
            memset(newob, 0, sizeof(*newob));

            tail->base = newob;
            newob->type = svn_diff__type_diff_modified;
            newob->original_start = i1;
            newob->modified_start = tempdiff->modified_start + tempdiff->modified_length;
            newob->modified_length = 0;
            newob->original_length = tempdiff->original_length - prefix;
            newob->next = tempdiff->next;

            tempdiff->original_length = prefix;
            tempdiff->next = newob;

            // now make tempdiff point to the moved part (& any suffix)
            tempdiff = newob;
        }

        tail->moved_to = j1;
        
        apr_off_t suffix = (tempdiff->original_length) - (i2- (tempdiff->original_start)) - 1;
        if (suffix)
        {
            // break off any suffix from tempdiff
            // newob will be the suffix, and will get all the right side
            svn_diff_t * newob = (svn_diff_t *) apr_palloc(pool, sizeof (*newob));
            memset(newob, 0, sizeof(*newob));
            newob->type = svn_diff__type_diff_modified;

            newob->original_start = i2 + 1;
            newob->modified_start = tempdiff->modified_start;
            newob->modified_length = tempdiff->modified_length;
            newob->original_length = suffix;
            newob->next = tempdiff->next;

            tempdiff->modified_length = 0;
            tempdiff->original_length -= suffix;
            tempdiff->next = newob;
        }
    }
    // Scan through diff blocks, finding moved sections from right side
    // and splitting them out
    // That is, we actually fragment diff blocks as we find moved sections
    tsvn_svn_diff_t_extension * existing = head;
    tail = NULL;
    for(tempdiff = diffYourBase; tempdiff; tempdiff = tempdiff->next)
    {
        // scan down block for a match
        if(tempdiff->type != svn_diff__type_diff_modified)
            continue;

        EquivalencyGroup * pGroup = NULL;
        int j = 0; 
        for(j = (int)tempdiff->modified_start; (j - tempdiff->modified_start) < tempdiff->modified_length; ++j)
        {
            EquivalencyGroup * group = map.find(m_arYourFile.GetAt(j));
            if(group->IsPerfectMatch())
            {
                pGroup = group;
                break;
            }
        }

        // if no match, go to next diff block
        if (!pGroup)
        {
            AdjustExistingAndTail(tempdiff, existing, tail);
            continue;
        }

        // found a match
        int i = pGroup->m_LinesLeft.GetSingle();
        // Ok, now our moved block is the single line (i,j)

        // extend moved block upward as far as possible
        int i1 = i-1;
        int j1 = j-1;
        for ( ; j1>=tempdiff->modified_start; --i1, --j1)
        {
            EquivalencyGroup * pGroup0 = map.find(m_arBaseFile.GetAt(i1));
            EquivalencyGroup * pGroup1 = map.find(m_arYourFile.GetAt(j1));
            if (pGroup0 != pGroup1)
                break;
            pGroup0->m_LinesLeft.Remove(i1);
            pGroup1->m_LinesRight.Remove(j1);
        }
        ++i1;
        ++j1;
        // Ok, now our moved block is (i1..i,j1..j)

        // extend moved block downward as far as possible
        int i2 = i+1;
        int j2 = j+1;
        for ( ; j2-(tempdiff->modified_start) < tempdiff->modified_length; ++i2,++j2)
        {
            if(i2 >= m_arBaseFile.GetCount() || j2 >= m_arYourFile.GetCount())
                break;
            EquivalencyGroup * pGroup0 = map.find(m_arBaseFile.GetAt(i2));
            EquivalencyGroup * pGroup1 = map.find(m_arYourFile.GetAt(j2));
            if (pGroup0 != pGroup1)
                break;
            pGroup0->m_LinesLeft.Remove(i2);
            pGroup1->m_LinesRight.Remove(j2);
        }
        --i2;
        --j2;
        // Ok, now our moved block is (i1..i2,j1..j2)
        tsvn_svn_diff_t_extension * newTail = NULL;
        if(existing && existing->base == tempdiff)
        {
            newTail = existing;
        }
        else
        {
            newTail = CreateDiffExtension(tempdiff, pool);
            if(head == NULL)
            {
                head = newTail;
            }
            else if(tail)
            {
                newTail->next = tail->next;
                tail->next = newTail;
            }
        }
        tail = newTail;
        
        apr_off_t prefix = j1 - (tempdiff->modified_start);
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
            svn_diff_t * newob = (svn_diff_t *) apr_palloc(pool, sizeof (*newob));
            memset(newob, 0, sizeof(*newob));
            newob->type = svn_diff__type_diff_modified;
     
            if(existing == newTail)
            {
                newTail = CreateDiffExtension(newob, pool);
                newTail->next = tail->next;
                tail->next = newTail;
                tail = newTail;
            }
            tail->base = newob;
            newob->original_start = tempdiff->original_start + tempdiff->original_length;
            newob->modified_start = j1;
            newob->modified_length = tempdiff->modified_length - prefix;
            newob->original_length = 0;
            newob->next = tempdiff->next;

            tempdiff->modified_length = prefix;
            tempdiff->next = newob;

            // now make tempdiff point to the moved part (& any suffix)
            tempdiff = newob;
        }
        // now tempdiff points to a moved diff chunk with no prefix, but maybe a suffix

        tail->moved_from = i1;

        apr_off_t suffix = (tempdiff->modified_length) - (j2-(tempdiff->modified_start)) - 1;
        if (suffix)
        {
            // break off any suffix from tempdiff
            // newob will be the suffix, and will get all the left side
            svn_diff_t * newob = (svn_diff_t *) apr_palloc(pool, sizeof (*newob));
            memset(newob, 0, sizeof(*newob));
            tsvn_svn_diff_t_extension * eNewOb = CreateDiffExtension(newob, pool);

            newob->type = svn_diff__type_diff_modified;
            newob->original_start = tempdiff->original_start;
            newob->modified_start = j2+1;
            newob->modified_length = suffix;
            newob->original_length = tempdiff->original_length;
            newob->next = tempdiff->next;
            eNewOb->moved_from = -1;
            eNewOb->moved_to = tail->moved_to;

            tempdiff->modified_length -= suffix;
            tempdiff->original_length = 0;
            tail->moved_to = -1;
            tempdiff->next = newob;
            eNewOb->next = tail->next;
            tail->next = eNewOb;
            existing = tail = eNewOb;
        }
        AdjustExistingAndTail(tempdiff, existing, tail);
    }
    return head;
}