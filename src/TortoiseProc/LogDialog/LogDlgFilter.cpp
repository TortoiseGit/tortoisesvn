// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2017, 2019, 2021-2022 - TortoiseSVN

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
#include "LogDlgFilter.h"
#include "LogDlg.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"

namespace
{
#ifndef _M_ARM64
BOOL sse2Supported = ::IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);
#else
BOOL sse2Supported = false;
#endif
// case-sensitivity optimization functions

bool IsAllASCII7(const CString& s)
{
    for (int i = 0, count = s.GetLength(); i < count; ++i)
        if (s[i] >= 0x80)
            return false;

    return true;
}

void FastLowerCaseConversion(char* s, size_t size)
{
    // most of our strings will be tens of bytes long
    // -> afford some minor overhead to handle the main part very fast
#ifndef _M_ARM64
    if (sse2Supported)
    {
        __m128i zero  = _mm_setzero_si128();
        __m128i aMask = _mm_set_epi8('@', '@', '@', '@', '@', '@', '@', '@',
                                     '@', '@', '@', '@', '@', '@', '@', '@');
        __m128i zMask = _mm_set_epi8('[', '[', '[', '[', '[', '[', '[', '[',
                                     '[', '[', '[', '[', '[', '[', '[', '[');
        __m128i diff  = _mm_set_epi8(32, 32, 32, 32, 32, 32, 32, 32,
                                    32, 32, 32, 32, 32, 32, 32, 32);

        char* end = s + size;
        for (; s + sizeof(zero) <= end; s += sizeof(zero))
        {
            // fetch the next 16 bytes from the source

            __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s));

            // check for string end

            if (_mm_movemask_epi8(_mm_cmpeq_epi8(chunk, zero)) != 0)
                break;

            // identify chars that need correction

            __m128i aOrLarger   = _mm_cmpgt_epi8(chunk, aMask);
            __m128i zOrSmaller  = _mm_cmplt_epi8(chunk, zMask);
            __m128i isUpperCase = _mm_and_si128(aOrLarger, zOrSmaller);

            // apply the diff only where identified previously

            __m128i correction = _mm_and_si128(isUpperCase, diff);
            chunk              = _mm_add_epi8(chunk, correction);

            // store the updated data

            _mm_storeu_si128(reinterpret_cast<__m128i*>(s), chunk);
        };
    }
#endif

    for (char c = *s; c != 0; c = *++s)
        if ((c <= 'Z') && (c >= 'A'))
            *s += 'a' - 'A';
}

// translate positions within a UTF8 string to the position these
// characters would have in an UTF16 string. Advance utf8Pos until
// target has been reached. Update utf16Pos accordingly.

void AdvanceUnicodePos(const std::string& textUTF8, size_t& utf8Pos, size_t& utf16Pos, size_t target)
{
    for (; utf8Pos < target; ++utf8Pos)
    {
        unsigned char c = textUTF8[utf8Pos];
        if ((c < 0x80) || (c >= 0xc0))
            ++utf16Pos;
    }
}

}; // namespace

// filter utility method

bool CLogDlgFilter::Match(char* text, size_t size) const
{
    // empty text does not match

    if (size == 0)
        return false;

    if (patterns.empty())
    {
        // normalize to lower case

        if (!caseSensitive)
            if (fastLowerCase)
                FastLowerCaseConversion(text, size);
            else
            {
                // all these strings are in utf-8, which means all the
                // standard c-APIs to convert the string directly to
                // lowercase won't work: we first have to convert
                // the string to wide-char. Very very slow, but unavoidable
                // if we want to make this work right.
                CStringA as = CStringA(text, static_cast<int>(size));
                CString  s  = CUnicodeUtils::GetUnicode(as);
                s.MakeLower();
                as = CUnicodeUtils::GetUTF8(s);
                strncpy_s(text, size + 1, as, size);
            }

        // require all strings to be present

        bool currentValue = true;
        for (size_t i = 0, count = subStringConditions.size(); i < count; ++i)
        {
            const auto& [subString, prefix, nextOrIndex] = subStringConditions[i];
            bool found                                   = strstr(text, subString.c_str()) != nullptr;
            switch (prefix)
            {
                case Prefix::AndNot:
                    found = !found;
                    [[fallthrough]];
                case Prefix::And:
                    if (!found)
                    {
                        // not a match, so skip to the next "+"-prefixed item

                        if (nextOrIndex == 0)
                            return false;

                        currentValue = false;
                        i            = nextOrIndex - 1;
                    }
                    break;

                case Prefix::Or:
                    currentValue |= found;
                    if (!currentValue)
                    {
                        // not a match, so skip to the next "+"-prefixed item

                        if (nextOrIndex == 0)
                            return false;

                        i = nextOrIndex - 1;
                    }
                    break;
            }
        }
    }
    else
    {
        for (const auto& pattern : patterns)
        {
            try
            {
                if (!regex_search(text, text + size, pattern, std::regex_constants::match_any))
                    return false;
            }
            catch (std::exception& /*e*/)
            {
                return false;
            }
        }
    }

    return true;
}

