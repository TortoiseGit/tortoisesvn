// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "StdAfx.h"
#include "Workingfile.h"
#include "Utils.h"
#include "resource.h"

CWorkingFile::CWorkingFile(void)
{
}

CWorkingFile::~CWorkingFile(void)
{
}

void 
CWorkingFile::SetFileName(const CString& newFilename)
{
	m_sFilename = newFilename;
	m_sFilename.Replace('/', '\\');
	m_sDescriptiveName.Empty();
}

void 
CWorkingFile::SetDescriptiveName(const CString& newDescName)
{
	m_sDescriptiveName = newDescName;
}

//
// Make an empty file with this name
void 
CWorkingFile::CreateEmptyFile()
{
	HANDLE hFile = CreateFile(m_sFilename, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CloseHandle(hFile);
}

// Move the details of the specified file to the current one, and then mark the specified file
// as out of use
void 
CWorkingFile::TransferDetailsFrom(CWorkingFile& rightHandFile)
{
	// We don't do this to files which are already in use
	ASSERT(!InUse());

	m_sFilename = rightHandFile.m_sFilename;
	m_sDescriptiveName = rightHandFile.m_sDescriptiveName;
	rightHandFile.SetOutOfUse();
}

CString 
CWorkingFile::GetWindowName() const
{
	CString sErrMsg = _T("");
	// TortoiseMerge allows non-existing files to be used in a merge
	// Inform the user (in a non-intrusive way) if a file is absent
	if (! this->Exists())
	{
		sErrMsg = CString(MAKEINTRESOURCE(IDS_NOTFOUNDVIEWTITLEINDICATOR));
	}

	if(m_sDescriptiveName.IsEmpty())
	{
		// We don't have a proper name - use the filename part of the path
		// return the filename part of the path.
		return CUtils::GetFileNameFromPath(m_sFilename) + _T(" ") + sErrMsg;
	}
	else if (sErrMsg.IsEmpty())
	{
		return m_sDescriptiveName;
	}
	return m_sDescriptiveName + _T(" ") + sErrMsg;
}

bool
CWorkingFile::Exists() const
{
	return (!!PathFileExists(m_sFilename));
}
