// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "StdAfx.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"
#include "Utils.h"

CTSVNPath::CTSVNPath(void) :
	m_bDirectoryKnown(false),
	m_bIsDirectory(false),
	m_bIsURL(false),
	m_bURLKnown(false)
{
}

CTSVNPath::~CTSVNPath(void)
{
}
// Create a TSVNPath object from an unknown path type (same as using SetFromUnknown)
CTSVNPath::CTSVNPath(const CString& sUnknownPath)
{
	SetFromUnknown(sUnknownPath);
}

void CTSVNPath::SetFromSVN(const char* pPath)
{
	Reset();

	WCHAR buf[MAX_PATH*4];
	if (!MultiByteToWideChar(CP_UTF8, 0, pPath, -1, buf, MAX_PATH*4))
		buf[0] = 0;

	m_sFwdslashPath = CString(buf);
}
void CTSVNPath::SetFromSVN(const CString& sPath)
{
	Reset();
	m_sFwdslashPath = sPath;
}

void CTSVNPath::SetFromWin(LPCTSTR pPath)
{
	Reset();
	m_sBackslashPath = pPath;
}
void CTSVNPath::SetFromWin(const CString& sPath)
{
	Reset();
	m_sBackslashPath = sPath;
}
void CTSVNPath::SetFromWin(const CString& sPath, bool bIsDirectory)
{
	Reset();
	m_sBackslashPath = sPath;
	m_bIsDirectory = bIsDirectory;
	m_bDirectoryKnown = true;
}
void CTSVNPath::SetFromUnknown(const CString& sPath)
{
	Reset();
	// Just set whichever path we think is most likely to be used
	SetFwdslashPath(sPath);
}

LPCTSTR CTSVNPath::GetWinPath() const
{
	if(IsEmpty())
	{
		return _T("");
	}
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
	}
	return m_sBackslashPath;
}
// This is a temporary function, to be used during the migration to 
// the path class.  Ultimately, functions consuming paths should take a CTSVNPath&, not a CString
const CString& CTSVNPath::GetWinPathString() const
{
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
	}
	return m_sBackslashPath;
}

const CString& CTSVNPath::GetSVNPathString() const
{
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
	}
	return m_sFwdslashPath;
}


const char* CTSVNPath::GetSVNApiPath() const
{
	// This funny-looking 'if' is to avoid a subtle problem with empty paths, whereby
	// each call to GetSVNApiPath returns a different pointer value.
	// If you made multiple calls to GetSVNApiPath on the same string, only the last
	// one would give you a valid pointer to an empty string, because each 
	// call would invalidate the previous call's return. 
	if(IsEmpty())
	{
		return "";
	}
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
	}
	if(m_sUTF8FwdslashPath.IsEmpty())
	{
		SetUTF8FwdslashPath(m_sFwdslashPath);
	}
	if (svn_path_is_url(m_sUTF8FwdslashPath))
	{
		if (!CUtils::IsEscaped(m_sUTF8FwdslashPath))
		{
			return CUtils::PathEscape(m_sUTF8FwdslashPath);
		}
	}
	return m_sUTF8FwdslashPath;
}

const CString& CTSVNPath::GetUIPathString() const
{
	if (m_sUIPath.IsEmpty())
	{
		if (IsUrl())
		{
			CStringA sUIPathA = GetSVNApiPath();
			CUtils::Unescape(sUIPathA.GetBuffer());
			sUIPathA.ReleaseBuffer();
			m_sUIPath = CUnicodeUtils::GetUnicode(sUIPathA);
		}
		else
		{
			m_sUIPath = GetWinPathString();
		}
	}
	return m_sUIPath;
}

void CTSVNPath::SetFwdslashPath(const CString& sPath) const
{
	m_sFwdslashPath = sPath;
	m_sFwdslashPath.Trim();
	m_sFwdslashPath.Replace('\\', '/');
	m_sFwdslashPath.TrimRight('/');	
	//workaround for Subversions UNC-path bug
	if (m_sFwdslashPath.Left(10).CompareNoCase(_T("file://///"))==0)
	{
		m_sFwdslashPath.Replace(_T("file://///"), _T("file:///\\"));
	}
	else if (m_sFwdslashPath.Left(9).CompareNoCase(_T("file:////"))==0)
	{
		m_sFwdslashPath.Replace(_T("file:////"), _T("file:///\\"));
	}
	m_sUTF8FwdslashPath.Empty();
}

