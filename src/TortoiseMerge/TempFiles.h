#pragma once

/**
 * \ingroup TortoiseMerge
 *
 * Keeps track of all used temporary files and destroys (deletes)
 * them when this object is destroyed.
 */
class CTempFiles
{
public:
	CTempFiles(void);
	~CTempFiles(void);
	CString GetTempFilePath();
protected:
	CStringArray		m_arTempFileList;
};
