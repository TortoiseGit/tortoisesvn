// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "SVNAdminDir.h"
#include "SVNHelpers.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "DebugOutput.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include <regex>
#include <functional>
#include <memory>

#if defined(_MFC_VER)
#include "AppUtils.h"
#include "StringUtils.h"
#endif


CTSVNPath::CTSVNPath(void) :
    m_bDirectoryKnown(false),
    m_bIsDirectory(false),
    m_bIsURL(false),
    m_bURLKnown(false),
    m_bHasAdminDirKnown(false),
    m_bHasAdminDir(false),
    m_bIsValidOnWindowsKnown(false),
    m_bIsValidOnWindows(false),
    m_bIsReadOnly(false),
    m_bIsAdminDirKnown(false),
    m_bIsAdminDir(false),
    m_bIsWCRootKnown(false),
    m_bIsWCRoot(false),
    m_bExists(false),
    m_bExistsKnown(false),
    m_bLastWriteTimeKnown(0),
    m_lastWriteTime(0),
    m_customData(NULL),
    m_bIsSpecialDirectoryKnown(false),
    m_bIsSpecialDirectory(false),
    m_fileSize(0)
{
}

CTSVNPath::~CTSVNPath(void)
{
}
// Create a TSVNPath object from an unknown path type (same as using SetFromUnknown)
CTSVNPath::CTSVNPath(const CString& sUnknownPath) :
    m_bDirectoryKnown(false),
    m_bIsDirectory(false),
    m_bIsURL(false),
    m_bURLKnown(false),
    m_bHasAdminDirKnown(false),
    m_bHasAdminDir(false),
    m_bIsValidOnWindowsKnown(false),
    m_bIsValidOnWindows(false),
    m_bIsReadOnly(false),
    m_bIsAdminDirKnown(false),
    m_bIsAdminDir(false),
    m_bIsWCRootKnown(false),
    m_bIsWCRoot(false),
    m_bExists(false),
    m_bExistsKnown(false),
    m_bLastWriteTimeKnown(0),
    m_lastWriteTime(0),
    m_fileSize(0),
    m_customData(NULL),
    m_bIsSpecialDirectoryKnown(false),
    m_bIsSpecialDirectory(false)
{
    SetFromUnknown(sUnknownPath);
}

void CTSVNPath::SetFromSVN(const char* pPath)
{
    Reset();
    if (pPath == NULL)
        return;
    m_sFwdslashPath = CUnicodeUtils::GetUnicode(pPath);
    SanitizeRootPath(m_sFwdslashPath, true);
}

void CTSVNPath::SetFromSVN(const char* pPath, bool bIsDirectory)
{
    SetFromSVN(pPath);
    m_bDirectoryKnown = true;
    m_bIsDirectory = bIsDirectory;
}

void CTSVNPath::SetFromSVN(const CString& sPath)
{
    Reset();
    m_sFwdslashPath = sPath;
    SanitizeRootPath(m_sFwdslashPath, true);
}

void CTSVNPath::SetFromWin(LPCTSTR pPath)
{
    Reset();
    m_sBackslashPath = pPath;
    m_sBackslashPath.Replace(L"\\\\?\\", L"");
    SanitizeRootPath(m_sBackslashPath, false);
    ATLASSERT(m_sBackslashPath.Find('/')<0);
}
void CTSVNPath::SetFromWin(const CString& sPath)
{
    Reset();
    m_sBackslashPath = sPath;
    m_sBackslashPath.Replace(L"\\\\?\\", L"");
    SanitizeRootPath(m_sBackslashPath, false);
}
void CTSVNPath::SetFromWin(LPCTSTR pPath, bool bIsDirectory)
{
    Reset();
    m_sBackslashPath = pPath;
    m_bIsDirectory = bIsDirectory;
    m_bDirectoryKnown = true;
    SanitizeRootPath(m_sBackslashPath, false);
}
void CTSVNPath::SetFromWin(const CString& sPath, bool bIsDirectory)
{
    Reset();
    m_sBackslashPath = sPath;
    m_bIsDirectory = bIsDirectory;
    m_bDirectoryKnown = true;
    SanitizeRootPath(m_sBackslashPath, false);
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
        return L"";
    }
    if(m_sBackslashPath.IsEmpty())
    {
        SetBackslashPath(m_sFwdslashPath);
    }
    if(m_sBackslashPath.GetLength() >= 248)
    {
        m_sLongBackslashPath = L"\\\\?\\" + m_sBackslashPath;
        return m_sLongBackslashPath;
    }
    return m_sBackslashPath;
}

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


const char* CTSVNPath::GetSVNApiPath(apr_pool_t *pool) const
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
        m_sUTF8FwdslashPathEscaped = CPathUtils::PathEscape(m_sUTF8FwdslashPath);
        m_sUTF8FwdslashPathEscaped.Replace("file:////", "file://");
        m_sUTF8FwdslashPathEscaped = svn_uri_canonicalize(m_sUTF8FwdslashPathEscaped, pool);
        return m_sUTF8FwdslashPathEscaped;
    }
    else
    {
        m_sUTF8FwdslashPath = svn_dirent_canonicalize(m_sUTF8FwdslashPath, pool);
        // for UNC paths that point to the server directly (e.g., \\MYSERVER), not
        // to a share on the server, the svn_dirent_canonicalize() API returns
        // a wrong path that asserts when subversion checks that the path is absolute
        // (it returns /MYSERVER instead of //MYSERVER).
        // We can't just add the second slash, since that would assert when svn checks
        // for canonicalized paths.
        // Since the network server itself isn't interesting anyway but only shares,
        // we just return an empty path here.
        if (!svn_dirent_is_absolute(m_sUTF8FwdslashPath))
            m_sUTF8FwdslashPath.Empty();
    }
    return m_sUTF8FwdslashPath;
}

