// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2012 - TortoiseSVN

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
#include <map>
#include <tuple>

enum UserPropType
{
    UserPropTypeUnknown,
    UserPropTypeBool,           // checkbox
    UserPropTypeState,          // combo list
    UserPropTypeSingleLine,
    UserPropTypeMultiLine,
};

class UserProp
{
public:
    UserProp(bool bFile)
        : propType(UserPropTypeUnknown)
        , file(bFile)
        , m_menuID(0)
    {
    }

    bool Parse(const CString& line);
    void SetMenuID(int menuID) {m_menuID = menuID;}
    int  GetMenuID() const {return m_menuID;}
public:
    CString         propName;
    UserPropType    propType;
    bool            file;                           // if true, property is used on files. if false, property is for directories only
    CString         labelText;
    CString         validationRegex;
    CString         boolYes;
    CString         boolNo;
    CString         boolCheckText;
    CString         stateDefaultVal;
    std::vector<std::tuple<CString, CString>>  stateEntries;
private:
    int             m_menuID;
};

