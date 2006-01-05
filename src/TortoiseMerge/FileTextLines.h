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
#pragma once

// A template class to make an array which looks like a CStringArray or CDWORDArray but
// is infact based on a STL array, which is much faster at large sizes
template <typename T> class CStdArray
{
public:
	int GetCount() const { return (int)m_vec.size(); }
	const T& GetAt(int index) const { return m_vec[index]; }
	void RemoveAt(int index)	{ m_vec.erase(m_vec.begin()+index); }
	void InsertAt(int index, const T& strVal)	{ m_vec.insert(m_vec.begin()+index, strVal); }
	void InsertAt(int index, const T& strVal, int nCopies)	{ m_vec.insert(m_vec.begin()+index, nCopies, strVal); }
	void SetAt(int index, const T& strVal)	{ m_vec[index] = strVal; }
	void Add(const T& strVal)	 { m_vec.push_back(strVal); }
	void RemoveAll()				{ m_vec.clear(); }
	void Reserve(int lengthHint)	{ m_vec.reserve(lengthHint); }

private:
	std::vector<T> m_vec;
};

typedef CStdArray<CString> CStdCStringArray;
typedef CStdArray<DWORD> CStdDWORDArray;


/**
 * \ingroup TortoiseMerge
 *
 * Represents an array of text lines which are read from a file.
 * This class is also responsible for determing the encoding of
 * the file (e.g. UNICODE, UTF8, ASCII, ...).
 */
class CFileTextLines  : public CStdCStringArray
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
	enum UnicodeType
	{
		AUTOTYPE,
		BINARY,
		ASCII,
		UNICODE_LE,
		UTF8,
		UTF8BOM,
	};

	/**
	 * Loads the text file and adds each line to the array
	 * \param sFilePath the path to the file
	 */
	BOOL		Load(const CString& sFilePath, int lengthHint = 0);
	/**
	 * Saves the whole array of text lines to a file, preserving
	 * the line endings detected at Load()
	 * \param sFilePath the path to save the file to
	 */
	BOOL		Save(const CString& sFilePath, BOOL bIgnoreWhitespaces = FALSE, BOOL bIgnoreLineendings = FALSE, BOOL bIgnoreCase = FALSE);
	/**
	 * Returns an error string of the last failed operation
	 */
	CString		GetErrorString() const  {return m_sErrorString;}
	/**
	 * Copies the settings of a file like the line ending styles
	 * to another CFileTextLines object.
	 */
	void		CopySettings(CFileTextLines * pFileToCopySettingsTo);

	CFileTextLines::UnicodeType GetUnicodeType() const  {return m_UnicodeType;}

private:
	/**
	 * Checks the line endings in a text buffer
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	CFileTextLines::LineEndings CheckLineEndings(LPVOID pBuffer, int cb); 
	/**
	 * Checks the unicode type in a text buffer
	 * \param pBuffer pointer to the buffer containing text
	 * \param cd size of the text buffer in bytes
	 */
	CFileTextLines::UnicodeType CheckUnicodeType(LPVOID pBuffer, int cb); 

	void		SetErrorString();

	void StripAsciiWhiteSpace(CStringA& sLine);


private:
	CString		m_sErrorString;
	CFileTextLines::UnicodeType	m_UnicodeType;
	CFileTextLines::LineEndings m_LineEndings;
};
