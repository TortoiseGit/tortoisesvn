// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010, 2012-2014, 2021 - TortoiseSVN

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

#pragma once
#include "TSVNPath.h"
#include "SVN.h"
#include "SVNInfo.h"
#include "DragDropImpl.h"
#include <vector>
#include <Shldisp.h>

extern CLIPFORMAT CF_FILECONTENTS;
extern CLIPFORMAT CF_FILEDESCRIPTOR;
extern CLIPFORMAT CF_PREFERREDDROPEFFECT;
extern CLIPFORMAT CF_SVNURL;
extern CLIPFORMAT CF_INETURL;
extern CLIPFORMAT CF_SHELLURL;
extern CLIPFORMAT CF_FILE_ATTRIBUTES_ARRAY;

#define SVNDATAOBJECT_NUMFORMATS 10

/**
 * \ingroup SVN
 * Represents a Subversion URL or path as an IDataObject.
 * This can be used for drag and drop operations to other applications like
 * the shell itself.
 */
class SVNDataObject : public IDataObject
    , public IDataObjectAsyncCapability
{
public:
    /**
     * Constructs the SVNDataObject.
     * \param svnpaths a list of paths or URLs. Local paths must be inside a
     *                 working copy, URLs must point to a Subversion repository.
     * \param peg      the peg revision the URL points to, or SVNRev::REV_WC/SVNRev::REV_BASE
     * \param rev      the revision the URL points to, or SVNRev::REV_WC/SVNRev::REV_BASE
     * \param bFilesAsUrlLinks
     */
    SVNDataObject(const CTSVNPathList& svnpaths, SVNRev peg, SVNRev rev, bool bFilesAsUrlLinks = false);
    virtual ~SVNDataObject();

    //IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE   AddRef() override;
    ULONG STDMETHODCALLTYPE   Release() override;

    //IDataObject
    HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pformatetcIn, STGMEDIUM* pMedium) override;
    HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pMedium) override;
    HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pFormatEtc) override;
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC* pFormatEctIn, FORMATETC* pFormatEtcOut) override;
    HRESULT STDMETHODCALLTYPE SetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium, BOOL fRelease) override;
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc) override;
    HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection) override;
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) override;
    HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA** ppEnumAdvise) override;

    //IDataObjectAsyncCapability
    HRESULT STDMETHODCALLTYPE SetAsyncMode(BOOL fDoOpAsync) override;
    HRESULT STDMETHODCALLTYPE GetAsyncMode(BOOL* pfIsOpAsync) override;
    HRESULT STDMETHODCALLTYPE StartOperation(IBindCtx* pbcReserved) override;
    HRESULT STDMETHODCALLTYPE InOperation(BOOL* pfInAsyncOp) override;
    HRESULT STDMETHODCALLTYPE EndOperation(HRESULT hResult, IBindCtx* pbcReserved, DWORD dwEffects) override;

    HRESULT SetDropDescription(DROPIMAGETYPE image, LPCWSTR format, LPCWSTR insert);

private:
    static void CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc);

private:
    struct SVNObjectInfoData
    {
        CTSVNPath   rootPath;
        SVNInfoData infoData;
    };

private:
    SVN                            m_svn;
    CTSVNPathList                  m_svnPaths;
    SVNRev                         m_pegRev;
    SVNRev                         m_revision;
    bool                           m_bFilesAsUrlLinks;
    std::vector<SVNObjectInfoData> m_allPaths;
    long                           m_cRefCount;
    BOOL                           m_bInOperation;
    BOOL                           m_bIsAsync;
    std::vector<FORMATETC*>        m_vecFormatEtc;
    std::vector<STGMEDIUM*>        m_vecStgMedium;
};

/**
 * Helper class for the SVNDataObject class: implements the enumerator
 * for the supported clipboard formats of the SVNDataObject class.
 */
class CSVNEnumFormatEtc : public IEnumFORMATETC
{
public:
    virtual ~CSVNEnumFormatEtc() = default;
    CSVNEnumFormatEtc(const std::vector<FORMATETC*>& vec, bool localOnly);
    CSVNEnumFormatEtc(const std::vector<FORMATETC>& vec, bool localOnly);
    //IUnknown members
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override;
    ULONG STDMETHODCALLTYPE   AddRef() override;
    ULONG STDMETHODCALLTYPE   Release() override;

    //IEnumFORMATETC members
    HRESULT STDMETHODCALLTYPE Next(ULONG, LPFORMATETC, ULONG*) override;
    HRESULT STDMETHODCALLTYPE Skip(ULONG) override;
    HRESULT STDMETHODCALLTYPE Reset() override;
    HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC**) override;

private:
    void Init(bool localonly);

private:
    std::vector<FORMATETC> m_vecFormatEtc;
    FORMATETC              m_formats[SVNDATAOBJECT_NUMFORMATS];
    ULONG                  m_cRefCount;
    size_t                 m_iCur;
    bool                   m_localOnly;
};
