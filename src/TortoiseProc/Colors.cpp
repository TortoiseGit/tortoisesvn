// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include ".\colors.h"

using namespace Gdiplus;

CColors::CColors(void) : m_regAdded(_T("Software\\TortoiseSVN\\Colors\\Added"), RGB(100, 0, 100))
	, m_regCmd(_T("Software\\TortoiseSVN\\Colors\\Cmd"), ::GetSysColor(COLOR_GRAYTEXT))
	, m_regConflict(_T("Software\\TortoiseSVN\\Colors\\Conflict"), RGB(255, 0, 0))
	, m_regModified(_T("Software\\TortoiseSVN\\Colors\\Modified"), RGB(0, 50, 160))
	, m_regMerged(_T("Software\\TortoiseSVN\\Colors\\Merged"), RGB(0, 100, 0))
	, m_regDeleted(_T("Software\\TortoiseSVN\\Colors\\Deleted"), RGB(100, 0, 0))
	, m_regLastCommit(_T("Software\\TortoiseSVN\\Colors\\LastCommit"), RGB(100, 100, 100))
	, m_regDeletedNode(_T("Software\\TortoiseSVN\\Colors\\DeletedNode"), RGB(255, 0, 0))
	, m_regAddedNode(_T("Software\\TortoiseSVN\\Colors\\AddedNode"), RGB(0, 255, 0))
	, m_regReplacedNode(_T("Software\\TortoiseSVN\\Colors\\ReplacedNode"), RGB(0, 255, 0))
	, m_regRenamedNode(_T("Software\\TortoiseSVN\\Colors\\RenamedNode"), RGB(0, 0, 255))
	, m_regLastCommitNode(_T("Software\\TortoiseSVN\\Colors\\LastCommitNode"), RGB(200, 200, 200))
	, m_regPropertyChanged(_T("Software\\TortoiseSVN\\Colors\\PropertyChanged"), RGB(0, 50, 160))

    , m_regGDPDeletedNode (_T("Software\\TortoiseSVN\\Colors\\GDI+DeletedNode"), (DWORD)Color::Red)
    , m_regGDPAddedNode (_T("Software\\TortoiseSVN\\Colors\\GDI+AddedNode"), (DWORD)0xff00ff00)
    , m_regGDPRenamedNode (_T("Software\\TortoiseSVN\\Colors\\GDI+RenamedNode"), (DWORD)Color::Blue)
	, m_regGDPLastCommit (_T("Software\\TortoiseSVN\\Colors\\GDI+LastCommitNode"), ::GetSysColor(COLOR_WINDOW) + Color::Black)

	, m_regGDPModifiedNode (_T("Software\\TortoiseSVN\\Colors\\GDI+ModifiedNode"), ::GetSysColor(COLOR_WINDOWTEXT) + Color::Black)
	, m_regGDPWCNode (_T("Software\\TortoiseSVN\\Colors\\GDI+WCNode"), ::GetSysColor(COLOR_WINDOW) + Color::Black)
	, m_regGDPUnchangedNode (_T("Software\\TortoiseSVN\\Colors\\GDI+UnchangedNode"), ::GetSysColor(COLOR_WINDOW) + Color::Black)
	, m_regGDPTagOverlay (_T("Software\\TortoiseSVN\\Colors\\GDI+TagOverlay"), 0x80fafa60)
	, m_regGDPTrunkOverlay (_T("Software\\TortoiseSVN\\Colors\\GDI+TrunkOverlay"), 0x4040FF40)

	, m_regGDPStripeColor1 (_T("Software\\TortoiseSVN\\Colors\\GDI+Stripe1"), 0x18F0F0C0)
	, m_regGDPStripeColor2 (_T("Software\\TortoiseSVN\\Colors\\GDI+Stripe2"), 0x18A0D0E0)
{
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
	case Modified:	        return &m_regModified;
	case Merged:	        return &m_regMerged;
	case Deleted:	        return &m_regDeleted;
	case Added:		        return &m_regAdded;
	case LastCommit:	    return &m_regAdded;
	case DeletedNode:	    return &m_regDeletedNode;
	case AddedNode: 	    return &m_regAddedNode;
	case ReplacedNode:	    return &m_regReplacedNode;
	case RenamedNode:   	return &m_regRenamedNode;
	case LastCommitNode:    return &m_regLastCommitNode;
	case PropertyChanged:	return &m_regPropertyChanged;
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
    CRegDWORD* setting = GetRegistrySetting (col);
    if (setting == NULL)
        return RGB (0,0,0);

    return bDefault ? setting->defaultValue() : (DWORD)*setting;
}

void CColors::SetColor(Colors col, COLORREF cr)
{
    CRegDWORD* setting = GetRegistrySetting (col);
    assert (setting != NULL);

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
        return (DWORD)*lecagySetting + Color::Black;

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
    if (lecagySetting->exists())
        lecagySetting->removeKey();
}