std::vector<CHARRANGE> CLogDlgFilter::GetMatchRanges(std::wstring& textUTF16) const
{
    std::vector<CHARRANGE> ranges;

    // normalize to lower case

    if (!caseSensitive && !textUTF16.empty())
        _wcslwr_s(&textUTF16.at(0), textUTF16.length() + 1);
    std::string textUTF8 = CUnicodeUtils::StdGetUTF8(textUTF16);

    if (patterns.empty())
    {
        // require all strings to be present

        const char* toScan = textUTF8.c_str();
        for (auto iter = subStringConditions.begin(), end = subStringConditions.end(); iter != end; ++iter)
        {
            if (iter->prefix != Prefix::AndNot)
            {
                const char* toFind       = iter->subString.c_str();
                size_t      toFindLength = iter->subString.length();
                const char* pFound       = strstr(toScan, toFind);
                while (pFound)
                {
                    CHARRANGE range;
                    range.cpMin = static_cast<LONG>(pFound - toScan);
                    range.cpMax = static_cast<LONG>(range.cpMin + toFindLength);
                    ranges.push_back(range);
                    pFound = strstr(pFound + 1, toFind);
                }
            }
        }
    }
    else
    {
        for (const auto& pattern : patterns)
        {
            const std::sregex_iterator end;
            for (std::sregex_iterator it2(textUTF8.begin(), textUTF8.end(), pattern); it2 != end; ++it2)
            {
                ptrdiff_t matchPosId = it2->position(0);
                CHARRANGE range      = {static_cast<LONG>(matchPosId), static_cast<LONG>(matchPosId + (*it2)[0].str().size())};
                ranges.push_back(range);
            }
        }
    }

    // the caller expects the result to be ordered by position which
    // which may not be the case after scanninf for multiple sub-strings

    if (ranges.size() > 1)
    {
        auto begin = ranges.begin();
        auto end   = ranges.end();

        std::sort(begin, end,
                  [](const CHARRANGE& lhs, const CHARRANGE& rhs) -> bool { return lhs.cpMin < rhs.cpMin; });

        // once we are at it, merge adjacent and / or overlapping ranges

        auto target = begin;
        for (auto source = begin + 1; source != end; ++source)
            if (target->cpMax < source->cpMin)
                *(++target) = *source;
            else
                target->cpMax = max(target->cpMax, source->cpMax);

        ranges.erase(++target, end);
    }

    // translate UTF8 ranges to UTF16 ranges

    size_t utf8Pos  = 0;
    size_t utf16Pos = 0;
    for (auto iter = ranges.begin(), end = ranges.end(); iter != end; ++iter)
    {
        AdvanceUnicodePos(textUTF8, utf8Pos, utf16Pos, iter->cpMin);
        iter->cpMin = static_cast<LONG>(utf16Pos);
        AdvanceUnicodePos(textUTF8, utf8Pos, utf16Pos, iter->cpMax);
        iter->cpMax = static_cast<LONG>(utf16Pos);
    }

    return ranges;
}

// called to parse a (potentially incorrect) regex spec

bool CLogDlgFilter::ValidateRegexp(const char* regexpStr, std::vector<std::regex>& pattrns) const
{
    try
    {
        std::regex_constants::syntax_option_type type = caseSensitive
                                                            ? std::regex_constants::ECMAScript
                                                            : std::regex_constants::ECMAScript | std::regex_constants::icase;

        std::regex pat = std::regex(regexpStr, type);
        pattrns.push_back(pat);
        return true;
    }
    catch (std::exception&)
    {
    }
    return false;
}

// construction utility

void CLogDlgFilter::AddSubString(CString token, Prefix prefix)
{
    if (token.IsEmpty())
        return;

    if (!caseSensitive)
    {
        token.MakeLower();

        // If the search string (UTF16!) is pure ASCII-7,
        // no locale specifics should apply.
        // Exceptions to C-locale are usually either limited
        // to lowercase -> uppercase conversion or they map
        // their lowercase chars beyond U+0x80.

        fastLowerCase &= IsAllASCII7(token);
    }

    // add condition to list

    SCondition condition = {static_cast<const char*>(CUnicodeUtils::GetUTF8(token)), prefix, 0};
    subStringConditions.push_back(condition);

    // update previous conditions

    size_t newPos = subStringConditions.size() - 1;
    if (prefix == Prefix::Or)
        for (size_t i = newPos; i > 0; --i)
        {
            if (subStringConditions[i - 1].nextOrIndex > 0)
                break;

            subStringConditions[i - 1].nextOrIndex = newPos;
        }
}