void CTSVNPath::SetBackslashPath(const CString& sPath) const
{
	m_sBackslashPath = sPath;
	m_sBackslashPath.Trim();
	m_sBackslashPath.Replace('/', '\\');
	m_sBackslashPath.TrimRight('\\');
}

void CTSVNPath::SetUTF8FwdslashPath(const CString& sPath) const
{
	// Only set this from a forward-slash path
	ASSERT(sPath.Find('\\') == -1);
	m_sUTF8FwdslashPath = CUnicodeUtils::GetUTF8(sPath);
}

bool CTSVNPath::IsUrl() const
{
	if (!m_bURLKnown)
	{
		EnsureFwdslashPathSet();
		if(m_sUTF8FwdslashPath.IsEmpty())
		{
			SetUTF8FwdslashPath(m_sFwdslashPath);
		}
		m_bIsURL = !!svn_path_is_url(m_sUTF8FwdslashPath);
		m_bURLKnown = true;
	}
	return m_bIsURL;
}

bool CTSVNPath::IsDirectory() const
{
	if(!m_bDirectoryKnown)
	{
		EnsureBackslashPathSet();
		m_bIsDirectory = !!PathIsDirectory(m_sBackslashPath);
		m_bDirectoryKnown = true;
	}
	return m_bIsDirectory;
}

void CTSVNPath::EnsureBackslashPathSet() const
{
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
		ASSERT(IsEmpty() || !m_sBackslashPath.IsEmpty());
	}
}
void CTSVNPath::EnsureFwdslashPathSet() const
{
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
		ASSERT(IsEmpty() || !m_sFwdslashPath.IsEmpty());
	}
}


// Reset all the caches
void CTSVNPath::Reset()
{
	m_bDirectoryKnown = false;
	m_bURLKnown = false;
	m_sBackslashPath.Empty();
	m_sFwdslashPath.Empty();
	m_sUTF8FwdslashPath.Empty();
	ASSERT(IsEmpty());
}

CTSVNPath CTSVNPath::GetDirectory() const
{
	if(IsDirectory())
	{
		return *this;
	}
	return GetContainingDirectory();
}

CTSVNPath CTSVNPath::GetContainingDirectory() const
{
	EnsureBackslashPathSet();
	CTSVNPath retVal;
	retVal.SetFromWin(m_sBackslashPath.Left(m_sBackslashPath.ReverseFind('\\')));
	return retVal;
}

CString CTSVNPath::GetRootPathString() const
{
	EnsureBackslashPathSet();
	CString workingPath = m_sBackslashPath;
	LPTSTR pPath = workingPath.GetBuffer(MAX_PATH);
	VERIFY(::PathStripToRoot(pPath));
	workingPath.ReleaseBuffer();
	return workingPath;
}


CString CTSVNPath::GetFilename() const
{
	ASSERT(!IsDirectory());
	return GetFileOrDirectoryName();
}

CString CTSVNPath::GetFileOrDirectoryName() const
{
	EnsureBackslashPathSet();
	return m_sBackslashPath.Mid(m_sBackslashPath.ReverseFind('\\')+1);
}

CString CTSVNPath::GetFileExtension() const
{
	if(!IsDirectory())
	{
		EnsureBackslashPathSet();
		int dotPos = m_sBackslashPath.ReverseFind('.');
		int slashPos = m_sBackslashPath.ReverseFind('\\');
		if (dotPos > slashPos)
			return m_sBackslashPath.Mid(dotPos);
	}
	return CString();
}


bool CTSVNPath::ArePathStringsEqual(const CString& sP1, const CString& sP2)
{
	int length = sP1.GetLength();
	if(length != sP2.GetLength())
	{
		// Different lengths
		return false;
	}
	// We work from the end of the strings, because path differences
	// are more likely to occur at the far end of a string
	LPCTSTR pP1Start = sP1;
	LPCTSTR pP1 = pP1Start+(length-1);
	LPCTSTR pP2 = ((LPCTSTR)sP2)+(length-1);
	while(length-- > 0)
	{
		if(_totupper(*pP1--) != _totupper(*pP2--))
		{
			return false;
		}
	}
	return true;
}

