// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2017, 2020-2021 - TortoiseSVN

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
#pragma warning(push)
#include "diff.h"
#pragma warning(pop)
#include "TempFile.h"
#include "registry.h"
#include "resource.h"
#include "DiffData.h"
#include "UnicodeUtils.h"
#include "SVNAdminDir.h"
#include "svn_dso.h"
#include "MovedBlocks.h"

#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
int CDiffData::abort_on_pool_failure(int /*retcode*/)
{
    abort();
    // ReSharper disable once CppUnreachableCode
    return -1;
}
#pragma warning(pop)

CDiffData::CDiffData()
    : m_bPatchRequired(false)
    , m_bViewMovedBlocks(false)
{
    apr_initialize();
    svn_dso_initialize2();
    g_SVNAdminDir.Init();

    m_bBlame = false;

    m_sPatchOriginal = L": original";
    m_sPatchPatched  = L": patched";
}

CDiffData::~CDiffData()
{
    g_SVNAdminDir.Close();
    apr_terminate();
}

void CDiffData::SetMovedBlocks(bool bViewMovedBlocks /* = true*/)
{
    m_bViewMovedBlocks = bViewMovedBlocks;
}

int CDiffData::GetLineCount() const
{
    int count = static_cast<int>(m_arBaseFile.GetCount());
    if (count < m_arTheirFile.GetCount())
        count = m_arTheirFile.GetCount();
    if (count < m_arYourFile.GetCount())
        count = m_arYourFile.GetCount();
    return count;
}

svn_diff_file_ignore_space_t CDiffData::GetIgnoreSpaceMode(IgnoreWS ignoreWs)
{
    switch (ignoreWs)
    {
        case IgnoreWS::None:
            return svn_diff_file_ignore_space_none;
        case IgnoreWS::AllWhiteSpaces:
            return svn_diff_file_ignore_space_all;
        case IgnoreWS::WhiteSpaces:
            return svn_diff_file_ignore_space_change;
        default:
            return svn_diff_file_ignore_space_none;
    }
}

svn_diff_file_options_t* CDiffData::CreateDiffFileOptions(IgnoreWS ignoreWs, bool bIgnoreEOL, apr_pool_t* pool)
{
    svn_diff_file_options_t* options = svn_diff_file_options_create(pool);
    options->ignore_eol_style        = bIgnoreEOL;
    options->ignore_space            = GetIgnoreSpaceMode(ignoreWs);
    return options;
}

bool CDiffData::HandleSvnError(svn_error_t* svnerr)
{
    TRACE(L"diff-error in CDiffData::Load()\n");
    TRACE(L"diff-error in CDiffData::Load()\n");
    CStringA sMsg = CStringA(svnerr->message);
    while (svnerr->child)
    {
        svnerr = svnerr->child;
        sMsg += L"\n";
        sMsg += CStringA(svnerr->message);
    }
    CString readableMsg = CUnicodeUtils::GetUnicode(sMsg);
    m_sError.Format(IDS_ERR_DIFF_DIFF, static_cast<LPCWSTR>(readableMsg));
    svn_error_clear(svnerr);
    return false;
}

void CDiffData::TieMovedBlocks(int from, int to, apr_off_t length)
{
    for (int i = 0; i < length; i++, to++, from++)
    {
        int fromIndex = m_yourBaseLeft.FindLineNumber(from);
        if (fromIndex < 0)
            return;
        int toIndex = m_yourBaseRight.FindLineNumber(to);
        if (toIndex < 0)
            return;
        m_yourBaseLeft.SetMovedIndex(fromIndex, toIndex, true);
        m_yourBaseRight.SetMovedIndex(toIndex, fromIndex, false);

        toIndex = m_yourBaseBoth.FindLineNumber(to);
        if (toIndex < 0)
            return;
        while ((toIndex < m_yourBaseBoth.GetCount()) &&
               ((m_yourBaseBoth.GetState(toIndex) != DIFFSTATE_ADDED) &&
                    (!m_yourBaseBoth.IsMoved(toIndex)) ||
                (m_yourBaseBoth.IsMovedFrom(toIndex)) ||
                (m_yourBaseBoth.GetLineNumber(toIndex) != to)))
        {
            toIndex++;
        }

        fromIndex = m_yourBaseBoth.FindLineNumber(from);
        if (fromIndex < 0)
            return;
        while ((fromIndex < m_yourBaseBoth.GetCount()) &&
               ((m_yourBaseBoth.GetState(fromIndex) != DIFFSTATE_REMOVED) &&
                    (!m_yourBaseBoth.IsMoved(fromIndex)) ||
                (!m_yourBaseBoth.IsMovedFrom(fromIndex)) ||
                (m_yourBaseBoth.GetLineNumber(fromIndex) != from)))
        {
            fromIndex++;
        }
        if ((fromIndex < m_yourBaseBoth.GetCount()) && (toIndex < m_yourBaseBoth.GetCount()))
        {
            m_yourBaseBoth.SetMovedIndex(fromIndex, toIndex, true);
            m_yourBaseBoth.SetMovedIndex(toIndex, fromIndex, false);
        }
    }
}

bool CDiffData::CompareWithIgnoreWS(CString s1, CString s2, IgnoreWS ignoreWs)
{
    if (ignoreWs == IgnoreWS::WhiteSpaces)
    {
        s1 = s1.TrimLeft(L" \t");
        s2 = s2.TrimLeft(L" \t");
    }
    else
    {
        s1 = s1.TrimRight(L" \t");
        s2 = s2.TrimRight(L" \t");
    }

    return s1 != s2;
}

