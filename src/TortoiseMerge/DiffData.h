// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2008, 2010-2014, 2017, 2020-2021 - TortoiseSVN

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
#pragma once

#pragma warning(push)
#include "svn_diff.h"
#include "apr_pools.h"
#pragma warning(pop)
#include "FileTextLines.h"
#include "WorkingFile.h"
#include "ViewData.h"
#include "MovedBlocks.h"

#define DIFF_EMPTYLINENUMBER ((DWORD)-1)

enum class IgnoreWS : int
{
    None           = 0,
    AllWhiteSpaces = 1,
    WhiteSpaces    = 2, // whitespaces at the beginning of a line
};

/**
 * \ingroup TortoiseMerge
 * Main class for handling diffs.
 */
class CDiffData
{
public:
    CDiffData();
    virtual ~CDiffData();

    BOOL    Load();
    void    SetBlame(bool bBlame = true) { m_bBlame = bBlame; }
    void    SetMovedBlocks(bool bViewMovedBlocks = true);
    int     GetLineCount() const;
    CString GetError() const { return m_sError; }
    void    SetCommentTokens(const CString& sLineStart, const CString& sBlockStart, const CString& sBlockEnd);
    void    SetRegexTokens(const std::wregex& rx, const std::wstring& replacement);

    bool IsBaseFileInUse() const { return m_baseFile.InUse(); }
    bool IsTheirFileInUse() const { return m_theirFile.InUse(); }
    bool IsYourFileInUse() const { return m_yourFile.InUse(); }

private:
    bool DoTwoWayDiff(const CString& sBaseFilename, const CString& sYourFilename, IgnoreWS ignoreWs, bool bIgnoreEOL, bool bIgnoreCase, bool bIgnoreComments, apr_pool_t* pool);

    void StickAndSkip(svn_diff_t*& tempDiff, apr_off_t& originalLengthSticked, apr_off_t& modifiedLengthSticked) const;
    bool DoThreeWayDiff(const CString& sBaseFilename, const CString& sYourFilename, const CString& sTheirFilename, IgnoreWS ignoreWs, bool bIgnoreEOL, bool bIgnoreCase, bool bIgnoreComments, apr_pool_t* pool);
    /**
     * Moved blocks detection for further highlighting,
     * implemented exclusively for TwoWayDiff
     **/
    tsvn_svn_diff_t_extension* MovedBlocksDetect(svn_diff_t* diffYourBase, IgnoreWS ignoreWs, apr_pool_t* pool) const;

    void TieMovedBlocks(int from, int to, apr_off_t length);

    static void HideUnchangedSections(CViewData* data1, CViewData* data2, CViewData* data3);

    static svn_diff_file_ignore_space_t GetIgnoreSpaceMode(IgnoreWS ignoreWs);
    static svn_diff_file_options_t*     CreateDiffFileOptions(IgnoreWS ignoreWs, bool bIgnoreEOL, apr_pool_t* pool);
    bool                                HandleSvnError(svn_error_t* svnerr);
    static bool                         CompareWithIgnoreWS(CString s1, CString s2, IgnoreWS ignoreWs);

public:
    CWorkingFile m_baseFile;
    CWorkingFile m_theirFile;
    CWorkingFile m_yourFile;
    CWorkingFile m_mergedFile;

    CString m_sDiffFile;
    CString m_sPatchPath;
    CString m_sPatchOriginal;
    CString m_sPatchPatched;
    bool    m_bPatchRequired;

public:
    CFileTextLines m_arBaseFile;
    CFileTextLines m_arTheirFile;
    CFileTextLines m_arYourFile;
    CViewData      m_yourBaseBoth;   ///< one-pane view, diff between 'yours' and 'base' (in three-pane view: right view)
    CViewData      m_yourBaseLeft;   ///< two-pane view, diff between 'yours' and 'base', left view
    CViewData      m_yourBaseRight;  ///< two-pane view, diff between 'yours' and 'base', right view
    CViewData      m_theirBaseBoth;  ///< one-pane view, diff between 'theirs' and 'base' (in three-pane view: left view)
    CViewData      m_theirBaseLeft;  ///< two-pane view, diff between 'theirs' and 'base', left view
    CViewData      m_theirBaseRight; ///< two-pane view, diff between 'theirs' and 'base', right view
    CViewData      m_diff3;          ///< three-pane view, bottom pane
    CString        m_sError;

    static int abort_on_pool_failure(int retcode);

protected:
    bool         m_bBlame;
    bool         m_bViewMovedBlocks;
    CString      m_commentLineStart;
    CString      m_commentBlockStart;
    CString      m_commentBlockEnd;
    std::wregex  m_rx;
    std::wstring m_replacement;
};
