// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2009, 2011, 2014, 2021 - TortoiseSVN

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
#include "SVNLineDiff.h"

const svn_diff_fns2_t SVNLineDiff::SVN_LINE_DIFF_VTABLE =
    {
        SVNLineDiff::datasourcesOpen,
        SVNLineDiff::datasourceClose,
        SVNLineDiff::nextToken,
        SVNLineDiff::compareToken,
        SVNLineDiff::discardToken,
        SVNLineDiff::discardAllToken};

#define SVNLINEDIFF_CHARTYPE_NONE         0
#define SVNLINEDIFF_CHARTYPE_ALPHANUMERIC 1
#define SVNLINEDIFF_CHARTYPE_SPACE        2
#define SVNLINEDIFF_CHARTYPE_OTHER        3

using LineParser = void (*)(LPCWSTR line, apr_size_t lineLength, std::vector<std::wstring>& tokens);

void SVNLineDiff::ParseLineWords(LPCWSTR line, apr_size_t lineLength, std::vector<std::wstring>& tokens)
{
    std::wstring token;
    int          prevCharType = SVNLINEDIFF_CHARTYPE_NONE;
    tokens.reserve(lineLength / 2);
    for (apr_size_t i = 0; i < lineLength; ++i)
    {
        int charType =
            IsCharAlphaNumeric(line[i]) ? SVNLINEDIFF_CHARTYPE_ALPHANUMERIC : isCharWhiteSpace(line[i]) ? SVNLINEDIFF_CHARTYPE_SPACE
                                                                                                        : SVNLINEDIFF_CHARTYPE_OTHER;

        // Token is a sequence of either alphanumeric or whitespace characters.
        // Treat all other characters as a separate tokens.
        if (charType == prevCharType && charType != SVNLINEDIFF_CHARTYPE_OTHER)
            token += line[i];
        else
        {
            if (!token.empty())
                tokens.push_back(token);
            token = line[i];
        }
        prevCharType = charType;
    }
    if (!token.empty())
        tokens.push_back(token);
}

void SVNLineDiff::ParseLineChars(
    LPCWSTR line, apr_size_t lineLength, std::vector<std::wstring>& tokens)
{
    std::wstring token;
    for (apr_size_t i = 0; i < lineLength; ++i)
    {
        token = line[i];
        tokens.push_back(token);
    }
}

svn_error_t* SVNLineDiff::datasourcesOpen(void* baton, apr_off_t* prefixLines, apr_off_t* /*suffix_lines*/, const svn_diff_datasource_e* dataSources, apr_size_t dataSourceLen)
{
    SVNLineDiff* lineDiff = static_cast<SVNLineDiff*>(baton);
    LineParser   parser   = lineDiff->m_bWordDiff ? ParseLineWords : ParseLineChars;
    for (apr_size_t i = 0; i < dataSourceLen; i++)
    {
        switch (dataSources[i])
        {
            case svn_diff_datasource_original:
                parser(lineDiff->m_line1, lineDiff->m_line1Length, lineDiff->m_line1Tokens);
                break;
            case svn_diff_datasource_modified:
                parser(lineDiff->m_line2, lineDiff->m_line2Length, lineDiff->m_line2Tokens);
                break;
            case svn_diff_datasource_latest:
                break;
            case svn_diff_datasource_ancestor:
                break;
            default:;
        }
    }
    // Don't claim any prefix matches.
    *prefixLines = 0;

    return nullptr;
}

svn_error_t* SVNLineDiff::datasourceClose(void* /*baton*/, svn_diff_datasource_e /*datasource*/)
{
    return nullptr;
}

void SVNLineDiff::NextTokenWords(apr_uint32_t* hash, void** token, apr_size_t& linePos, const std::vector<std::wstring>& tokens)
{
    if (linePos < tokens.size())
    {
        *token = static_cast<void*>(const_cast<wchar_t*>(tokens[linePos].c_str()));
        *hash  = SVNLineDiff::Adler32(0, tokens[linePos].c_str(), tokens[linePos].size());
        linePos++;
    }
}

void SVNLineDiff::NextTokenChars(apr_uint32_t* hash, void** token, apr_size_t& linePos, LPCWSTR line, apr_size_t lineLength)
{
    if (linePos < lineLength)
    {
        *token = static_cast<void*>(const_cast<wchar_t*>(&line[linePos]));
        *hash  = line[linePos];
        linePos++;
    }
}

svn_error_t* SVNLineDiff::nextToken(apr_uint32_t* hash, void** token, void* baton, svn_diff_datasource_e dataSource)
{
    SVNLineDiff* linediff = static_cast<SVNLineDiff*>(baton);
    *token                = nullptr;
    switch (dataSource)
    {
        case svn_diff_datasource_original:
            if (linediff->m_bWordDiff)
                NextTokenWords(hash, token, linediff->m_line1Pos, linediff->m_line1Tokens);
            else
                NextTokenChars(hash, token, linediff->m_line1Pos, linediff->m_line1, linediff->m_line1Length);
            break;
        case svn_diff_datasource_modified:
            if (linediff->m_bWordDiff)
                NextTokenWords(hash, token, linediff->m_line2Pos, linediff->m_line2Tokens);
            else
                NextTokenChars(hash, token, linediff->m_line2Pos, linediff->m_line2, linediff->m_line2Length);
            break;
        case svn_diff_datasource_latest:
            break;
        case svn_diff_datasource_ancestor:
            break;
        default:;
    }
    return nullptr;
}

