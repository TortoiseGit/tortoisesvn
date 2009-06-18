// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
		temp = m_cmdUrl.Tokenize(_T("?"), pos);
		if (temp.IsEmpty())
		{
			break;
		}
		else if (temp.Left(8).CompareNoCase(_T("tsvncmd:")) == 0)
			temp = temp.Mid(8);

		if (temp.Left(8).CompareNoCase(_T("command:")) == 0)
		{
			CString cmd = temp.Mid(8);
			bool isCmdAllowed = false;

			if (cmd.CompareNoCase(_T("update")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("commit")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("diff")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("repobrowser")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("checkout")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("export")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("blame")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("repostatus")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("revisiongraph")) == 0)
				isCmdAllowed = true;
			else if (cmd.CompareNoCase(_T("showcompare")) == 0)
				isCmdAllowed = true;

			if (!isCmdAllowed)
				return CString();	// command is not on the allowed list, return empty command line
		}
		temp = CPathUtils::PathUnescape(temp);
		// if the param has spaces in it, enquote it
		if (temp.Find(_T(" ")) >= 0)
		{
			temp.Replace(_T(":"), _T(":\""));
			temp = temp + _T("\"");
		}
		sCmdLine += _T(" /");
		sCmdLine += temp;
	} 

	return sCmdLine;
}