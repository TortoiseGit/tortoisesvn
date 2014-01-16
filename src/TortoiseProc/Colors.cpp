// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010, 2013-2014 - TortoiseSVN

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
#include "Colors.h"

using namespace Gdiplus;

static DWORD SysColorAsColor (int index)
{
    Color color;
    color.SetFromCOLORREF (::GetSysColor (index));
    return color.GetValue();
}

CColors::CColors(void) : m_regAdded(L"Software\\TortoiseSVN\\Colors\\Added", RGB(100, 0, 100))
    , m_regCmd(L"Software\\TortoiseSVN\\Colors\\Cmd", ::GetSysColor(COLOR_GRAYTEXT))
    , m_regConflict(L"Software\\TortoiseSVN\\Colors\\Conflict", RGB(255, 0, 0))
    , m_regModified(L"Software\\TortoiseSVN\\Colors\\Modified", RGB(0, 50, 160))
    , m_regMerged(L"Software\\TortoiseSVN\\Colors\\Merged", RGB(0, 100, 0))
    , m_regDeleted(L"Software\\TortoiseSVN\\Colors\\Deleted", RGB(100, 0, 0))
    , m_regLastCommit(L"Software\\TortoiseSVN\\Colors\\LastCommit", RGB(100, 100, 100))
    , m_regDeletedNode(L"Software\\TortoiseSVN\\Colors\\DeletedNode", RGB(255, 0, 0))
    , m_regAddedNode(L"Software\\TortoiseSVN\\Colors\\AddedNode", RGB(0, 255, 0))
    , m_regReplacedNode(L"Software\\TortoiseSVN\\Colors\\ReplacedNode", RGB(0, 255, 0))
    , m_regRenamedNode(L"Software\\TortoiseSVN\\Colors\\RenamedNode", RGB(0, 0, 255))
    , m_regLastCommitNode(L"Software\\TortoiseSVN\\Colors\\LastCommitNode", RGB(200, 200, 200))
    , m_regPropertyChanged(L"Software\\TortoiseSVN\\Colors\\PropertyChanged", RGB(0, 50, 160))
    , m_regFilterMatch(L"Software\\TortoiseSVN\\Colors\\FilterMatch", RGB(200, 0, 0))

    , m_regGDPDeletedNode (L"Software\\TortoiseSVN\\Colors\\GDI+DeletedNode", (DWORD)Color::Red)
    , m_regGDPAddedNode (L"Software\\TortoiseSVN\\Colors\\GDI+AddedNode", (DWORD)0xff00ff00)
    , m_regGDPRenamedNode (L"Software\\TortoiseSVN\\Colors\\GDI+RenamedNode", (DWORD)Color::Blue)
    , m_regGDPLastCommit (L"Software\\TortoiseSVN\\Colors\\GDI+LastCommitNode", SysColorAsColor(COLOR_WINDOW))

    , m_regGDPModifiedNode (L"Software\\TortoiseSVN\\Colors\\GDI+ModifiedNode", SysColorAsColor(COLOR_WINDOWTEXT))
    , m_regGDPWCNode (L"Software\\TortoiseSVN\\Colors\\GDI+WCNode", SysColorAsColor(COLOR_WINDOW))
    , m_regGDPUnchangedNode (L"Software\\TortoiseSVN\\Colors\\GDI+UnchangedNode", SysColorAsColor(COLOR_WINDOW))
    , m_regGDPTagOverlay (L"Software\\TortoiseSVN\\Colors\\GDI+TagOverlay", 0x80fafa60)
    , m_regGDPTrunkOverlay (L"Software\\TortoiseSVN\\Colors\\GDI+TrunkOverlay", 0x4040FF40)

    , m_regGDPStripeColor1 (L"Software\\TortoiseSVN\\Colors\\GDI+Stripe1", 0x18F0F0C0)
    , m_regGDPStripeColor2 (L"Software\\TortoiseSVN\\Colors\\GDI+Stripe2", 0x18A0D0E0)

    , m_regGDPWCNodeBorder (L"Software\\TortoiseSVN\\Colors\\GDI+WCBorder", 0xFFD00000)

    , m_regCTMarkers (L"Software\\TortoiseSVN\\Colors\\MarkersTable", 0)
{
    m_regCTMarkers.GetDefaults()[0] = Color::MakeARGB (255, 250, 250, 92);
    m_regCTMarkers.GetDefaults()[1] = Color::MakeARGB (255, 160, 92, 250);
}

CColors::~CColors(void)
{
}