bool CTSVNPath::IsEmpty() const
{
	return m_sFwdslashPath.IsEmpty() && m_sBackslashPath.IsEmpty();
}

// Test if both paths refer to the same item
// Ignores case and slash direction
bool CTSVNPath::IsEquivalentTo(const CTSVNPath& rhs) const
{
	// Try and find a slash direction which avoids having to convert
	// both filenames
	if(!m_sBackslashPath.IsEmpty())
	{
		// *We've* got a \ path - make sure that the RHS also has a \ path
		rhs.EnsureBackslashPathSet();
		return ArePathStringsEqual(m_sBackslashPath, rhs.m_sBackslashPath);
	}
	else
	{
		// Assume we've got a fwdslash path and make sure that the RHS has one
		rhs.EnsureFwdslashPathSet();
		return ArePathStringsEqual(m_sFwdslashPath, rhs.m_sFwdslashPath);
	}
}

bool 
CTSVNPath::IsAncestorOf(const CTSVNPath& possibleDescendant) const
{
	if(!IsDirectory())
	{
		// If we're not a directory, then we can't be an ancestor of anybody
		return false;
	}
	possibleDescendant.EnsureBackslashPathSet();
	EnsureBackslashPathSet();

	return ArePathStringsEqual(m_sBackslashPath, possibleDescendant.m_sBackslashPath.Left(m_sBackslashPath.GetLength()));
}

// Get a string representing the file path, optionally with a base 
// section stripped off the front.
CString CTSVNPath::GetDisplayString(const CTSVNPath* pOptionalBasePath /* = NULL*/) const
{
	EnsureFwdslashPathSet();
	if(pOptionalBasePath != NULL)
	{
		// Find the length of the base-path without having to do an 'ensure' on it
		int baseLength = max(pOptionalBasePath->m_sBackslashPath.GetLength(), pOptionalBasePath->m_sFwdslashPath.GetLength());

		// Now, chop that baseLength of the front of the path
		return m_sFwdslashPath.Mid(baseLength).TrimLeft('/');
	}
	return m_sFwdslashPath;
}

int 
CTSVNPath::Compare(const CTSVNPath& left, const CTSVNPath& right)
{
	left.EnsureFwdslashPathSet();
	right.EnsureFwdslashPathSet();
	return left.m_sFwdslashPath.CompareNoCase(right.m_sFwdslashPath);
}

bool
CTSVNPath::PredLeftEarlierThanRight(const CTSVNPath& left, const CTSVNPath& right)
{
	return Compare(left, right) < 0;
}
bool
CTSVNPath::PredLeftEquivalentToRight(const CTSVNPath& left, const CTSVNPath& right)
{
	return left.IsEquivalentTo(right);
}

void CTSVNPath::AppendRawString(const CString& sAppend)
{
	EnsureFwdslashPathSet();
	CString strCopy = m_sFwdslashPath += sAppend;
	SetFromUnknown(strCopy);
}

void CTSVNPath::AppendPathString(const CString& sAppend)
{
	EnsureFwdslashPathSet();
	CString cleanAppend(sAppend);
	cleanAppend.Replace('\\', '/');
	cleanAppend.TrimLeft('/');
	CString strCopy = m_sFwdslashPath + _T("/") + cleanAppend;
	SetFromSVN(strCopy);
}



//////////////////////////////////////////////////////////////////////////

CTSVNPathList::CTSVNPathList()
{

}

// A constructor which allows a path list to be easily built which one initial entry in
CTSVNPathList::CTSVNPathList(const CTSVNPath& firstEntry)
{
	AddPath(firstEntry);
}

void CTSVNPathList::AddPath(const CTSVNPath& newPath)
{
	m_paths.push_back(newPath);
	m_commonBaseDirectory.Reset();
}
int CTSVNPathList::GetCount() const
{
	return (int)m_paths.size();
}
void CTSVNPathList::Clear()
{
	m_paths.clear();
	m_commonBaseDirectory.Reset();
}

const CTSVNPath& CTSVNPathList::operator[](int index) const
{
	ASSERT(index >= 0 && index < (int)m_paths.size());
	return m_paths[index];
}

