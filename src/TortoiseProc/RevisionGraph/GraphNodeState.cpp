// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "GraphNodeState.h"
#include "FullGraph.h"
#include "FullGraphNode.h"
#include "VisibleGraphNode.h"

// restore state from saved data

void CGraphNodeStates::RestoreStates 
    ( const TSavedStates& saved
    , const CFullGraphNode* node
    , TDescriptorMap& mapping)
{
    // crawl the branch and its sub-tree
    // restore state of every node that had a state before

    TSavedStates::const_iterator end = saved.end();
    for (; node != NULL; node = node->GetNext())
    {
        // breadth-first

        for ( const CFullGraphNode::CCopyTarget* target 
                = node->GetFirstCopyTarget()
            ; target != NULL
            ; target = target->next())
        {
            RestoreStates (saved, target->value(), mapping);
        }

        // (rev, path) lookup for this node

        TNodeDescriptor key (node->GetRevision(), node->GetPath());
        TSavedStates::const_iterator iter = saved.find (key);

        // restore previous state info, if it had one

        if (iter != end)
        {
            mapping.insert (std::make_pair (key, node));
            SetFlags (node, iter->second);
        }
    }
}

void CGraphNodeStates::RestoreLinks
    ( const TSavedLinks& saved
    , const TDescriptorMap& mapping)
{
    TDescriptorMap::const_iterator endMapping = mapping.end();
    for ( TSavedLinks::const_iterator iter = saved.begin(), end = saved.end()
        ; iter != end
        ; ++iter)
    {
        TDescriptorMap::const_iterator firstMapping 
            = mapping.find (iter->first.first);
        TDescriptorMap::const_iterator secondMapping 
            = mapping.find (iter->second.first);

        if ((firstMapping != endMapping) && (secondMapping != endMapping))
        {
            TNodeState firstState (firstMapping->second, iter->first.second);
            TNodeState secondState (secondMapping->second, iter->first.second);

            links.insert (std::make_pair (firstState, secondState));
        }
    }
}

// construction / destruction

CGraphNodeStates::CGraphNodeStates(void)
{
}

CGraphNodeStates::~CGraphNodeStates(void)
{
}

// store, update and qeuery state

void CGraphNodeStates::SetFlags (const CFullGraphNode* node, DWORD flags)
{
    states[node] |= flags;
}

void CGraphNodeStates::InternalResetFlags (const CFullGraphNode* node, DWORD flags)
{
    TStates::iterator iter = states.find (node);
    if (iter != states.end())
    {
        iter->second &= DWORD(-1) - flags;
        if (iter->second == 0)
            states.erase (iter);
    }
}

void CGraphNodeStates::ResetFlags (const CFullGraphNode* node, DWORD flags)
{
    InternalResetFlags (node, flags);

    TNodeState nodeState (node, flags);
    for ( TLinks::iterator iter = links.find (nodeState)
        ; iter != links.end()
        ; iter = links.find (nodeState))
    {
        TNodeState targetState = iter->second;
        links.erase (iter);

        InternalResetFlags (targetState.first, targetState.second);

        TLinks::iterator targetIter = links.find (targetState);
        if (targetIter != links.end())
            links.erase (targetIter);
    }
}

DWORD CGraphNodeStates::GetFlags (const CFullGraphNode* node) const
{
    TStates::const_iterator iter = states.find (node);
    return iter == states.end()
        ? 0
        : iter->second;
}

// crawl the tree, find the next relavant entries and combine
// the status info

std::pair<const CFullGraphNode*, DWORD>
CGraphNodeStates::FindPreviousRelevant ( const CVisibleGraphNode* node
                                       , DWORD flags
                                       , DWORD myFlags
                                       , DWORD mirroredFlags) const
{
    // already set for this node?

    const CFullGraphNode* base = node->GetBase();
    if ((flags & myFlags) != 0)
        return std::make_pair (base, myFlags);

    // if there still is a predecessor in the visible tree,
    // it obviously didn't get folded or split

    if (node->GetSource() != NULL)
        return std::pair<const CFullGraphNode*, DWORD> (0, 0);

    // walk up the (unfiltered) tree until we find something relevant

    while (base != NULL)
    {
        const CFullGraphNode* previous = base->GetPrevious();
        if (previous == NULL)
        {
            // start of branch:
            // is there a copy source and is it relevant?

            base = base->GetCopySource();
            if ((base != NULL) && (GetFlags (base) & SPLIT_RIGHT))
            {
                return std::make_pair (base, SPLIT_RIGHT);
            }

            // nothing found

            break;
        }

        base = previous;

        DWORD baseFlags = GetFlags (base);
        if (baseFlags & mirroredFlags)
            return std::make_pair (base, mirroredFlags);
        if (baseFlags & myFlags)
            return std::make_pair (base, myFlags);
    }

    // nothing found

    return std::pair<const CFullGraphNode*, DWORD> (0, 0);
}

std::pair<const CFullGraphNode*, DWORD>
CGraphNodeStates::FindNextRelevant ( const CVisibleGraphNode* node
                                   , DWORD flags
                                   , DWORD myFlags
                                   , DWORD mirroredFlags) const
{
    // already set for this node?

    const CFullGraphNode* base = node->GetBase();
    if ((flags & myFlags) != 0)
        return std::make_pair (base, myFlags);

    // if there still is a next in the visible tree,
    // it obviously didn't get folded or split

    if (node->GetNext() != NULL)
        return std::pair<const CFullGraphNode*, DWORD> (0, 0);

    // walk down the (unfiltered) tree until we find something relevant

    while (base != NULL)
    {
        base = base->GetNext();

        DWORD baseFlags = GetFlags (base);
        if (baseFlags & mirroredFlags)
            return std::make_pair (base, mirroredFlags);
        if (baseFlags & myFlags)
            return std::make_pair (base, myFlags);
    }

    // nothing found

    return std::pair<const CFullGraphNode*, DWORD> (0, 0);
}