CRegDWORD* CColors::GetRegistrySetting (Colors id)
{
    switch (id)
    {
    case Cmd:               return &m_regCmd;
    case Conflict:          return &m_regConflict;
    case Modified:          return &m_regModified;
    case Merged:            return &m_regMerged;
    case Deleted:           return &m_regDeleted;
    case Added:             return &m_regAdded;
    case LastCommit:        return &m_regAdded;
    case DeletedNode:       return &m_regDeletedNode;
    case AddedNode:         return &m_regAddedNode;
    case ReplacedNode:      return &m_regReplacedNode;
    case RenamedNode:       return &m_regRenamedNode;
    case LastCommitNode:    return &m_regLastCommitNode;
    case PropertyChanged:   return &m_regPropertyChanged;
    case FilterMatch:       return &m_regFilterMatch;
    }

    return NULL;
}

CRegDWORD* CColors::GetRegistrySetting (GDIPlusColor id)
{
    switch (id)
    {
    case gdpDeletedNode:    return &m_regGDPDeletedNode;
    case gdpAddedNode:      return &m_regGDPAddedNode;
    case gdpRenamedNode:    return &m_regGDPRenamedNode;
    case gdpLastCommitNode: return &m_regGDPLastCommit;
    case gdpModifiedNode:   return &m_regGDPModifiedNode;
    case gdpWCNode:         return &m_regGDPWCNode;
    case gdpUnchangedNode:  return &m_regGDPUnchangedNode;
    case gdpTagOverlay:     return &m_regGDPTagOverlay;
    case gdpTrunkOverlay:   return &m_regGDPTrunkOverlay;
    case gdpStripeColor1:   return &m_regGDPStripeColor1;
    case gdpStripeColor2:   return &m_regGDPStripeColor2;
    case gdpWCNodeBorder:   return &m_regGDPWCNodeBorder;
    }

    return NULL;
}

CRegDWORDList* CColors::GetRegistrySetting (GDIPlusColorTable id)
{
    switch (id)
    {
    case ctMarkers:         return &m_regCTMarkers;
    }

    return NULL;
}

CRegDWORD* CColors::GetLegacyRegistrySetting (GDIPlusColor id)
{
    switch (id)
    {
    case gdpDeletedNode:    return &m_regDeletedNode;
    case gdpAddedNode:      return &m_regAddedNode;
    case gdpRenamedNode:    return &m_regRenamedNode;
    case gdpLastCommitNode: return &m_regLastCommit;
    }

    return NULL;
}

COLORREF CColors::GetColor (Colors col, bool bDefault)
{
    switch (col)
    {
    case DryRunConflict:
        {
            COLORREF c = GetColor(Conflict);
            static const int scale = 150;
            long red   = MulDiv(GetRValue(c),(255-scale),255);
            long green = MulDiv(GetGValue(c),(255-scale),255);
            long blue  = MulDiv(GetBValue(c),(255-scale),255);

            return RGB(red, green, blue);
        }
        break;
    default:
        {
            CRegDWORD* setting = GetRegistrySetting (col);
            if (setting == NULL)
                return RGB (0,0,0);

            return bDefault ? setting->defaultValue() : (DWORD)*setting;
        }
    }
}

void CColors::SetColor(Colors col, COLORREF cr)
{
    CRegDWORD* setting = GetRegistrySetting (col);
    assert (setting != NULL);
    if (setting == nullptr)
        return;
    *setting = cr;
}

Color CColors::GetColor (GDIPlusColor id, bool bDefault)
{
    CRegDWORD* setting = GetRegistrySetting (id);
    if (setting == NULL)
        return Color();

    if (bDefault)
        return setting->defaultValue();

    if (setting->exists())
        return (DWORD)*setting;

    CRegDWORD* lecagySetting = GetLegacyRegistrySetting (id);
    if ((lecagySetting != NULL) && lecagySetting->exists())
    {
        Color result;
        result.SetFromCOLORREF (*lecagySetting);
        return result;
    }

    return setting->defaultValue();
}

void CColors::SetColor (GDIPlusColor id, Color color)
{
    CRegDWORD* setting = GetRegistrySetting (id);
    if (setting == NULL)
        return;

    *setting = color.GetValue();

    // remove legacy info

    CRegDWORD* lecagySetting = GetLegacyRegistrySetting (id);
    if ((lecagySetting != NULL) && lecagySetting->exists())
        lecagySetting->removeKey();
}

Gdiplus::Color CColors::GetColor (GDIPlusColorTable id, int index, bool bDefault)
{
    CRegDWORDList* list = GetRegistrySetting (id);
    if (list == NULL)
        return Color();

    CRegDWORD& setting = (*list)[index];
    return bDefault
        ? setting.defaultValue()
        : (DWORD)setting;
}

void CColors::SetColor (GDIPlusColorTable id, int index, Gdiplus::Color color)
{
    CRegDWORDList* setting = GetRegistrySetting (id);
    if (setting == NULL)
        return;

    (*setting)[index] = color.GetValue();
}
