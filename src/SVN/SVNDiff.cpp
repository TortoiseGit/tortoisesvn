// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2016, 2018, 2021-2022 - TortoiseSVN

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
#pragma warning(push)
#include "SVNDiff.h"
#include "svn_types.h"
#pragma warning(pop)

#include "resource.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "FileDiffDlg.h"
#include "ProgressDlg.h"
#include "Blame.h"

SVNDiff::SVNDiff(SVN* pSVN /* = NULL */, HWND hWnd /* = NULL */, bool bRemoveTempFiles /* = false */)
    : m_pSVN(nullptr)
    , m_bDeleteSVN(false)
    , m_hWnd(nullptr)
    , m_bRemoveTempFiles(false)
    , m_headPeg(SVNRev::REV_HEAD)
    , m_bAlternativeTool(false)
    , m_jumpLine(0)
{
    if (pSVN)
        m_pSVN = pSVN;
    else
    {
        m_pSVN       = new SVN;
        m_bDeleteSVN = true;
    }
    m_hWnd             = hWnd;
    m_bRemoveTempFiles = bRemoveTempFiles;
}

SVNDiff::~SVNDiff()
{
    if (m_bDeleteSVN)
        delete m_pSVN;
}

bool SVNDiff::DiffWCFile(HWND hParent, const CTSVNPath& filePath,
                         bool               ignoreProps,
                         svn_wc_status_kind status, /* = svn_wc_status_none */
                         svn_wc_status_kind textStatus /* = svn_wc_status_none */,
                         svn_wc_status_kind propStatus /* = svn_wc_status_none */,
                         svn_wc_status_kind remoteTextStatus /* = svn_wc_status_none */,
                         svn_wc_status_kind remotePropStatus /* = svn_wc_status_none */) const
{
    CTSVNPath    basePath;
    CTSVNPath    remotePath;
    SVNRev       remoteRev;
    svn_revnum_t baseRev = 0;

    // first diff the remote properties against the wc props
    // TODO: should we attempt to do a three way diff with the properties too
    // if they're modified locally and remotely?
    if (!ignoreProps && (remotePropStatus > svn_wc_status_normal))
    {
        DiffProps(filePath, SVNRev::REV_HEAD, SVNRev::REV_WC, baseRev);
    }
    if (!ignoreProps && (propStatus > svn_wc_status_normal) && (filePath.IsDirectory()))
    {
        DiffProps(filePath, SVNRev::REV_WC, SVNRev::REV_BASE, baseRev);
    }
    if (filePath.IsDirectory())
        return true;

    if ((status > svn_wc_status_normal) || (textStatus > svn_wc_status_normal))
    {
        basePath = SVN::GetPristinePath(hParent, filePath);
        if (baseRev == 0)
        {
            SVNStatus            stat;
            CTSVNPath            dummy;
            svn_client_status_t* s = stat.GetFirstFileStatus(filePath, dummy);
            if (s)
                baseRev = s->revision >= 0 ? s->revision : s->changed_rev;
        }
        // If necessary, convert the line-endings on the file before diffing
        if (static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\ConvertBase", TRUE)))
        {
            CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            if (!m_pSVN->Export(filePath, temporaryFile, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE)))
            {
                temporaryFile.Reset();
            }
            else
            {
                basePath = temporaryFile;
                SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            }
        }
    }

    if (remoteTextStatus > svn_wc_status_normal)
    {
        remotePath = CTempFiles::Instance().GetTempFilePath(false, filePath, SVNRev::REV_HEAD);

        CProgressDlg progressDlg;
        progressDlg.SetTitle(IDS_APPNAME);
        progressDlg.SetTime(false);
        m_pSVN->SetAndClearProgressInfo(&progressDlg, true); // activate progress bar
        progressDlg.ShowModeless(GetHWND());
        progressDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, static_cast<LPCWSTR>(filePath.GetUIFileOrDirectoryName()));
        remoteRev = SVNRev::REV_HEAD;
        if (!m_pSVN->Export(filePath, remotePath, remoteRev, remoteRev))
        {
            progressDlg.Stop();
            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            m_pSVN->ShowErrorDialog(GetHWND());
            return false;
        }
        progressDlg.Stop();
        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
        SetFileAttributes(remotePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    }

    CString name = filePath.GetUIFileOrDirectoryName();
    CString n1, n2, n3;
    n1.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(name));
    if (baseRev)
        n2.FormatMessage(IDS_DIFF_BASENAMEREV, static_cast<LPCWSTR>(name), baseRev);
    else
        n2.Format(IDS_DIFF_BASENAME, static_cast<LPCWSTR>(name));
    n3.Format(IDS_DIFF_REMOTENAME, static_cast<LPCWSTR>(name));

    if ((textStatus <= svn_wc_status_normal) && (propStatus <= svn_wc_status_normal) && (status <= svn_wc_status_normal))
    {
        // Hasn't changed locally - diff remote against WC
        return CAppUtils::StartExtDiff(
            filePath, remotePath, n1, n3, filePath, filePath, SVNRev::REV_WC, remoteRev, remoteRev, CAppUtils::DiffFlags().AlternativeTool(m_bAlternativeTool),
            m_jumpLine, filePath.GetFileOrDirectoryName(), L"");
    }
    else if (remotePath.IsEmpty())
    {
        return DiffFileAgainstBase(hParent, filePath, baseRev, ignoreProps, status, textStatus, propStatus);
    }
    else
    {
        // Three-way diff
        CAppUtils::MergeFlags flags;
        flags.bAlternativeTool = m_bAlternativeTool;
        flags.bReadOnly        = true;
        return !!CAppUtils::StartExtMerge(flags,
                                          basePath, remotePath, filePath, CTSVNPath(), false, n2, n3, n1, CString(), filePath.GetFileOrDirectoryName());
    }
}

