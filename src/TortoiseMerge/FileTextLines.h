#pragma once
#include "afxcoll.h"

/**
 * \ingroup TortoiseMerge
 *
 * Represents an array of text lines which are read from a file.
 */
class CFileTextLines : public CStringArray
{
public:
	CFileTextLines(void);
	~CFileTextLines(void);

	enum LineEndings
	{
		AUTOLINE,
		LF,
		CRLF,
		LFCR,
		CR,
	};

	/**
	 * Loads the text file and adds each line to the array
	 * \param sFilePath the path to the file
	 */
	BOOL		Load(CString sFilePath);
	/**
	 * Saves the whole array of text lines to a file, preserving
	 * the line endings detected at Load()
	 * \param sFilePath the path to save the file to
	 */
	BOOL		Save(CString sFilePath);
	/**
	 * Returns an error string of the last failed operation
	 */
	CString		GetErrorString() {return m_sErrorString;}
	/**
	 * Copies the settings of a file like the line ending styles
	 * to another CFileTextLines object.
	 */
	void		CopySettings(CFileTextLines * pFileToCopySettingsTo);


	BOOL		m_bUnicode;
	CFileTextLines::LineEndings m_LineEndings;
protected:
	/**
	 * Checks if a buffer is an UNICODE text 
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	BOOL		IsTextUnicode(LPVOID pBuffer, int cb);
	/**
	 * Checks the line endings in a text buffer
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	CFileTextLines::LineEndings CheckLineEndings(LPVOID pBuffer, int cb); 

	void		GetLastError();

	CString		m_sErrorString;

};