void CDiffData::StickAndSkip(svn_diff_t*& tempDiff, apr_off_t& originalLengthSticked, apr_off_t& modifiedLengthSticked) const
{
    if ((m_bViewMovedBlocks) && (tempDiff->type == svn_diff__type_diff_modified))
    {
        svn_diff_t* nextDiff = tempDiff->next;
        while ((nextDiff) && (nextDiff->type == svn_diff__type_diff_modified))
        {
            originalLengthSticked += nextDiff->original_length;
            modifiedLengthSticked += nextDiff->modified_length;
            tempDiff = nextDiff;
            nextDiff = tempDiff->next;
        }
    }
}

BOOL CDiffData::Load()
{
    m_arBaseFile.RemoveAll();
    m_arYourFile.RemoveAll();
    m_arTheirFile.RemoveAll();

    m_yourBaseBoth.Clear();
    m_yourBaseLeft.Clear();
    m_yourBaseRight.Clear();

    m_theirBaseBoth.Clear();
    m_theirBaseLeft.Clear();
    m_theirBaseRight.Clear();

    m_diff3.Clear();

    CRegDWORD regIgnoreWS       = CRegDWORD(L"Software\\TortoiseMerge\\IgnoreWS");
    CRegDWORD regIgnoreEOL      = CRegDWORD(L"Software\\TortoiseMerge\\IgnoreEOL", TRUE);
    CRegDWORD regIgnoreCase     = CRegDWORD(L"Software\\TortoiseMerge\\CaseInsensitive", FALSE);
    CRegDWORD regIgnoreComments = CRegDWORD(L"Software\\TortoiseMerge\\IgnoreComments", FALSE);
    IgnoreWS  ignoreWs          = static_cast<IgnoreWS>(static_cast<DWORD>(regIgnoreWS));
    bool      bIgnoreEOL        = static_cast<DWORD>(regIgnoreEOL) != 0;
    BOOL      bIgnoreCase       = static_cast<DWORD>(regIgnoreCase) != 0;
    bool      bIgnoreComments   = static_cast<DWORD>(regIgnoreComments) != 0;

    // The Subversion diff API only can ignore whitespaces and eol styles.
    // It also can only handle one-byte charsets.
    // To ignore case changes or to handle UTF-16 files, we have to
    // save the original file in UTF-8 and/or the letters changed to lowercase
    // so the Subversion diff can handle those.
    CString sConvertedBaseFilename  = m_baseFile.GetFilename();
    CString sConvertedYourFilename  = m_yourFile.GetFilename();
    CString sConvertedTheirFilename = m_theirFile.GetFilename();

    m_baseFile.StoreFileAttributes();
    m_theirFile.StoreFileAttributes();
    m_yourFile.StoreFileAttributes();
    //m_mergedFile.StoreFileAttributes();

    bool bBaseNeedConvert  = false;
    bool bTheirNeedConvert = false;
    bool bYourNeedConvert  = false;
    bool bBaseIsUtf8       = false;
    bool bTheirIsUtf8      = false;
    bool bYourIsUtf8       = false;

    if (IsBaseFileInUse())
    {
        if (!m_arBaseFile.Load(m_baseFile.GetFilename()))
        {
            m_sError = m_arBaseFile.GetErrorString();
            return FALSE;
        }
        bBaseNeedConvert = bIgnoreCase || bIgnoreComments || (m_arBaseFile.NeedsConversion()) || !m_rx._Empty();
        bBaseIsUtf8      = (m_arBaseFile.GetUnicodeType() != CFileTextLines::ASCII) || bBaseNeedConvert;
    }

    if (IsTheirFileInUse())
    {
        // m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
        // It's a fair guess that the files will be roughly the same size
        if (!m_arTheirFile.Load(m_theirFile.GetFilename(), m_arBaseFile.GetCount()))
        {
            m_sError = m_arTheirFile.GetErrorString();
            return FALSE;
        }
        bTheirNeedConvert = bIgnoreCase || bIgnoreComments || (m_arTheirFile.NeedsConversion()) || !m_rx._Empty();
        bTheirIsUtf8      = (m_arTheirFile.GetUnicodeType() != CFileTextLines::ASCII) || bTheirNeedConvert;
    }

    if (IsYourFileInUse())
    {
        // m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
        // It's a fair guess that the files will be roughly the same size
        if (!m_arYourFile.Load(m_yourFile.GetFilename(), m_arBaseFile.GetCount()))
        {
            m_sError = m_arYourFile.GetErrorString();
            return FALSE;
        }
        bYourNeedConvert = bIgnoreCase || bIgnoreComments || (m_arYourFile.NeedsConversion()) || !m_rx._Empty();
        bYourIsUtf8      = (m_arYourFile.GetUnicodeType() != CFileTextLines::ASCII) || bYourNeedConvert;
    }

    // in case at least one of the files got converted or is UTF8
    // we have to convert all non UTF8 (ASCII) files
    // otherwise one file might be in ANSI and the other in UTF8 and we'll end up
    // with lines marked as different throughout the files even though the lines
    // would show no change at all in the viewer.

    // convert all files we need to
    bool bIsUtf8 = bBaseIsUtf8 || bTheirIsUtf8 || bYourIsUtf8; // any file end as UTF8
    bBaseNeedConvert |= (IsBaseFileInUse() && !bBaseIsUtf8 && bIsUtf8);
    if (bBaseNeedConvert)
    {
        sConvertedBaseFilename = CTempFiles::Instance().GetTempFilePathString();
        m_baseFile.SetConvertedFileName(sConvertedBaseFilename);
        m_arBaseFile.Save(sConvertedBaseFilename, true, true, 0, bIgnoreCase, m_bBlame, bIgnoreComments, m_commentLineStart, m_commentBlockStart, m_commentBlockEnd, m_rx, m_replacement);
    }
    bYourNeedConvert |= (IsYourFileInUse() && !bYourIsUtf8 && bIsUtf8);
    if (bYourNeedConvert)
    {
        sConvertedYourFilename = CTempFiles::Instance().GetTempFilePathString();
        m_yourFile.SetConvertedFileName(sConvertedYourFilename);
        m_arYourFile.Save(sConvertedYourFilename, true, true, 0, bIgnoreCase, m_bBlame, bIgnoreComments, m_commentLineStart, m_commentBlockStart, m_commentBlockEnd, m_rx, m_replacement);
    }
    bTheirNeedConvert |= (IsTheirFileInUse() && !bTheirIsUtf8 && bIsUtf8);
    if (bTheirNeedConvert)
    {
        sConvertedTheirFilename = CTempFiles::Instance().GetTempFilePathString();
        m_theirFile.SetConvertedFileName(sConvertedTheirFilename);
        m_arTheirFile.Save(sConvertedTheirFilename, true, true, 0, bIgnoreCase, m_bBlame, bIgnoreComments, m_commentLineStart, m_commentBlockStart, m_commentBlockEnd, m_rx, m_replacement);
    }

    // Calculate the number of lines in the largest of the three files
    int lengthHint = GetLineCount();

    try
    {
        m_yourBaseBoth.Reserve(lengthHint);
        m_yourBaseLeft.Reserve(lengthHint);
        m_yourBaseRight.Reserve(lengthHint);

        m_theirBaseBoth.Reserve(lengthHint);
        m_theirBaseLeft.Reserve(lengthHint);
        m_theirBaseRight.Reserve(lengthHint);
    }
    catch (CMemoryException* e)
    {
        e->GetErrorMessage(m_sError.GetBuffer(255), 255);
        m_sError.ReleaseBuffer();
        e->Delete();
        return FALSE;
    }

    apr_pool_t* pool = nullptr;
    apr_pool_create_ex(&pool, nullptr, abort_on_pool_failure, nullptr);

    // Is this a two-way diff?
    if (IsBaseFileInUse() && IsYourFileInUse() && !IsTheirFileInUse())
    {
        if (!DoTwoWayDiff(sConvertedBaseFilename, sConvertedYourFilename, ignoreWs, bIgnoreEOL, !!bIgnoreCase, bIgnoreComments, pool))
        {
            apr_pool_destroy(pool); // free the allocated memory
            return FALSE;
        }
    }

    if (IsBaseFileInUse() && IsTheirFileInUse() && !IsYourFileInUse())
    {
        ASSERT(FALSE);
    }

    // Is this a three-way diff?
    if (IsBaseFileInUse() && IsTheirFileInUse() && IsYourFileInUse())
    {
        m_diff3.Reserve(lengthHint);

        if (!DoThreeWayDiff(sConvertedBaseFilename, sConvertedYourFilename, sConvertedTheirFilename, ignoreWs, bIgnoreEOL, !!bIgnoreCase, bIgnoreComments, pool))
        {
            apr_pool_destroy(pool); // free the allocated memory
            return FALSE;
        }
    }

    // free the allocated memory
    apr_pool_destroy(pool);

    m_arBaseFile.RemoveAll();
    m_arYourFile.RemoveAll();
    m_arTheirFile.RemoveAll();

    return TRUE;
}