bool SVNDiff::DiffFileAgainstBase(HWND hParent, const CTSVNPath& filePath, svn_revnum_t& baseRev, bool ignoreProps, svn_wc_status_kind status /*= svn_wc_status_none*/, svn_wc_status_kind textStatus /*= svn_wc_status_none*/, svn_wc_status_kind propStatus /*= svn_wc_status_none*/) const
{
    bool retValue     = false;
    bool fileExternal = false;
    if ((textStatus == svn_wc_status_none) || (propStatus == svn_wc_status_none))
    {
        SVNStatus stat;
        stat.GetStatus(filePath);
        if (stat.status == nullptr)
            return false;
        textStatus   = stat.status->text_status;
        propStatus   = stat.status->prop_status;
        fileExternal = stat.status->file_external != 0;
    }
    if (!ignoreProps && (propStatus > svn_wc_status_normal))
    {
        DiffProps(filePath, SVNRev::REV_WC, SVNRev::REV_BASE, baseRev);
    }

    if (filePath.IsDirectory())
        return true;
    if ((status >= svn_wc_status_normal) || (textStatus >= svn_wc_status_normal))
    {
        CTSVNPath basePath(SVN::GetPristinePath(hParent, filePath));
        if (baseRev == 0)
        {
            SVNInfo            info;
            const SVNInfoData* infoData = info.GetFirstFileInfo(filePath, SVNRev(), SVNRev());

            if (infoData)
            {
                if (infoData->copyFromUrl && infoData->copyFromUrl[0])
                    baseRev = infoData->copyFromRev;
                else
                    baseRev = infoData->lastChangedRev;
            }
        }
        // If necessary, convert the line-endings on the file before diffing
        // note: file externals can not be exported
        if (static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\ConvertBase", TRUE)) && (!fileExternal))
        {
            CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            if (!m_pSVN->Export(filePath, temporaryFile, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE)))
            {
                temporaryFile.Reset();
            }
            else
            {
                basePath = temporaryFile;
                SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            }
        }
        // for added/deleted files, we don't have a BASE file.
        // create an empty temp file to be used.
        if (!basePath.Exists())
        {
            basePath = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
        }
        CString   name       = filePath.GetFilename();
        CTSVNPath wcFilePath = filePath;
        if (!wcFilePath.Exists())
        {
            wcFilePath = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
            SetFileAttributes(wcFilePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
        }
        CString n1, n2;
        n1.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(name));
        if (baseRev)
            n2.FormatMessage(IDS_DIFF_BASENAMEREV, static_cast<LPCWSTR>(name), baseRev);
        else
            n2.Format(IDS_DIFF_BASENAME, static_cast<LPCWSTR>(name));
        retValue = CAppUtils::StartExtDiff(
            basePath, wcFilePath, n2, n1,
            filePath, filePath, SVNRev::REV_BASE, SVNRev::REV_WC, SVNRev::REV_BASE,
            CAppUtils::DiffFlags().Wait().AlternativeTool(m_bAlternativeTool), m_jumpLine, name, L"");
    }
    return retValue;
}

