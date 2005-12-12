#include "stdafx.h"
#include "shlobj.h"
#include "objbase.h"
#include "ShellFileOp.h"

CShellFileOp::CShellFileOp()
{
	ResetInternalData();
}

CShellFileOp::~CShellFileOp()
{
}

bool CShellFileOp::AddSourceFile(LPCTSTR szPath)
{
	try
	{
		m_SourceFileList.AddTail(szPath);
	}
	catch(CMemoryException *)
	{
		TRACE0("Memory exception in CShellFileOp::AddSourceFile()!\n");
		return false;
	}
	return true;
}

bool CShellFileOp::AddDestFile(LPCTSTR szPath)
{
	try
	{
		m_DestinationFileList.AddTail(szPath);
	}
	catch(CMemoryException *)
	{
		TRACE0("Memory exception in CShellFileOp::AddDestFile()!\n");
		return false;
	}
	return true;
}

void CShellFileOp::SetOperationFlags(UINT uOpType, HWND hWnd, FILEOP_FLAGS fFlags)
{
	// Validate the op type.  If this assert fires, check the operation
	// type param that you're passing in.
	ASSERT ( uOpType == FO_COPY  ||  uOpType == FO_DELETE  ||
		uOpType == FO_MOVE  ||  uOpType == FO_RENAME );

	m_FOS.wFunc = uOpType;
	m_FOS.hwnd = hWnd;
	m_FOS.fFlags = fFlags;

	m_bFlagsSet = true;
}

void CShellFileOp::SetProgressDlgTitle(UINT nTitleID)
{
	CString sTitle(MAKEINTRESOURCE(nTitleID));
	SetProgressDlgTitle((LPCTSTR)sTitle);
}

void CShellFileOp::SetProgressDlgTitle(LPCTSTR szTitle)
{
	m_sTitle = szTitle;
}

bool CShellFileOp::AnyOperationsAborted()
{
	return !!m_FOS.fAnyOperationsAborted;
}

void CShellFileOp::Reset()
{
	ResetInternalData();
}

bool CShellFileOp::Start(int *pnAPIReturn /*=NULL*/)
{
	TCHAR* szzSourceFiles = NULL;
	TCHAR* szzDestFiles = NULL;
	DWORD  dwSourceBufferSize = 0;
	DWORD  dwDestBufferSize = 0;
	int    nAPIRet = 0;

	if (!m_bFlagsSet)
	{
		// the SHFILEOPSTRUCT wasn't set
		return false;
	}

	if (m_SourceFileList.IsEmpty())
	{
		// nothing to do!
		return false;
	}

	if (m_FOS.wFunc != FO_DELETE  &&  m_DestinationFileList.IsEmpty())
	{
		// no destination given, but we need one!
		return false;
	}

	if (m_FOS.wFunc != FO_DELETE)
	{
		if (m_DestinationFileList.GetCount() != 1  &&
			m_DestinationFileList.GetCount() != m_SourceFileList.GetCount())
		{
			// wrong number of destination paths
			return false;
		}
	}
	// build the big double-null-terminated buffers

	dwSourceBufferSize = GetRequiredBufferSize(m_SourceFileList);

	if (m_FOS.wFunc != FO_DELETE)
	{
		dwDestBufferSize = GetRequiredBufferSize(m_DestinationFileList);
	}

	try
	{
		szzSourceFiles = (LPTSTR) new BYTE[dwSourceBufferSize];

		if (m_FOS.wFunc != FO_DELETE)
		{
			szzDestFiles = (LPTSTR) new BYTE[dwDestBufferSize];
		}
	}
	catch(CMemoryException *)
	{
		TRACE0("Memory exception in CShellFileOp::Start()!\n");
		if (szzSourceFiles)
			delete [] szzSourceFiles;
		return false;
	}

	FillSzzBuffer(szzSourceFiles, dwSourceBufferSize, m_SourceFileList);

	if (m_FOS.wFunc != FO_DELETE)
	{
		FillSzzBuffer(szzDestFiles, dwDestBufferSize, m_DestinationFileList);
	}

	m_FOS.pFrom = szzSourceFiles;
	m_FOS.pTo = szzDestFiles;
	m_FOS.lpszProgressTitle = (LPCTSTR)m_sTitle;


	// If there are 2 or more strings in
	// the destination list, set the
	// MULTIDESTFILES flag.
	if (m_DestinationFileList.GetCount() > 1)
	{
		m_FOS.fFlags |= FOF_MULTIDESTFILES;
	}

	nAPIRet = SHFileOperation( &m_FOS);

	if (NULL != pnAPIReturn)
	{
		*pnAPIReturn = nAPIRet;
	}

	// Free buffers.
	if (NULL != szzSourceFiles)
	{
		delete [] szzSourceFiles;
	}

	if (NULL != szzDestFiles)
	{
		delete [] szzDestFiles;
	}

	return 0 == nAPIRet;
}


void CShellFileOp::ResetInternalData()
{
	m_SourceFileList.RemoveAll();
	m_DestinationFileList.RemoveAll();

	m_bFlagsSet = false;

	// And clear out other stuff...
	m_sTitle.Empty();

	ZeroMemory(&m_FOS, sizeof(m_FOS));
}

DWORD CShellFileOp::GetRequiredBufferSize(const CStringList& list)
{
	DWORD    dwRetVal = 0;
	POSITION pos;
	CString  cstr;

	pos = list.GetHeadPosition();

	while (NULL != pos)
	{
		cstr = list.GetNext(pos);

		dwRetVal += sizeof(TCHAR)*(cstr.GetLength() + 1);
	}

	return dwRetVal + sizeof(TCHAR);    // add one more for the \0 at the end
}

void CShellFileOp::FillSzzBuffer(TCHAR* pBuffer, size_t buflen, const CStringList& list)
{
	TCHAR*   pCurrPos;
	POSITION pos;
	CString  cstr;
	pCurrPos = pBuffer;

	pos = list.GetHeadPosition();

	while (NULL != pos)
	{
		cstr = list.GetNext(pos);

		_tcscpy_s(pCurrPos, (buflen/sizeof(TCHAR)) - (pCurrPos - pBuffer), (LPCTSTR)cstr);
		pCurrPos = _tcsinc(_tcschr(pCurrPos, '\0' ));;
	}

	*pCurrPos = '\0';
}