bool CDiffData::DoTwoWayDiff(const CString& sBaseFilename, const CString& sYourFilename, IgnoreWS ignoreWs, bool bIgnoreEOL, bool bIgnoreCase, bool bIgnoreComments, apr_pool_t* pool)
{
    svn_diff_file_options_t* options = CreateDiffFileOptions(ignoreWs, bIgnoreEOL, pool);
    // convert CString filenames (UTF-16 or ANSI) to UTF-8
    CStringA sBaseFilenameUtf8 = CUnicodeUtils::GetUTF8(sBaseFilename);
    CStringA sYourFilenameUtf8 = CUnicodeUtils::GetUTF8(sYourFilename);

    svn_diff_t*  diffYourBase = nullptr;
    svn_error_t* svnErr       = svn_diff_file_diff_2(&diffYourBase, sBaseFilenameUtf8, sYourFilenameUtf8, options, pool);

    if (svnErr)
        return HandleSvnError(svnErr);

    tsvn_svn_diff_t_extension* movedBlocks = nullptr;
    if (m_bViewMovedBlocks)
        movedBlocks = MovedBlocksDetect(diffYourBase, ignoreWs, pool); // Side effect is that diffs are now splitted

    svn_diff_t* tempdiff = diffYourBase;
    LONG        baseline = 0;
    LONG        yourline = 0;
    while (tempdiff)
    {
        svn_diff__type_e diffType = tempdiff->type;
        // Side effect described above overcoming - sticking together
        apr_off_t originalLengthSticked = tempdiff->original_length;
        apr_off_t modifiedLengthSticked = tempdiff->modified_length;
        StickAndSkip(tempdiff, originalLengthSticked, modifiedLengthSticked);

        for (int i = 0; i < originalLengthSticked; i++)
        {
            if (baseline >= m_arBaseFile.GetCount())
            {
                m_sError.LoadString(IDS_ERR_DIFF_NEWLINES);
                return false;
            }
            const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseline);
            EOL            endingBase       = m_arBaseFile.GetLineEnding(baseline);
            if (diffType == svn_diff__type_common)
            {
                if (yourline >= m_arYourFile.GetCount())
                {
                    m_sError.LoadString(IDS_ERR_DIFF_NEWLINES);
                    return false;
                }
                const CString& sCurrentYourLine = m_arYourFile.GetAt(yourline);
                EOL            endingYours      = m_arYourFile.GetLineEnding(yourline);
                if (sCurrentBaseLine != sCurrentYourLine)
                {
                    bool changedWS = false;
                    if (ignoreWs == IgnoreWS::WhiteSpaces)
                        changedWS = CompareWithIgnoreWS(sCurrentBaseLine, sCurrentYourLine, ignoreWs);
                    if (changedWS || ignoreWs == IgnoreWS::None)
                    {
                        // one-pane view: two lines, one 'removed' and one 'added'
                        m_yourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_REMOVEDWHITESPACE, yourline, endingBase, changedWS && ignoreWs != IgnoreWS::None ? HIDESTATE_HIDDEN : HIDESTATE_SHOWN, -1);
                        m_yourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_ADDEDWHITESPACE, yourline, endingYours, changedWS && ignoreWs != IgnoreWS::None ? HIDESTATE_HIDDEN : HIDESTATE_SHOWN, -1);
                    }
                    else
                    {
                        m_yourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingBase, HIDESTATE_HIDDEN, -1);
                    }
                }
                else
                {
                    m_yourBaseBoth.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingBase, HIDESTATE_HIDDEN, -1);
                }
                yourline++; //in both files
            }
            else
            { // small trick - we need here a baseline, but we fix it back to yourline at the end of routine
                m_yourBaseBoth.AddData(sCurrentBaseLine, DIFFSTATE_REMOVED, -1, endingBase, HIDESTATE_SHOWN, -1);
            }
            baseline++;
        }
        if (diffType == svn_diff__type_diff_modified)
        {
            for (int i = 0; i < modifiedLengthSticked; i++)
            {
                if (m_arYourFile.GetCount() > yourline)
                {
                    m_yourBaseBoth.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
                }
                yourline++;
            }
        }
        tempdiff = tempdiff->next;
    }

    HideUnchangedSections(&m_yourBaseBoth, nullptr, nullptr);

    tempdiff = diffYourBase;
    baseline = 0;
    yourline = 0;

    // arbitrary length of 500
    static const int maxStringLengthForWhiteSpaceCheck = 500;
    auto             s1                                = std::make_unique<wchar_t[]>(maxStringLengthForWhiteSpaceCheck);
    auto             s2                                = std::make_unique<wchar_t[]>(maxStringLengthForWhiteSpaceCheck);
    while (tempdiff)
    {
        if (tempdiff->type == svn_diff__type_common)
        {
            for (int i = 0; i < tempdiff->original_length; i++)
            {
                const CString& sCurrentYourLine = m_arYourFile.GetAt(yourline);
                EOL            endingYours      = m_arYourFile.GetLineEnding(yourline);
                const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseline);
                EOL            endingBase       = m_arBaseFile.GetLineEnding(baseline);
                if (sCurrentBaseLine != sCurrentYourLine)
                {
                    bool changedNonWS = false;
                    auto ds           = DIFFSTATE_NORMAL;

                    if (ignoreWs == IgnoreWS::AllWhiteSpaces)
                    {
                        // the strings could be identical in relation to a filter.
                        // so to find out if there are whitespace changes, we have to strip the strings
                        // of all non-whitespace chars and then compare them.
                        // Note: this is not really fast! So we only do that if the lines are not too long (arbitrary value)
                        if ((sCurrentBaseLine.GetLength() < maxStringLengthForWhiteSpaceCheck) &&
                            (sCurrentYourLine.GetLength() < maxStringLengthForWhiteSpaceCheck))
                        {
                            auto pLine1 = static_cast<LPCWSTR>(sCurrentBaseLine);
                            auto pLine2 = static_cast<LPCWSTR>(sCurrentYourLine);
                            auto pS1    = s1.get();
                            while (*pLine1)
                            {
                                if ((*pLine1 == ' ') || (*pLine1 == '\t'))
                                {
                                    *pS1 = *pLine1;
                                    ++pS1;
                                }
                                ++pLine1;
                            }
                            *pS1 = '\0';

                            pS1      = s1.get();
                            auto pS2 = s2.get();
                            while (*pLine2)
                            {
                                if ((*pLine2 == ' ') || (*pLine2 == '\t'))
                                {
                                    *pS2 = *pLine2;
                                    ++pS2;
                                }
                                ++pLine2;
                            }
                            *pS2                      = '\0';
                            auto hasWhitespaceChanges = wcscmp(s1.get(), s2.get()) != 0;
                            if (hasWhitespaceChanges)
                                ds = DIFFSTATE_WHITESPACE;
                        }
                    }
                    if (ignoreWs == IgnoreWS::WhiteSpaces)
                        changedNonWS = CompareWithIgnoreWS(sCurrentBaseLine, sCurrentYourLine, ignoreWs);
                    if (!changedNonWS)
                    {
                        ds = DIFFSTATE_NORMAL;
                    }
                    if ((ds == DIFFSTATE_NORMAL) && (!m_rx._Empty() || bIgnoreCase || bIgnoreComments))
                    {
                        ds = DIFFSTATE_FILTEREDDIFF;
                    }

                    m_yourBaseLeft.AddData(sCurrentBaseLine, ds, baseline, endingBase, (ds == DIFFSTATE_NORMAL) && (ignoreWs != IgnoreWS::None) ? HIDESTATE_HIDDEN : HIDESTATE_SHOWN, -1);
                    m_yourBaseRight.AddData(sCurrentYourLine, ds, yourline, endingYours, (ds == DIFFSTATE_NORMAL) && (ignoreWs != IgnoreWS::None) ? HIDESTATE_HIDDEN : HIDESTATE_SHOWN, -1);
                }
                else
                {
                    m_yourBaseLeft.AddData(sCurrentBaseLine, DIFFSTATE_NORMAL, baseline, endingBase, HIDESTATE_HIDDEN, -1);
                    m_yourBaseRight.AddData(sCurrentYourLine, DIFFSTATE_NORMAL, yourline, endingYours, HIDESTATE_HIDDEN, -1);
                }
                baseline++;
                yourline++;
            }
        }
        if (tempdiff->type == svn_diff__type_diff_modified)
        {
            // now we trying to stick together parts, that were splitted by MovedBlocks
            apr_off_t originalLengthSticked = tempdiff->original_length;
            apr_off_t modifiedLengthSticked = tempdiff->modified_length;
            StickAndSkip(tempdiff, originalLengthSticked, modifiedLengthSticked);

            apr_off_t originalLength = originalLengthSticked;
            for (int i = 0; i < modifiedLengthSticked; i++)
            {
                if (m_arYourFile.GetCount() > yourline)
                {
                    EOL endingYours = m_arYourFile.GetLineEnding(yourline);
                    m_yourBaseRight.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, endingYours, HIDESTATE_SHOWN, -1);
                    if (originalLength-- <= 0)
                    {
                        m_yourBaseLeft.AddEmpty();
                    }
                    else
                    {
                        EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
                        m_yourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, endingBase, HIDESTATE_SHOWN, -1);
                        baseline++;
                    }
                    yourline++;
                }
            }
            apr_off_t modifiedLength = modifiedLengthSticked;
            for (int i = 0; i < originalLengthSticked; i++)
            {
                if ((modifiedLength-- <= 0) && (m_arBaseFile.GetCount() > baseline))
                {
                    EOL endingBase = m_arBaseFile.GetLineEnding(baseline);
                    m_yourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, endingBase, HIDESTATE_SHOWN, -1);
                    m_yourBaseRight.AddEmpty();
                    baseline++;
                }
            }
        }
        tempdiff = tempdiff->next;
    }
    // add last (empty) lines if needed - diff don't report those
    if (m_arBaseFile.GetCount() > baseline)
    {
        if (m_arYourFile.GetCount() > yourline)
        {
            // last line is missing in both files add them to end and mark as no diff
            m_yourBaseLeft.AddData(m_arBaseFile.GetAt(baseline), DIFFSTATE_NORMAL, baseline, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN, -1);
            m_yourBaseRight.AddData(m_arYourFile.GetAt(yourline), DIFFSTATE_NORMAL, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN, -1);
            yourline++;
            baseline++;
        }
        else
        {
            ViewData oViewData(m_arBaseFile.GetAt(baseline), DIFFSTATE_REMOVED, baseline, m_arBaseFile.GetLineEnding(baseline), HIDESTATE_SHOWN);
            baseline++;

            // find first EMPTY line in last blok
            int nPos = m_yourBaseLeft.GetCount();
            // ReSharper disable once CppPossiblyErroneousEmptyStatements
            while (--nPos >= 0 && m_yourBaseLeft.GetState(nPos) == DIFFSTATE_EMPTY)
                ;
            if (++nPos < m_yourBaseLeft.GetCount())
            {
                m_yourBaseLeft.SetData(nPos, oViewData);
            }
            else
            {
                m_yourBaseLeft.AddData(oViewData);
                m_yourBaseRight.AddEmpty();
            }
        }
    }
    else if (m_arYourFile.GetCount() > yourline)
    {
        ViewData oViewData(m_arYourFile.GetAt(yourline), DIFFSTATE_ADDED, yourline, m_arYourFile.GetLineEnding(yourline), HIDESTATE_SHOWN);
        yourline++;

        // try to move last line higher
        int nPos = m_yourBaseRight.GetCount();
        // ReSharper disable once CppPossiblyErroneousEmptyStatements
        while (--nPos >= 0 && m_yourBaseRight.GetState(nPos) == DIFFSTATE_EMPTY)
            ;
        if (++nPos < m_yourBaseRight.GetCount())
        {
            m_yourBaseRight.SetData(nPos, oViewData);
        }
        else
        {
            m_yourBaseLeft.AddEmpty();
            m_yourBaseRight.AddData(oViewData);
        }
    }

    // Fixing results for conforming moved blocks

    while (movedBlocks)
    {
        tempdiff = movedBlocks->base;
        if (movedBlocks->movedTo != -1)
        {
            // set states in a block original:length -> moved_to:length
            TieMovedBlocks(static_cast<int>(tempdiff->original_start), movedBlocks->movedTo, tempdiff->original_length);
        }
        if (movedBlocks->movedFrom != -1)
        {
            // set states in a block modified:length -> moved_from:length
            TieMovedBlocks(movedBlocks->movedFrom, static_cast<int>(tempdiff->modified_start), tempdiff->modified_length);
        }
        movedBlocks = movedBlocks->next;
    }

    // replace baseline with the yourline in m_YourBaseBoth
    /*    yourline = 0;
    for(int i=0; i<m_YourBaseBoth.GetCount(); i++)
    {
        DiffStates state = m_YourBaseBoth.GetState(i);
        if((state == DIFFSTATE_REMOVED)||(state == DIFFSTATE_MOVED_FROM))
        {
            m_YourBaseBoth.SetLineNumber(i, -1);
        }
        else
        {
            yourline++;
        }
    }//*/

    TRACE(L"done with 2-way diff\n");

    HideUnchangedSections(&m_yourBaseLeft, &m_yourBaseRight, nullptr);

    return true;
}

