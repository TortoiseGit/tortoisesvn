#include "StdAfx.h"
#include "GraphNodeState.h"
#include "FullGraph.h"
#include "FullGraphNode.h"

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

// if we reset a flag in source, reset the corresponding flag in target.
// Also, set it initially, when adding this link.

void CGraphNodeStates::AddLink ( const CFullGraphNode* source
                               , const CFullGraphNode* target
                               , DWORD flags)
{
    assert (states.find (source) != states.end());

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
    }
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
