// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2010, 2012 - TortoiseSVN

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

PreserveChdir::PreserveChdir() :
    size(GetCurrentDirectory(0, NULL)),
    originalCurrentDirectory(new TCHAR[size])
{
    if (size > 0)
        if (GetCurrentDirectory((DWORD)size, originalCurrentDirectory.get()) !=0)
            return; // succeeded

    // GetCurrentDirectory failed
    originalCurrentDirectory.reset();
}

PreserveChdir::~PreserveChdir()
{
    if (!originalCurrentDirectory)
        return; // nothing to do

    // _tchdir is an expensive function - don't call it unless we really have to
    const DWORD len = GetCurrentDirectory(0, NULL);
    if(len == size) {
        // same size, must check contents
        std::unique_ptr<TCHAR[]> currentDirectory(new TCHAR[len]);
        if(GetCurrentDirectory(len, currentDirectory.get()) != 0)
            if(_tcscmp(currentDirectory.get(), originalCurrentDirectory.get()) == 0)
                return; // no change required, reset of no use as dtor is called exactly once
    }

    // must reset directory
    if(SetCurrentDirectory(originalCurrentDirectory.get()))
        originalCurrentDirectory.reset();
}
