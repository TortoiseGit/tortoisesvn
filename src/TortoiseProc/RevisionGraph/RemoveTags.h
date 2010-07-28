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
#pragma once

// include base classes

#include "CopyFilterOptions.h"
#include "ModificationOptions.h"
#include "revisiongraphoptionsimpl.h"
#include "Resource.h"

/** Remove all branches / tags that have been deleted and have
* no surviving copy.
*/

class CRemoveTags
    : public CModificationOptionImpl
                < CCombineInterface
                    < ICopyFilterOption
                    , IModificationOption>
                , 90
                , ID_VIEW_REMOVETAGS
                , true           // crawl branches first
                , false          // root last
                , true>          // this is a cyclic option
{
public:

    /// construction

    CRemoveTags (CRevisionGraphOptionList& list);

    /// implement ICopyFilterOption (pre-filter most nodes)

    virtual EResult ShallRemove (const CFullGraphNode* node) const;

    /// implement IModificationOption (post-filter deleted non-tagged branches)

    virtual void Apply (CVisibleGraph* graph, CVisibleGraphNode* node);
};
