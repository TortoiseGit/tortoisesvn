// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010 - TortoiseSVN

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
#include <gdiplus.h>
#include "registry.h"

/**
 * \ingroup TortoiseProc
 * Handles the UI colors used in TortoiseSVN
 */
class CColors
{
public:
    CColors(void);
    ~CColors(void);

    enum Colors
    {
        Cmd = 0,
        Conflict = 1,
        Modified = 2,
        Merged = 3,
        Deleted = 4,
        Added = 5,
        LastCommit = 6,
        DeletedNode = 7,
        AddedNode = 8,
        ReplacedNode = 9,
        RenamedNode = 10,
        LastCommitNode = 11,
        PropertyChanged = 12,
        FilterMatch = 13
    };

    enum GDIPlusColor
    {
        gdpDeletedNode = 7,
        gdpAddedNode = 8,
        gdpRenamedNode = 10,
        gdpLastCommitNode = 11,

        gdpModifiedNode = 13,
        gdpWCNode = 14,
        gdpUnchangedNode = 15,
        gdpTagOverlay = 16,
        gdpTrunkOverlay = 17,

        gdpStripeColor1 = 18,
        gdpStripeColor2 = 19,

        gdpWCNodeBorder = 20
    };

    enum GDIPlusColorTable
    {
        ctMarkers = 0
    };

    COLORREF GetColor (Colors col, bool bDefault = false);
    void SetColor(Colors col, COLORREF cr);

    Gdiplus::Color GetColor (GDIPlusColor id, bool bDefault = false);
    void SetColor (GDIPlusColor id, Gdiplus::Color color);

    Gdiplus::Color GetColor (GDIPlusColorTable id, int index, bool bDefault = false);
    void SetColor (GDIPlusColorTable id, int index, Gdiplus::Color color);

private:
    CRegDWORD m_regCmd;
    CRegDWORD m_regConflict;
    CRegDWORD m_regModified;
    CRegDWORD m_regMerged;
    CRegDWORD m_regDeleted;
    CRegDWORD m_regAdded;
    CRegDWORD m_regLastCommit;
    CRegDWORD m_regDeletedNode;
    CRegDWORD m_regAddedNode;
    CRegDWORD m_regReplacedNode;
    CRegDWORD m_regRenamedNode;
    CRegDWORD m_regLastCommitNode;
    CRegDWORD m_regPropertyChanged;
    CRegDWORD m_regFilterMatch;

    CRegDWORD m_regGDPDeletedNode;
    CRegDWORD m_regGDPAddedNode;
    CRegDWORD m_regGDPRenamedNode;
    CRegDWORD m_regGDPLastCommit;
    CRegDWORD m_regGDPModifiedNode;
    CRegDWORD m_regGDPWCNode;
    CRegDWORD m_regGDPUnchangedNode;
    CRegDWORD m_regGDPTagOverlay;
    CRegDWORD m_regGDPTrunkOverlay;
    CRegDWORD m_regGDPStripeColor1;
    CRegDWORD m_regGDPStripeColor2;
    CRegDWORD m_regGDPWCNodeBorder;

    CRegDWORDList m_regCTMarkers;

    // utilities

    CRegDWORD* GetRegistrySetting (Colors id);
    CRegDWORD* GetRegistrySetting (GDIPlusColor id);
    CRegDWORDList* GetRegistrySetting (GDIPlusColorTable id);
    CRegDWORD* GetLegacyRegistrySetting (GDIPlusColor id);
};