// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2010, 2012, 2014, 2016, 2021 - TortoiseSVN

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
#include "PreserveChdir.h"

PreserveChdir::PreserveChdir()
    : m_size(GetCurrentDirectory(0, nullptr))
    , m_originalCurrentDirectory(new wchar_t[m_size])
{
    if (m_size > 0)
        if (GetCurrentDirectory(static_cast<DWORD>(m_size), m_originalCurrentDirectory.get()) != 0)
            return; // succeeded

    // GetCurrentDirectory failed
    m_originalCurrentDirectory.reset();
}

PreserveChdir::~PreserveChdir()
{
    if (!m_originalCurrentDirectory)
        return; // nothing to do

    // _tchdir is an expensive function - don't call it unless we really have to
    const DWORD len = GetCurrentDirectory(0, nullptr);
    if (len == m_size)
    {
        // same size, must check contents
        auto currentDirectory = std::make_unique<wchar_t[]>(len);
        if (GetCurrentDirectory(len, currentDirectory.get()) != 0)
            if (wcscmp(currentDirectory.get(), m_originalCurrentDirectory.get()) == 0)
                return; // no change required, reset of no use as dtor is called exactly once
    }

    // must reset directory
    if (SetCurrentDirectory(m_originalCurrentDirectory.get()))
        m_originalCurrentDirectory.reset();
}
