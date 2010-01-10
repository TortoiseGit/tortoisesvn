// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2010 - TortoiseSVN

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
#include "StdAfx.h"
#include "LogFile.h"
#include "PathUtils.h"

CLogFile::CLogFile(void)
{
	m_maxlines = CRegStdDWORD(_T("Software\\TortoiseSVN\\MaxLinesInLogfile"), 4000);
}

CLogFile::~CLogFile(void)
{
}

bool CLogFile::Open()
{
	CTSVNPath logfile = CTSVNPath(CPathUtils::GetAppDataDirectory() + _T("\\logfile.txt"));
	return Open(logfile);
}

bool CLogFile::Open(const CTSVNPath& logfile)
{
	m_newLines.clear();
	m_logfile = logfile;
	if (!logfile.Exists())
	{
		CPathUtils::MakeSureDirectoryPathExists(logfile.GetContainingDirectory().GetWinPath());
	}

	return true;
}

bool CLogFile::AddLine(const CString& line)
{
	m_newLines.push_back(line);
	return true;
}

bool CLogFile::Close()
{
	std::deque<CString> lines;
	try
	{
		CString strLine;
		CStdioFile file;

		int retrycounter = 10;
		// try to open the file for about two seconds - some other TSVN process might be
		// writing to the file and we just wait a little for this to finish
		while (!file.Open(m_logfile.GetWinPath(), CFile::typeText | CFile::modeReadWrite | CFile::modeCreate | CFile::modeNoTruncate) && retrycounter)
		{
			retrycounter--;
			Sleep(200);
		}
		if (retrycounter == 0)
			return false;

		// this while() loop uses up the most time of this method.
		// Optimizing it isn't feasible, since even reading the file in
		// one block, convert the content to utf-16 and then parse the lines
		// is not faster.
		// the only way to optimize this (haven't tried) would be to
		// not parse the lines at all and just add the new lines to the end
		// of the log file. But then we can't limit the file to a number of
		// lines anymore but only to a certain size.
		while (file.ReadString(strLine))
		{
			lines.push_back(strLine);
		}
		file.SetLength(0);

		AdjustSize(lines);

		for (std::deque<CString>::const_iterator it = lines.begin(); it != lines.end(); ++it)
		{
			file.WriteString(*it);
			file.WriteString(_T("\n"));
		}
		for (std::deque<CString>::const_iterator it = m_newLines.begin(); it != m_newLines.end(); ++it)
		{
			file.WriteString(*it);
			file.WriteString(_T("\n"));
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading autolist regex file\n");
		pE->Delete();
		return false;
	}
	return true;
}

bool CLogFile::AddTimeLine()
{
	CString sLine;
	// first add an empty line as a separator
	m_newLines.push_back(sLine);
	// now add the time string
	TCHAR datebuf[4096] = {0};
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, datebuf, 4096);
	sLine = datebuf;
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, datebuf, 4096);
	sLine += _T(" - ");
	sLine += datebuf;
	m_newLines.push_back(sLine);
	return true;
}

void CLogFile::AdjustSize(std::deque<CString>& lines)
{
	DWORD maxlines = m_maxlines;

	while ((lines.size() + m_newLines.size()) > maxlines)
	{
		lines.pop_front();
	}
}