const CString& CTSVNPath::GetUIPathString() const
{
    if (m_sUIPath.IsEmpty())
    {
#if defined(_MFC_VER)
        //BUGBUG HORRIBLE!!! - CPathUtils::IsEscaped doesn't need to be MFC-only
        if (IsUrl())
        {
            m_sUIPath = CPathUtils::PathUnescape(GetSVNPathString());
            m_sUIPath.Replace(L"file:////", L"file://");

        }
        else
#endif
        {
            m_sUIPath = GetWinPathString();
        }
    }
    return m_sUIPath;
}

void CTSVNPath::SetFwdslashPath(const CString& sPath) const
{
    m_sFwdslashPath = sPath;
    m_sFwdslashPath.Replace('\\', '/');

    // We don't leave a trailing /
    m_sFwdslashPath.TrimRight('/');
    m_sFwdslashPath.Replace(L"//?/", L"");

    SanitizeRootPath(m_sFwdslashPath, true);

    m_sFwdslashPath.Replace(L"file:////", L"file://");

    m_sUTF8FwdslashPath.Empty();
}

void CTSVNPath::SetBackslashPath(const CString& sPath) const
{
    m_sBackslashPath = sPath;
    m_sBackslashPath.Replace('/', '\\');
    m_sBackslashPath.TrimRight('\\');
    SanitizeRootPath(m_sBackslashPath, false);
}

void CTSVNPath::SetUTF8FwdslashPath(const CString& sPath) const
{
    m_sUTF8FwdslashPath = CUnicodeUtils::GetUTF8(sPath);
}

void CTSVNPath::SanitizeRootPath(CString& sPath, bool bIsForwardPath) const
{
    // Make sure to add the trailing slash to root paths such as 'C:'
    if (sPath.GetLength() == 2 && sPath[1] == ':')
    {
        sPath += (bIsForwardPath) ? L"/" : L"\\";
    }
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
        UpdateAttributes();
    }
    return m_bIsDirectory;
}

bool CTSVNPath::Exists() const
{
    if (!m_bExistsKnown)
    {
        UpdateAttributes();
    }
    return m_bExists;
}

bool CTSVNPath::Delete(bool bTrash) const
{
    EnsureBackslashPathSet();
    ::SetFileAttributes(m_sBackslashPath, FILE_ATTRIBUTE_NORMAL);
    bool bRet = false;
    if (Exists())
    {
        if ((bTrash)||(IsDirectory()))
        {
            std::unique_ptr<TCHAR[]> buf(new TCHAR[m_sBackslashPath.GetLength()+2]);
            wcscpy_s(buf.get(), m_sBackslashPath.GetLength()+2, m_sBackslashPath);
            buf[m_sBackslashPath.GetLength()] = 0;
            buf[m_sBackslashPath.GetLength()+1] = 0;
            bRet = CTSVNPathList::DeleteViaShell(buf.get(), bTrash, NULL);
        }
        else
        {
            bRet = !!::DeleteFile(m_sBackslashPath);
        }
    }
    m_bExists = false;
    m_bExistsKnown = true;
    return bRet;
}

__int64  CTSVNPath::GetLastWriteTime() const
{
    if(!m_bLastWriteTimeKnown)
    {
        UpdateAttributes();
    }
    return m_lastWriteTime;
}

__int64 CTSVNPath::GetFileSize() const
{
    if(!m_bDirectoryKnown)
    {
        UpdateAttributes();
    }
    return m_fileSize;
}

bool CTSVNPath::IsReadOnly() const
{
    if(!m_bLastWriteTimeKnown)
    {
        UpdateAttributes();
    }
    return m_bIsReadOnly;
}

