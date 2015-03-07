// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009, 2011, 2014-2015 - TortoiseSVN
// Copyright (C) 2103 - TortoiseGit

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
#include "CmdLineParser.h"
#include <locale>
#include <algorithm>

const wchar_t CCmdLineParser::m_sDelims[] = L"-/";
const wchar_t CCmdLineParser::m_sQuotes[] = L"\"";
const wchar_t CCmdLineParser::m_sValueSep[] = L" :"; // don't forget space!!


CCmdLineParser::CCmdLineParser(LPCWSTR sCmdLine)
{
    if (sCmdLine)
    {
        Parse(sCmdLine);
    }
}

CCmdLineParser::~CCmdLineParser()
{
    m_valueMap.clear();
}

BOOL CCmdLineParser::Parse(LPCWSTR sCmdLine)
{
    const std::wstring sEmpty = L"";          //use this as a value if no actual value is given in commandline
    int nArgs = 0;

    if (!sCmdLine)
        return false;

    m_valueMap.clear();
    m_sCmdLine = sCmdLine;

    LPCWSTR sCurrent = sCmdLine;

    for (;;)
    {
        //format is  -Key:"arg"

        if (sCurrent[0] == 0)
            break;      // no more data, leave loop

        LPCWSTR sArg = wcspbrk(sCurrent, m_sDelims);
        if (!sArg)
            break; // no (more) delimiters found
        sArg = _wcsinc(sArg);

        if (sArg[0] == 0)
            break; // ends with delim

        LPCWSTR sVal = wcspbrk(sArg, m_sValueSep);
        if (sVal == NULL)
        {
            std::wstring Key(sArg);
            std::transform(Key.begin(), Key.end(), Key.begin(), ::tolower);
            m_valueMap.insert(CValsMap::value_type(Key, sEmpty));
            break;
        }
        else
        {
            std::wstring Key(sArg, (int)(sVal - sArg));
            std::transform(Key.begin(), Key.end(), Key.begin(), ::tolower);

            LPCWSTR sQuote(NULL), sEndQuote(NULL);
            if (wcslen(sVal) > 0)
            {
                if (sVal[0] != ' ')
                    sVal = _wcsinc(sVal);
                else
                {
                    while (wcslen(sVal) > 0 && sVal[0] == ' ')
                        sVal = _wcsinc(sVal);
                }

                LPCWSTR nextArg = wcspbrk(sVal, m_sDelims);

                sQuote = wcspbrk(sVal, m_sQuotes);

                if (nextArg == sVal)
                {
                    // current key has no value, but a next key exist - so don't use next key as value of current one
                    --sVal;
                    sQuote = sVal;
                    sEndQuote = sVal;
                }
                else if (nextArg != NULL && nextArg < sQuote)
                {
                    // current key has a value w/o quotes, but next key one has value in quotes
                    sQuote = sVal;
                    sEndQuote = wcschr(sQuote, ' ');
                }
                else
                {
                    if (sQuote == sVal)
                    {
                        // string with quotes (defined in m_sQuotes, e.g. '")
                        sQuote = _wcsinc(sVal);
                        sEndQuote = wcspbrk(sQuote, m_sQuotes);
                    }
                    else
                    {
                        sQuote = sVal;
                        sEndQuote = wcschr(sQuote, ' ');
                    }
                }
            }

            if (sEndQuote == NULL)
            {
                // no end quotes or terminating space, take the rest of the string to its end
                if (!Key.empty() && sQuote)
                {
                    std::wstring csVal(sQuote);
                    m_valueMap.insert(CValsMap::value_type(Key, csVal));
                }
                break;
            }
            else
            {
                // end quote
                if (!Key.empty())
                {
                    std::wstring csVal(sQuote, (int)(sEndQuote - sQuote));
                    m_valueMap.insert(CValsMap::value_type(Key, csVal));
                }
                sCurrent = _wcsinc(sEndQuote);
                continue;
            }
        }
    }

    return (nArgs > 0);     //TRUE if arguments were found
}

CCmdLineParser::CValsMap::const_iterator CCmdLineParser::findKey(LPCWSTR sKey) const
{
    std::wstring s(sKey);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return m_valueMap.find(s);
}

BOOL CCmdLineParser::HasKey(LPCWSTR sKey) const
{
    CValsMap::const_iterator it = findKey(sKey);
    if (it == m_valueMap.end())
        return false;
    return true;
}


BOOL CCmdLineParser::HasVal(LPCWSTR sKey) const
{
    CValsMap::const_iterator it = findKey(sKey);
    if (it == m_valueMap.end())
        return false;
    if (it->second.empty())
        return false;
    return true;
}

LPCWSTR CCmdLineParser::GetVal(LPCWSTR sKey) const
{
    CValsMap::const_iterator it = findKey(sKey);
    if (it == m_valueMap.end())
        return 0;
    return it->second.c_str();
}

LONG CCmdLineParser::GetLongVal(LPCWSTR sKey) const
{
    CValsMap::const_iterator it = findKey(sKey);
    if (it == m_valueMap.end())
        return 0;
    return _tstol(it->second.c_str());
}

__int64 CCmdLineParser::GetLongLongVal(LPCWSTR sKey) const
{
    CValsMap::const_iterator it = findKey(sKey);
    if (it == m_valueMap.end())
        return 0;
    return _wtoi64(it->second.c_str());
}

CCmdLineParser::ITERPOS CCmdLineParser::begin() const
{
    return m_valueMap.begin();
}

CCmdLineParser::ITERPOS CCmdLineParser::getNext(ITERPOS& pos, std::wstring& sKey, std::wstring& sValue) const
{
    if (m_valueMap.end() == pos)
    {
        sKey.clear();
        return pos;
    }
    else
    {
        sKey = pos->first;
        sValue = pos->second;
        pos++;
        return pos;
    }
}

BOOL CCmdLineParser::isLast(const ITERPOS& pos) const
{
    return (pos == m_valueMap.end());
}
