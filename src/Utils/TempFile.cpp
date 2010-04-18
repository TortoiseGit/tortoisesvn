// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "TempFile.h"
#include "auto_buffer.h"
#include "SVNError.h"
#include "PathUtils.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
    m_TempFileList.DeleteAllPaths(false, false);
}

CTempFiles& CTempFiles::Instance()
{
    static CTempFiles instance;
    return instance;
}

void CTempFiles::CheckLastError()
{
    DWORD lastError = GetLastError();

    if (lastError != ERROR_ALREADY_EXISTS)
    {
        // no simple name collision -> bail out

        SVNError::ThrowLastError (lastError);
    }
}

CTSVNPath CTempFiles::ConstructTempPath(const CTSVNPath& path, const SVNRev revision)
{
    DWORD len = ::GetTempPath(0, NULL);
    auto_buffer<TCHAR> temppath (len+1);
    auto_buffer<TCHAR> tempF (len+50);
    ::GetTempPath (len+1, temppath);
    CTSVNPath tempfile;
    CString possibletempfile;
    if (path.IsEmpty())
    {
        ::GetTempFileName (temppath, TEXT("svn"), 0, tempF);
        tempfile = CTSVNPath (tempF.get());
    }
    else
    {
        int i=0;
        do
        {
            // use the UI path, which does unescaping for urls
            CString filename = path.GetUIFileOrDirectoryName();

            // the inner loop assures that the resulting path is < MAX_PATH
            // if that's not possible without reducing the 'filename' to less than 5 chars, use a path
            // that's longer than MAX_PATH (in that case, we can't really do much to avoid longer paths)
            do 
            {
                if (revision.IsValid())
                {
                    possibletempfile.Format(_T("%s%s-rev%s.svn%3.3x.tmp%s"), temppath, (LPCTSTR)filename, (LPCTSTR)revision.ToString(), i, (LPCTSTR)path.GetFileExtension());
                }
                else
                {
                    possibletempfile.Format(_T("%s%s.svn%3.3x.tmp%s"), temppath, (LPCTSTR)filename, i, (LPCTSTR)path.GetFileExtension());
                }
                tempfile.SetFromWin(possibletempfile);
                filename = filename.Left(filename.GetLength()-1);
            } while (   (filename.GetLength() > 4) 
                     && (revision.IsValid() || tempfile.GetWinPathString().GetLength() >= MAX_PATH));
            i++;
        } while (PathFileExists(tempfile.GetWinPath()));
    }

    // caller has to actually grab the file path

    return tempfile;
}

CTSVNPath CTempFiles::CreateTempPath (bool bRemoveAtEnd, const CTSVNPath& path, const SVNRev revision, bool directory)
{
    bool succeeded = false;
    for (int retryCount = 0; retryCount < MAX_RETRIES; ++retryCount)
    {
        CTSVNPath tempfile = ConstructTempPath (path, revision);

        // now create the temp file / directory, so that subsequent calls to GetTempFile() return
        // different filenames. 
        // Handle races, i.e. name collisions.

        if (directory)
        {
            if (CreateDirectory (tempfile.GetWinPath(), NULL) == FALSE)
                CheckLastError();
            else
                succeeded = true;
        }
        else
        {
            HANDLE hFile = CreateFile(tempfile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
            if (hFile== INVALID_HANDLE_VALUE)
            {
                CheckLastError();
            }
            else
            {
                CloseHandle(hFile);
                succeeded = true;           
            }
        }

        // done?

        if (succeeded)
        {
            if (bRemoveAtEnd)
                m_TempFileList.AddPath(tempfile);

            return tempfile;
        }
    }

    // give up

    SVNError::ThrowLastError();

    // make compiler happy

    return CTSVNPath();
}

CTSVNPath CTempFiles::GetTempFilePath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev revision /* = SVNRev() */)
{
    return CreateTempPath (bRemoveAtEnd, path, revision, false);
}

CTSVNPath CTempFiles::GetTempDirPath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev revision /* = SVNRev() */)
{
    return CreateTempPath (bRemoveAtEnd, path, revision, true);
}