void CTSVNPath::UpdateAttributes() const
{
    EnsureBackslashPathSet();
    WIN32_FILE_ATTRIBUTE_DATA attribs;
    if (m_sBackslashPath.GetLength() >= 248)
        m_sLongBackslashPath = L"\\\\?\\" + m_sBackslashPath;
    if(GetFileAttributesEx(m_sBackslashPath.GetLength() >= 248 ? m_sLongBackslashPath : m_sBackslashPath, GetFileExInfoStandard, &attribs))
    {
        m_bIsDirectory = !!(attribs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        // don't cast directly to an __int64:
        // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724284%28v=vs.85%29.aspx
        // "Do not cast a pointer to a FILETIME structure to either a ULARGE_INTEGER* or __int64* value
        // because it can cause alignment faults on 64-bit Windows."
        m_lastWriteTime = static_cast<__int64>(attribs.ftLastWriteTime.dwHighDateTime) << 32 | attribs.ftLastWriteTime.dwLowDateTime;
        if (m_bIsDirectory)
        {
            m_fileSize = 0;
        }
        else
        {
            m_fileSize = ((INT64)( (DWORD)(attribs.nFileSizeLow) ) | ( (INT64)( (DWORD)(attribs.nFileSizeHigh) )<<32 ));
        }
        m_bIsReadOnly = !!(attribs.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
        m_bExists = true;
    }
    else
    {
        DWORD err = GetLastError();
        m_lastWriteTime = 0;
        m_fileSize = 0;
        if ((err == ERROR_FILE_NOT_FOUND)||(err == ERROR_PATH_NOT_FOUND)||(err == ERROR_INVALID_NAME))
        {
            m_bIsDirectory = false;
            m_bExists = false;
        }
        else if (err == ERROR_NOT_READY)
        {
            // this is a device root (e.g. a DVD drive with no media in it)
            m_bIsDirectory = true;
            m_bExists = true;
        }
        else
        {
            m_bIsDirectory = false;
            m_bExists = true;
            return;
        }
    }
    m_bDirectoryKnown = true;
    m_bLastWriteTimeKnown = true;
    m_bExistsKnown = true;
}


void CTSVNPath::EnsureBackslashPathSet() const
{
    if(m_sBackslashPath.IsEmpty())
    {
        SetBackslashPath(m_sFwdslashPath);
        ATLASSERT(IsEmpty() || !m_sBackslashPath.IsEmpty());
    }
}
void CTSVNPath::EnsureFwdslashPathSet() const
{
    if(m_sFwdslashPath.IsEmpty())
    {
        SetFwdslashPath(m_sBackslashPath);
        ATLASSERT(IsEmpty() || !m_sFwdslashPath.IsEmpty());
    }
}


// Reset all the caches
void CTSVNPath::Reset()
{
    m_bDirectoryKnown = false;
    m_bURLKnown = false;
    m_bLastWriteTimeKnown = false;
    m_bHasAdminDirKnown = false;
    m_bIsValidOnWindowsKnown = false;
    m_bIsAdminDirKnown = false;
    m_bExistsKnown = false;
    m_bIsWCRootKnown = false;
    m_bIsWCRoot = false;
    m_bIsSpecialDirectoryKnown = false;
    m_bIsSpecialDirectory = false;

    m_sBackslashPath.Empty();
    m_sLongBackslashPath.Empty();
    m_sFwdslashPath.Empty();
    m_sUTF8FwdslashPath.Empty();
    m_sUIPath.Empty();
    ATLASSERT(IsEmpty());
}

CTSVNPath CTSVNPath::GetDirectory() const
{
    if ((IsDirectory())||(!Exists()))
    {
        return *this;
    }
    return GetContainingDirectory();
}

CTSVNPath CTSVNPath::GetContainingDirectory() const
{
    EnsureBackslashPathSet();

    CString sDirName = m_sBackslashPath.Left(m_sBackslashPath.ReverseFind('\\'));
    if(sDirName.GetLength() == 2 && sDirName[1] == ':')
    {
        // This is a root directory, which needs a trailing slash
        sDirName += '\\';
        if(sDirName == m_sBackslashPath)
        {
            // We were clearly provided with a root path to start with - we should return nothing now
            sDirName.Empty();
        }
    }
    if(sDirName.GetLength() == 1 && sDirName[0] == '\\')
    {
        // We have an UNC path and we already are the root
        sDirName.Empty();
    }
    CTSVNPath retVal;
    retVal.SetFromWin(sDirName);
    return retVal;
}

CString CTSVNPath::GetRootPathString() const
{
    EnsureBackslashPathSet();
    CString workingPath = m_sBackslashPath;
    LPTSTR pPath = workingPath.GetBuffer(MAX_PATH);     // MAX_PATH ok here.
    ATLVERIFY(::PathStripToRoot(pPath));
    workingPath.ReleaseBuffer();
    return workingPath;
}


CString CTSVNPath::GetFilename() const
{
    ATLASSERT(!IsDirectory());
    return GetFileOrDirectoryName();
}

CString CTSVNPath::GetFileOrDirectoryName() const
{
    EnsureBackslashPathSet();
    return m_sBackslashPath.Mid(m_sBackslashPath.ReverseFind('\\')+1);
}

CString CTSVNPath::GetUIFileOrDirectoryName() const
{
    GetUIPathString();
    CString sUIPath = m_sUIPath;
    sUIPath.Replace('\\', '/');
    return sUIPath.Mid(sUIPath.ReverseFind('/')+1);
}

CString CTSVNPath::GetFileExtension() const
{
    if(!IsDirectory())
    {
        return GetFileOrDirExtension();
    }
    return CString();
}

CString CTSVNPath::GetFileOrDirExtension() const
{
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
        if(_totlower(*pP1--) != _totlower(*pP2--))
        {
            return false;
        }
    }
    return true;
}

bool CTSVNPath::ArePathStringsEqualWithCase(const CString& sP1, const CString& sP2)
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
        if((*pP1--) != (*pP2--))
        {
            return false;
        }
    }
    return true;
}

bool CTSVNPath::IsEmpty() const
{
    // Check the backward slash path first, since the chance that this
    // one is set is higher. In case of a 'false' return value it's a little
    // bit faster.
    return m_sBackslashPath.IsEmpty() && m_sFwdslashPath.IsEmpty();
}

// Test if both paths refer to the same item
// Ignores slash direction
bool CTSVNPath::IsEquivalentTo(const CTSVNPath& rhs) const
{
    // Try and find a slash direction which avoids having to convert
    // both filenames
    if(!m_sBackslashPath.IsEmpty())
    {
        // *We've* got a \ path - make sure that the RHS also has a \ path
        rhs.EnsureBackslashPathSet();
        return ArePathStringsEqualWithCase(m_sBackslashPath, rhs.m_sBackslashPath);
    }
    else
    {
        // Assume we've got a fwdslash path and make sure that the RHS has one
        rhs.EnsureFwdslashPathSet();
        return ArePathStringsEqualWithCase(m_sFwdslashPath, rhs.m_sFwdslashPath);
    }
}

// Test if both paths refer to the same item
// Ignores case and slash direction
bool CTSVNPath::IsEquivalentToWithoutCase(const CTSVNPath& rhs) const
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

bool CTSVNPath::IsAncestorOf(const CTSVNPath& possibleDescendant) const
{
    possibleDescendant.EnsureBackslashPathSet();
    EnsureBackslashPathSet();

    bool bPathStringsEqual = ArePathStringsEqual(m_sBackslashPath, possibleDescendant.m_sBackslashPath.Left(m_sBackslashPath.GetLength()));
    if (m_sBackslashPath.GetLength() >= possibleDescendant.GetWinPathString().GetLength())
    {
        return bPathStringsEqual;
    }

    return (bPathStringsEqual &&
            ((possibleDescendant.m_sBackslashPath[m_sBackslashPath.GetLength()] == '\\')||
            (m_sBackslashPath.GetLength()==3 && m_sBackslashPath[1]==':')));
}

// Get a string representing the file path, optionally with a base
// section stripped off the front.
LPCTSTR CTSVNPath::GetDisplayString(const CTSVNPath* pOptionalBasePath /* = NULL*/) const
{
    EnsureFwdslashPathSet();
    if(pOptionalBasePath != NULL)
    {
        // Find the length of the base-path without having to do an 'ensure' on it
        int baseLength = max(pOptionalBasePath->m_sBackslashPath.GetLength(), pOptionalBasePath->m_sFwdslashPath.GetLength());

        // Now, chop that baseLength of the front of the path
        LPCTSTR result = m_sFwdslashPath;
        result += baseLength;
        while (*result == '/')
            ++result;

        return result;
    }
    return m_sFwdslashPath;
}