bool CDiffData::DoThreeWayDiff(const CString& sBaseFilename, const CString& sYourFilename, const CString& sTheirFilename, IgnoreWS ignoreWs, bool bIgnoreEOL, bool bIgnoreCase, bool bIgnoreComments, apr_pool_t* pool)
{
    // the following three arrays are used to check for conflicts even in case the
    // user has ignored spaces/eols.
    CStdDWORDArray arDiff3LinesBase;
    CStdDWORDArray arDiff3LinesYour;
    CStdDWORDArray arDiff3LinesTheir;
#define AddLines(baseline, yourline, theirline) arDiff3LinesBase.Add(baseline), arDiff3LinesYour.Add(yourline), arDiff3LinesTheir.Add(theirline)
    int lengthHint = GetLineCount();

    arDiff3LinesBase.Reserve(lengthHint);
    arDiff3LinesYour.Reserve(lengthHint);
    arDiff3LinesTheir.Reserve(lengthHint);

    svn_diff_file_options_t* options = CreateDiffFileOptions(ignoreWs, bIgnoreEOL, pool);

    // convert CString filenames (UTF-16 or ANSI) to UTF-8
    CStringA sBaseFilenameUtf8  = CUnicodeUtils::GetUTF8(sBaseFilename);
    CStringA sYourFilenameUtf8  = CUnicodeUtils::GetUTF8(sYourFilename);
    CStringA sTheirFilenameUtf8 = CUnicodeUtils::GetUTF8(sTheirFilename);

    svn_diff_t*  diffTheirYourBase = nullptr;
    svn_error_t* svnErr            = svn_diff_file_diff3_2(&diffTheirYourBase, sBaseFilenameUtf8, sTheirFilenameUtf8, sYourFilenameUtf8, options, pool);
    if (svnErr)
        return HandleSvnError(svnErr);

    svn_diff_t* tempDiff  = diffTheirYourBase;
    LONG        baseLine  = 0;
    LONG        yourLine  = 0;
    LONG        theirLine = 0;
    LONG        resLine   = 0;
    // common ViewData
    const ViewData emptyConflictEmpty(L"", DIFFSTATE_CONFLICTEMPTY, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN);
    const ViewData emptyIdenticalRemoved(L"", DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN);
    while (tempDiff)
    {
        if (tempDiff->type == svn_diff__type_common)
        {
            ASSERT((tempDiff->latest_length == tempDiff->modified_length) && (tempDiff->modified_length == tempDiff->original_length));
            for (int i = 0; i < tempDiff->original_length; i++)
            {
                if ((m_arYourFile.GetCount() > yourLine) && (m_arTheirFile.GetCount() > theirLine))
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_NORMAL, resLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_HIDDEN, -1);
                    m_yourBaseBoth.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_NORMAL, yourLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_HIDDEN, -1);
                    m_theirBaseBoth.AddData(m_arTheirFile.GetAt(theirLine), DIFFSTATE_NORMAL, theirLine, m_arTheirFile.GetLineEnding(theirLine), HIDESTATE_HIDDEN, -1);

                    baseLine++;
                    yourLine++;
                    theirLine++;
                    resLine++;
                }
            }
        }
        else if (tempDiff->type == svn_diff__type_diff_common)
        {
            ASSERT(tempDiff->latest_length == tempDiff->modified_length);
            //both theirs and yours have lines replaced
            for (int i = 0; i < tempDiff->original_length; i++)
            {
                if (m_arBaseFile.GetCount() > baseLine)
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseLine), HIDESTATE_SHOWN, -1);
                    m_yourBaseBoth.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);
                    m_theirBaseBoth.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);

                    baseLine++;
                }
            }
            for (int i = 0; i < tempDiff->modified_length; i++)
            {
                if ((m_arYourFile.GetCount() > yourLine) && (m_arTheirFile.GetCount() > theirLine))
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_IDENTICALADDED, resLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_SHOWN, -1);
                    m_yourBaseBoth.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_IDENTICALADDED, yourLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_SHOWN, -1);
                    m_theirBaseBoth.AddData(m_arTheirFile.GetAt(theirLine), DIFFSTATE_IDENTICALADDED, resLine, m_arTheirFile.GetLineEnding(theirLine), HIDESTATE_SHOWN, -1);

                    yourLine++;
                    theirLine++;
                    resLine++;
                }
            }
        }
        else if (tempDiff->type == svn_diff__type_conflict)
        {
            apr_off_t length   = max(tempDiff->original_length, tempDiff->modified_length);
            length             = max(tempDiff->latest_length, length);
            apr_off_t original = tempDiff->original_length;
            apr_off_t modified = tempDiff->modified_length;
            apr_off_t latest   = tempDiff->latest_length;

            apr_off_t originalResolved = 0;
            apr_off_t modifiedResolved = 0;
            apr_off_t latestResolved   = 0;

            LONG base  = baseLine;
            LONG your  = yourLine;
            LONG their = theirLine;
            if (tempDiff->resolved_diff)
            {
                originalResolved = tempDiff->resolved_diff->original_length;
                modifiedResolved = tempDiff->resolved_diff->modified_length;
                latestResolved   = tempDiff->resolved_diff->latest_length;
            }
            for (int i = 0; i < length; i++)
            {
                EOL endingBase = m_arBaseFile.GetCount() > baseLine ? m_arBaseFile.GetLineEnding(baseLine) : EOL_AUTOLINE;
                if (original)
                {
                    if (m_arBaseFile.GetCount() > baseLine)
                    {
                        AddLines(baseLine, yourLine, theirLine);

                        m_diff3.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, endingBase, HIDESTATE_SHOWN, -1);
                        m_yourBaseBoth.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, endingBase, HIDESTATE_SHOWN, -1);
                        m_theirBaseBoth.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_IDENTICALREMOVED, DIFF_EMPTYLINENUMBER, endingBase, HIDESTATE_SHOWN, -1);
                    }
                }
                else if ((originalResolved) || ((modifiedResolved) && (latestResolved)))
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(emptyIdenticalRemoved);
                    if ((latestResolved) && (modifiedResolved))
                    {
                        m_yourBaseBoth.AddData(emptyIdenticalRemoved);
                        m_theirBaseBoth.AddData(emptyIdenticalRemoved);
                    }
                }
                if (original)
                {
                    original--;
                    baseLine++;
                }
                if (originalResolved)
                    originalResolved--;

                if (modified)
                {
                    modified--;
                    theirLine++;
                }
                if (modifiedResolved)
                    modifiedResolved--;
                if (latest)
                {
                    latest--;
                    yourLine++;
                }
                if (latestResolved)
                    latestResolved--;
            }
            original  = tempDiff->original_length;
            modified  = tempDiff->modified_length;
            latest    = tempDiff->latest_length;
            baseLine  = base;
            yourLine  = your;
            theirLine = their;
            if (tempDiff->resolved_diff)
            {
                originalResolved = tempDiff->resolved_diff->original_length;
                modifiedResolved = tempDiff->resolved_diff->modified_length;
                latestResolved   = tempDiff->resolved_diff->latest_length;
            }
            for (int i = 0; i < length; i++)
            {
                if ((latest) || (modified))
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(L"", DIFFSTATE_CONFLICTED, resLine, EOL_NOENDING, HIDESTATE_SHOWN, -1);

                    resLine++;
                }

                if (latest)
                {
                    if (m_arYourFile.GetCount() > yourLine)
                    {
                        m_yourBaseBoth.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_CONFLICTADDED, yourLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_SHOWN, -1);
                    }
                }
                else if ((latestResolved) || (modified) || (modifiedResolved))
                {
                    m_yourBaseBoth.AddData(emptyConflictEmpty);
                }
                if (modified)
                {
                    if (m_arTheirFile.GetCount() > theirLine)
                    {
                        m_theirBaseBoth.AddData(m_arTheirFile.GetAt(theirLine), DIFFSTATE_CONFLICTADDED, theirLine, m_arTheirFile.GetLineEnding(theirLine), HIDESTATE_SHOWN, -1);
                    }
                }
                else if ((modifiedResolved) || (latest) || (latestResolved))
                {
                    m_theirBaseBoth.AddData(emptyConflictEmpty);
                }
                if (original)
                {
                    original--;
                    baseLine++;
                }
                if (originalResolved)
                    originalResolved--;
                if (modified)
                {
                    modified--;
                    theirLine++;
                }
                if (modifiedResolved)
                    modifiedResolved--;
                if (latest)
                {
                    latest--;
                    yourLine++;
                }
                if (latestResolved)
                    latestResolved--;
            }
        }
        else if (tempDiff->type == svn_diff__type_diff_modified)
        {
            //deleted!
            for (int i = 0; i < tempDiff->original_length; i++)
            {
                if ((m_arBaseFile.GetCount() > baseLine) && (m_arYourFile.GetCount() > yourLine))
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_THEIRSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseLine), HIDESTATE_SHOWN, -1);
                    m_yourBaseBoth.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_NORMAL, yourLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_SHOWN, -1);
                    m_theirBaseBoth.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_THEIRSREMOVED, DIFF_EMPTYLINENUMBER, EOL_NOENDING, HIDESTATE_SHOWN, -1);

                    baseLine++;
                    yourLine++;
                }
            }
            //added
            for (int i = 0; i < tempDiff->modified_length; i++)
            {
                if (m_arTheirFile.GetCount() > theirLine)
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(m_arTheirFile.GetAt(theirLine), DIFFSTATE_THEIRSADDED, resLine, m_arTheirFile.GetLineEnding(theirLine), HIDESTATE_SHOWN, -1);
                    m_yourBaseBoth.AddEmpty();
                    m_theirBaseBoth.AddData(m_arTheirFile.GetAt(theirLine), DIFFSTATE_THEIRSADDED, theirLine, m_arTheirFile.GetLineEnding(theirLine), HIDESTATE_SHOWN, -1);

                    theirLine++;
                    resLine++;
                }
            }
        }
        else if (tempDiff->type == svn_diff__type_diff_latest)
        {
            //YOURS differs from base

            for (int i = 0; i < tempDiff->original_length; i++)
            {
                if ((m_arBaseFile.GetCount() > baseLine) && (m_arTheirFile.GetCount() > theirLine))
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_YOURSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseLine), HIDESTATE_SHOWN, -1);
                    m_yourBaseBoth.AddData(m_arBaseFile.GetAt(baseLine), DIFFSTATE_YOURSREMOVED, DIFF_EMPTYLINENUMBER, m_arBaseFile.GetLineEnding(baseLine), HIDESTATE_SHOWN, -1);
                    m_theirBaseBoth.AddData(m_arTheirFile.GetAt(theirLine), DIFFSTATE_NORMAL, theirLine, m_arTheirFile.GetLineEnding(theirLine), HIDESTATE_HIDDEN, -1);

                    baseLine++;
                    theirLine++;
                }
            }
            for (int i = 0; i < tempDiff->latest_length; i++)
            {
                if (m_arYourFile.GetCount() > yourLine)
                {
                    AddLines(baseLine, yourLine, theirLine);

                    m_diff3.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_YOURSADDED, resLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_SHOWN, -1);
                    m_yourBaseBoth.AddData(m_arYourFile.GetAt(yourLine), DIFFSTATE_IDENTICALADDED, yourLine, m_arYourFile.GetLineEnding(yourLine), HIDESTATE_SHOWN, -1);
                    m_theirBaseBoth.AddEmpty();

                    yourLine++;
                    resLine++;
                }
            }
        }
        else
        {
            TRACE(L"something bad happened during diff!\n");
        }
        tempDiff = tempDiff->next;

    } // while (tempdiff)

    if ((options->ignore_space != svn_diff_file_ignore_space_none) || (bIgnoreCase || bIgnoreEOL || bIgnoreComments || !m_rx._Empty()))
    {
        // If whitespaces are ignored, a conflict could have been missed
        // We now go through all lines again and check if they're identical.
        // If they're not, then that means it is a conflict, and we
        // mark the conflict with the proper colors.
        for (long i = 0; i < m_diff3.GetCount(); ++i)
        {
            DiffStates state1 = m_yourBaseBoth.GetState(i);
            DiffStates state2 = m_theirBaseBoth.GetState(i);

            if (((state1 == DIFFSTATE_IDENTICALADDED) || (state1 == DIFFSTATE_NORMAL)) &&
                ((state2 == DIFFSTATE_IDENTICALADDED) || (state2 == DIFFSTATE_NORMAL)))
            {
                LONG lineYour  = arDiff3LinesYour.GetAt(i);
                LONG lineTheir = arDiff3LinesTheir.GetAt(i);
                LONG lineBase  = arDiff3LinesBase.GetAt(i);
                if ((lineYour < m_arYourFile.GetCount()) &&
                    (lineTheir < m_arTheirFile.GetCount()) &&
                    (lineBase < m_arBaseFile.GetCount()))
                {
                    if (((m_arYourFile.GetLineEnding(lineYour) != m_arBaseFile.GetLineEnding(lineBase)) &&
                         (m_arTheirFile.GetLineEnding(lineTheir) != m_arBaseFile.GetLineEnding(lineBase)) &&
                         (m_arYourFile.GetLineEnding(lineYour) != m_arTheirFile.GetLineEnding(lineTheir))) ||
                        ((m_arYourFile.GetAt(lineYour).Compare(m_arBaseFile.GetAt(lineBase)) != 0) &&
                         (m_arTheirFile.GetAt(lineTheir).Compare(m_arBaseFile.GetAt(lineBase)) != 0) &&
                         (m_arYourFile.GetAt(lineYour).Compare(m_arTheirFile.GetAt(lineTheir)) != 0)))
                    {
                        m_diff3.SetState(i, DIFFSTATE_CONFLICTED_IGNORED);
                        m_yourBaseBoth.SetState(i, DIFFSTATE_CONFLICTADDED);
                        m_theirBaseBoth.SetState(i, DIFFSTATE_CONFLICTADDED);
                    }
                }
            }
        }
    }
    ASSERT(m_diff3.GetCount() == m_yourBaseBoth.GetCount());
    ASSERT(m_theirBaseBoth.GetCount() == m_yourBaseBoth.GetCount());

    TRACE(L"done with 3-way diff\n");

    HideUnchangedSections(&m_diff3, &m_yourBaseBoth, &m_theirBaseBoth);

    return true;
}

