// TortoiseSVN - a Windows shell extension for easy version control

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
#include "RemoveTags.h"
#include "FullGraphNode.h"
#include "VisibleGraphNode.h"

// construction

CRemoveTags::CRemoveTags (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement ICopyFilterOption

ICopyFilterOption::EResult
CRemoveTags::ShallRemove (const CFullGraphNode* node) const
{
    DWORD forbidden = CNodeClassification::COPIES_TO_TRUNK
                    | CNodeClassification::COPIES_TO_BRANCH
                    | CNodeClassification::COPIES_TO_OTHER;

    return node->GetClassification().Matches (CNodeClassification::IS_TAG, forbidden)
         ? ICopyFilterOption::REMOVE_SUBTREE
         : ICopyFilterOption::KEEP_NODE;
}

/// implement IModificationOption (post-filter after possibly removing sub-branches)

void CRemoveTags::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    if (   (node->GetNext() == NULL)
        && (node->GetFirstCopyTarget() == NULL)
        && (node->GetFirstTag() == NULL)
        && (node->GetClassification().Is (CNodeClassification::IS_TAG)))
    {
        node->DropNode (graph, true);
    }
}
