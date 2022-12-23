// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014, 2020-2022 - TortoiseSVN
// Copyright (C) 2019 - TortoiseGit

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
#include "TempFile.h"
#include "DirFileEnum.h"
#include "SmartHandle.h"

CTempFiles::CTempFiles()
{
}

CTempFiles::~CTempFiles()
{
    m_tempFileList.DeleteAllPaths(false, false, nullptr);
}

CTempFiles& CTempFiles::Instance()
{
    static CTempFiles instance;
    return instance;
}

CTSVNPath CTempFiles::ConstructTempPath(const CTSVNPath& path, const SVNRev& revision) const
{
    DWORD len      = ::GetTempPath(0, nullptr);
    auto  tempPath = std::make_unique<wchar_t[]>(len + 1LL);
    auto  tempF    = std::make_unique<wchar_t[]>(len + 50LL);
    ::GetTempPath(len + 1, tempPath.get());
    CTSVNPath tempFile;
    CString   possibleTempFile;
    if (path.IsEmpty())
    {
        ::GetTempFileName(tempPath.get(), L"svn", 0, tempF.get());
        tempFile = CTSVNPath(tempF.get());
    }
    else
    {
        int i = 0;
        do
        {
            // use the UI path, which does unescaping for urls
            CString filename = path.GetUIFileOrDirectoryName();
            // remove illegal chars which could be present in a temp filename
            filename.Remove('?');
            filename.Remove('*');
            filename.Remove('<');
            filename.Remove('>');
            filename.Remove('|');
            filename.Remove('"');
            filename.Remove(':');
            filename.Remove('\'');
            // the inner loop assures that the resulting path is < MAX_PATH
            // if that's not possible without reducing the 'filename' to less than 5 chars, use a path
            // that's longer than MAX_PATH (in that case, we can't really do much to avoid longer paths)
            do
            {
                if (revision.IsValid())
                {
                    possibleTempFile.Format(L"%s%s-rev%s.svn%3.3x.tmp%s", tempPath.get(), static_cast<LPCWSTR>(filename), static_cast<LPCWSTR>(revision.ToString()), i, static_cast<LPCWSTR>(path.GetFileExtension()));
                }
                else
                {
                    possibleTempFile.Format(L"%s%s.svn%3.3x.tmp%s", tempPath.get(), static_cast<LPCWSTR>(filename), i, static_cast<LPCWSTR>(path.GetFileExtension()));
                }
                tempFile.SetFromWin(possibleTempFile);
                filename = filename.Left(filename.GetLength() - 1);
            } while ((filename.GetLength() > 4) && (tempFile.GetWinPathString().GetLength() >= MAX_PATH));
            i++;
            // now create the temp file in a thread safe way, so that subsequent calls to GetTempFile() return different filenames.
            CAutoFile hFile   = CreateFile(tempFile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, nullptr);
            auto      lastErr = GetLastError();
            if (hFile || ((lastErr != ERROR_FILE_EXISTS) && (lastErr != ERROR_ACCESS_DENIED)))
                break;
        } while (true);
    }

    // caller has to actually grab the file path

    return tempFile;
}

CTSVNPath CTempFiles::CreateTempPath(bool bRemoveAtEnd, const CTSVNPath& path, const SVNRev& revision, bool directory)
{
    bool succeeded = false;
    for (int retryCount = 0; retryCount < MAX_RETRIES; ++retryCount)
    {
        CTSVNPath tempFile = ConstructTempPath(path, revision);

        // now create the temp file / directory, so that subsequent calls to GetTempFile() return
        // different filenames.
        // Handle races, i.e. name collisions.

        if (directory)
        {
            DeleteFile(tempFile.GetWinPath());
            if (CreateDirectory(tempFile.GetWinPath(), nullptr) == FALSE)
            {
                auto lastErr = GetLastError();
                if ((lastErr != ERROR_ALREADY_EXISTS) && (lastErr != ERROR_ACCESS_DENIED))
                    return CTSVNPath();
            }
            else
                succeeded = true;
        }
        else
        {
            CAutoFile hFile = CreateFile(tempFile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
            if (!hFile)
            {
                auto lastErr = GetLastError();
                if ((lastErr != ERROR_ALREADY_EXISTS) && (lastErr != ERROR_ACCESS_DENIED))
                    return CTSVNPath();
            }
            else
            {
                succeeded = true;
            }
        }

        // done?

        if (succeeded)
        {
            if (bRemoveAtEnd)
                m_tempFileList.AddPath(tempFile);

            return tempFile;
        }
    }

    // give up

    return CTSVNPath();
}

CTSVNPath CTempFiles::GetTempFilePath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev& revision /* = SVNRev() */)
{
    return CreateTempPath(bRemoveAtEnd, path, revision, false);
}

CString CTempFiles::GetTempFilePathString(bool bRemoveAtEnd/* = true*/)
{
    return CreateTempPath(bRemoveAtEnd, CTSVNPath(), SVNRev(), false).GetWinPathString();
}

CTSVNPath CTempFiles::GetTempDirPath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev& revision /* = SVNRev() */)
{
    return CreateTempPath(bRemoveAtEnd, path, revision, true);
}

void CTempFiles::DeleteOldTempFiles(LPCWSTR wildCard)
{
    DWORD len  = ::GetTempPath(0, nullptr);
    auto  path = std::make_unique<wchar_t[]>(len + 100LL);
    len        = ::GetTempPath(len + 100, path.get());
    if (len == 0)
        return;

    CSimpleFileFind finder = CSimpleFileFind(path.get(), wildCard);
    FILETIME        sysFileTime;
    ::GetSystemTimeAsFileTime(&sysFileTime);
    __int64 sysTime = static_cast<long long>(sysFileTime.dwLowDateTime) | static_cast<long long>(sysFileTime.dwHighDateTime) << 32LL;
    while (finder.FindNextFileNoDirectories())
    {
        CString filepath = finder.GetFilePath();

        FILETIME createFileTime = finder.GetCreateTime();
        __int64  createTime     = static_cast<long long>(createFileTime.dwLowDateTime) | static_cast<long long>(createFileTime.dwHighDateTime) << 32LL;
        createTime += 864000000000LL; //only delete files older than a day
        if (createTime < sysTime)
        {
            ::SetFileAttributes(filepath, FILE_ATTRIBUTE_NORMAL);
            ::DeleteFile(filepath);
        }
    }
}
