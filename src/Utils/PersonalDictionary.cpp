// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
//

#include "StdAfx.h"
#include <fstream>
#include "PersonalDictionary.h"

CPersonalDictionary::CPersonalDictionary(LONG lLanguage /* = 0*/) :
	m_bLoaded(false)
{
	m_lLanguage = lLanguage;
}

CPersonalDictionary::~CPersonalDictionary()
{
}

bool CPersonalDictionary::Load()
{
	CString sWord;
	TCHAR line[PDICT_MAX_WORD_LENGTH + 1];

	if (m_bLoaded)
		return true;
	TCHAR path[MAX_PATH];		//MAX_PATH ok here.
	TCHAR sLang[10];
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path)!=S_OK)
		return false;
	_tcscat_s(path, MAX_PATH, _T("\\TortoiseSVN"));

	if (!PathIsDirectory(path))
		CreateDirectory(path, NULL);

	if (m_lLanguage==0)
		m_lLanguage = GetUserDefaultLCID();

	_stprintf_s(sLang, 10, _T("\\%ld"), m_lLanguage);
	_tcscat_s(path, MAX_PATH, sLang);
	_tcscat_s(path, MAX_PATH, _T(".dic"));

	std::wifstream File;
	char filepath[MAX_PATH+1];
	ZeroMemory(filepath, sizeof(filepath));
	WideCharToMultiByte(CP_ACP, NULL, path, -1, filepath, MAX_PATH, NULL, NULL);
	File.open(filepath);
	if (!File.good())
	{
		return false;
	}
	std::vector<std::wstring> entry;
	do
	{
		File.getline(line, sizeof(line)/sizeof(TCHAR));
		sWord = line;
		dict.insert(sWord);
	} while (File.gcount() > 0);
	File.close();
	m_bLoaded = true;
	return true;
}

bool CPersonalDictionary::AddWord(const CString& sWord)
{
	if (sWord.GetLength() >= PDICT_MAX_WORD_LENGTH)
		return false;
	dict.insert(sWord);
	return true;
}

bool CPersonalDictionary::FindWord(const CString& sWord)
{
	if (!m_bLoaded)
		Load();
	// even if the load failed for some reason, we mark it as loaded
	// and just assume an empty personal dictionary
	m_bLoaded = true;
	std::set<CString>::iterator it;
	it = dict.find(sWord);
	return (it != dict.end());
}

bool CPersonalDictionary::Save()
{
	TCHAR path[MAX_PATH];		//MAX_PATH ok here.
	TCHAR sLang[10];
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path)!=S_OK)
		return false;
	_tcscat_s(path, MAX_PATH, _T("\\TortoiseSVN"));

	if (!PathIsDirectory(path))
		CreateDirectory(path, NULL);

	if (m_lLanguage==0)
		m_lLanguage = GetUserDefaultLCID();

	_stprintf_s(sLang, 10, _T("\\%ld"), m_lLanguage);
	_tcscat_s(path, MAX_PATH, sLang);
	_tcscat_s(path, MAX_PATH, _T(".dic"));

	std::wofstream File;
	char filepath[MAX_PATH+1];
	ZeroMemory(filepath, sizeof(filepath));
	WideCharToMultiByte(CP_ACP, NULL, path, -1, filepath, MAX_PATH, NULL, NULL);
	File.open(filepath, std::ios_base::binary);
	for (std::set<CString>::iterator it = dict.begin(); it != dict.end(); ++it)
	{
		File << (LPCTSTR)*it << _T("\n");
	}
	File.close();
	return true;
}
