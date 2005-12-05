#pragma once

#include "objbase.h"

/**
 * \ingroup Utils
 * 
 * A wrapper for SHFileOperation.
 * Helper methods are provided to handle the multiple source
 * and destination buffers.
 *
 */
class CShellFileOp  
{
public:
	CShellFileOp();
	~CShellFileOp();

public:
	bool AddSourceFile(LPCTSTR szPath);
	bool AddDestFile(LPCTSTR szPath);

	void SetOperationFlags(UINT uOpType, HWND hWnd, FILEOP_FLAGS fFlags);

	/// sets the title for the dialog shown during the operation.
	void SetProgressDlgTitle(LPCTSTR szTitle);
	void SetProgressDlgTitle(UINT nTitleID);

	/// starts the operation specified in SetOperationFlags()
	bool Start(int *pnAPIReturn = NULL);

	/// returns true if the operation was aborted.
	bool AnyOperationsAborted();

	/// Clears the source and destination buffers.
	void Reset();
	const CStringList& GetSourceFileList() const {return m_SourceFileList;}
	const CStringList& GetDestFileList() const {return m_DestinationFileList;}

protected:
	bool        m_bFlagsSet;

	CStringList m_SourceFileList;
	CStringList m_DestinationFileList;

	SHFILEOPSTRUCT m_FOS;

	CString     m_sTitle;

protected:
	void  ResetInternalData();
	DWORD GetRequiredBufferSize(const CStringList& list);
	void  FillSzzBuffer(TCHAR*, size_t buflen, const CStringList& list);
};