// construction

CLogDlgFilter::CLogDlgFilter()
    : negate(false)
    , attributeSelector(UINT_MAX)
    , caseSensitive(false)
    , fastLowerCase(false)
    , from(0)
    , to(0)
    , scanRelevantPathsOnly(false)
    , revToKeep(0)
    , scratch(0xfff0)
    , mergedRevs(nullptr)
    , hideNonMergeable(false)
    , minRev(0)
{
}

CLogDlgFilter::CLogDlgFilter(const CLogDlgFilter& rhs)
{
    operator=(rhs);
}

CLogDlgFilter::CLogDlgFilter(const CString& filter,
                             bool           filterWithRegex,
                             DWORD          selectedFilter,
                             bool           caseSensitive,
                             __time64_t from, __time64_t to,
                             bool                    scanRelevantPathsOnly,
                             std::set<svn_revnum_t>* mergedRevs,
                             bool                    hideNonMergeable,
                             svn_revnum_t minRev, svn_revnum_t revToKeep)
    : negate(false)
    , attributeSelector(selectedFilter)
    , caseSensitive(caseSensitive)
    , fastLowerCase(false)
    , from(from)
    , to(to)
    , scanRelevantPathsOnly(scanRelevantPathsOnly)
    , revToKeep(revToKeep)
    , scratch(0xfff0)
    , mergedRevs(mergedRevs)
    , hideNonMergeable(hideNonMergeable)
    , minRev(minRev)
{
    // decode string matching spec

    bool    useRegex   = filterWithRegex && !filter.IsEmpty();
    CString filterText = filter;

    // if the first char is '!', negate the filter
    if (filter.GetLength() && filter[0] == '!')
    {
        negate     = true;
        filterText = filterText.Mid(1);
    }

    if (useRegex)
        useRegex = ValidateRegexp(CUnicodeUtils::GetUTF8(filterText), patterns);

    if (!useRegex)
    {
        fastLowerCase = !caseSensitive;

        // now split the search string into words so we can search for each of them

        int curPos = 0;
        int length = filterText.GetLength();

        while ((curPos < length) && (curPos >= 0))
        {
            // skip spaces

            for (; (curPos < length) && (filterText[curPos] == ' '); ++curPos)
            {
            }

            // has it a prefix?

            Prefix prefix = Prefix::And;
            if (curPos < length)
                switch (filterText[curPos])
                {
                    case '-':
                        prefix = Prefix::AndNot;
                        ++curPos;
                        break;

                    case '+':
                        prefix = Prefix::Or;
                        ++curPos;
                        break;
                }

            // escaped string?

            if ((curPos < length) && (filterText[curPos] == '"'))
            {
                CString subString;
                while (++curPos < length)
                {
                    if (filterText[curPos] == '"')
                    {
                        // double double quotes?

                        if ((++curPos < length) && (filterText[curPos] == '"'))
                        {
                            // keep one and continue within sub-string

                            subString.AppendChar('"');
                        }
                        else
                        {
                            // end of sub-string?

                            if ((curPos >= length) || (filterText[curPos] == ' '))
                            {
                                break;
                            }
                            else
                            {
                                // add to sub-string & continue within it

                                subString.AppendChar('"');
                                subString.AppendChar(filterText[curPos]);
                            }
                        }
                    }
                    else
                    {
                        subString.AppendChar(filterText[curPos]);
                    }
                }

                AddSubString(subString, prefix);
                ++curPos;
                continue;
            }

            // ordinary sub-string

            AddSubString(filterText.Tokenize(L" ", curPos), prefix);
        }
    }
}

// apply filter

namespace
{
// concatenate strings

void AppendString(CStringBuffer& target, const std::string& toAppend)
{
    target.Append(' ');
    target.Append(toAppend);
}

// convert path objects

void GetPath(const LogCache::CDictionaryBasedPath& path, CStringBuffer& target)
{
    target.Append('|');
    char*  buffer = target.GetBuffer(MAX_PATH + 16);
    size_t size   = path.GetPath(buffer, MAX_PATH) - buffer;
    SecureZeroMemory(buffer + size, 16);

    // relative path strings are never empty

    if (CPathUtils::ContainsEscapedChars(buffer, APR_ALIGN(size, 16)))
        size = CPathUtils::Unescape(buffer) - buffer;

    // mark buffer as used

    target.AddSize(size);
}
} // namespace

