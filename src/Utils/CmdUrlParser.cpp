// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009, 2012, 2014 - TortoiseSVN

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
#include "CmdUrlParser.h"
#include "PathUtils.h"

CmdUrlParser::CmdUrlParser(const CString &url) : m_cmdUrl(url)
{

}

CmdUrlParser::~CmdUrlParser()
{

}

CString CmdUrlParser::GetCommandLine()
{
    CString sCmdLine;
    CString temp;
    int pos = 0;
    for(;;)
    {
        temp = m_cmdUrl.Tokenize(L"?", pos);
        if (temp.IsEmpty())
        {
            break;
        }
        else if (temp.Left(8).CompareNoCase(L"tsvncmd:") == 0)
            temp = temp.Mid(8);

        if (temp.Left(8).CompareNoCase(L"command:") == 0)
        {
            CString cmd = temp.Mid(8);
            bool isCmdAllowed = false;

            if (cmd.CompareNoCase(L"update") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"commit") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"diff") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"repobrowser") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"checkout") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"export") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"blame") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"repostatus") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"revisiongraph") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"showcompare") == 0)
                isCmdAllowed = true;
            else if (cmd.CompareNoCase(L"log") == 0)
                isCmdAllowed = true;

            if (!isCmdAllowed)
                return CString();   // command is not on the allowed list, return empty command line
        }
        temp = CPathUtils::PathUnescape(temp);
        // if the param has spaces in it, enquote it
        if (temp.Find(L" ") >= 0)
        {
            // only insert a quote after the first colon: subsequent colons might be
            // part of an url (e.g., http_:_//something)
            int keyIndex = temp.Find(L":") + 1;
            temp.Insert(keyIndex, L"\"");
            temp = temp + L"\"";
        }
        sCmdLine += L" /";
        sCmdLine += temp;
    }

    return sCmdLine;
}

#ifdef _DEBUG
static class CmdUrlParserTest
{
public:
    CmdUrlParserTest()
    {
        CmdUrlParser p(L"tsvncmd:command:showcompare?url1:https://svn/trunk/Scripts/Desert Storm.ini?url2:https://svn/trunk/Scripts/Desert Storm.ini?revision1:229?revision2:230");
        CString cmdline = p.GetCommandLine();
        ATLASSERT(cmdline==L" /command:showcompare /url1:\"https://svn/trunk/Scripts/Desert Storm.ini\" /url2:\"https://svn/trunk/Scripts/Desert Storm.ini\" /revision1:229 /revision2:230");
    }
} CmdUrlParserTestObject;
#endif
