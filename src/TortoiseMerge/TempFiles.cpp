#include "StdAfx.h"
#include ".\tempfiles.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
	for (int i=0; i<m_arTempFileList.GetCount(); i++)
	{
		DeleteFile(m_arTempFileList.GetAt(i));
	}
}

CString CTempFiles::GetTempFilePath()
{
	TCHAR path[MAX_PATH];
	TCHAR tempF[MAX_PATH];
	DWORD len = ::GetTempPath (MAX_PATH, path);
	UINT unique = ::GetTempFileName (path, TEXT("tsm"), 0, tempF);
	CString tempfile = CString(tempF);
	m_arTempFileList.Add(tempfile);
	return tempfile;
}
