// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010, 2012 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "EditPropBase.h"


EditPropBase::EditPropBase()
    : m_bFolder(false)
    , m_bMultiple(false)
    , m_bIsBinary(false)
    , m_bChanged(false)
    , m_bRevProps(false)
    , m_bRecursive(false)
{
}

EditPropBase::~EditPropBase()
{
}

void EditPropBase::SetPropertyName(const std::string& sName)
{
    m_PropName = sName;
}

void EditPropBase::SetPropertyValue(const std::string& sValue)
{
    if (SVNProperties::IsBinary(sValue))
    {
        m_bIsBinary = true;
    }
    else
    {
        m_bIsBinary = false;
    }
    m_PropValue = sValue;
}