bool CTSVNPathList::AreAllPathsFiles() const
{
	// Look through the vector for any directories - if we find them, return false
	return std::find_if(m_paths.begin(), m_paths.end(), std::mem_fun_ref(CTSVNPath::IsDirectory)) != m_paths.end();
}

bool CTSVNPathList::LoadFromTemporaryFile(const CTSVNPath& filename)
{
	Clear();
	try
	{
		CString strLine;
		CStdioFile file(filename.GetWinPath(), CFile::typeBinary | CFile::modeRead);

		// for every selected file/folder
		CTSVNPath path;
		while (file.ReadString(strLine))
		{
			path.SetFromUnknown(strLine);
			AddPath(path);
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading target file list\n");
		TCHAR error[10000] = {0};
		pE->GetErrorMessage(error, 10000);
		CMessageBox::Show(NULL, error, _T("TortoiseSVN"), MB_ICONERROR);
		pE->Delete();
		return false;
	}
	return true;
}



void CTSVNPathList::LoadFromAsteriskSeparatedString(const CString& sPathString)
{
	int pos = 0;
	CString temp;
	for(;;)
	{
		temp = sPathString.Tokenize(_T("*"),pos);
		if(temp.IsEmpty())
		{
			break;
		}
		AddPath(CTSVNPath(CUtils::GetLongPathname(temp)));
	} 
}

bool 
CTSVNPathList::AreAllPathsFilesInOneDirectory() const
{
	// Check if all the paths are files and in the same directory
	PathVector::const_iterator it;
	m_commonBaseDirectory.Reset();
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if(it->IsDirectory())
		{
			return false;
		}
		const CTSVNPath& baseDirectory = it->GetDirectory();
		if(m_commonBaseDirectory.IsEmpty())
		{
			m_commonBaseDirectory = baseDirectory;
		}
		else if(!m_commonBaseDirectory.IsEquivalentTo(baseDirectory))
		{
			// Different path
			m_commonBaseDirectory.Reset();
			return false;
		}
	}
	return true;
}

CTSVNPath CTSVNPathList::GetCommonDirectory() const
{
	ASSERT(!m_commonBaseDirectory.IsEmpty());
	return m_commonBaseDirectory;
}

bool CTSVNPathList::WriteToTemporaryFile(const CString& sFilename) const
{
	try
	{
		CStdioFile file(sFilename, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
		PathVector::const_iterator it;
		for(it = m_paths.begin(); it != m_paths.end(); ++it)
		{
			file.WriteString(it->GetSVNPathString()+_T("\n"));
		} 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in writing temp file\n");
		pE->Delete();
		return false;
	}
	return true;
}

void CTSVNPathList::SortByPathname()
{
	std::sort(m_paths.begin(), m_paths.end(), &CTSVNPath::PredLeftEarlierThanRight);
}

void CTSVNPathList::DeleteAllFiles()
{
	PathVector::const_iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		ASSERT(!it->IsDirectory());
		::DeleteFile(it->GetWinPath());
	}
	Clear();
}

void CTSVNPathList::RemoveDuplicates()
{
	SortByPathname();
	// Remove the duplicates
	// (Unique moves them to the end of the vector, then erase chops them off)
	m_paths.erase(std::unique(m_paths.begin(), m_paths.end(), &CTSVNPath::PredLeftEquivalentToRight), m_paths.end());
}

//////////////////////////////////////////////////////////////////////////


#if defined(_DEBUG)
// Some test cases for these classes
static class CPathTests
{
public:
	CPathTests()
	{
		GetDirectoryTest();
		SortTest();
		RawAppendTest();
		PathAppendTest();
		RemoveDuplicatesTest();
		ListLoadingTest();
	}

private:
	void GetDirectoryTest()
	{
		// Bit tricky, this test, because we need to know something about the file
		// layout on the machine which is running the test
		TCHAR winDir[MAX_PATH+1];
		GetWindowsDirectory(winDir, MAX_PATH);
		CString sWinDir(winDir);

		CTSVNPath testPath;
		// This is a file which we know will always be there
		testPath.SetFromUnknown(sWinDir + _T("\\win.ini"));
		ASSERT(!testPath.IsDirectory());
		ASSERT(testPath.GetDirectory().GetWinPathString() == sWinDir);
		ASSERT(testPath.GetContainingDirectory().GetWinPathString() == sWinDir);

		// Now do the test on the win directory itself - It's hard to be sure about the containing directory
		// but we know it must be different to the directory itself
		testPath.SetFromUnknown(sWinDir);
		ASSERT(testPath.IsDirectory());
		ASSERT(testPath.GetDirectory().GetWinPathString() == sWinDir);
		ASSERT(testPath.GetContainingDirectory().GetWinPathString() != sWinDir);
		ASSERT(testPath.GetContainingDirectory().GetWinPathString().GetLength() < sWinDir.GetLength());
	}