int CTSVNPath::CompareWithCase(const CTSVNPath& left, const CTSVNPath& right)
{
    left.EnsureFwdslashPathSet();
    right.EnsureFwdslashPathSet();
    return left.m_sFwdslashPath.Compare(right.m_sFwdslashPath);
}

int CTSVNPath::Compare(const CTSVNPath& left, const CTSVNPath& right)
{
    left.EnsureBackslashPathSet();
    right.EnsureBackslashPathSet();
    return CStringUtils::FastCompareNoCase (left.m_sBackslashPath, right.m_sBackslashPath);
}

bool operator<(const CTSVNPath& left, const CTSVNPath& right)
{
    return CTSVNPath::Compare(left, right) < 0;
}

bool CTSVNPath::PredLeftEquivalentToRight(const CTSVNPath& left, const CTSVNPath& right)
{
    return left.IsEquivalentTo(right);
}

bool CTSVNPath::CheckChild(const CTSVNPath &parent, const CTSVNPath& child)
{
    return parent.IsAncestorOf(child);
}

void CTSVNPath::AppendRawString(const CString& sAppend)
{
    EnsureFwdslashPathSet();
    CString strCopy = m_sFwdslashPath += sAppend;
    SetFromUnknown(strCopy);
}

void CTSVNPath::AppendPathString(const CString& sAppend)
{
    EnsureBackslashPathSet();
    CString cleanAppend(sAppend);
    cleanAppend.Replace('/', '\\');
    cleanAppend.TrimLeft('\\');
    m_sBackslashPath.TrimRight('\\');
    CString strCopy = m_sBackslashPath + L"\\" + cleanAppend;
    SetFromWin(strCopy);
}


bool CTSVNPath::IsAdminDir() const
{
    if (m_bIsAdminDirKnown)
        return m_bIsAdminDir;

    EnsureBackslashPathSet();
    m_bIsAdminDir = g_SVNAdminDir.IsAdminDirPath(m_sBackslashPath);
    m_bIsAdminDirKnown = true;
    return m_bIsAdminDir;
}

bool CTSVNPath::IsWCRoot() const
{
    if (m_bIsWCRootKnown)
        return m_bIsWCRoot;

    EnsureBackslashPathSet();
    m_bIsWCRoot = g_SVNAdminDir.IsWCRoot(m_sBackslashPath);
    m_bIsWCRootKnown = true;
    return m_bIsWCRoot;
}

bool CTSVNPath::IsValidOnWindows() const
{
    if (m_bIsValidOnWindowsKnown)
        return m_bIsValidOnWindows;

    m_bIsValidOnWindows = true;
    EnsureBackslashPathSet();
    std::wstring checkPath = m_sBackslashPath;
    if (IsUrl())
    {
        CString uipath = CPathUtils::PathUnescape(GetSVNPathString());
        uipath.Replace('/', '\\');
        checkPath = uipath.Mid(uipath.Find('\\', uipath.Find(L":\\\\")+3)+1);
    }
    try
    {
        // now check for illegal filenames
        std::tr1::wregex rx2(L"(\\\\(lpt\\d|com\\d|aux|nul|prn|con)(\\\\|$))|\\*|[^\\\\]\\?|\\||<|>|\\:[^\\\\]", std::tr1::regex_constants::icase | std::tr1::regex_constants::ECMAScript);
        if (std::tr1::regex_search(checkPath, rx2, std::tr1::regex_constants::match_default))
            m_bIsValidOnWindows = false;
    }
    catch (std::exception) {}

    m_bIsValidOnWindowsKnown = true;
    return m_bIsValidOnWindows;
}

