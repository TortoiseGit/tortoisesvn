// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2011, 2021 - TortoiseSVN

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
#include "svn_diff.h"
#include "diff.h"

/**
 * \ingroup TortoiseMerge
 * Handles diffs of single lines. Used for inline diffs.
 */
class SVNLineDiff
{
public:
    SVNLineDiff();
    ~SVNLineDiff();

    bool Diff(svn_diff_t** diff, LPCWSTR line1, apr_size_t len1, LPCWSTR line2, apr_size_t len2, bool bWordDiff);
    /** Checks if we really should show inline diffs.
     * Inline diffs are only useful if the two lines are not
     * completely different but at least a little bit similar.
     */
    static bool ShowInlineDiff(svn_diff_t* diff);

    std::vector<std::wstring> m_line1Tokens;
    std::vector<std::wstring> m_line2Tokens;

private:
    apr_pool_t* m_pool;
    apr_pool_t* m_subPool;
    LPCWSTR     m_line1;
    apr_size_t  m_line1Length;
    LPCWSTR     m_line2;
    apr_size_t  m_line2Length;
    apr_size_t  m_line1Pos;
    apr_size_t  m_line2Pos;

    bool m_bWordDiff;

    static svn_error_t* datasourcesOpen(void* baton, apr_off_t* prefixLines, apr_off_t* suffixLines, const svn_diff_datasource_e* dataSources, apr_size_t dataSourceLen);
    static svn_error_t* datasourceClose(void* baton, svn_diff_datasource_e dataSource);
    static svn_error_t* nextToken(apr_uint32_t* hash, void** token, void* baton, svn_diff_datasource_e dataSource);
    static svn_error_t* compareToken(void* baton, void* token1, void* token2, int* compare);
    static void         discardToken(void* baton, void* token);
    static void         discardAllToken(void* baton);
    static bool         isCharWhiteSpace(wchar_t c);

    static apr_uint32_t          Adler32(apr_uint32_t checksum, const WCHAR* data, apr_size_t len);
    static void                  ParseLineWords(LPCWSTR line, apr_size_t lineLength, std::vector<std::wstring>& tokens);
    static void                  ParseLineChars(LPCWSTR line, apr_size_t lineLength, std::vector<std::wstring>& tokens);
    static void                  NextTokenWords(apr_uint32_t* hash, void** token, apr_size_t& linePos, const std::vector<std::wstring>& tokens);
    static void                  NextTokenChars(apr_uint32_t* hash, void** token, apr_size_t& linePos, LPCWSTR line, apr_size_t lineLength);
    static const svn_diff_fns2_t SVN_LINE_DIFF_VTABLE;
};