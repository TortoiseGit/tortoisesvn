// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#include "StdAfx.h"
#include "shlwapi.h"
#include <fstream>
#include "codecvt.h"
#include "Utils.h"
#include "ResModule.h"
#include ".\pofile.h"

#define MYERROR	{CUtils::Error(); return FALSE;}

CPOFile::CPOFile()
{
}

CPOFile::~CPOFile(void)
{
}

BOOL CPOFile::ParseFile(LPCTSTR szPath, BOOL bUpdateExisting /* = TRUE */)
{
	if (!PathFileExists(szPath))
		return FALSE;

	if (!m_bQuiet)
		_tcprintf(_T("parsing file %s...\n"), szPath);

	int nEntries = 0;
	int nDeleted = 0;
	int nTranslated = 0;
	//since stream classes still expect the filepath in char and not wchar_t
	//we need to convert the filepath to multibyte
	char filepath[MAX_PATH+1];
	ZeroMemory(filepath, sizeof(filepath));
	WideCharToMultiByte(CP_ACP, NULL, szPath, -1, filepath, MAX_PATH, NULL, NULL);

	std::wifstream File;
	File.imbue(std::locale(std::locale(), new utf8_conversion()));
	File.open(filepath);
	if (!File.good())
	{
		_ftprintf(stderr, _T("can't open input file %s\n"), szPath);
		return FALSE;
	}
	TCHAR line[2*MAX_STRING_LENGTH];
	std::vector<std::wstring> entry;
	do
	{
		File.getline(line, sizeof(line)/sizeof(TCHAR));
		if (line[0]==0)
		{
			//empty line means end of entry!
			RESOURCEENTRY resEntry;
			std::wstring msgid;
			int type = 0;
			for (std::vector<std::wstring>::iterator I = entry.begin(); I != entry.end(); ++I)
			{
				if (_tcsncmp(I->c_str(), _T("# "), 2)==0)
				{
					//user comment
					resEntry.translatorcomments.push_back(I->c_str());
					type = 0;
				}
				if (_tcsncmp(I->c_str(), _T("#."), 2)==0)
				{
					//automatic comments
					resEntry.automaticcomments.push_back(I->c_str());
					type = 0;
				}
				if (_tcsncmp(I->c_str(), _T("#:"), 2)==0)
				{
					//reference
					resEntry.reference = I->c_str();
					type = 0;
				}
				if (_tcsncmp(I->c_str(), _T("#,"), 2)==0)
				{
					//flag
					resEntry.flag = I->c_str();
					type = 0;
				}
				if (_tcsncmp(I->c_str(), _T("msgid"), 5)==0)
				{
					//message id
					msgid = I->c_str();
					msgid = std::wstring(msgid.substr(7, msgid.size() - 8));
					nEntries++;
					type = 1;
				}
				if (_tcsncmp(I->c_str(), _T("msgstr"), 6)==0)
				{
					//message string
					resEntry.msgstr = I->c_str();
					resEntry.msgstr = resEntry.msgstr.substr(8, resEntry.msgstr.length() - 9);
					if (resEntry.msgstr.size()>0)
						nTranslated++;
					type = 2;
				}
				if (_tcsncmp(I->c_str(), _T("\""), 1)==0)
				{
					if (type == 1)
					{
						std::wstring temp = I->c_str();
						temp = temp.substr(1, temp.length()-2);
						msgid += temp;
					}
					if (type == 2)
					{
						if (resEntry.msgstr.size() == 0)
							nTranslated++;
						std::wstring temp = I->c_str();
						temp = temp.substr(1, temp.length()-2);
						resEntry.msgstr += temp;
					}
				}
			}
			entry.clear();
			if ((!bUpdateExisting)||(this->count(msgid) != 0))
			{
				RESOURCEENTRY e = (*this)[msgid];
				resEntry.reference = e.reference;
				(*this)[msgid] = resEntry;
			}
			else
				nDeleted++;
			msgid.clear();
		} // if (line[0]=='\n')
		else
		{
			entry.push_back(line);
		}
	} while (File.gcount() > 0);
	printf(File.getloc().name().c_str());
	File.close();
	RESOURCEENTRY emptyentry;
	(*this)[std::wstring(_T(""))] = emptyentry;
	if (!m_bQuiet)
		_tcprintf(_T("%d Entries found, %d were already translated and %d got deleted\n"), nEntries, nTranslated, nDeleted);
	return TRUE;
}

BOOL CPOFile::SaveFile(LPCTSTR szPath)
{
	//since stream classes still expect the filepath in char and not wchar_t
	//we need to convert the filepath to multibyte
	char filepath[MAX_PATH+1];
	int nEntries = 0;
	ZeroMemory(filepath, sizeof(filepath));
	WideCharToMultiByte(CP_ACP, NULL, szPath, -1, filepath, MAX_PATH, NULL, NULL);

	std::wofstream File;
//	File.open(filepath, std::ios_base::binary);
//	File << _T("\xEF\xBB\xBF");
//	File.close();
	File.imbue(std::locale(std::locale(), new utf8_conversion()));
	File.open(filepath, std::ios_base::binary);
	File << _T("# SOME DESCRIPTIVE TITLE.\n");
	File << _T("# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER\n");
	File << _T("# This file is distributed under the same license as the PACKAGE package.\n");
	File << _T("# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n");
	File << _T("#\n");
	File << _T("#, fuzzy\n");
	File << _T("msgid \"\"\n");
	File << _T("msgstr \"\"\n");
	File << _T("\"Project-Id-Version: PACKAGE VERSION\\n\"\n");
	File << _T("\"Report-Msgid-Bugs-To: \\n\"\n");
	File << _T("\"POT-Creation-Date: 1900-01-01 00:00+0000\\n\"\n");
	File << _T("\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n");
	File << _T("\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n");
	File << _T("\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n");
	File << _T("\"MIME-Version: 1.0\\n\"\n");
	File << _T("\"Content-Type: text/plain; charset=UTF-8\\n\"\n");
	File << _T("\"Content-Transfer-Encoding: 8bit\\n\"\n\n");

	for (std::map<std::wstring, RESOURCEENTRY>::iterator I = this->begin(); I != this->end(); ++I)
	{
		if (I->first.size() == 0)
			continue;
		RESOURCEENTRY entry = I->second;
		for (std::vector<std::wstring>::iterator II = entry.automaticcomments.begin(); II != entry.automaticcomments.end(); ++II)
		{
			File << II->c_str() << _T("\n");
		}
		for (std::vector<std::wstring>::iterator II = entry.translatorcomments.begin(); II != entry.translatorcomments.end(); ++II)
		{
			File << II->c_str() << _T("\n");
		}
		if (I->second.reference.length() > 0)
			File << (I->second.reference.c_str()) << _T("\n");
		if (I->second.flag.length() > 0)
			File << (I->second.flag.c_str()) << _T("\n");
		File << (_T("msgid \"")) << (I->first.c_str()) << _T("\"\n");
		File << (_T("msgstr \"")) << (I->second.msgstr.c_str()) << _T("\"\n\n");
		nEntries++;
	}
	File.close();
	if (!m_bQuiet)
		_tcprintf(_T("File %s saved, containing %d entries\n"), szPath, nEntries);
	return TRUE;
}