bool CTSVNPath::IsSpecialDirectory() const
{
    if (m_bIsSpecialDirectoryKnown)
        return m_bIsSpecialDirectory;

    static LPCTSTR specialDirectories[]
        = { L"trunk", L"tags", L"branches" };

    for (int i=0 ; i<_countof(specialDirectories) ; ++i)
    {
        CString name = GetFileOrDirectoryName();
        if (0 == name.CompareNoCase(specialDirectories[i]))
        {
            m_bIsSpecialDirectory = true;
            break;
        }
    }

    m_bIsSpecialDirectoryKnown = true;

    return m_bIsSpecialDirectory;
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

CTSVNPath& CTSVNPathList::operator[](INT_PTR index)
{
    ATLASSERT(index >= 0 && index < (INT_PTR)m_paths.size());
    m_commonBaseDirectory.Reset();
    return m_paths[index];
}

const CTSVNPath& CTSVNPathList::operator[](INT_PTR index) const
{
    ATLASSERT(index >= 0 && index < (INT_PTR)m_paths.size());
    return m_paths[index];
}

bool CTSVNPathList::AreAllPathsFiles() const
{
    // Look through the vector for any directories - if we find them, return false
    return std::find_if(m_paths.begin(), m_paths.end(), std::mem_fun_ref(&CTSVNPath::IsDirectory)) == m_paths.end();
}


#if defined(_MFC_VER)

bool CTSVNPathList::LoadFromFile(const CTSVNPath& filename)
{
    Clear();
    try
    {
        CString strLine;
        CStdioFile file(filename.GetWinPath(), CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite);

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
        std::unique_ptr<TCHAR[]> error(new TCHAR[10000]);
        pE->GetErrorMessage(error.get(), 10000);
        ::MessageBox(NULL, error.get(), L"TortoiseSVN", MB_ICONERROR);
        pE->Delete();
        return false;
    }
    return true;
}

bool CTSVNPathList::WriteToFile(const CString& sFilename, bool bANSI /* = false */) const
{
    try
    {
        if (bANSI)
        {
            CStdioFile file(sFilename, CFile::typeText | CFile::modeReadWrite | CFile::modeCreate);
            PathVector::const_iterator it;
            for(it = m_paths.begin(); it != m_paths.end(); ++it)
            {
                CStringA line = CStringA(it->GetSVNPathString()) + '\n';
                file.Write(line, line.GetLength());
            }
            file.Close();
        }
        else
        {
            CStdioFile file(sFilename, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
            PathVector::const_iterator it;
            for(it = m_paths.begin(); it != m_paths.end(); ++it)
            {
                file.WriteString(it->GetSVNPathString()+L"\n");
            }
            file.Close();
        }
    }
    catch (CFileException* pE)
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException in writing temp file\n");
        pE->Delete();
        return false;
    }
    return true;
}
#endif // _MFC_VER


void CTSVNPathList::LoadFromAsteriskSeparatedString(const CString& sPathString)
{
    if (GetCount() > 0)
        Clear();

    int pos = 0;
    CString temp;
    for(;;)
    {
        temp = sPathString.Tokenize(L"*",pos);
        if(temp.IsEmpty())
        {
            break;
        }
        AddPath(CTSVNPath(CPathUtils::GetLongPathname(temp)));
    }
}

CString CTSVNPathList::CreateAsteriskSeparatedString() const
{
    CString sRet;
    PathVector::const_iterator it;
    for(it = m_paths.begin(); it != m_paths.end(); ++it)
    {
        if (!sRet.IsEmpty())
            sRet += L"*";
        sRet += it->IsUrl() ? it->GetSVNPathString() : it->GetWinPathString();
    }
    return sRet;
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
    if (m_commonBaseDirectory.IsEmpty())
    {
        PathVector::const_iterator it;
        for(it = m_paths.begin(); it != m_paths.end(); ++it)
        {
            const CTSVNPath& baseDirectory = it->GetDirectory();
            if(m_commonBaseDirectory.IsEmpty())
            {
                m_commonBaseDirectory = baseDirectory;
            }
            else if(!m_commonBaseDirectory.IsEquivalentTo(baseDirectory))
            {
                // Different path
                m_commonBaseDirectory.Reset();
                break;
            }
        }
    }
    // since we only checked strings, not paths,
    // we have to make sure now that we really return a *path* here
    PathVector::const_iterator iter;
    for(iter = m_paths.begin(); iter != m_paths.end(); ++iter)
    {
        if (!m_commonBaseDirectory.IsAncestorOf(*iter))
        {
            m_commonBaseDirectory = m_commonBaseDirectory.GetContainingDirectory();
            break;
        }
    }
    return m_commonBaseDirectory;
}

CTSVNPath CTSVNPathList::GetCommonRoot() const
{
    if (GetCount() == 0)
        return CTSVNPath();
    if (GetCount() == 1)
        return m_paths[0];

    // first entry is common root for itself
    // (add trailing '\\' to detect partial matches of the last path element)

    CString root = m_paths[0].GetWinPathString() + '\\';
    int rootLength = root.GetLength();

    // determine common path string prefix

    for ( PathVector::const_iterator it = m_paths.begin()+1
        ; it != m_paths.end()
        ; ++it)
    {
        CString path = it->GetWinPathString() + '\\';

        int newLength = CStringUtils::GetMatchingLength (root, path);
        if (newLength != rootLength)
        {
            root.Delete (newLength, rootLength);
            rootLength = newLength;
        }
    }

    // remove the last (partial) path element

    if (rootLength > 0)
        root.Delete (root.ReverseFind ('\\'), rootLength);

    // done

    return CTSVNPath (root);
}

void CTSVNPathList::SortByPathname(bool bReverse /*= false*/)
{
    std::sort(m_paths.begin(), m_paths.end());
    if (bReverse)
        std::reverse(m_paths.begin(), m_paths.end());
}

void CTSVNPathList::DeleteAllPaths(bool bTrash, bool bFilesOnly, HWND hErrorWnd)
{
    PathVector::const_iterator it;
    SortByPathname (true); // nested ones first

    CString sPaths;
    for (it = m_paths.begin(); it != m_paths.end(); ++it)
    {
        if ((it->Exists())&&((it->IsDirectory() != bFilesOnly)||!bFilesOnly))
        {
            if (!it->IsDirectory())
                ::SetFileAttributes(it->GetWinPath(), FILE_ATTRIBUTE_NORMAL);

            sPaths += it->GetWinPath();
            sPaths += '\0';
        }
    }
    sPaths += '\0';
    sPaths += '\0';
    DeleteViaShell((LPCTSTR)sPaths, bTrash, hErrorWnd);
    Clear();
}

void CTSVNPathList::RemoveDuplicates()
{
    SortByPathname();
    // Remove the duplicates
    // (Unique moves them to the end of the vector, then erase chops them off)
    m_paths.erase(std::unique(m_paths.begin(), m_paths.end(), &CTSVNPath::PredLeftEquivalentToRight), m_paths.end());
}

void CTSVNPathList::RemoveAdminPaths()
{
    PathVector::iterator it;
    for(it = m_paths.begin(); it != m_paths.end(); )
    {
        if (it->IsAdminDir())
        {
            m_paths.erase(it);
            it = m_paths.begin();
        }
        else
            ++it;
    }
}

void CTSVNPathList::RemovePath(const CTSVNPath& path)
{
    PathVector::iterator it;
    for(it = m_paths.begin(); it != m_paths.end(); ++it)
    {
        if (it->IsEquivalentTo(path))
        {
            m_paths.erase(it);
            return;
        }
    }
}

void CTSVNPathList::RemoveChildren()
{
    SortByPathname();
    m_paths.erase(std::unique(m_paths.begin(), m_paths.end(), &CTSVNPath::CheckChild), m_paths.end());
}

bool CTSVNPathList::IsEqual(const CTSVNPathList& list)
{
    if (list.GetCount() != GetCount())
        return false;
    for (int i=0; i<list.GetCount(); ++i)
    {
        if (!list[i].IsEquivalentTo(m_paths[i]))
            return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////

apr_array_header_t * CTSVNPathList::MakePathArray (apr_pool_t *pool) const
{
    apr_array_header_t *targets = apr_array_make (pool, GetCount(), sizeof(const char *));

    for(int nItem = 0; nItem < GetCount(); nItem++)
    {
        const char * target = m_paths[nItem].GetSVNApiPath(pool);
        (*((const char **) apr_array_push (targets))) = target;
    }

    return targets;
}

bool CTSVNPathList::DeleteViaShell(LPCTSTR path, bool bTrash, HWND hErrorWnd)
{
    SHFILEOPSTRUCT shop = {0};
    shop.hwnd = hErrorWnd;
    shop.wFunc = FO_DELETE;
    shop.pFrom = path;
    shop.fFlags = FOF_NOCONFIRMATION|FOF_SILENT|FOF_NO_CONNECTED_ELEMENTS;
    if (hErrorWnd == NULL)
        shop.fFlags |= FOF_NOERRORUI;
    if (bTrash)
        shop.fFlags |= FOF_ALLOWUNDO;
    const bool bRet = (SHFileOperation(&shop) == 0);
    return bRet;
}

//////////////////////////////////////////////////////////////////////////


#if defined(_DEBUG)
// Some test cases for these classes
static class CTSVNPathTests
{
public:
    CTSVNPathTests()
    {
        apr_initialize();
        pool = svn_pool_create(NULL);
        GetDirectoryTest();
        AdminDirTest();
        SortTest();
        RawAppendTest();
        PathAppendTest();
        RemoveDuplicatesTest();
        RemoveChildrenTest();
        ContainingDirectoryTest();
        AncestorTest();
        SubversionPathTest();
        GetCommonRootTest();
#if defined(_MFC_VER)
        ValidPathAndUrlTest();
        ListLoadingTest();
#endif
        apr_terminate();
    }

private:
    apr_pool_t * pool;
    void GetDirectoryTest()
    {
        // Bit tricky, this test, because we need to know something about the file
        // layout on the machine which is running the test
        TCHAR winDir[MAX_PATH + 1] = { 0 };
        GetWindowsDirectory(winDir, _countof(winDir));
        CString sWinDir(winDir);

        CTSVNPath testPath;
        // This is a file which we know will always be there
        testPath.SetFromUnknown(sWinDir + L"\\win.ini");
        ATLASSERT(!testPath.IsDirectory());
        ATLASSERT(testPath.GetDirectory().GetWinPathString() == sWinDir);
        ATLASSERT(testPath.GetContainingDirectory().GetWinPathString() == sWinDir);

        // Now do the test on the win directory itself - It's hard to be sure about the containing directory
        // but we know it must be different to the directory itself
        testPath.SetFromUnknown(sWinDir);
        ATLASSERT(testPath.IsDirectory());
        ATLASSERT(testPath.GetDirectory().GetWinPathString() == sWinDir);
        ATLASSERT(testPath.GetContainingDirectory().GetWinPathString() != sWinDir);
        ATLASSERT(testPath.GetContainingDirectory().GetWinPathString().GetLength() < sWinDir.GetLength());

        // Try a root path
        testPath.SetFromUnknown(L"C:\\");
        ATLASSERT(testPath.IsDirectory());
        ATLASSERT(testPath.GetDirectory().GetWinPathString().CompareNoCase(L"C:\\")==0);
        ATLASSERT(testPath.GetContainingDirectory().IsEmpty());
        // Try a root UNC path
        testPath.SetFromUnknown(L"\\MYSTATION");
        ATLASSERT(testPath.GetContainingDirectory().IsEmpty());

        // test the UI path methods
        testPath.SetFromUnknown(L"c:\\testing%20test");
        ATLASSERT(testPath.GetUIFileOrDirectoryName().CompareNoCase(L"testing%20test") == 0);
#ifdef _MFC_VER
        testPath.SetFromUnknown(L"http://server.com/testing%20special%20chars%20%c3%a4%c3%b6%c3%bc");
        ATLASSERT(testPath.GetUIFileOrDirectoryName().CompareNoCase(L"testing special chars \344\366\374") == 0);
#endif
    }

    void AdminDirTest()
    {
        CTSVNPath testPath;
        testPath.SetFromUnknown(L"c:\\.svndir");
        ATLASSERT(!testPath.IsAdminDir());
        testPath.SetFromUnknown(L"c:\\test.svn");
        ATLASSERT(!testPath.IsAdminDir());
        testPath.SetFromUnknown(L"c:\\.svn");
        ATLASSERT(testPath.IsAdminDir());
        testPath.SetFromUnknown(L"c:\\.svndir\\test");
        ATLASSERT(!testPath.IsAdminDir());
        testPath.SetFromUnknown(L"c:\\.svn\\test");
        ATLASSERT(testPath.IsAdminDir());

        CTSVNPathList pathList;
        pathList.AddPath(CTSVNPath(L"c:\\.svndir"));
        pathList.AddPath(CTSVNPath(L"c:\\.svn"));
        pathList.AddPath(CTSVNPath(L"c:\\.svn\\test"));
        pathList.AddPath(CTSVNPath(L"c:\\test"));
        pathList.RemoveAdminPaths();
        ATLASSERT(pathList.GetCount()==2);
        pathList.Clear();
        pathList.AddPath(CTSVNPath(L"c:\\test"));
        pathList.RemoveAdminPaths();
        ATLASSERT(pathList.GetCount()==1);
    }

    void SortTest()
    {
        CTSVNPathList testList;
        CTSVNPath testPath;
        testPath.SetFromUnknown(L"c:/Z");
        testList.AddPath(testPath);
        testPath.SetFromUnknown(L"c:/B");
        testList.AddPath(testPath);
        testPath.SetFromUnknown(L"c:\\a");
        testList.AddPath(testPath);
        testPath.SetFromUnknown(L"c:/Test");
        testList.AddPath(testPath);

        testList.SortByPathname();

        ATLASSERT(testList[0].GetWinPathString() == L"c:\\a");
        ATLASSERT(testList[1].GetWinPathString() == L"c:\\B");
        ATLASSERT(testList[2].GetWinPathString() == L"c:\\Test");
        ATLASSERT(testList[3].GetWinPathString() == L"c:\\Z");
    }

    void RawAppendTest()
    {
        CTSVNPath testPath(L"c:/test/");
        testPath.AppendRawString(L"/Hello");
        ATLASSERT(testPath.GetWinPathString() == L"c:\\test\\Hello");

        testPath.AppendRawString(L"\\T2");
        ATLASSERT(testPath.GetWinPathString() == L"c:\\test\\Hello\\T2");

        CTSVNPath testFilePath(L"C:\\windows\\win.ini");
        CTSVNPath testBasePath(L"c:/temp/myfile.txt");
        testBasePath.AppendRawString(testFilePath.GetFileExtension());
        ATLASSERT(testBasePath.GetWinPathString() == L"c:\\temp\\myfile.txt.ini");
    }

    void PathAppendTest()
    {
        CTSVNPath testPath(L"c:/test/");
        testPath.AppendPathString(L"/Hello");
        ATLASSERT(testPath.GetWinPathString() == L"c:\\test\\Hello");

        testPath.AppendPathString(L"T2");
        ATLASSERT(testPath.GetWinPathString() == L"c:\\test\\Hello\\T2");

        CTSVNPath testFilePath(L"C:\\windows\\win.ini");
        CTSVNPath testBasePath(L"c:/temp/myfile.txt");
        // You wouldn't want to do this in real life - you'd use append-raw
        testBasePath.AppendPathString(testFilePath.GetFileExtension());
        ATLASSERT(testBasePath.GetWinPathString() == L"c:\\temp\\myfile.txt\\.ini");
    }

    void RemoveDuplicatesTest()
    {
        CTSVNPathList list;
        list.AddPath(CTSVNPath(L"Z"));
        list.AddPath(CTSVNPath(L"A"));
        list.AddPath(CTSVNPath(L"E"));
        list.AddPath(CTSVNPath(L"E"));

        ATLASSERT(list[2].IsEquivalentTo(list[3]));
        ATLASSERT(list[2]==list[3]);

        ATLASSERT(list.GetCount() == 4);

        list.RemoveDuplicates();

        ATLASSERT(list.GetCount() == 3);

        ATLASSERT(list[0].GetWinPathString() == L"A");
        ATLASSERT(list[1].GetWinPathString().Compare(L"E") == 0);
        ATLASSERT(list[2].GetWinPathString() == L"Z");
    }

    void RemoveChildrenTest()
    {
        CTSVNPathList list;
        list.AddPath(CTSVNPath(L"c:\\test"));
        list.AddPath(CTSVNPath(L"c:\\test\\file"));
        list.AddPath(CTSVNPath(L"c:\\testfile"));
        list.AddPath(CTSVNPath(L"c:\\parent"));
        list.AddPath(CTSVNPath(L"c:\\parent\\child"));
        list.AddPath(CTSVNPath(L"c:\\parent\\child1"));
        list.AddPath(CTSVNPath(L"c:\\parent\\child2"));

        ATLASSERT(list.GetCount() == 7);

        list.RemoveChildren();

        ATLTRACE("count = %d\n", list.GetCount());
        ATLASSERT(list.GetCount() == 3);

        list.SortByPathname();

        ATLASSERT(list[0].GetWinPathString().Compare(L"c:\\parent") == 0);
        ATLASSERT(list[1].GetWinPathString().Compare(L"c:\\test") == 0);
        ATLASSERT(list[2].GetWinPathString().Compare(L"c:\\testfile") == 0);
    }

#if defined(_MFC_VER)
    void ListLoadingTest()
    {
        TCHAR buf[MAX_PATH] = { 0 };
        GetCurrentDirectory(_countof(buf), buf);
        CString sPathList(L"Path1*c:\\path2 with spaces and stuff*\\funnypath\\*");
        CTSVNPathList testList;
        testList.LoadFromAsteriskSeparatedString(sPathList);

        ATLASSERT(testList.GetCount() == 3);
        ATLASSERT(testList[0].GetWinPathString() == CString(buf) + L"\\Path1");
        ATLASSERT(testList[1].GetWinPathString() == L"c:\\path2 with spaces and stuff");
        ATLASSERT(testList[2].GetWinPathString() == L"\\funnypath");

        ATLASSERT(testList.GetCommonRoot().GetWinPathString().IsEmpty());
        sPathList = L"c:\\path2 with spaces and stuff*c:\\funnypath\\*";
        testList.LoadFromAsteriskSeparatedString(sPathList);
        ATLASSERT(testList.GetCommonRoot().GetWinPathString() == L"c:\\");
    }
#endif

    void ContainingDirectoryTest()
    {

        CTSVNPath testPath;
        testPath.SetFromWin(L"c:\\a\\b\\c\\d\\e");
        CTSVNPath dir;
        dir = testPath.GetContainingDirectory();
        ATLASSERT(dir.GetWinPathString() == L"c:\\a\\b\\c\\d");
        dir = dir.GetContainingDirectory();
        ATLASSERT(dir.GetWinPathString() == L"c:\\a\\b\\c");
        dir = dir.GetContainingDirectory();
        ATLASSERT(dir.GetWinPathString() == L"c:\\a\\b");
        dir = dir.GetContainingDirectory();
        ATLASSERT(dir.GetWinPathString() == L"c:\\a");
        dir = dir.GetContainingDirectory();
        ATLASSERT(dir.GetWinPathString() == L"c:\\");
        dir = dir.GetContainingDirectory();
        ATLASSERT(dir.IsEmpty());
        ATLASSERT(dir.GetWinPathString().IsEmpty());
    }

    void AncestorTest()
    {
        CTSVNPath testPath;
        testPath.SetFromWin(L"c:\\windows");
        ATLASSERT(testPath.IsAncestorOf(CTSVNPath(L"c:\\"))==false);
        ATLASSERT(testPath.IsAncestorOf(CTSVNPath(L"c:\\windows")));
        ATLASSERT(testPath.IsAncestorOf(CTSVNPath(L"c:\\windowsdummy"))==false);
        ATLASSERT(testPath.IsAncestorOf(CTSVNPath(L"c:\\windows\\test.txt")));
        ATLASSERT(testPath.IsAncestorOf(CTSVNPath(L"c:\\windows\\system32\\test.txt")));
    }

    void SubversionPathTest()
    {
        CTSVNPath testPath;
        testPath.SetFromWin(L"c:\\");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "C:/") == 0);
        testPath.SetFromWin(L"c:\\folder");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "C:/folder") == 0);
        testPath.SetFromWin(L"c:\\a\\b\\c\\d\\e");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "C:/a/b/c/d/e") == 0);
        testPath.SetFromUnknown(L"http://testing/");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing") == 0);
        testPath.SetFromSVN(NULL);
        ATLASSERT(strlen(testPath.GetSVNApiPath(pool))==0);
        testPath.SetFromWin(L"\\\\a\\b\\c\\d\\e");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "//a/b/c/d/e") == 0);
        testPath.SetFromWin(L"\\\\?\\C:\\Windows");
        ATLASSERT(wcscmp(testPath.GetWinPath(), L"C:\\Windows")==0);
        testPath.SetFromUnknown(L"\\\\?\\C:\\Windows");
        ATLASSERT(wcscmp(testPath.GetWinPath(), L"C:\\Windows")==0);
