// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2005 - Stefan Kueng

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
#include "FileTextLines.h"


#define PATCHSTATE_REMOVED	0
#define PATCHSTATE_ADDED	1
#define PATCHSTATE_CONTEXT	2

/**
 * \ingroup TortoiseMerge
 *
 * Handles unified diff files, parses them and also is able to
 * apply those diff files.
 *
 * \todo enhance the parser to work with other diff files than
 * those created by subversion clients.
 * \todo rewrite the parser to use regular expressions
 */
class CPatch
{
public:
	CPatch(void);
	~CPatch(void);

	BOOL		OpenUnifiedDiffFile(const CString& filename);
	BOOL		PatchFile(const CString& sPath, const CString& sSavePath = _T(""), const CString& sBaseFile = _T(""));
	int			GetNumberOfFiles() const  {return m_arFileDiffs.GetCount();}
	CString		GetFilename(int nIndex);
	CString		GetRevision(int nIndex);
	CString		GetFilename2(int nIndex);
	CString		GetRevision2(int nIndex);
	CString		GetErrorMessage() const  {return m_sErrorMessage;}
	CString		CheckPatchPath(const CString& path);
protected:
	void		FreeMemory();
	BOOL		HasExpandedKeyWords(const CString& line);
	int			CountMatches(const CString& path);
	int			CountDirMatches(const CString& path);
	struct Chunk
	{
		LONG					lRemoveStart;
		LONG					lRemoveLength;
		LONG					lAddStart;
		LONG					lAddLength;
		CStdCStringArray		arLines;
		CStdDWORDArray			arLinesStates;
	};

	struct Chunks
	{
		CString					sFilePath;
		CString					sRevision;
		CString					sFilePath2;
		CString					sRevision2;
		CStdArray<Chunk*>		chunks;
	};

	CStdArray<Chunks*>			m_arFileDiffs;
	CString						m_sErrorMessage;
	CFileTextLines::UnicodeType m_UnicodeType;
};