std::pair<const CFullGraphNode*, DWORD>
CGraphNodeStates::FindRightRelevant ( const CVisibleGraphNode* node
                                    , DWORD flags
                                    , DWORD myFlags
                                    , DWORD mirroredFlags) const
{
    // already set for this node?

    const CFullGraphNode* base = node->GetBase();
    if ((flags & myFlags) != 0)
        return std::make_pair (base, myFlags);

    // any split sub-branch?

    for ( const CFullGraphNode::CCopyTarget* target = base->GetFirstCopyTarget()
        ; target != NULL
        ; target = target->next())
    {
        base = target->value();
        if (GetFlags (base) & mirroredFlags)
            return std::make_pair (base, mirroredFlags);
    }

    // nothing found

    return std::pair<const CFullGraphNode*, DWORD> (0, 0);
}

DWORD CGraphNodeStates::GetFlags (const CVisibleGraphNode* node) const
{
    assert (node != NULL);

    // this node's flags

    DWORD result = GetFlags (node->GetBase());

    // look for flags on hidden nodes as well as mirrored flags

    TFlaggedNode previousFlags = FindPreviousRelevant ( node
                                                      , result
                                                      , COLLAPSED_ABOVE | SPLIT_ABOVE
                                                      , SPLIT_BELOW);

    TFlaggedNode nextFlags = FindPreviousRelevant ( node
                                                  , result
                                                  , COLLAPSED_BELOW | SPLIT_BELOW
                                                  , SPLIT_ABOVE);

    TFlaggedNode rightFlags = FindPreviousRelevant ( node
                                                   , result
                                                   , COLLAPSED_RIGHT | SPLIT_RIGHT
                                                   , SPLIT_ABOVE);

    // combine results

    result |= (previousFlags.second & SPLIT_BELOW) 
            ? SPLIT_ABOVE 
            : previousFlags.second;

    result |= (nextFlags.second & SPLIT_ABOVE) 
            ? SPLIT_BELOW
            : nextFlags.second;

    result |= (rightFlags.second & SPLIT_ABOVE) 
            ? SPLIT_RIGHT
            : rightFlags.second;

    // done here

    return result;
}

// if we reset a flag in source, reset the corresponding flag in target.
// Also, set it initially, when adding this link.

void CGraphNodeStates::AddLink ( const CFullGraphNode* source
                               , const CFullGraphNode* target
                               , DWORD flags)
{
/*    assert (states.find (source) != states.end());

    TNodeState sourceState (source, flags);
    TNodeState targetState (target, 0);

    if (flags & SPLIT_BELOW)
        targetState.second = SPLIT_ABOVE;
    if (flags & SPLIT_RIGHT)
        targetState.second = SPLIT_ABOVE;
    if (flags & SPLIT_ABOVE)
        targetState.second = SPLIT_BELOW;

    if (targetState.second)
    {
        SetFlags (target, targetState.second);

        links.insert (std::make_pair (targetState, sourceState));
        links.insert (std::make_pair (sourceState, targetState));
    }*/
}

// quick update all

void CGraphNodeStates::ResetFlags (DWORD flags)
{
    for (TStates::iterator iter = states.begin(); iter != states.end();)
    {
        iter->second &= DWORD(-1) - flags;
        if (iter->second == 0)
            iter = states.erase (iter);
        else
            ++iter;
    }
}

// disjuctive combination of all flags currently set

DWORD CGraphNodeStates::GetCombinedFlags() const
{
    DWORD result = 0;
    for ( TStates::const_iterator iter = states.begin(), end = states.end()
        ; iter != end
        ; ++iter)
    {
        result |= iter->second;
    }

    return result;
}

// re-qeuery support

CGraphNodeStates::TSavedData CGraphNodeStates::SaveData() const
{
    TSavedData result;

    for ( TStates::const_iterator iter = states.begin(), end = states.end()
        ; iter != end
        ; ++iter)
    {
        TNodeDescriptor key ( iter->first->GetRevision()
                            , iter->first->GetPath());
        result.first.insert (std::make_pair (key, iter->second));
    }

    for ( TLinks::const_iterator iter = links.begin(), end = links.end()
        ; iter != end
        ; ++iter)
    {
        TNodeDescriptor sourceKey ( iter->first.first->GetRevision()
                                  , iter->first.first->GetPath());
        TNodeDescriptor targetKey ( iter->second.first->GetRevision()
                                  , iter->second.first->GetPath());

        TStateDescriptor sourceState (sourceKey, iter->first.second);
        TStateDescriptor targetState (targetKey, iter->second.second);

        result.second.insert (std::make_pair (sourceState, targetState));
    }

    return result;
}

void CGraphNodeStates::LoadData
    ( const CGraphNodeStates::TSavedData& saved
    , const CFullGraph* graph)
{
    TDescriptorMap mapping;

    states.clear();
    RestoreStates (saved.first, graph->GetRoot(), mapping);

    links.clear();
    RestoreLinks (saved.second, mapping);
}