bool SVNDiff::UnifiedDiff(CTSVNPath& tempFile, const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg, bool prettyPrint, const CString& options, bool bIgnoreAncestry /* = false */, bool bIgnoreProperties /* = true */) const
{
    tempFile            = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, CTSVNPath(L"Test.diff"));
    bool         bIsUrl = !!SVN::PathIsURL(url1);

    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_UNIFIEDDIFF)));
    progDlg.SetTime(false);
    m_pSVN->SetAndClearProgressInfo(&progDlg);
    progDlg.ShowModeless(GetHWND());
    // find the root of the files
    CTSVNPathList plist;
    plist.AddPath(url1);
    plist.AddPath(url2);
    CTSVNPath relativeTo = plist.GetCommonRoot();
    if (!relativeTo.IsUrl())
    {
        if (!relativeTo.IsDirectory())
            relativeTo = relativeTo.GetContainingDirectory();
    }
    if (relativeTo.IsEmpty() && url1.Exists() && url2.IsUrl())
    {
        // the source path exists, i.e. it's a local path, so
        // use this as the relative url
        relativeTo = url1.GetDirectory();
    }
    // the 'relativeTo' path must be a path: svn throws an error if it's used for urls.
    else if ((!url2.IsEquivalentTo(url1) && (relativeTo.IsEquivalentTo(url1) || relativeTo.IsEquivalentTo(url2))) || url1.IsUrl() || url2.IsUrl())
        relativeTo.Reset();
    if ((!url1.IsEquivalentTo(url2)) || ((rev1.IsWorking() || rev1.IsBase()) && (rev2.IsWorking() || rev2.IsBase())))
    {
        if (!m_pSVN->Diff(url1, rev1, url2, rev2, relativeTo, svn_depth_infinity, true, false, false, false, false, false, bIgnoreProperties, false, prettyPrint, options, bIgnoreAncestry, tempFile))
        {
            progDlg.Stop();
            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            m_pSVN->ShowErrorDialog(GetHWND());
            return false;
        }
    }
    else
    {
        if (!m_pSVN->PegDiff(url1, (peg.IsValid() ? peg : (bIsUrl ? m_headPeg : SVNRev::REV_WC)), rev1, rev2, relativeTo, svn_depth_infinity, true, false, false, false, false, false, bIgnoreProperties, false, prettyPrint, options, false, tempFile))
        {
            if (!m_pSVN->Diff(url1, rev1, url2, rev2, relativeTo, svn_depth_infinity, true, false, false, false, false, false, bIgnoreProperties, false, prettyPrint, options, false, tempFile))
            {
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                m_pSVN->ShowErrorDialog(GetHWND());
                return false;
            }
        }
    }
    if (CAppUtils::CheckForEmptyDiff(tempFile))
    {
        progDlg.Stop();
        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
        TaskDialog(GetHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_EMPTYDIFF), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
        return false;
    }
    progDlg.Stop();
    m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
    return true;
}

bool SVNDiff::ShowUnifiedDiff(const CTSVNPath& url1, const SVNRev& rev1,
                              const CTSVNPath& url2, const SVNRev& rev2,
                              const SVNRev&  peg,
                              bool           prettyPrint,
                              const CString& options,
                              bool           bIgnoreAncestry /* = false */,
                              bool /*blame*/,
                              bool bIgnoreProperties /* = true */) const
{
    CTSVNPath tempFile;
    if (UnifiedDiff(tempFile, url1, rev1, url2, rev2, peg, prettyPrint, options, bIgnoreAncestry, bIgnoreProperties))
    {
        CString       title;
        CTSVNPathList list;
        list.AddPath(url1);
        list.AddPath(url2);
        if (url1.IsEquivalentTo(url2))
            title.FormatMessage(IDS_SVNDIFF_ONEURL, static_cast<LPCWSTR>(rev1.ToString()), static_cast<LPCWSTR>(rev2.ToString()), static_cast<LPCWSTR>(url1.GetUIFileOrDirectoryName()));
        else
        {
            CTSVNPath root = list.GetCommonRoot();
            CString   u1   = url1.GetUIPathString().Mid(root.GetUIPathString().GetLength());
            CString   u2   = url2.GetUIPathString().Mid(root.GetUIPathString().GetLength());
            title.FormatMessage(IDS_SVNDIFF_TWOURLS, static_cast<LPCWSTR>(rev1.ToString()), static_cast<LPCWSTR>(u1), static_cast<LPCWSTR>(rev2.ToString()), static_cast<LPCWSTR>(u2));
        }
        return !!CAppUtils::StartUnifiedDiffViewer(tempFile.GetWinPathString(), title);
    }
    return false;
}

