// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007, 2009-2010, 2014, 2021 - TortoiseSVN

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
#include "ViewData.h"

CViewData::CViewData()
    : m_nMarkedBlocks(0)
{
}

CViewData::~CViewData()
{
}

void CViewData::AddData(const CString& sLine, DiffStates state, int linenumber, EOL ending, HideState hide, int movedIndex)
{
    ViewData data;
    data.sLine      = sLine;
    data.state      = state;
    data.lineNumber = linenumber;
    data.ending     = ending;
    data.hideState  = hide;
    data.movedIndex = movedIndex;
    return AddData(data);
}

void CViewData::AddData(const ViewData& data)
{
    return m_data.push_back(data);
}

void CViewData::InsertData(int index, const CString& sLine, DiffStates state, int linenumber, EOL ending, HideState hide, int movedIndex)
{
    ViewData data;
    data.sLine      = sLine;
    data.state      = state;
    data.lineNumber = linenumber;
    data.ending     = ending;
    data.hideState  = hide;
    data.movedIndex = movedIndex;
    return InsertData(index, data);
}

void CViewData::InsertData(int index, const ViewData& data)
{
    m_data.insert(m_data.begin() + index, data);
}

int CViewData::FindLineNumber(int number) const
{
    for (size_t i = 0; i < m_data.size(); ++i)
        if (m_data[i].lineNumber >= number)
            return static_cast<int>(i);
    return -1;
}
