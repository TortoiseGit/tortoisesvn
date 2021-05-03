// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008, 2010, 2014, 2016, 2019, 2021 - TortoiseSVN

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
#include <fstream>
#include <codecvt>
#include "PersonalDictionary.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"

CPersonalDictionary::CPersonalDictionary(LONG lLanguage /* = 0*/)
    : m_bLoaded(false)
{
    m_lLanguage = lLanguage;
}

CPersonalDictionary::~CPersonalDictionary()
{
}

bool CPersonalDictionary::Load()
{
    char line[PDICT_MAX_WORD_LENGTH + 1]{};

    if (m_bLoaded)
        return true;
    wchar_t path[MAX_PATH] = {0}; //MAX_PATH ok here.
    wcscpy_s(path, CPathUtils::GetAppDataDirectory());

    if (m_lLanguage == 0)
        m_lLanguage = GetUserDefaultLCID();

    wchar_t sLang[10] = {0};
    swprintf_s(sLang, L"%ld", m_lLanguage);
    wcscat_s(path, sLang);
    wcscat_s(path, L".dic");

    std::ifstream file;
    char          filepath[MAX_PATH + 1] = {0};
    WideCharToMultiByte(CP_ACP, NULL, path, -1, filepath, _countof(filepath) - 1, nullptr, nullptr);
    file.open(filepath);
    if (!file.good())
    {
        return false;
    }
    do
    {
        file.getline(line, _countof(line));
        CString sWord = CUnicodeUtils::GetUnicode(line);
        if (!sWord.IsEmpty())
            dict.insert(sWord);
    } while (file.gcount() > 0);
    file.close();
    m_bLoaded = true;
    return true;
}

bool CPersonalDictionary::AddWord(const CString& sWord)
{
    if (!m_bLoaded)
        Load();
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
    auto it   = dict.find(sWord);
    return (it != dict.end());
}

bool CPersonalDictionary::Save()
{
    if (!m_bLoaded)
        return false;
    wchar_t path[MAX_PATH] = {0}; //MAX_PATH ok here.
    wcscpy_s(path, CPathUtils::GetAppDataDirectory());

    if (m_lLanguage == 0)
        m_lLanguage = GetUserDefaultLCID();

    wchar_t sLang[10] = {0};
    swprintf_s(sLang, L"%ld", m_lLanguage);
    wcscat_s(path, sLang);
    wcscat_s(path, L".dic");

    std::ofstream file;
    char          filepath[MAX_PATH + 1] = {0};
    WideCharToMultiByte(CP_ACP, NULL, path, -1, filepath, _countof(filepath) - 1, nullptr, nullptr);
    file.open(filepath, std::ios_base::binary);
    for (std::set<CString>::iterator it = dict.begin(); it != dict.end(); ++it)
    {
        if (!it->IsEmpty())
            file << CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(*it)).c_str() << "\n";
    }
    file.close();
    return true;
}
