#include "StdAfx.h"
#include "GraphNodeState.h"
#include "FullGraph.h"
#include "FullGraphNode.h"

// restore state from saved data

void CGraphNodeStates::RestoreStates 
    ( const CGraphNodeStates::TSavedStates& saved
    , const CFullGraphNode* node)
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
            RestoreStates (saved, target->value());
        }

        // (rev, path) lookup for this node

        TSavedStates::key_type key (node->GetRevision(), node->GetPath());
        TSavedStates::const_iterator iter = saved.find (key);

        // restore previous state info, if it had one

        if (iter != end)
            SetFlags (node, iter->second);
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

void CGraphNodeStates::ResetFlags (const CFullGraphNode* node, DWORD flags)
{
    TStates::iterator iter = states.find (node);
    iter->second &= DWORD(-1) - flags;

    if (iter->second == 0)
        states.erase (iter);
}

DWORD CGraphNodeStates::GetFlags (const CFullGraphNode* node) const
{
    TStates::const_iterator iter = states.find (node);
    return iter == states.end()
        ? 0
        : iter->second;
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

CGraphNodeStates::TSavedStates CGraphNodeStates::SaveStates() const
{
    TSavedStates result;

    for ( TStates::const_iterator iter = states.begin(), end = states.end()
        ; iter != end
        ; ++iter)
    {
        TSavedStates::key_type key ( iter->first->GetRevision()
                                   , iter->first->GetPath());
        result.insert (std::make_pair (key, iter->second));
    }

    return result;
}

void CGraphNodeStates::LoadStates 
    ( const CGraphNodeStates::TSavedStates& saved
    , const CFullGraph* graph)
{
    states.clear();
    RestoreStates (saved, graph->GetRoot());
}
