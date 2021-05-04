// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2014, 2016, 2021 - TortoiseSVN

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
#include "stdafx.h"
#include "SVNDataObject.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "TempFile.h"
#include "StringUtils.h"
#include <strsafe.h>

CLIPFORMAT CF_FILECONTENTS          = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILECONTENTS));
CLIPFORMAT CF_FILEDESCRIPTOR        = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR));
CLIPFORMAT CF_PREFERREDDROPEFFECT   = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT));
CLIPFORMAT CF_SVNURL                = static_cast<CLIPFORMAT>(RegisterClipboardFormat(L"TSVN_SVNURL"));
CLIPFORMAT CF_INETURL               = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_INETURL));
CLIPFORMAT CF_SHELLURL              = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_SHELLURL));
CLIPFORMAT CF_FILE_ATTRIBUTES_ARRAY = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILE_ATTRIBUTES_ARRAY));

SVNDataObject::SVNDataObject(const CTSVNPathList& svnpaths, SVNRev peg, SVNRev rev, bool bFilesAsUrlLinks)
    : m_svnPaths(svnpaths)
    , m_pegRev(peg)
    , m_revision(rev)
    , m_bFilesAsUrlLinks(bFilesAsUrlLinks)
    , m_cRefCount(0)
    , m_bInOperation(FALSE)
    , m_bIsAsync(TRUE)
{
}

SVNDataObject::~SVNDataObject()
{
    for (size_t i = 0; i < m_vecStgMedium.size(); ++i)
    {
        ReleaseStgMedium(m_vecStgMedium[i]);
        delete m_vecStgMedium[i];
    }

    for (size_t j = 0; j < m_vecFormatEtc.size(); ++j)
        delete m_vecFormatEtc[j];
}

//////////////////////////////////////////////////////////////////////////
// IUnknown
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP SVNDataObject::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
        return E_POINTER;
    *ppvObject = nullptr;
    if (IsEqualIID(IID_IUnknown, riid) || IsEqualIID(IID_IDataObject, riid))
        *ppvObject = static_cast<IDataObject*>(this);
    else if (IsEqualIID(riid, IID_IDataObjectAsyncCapability))
        *ppvObject = static_cast<IDataObjectAsyncCapability*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
SVNDataObject::AddRef()
{
    return ++m_cRefCount;
}