#if defined(_MFC_VER)
        testPath.SetFromUnknown(L"http://testing again");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing%20again") == 0);
        testPath.SetFromUnknown(L"http://testing%20again");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing%20again") == 0);
        testPath.SetFromUnknown(L"http://testing special chars \344\366\374");
        ATLASSERT(strcmp(testPath.GetSVNApiPath(pool), "http://testing%20special%20chars%20%c3%a4%c3%b6%c3%bc") == 0);
#endif
    }

    void GetCommonRootTest()
    {
        CTSVNPath pathA (L"C:\\Development\\LogDlg.cpp");
        CTSVNPath pathB (L"C:\\Development\\LogDlg.h");
        CTSVNPath pathC (L"C:\\Development\\SomeDir\\LogDlg.h");

        CTSVNPathList list;
        list.AddPath(pathA);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development\\LogDlg.cpp")==0);
        list.AddPath(pathB);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development")==0);
        list.AddPath(pathC);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development")==0);
#ifdef _MFC_VER
        CString sPathList = L"D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup64.wxs*D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup64.wxs";
        list.LoadFromAsteriskSeparatedString(sPathList);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"D:\\Development\\StExBar")==0);

        sPathList = L"c:\\windows\\explorer.exe*c:\\windows";
        list.LoadFromAsteriskSeparatedString(sPathList);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows")==0);

        sPathList = L"c:\\windows\\*c:\\windows";
        list.LoadFromAsteriskSeparatedString(sPathList);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows")==0);

        sPathList = L"c:\\windows\\system32*c:\\windows\\system";
        list.LoadFromAsteriskSeparatedString(sPathList);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows")==0);

        sPathList = L"c:\\windowsdummy*c:\\windows";
        list.LoadFromAsteriskSeparatedString(sPathList);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\")==0);

        sPathList = L"https://svn.test.com/appidgd3fbn16y8*https://svn.test.com/appid";
        list.LoadFromAsteriskSeparatedString(sPathList);
        ATLASSERT(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"https:\\\\svn.test.com")==0);
