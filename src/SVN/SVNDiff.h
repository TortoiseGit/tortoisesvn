// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2011, 2013-2014, 2016, 2018, 2021-2022 - TortoiseSVN

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
#include "SVNRev.h"
#include "SVN.h"

/**
 * \ingroup SVN
 * Provides methods to diff URL's and paths.
 *
 * \remark this class shows message boxes for errors.
 */
class SVNDiff
{
public:
    // delete copy constructor to prevent accidental copies
    SVNDiff(const SVNDiff& /*d*/) = delete;
    SVNDiff(SVN* pSVN = nullptr, HWND hWnd = nullptr, bool bRemoveTempFiles = false);
    ~SVNDiff();

    void SetAlternativeTool(bool bAlternativeTool) { m_bAlternativeTool = bAlternativeTool; }
    void SetJumpLine(int line) { m_jumpLine = line; }

    /**
     * Diff a single file against its text-base
     * \param filePath The file to diff
     * \param baseRev
     * \param status
     * \param textStatus
     * \param propStatus
     */
    bool DiffFileAgainstBase(HWND hParent, const CTSVNPath& filePath, svn_revnum_t& baseRev, bool ignoreProps, svn_wc_status_kind status = svn_wc_status_none, svn_wc_status_kind textStatus = svn_wc_status_none, svn_wc_status_kind propStatus = svn_wc_status_none) const;

    /**
     * Shows a diff of a file in the working copy with its BASE.
     */
    bool DiffWCFile(HWND hParent, const CTSVNPath& filePath,
                    bool               ignoreProps,
                    svn_wc_status_kind status           = svn_wc_status_none,
                    svn_wc_status_kind textStatus       = svn_wc_status_none,
                    svn_wc_status_kind propStatus       = svn_wc_status_none,
                    svn_wc_status_kind remoteTextStatus = svn_wc_status_none,
                    svn_wc_status_kind remotePropStatus = svn_wc_status_none) const;

    /**
     * Produces a unified diff of the \a url1 in \a rev1 and \a url2 in \a rev2 and shows
     * it in the unified diff viewer.
     * If \a peg revision isn't specified, then the following algorithm is used to find
     * the peg revision:\n
     * If \a url points to a local path, then WC is used as the peg revision, otherwise
     * HEAD is used for the peg revision.
     *
     * \remark the peg revision is only used if \a url1 is the same as \a url2
     */
    bool ShowUnifiedDiff(const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg, bool prettyPrint, const CString& options, bool bIgnoreAncestry = false, bool /*blame*/ = false, bool bIgnoreProperties = true) const;

    /**
     * See ShowUnifiedDiff().
     * Unlike ShowUnifiedDiff(), this method returns the path to the saved unified diff
     * without starting the diff viewer.
     */
    bool UnifiedDiff(CTSVNPath& tempFile, const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg, bool prettyPrint, const CString& options, bool bIgnoreAncestry = false, bool bIgnoreProperties = true) const;

    /**
     * Compares two revisions of a path and shows them in a GUI.
     *
     * To compare one url in two revisions, pass the same url for \a url1
     * and \a url2.
     * Note: if you pass two different urls, the \a peg revision is ignored.
     *
     * If \a url1 is a working copy path and \a rev1 is REV_WC, then \a rev2 is
     * compared against the existing working copy.
     *
     * In case \a url1 is an URL and not a local path, then the file diff dialog
     * is used to show the diff.
     */
    bool ShowCompare(const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, SVNRev peg, bool ignoreProps, bool prettyPrint, const CString& options, bool ignoreAncestry = false, bool blame = false, svn_node_kind_t nodeKind = svn_node_unknown) const;

    bool DiffProps(const CTSVNPath& filePath, const SVNRev& rev1, const SVNRev& rev2, svn_revnum_t& baseRev) const;

    /**
     * Sets the Peg revision to use instead of HEAD.
     */
    void SetHEADPeg(const SVNRev& headPeg) { m_headPeg = headPeg; }

private:
    HWND GetHWND() const { return (::IsWindow(m_hWnd) ? m_hWnd : nullptr); }

private:
    SVN*   m_pSVN;
    bool   m_bDeleteSVN;
    HWND   m_hWnd;
    bool   m_bRemoveTempFiles;
    SVNRev m_headPeg;
    bool   m_bAlternativeTool;
    int    m_jumpLine;
};
