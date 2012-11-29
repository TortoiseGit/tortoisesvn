// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012 - TortoiseSVN

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

CLIPFORMAT CF_FILECONTENTS = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS);
CLIPFORMAT CF_FILEDESCRIPTOR = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
CLIPFORMAT CF_PREFERREDDROPEFFECT = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
CLIPFORMAT CF_SVNURL = (CLIPFORMAT)RegisterClipboardFormat(_T("TSVN_SVNURL"));
CLIPFORMAT CF_INETURL = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_INETURL);
CLIPFORMAT CF_SHELLURL = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLURL);

SVNDataObject::SVNDataObject(const CTSVNPathList& svnpaths, SVNRev peg, SVNRev rev, bool bFilesAsUrlLinks) : m_svnPaths(svnpaths)
    , m_pegRev(peg)
    , m_revision(rev)
    , m_bFilesAsUrlLinks(bFilesAsUrlLinks)
    , m_bInOperation(FALSE)
    , m_bIsAsync(TRUE)
    , m_cRefCount(0)
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

#ifndef __IDataObjectAsyncCapability_FWD_DEFINED__
#define IID_IDataObjectAsyncCapability IID_IAsyncOperation
#endif

STDMETHODIMP SVNDataObject::QueryInterface(REFIID riid, void** ppvObject)
{
    if(ppvObject == 0)
        return E_POINTER;
    *ppvObject = NULL;
    if (IsEqualIID(IID_IUnknown, riid) || IsEqualIID(IID_IDataObject, riid))
        *ppvObject=static_cast<IDataObject*>(this);
    else if (IsEqualIID(riid, IID_IDataObjectAsyncCapability))
        *ppvObject = static_cast<IDataObjectAsyncCapability*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) SVNDataObject::AddRef(void)
{
    return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) SVNDataObject::Release(void)
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
STDMETHODIMP SVNDataObject::GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
    if (pformatetcIn == NULL)
        return E_INVALIDARG;
    if (pmedium == NULL)
        return E_POINTER;
    pmedium->hGlobal = NULL;

    if ((pformatetcIn->tymed & TYMED_ISTREAM) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILECONTENTS))
    {
        // supports the IStream format.
        // The lindex param is the index of the file to return
        CTSVNPath filepath;
        IStream * pIStream = NULL;

        if (m_bFilesAsUrlLinks)
        {
            if ((pformatetcIn->lindex >= 0)&&(pformatetcIn->lindex < (LONG)m_allPaths.size()))
            {
                filepath = CTempFiles::Instance().GetTempFilePath(true);
                CString sTemp;
                sTemp.Format(L"[InternetShortcut]\nURL=%s\n", (LPCWSTR)m_allPaths[pformatetcIn->lindex].infodata.url);
                CStringUtils::WriteStringToTextFile(filepath.GetWinPath(), (LPCWSTR)sTemp);
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
                if ((pformatetcIn->lindex >= 0)&&(pformatetcIn->lindex < (LONG)m_allPaths.size()))
                {
                    filepath = m_allPaths[pformatetcIn->lindex].rootpath;
                }
            }
            else
            {
                filepath = CTempFiles::Instance().GetTempFilePath(true);
                if ((pformatetcIn->lindex >= 0)&&(pformatetcIn->lindex < (LONG)m_allPaths.size()))
                {
                    if (!m_svn.Export(CTSVNPath(m_allPaths[pformatetcIn->lindex].infodata.url), filepath, m_pegRev, m_revision))
                    {
                        DeleteFile(filepath.GetWinPath());
                        return STG_E_ACCESSDENIED;
                    }
                }
            }
        }
        HRESULT res = SHCreateStreamOnFileEx(filepath.GetWinPath(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, NULL, &pIStream);
        if (res == S_OK)
        {
            pmedium->pstm = pIStream;
            pmedium->tymed = TYMED_ISTREAM;
            return S_OK;
        }
        return res;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_FILEDESCRIPTOR))
    {
        if (m_bFilesAsUrlLinks)
        {
            for (int i=0; i<m_svnPaths.GetCount(); ++i)
            {
                SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], SVNInfoData()};
                id.infodata.url = m_svnPaths[i].GetSVNPathString();
                m_allPaths.push_back(id);
            }
        }
        else
        {
            // now it is time to get all sub folders for the directories we have
            SVNInfo svnInfo;
            // find the common directory of all the paths
            CTSVNPath commonDir;
            bool bAllUrls = true;
            for (int i=0; i<m_svnPaths.GetCount(); ++i)
            {
                if (!m_svnPaths[i].IsUrl())
                    bAllUrls = false;
                if (commonDir.IsEmpty())
                    commonDir = m_svnPaths[i].GetContainingDirectory();
                if (!commonDir.IsEquivalentTo(m_svnPaths[i].GetContainingDirectory()))
                {
                    commonDir.Reset();
                    break;
                }
            }
            if (bAllUrls && (m_svnPaths.GetCount() > 1) && !commonDir.IsEmpty())
            {
                // if all paths are in the same directory, we can fetch the info recursively
                // from the parent folder to save a lot of time.
                const SVNInfoData * infodata = svnInfo.GetFirstFileInfo(commonDir, m_pegRev, m_revision, svn_depth_infinity);
                while (infodata)
                {
                    // check if the returned item is one in our list
                    for (int i=0; i<m_svnPaths.GetCount(); ++i)
                    {
                        if (m_svnPaths[i].IsAncestorOf(CTSVNPath(infodata->url)))
                        {
                            SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], *infodata};
                            m_allPaths.push_back(id);
                            break;
                        }
                    }
                    infodata = svnInfo.GetNextFileInfo();
                }
            }
            else
            {
                for (int i = 0; i < m_svnPaths.GetCount(); ++i)
                {
                    if (m_svnPaths[i].IsUrl())
                    {
                        const SVNInfoData * infodata = svnInfo.GetFirstFileInfo(m_svnPaths[i], m_pegRev, m_revision, svn_depth_infinity);
                        while (infodata)
                        {
                            SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], *infodata};
                            m_allPaths.push_back(id);
                            infodata = svnInfo.GetNextFileInfo();
                        }
                    }
                    else
                    {
                        SVNDataObject::SVNObjectInfoData id = {m_svnPaths[i], SVNInfoData()};
                        m_allPaths.push_back(id);
                    }
                }
            }
        }
        size_t dataSize = sizeof(FILEGROUPDESCRIPTOR) + ((m_allPaths.size() - 1) * sizeof(FILEDESCRIPTOR));
        HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, dataSize);

        FILEGROUPDESCRIPTOR* files = (FILEGROUPDESCRIPTOR*)GlobalLock(data);
        files->cItems = static_cast<UINT>(m_allPaths.size());
        int index = 0;
        for (vector<SVNDataObject::SVNObjectInfoData>::const_iterator it = m_allPaths.begin(); it != m_allPaths.end(); ++it)
        {
            CString temp;
            if (it->rootpath.IsUrl())
            {
                temp = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(it->rootpath.GetContainingDirectory().GetSVNPathString())));
                temp = it->infodata.url.Mid(temp.GetLength()+1);
                // we have to unescape the urls since the local file system doesn't need them
                // escaped and it would only look really ugly (and be wrong).
                temp = CPathUtils::PathUnescape(temp);
                if (m_bFilesAsUrlLinks)
                    temp += L".url";
                temp.Replace(_T("/"), _T("\\"));
            }
            else
            {
                temp = it->rootpath.GetUIFileOrDirectoryName();
                if (m_bFilesAsUrlLinks)
                    temp += L".url";
            }
            if (temp.GetLength() < MAX_PATH)
                _tcscpy_s(files->fgd[index].cFileName, (LPCTSTR)temp);
            else
            {
                files->cItems--;
                continue;
            }
            files->fgd[index].dwFlags = FD_ATTRIBUTES | FD_PROGRESSUI | FD_FILESIZE | FD_LINKUI;
            if (it->rootpath.IsUrl())
            {
                if (m_bFilesAsUrlLinks)
                    files->fgd[index].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                else
                    files->fgd[index].dwFileAttributes = (it->infodata.kind == svn_node_dir) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

                struct tm newtime;
                SYSTEMTIME systime;
                __time64_t ttt = it->infodata.lastchangedtime;
                _localtime64_s(&newtime, &ttt);

                systime.wDay = (WORD)newtime.tm_mday;
                systime.wDayOfWeek = (WORD)newtime.tm_wday;
                systime.wHour = (WORD)newtime.tm_hour;
                systime.wMilliseconds = 0;
                systime.wMinute = (WORD)newtime.tm_min;
                systime.wMonth = (WORD)newtime.tm_mon+1;
                systime.wSecond = (WORD)newtime.tm_sec;
                systime.wYear = (WORD)newtime.tm_year+1900;
                SystemTimeToFileTime(&systime, &files->fgd[index].ftLastWriteTime);
                files->fgd[index].dwFlags |= FD_WRITESTIME;
            }
            else
            {
                if (m_bFilesAsUrlLinks)
                    files->fgd[index].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                else
                    files->fgd[index].dwFileAttributes = it->rootpath.IsDirectory() ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            }
            // Always set the file size to 0 even if we 'know' the file size (infodata.size64).
            // Because for text files, the file size is too low due to the EOLs getting converted
            // to CRLF (from LF as stored in the repository). And a too low file size set here
            // prevents the shell from reading the full file later - it only reads the stream up
            // to the number of bytes specified here. Which means we would end up with a truncated
            // text file (binary files are still ok though).
            files->fgd[index].nFileSizeLow = 0;
            files->fgd[index].nFileSizeHigh = 0;

            ++index;
        }

        GlobalUnlock(data);

        pmedium->hGlobal = data;
        pmedium->tymed = TYMED_HGLOBAL;
        return S_OK;
    }
    // handling CF_PREFERREDDROPEFFECT is necessary to tell the shell that it should *not* ask for the
    // CF_FILEDESCRIPTOR until the drop actually occurs. If we don't handle CF_PREFERREDDROPEFFECT, the shell
    // will ask for the file descriptor for every object (file/folder) the mouse pointer hovers over and is
    // a potential drop target.
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->cfFormat == CF_PREFERREDDROPEFFECT))
    {
        HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, sizeof(DWORD));
        DWORD* effect = (DWORD*) GlobalLock(data);
        if (m_bFilesAsUrlLinks)
            (*effect) = DROPEFFECT_LINK;
        else
            (*effect) = DROPEFFECT_COPY;
        GlobalUnlock(data);
        pmedium->hGlobal = data;
        pmedium->tymed = TYMED_HGLOBAL;
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
            for (int i=0; i<m_svnPaths.GetCount(); ++i)
            {
                if (m_svnPaths[i].IsUrl())
                    text += m_svnPaths[i].GetSVNPathString();
                else
                    text += m_svnPaths[i].GetWinPathString();
                text += _T("\r\n");
            }
        }
        CStringA texta = CUnicodeUtils::GetUTF8(text);
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GHND, (texta.GetLength()+1)*sizeof(char));
        if (pmedium->hGlobal)
        {
            char* pMem = (char*)GlobalLock(pmedium->hGlobal);
            strcpy_s(pMem, texta.GetLength()+1, (LPCSTR)texta);
            GlobalUnlock(pmedium->hGlobal);
        }
        pmedium->pUnkForRelease = NULL;
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
            for (int i=0; i<m_svnPaths.GetCount(); ++i)
            {
                if (m_svnPaths[i].IsUrl())
                    text += m_svnPaths[i].GetSVNPathString();
                else
                    text += m_svnPaths[i].GetWinPathString();
                text += _T("\r\n");
            }
        }
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GHND, (text.GetLength()+1)*sizeof(TCHAR));
        if (pmedium->hGlobal)
        {
            TCHAR* pMem = (TCHAR*)GlobalLock(pmedium->hGlobal);
            _tcscpy_s(pMem, text.GetLength()+1, (LPCTSTR)text);
            GlobalUnlock(pmedium->hGlobal);
        }
        pmedium->pUnkForRelease = NULL;
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
            for (int i=0; i<m_svnPaths.GetCount(); ++i)
            {
                if (m_svnPaths[i].IsUrl())
                {
                    text += m_svnPaths[i].GetSVNPathString();
                    text += _T("?");
                    text += m_revision.ToString();
                }
                else
                    text += m_svnPaths[i].GetWinPathString();
                text += _T("\r\n");
            }
        }
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GHND, (text.GetLength()+1)*sizeof(TCHAR));
        if (pmedium->hGlobal)
        {
            TCHAR* pMem = (TCHAR*)GlobalLock(pmedium->hGlobal);
            _tcscpy_s(pMem, text.GetLength()+1, (LPCTSTR)text);
            GlobalUnlock(pmedium->hGlobal);
        }
        pmedium->pUnkForRelease = NULL;
        return S_OK;
    }
    else if ((pformatetcIn->tymed & TYMED_HGLOBAL) && (pformatetcIn->dwAspect == DVASPECT_CONTENT) && (pformatetcIn->cfFormat == CF_HDROP))
    {
        int nLength = 0;

        for (int i=0;i<m_svnPaths.GetCount();i++)
        {
            nLength += m_svnPaths[i].GetWinPathString().GetLength();
            nLength += 1; // '\0' separator
        }

        int nBufferSize = sizeof(DROPFILES) + (nLength+1)*sizeof(TCHAR);
        std::unique_ptr<char[]> pBuffer(new char[nBufferSize]);

        SecureZeroMemory(pBuffer.get(), nBufferSize);

        DROPFILES* df = (DROPFILES*)pBuffer.get();
        df->pFiles = sizeof(DROPFILES);
        df->fWide = 1;

        TCHAR* pFilenames = (TCHAR*)(pBuffer.get() + sizeof(DROPFILES));
        TCHAR* pCurrentFilename = pFilenames;

        for (int i=0;i<m_svnPaths.GetCount();i++)
        {
            CString str = m_svnPaths[i].GetWinPathString();
            wcscpy_s(pCurrentFilename, str.GetLength()+1, (LPCWSTR)str);
            pCurrentFilename += str.GetLength();
            *pCurrentFilename = '\0'; // separator between file names
            pCurrentFilename++;
        }
        *pCurrentFilename = '\0'; // terminate array

        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE|GMEM_DDESHARE, nBufferSize);
        if (pmedium->hGlobal)
        {
            LPVOID pMem = ::GlobalLock(pmedium->hGlobal);
            if (pMem)
                memcpy(pMem, pBuffer.get(), nBufferSize);
            GlobalUnlock(pmedium->hGlobal);
        }
        pmedium->pUnkForRelease = NULL;
        return S_OK;
    }

    for (size_t i=0; i<m_vecFormatEtc.size(); ++i)
    {
        if ((pformatetcIn->tymed == m_vecFormatEtc[i]->tymed) &&
            (pformatetcIn->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
            (pformatetcIn->cfFormat == m_vecFormatEtc[i]->cfFormat))
        {
            CopyMedium(pmedium, m_vecStgMedium[i], m_vecFormatEtc[i]);
            return S_OK;
        }
    }

    return DV_E_FORMATETC;
}

