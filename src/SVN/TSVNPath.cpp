#include "StdAfx.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"

CTSVNPath::CTSVNPath(void) :
	m_bDirectoryKnown(false),
	m_bIsDirectory(false)
{
}

CTSVNPath::~CTSVNPath(void)
{
}
void CTSVNPath::SetFromSVN(const char* pPath)
{
	Reset();
	m_sFwdslashPath = CString(pPath);
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


const char* CTSVNPath::GetSVNPathNarrow() const
{
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
	}
	if(m_sUTF8FwdslashPath.IsEmpty())
	{
		SetUTF8FwdslashPath(m_sFwdslashPath);
	}
	return m_sUTF8FwdslashPath;
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
	ASSERT(!IsEmpty());
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
		ASSERT(!m_sBackslashPath.IsEmpty());
	}
}
void CTSVNPath::EnsureFwdslashPathSet() const
{
	ASSERT(!IsEmpty());
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
		ASSERT(!m_sFwdslashPath.IsEmpty());
	}
}


// Reset all the caches
void CTSVNPath::Reset()
{
	m_bDirectoryKnown = false;
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
	EnsureBackslashPathSet();
	CTSVNPath retVal;
	retVal.SetFromWin(m_sBackslashPath.Left(m_sBackslashPath.ReverseFind('\\')));
	return retVal;
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
	ASSERT(!IsDirectory());
	EnsureBackslashPathSet();
	int dotPos = m_sBackslashPath.ReverseFind('.');
	int slashPos = m_sBackslashPath.ReverseFind('\\');
	if (dotPos > slashPos)
		return m_sBackslashPath.Mid(dotPos);
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
bool CTSVNPath::IsEquivalentTo(const CTSVNPath& rhs)
{
	// Try and find a slash direction which avoids having to convert
	// both files
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

void CTSVNPath::AppendString(const CString& sAppend)
{
	EnsureFwdslashPathSet();
	if(m_sFwdslashPath[m_sFwdslashPath.GetLength()-1] != '/')
	{
		m_sFwdslashPath += '/';
	}
	CString strCopy = m_sFwdslashPath += sAppend;
	Reset();
	m_sFwdslashPath = strCopy;
}




//////////////////////////////////////////////////////////////////////////

CTSVNPathList::CTSVNPathList()
{

}

void CTSVNPathList::AddPath(const CTSVNPath& newPath)
{
	m_paths.push_back(newPath);
}

int 
CTSVNPathList::GetCount() const
{
	return (int)m_paths.size();
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

bool CTSVNPathList::LoadFromTemporaryFile(const CString& sFilename)
{
	m_paths.clear();
	try
	{
		CString strLine;
		CStdioFile file(sFilename, CFile::typeBinary | CFile::modeRead);

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
	std::sort(m_paths.begin(), m_paths.end(), &CTSVNPath::Compare);
}