	void SortTest()
	{
		CTSVNPathList testList;
		CTSVNPath testPath;
		testPath.SetFromUnknown(_T("c:/Z"));
		testList.AddPath(testPath);
		testPath.SetFromUnknown(_T("c:/B"));
		testList.AddPath(testPath);
		testPath.SetFromUnknown(_T("c:\\a"));
		testList.AddPath(testPath);
		testPath.SetFromUnknown(_T("c:/Test"));
		testList.AddPath(testPath);

		testList.SortByPathname();

		ASSERT(testList[0].GetWinPathString() == _T("c:\\a"));
		ASSERT(testList[1].GetWinPathString() == _T("c:\\B"));
		ASSERT(testList[2].GetWinPathString() == _T("c:\\Test"));
		ASSERT(testList[3].GetWinPathString() == _T("c:\\Z"));
	}

	void RawAppendTest()
	{
		CTSVNPath testPath(_T("c:/test/"));
		testPath.AppendRawString(_T("/Hello"));
		ASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello"));

		testPath.AppendRawString(_T("\\T2"));
		ASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello\\T2"));

		CTSVNPath testFilePath(_T("C:\\windows\\win.ini"));
		CTSVNPath testBasePath(_T("c:/temp/myfile.txt"));
		testBasePath.AppendRawString(testFilePath.GetFileExtension());
		ASSERT(testBasePath.GetWinPathString() == _T("c:\\temp\\myfile.txt.ini"));
	}

	void PathAppendTest()
	{
		CTSVNPath testPath(_T("c:/test/"));
		testPath.AppendPathString(_T("/Hello"));
		ASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello"));

		testPath.AppendPathString(_T("T2"));
		ASSERT(testPath.GetWinPathString() == _T("c:\\test\\Hello\\T2"));

		CTSVNPath testFilePath(_T("C:\\windows\\win.ini"));
		CTSVNPath testBasePath(_T("c:/temp/myfile.txt"));
		// You wouldn't want to do this in real life - you'd use append-raw
		testBasePath.AppendPathString(testFilePath.GetFileExtension());
		ASSERT(testBasePath.GetWinPathString() == _T("c:\\temp\\myfile.txt\\.ini"));
	}

	void RemoveDuplicatesTest()
	{
		CTSVNPathList list;
		list.AddPath(CTSVNPath(_T("Z")));
		list.AddPath(CTSVNPath(_T("A")));
		list.AddPath(CTSVNPath(_T("E")));
		list.AddPath(CTSVNPath(_T("e")));

		ASSERT(list.GetCount() == 4);

		list.RemoveDuplicates();

		ASSERT(list.GetCount() == 3);

		ASSERT(list[0].GetWinPathString() == _T("A"));
		ASSERT(list[1].GetWinPathString().CompareNoCase(_T("e")) == 0);
		ASSERT(list[2].GetWinPathString() == _T("Z"));
	}

	void ListLoadingTest()
	{
		CString sPathList(_T("Path1*c:\\path2 with spaces and stuff*\\funnypath\\*"));
		CTSVNPathList testList;
		testList.LoadFromAsteriskSeparatedString(sPathList);

		ASSERT(testList.GetCount() == 3);
		ASSERT(testList[0].GetWinPathString() == _T("Path1"));
		ASSERT(testList[1].GetWinPathString() == _T("c:\\path2 with spaces and stuff"));
		ASSERT(testList[2].GetWinPathString() == _T("\\funnypath"));
	}



} TSVNPathTests;
#endif