#endif
    }

    void ValidPathAndUrlTest()
    {
        CTSVNPath testPath;
        testPath.SetFromWin(L"c:\\a\\b\\c.test.txt");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"D:\\.Net\\SpindleSearch\\");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\test folder\\file");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\folder\\");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\ext.ext.ext\\ext.ext.ext.ext");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\.svn");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\com\\file");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\test\\conf");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\LPT");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\test\\LPT");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\com1test");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"\\\\?\\c:\\test\\com1test");
        ATLASSERT(testPath.IsValidOnWindows());

        testPath.SetFromWin(L"\\\\Share\\filename");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"\\\\Share\\filename.extension");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromWin(L"\\\\Share\\.svn");
        ATLASSERT(testPath.IsValidOnWindows());

        // now the negative tests
        testPath.SetFromWin(L"c:\\test:folder");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\file<name");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\something*else");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\folder\\file?nofile");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\ext.>ension");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\com1\\filename");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\com1");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"c:\\com1\\AuX");
        ATLASSERT(!testPath.IsValidOnWindows());

        testPath.SetFromWin(L"\\\\Share\\lpt9\\filename");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"\\\\Share\\prn");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromWin(L"\\\\Share\\NUL");
        ATLASSERT(!testPath.IsValidOnWindows());

        // now come some URL tests
        testPath.SetFromSVN(L"http://myserver.com/repos/trunk");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"https://myserver.com/repos/trunk/file%20with%20spaces");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"svn://myserver.com/repos/trunk/file with spaces");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"svn+ssh://www.myserver.com/repos/trunk");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"http://localhost:90/repos/trunk");
        ATLASSERT(testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"file:///C:/SVNRepos/Tester/Proj1/tags/t2");
        ATLASSERT(testPath.IsValidOnWindows());
        // and some negative URL tests
        testPath.SetFromSVN(L"https://myserver.com/rep:os/trunk/file%20with%20spaces");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"svn://myserver.com/rep<os/trunk/file with spaces");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"svn+ssh://www.myserver.com/repos/trunk/prn/");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"http://localhost:90/repos/trunk/com1");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"http://localhost:90/repos/trunk/Blame3-%3Eblame.cpp");
        ATLASSERT(!testPath.IsValidOnWindows());
        testPath.SetFromSVN(L"");
        ATLASSERT(!testPath.IsUrl());
    }

} TSVNPathTestobject;
#endif