bool SVNDiff::ShowCompare(const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, SVNRev peg, bool ignoreProps, bool prettyPrint, const CString& options, bool ignoreAncestry /*= false*/, bool blame /*= false*/, svn_node_kind_t nodeKind /*= svn_node_unknown*/) const
{
    CTSVNPath    tempFile;
    CString      mimeType;
    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    progDlg.SetTime(false);
    m_pSVN->SetAndClearProgressInfo(&progDlg);
    CAppUtils::DiffFlags diffFlags;
    diffFlags.ReadOnly().AlternativeTool(m_bAlternativeTool);

    if ((m_pSVN->PathIsURL(url1)) || (!rev1.IsWorking()) || (!url1.IsEquivalentTo(url2)))
    {
        // no working copy path!
        progDlg.ShowModeless(GetHWND());

        tempFile = CTempFiles::Instance().GetTempFilePath(false, url1);
        // first find out if the url points to a file or dir
        CString sRepoRoot;
        if ((nodeKind != svn_node_dir) && (nodeKind != svn_node_file))
        {
            progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_INFO)));
            SVNInfo            info;
            const SVNInfoData* data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : m_headPeg), rev1, svn_depth_empty);
            if (data == nullptr)
            {
                data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev1), rev1, svn_depth_empty);
                if (data == nullptr)
                {
                    data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev2), rev1, svn_depth_empty);
                    if (data == nullptr)
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        info.ShowErrorDialog(GetHWND());
                        return false;
                    }
                    else
                    {
                        sRepoRoot = data->reposRoot;
                        nodeKind  = data->kind;
                        peg       = peg.IsValid() ? peg : rev2;
                    }
                }
                else
                {
                    sRepoRoot = data->reposRoot;
                    nodeKind  = data->kind;
                    peg       = peg.IsValid() ? peg : rev1;
                }
            }
            else
            {
                sRepoRoot = data->reposRoot;
                nodeKind  = data->kind;
                peg       = peg.IsValid() ? peg : m_headPeg;
            }
        }
        else
        {
            sRepoRoot = m_pSVN->GetRepositoryRoot(url1);
            peg       = peg.IsValid() ? peg : m_headPeg;
        }
        if (nodeKind == svn_node_dir)
        {
            if (rev1.IsWorking())
            {
                if (UnifiedDiff(tempFile, url1, rev1, url2, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC), prettyPrint, options))
                {
                    CString sWc;
                    sWc.LoadString(IDS_DIFF_WORKINGCOPY);
                    progDlg.Stop();
                    m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                    return !!CAppUtils::StartExtPatch(tempFile, url1.GetDirectory(), sWc, url2.GetSVNPathString(), TRUE);
                }
            }
            else
            {
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                CFileDiffDlg fDlg;
                fDlg.DoBlame(blame);
                if (url1.IsEquivalentTo(url2))
                {
                    fDlg.SetDiff(url1, (peg.IsValid() ? peg : m_headPeg), rev1, rev2, svn_depth_infinity, ignoreAncestry);
                    fDlg.DoModal();
                }
                else
                {
                    fDlg.SetDiff(url1, rev1, url2, rev2, svn_depth_infinity, ignoreAncestry);
                    fDlg.DoModal();
                }
            }
        }
        else
        {
            if (url1.IsEquivalentTo(url2) && !ignoreProps)
            {
                svn_revnum_t baseRev = 0;
                DiffProps(url1, rev2, rev1, baseRev);
            }
            // diffing two revs of a file, so export two files
            CTSVNPath tempFile1 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, blame ? CTSVNPath() : url1, rev1);
            CTSVNPath tempFile2 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, blame ? CTSVNPath() : url2, rev2);

            m_pSVN->SetAndClearProgressInfo(&progDlg, true); // activate progress bar
            progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, static_cast<LPCWSTR>(url1.GetUIFileOrDirectoryName()), static_cast<LPCWSTR>(rev1.ToString()));
            CAppUtils::GetMimeType(url1, mimeType, rev1);
            CBlame blamer;
            blamer.SetAndClearProgressInfo(&progDlg, true);
            if (blame)
            {
                if (!blamer.BlameToFile(url1, 1, rev1, peg.IsValid() ? peg : rev1, tempFile1, options, TRUE, TRUE))
                {
                    if ((peg.IsValid()) && (blamer.GetSVNError()->apr_err != SVN_ERR_CLIENT_IS_BINARY_FILE))
                    {
                        if (!blamer.BlameToFile(url1, 1, rev1, rev1, tempFile1, options, TRUE, TRUE))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            blamer.ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        if (blamer.GetSVNError()->apr_err != SVN_ERR_CLIENT_IS_BINARY_FILE)
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        }
                        blamer.ShowErrorDialog(GetHWND());
                        if (blamer.GetSVNError()->apr_err == SVN_ERR_CLIENT_IS_BINARY_FILE)
                            blame = false;
                        else
                            return false;
                    }
                }
            }
            if (!blame)
            {
                bool tryWorking = (!m_pSVN->PathIsURL(url1) && rev1.IsWorking() && PathFileExists(url1.GetWinPath()));
                if (!m_pSVN->Export(url1, tempFile1, peg.IsValid() && !tryWorking ? peg : rev1, rev1))
                {
                    if (peg.IsValid())
                    {
                        if (!m_pSVN->Export(url1, tempFile1, rev1, rev1))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
            }
            SetFileAttributes(tempFile1.GetWinPath(), FILE_ATTRIBUTE_READONLY);

            progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, static_cast<LPCWSTR>(url2.GetUIFileOrDirectoryName()), static_cast<LPCWSTR>(rev2.ToString()));
            progDlg.SetProgress(50, 100);
            if (blame)
            {
                if (!blamer.BlameToFile(url2, 1, rev2, peg.IsValid() ? peg : rev2, tempFile2, options, TRUE, TRUE))
                {
                    if (peg.IsValid())
                    {
                        if (!blamer.BlameToFile(url2, 1, rev2, rev2, tempFile2, options, TRUE, TRUE))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
            }
            else
            {
                if (!m_pSVN->Export(url2, tempFile2, peg.IsValid() ? peg : rev2, rev2))
                {
                    if (peg.IsValid())
                    {
                        if (!m_pSVN->Export(url2, tempFile2, rev2, rev2))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
            }
            SetFileAttributes(tempFile2.GetWinPath(), FILE_ATTRIBUTE_READONLY);

            progDlg.SetProgress(100, 100);
            progDlg.Stop();
            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));

            CString revName1, revName2, fName;
            if (url1.IsEquivalentTo(url2))
            {
                revName1.Format(L"%s Revision %s", static_cast<LPCWSTR>(url1.GetUIFileOrDirectoryName()), static_cast<LPCWSTR>(rev1.ToString()));
                revName2.Format(L"%s Revision %s", static_cast<LPCWSTR>(url2.GetUIFileOrDirectoryName()), static_cast<LPCWSTR>(rev2.ToString()));
                fName = url1.GetUIFileOrDirectoryName();
            }
            else
            {
                if (sRepoRoot.IsEmpty())
                {
                    revName1.Format(L"%s Revision %s", static_cast<LPCWSTR>(url1.GetSVNPathString()), static_cast<LPCWSTR>(rev1.ToString()));
                    revName2.Format(L"%s Revision %s", static_cast<LPCWSTR>(url2.GetSVNPathString()), static_cast<LPCWSTR>(rev2.ToString()));
                }
                else
                {
                    if (url1.IsUrl())
                        revName1.Format(L"%s Revision %s", static_cast<LPCWSTR>(url1.GetSVNPathString().Mid(sRepoRoot.GetLength())), static_cast<LPCWSTR>(rev1.ToString()));
                    else
                        revName1.Format(L"%s Revision %s", static_cast<LPCWSTR>(url1.GetSVNPathString()), static_cast<LPCWSTR>(rev1.ToString()));
                    if (url2.IsUrl() && (url2.GetSVNPathString().Left(sRepoRoot.GetLength()).Compare(sRepoRoot) == 0))
                        revName2.Format(L"%s Revision %s", static_cast<LPCWSTR>(url2.GetSVNPathString().Mid(sRepoRoot.GetLength())), static_cast<LPCWSTR>(rev2.ToString()));
                    else
                        revName2.Format(L"%s Revision %s", static_cast<LPCWSTR>(url2.GetSVNPathString()), static_cast<LPCWSTR>(rev2.ToString()));
                }
            }
            return CAppUtils::StartExtDiff(tempFile1, tempFile2, revName1, revName2, url1, url2, rev1, rev2, peg, diffFlags.Blame(blame), m_jumpLine, fName, mimeType);
        }
    }
    else
    {
        // compare with working copy
        if (PathIsDirectory(url1.GetWinPath()))
        {
            if (UnifiedDiff(tempFile, url1, rev1, url1, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC), prettyPrint, options))
            {
                CString sWc, sRev;
                sWc.LoadString(IDS_DIFF_WORKINGCOPY);
                sRev.Format(IDS_DIFF_REVISIONPATCHED, static_cast<LONG>(rev2));
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                return !!CAppUtils::StartExtPatch(tempFile, url1.GetDirectory(), sWc, sRev, TRUE);
            }
        }
        else
        {
            ASSERT(rev1.IsWorking());

            if (url1.IsEquivalentTo(url2) && !ignoreProps)
            {
                svn_revnum_t baseRev = 0;
                DiffProps(url1, rev1, rev2, baseRev);
            }

            m_pSVN->SetAndClearProgressInfo(&progDlg, true); // activate progress bar
            progDlg.ShowModeless(GetHWND());
            progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, static_cast<LPCWSTR>(url1.GetUIFileOrDirectoryName()), static_cast<LPCWSTR>(rev2.ToString()));

            tempFile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url1, rev2);
            if (blame)
            {
                CBlame blamer;
                if (!blamer.BlameToFile(url1, 1, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC), tempFile, options, TRUE, TRUE))
                {
                    if (peg.IsValid())
                    {
                        if (!blamer.BlameToFile(url1, 1, rev2, SVNRev::REV_WC, tempFile, options, TRUE, TRUE))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                SetFileAttributes(tempFile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
                CTSVNPath tempFile2 = CTempFiles::Instance().GetTempFilePath(false, url1);
                if (!blamer.BlameToFile(url1, 1, SVNRev::REV_WC, SVNRev::REV_WC, tempFile2, options, TRUE, TRUE))
                {
                    progDlg.Stop();
                    m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                    m_pSVN->ShowErrorDialog(GetHWND());
                    return false;
                }
                CString revName, wcName;
                revName.Format(L"%s Revision %ld", static_cast<LPCWSTR>(url1.GetFilename()), static_cast<LONG>(rev2));
                wcName.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(url1.GetFilename()));
                m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                return CAppUtils::StartExtDiff(tempFile, tempFile2, revName, wcName, url1, url2, rev1, rev2, peg, diffFlags, m_jumpLine, url1.GetFileOrDirectoryName(), L"");
            }
            else
            {
                if (!m_pSVN->Export(url1, tempFile, (peg.IsValid() ? peg : SVNRev::REV_WC), rev2))
                {
                    if (peg.IsValid())
                    {
                        if (!m_pSVN->Export(url1, tempFile, SVNRev::REV_WC, rev2))
                        {
                            progDlg.Stop();
                            m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            m_pSVN->ShowErrorDialog(GetHWND());
                            return false;
                        }
                    }
                    else
                    {
                        progDlg.Stop();
                        m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        m_pSVN->ShowErrorDialog(GetHWND());
                        return false;
                    }
                }
                progDlg.Stop();
                m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                SetFileAttributes(tempFile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
                CString revName, wcName;
                revName.Format(L"%s Revision %s", static_cast<LPCWSTR>(url1.GetFilename()), static_cast<LPCWSTR>(rev2.ToString()));
                wcName.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(url1.GetFilename()));
                return CAppUtils::StartExtDiff(tempFile, url1, revName, wcName, url1, url1, rev2, rev1, peg, diffFlags, m_jumpLine, url1.GetFileOrDirectoryName(), L"");
            }
        }
    }
    m_pSVN->SetAndClearProgressInfo(static_cast<HWND>(nullptr));
    return false;
}

