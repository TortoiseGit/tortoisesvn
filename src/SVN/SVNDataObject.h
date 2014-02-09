// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010, 2012-2014 - TortoiseSVN

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

extern  CLIPFORMAT  CF_FILECONTENTS;
extern  CLIPFORMAT  CF_FILEDESCRIPTOR;
extern  CLIPFORMAT  CF_PREFERREDDROPEFFECT;
extern  CLIPFORMAT  CF_SVNURL;
extern  CLIPFORMAT  CF_INETURL;
extern  CLIPFORMAT  CF_SHELLURL;


#define SVNDATAOBJECT_NUMFORMATS 9

/**
 * \ingroup SVN
 * Represents a Subversion URL or path as an IDataObject.
 * This can be used for drag and drop operations to other applications like
 * the shell itself.
 */
class SVNDataObject : public IDataObject, public IDataObjectAsyncCapability
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
    ~SVNDataObject();

    //IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    //IDataObject
    virtual HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium);
    virtual HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium);
    virtual HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pformatetc);
    virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC* pformatectIn, FORMATETC* pformatetcOut);
    virtual HRESULT STDMETHODCALLTYPE SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease);
    virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc);
    virtual HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
    virtual HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection);
    virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA** ppenumAdvise);

    //IDataObjectAsyncCapability
    virtual HRESULT STDMETHODCALLTYPE SetAsyncMode(BOOL fDoOpAsync);
    virtual HRESULT STDMETHODCALLTYPE GetAsyncMode(BOOL* pfIsOpAsync);
    virtual HRESULT STDMETHODCALLTYPE StartOperation(IBindCtx* pbcReserved);
    virtual HRESULT STDMETHODCALLTYPE InOperation(BOOL* pfInAsyncOp);
    virtual HRESULT STDMETHODCALLTYPE EndOperation(HRESULT hResult, IBindCtx* pbcReserved, DWORD dwEffects);

    HRESULT SetDropDescription(DROPIMAGETYPE image, LPCTSTR format, LPCTSTR insert);

private:
    void CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc);

private:
    struct SVNObjectInfoData
    {
        CTSVNPath               rootpath;
        SVNInfoData             infodata;
    } SVNobjectInfoData;

private:
    SVN                         m_svn;
    CTSVNPathList               m_svnPaths;
    SVNRev                      m_pegRev;
    SVNRev                      m_revision;
    bool                        m_bFilesAsUrlLinks;
    std::vector<SVNObjectInfoData>   m_allPaths;
    long                        m_cRefCount;
    BOOL                        m_bInOperation;
    BOOL                        m_bIsAsync;
    std::vector<FORMATETC*>     m_vecFormatEtc;
    std::vector<STGMEDIUM*>     m_vecStgMedium;
};


/**
 * Helper class for the SVNDataObject class: implements the enumerator
 * for the supported clipboard formats of the SVNDataObject class.
 */
class CSVNEnumFormatEtc : public IEnumFORMATETC
{
public:
    CSVNEnumFormatEtc(const std::vector<FORMATETC*>& vec, bool localonly);
    CSVNEnumFormatEtc(const std::vector<FORMATETC>& vec, bool localonly);
    //IUnknown members
    STDMETHOD(QueryInterface)(REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //IEnumFORMATETC members
    STDMETHOD(Next)(ULONG, LPFORMATETC, ULONG*);
    STDMETHOD(Skip)(ULONG);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumFORMATETC**);
private:
    void                        Init(bool localonly);
private:
    std::vector<FORMATETC>      m_vecFormatEtc;
    FORMATETC                   m_formats[SVNDATAOBJECT_NUMFORMATS];
    ULONG                       m_cRefCount;
    size_t                      m_iCur;
    bool                        m_localonly;
};