STDMETHODIMP_(ULONG)
SVNDataObject::Release()
{
    --m_cRefCount;
    if (m_cRefCount == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefCount;
}

//////////////////////////////////////////////////////////////////////////
// IDataObject
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP SVNDataObject::GetData(FORMATETC* pformatetcIn, STGMEDIUM* pMedium)
{
    if (pformatetcIn == nullptr)
        return E_INVALIDARG;
    if (pMedium == nullptr)
        return E_POINTER;
    pMedium->hGlobal = nullptr;

    if ((pformatetcIn->tymed & TYMED_ISTREAM) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILECONTENTS))
    {
        // supports the IStream format.
        // The lindex param is the index of the file to return
        CTSVNPath filePath;
        IStream*  pIStream = nullptr;

        if (m_bFilesAsUrlLinks)
        {
            if ((pformatetcIn->lindex >= 0) && (pformatetcIn->lindex < static_cast<LONG>(m_allPaths.size())))
            {
                filePath = CTempFiles::Instance().GetTempFilePath(true);
                CString sTemp;
                sTemp.Format(L"[InternetShortcut]\nURL=%s\n", static_cast<LPCWSTR>(m_allPaths[pformatetcIn->lindex].infoData.url));
                CStringUtils::WriteStringToTextFile(filePath.GetWinPath(), static_cast<LPCWSTR>(sTemp));
            }
            else
                return E_INVALIDARG;
        }
        else
        {
            // Note: we don't get called for directories since those are simply created and don't
            // need to be fetched.

            // Note2: It would be really nice if we could get a stream from the subversion library
            // from where we could read the file contents. But the Subversion lib is not implemented
            // to *read* from a remote stream but so that the library *writes* to a stream we pass.
            // Since we can't get such a read stream, we have to fetch the file in whole first to
            // a temp location and then pass the shell an IStream for that temp file.
            if (m_revision.IsWorking())
            {
                if ((pformatetcIn->lindex >= 0) && (pformatetcIn->lindex < static_cast<LONG>(m_allPaths.size())))
                {
                    filePath = m_allPaths[pformatetcIn->lindex].rootPath;
                }
            }
            else
            {
                filePath = CTempFiles::Instance().GetTempFilePath(true);
                if ((pformatetcIn->lindex >= 0) && (pformatetcIn->lindex < static_cast<LONG>(m_allPaths.size())))
                {
                    if (!m_svn.Export(CTSVNPath(m_allPaths[pformatetcIn->lindex].infoData.url), filePath, m_pegRev, m_revision))
                    {
                        DeleteFile(filePath.GetWinPath());
                        return STG_E_ACCESSDENIED;
                    }
                }
            }
        }
        HRESULT res = SHCreateStreamOnFileEx(filePath.GetWinPath(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pIStream);
        if (res == S_OK)
        {
            // https://devblogs.microsoft.com/oldnewthing/20140918-00/?p=44033
            LARGE_INTEGER liZero = {0, 0};
            pIStream->Seek(liZero, STREAM_SEEK_END, nullptr);

            pMedium->pstm  = pIStream;
            pMedium->tymed = TYMED_ISTREAM;
            return S_OK;
        }
        return res;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILEDESCRIPTOR))
    {
        if (m_bFilesAsUrlLinks)
        {
            for (int i = 0; i < m_svnPaths.GetCount(); ++i)
            {
                SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], SVNInfoData()};
                id.infoData.url                     = m_svnPaths[i].GetSVNPathString();
                m_allPaths.push_back(id);
            }
        }
        else
        {
            // now it is time to get all sub folders for the directories we have
            SVNInfo svnInfo;
            // find the common directory of all the paths
            CTSVNPath commonDir;
            for (int i = 0; i < m_svnPaths.GetCount(); ++i)
            {
                if (commonDir.IsEmpty())
                    commonDir = m_svnPaths[i].GetContainingDirectory();
                if (!commonDir.IsEquivalentTo(m_svnPaths[i].GetContainingDirectory()))
                {
                    commonDir.Reset();
                    break;
                }
            }
            for (int i = 0; i < m_svnPaths.GetCount(); ++i)
            {
                if (m_svnPaths[i].IsUrl())
                {
                    const SVNInfoData* infoData = svnInfo.GetFirstFileInfo(m_svnPaths[i], m_pegRev, m_revision, svn_depth_infinity);
                    while (infoData)
                    {
                        SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], *infoData};
                        m_allPaths.push_back(id);
                        infoData = svnInfo.GetNextFileInfo();
                    }
                }
                else
                {
                    SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], SVNInfoData()};
                    m_allPaths.push_back(id);
                }
            }
        }
        size_t  dataSize = sizeof(FILEGROUPDESCRIPTOR) + ((m_allPaths.size() - 1) * sizeof(FILEDESCRIPTOR));
        HGLOBAL data     = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, dataSize);
        if (!data)
            return E_OUTOFMEMORY;
        FILEGROUPDESCRIPTOR* files = static_cast<FILEGROUPDESCRIPTORW*>(GlobalLock(data));
        if (!files)
        {
            GlobalFree(data);
            return E_OUTOFMEMORY;
        }
        files->cItems = static_cast<UINT>(m_allPaths.size());
        int index     = 0;
        for (std::vector<SVNDataObject::SVNObjectInfoData>::const_iterator it = m_allPaths.begin(); it != m_allPaths.end(); ++it)
        {
            CString temp;
            if (it->rootPath.IsUrl())
            {
                temp = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(it->rootPath.GetContainingDirectory().GetSVNPathString())));
                temp = it->infoData.url.Mid(temp.GetLength() + 1);
                // we have to unescape the urls since the local file system doesn't need them
                // escaped and it would only look really ugly (and be wrong).
                temp = CPathUtils::PathUnescape(temp);
                if (m_bFilesAsUrlLinks)
                    temp += L".url";
                temp.Replace(L"/", L"\\");
            }
            else
            {
                temp = it->rootPath.GetUIFileOrDirectoryName();
                if (m_bFilesAsUrlLinks)
                    temp += L".url";
            }
            if (temp.GetLength() < MAX_PATH)
                wcscpy_s(files->fgd[index].cFileName, static_cast<LPCWSTR>(temp));
            else
            {
                files->cItems--;
                continue;
            }
            files->fgd[index].dwFlags = FD_ATTRIBUTES | FD_PROGRESSUI | FD_FILESIZE | FD_LINKUI;
            if (it->rootPath.IsUrl())
            {
                if (m_bFilesAsUrlLinks)
                    files->fgd[index].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                else
                    files->fgd[index].dwFileAttributes = (it->infoData.kind == svn_node_dir) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

                // convert time to FILETIME
                LONGLONG ll;
#define APR_DELTA_EPOCH_IN_USEC APR_TIME_C(11644473600000000);
                ll = it->infoData.lastChangedTime * 1000000L;               // create apr_time
                ll += APR_DELTA_EPOCH_IN_USEC;                              // add apr_time offset required
                ll                                               = ll * 10; // to get the FILETIME
                files->fgd[index].ftLastWriteTime.dwLowDateTime  = static_cast<DWORD>(ll);
                files->fgd[index].ftLastWriteTime.dwHighDateTime = static_cast<DWORD>(ll >> 32);
                files->fgd[index].dwFlags |= FD_WRITESTIME;
            }
            else
            {
                if (m_bFilesAsUrlLinks)
                    files->fgd[index].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                else
                    files->fgd[index].dwFileAttributes = it->rootPath.IsDirectory() ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            }
            // Always set the file size to 0 even if we 'know' the file size (infoData.size64).
            // Because for text files, the file size is too low due to the EOLs getting converted
            // to CRLF (from LF as stored in the repository). And a too low file size set here
            // prevents the shell from reading the full file later - it only reads the stream up
            // to the number of bytes specified here. Which means we would end up with a truncated
            // text file (binary files are still ok though).
            files->fgd[index].nFileSizeLow  = 0;
            files->fgd[index].nFileSizeHigh = 0;

            ++index;
        }

        GlobalUnlock(data);

        pMedium->hGlobal = data;
        pMedium->tymed   = TYMED_HGLOBAL;
        return S_OK;
    }
    // handling CF_PREFERREDDROPEFFECT is necessary to tell the shell that it should *not* ask for the
    // CF_FILEDESCRIPTOR until the drop actually occurs. If we don't handle CF_PREFERREDDROPEFFECT, the shell
    // will ask for the file descriptor for every object (file/folder) the mouse pointer hovers over and is
    // a potential drop target.
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->cfFormat == CF_PREFERREDDROPEFFECT))
    {
        HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, sizeof(DWORD));
        if (!data)
            return E_OUTOFMEMORY;
        DWORD* effect = static_cast<DWORD*>(GlobalLock(data));
        if (!effect)
        {
            GlobalFree(data);
            return E_OUTOFMEMORY;
        }
        if (m_bFilesAsUrlLinks)
            (*effect) = DROPEFFECT_LINK;
        else
            (*effect) = DROPEFFECT_COPY;
        GlobalUnlock(data);
        pMedium->hGlobal = data;
        pMedium->tymed   = TYMED_HGLOBAL;
        return S_OK;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_TEXT))
    {
        // caller wants text
        // create the string from the path list
        CString text;
        if (m_svnPaths.GetCount())
        {
            // create a single string where the URLs are separated by newlines
            for (int i = 0; i < m_svnPaths.GetCount(); ++i)
            {
                if (m_svnPaths[i].IsUrl())
                    text += m_svnPaths[i].GetSVNPathString();
                else
                    text += m_svnPaths[i].GetWinPathString();
                text += L"\r\n";
            }
        }
        CStringA texta   = CUnicodeUtils::GetUTF8(text);
        pMedium->tymed   = TYMED_HGLOBAL;
        pMedium->hGlobal = GlobalAlloc(GHND, (texta.GetLength() + 1) * sizeof(char));
        if (pMedium->hGlobal)
        {
            char* pMem = static_cast<char*>(GlobalLock(pMedium->hGlobal));
            strcpy_s(pMem, texta.GetLength() + 1, static_cast<LPCSTR>(texta));
            GlobalUnlock(pMedium->hGlobal);
        }
        pMedium->pUnkForRelease = nullptr;
        return S_OK;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && ((pformatetcIn->cfFormat == CF_UNICODETEXT) || (pformatetcIn->cfFormat == CF_INETURL) || (pformatetcIn->cfFormat == CF_SHELLURL)))
    {
        // caller wants Unicode text
        // create the string from the path list
        CString text;
        if (m_svnPaths.GetCount())
        {
            // create a single string where the URLs are separated by newlines
            for (int i = 0; i < m_svnPaths.GetCount(); ++i)
            {
                if (m_svnPaths[i].IsUrl())
                    text += m_svnPaths[i].GetSVNPathString();
                else
                    text += m_svnPaths[i].GetWinPathString();
                text += L"\r\n";
            }
        }
        pMedium->tymed   = TYMED_HGLOBAL;
        pMedium->hGlobal = GlobalAlloc(GHND, (text.GetLength() + 1) * sizeof(wchar_t));
        if (pMedium->hGlobal)
        {
            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(pMedium->hGlobal));
            wcscpy_s(pMem, text.GetLength() + 1, static_cast<LPCWSTR>(text));
            GlobalUnlock(pMedium->hGlobal);
        }
        pMedium->pUnkForRelease = nullptr;
        return S_OK;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_SVNURL))
    {
        // caller wants the svn url
        // create the string from the path list
        CString text;
        if (m_svnPaths.GetCount())
        {
            // create a single string where the URLs are separated by newlines
            for (int i = 0; i < m_svnPaths.GetCount(); ++i)
            {
                if (m_svnPaths[i].IsUrl())
                {
                    text += m_svnPaths[i].GetSVNPathString();
                    text += L"?";
                    text += m_revision.ToString();
                }
                else
                    text += m_svnPaths[i].GetWinPathString();
                text += L"\r\n";
            }
        }
        pMedium->tymed   = TYMED_HGLOBAL;
        pMedium->hGlobal = GlobalAlloc(GHND, (text.GetLength() + 1) * sizeof(wchar_t));
        if (pMedium->hGlobal)
        {
            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(pMedium->hGlobal));
            wcscpy_s(pMem, text.GetLength() + 1, static_cast<LPCWSTR>(text));
            GlobalUnlock(pMedium->hGlobal);
        }
        pMedium->pUnkForRelease = nullptr;
        return S_OK;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_HDROP))
    {
        int nLength = 0;

        for (int i = 0; i < m_svnPaths.GetCount(); i++)
        {
            nLength += m_svnPaths[i].GetWinPathString().GetLength();
            nLength += 1; // '\0' separator
        }

        int  nBufferSize = sizeof(DROPFILES) + (nLength + 1LL) * sizeof(wchar_t);
        auto pBuffer     = std::make_unique<char[]>(nBufferSize);

        SecureZeroMemory(pBuffer.get(), nBufferSize);

        DROPFILES* df = reinterpret_cast<DROPFILES*>(pBuffer.get());
        df->pFiles    = sizeof(DROPFILES);
        df->fWide     = 1;

        wchar_t* pFilenames       = reinterpret_cast<wchar_t*>(pBuffer.get() + sizeof(DROPFILES));
        wchar_t* pCurrentFilename = pFilenames;

        for (int i = 0; i < m_svnPaths.GetCount(); i++)
        {
            CString str = m_svnPaths[i].GetWinPathString();
            wcscpy_s(pCurrentFilename, str.GetLength() + 1, static_cast<LPCWSTR>(str));
            pCurrentFilename += str.GetLength();
            *pCurrentFilename = '\0'; // separator between file names
            pCurrentFilename++;
        }
        *pCurrentFilename = '\0'; // terminate array

        pMedium->tymed   = TYMED_HGLOBAL;
        pMedium->hGlobal = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, nBufferSize);
        if (pMedium->hGlobal)
        {
            LPVOID pMem = ::GlobalLock(pMedium->hGlobal);
            if (pMem)
                memcpy(pMem, pBuffer.get(), nBufferSize);
            GlobalUnlock(pMedium->hGlobal);
        }
        pMedium->pUnkForRelease = nullptr;
        return S_OK;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILE_ATTRIBUTES_ARRAY))
    {
        int  nBufferSize = sizeof(FILE_ATTRIBUTES_ARRAY) + m_svnPaths.GetCount() * sizeof(DWORD);
        auto pBuffer     = std::make_unique<char[]>(nBufferSize);

        SecureZeroMemory(pBuffer.get(), nBufferSize);

        FILE_ATTRIBUTES_ARRAY* cf   = reinterpret_cast<FILE_ATTRIBUTES_ARRAY*>(pBuffer.get());
        cf->cItems                  = m_svnPaths.GetCount();
        cf->dwProductFileAttributes = DWORD_MAX;
        cf->dwSumFileAttributes     = 0;
        for (int i = 0; i < m_svnPaths.GetCount(); i++)
        {
            DWORD fileAttribs         = m_svnPaths[i].IsUrl() ? FILE_ATTRIBUTE_NORMAL : m_svnPaths[i].GetFileAttributes();
            cf->rgdwFileAttributes[i] = fileAttribs;
            cf->dwProductFileAttributes &= fileAttribs;
            cf->dwSumFileAttributes |= fileAttribs;
        }

        pMedium->tymed   = TYMED_HGLOBAL;
        pMedium->hGlobal = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, nBufferSize);
        if (pMedium->hGlobal)
        {
            LPVOID pMem = ::GlobalLock(pMedium->hGlobal);
            if (pMem)
                memcpy(pMem, pBuffer.get(), nBufferSize);
            GlobalUnlock(pMedium->hGlobal);
        }
        pMedium->pUnkForRelease = nullptr;
        return S_OK;
    }

    for (size_t i = 0; i < m_vecFormatEtc.size(); ++i)
    {
        if ((pformatetcIn->tymed == m_vecFormatEtc[i]->tymed) &&
            (pformatetcIn->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
            (pformatetcIn->cfFormat == m_vecFormatEtc[i]->cfFormat))
        {
            CopyMedium(pMedium, m_vecStgMedium[i], m_vecFormatEtc[i]);
            return S_OK;
        }
    }

    return DV_E_FORMATETC;
}

