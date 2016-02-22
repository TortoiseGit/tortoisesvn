// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseSVN

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
#include "TestTempFile.h"

CTestTempFile::CTestTempFile()
{
    WCHAR tempPath[MAX_PATH + 1] = { 0 };
    WCHAR tempFile[MAX_PATH + 1] = { 0 };

    ::GetTempPath(_countof(tempPath), tempPath);
    ::GetTempFileName(tempPath, L"tsvn", 0, tempFile);

    m_TempFileName = tempFile;
}

CTestTempFile::~CTestTempFile()
{
    // Remove read-only attribute
    ::SetFileAttributes(m_TempFileName.c_str(), FILE_ATTRIBUTE_NORMAL);
    ::DeleteFile(m_TempFileName.c_str());
}

const std::wstring & CTestTempFile::GetFileName() const
{
    return m_TempFileName;
}