void CDiffData::HideUnchangedSections(CViewData* data1, CViewData* data2, CViewData* data3)
{
    if (!data1)
        return;

    CRegDWORD contextLines = CRegDWORD(L"Software\\TortoiseMerge\\ContextLines", 1);

    if (data1->GetCount() > 1)
    {
        HideState lastHideState = data1->GetHideState(0);
        if (lastHideState == HIDESTATE_HIDDEN)
        {
            data1->SetLineHideState(0, HIDESTATE_MARKER);
            if (data2)
                data2->SetLineHideState(0, HIDESTATE_MARKER);
            if (data3)
                data3->SetLineHideState(0, HIDESTATE_MARKER);
        }
        for (int i = 1; i < data1->GetCount(); ++i)
        {
            HideState hideState = data1->GetHideState(i);
            if (hideState != lastHideState)
            {
                if (hideState == HIDESTATE_SHOWN)
                {
                    // go back and show the last 'contextLines' lines to "SHOWN"
                    int lineBack = i - 1;
                    int stopLine = lineBack - static_cast<int>(static_cast<DWORD>(contextLines));
                    while ((lineBack >= 0) && (lineBack > stopLine))
                    {
                        data1->SetLineHideState(lineBack, HIDESTATE_SHOWN);
                        if (data2)
                            data2->SetLineHideState(lineBack, HIDESTATE_SHOWN);
                        if (data3)
                            data3->SetLineHideState(lineBack, HIDESTATE_SHOWN);
                        lineBack--;
                    }
                }
                else if ((hideState == HIDESTATE_HIDDEN) && (lastHideState != HIDESTATE_MARKER))
                {
                    // go forward and show the next 'contextLines' lines to "SHOWN"
                    int lineForward = i + 1;
                    int stopLine    = lineForward + static_cast<int>(static_cast<DWORD>(contextLines));
                    while ((lineForward < data1->GetCount()) && (lineForward < stopLine))
                    {
                        data1->SetLineHideState(lineForward, HIDESTATE_SHOWN);
                        if (data2)
                            data2->SetLineHideState(lineForward, HIDESTATE_SHOWN);
                        if (data3)
                            data3->SetLineHideState(lineForward, HIDESTATE_SHOWN);
                        lineForward++;
                    }
                    if ((lineForward < data1->GetCount()) && (data1->GetHideState(lineForward) == HIDESTATE_HIDDEN))
                    {
                        data1->SetLineHideState(lineForward, HIDESTATE_MARKER);
                        if (data2)
                            data2->SetLineHideState(lineForward, HIDESTATE_MARKER);
                        if (data3)
                            data3->SetLineHideState(lineForward, HIDESTATE_MARKER);
                    }
                }
            }
            lastHideState = hideState;
        }
    }
}

void CDiffData::SetCommentTokens(const CString& sLineStart, const CString& sBlockStart, const CString& sBlockEnd)
{
    m_commentLineStart  = sLineStart;
    m_commentBlockStart = sBlockStart;
    m_commentBlockEnd   = sBlockEnd;
}

void CDiffData::SetRegexTokens(const std::wregex& rx, const std::wstring& replacement)
{
    m_rx          = rx;
    m_replacement = replacement;
}