bool CLogDlgFilter::operator()(const CLogEntryData& entry) const
{
    // quick checks

    if (entry.GetRevision() == revToKeep)
        return true;

    __time64_t date = entry.GetDate();
    if (attributeSelector & LOGFILTER_DATERANGE)
    {
        if ((date < from) || (date > to))
            return false;
    }

    if (hideNonMergeable && mergedRevs && !mergedRevs->empty())
    {
        if (mergedRevs->find(entry.GetRevision()) != mergedRevs->end())
            return false;
        if (entry.GetRevision() < minRev)
            return false;
    }
    if (patterns.empty() && subStringConditions.empty() && !hideNonMergeable)
        return !negate;
    // we need to perform expensive string / pattern matching

    scratch.Clear();
    if (attributeSelector & LOGFILTER_BUGID)
        AppendString(scratch, entry.GetBugIDs());

    if (attributeSelector & LOGFILTER_MESSAGES)
        AppendString(scratch, entry.GetMessage());

    if (attributeSelector & LOGFILTER_PATHS)
    {
        const CLogChangedPathArray& paths = entry.GetChangedPaths();
        for (size_t cpPathIndex = 0, pathCount = paths.GetCount(); cpPathIndex < pathCount; ++cpPathIndex)
        {
            const CLogChangedPath& cPath = paths[cpPathIndex];
            if (!scanRelevantPathsOnly || cPath.IsRelevantForStartPath())
            {
                GetPath(cPath.GetCachedPath(), scratch);
                scratch.Append('|');
                scratch.Append(cPath.GetActionString());

                if (cPath.GetCopyFromRev() > 0)
                {
                    GetPath(cPath.GetCachedCopyFromPath(), scratch);

                    scratch.Append('|');

                    char buffer[10] = {0};
                    _itoa_s(cPath.GetCopyFromRev(), buffer, 10);
                    scratch.Append(buffer);
                }
            }
        }
    }

    if (attributeSelector & LOGFILTER_AUTHORS)
        AppendString(scratch, entry.GetAuthor());
    if (attributeSelector & LOGFILTER_DATE)
        AppendString(scratch, entry.GetDateString());

    if (attributeSelector & LOGFILTER_REVS)
    {
        scratch.Append(' ');

        char buffer[10] = {0};
        _itoa_s(entry.GetRevision(), buffer, 10);
        scratch.Append(buffer);
    }

    return Match(scratch, scratch.GetSize()) ^ negate;
}

CLogDlgFilter& CLogDlgFilter::operator=(const CLogDlgFilter& rhs)
{
    if (this != &rhs)
    {
        attributeSelector     = rhs.attributeSelector;
        caseSensitive         = rhs.caseSensitive;
        from                  = rhs.from;
        to                    = rhs.to;
        scanRelevantPathsOnly = rhs.scanRelevantPathsOnly;
        revToKeep             = rhs.revToKeep;
        negate                = rhs.negate;
        fastLowerCase         = rhs.fastLowerCase;

        subStringConditions = rhs.subStringConditions;
        patterns            = rhs.patterns;

        hideNonMergeable = rhs.hideNonMergeable;
        mergedRevs       = rhs.mergedRevs;
        minRev           = rhs.minRev;

        scratch.Clear();
    }

    return *this;
}

bool CLogDlgFilter::operator==(const CLogDlgFilter& rhs) const
{
    if (this == &rhs)
        return true;

    // if we are using regexes we cannot say whether filters are equal

    if (!patterns.empty() || !rhs.patterns.empty())
        return false;

    // compare global flags

    if (negate != rhs.negate ||
        attributeSelector != rhs.attributeSelector ||
        caseSensitive != rhs.caseSensitive ||
        from != rhs.from ||
        to != rhs.to ||
        scanRelevantPathsOnly != rhs.scanRelevantPathsOnly ||
        hideNonMergeable != rhs.hideNonMergeable ||
        revToKeep != rhs.revToKeep)
        return false;

    // compare sub-string defs

    if (subStringConditions.size() != rhs.subStringConditions.size())
        return false;

    for (auto lhsIt = subStringConditions.begin(), lhsEnd = subStringConditions.end(), rhsIt = rhs.subStringConditions.begin(); lhsIt != lhsEnd; ++rhsIt, ++lhsIt)
    {
        if (lhsIt->subString != rhsIt->subString || lhsIt->prefix != rhsIt->prefix || lhsIt->nextOrIndex != rhsIt->nextOrIndex)
            return false;
    }

    // no difference detected

    return true;
}

bool CLogDlgFilter::operator!=(const CLogDlgFilter& rhs) const
{
    return !operator==(rhs);
}

// std::regex is very slow when running concurrently
// in multiple threads. Empty filters don't need MT as well.

bool CLogDlgFilter::BenefitsFromMT() const
{
    // case-insensitive regular expressions don't like MT

    if (!patterns.empty() && !caseSensitive)
        return false;

    // empty filters don't need MT (and its potential overhead)

    if (patterns.empty() && subStringConditions.empty())
        return false;

    // otherwise, go mult-threading!

    return true;
}

bool CLogDlgFilter::IsFilterActive() const
{
    return !(patterns.empty() && subStringConditions.empty());
}