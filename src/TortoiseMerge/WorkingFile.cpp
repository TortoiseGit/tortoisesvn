#include "StdAfx.h"
#include "Workingfile.h"
#include "Utils.h"

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
	if(m_sDescriptiveName.IsEmpty())
	{
		// We don't have a proper name - use the filename part of the path
		// return the filename part of the path.
		return CUtils::GetFileNameFromPath(m_sFilename);
	}
	else
	{
		return m_sDescriptiveName;
	}
}
