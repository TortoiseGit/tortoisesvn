#pragma once

#define PATCHSTATE_REMOVED	0
#define PATCHSTATE_ADDED	1
#define PATCHSTATE_CONTEXT	2

class CPatch
{
public:
	CPatch(void);
	~CPatch(void);

	BOOL		OpenUnifiedDiffFile(CString filename);
	BOOL		PatchFile(CString sPath, CString sSavePath = _T(""), CString sBaseFile = _T(""));
	int			GetNumberOfFiles() {return (int)m_arFileDiffs.GetCount();}
	CString		GetFilename(int nIndex);
	CString		GetRevision(int nIndex);
	CString		GetErrorMessage() {return m_sErrorMessage;}

protected:
	void		FreeMemory();

	struct Chunk
	{
		LONG					lRemoveStart;
		LONG					lRemoveLength;
		LONG					lAddStart;
		LONG					lAddLength;
		CStringArray			arLines;
		CDWordArray				arLinesStates;
	};

	struct Chunks
	{
		CString					sFilePath;
		CString					sRevision;
		CArray<Chunk*, Chunk*>	chunks;
	};

	CArray<Chunks*, Chunks*>	m_arFileDiffs;
	CString						m_sErrorMessage;
};
