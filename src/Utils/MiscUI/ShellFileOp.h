// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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