bool SVNDiff::DiffProps(const CTSVNPath& filePath, const SVNRev& rev1, const SVNRev& rev2, svn_revnum_t& baseRev) const
{
    bool           retValue = false;
    // diff the properties
    SVNProperties  propsWc(filePath, rev1, false, false);
    SVNProperties  propsBase(filePath, rev2, false, false);

    constexpr auto MAX_PATH_LENGTH    = 80;
    WCHAR          pathBuf1[MAX_PATH] = {0};
    if (filePath.GetWinPathString().GetLength() >= MAX_PATH)
    {
        std::wstring str = filePath.GetWinPath();
        std::wregex  rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
        std::wstring replacement = L"$1$2...$3";
        std::wstring str2        = std::regex_replace(str, rx, replacement);
        if (str2.size() >= MAX_PATH)
            str2 = str2.substr(0, MAX_PATH - 2);
        PathCompactPathEx(pathBuf1, str2.c_str(), MAX_PATH_LENGTH, 0);
    }
    else
        PathCompactPathEx(pathBuf1, filePath.GetWinPath(), MAX_PATH_LENGTH, 0);

    if ((baseRev == 0) && (!filePath.IsUrl()) && (rev1.IsBase() || rev2.IsBase()))
    {
        SVNStatus            stat;
        CTSVNPath            dummy;
        svn_client_status_t* s = stat.GetFirstFileStatus(filePath, dummy);
        if (s)
            baseRev = s->revision;
    }
    // check for properties that got removed
    for (int baseindex = 0; baseindex < propsBase.GetCount(); ++baseindex)
    {
        std::string  baseName  = propsBase.GetItemName(baseindex);
        std::wstring baseNameU = CUnicodeUtils::StdGetUnicode(baseName);
        std::wstring baseValue = static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(propsBase.GetItemValue(baseindex).c_str()));
        bool         bFound    = false;
        for (int wcIndex = 0; wcIndex < propsWc.GetCount(); ++wcIndex)
        {
            if (baseName.compare(propsWc.GetItemName(wcIndex)) == 0)
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            // write the old property value to temporary file
            CTSVNPath wcPropFile   = CTempFiles::Instance().GetTempFilePath(false);
            CTSVNPath basePropFile = CTempFiles::Instance().GetTempFilePath(false);
            FILE*     pFile;
            _tfopen_s(&pFile, wcPropFile.GetWinPath(), L"wb");
            if (pFile)
            {
                fclose(pFile);
                FILE* pFile2;
                _tfopen_s(&pFile2, basePropFile.GetWinPath(), L"wb");
                if (pFile2)
                {
                    fputs(CUnicodeUtils::StdGetUTF8(baseValue).c_str(), pFile2);
                    fclose(pFile2);
                }
                else
                    return false;
            }
            else
                return false;
            SetFileAttributes(wcPropFile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            SetFileAttributes(basePropFile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            CString n1, n2;
            bool    bSwitch = false;
            if (rev1.IsWorking())
                n1.Format(IDS_DIFF_PROP_WCNAME, baseNameU.c_str());
            if (rev1.IsBase())
            {
                if (baseRev)
                    n1.FormatMessage(IDS_DIFF_PROP_BASENAMEREV, baseNameU.c_str(), baseRev);
                else
                    n1.Format(IDS_DIFF_PROP_BASENAME, baseNameU.c_str());
            }
            if (rev1.IsHead())
                n1.Format(IDS_DIFF_PROP_REMOTENAME, baseNameU.c_str());
            if (n1.IsEmpty())
            {
                CString temp;
                temp.Format(IDS_DIFF_REVISIONPATCHED, static_cast<LONG>(rev1));
                n1 = baseNameU.c_str();
                n1 += L" " + temp;
                bSwitch = true;
            }
            else
            {
                n1 = CString(pathBuf1) + L" - " + n1;
            }
            if (rev2.IsWorking())
                n2.Format(IDS_DIFF_PROP_WCNAME, baseNameU.c_str());
            if (rev2.IsBase())
            {
                if (baseRev)
                    n2.FormatMessage(IDS_DIFF_PROP_BASENAMEREV, baseNameU.c_str(), baseRev);
                else
                    n2.Format(IDS_DIFF_PROP_BASENAME, baseNameU.c_str());
            }
            if (rev2.IsHead())
                n2.Format(IDS_DIFF_PROP_REMOTENAME, baseNameU.c_str());
            if (n2.IsEmpty())
            {
                CString temp;
                temp.Format(IDS_DIFF_REVISIONPATCHED, static_cast<LONG>(rev2));
                n2 = baseNameU.c_str();
                n2 += L" " + temp;
                bSwitch = true;
            }
            else
            {
                n2 = CString(pathBuf1) + L" - " + n2;
            }
            if (bSwitch)
            {
                retValue = !!CAppUtils::StartExtDiffProps(wcPropFile, basePropFile, n1, n2, TRUE, TRUE);
            }
            else
            {
                retValue = !!CAppUtils::StartExtDiffProps(basePropFile, wcPropFile, n2, n1, TRUE, TRUE);
            }
        }
    }

    for (int wcindex = 0; wcindex < propsWc.GetCount(); ++wcindex)
    {
        std::string  wcName  = propsWc.GetItemName(wcindex);
        std::wstring wcNameU = CUnicodeUtils::StdGetUnicode(wcName);
        std::wstring wcValue = static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(propsWc.GetItemValue(wcindex).c_str()));
        std::wstring baseValue;
        bool         bDiffRequired = true;
        for (int baseIndex = 0; baseIndex < propsBase.GetCount(); ++baseIndex)
        {
            if (propsBase.GetItemName(baseIndex).compare(wcName) == 0)
            {
                baseValue = CUnicodeUtils::GetUnicode(propsBase.GetItemValue(baseIndex).c_str());
                if (baseValue.compare(wcValue) == 0)
                {
                    // name and value are identical
                    bDiffRequired = false;
                    break;
                }
            }
        }
        if (bDiffRequired)
        {
            // write both property values to temporary files
            CTSVNPath wcPropFile   = CTempFiles::Instance().GetTempFilePath(false);
            CTSVNPath basePropFile = CTempFiles::Instance().GetTempFilePath(false);
            FILE*     pFile;
            _tfopen_s(&pFile, wcPropFile.GetWinPath(), L"wb");
            if (pFile)
            {
                fputs(CUnicodeUtils::StdGetUTF8(wcValue).c_str(), pFile);
                fclose(pFile);
                FILE* pFile2;
                _tfopen_s(&pFile2, basePropFile.GetWinPath(), L"wb");
                if (pFile2)
                {
                    fputs(CUnicodeUtils::StdGetUTF8(baseValue).c_str(), pFile2);
                    fclose(pFile2);
                }
                else
                    return false;
            }
            else
                return false;
            SetFileAttributes(wcPropFile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            SetFileAttributes(basePropFile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            CString n1, n2;
            if (rev1.IsWorking())
                n1.Format(IDS_DIFF_WCNAME, wcNameU.c_str());
            if (rev1.IsBase())
                n1.Format(IDS_DIFF_BASENAME, wcNameU.c_str());
            if (rev1.IsHead())
                n1.Format(IDS_DIFF_REMOTENAME, wcNameU.c_str());
            if (n1.IsEmpty())
                n1.FormatMessage(IDS_DIFF_PROP_REVISIONNAME, wcNameU.c_str(), static_cast<LPCWSTR>(rev1.ToString()));
            else
                n1 = CString(pathBuf1) + L" - " + n1;
            if (rev2.IsWorking())
                n2.Format(IDS_DIFF_WCNAME, wcNameU.c_str());
            if (rev2.IsBase())
                n2.Format(IDS_DIFF_BASENAME, wcNameU.c_str());
            if (rev2.IsHead())
                n2.Format(IDS_DIFF_REMOTENAME, wcNameU.c_str());
            if (n2.IsEmpty())
                n2.FormatMessage(IDS_DIFF_PROP_REVISIONNAME, wcNameU.c_str(), static_cast<LPCWSTR>(rev2.ToString()));
            else
                n2 = CString(pathBuf1) + L" - " + n2;
            retValue = !!CAppUtils::StartExtDiffProps(basePropFile, wcPropFile, n2, n1, TRUE, TRUE);
        }
    }
    return retValue;
}