STDMETHODIMP SVNDataObject::GetDataHere(FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP SVNDataObject::QueryGetData(FORMATETC* pformatetc)
{
    if (pformatetc == NULL)
        return E_INVALIDARG;

    if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
        return DV_E_DVASPECT;

    if ((pformatetc->tymed & TYMED_ISTREAM) &&
        (pformatetc->dwAspect == DVASPECT_CONTENT) &&
        (pformatetc->cfFormat == CF_FILECONTENTS))
    {
        return S_OK;
    }
    if ((pformatetc->tymed & TYMED_HGLOBAL) &&
        (pformatetc->dwAspect == DVASPECT_CONTENT) &&
        ((pformatetc->cfFormat == CF_TEXT)||(pformatetc->cfFormat == CF_UNICODETEXT)||(pformatetc->cfFormat == CF_INETURL)||(pformatetc->cfFormat == CF_SHELLURL)||(pformatetc->cfFormat == CF_PREFERREDDROPEFFECT)))
    {
        return S_OK;
    }
    if ((pformatetc->tymed & TYMED_HGLOBAL) &&
        (pformatetc->dwAspect == DVASPECT_CONTENT) &&
        ((pformatetc->cfFormat == CF_SVNURL)||(pformatetc->cfFormat == CF_FILEDESCRIPTOR)) &&
        !m_revision.IsWorking())
    {
        return S_OK;
    }
    if ((pformatetc->tymed & TYMED_HGLOBAL) &&
        (pformatetc->dwAspect == DVASPECT_CONTENT) &&
        (pformatetc->cfFormat == CF_HDROP) &&
        m_revision.IsWorking())
    {
        return S_OK;
    }

    for (size_t i=0; i<m_vecFormatEtc.size(); ++i)
    {
        if ((pformatetc->tymed == m_vecFormatEtc[i]->tymed) &&
            (pformatetc->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
            (pformatetc->cfFormat == m_vecFormatEtc[i]->cfFormat))
            return S_OK;
    }

    return DV_E_TYMED;
}

STDMETHODIMP SVNDataObject::GetCanonicalFormatEtc(FORMATETC* /*pformatectIn*/, FORMATETC* pformatetcOut)
{
    if (pformatetcOut == NULL)
        return E_INVALIDARG;
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP SVNDataObject::SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease)
{
    if ((pformatetc == NULL) || (pmedium == NULL))
        return E_INVALIDARG;

    FORMATETC* fetc = new (std::nothrow) FORMATETC;
    STGMEDIUM* pStgMed = new (std::nothrow) STGMEDIUM;

    if ((fetc == NULL) || (pStgMed == NULL))
    {
        delete fetc;
        delete pStgMed;
        return E_OUTOFMEMORY;
    }
    SecureZeroMemory(fetc,sizeof(FORMATETC));
    SecureZeroMemory(pStgMed,sizeof(STGMEDIUM));

    // do we already store this format?
    for (size_t i=0; i<m_vecFormatEtc.size(); ++i)
    {
        if ((pformatetc->tymed == m_vecFormatEtc[i]->tymed) &&
            (pformatetc->dwAspect == m_vecFormatEtc[i]->dwAspect) &&
            (pformatetc->cfFormat == m_vecFormatEtc[i]->cfFormat))
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

    *fetc = *pformatetc;
    m_vecFormatEtc.push_back(fetc);

    if (fRelease)
        *pStgMed = *pmedium;
    else
    {
        CopyMedium(pStgMed, pmedium, pformatetc);
    }
    m_vecStgMedium.push_back(pStgMed);

    return S_OK;
}

STDMETHODIMP SVNDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{
    if (ppenumFormatEtc == NULL)
        return E_POINTER;

    *ppenumFormatEtc = NULL;
    switch (dwDirection)
    {
    case DATADIR_GET:
        *ppenumFormatEtc= new (std::nothrow) CSVNEnumFormatEtc(m_vecFormatEtc, !!m_revision.IsWorking());
        if (*ppenumFormatEtc == NULL)
            return E_OUTOFMEMORY;
        (*ppenumFormatEtc)->AddRef();
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
    switch(pMedSrc->tymed)
    {
    case TYMED_HGLOBAL:
        pMedDest->hGlobal = (HGLOBAL)OleDuplicateData(pMedSrc->hGlobal,pFmtSrc->cfFormat, NULL);
        break;
    case TYMED_GDI:
        pMedDest->hBitmap = (HBITMAP)OleDuplicateData(pMedSrc->hBitmap,pFmtSrc->cfFormat, NULL);
        break;
    case TYMED_MFPICT:
        pMedDest->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(pMedSrc->hMetaFilePict,pFmtSrc->cfFormat, NULL);
        break;
    case TYMED_ENHMF:
        pMedDest->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(pMedSrc->hEnhMetaFile,pFmtSrc->cfFormat, NULL);
        break;
    case TYMED_FILE:
        pMedSrc->lpszFileName = (LPOLESTR)OleDuplicateData(pMedSrc->lpszFileName,pFmtSrc->cfFormat, NULL);
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
    pMedDest->tymed = pMedSrc->tymed;
    pMedDest->pUnkForRelease = NULL;
    if(pMedSrc->pUnkForRelease != NULL)
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

HRESULT SVNDataObject::SetDropDescription(DROPIMAGETYPE image, LPCTSTR format, LPCTSTR insert)
{
    if(format == NULL || insert == NULL)
        return E_INVALIDARG;

    FORMATETC fetc = {0};
    fetc.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DROPDESCRIPTION);
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;
    fetc.tymed = TYMED_HGLOBAL;

    STGMEDIUM medium = {0};
    medium.hGlobal = GlobalAlloc(GHND, sizeof(DROPDESCRIPTION));
    if(medium.hGlobal == 0)
        return E_OUTOFMEMORY;

    DROPDESCRIPTION* pDropDescription = (DROPDESCRIPTION*)GlobalLock(medium.hGlobal);
    if (pDropDescription == nullptr)
        return E_FAIL;
    lstrcpyW(pDropDescription->szInsert, insert);
    lstrcpyW(pDropDescription->szMessage, format);
    pDropDescription->type = image;
    GlobalUnlock(medium.hGlobal);
    return SetData(&fetc, &medium, TRUE);
}

void CSVNEnumFormatEtc::Init(bool localonly)
{
    int index = 0;
    m_formats[index].cfFormat = CF_UNICODETEXT;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex = -1;
    m_formats[index].ptd = NULL;
    m_formats[index].tymed = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_TEXT;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex = -1;
    m_formats[index].ptd = NULL;
    m_formats[index].tymed = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_PREFERREDDROPEFFECT;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex = -1;
    m_formats[index].ptd = NULL;
    m_formats[index].tymed = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_INETURL;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex = -1;
    m_formats[index].ptd = NULL;
    m_formats[index].tymed = TYMED_HGLOBAL;
    index++;

    m_formats[index].cfFormat = CF_SHELLURL;
    m_formats[index].dwAspect = DVASPECT_CONTENT;
    m_formats[index].lindex = -1;
    m_formats[index].ptd = NULL;
    m_formats[index].tymed = TYMED_HGLOBAL;
    index++;

    if (localonly)
    {
        m_formats[index].cfFormat = CF_HDROP;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex = -1;
        m_formats[index].ptd = NULL;
        m_formats[index].tymed = TYMED_HGLOBAL;
        index++;
    }
    else
    {
        m_formats[index].cfFormat = CF_FILECONTENTS;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex = -1;
        m_formats[index].ptd = NULL;
        m_formats[index].tymed = TYMED_ISTREAM;
        index++;

        m_formats[index].cfFormat = CF_FILEDESCRIPTOR;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex = -1;
        m_formats[index].ptd = NULL;
        m_formats[index].tymed = TYMED_HGLOBAL;
        index++;

        m_formats[index].cfFormat = CF_SVNURL;
        m_formats[index].dwAspect = DVASPECT_CONTENT;
        m_formats[index].lindex = -1;
        m_formats[index].ptd = NULL;
        m_formats[index].tymed = TYMED_HGLOBAL;
        index++;
    }
    // clear possible leftovers
    while (index < SVNDATAOBJECT_NUMFORMATS)
    {
        m_formats[index].cfFormat = 0;
        m_formats[index].dwAspect = 0;
        m_formats[index].lindex = -1;
        m_formats[index].ptd = NULL;
        m_formats[index].tymed = 0;
        index++;
    }
}

CSVNEnumFormatEtc::CSVNEnumFormatEtc(const vector<FORMATETC>& vec, bool localonly) : m_cRefCount(0)
    , m_iCur(0)
{
    m_localonly = localonly;
    for (size_t i = 0; i < vec.size(); ++i)
        m_vecFormatEtc.push_back(vec[i]);
    Init(localonly);
}

CSVNEnumFormatEtc::CSVNEnumFormatEtc(const vector<FORMATETC*>& vec, bool localonly) : m_cRefCount(0)
    , m_iCur(0)
{
    m_localonly = localonly;
    for (size_t i = 0; i < vec.size(); ++i)
        m_vecFormatEtc.push_back(*vec[i]);
    Init(localonly);
}

STDMETHODIMP  CSVNEnumFormatEtc::QueryInterface(REFIID refiid, void** ppv)
{
    if(ppv == 0)
        return E_POINTER;
    *ppv = NULL;
    if (IID_IUnknown == refiid || IID_IEnumFORMATETC == refiid)
        *ppv = static_cast<IEnumFORMATETC*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CSVNEnumFormatEtc::AddRef(void)
{
    return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CSVNEnumFormatEtc::Release(void)
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
    if(celt <= 0)
        return E_INVALIDARG;
    if (pceltFetched == NULL && celt != 1) // pceltFetched can be NULL only for 1 item request
        return E_POINTER;
    if(lpFormatEtc == NULL)
        return E_POINTER;

    if (pceltFetched != NULL)
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

    if (pceltFetched != NULL)
        *pceltFetched = celt - cReturn;

    return (cReturn == 0) ? S_OK : S_FALSE;
}

STDMETHODIMP CSVNEnumFormatEtc::Skip(ULONG celt)
{
    if ((m_iCur + int(celt)) >= (SVNDATAOBJECT_NUMFORMATS + m_vecFormatEtc.size()))
        return S_FALSE;
    m_iCur += celt;
    return S_OK;
}

STDMETHODIMP CSVNEnumFormatEtc::Reset(void)
{
    m_iCur = 0;
    return S_OK;
}

STDMETHODIMP CSVNEnumFormatEtc::Clone(IEnumFORMATETC** ppCloneEnumFormatEtc)
{
    if (ppCloneEnumFormatEtc == NULL)
        return E_POINTER;

    CSVNEnumFormatEtc *newEnum = new (std::nothrow) CSVNEnumFormatEtc(m_vecFormatEtc, m_localonly);
    if (newEnum == NULL)
        return E_OUTOFMEMORY;

    newEnum->AddRef();
    newEnum->m_iCur = m_iCur;
    *ppCloneEnumFormatEtc = newEnum;
    return S_OK;
}