STDMETHODIMP SVNDataObject::GetDataHere(FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP SVNDataObject::QueryGetData(FORMATETC* pFormatEtc)
{
    if (pFormatEtc == nullptr)
        return E_INVALIDARG;

    if (!(DVASPECT_CONTENT & pFormatEtc->dwAspect))
        return DV_E_DVASPECT;

    if ((pFormatEtc->tymed & TYMED_ISTREAM) &&
        (pFormatEtc->dwAspect == DVASPECT_CONTENT) &&
        (pFormatEtc->cfFormat == CF_FILECONTENTS))
    {
        return S_OK;
    }
    if ((pFormatEtc->tymed & TYMED_HGLOBAL) &&
        (pFormatEtc->dwAspect == DVASPECT_CONTENT) &&
        ((pFormatEtc->cfFormat == CF_TEXT) || (pFormatEtc->cfFormat == CF_UNICODETEXT) || (pFormatEtc->cfFormat == CF_INETURL) || (pFormatEtc->cfFormat == CF_SHELLURL) || (pFormatEtc->cfFormat == CF_PREFERREDDROPEFFECT)))
    {
        return S_OK;
    }
    if ((pFormatEtc->tymed & TYMED_HGLOBAL) &&
        (pFormatEtc->dwAspect == DVASPECT_CONTENT) &&
        ((pFormatEtc->cfFormat == CF_SVNURL) || (pFormatEtc->cfFormat == CF_FILEDESCRIPTOR)) &&
        !m_revision.IsWorking())
    {
        return S_OK;
    }
    if ((pFormatEtc->tymed & TYMED_HGLOBAL) &&
        (pFormatEtc->dwAspect == DVASPECT_CONTENT) &&
        (pFormatEtc->cfFormat == CF_HDROP) &&
        m_revision.IsWorking())
    {
        return S_OK;
    }
    if ((pFormatEtc->tymed & TYMED_HGLOBAL) &&
        (pFormatEtc->dwAspect == DVASPECT_CONTENT) &&
        (pFormatEtc->cfFormat == CF_FILE_ATTRIBUTES_ARRAY))
    {
        return S_OK;
    }

    for (size_t i = 0; i < m_vecFormatEtc.size(); ++i)
    {
        if ((pFormatEtc->tymed == m_vecFormatEtc[i]->tymed) &&
            (pFormatEtc->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
            (pFormatEtc->cfFormat == m_vecFormatEtc[i]->cfFormat))
            return S_OK;
    }

    return DV_E_TYMED;
}

STDMETHODIMP SVNDataObject::GetCanonicalFormatEtc(FORMATETC* /*pformatectIn*/, FORMATETC* pFormatEtcOut)
{
    if (pFormatEtcOut == nullptr)
        return E_INVALIDARG;
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP SVNDataObject::SetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium, BOOL fRelease)
{
    if ((pFormatEtc == nullptr) || (pMedium == nullptr))
        return E_INVALIDARG;

    FORMATETC* fEtc    = new (std::nothrow) FORMATETC;
    STGMEDIUM* pStgMed = new (std::nothrow) STGMEDIUM;

    if ((fEtc == nullptr) || (pStgMed == nullptr))
    {
        delete fEtc;
        delete pStgMed;
        return E_OUTOFMEMORY;
    }
    SecureZeroMemory(fEtc, sizeof(FORMATETC));
    SecureZeroMemory(pStgMed, sizeof(STGMEDIUM));

    // do we already store this format?
    for (size_t i = 0; i < m_vecFormatEtc.size(); ++i)
    {
        if ((pFormatEtc->tymed == m_vecFormatEtc[i]->tymed) &&
            (pFormatEtc->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
            (pFormatEtc->cfFormat == m_vecFormatEtc[i]->cfFormat))
        {
            // yes, this format is already in our object:
            // we have to replace the existing format. To do that, we
            // remove the format we already have
            delete m_vecFormatEtc[i];
            m_vecFormatEtc.erase(m_vecFormatEtc.begin() + i);
            ReleaseStgMedium(m_vecStgMedium[i]);
            delete m_vecStgMedium[i];
            m_vecStgMedium.erase(m_vecStgMedium.begin() + i);
            break;
        }
    }

    *fEtc = *pFormatEtc;
    m_vecFormatEtc.push_back(fEtc);

    if (fRelease)
        *pStgMed = *pMedium;
    else
    {
        CopyMedium(pStgMed, pMedium, pFormatEtc);
    }
    m_vecStgMedium.push_back(pStgMed);

    return S_OK;
}

STDMETHODIMP SVNDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc)
{
    if (ppEnumFormatEtc == nullptr)
        return E_POINTER;

    *ppEnumFormatEtc = nullptr;
    switch (dwDirection)
    {
        case DATADIR_GET:
            *ppEnumFormatEtc = new (std::nothrow) CSVNEnumFormatEtc(m_vecFormatEtc, !!m_revision.IsWorking());
            if (*ppEnumFormatEtc == nullptr)
                return E_OUTOFMEMORY;
            (*ppEnumFormatEtc)->AddRef();
            break;
        default:
            return E_NOTIMPL;
    }
    return S_OK;
}

STDMETHODIMP SVNDataObject::DAdvise(FORMATETC* /*pformatetc*/, DWORD /*advf*/, IAdviseSink* /*pAdvSink*/, DWORD* /*pdwConnection*/)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP SVNDataObject::DUnadvise(DWORD /*dwConnection*/)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::EnumDAdvise(IEnumSTATDATA** /*ppenumAdvise*/)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

void SVNDataObject::CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
{
    switch (pMedSrc->tymed)
    {
        case TYMED_HGLOBAL:
            pMedDest->hGlobal = static_cast<HGLOBAL>(OleDuplicateData(pMedSrc->hGlobal, pFmtSrc->cfFormat, NULL));
            break;
        case TYMED_GDI:
            pMedDest->hBitmap = static_cast<HBITMAP>(OleDuplicateData(pMedSrc->hBitmap, pFmtSrc->cfFormat, NULL));
            break;
        case TYMED_MFPICT:
            pMedDest->hMetaFilePict = static_cast<HMETAFILEPICT>(OleDuplicateData(pMedSrc->hMetaFilePict, pFmtSrc->cfFormat, NULL));
            break;
        case TYMED_ENHMF:
            pMedDest->hEnhMetaFile = static_cast<HENHMETAFILE>(OleDuplicateData(pMedSrc->hEnhMetaFile, pFmtSrc->cfFormat, NULL));
            break;
        case TYMED_FILE:
            pMedSrc->lpszFileName = static_cast<LPOLESTR>(OleDuplicateData(pMedSrc->lpszFileName, pFmtSrc->cfFormat, NULL));
            break;
        case TYMED_ISTREAM:
            pMedDest->pstm = pMedSrc->pstm;
            pMedSrc->pstm->AddRef();
            break;
        case TYMED_ISTORAGE:
            pMedDest->pstg = pMedSrc->pstg;
            pMedSrc->pstg->AddRef();
            break;
        case TYMED_NULL:
        default:
            break;
    }
    pMedDest->tymed          = pMedSrc->tymed;
    pMedDest->pUnkForRelease = nullptr;
    if (pMedSrc->pUnkForRelease != nullptr)
    {
        pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
        pMedSrc->pUnkForRelease->AddRef();
    }
}

//////////////////////////////////////////////////////////////////////////
// IAsyncOperation
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE SVNDataObject::SetAsyncMode(BOOL fDoOpAsync)
{
    m_bIsAsync = fDoOpAsync;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::GetAsyncMode(BOOL* pfIsOpAsync)
{
    if (!pfIsOpAsync)
        return E_FAIL;

    *pfIsOpAsync = m_bIsAsync;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::StartOperation(IBindCtx* /*pbcReserved*/)
{
    m_bInOperation = TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::InOperation(BOOL* pfInAsyncOp)
{
    if (!pfInAsyncOp)
        return E_FAIL;

    *pfInAsyncOp = m_bInOperation;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE SVNDataObject::EndOperation(HRESULT /*hResult*/, IBindCtx* /*pbcReserved*/, DWORD /*dwEffects*/)
{
    m_bInOperation = FALSE;
    return S_OK;
}

HRESULT SVNDataObject::SetDropDescription(DROPIMAGETYPE image, LPCWSTR format, LPCWSTR insert)
{
    if (format == nullptr || insert == nullptr)
        return E_INVALIDARG;

    FORMATETC fEtc = {0};
    fEtc.cfFormat  = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DROPDESCRIPTION));
    fEtc.dwAspect  = DVASPECT_CONTENT;
    fEtc.lindex    = -1;
    fEtc.tymed     = TYMED_HGLOBAL;

    STGMEDIUM medium = {0};
    medium.hGlobal   = GlobalAlloc(GHND, sizeof(DROPDESCRIPTION));
    if (medium.hGlobal == nullptr)
        return E_OUTOFMEMORY;

    DROPDESCRIPTION* pDropDescription = static_cast<DROPDESCRIPTION*>(GlobalLock(medium.hGlobal));
    if (pDropDescription == nullptr)
        return E_FAIL;
    StringCchCopy(pDropDescription->szInsert, _countof(pDropDescription->szInsert), insert);
    StringCchCopy(pDropDescription->szMessage, _countof(pDropDescription->szMessage), format);
    pDropDescription->type = image;
    GlobalUnlock(medium.hGlobal);
    return SetData(&fEtc, &medium, TRUE);
}

void CSVNEnumFormatEtc::Init(bool localonly)
{
    int index                 = 0;
    m_formats[index].cfFormat = CF_UNICODETEXT;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex   = -1;
    m_formats[index].ptd      = nullptr;
    m_formats[index].tymed    = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_TEXT;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex   = -1;
    m_formats[index].ptd      = nullptr;
    m_formats[index].tymed    = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_PREFERREDDROPEFFECT;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex   = -1;
    m_formats[index].ptd      = nullptr;
    m_formats[index].tymed    = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_INETURL;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex   = -1;
    m_formats[index].ptd      = nullptr;
    m_formats[index].tymed    = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_SHELLURL;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex   = -1;
    m_formats[index].ptd      = nullptr;
    m_formats[index].tymed    = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_FILE_ATTRIBUTES_ARRAY;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex   = -1;
    m_formats[index].ptd      = nullptr;
    m_formats[index].tymed    = TYMED_HGLOBAL;
    index++;

    if (localonly)
    {
        m_formats[index].cfFormat = CF_HDROP;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex   = -1;
        m_formats[index].ptd      = nullptr;
        m_formats[index].tymed    = TYMED_HGLOBAL;
        index++;
    }
    else
    {
        m_formats[index].cfFormat = CF_FILECONTENTS;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex   = -1;
        m_formats[index].ptd      = nullptr;
        m_formats[index].tymed    = TYMED_ISTREAM;
        index++;

        m_formats[index].cfFormat = CF_FILEDESCRIPTOR;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex   = -1;
        m_formats[index].ptd      = nullptr;
        m_formats[index].tymed    = TYMED_HGLOBAL;
        index++;

        m_formats[index].cfFormat = CF_SVNURL;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex   = -1;
        m_formats[index].ptd      = nullptr;
        m_formats[index].tymed    = TYMED_HGLOBAL;
        index++;
    }
    // clear possible leftovers
    while (index < SVNDATAOBJECT_NUMFORMATS)
    {
        m_formats[index].cfFormat = 0;
        m_formats[index].dwAspect = 0;
        m_formats[index].lindex   = -1;
        m_formats[index].ptd      = nullptr;
        m_formats[index].tymed    = 0;
        index++;
    }
}

CSVNEnumFormatEtc::CSVNEnumFormatEtc(const std::vector<FORMATETC>& vec, bool localOnly)
    : m_cRefCount(0)
    , m_iCur(0)
{
    m_localOnly = localOnly;
    for (size_t i = 0; i < vec.size(); ++i)
        m_vecFormatEtc.push_back(vec[i]);
    Init(localOnly);
}

CSVNEnumFormatEtc::CSVNEnumFormatEtc(const std::vector<FORMATETC*>& vec, bool localOnly)
    : m_cRefCount(0)
    , m_iCur(0)
{
    m_localOnly = localOnly;
    for (size_t i = 0; i < vec.size(); ++i)
        m_vecFormatEtc.push_back(*vec[i]);
    Init(localOnly);
}

STDMETHODIMP CSVNEnumFormatEtc::QueryInterface(REFIID refiid, void** ppv)
{
    if (ppv == nullptr)
        return E_POINTER;
    *ppv = nullptr;
    if (IID_IUnknown == refiid || IID_IEnumFORMATETC == refiid)
        *ppv = static_cast<IEnumFORMATETC*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CSVNEnumFormatEtc::AddRef()
{
    return ++m_cRefCount;
}

STDMETHODIMP_(ULONG)
CSVNEnumFormatEtc::Release()
{
    --m_cRefCount;
    if (m_cRefCount == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefCount;
}

STDMETHODIMP CSVNEnumFormatEtc::Next(ULONG celt, LPFORMATETC lpFormatEtc, ULONG* pceltFetched)
{
    if (celt <= 0)
        return E_INVALIDARG;
    if (pceltFetched == nullptr && celt != 1) // pceltFetched can be NULL only for 1 item request
        return E_POINTER;
    if (lpFormatEtc == nullptr)
        return E_POINTER;

    if (pceltFetched != nullptr)
        *pceltFetched = 0;

    if (m_iCur >= SVNDATAOBJECT_NUMFORMATS)
        return S_FALSE;

    ULONG cReturn = celt;

    while (m_iCur < (SVNDATAOBJECT_NUMFORMATS + m_vecFormatEtc.size()) && cReturn > 0)
    {
        if (m_iCur < SVNDATAOBJECT_NUMFORMATS)
            *lpFormatEtc++ = m_formats[m_iCur++];
        else
            *lpFormatEtc++ = m_vecFormatEtc[m_iCur++ - SVNDATAOBJECT_NUMFORMATS];
        --cReturn;
    }

    if (pceltFetched != nullptr)
        *pceltFetched = celt - cReturn;

    return (cReturn == 0) ? S_OK : S_FALSE;
}

STDMETHODIMP CSVNEnumFormatEtc::Skip(ULONG celt)
{
    if ((m_iCur + static_cast<int>(celt)) >= (SVNDATAOBJECT_NUMFORMATS + m_vecFormatEtc.size()))
        return S_FALSE;
    m_iCur += celt;
    return S_OK;
}

STDMETHODIMP CSVNEnumFormatEtc::Reset()
{
    m_iCur = 0;
    return S_OK;
}

STDMETHODIMP CSVNEnumFormatEtc::Clone(IEnumFORMATETC** ppCloneEnumFormatEtc)
{
    if (ppCloneEnumFormatEtc == nullptr)
        return E_POINTER;

    CSVNEnumFormatEtc* newEnum = new (std::nothrow) CSVNEnumFormatEtc(m_vecFormatEtc, m_localOnly);
    if (newEnum == nullptr)
        return E_OUTOFMEMORY;

    newEnum->AddRef();
    newEnum->m_iCur       = m_iCur;
    *ppCloneEnumFormatEtc = newEnum;
    return S_OK;
}