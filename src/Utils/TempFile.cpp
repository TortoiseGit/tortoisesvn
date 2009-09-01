// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "Registry.h"
#include "TempFile.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
	m_TempFileList.DeleteAllFiles(false);
}

CTempFiles& CTempFiles::Instance()
{
	static CTempFiles instance;
	return instance;
}

CTSVNPath CTempFiles::GetTempFilePath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev revision /* = SVNRev() */)
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
		// use the UI path, which does unescaping for urls
		CString filename = path.GetUIFileOrDirectoryName();
		do
		{
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
			} while ((filename.GetLength() > 4) && (tempfile.GetWinPathString().GetLength() >= MAX_PATH));
			i++;
		} while (PathFileExists(tempfile.GetWinPath()));
	}
	//now create the temp file, so that subsequent calls to GetTempFile() return
	//different filenames.
	HANDLE hFile = CreateFile(tempfile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	CloseHandle(hFile);
	if (bRemoveAtEnd)
		m_TempFileList.AddPath(tempfile);
	return tempfile;
}

