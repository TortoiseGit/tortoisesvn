#include "StdAfx.h"
#include "shlwapi.h"
#include <fstream>
#include "Utils.h"
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
		MYERROR;

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
	File.open(filepath);
	TCHAR line[100*1024];
	std::vector<std::wstring> entry;
	do
	{
		File.getline(line, sizeof(line)/sizeof(TCHAR));
		if (line[0]==0)
		{
			//empty line means end of entry!
			RESOURCEENTRY resEntry;
			std::wstring msgid;
			for (std::vector<std::wstring>::iterator I = entry.begin(); I != entry.end(); ++I)
			{
				if (_tcsncmp(I->c_str(), _T("# "), 2)==0)
				{
					//user comment
					resEntry.translatorcomments = I->c_str();
				}
				if (_tcsncmp(I->c_str(), _T("#."), 2)==0)
				{
					//automatic comments
					resEntry.automaticcomments = I->c_str();
				}
				if (_tcsncmp(I->c_str(), _T("#:"), 2)==0)
				{
					//reference
					resEntry.reference = I->c_str();
				}
				if (_tcsncmp(I->c_str(), _T("#,"), 2)==0)
				{
					//flag
					resEntry.flag = I->c_str();
				}
				if (_tcsncmp(I->c_str(), _T("msgid"), 5)==0)
				{
					//message id
					msgid = I->c_str();
					msgid = std::wstring(msgid.substr(7, msgid.size() - 8));
					nEntries++;
				}
				if (_tcsncmp(I->c_str(), _T("msgstr"), 6)==0)
				{
					//message string
					resEntry.msgstr += I->c_str();
					resEntry.msgstr = resEntry.msgstr.substr(8, resEntry.msgstr.length() - 9);
					if (resEntry.msgstr.size()>0)
						nTranslated++;
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
	File.close();
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
	File.open(filepath);
	for (std::map<std::wstring, RESOURCEENTRY>::iterator I = this->begin(); I != this->end(); ++I)
	{
		if (I->second.translatorcomments.length() > 0)
			File << (I->second.translatorcomments.c_str()) << _T("\n");
		if (I->second.automaticcomments.length() > 0)
			File << (I->second.automaticcomments.c_str()) << _T("\n");
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