svn_error_t* SVNLineDiff::compareToken(void* baton, void* token1, void* token2, int* compare)
{
    SVNLineDiff* lineDiff = static_cast<SVNLineDiff*>(baton);
    if (lineDiff->m_bWordDiff)
    {
        LPCWSTR s1 = static_cast<LPCWSTR>(token1);
        LPCWSTR s2 = static_cast<LPCWSTR>(token2);
        if (s1 && s2)
        {
            *compare = wcscmp(s1, s2);
        }
    }
    else
    {
        wchar_t* c1 = static_cast<wchar_t*>(token1);
        wchar_t* c2 = static_cast<wchar_t*>(token2);
        if (c1 && c2)
        {
            if (*c1 == *c2)
                *compare = 0;
            else if (*c1 < *c2)
                *compare = -1;
            else
                *compare = 1;
        }
    }
    return nullptr;
}

void SVNLineDiff::discardToken(void* /*baton*/, void* /*token*/)
{
}

void SVNLineDiff::discardAllToken(void* /*baton*/)
{
}

SVNLineDiff::SVNLineDiff()
    : m_pool(nullptr)
    , m_subPool(nullptr)
    , m_line1(nullptr)
    , m_line1Length(0)
    , m_line2(nullptr)
    , m_line2Length(0)
    , m_line1Pos(0)
    , m_line2Pos(0)
    , m_bWordDiff(false)
{
    m_pool = svn_pool_create(NULL);
}

SVNLineDiff::~SVNLineDiff()
{
    svn_pool_destroy(m_pool);
};

bool SVNLineDiff::Diff(svn_diff_t** diff, LPCWSTR line1, apr_size_t len1, LPCWSTR line2, apr_size_t len2, bool bWordDiff)
{
    if (m_subPool)
        svn_pool_clear(m_subPool);
    else
        m_subPool = svn_pool_create(m_pool);

    if (m_subPool == nullptr)
        return false;

    m_bWordDiff   = bWordDiff;
    m_line1       = line1;
    m_line2       = line2;
    m_line1Length = len1 ? len1 : wcslen(m_line1);
    m_line2Length = len2 ? len2 : wcslen(m_line2);

    m_line1Pos = 0;
    m_line2Pos = 0;
    m_line1Tokens.clear();
    m_line2Tokens.clear();
    svn_error_t* err = svn_diff_diff_2(diff, this, &SVN_LINE_DIFF_VTABLE, m_subPool);
    if (err)
    {
        svn_error_clear(err);
        svn_pool_clear(m_subPool);
        return false;
    }
    return true;
}

#define ADLER_MOD_BASE       65521
#define ADLER_MOD_BLOCK_SIZE 5552

apr_uint32_t SVNLineDiff::Adler32(apr_uint32_t checksum, const WCHAR* data, apr_size_t len)
{
    const unsigned char* input = reinterpret_cast<const unsigned char*>(data);
    apr_uint32_t         s1    = checksum & 0xFFFF;
    apr_uint32_t         s2    = checksum >> 16;
    apr_uint32_t         b;
    len *= 2;
    apr_size_t blocks = len / ADLER_MOD_BLOCK_SIZE;

    len %= ADLER_MOD_BLOCK_SIZE;

    while (blocks--)
    {
        int count = ADLER_MOD_BLOCK_SIZE;
        while (count--)
        {
            b = *input++;
            s1 += b;
            s2 += s1;
        }

        s1 %= ADLER_MOD_BASE;
        s2 %= ADLER_MOD_BASE;
    }

    while (len--)
    {
        b = *input++;
        s1 += b;
        s2 += s1;
    }

    return ((s2 % ADLER_MOD_BASE) << 16) | (s1 % ADLER_MOD_BASE);
}

bool SVNLineDiff::isCharWhiteSpace(wchar_t c)
{
    return (c == ' ') || (c == '\t');
}

bool SVNLineDiff::ShowInlineDiff(svn_diff_t* diff)
{
    svn_diff_t* tempDiff   = diff;
    apr_size_t  diffCounts = 0;
    apr_off_t   origSize   = 0;
    apr_off_t   diffSize   = 0;
    while (tempDiff)
    {
        if (tempDiff->type == svn_diff__type_common)
        {
            origSize += tempDiff->original_length;
        }
        else
        {
            diffCounts++;
            diffSize += tempDiff->original_length;
            diffSize += tempDiff->modified_length;
        }
        tempDiff = tempDiff->next;
    }
    return (origSize > 0.5 * diffSize + 1.0 * diffCounts); // Multiplier values may be subject to individual preferences
}
