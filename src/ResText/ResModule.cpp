// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2015-2019, 2022 - TortoiseGit
// Copyright (C) 2003-2008, 2010-2017, 2019, 2021 - TortoiseSVN

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
#include "stdafx.h"
#include "Utils.h"
#include "UnicodeUtils.h"
#include "ResModule.h"
#include "OnOutOfScope.h"
#include <regex>
#include <memory>
#include <fstream>
#include <string>
#include <algorithm>
#include <functional>
#include <locale>
#include <codecvt>

#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <Imagehlp.h>

#pragma warning(pop)

#pragma comment(lib, "Imagehlp.lib")

#ifndef RT_RIBBON
#    define RT_RIBBON MAKEINTRESOURCE(28)
#endif

#define MYERROR          \
    {                    \
        CUtils::Error(); \
        return FALSE;    \
    }

static const WORD* AlignWORD(const WORD* pWord)
{
    const WORD* res = pWord;
    res += (((reinterpret_cast<UINT_PTR>(pWord) + 3) & ~3) - reinterpret_cast<UINT_PTR>(pWord)) / sizeof(WORD);
    return res;
}

std::wstring NumToStr(INT_PTR num)
{
    wchar_t buf[100];
    swprintf_s(buf, L"%Id", num);
    return buf;
}

CResModule::CResModule()
    : m_hResDll(nullptr)
    , m_hUpdateRes(nullptr)
    , m_bQuiet(false)
    , m_bRTL(false)
    , m_bAdjustEOLs(false)
    , m_bTranslatedStrings(0)
    , m_bDefaultStrings(0)
    , m_bTranslatedDialogStrings(0)
    , m_bDefaultDialogStrings(0)
    , m_bTranslatedMenuStrings(0)
    , m_bDefaultMenuStrings(0)
    , m_bTranslatedAcceleratorStrings(0)
    , m_bDefaultAcceleratorStrings(0)
    , m_bTranslatedRibbonTexts(0)
    , m_bDefaultRibbonTexts(0)
    , m_wTargetLang(0)
{
}

CResModule::~CResModule()
{
}

BOOL CResModule::ExtractResources(const std::vector<std::wstring>& fileList, LPCWSTR lpszPoFilePath, BOOL bNoUpdate, LPCWSTR lpszHeaderFile)
{
    for (auto I = fileList.cbegin(); I != fileList.cend(); ++I)
    {
        std::wstring filepath = *I;
        m_currentHeaderDataDialogs.clear();
        m_currentHeaderDataStrings.clear();
        m_currentHeaderDataMenus.clear();
        auto starpos = I->find('*');
        if (starpos != std::wstring::npos)
            filepath = I->substr(0, starpos);
        while (starpos != std::wstring::npos)
        {
            auto         starposnext = I->find('*', starpos + 1);
            std::wstring headerfile  = I->substr(starpos + 1, starposnext - starpos - 1);
            ScanHeaderFile(headerfile);
            starpos = starposnext;
        }
        m_hResDll = LoadLibraryEx(filepath.c_str(), nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
        if (!m_hResDll)
            MYERROR;

        size_t nEntries = m_stringEntries.size();
        // fill in the std::map with all translatable entries

        if (!m_bQuiet)
            fwprintf(stdout, L"Extracting StringTable....");
        EnumResourceNames(m_hResDll, RT_STRING, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
        if (!m_bQuiet)
            fwprintf(stdout, L"%4Iu Strings\n", m_stringEntries.size() - nEntries);
        nEntries = m_stringEntries.size();

        if (!m_bQuiet)
            fwprintf(stdout, L"Extracting Dialogs........");
        EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
        if (!m_bQuiet)
            fwprintf(stdout, L"%4Iu Strings\n", m_stringEntries.size() - nEntries);
        nEntries = m_stringEntries.size();

        if (!m_bQuiet)
            fwprintf(stdout, L"Extracting Menus..........");
        EnumResourceNames(m_hResDll, RT_MENU, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
        if (!m_bQuiet)
            fwprintf(stdout, L"%4Iu Strings\n", m_stringEntries.size() - nEntries);
        nEntries = m_stringEntries.size();
        if (!m_bQuiet)
            fwprintf(stdout, L"Extracting Accelerators...");
        EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
        if (!m_bQuiet)
            fwprintf(stdout, L"%4Iu Accelerators\n", m_stringEntries.size() - nEntries);
        nEntries = m_stringEntries.size();
        if (!m_bQuiet)
            fwprintf(stdout, L"Extracting Ribbons........");
        EnumResourceNames(m_hResDll, RT_RIBBON, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
        if (!m_bQuiet)
            fwprintf(stdout, L"%4Iu Strings\n", m_stringEntries.size() - nEntries);
        nEntries = m_stringEntries.size();

        // parse a probably existing file and update the translations which are
        // already done
        m_stringEntries.ParseFile(lpszPoFilePath, !bNoUpdate, m_bAdjustEOLs);

        FreeLibrary(m_hResDll);
    }

    // at last, save the new file
    return m_stringEntries.SaveFile(lpszPoFilePath, lpszHeaderFile);
}

BOOL CResModule::ExtractResources(LPCWSTR lpszSrcLangDllPath, LPCWSTR lpszPoFilePath, BOOL bNoUpdate, LPCWSTR lpszHeaderFile)
{
    m_hResDll = LoadLibraryEx(lpszSrcLangDllPath, nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
    if (!m_hResDll)
        MYERROR;
    OnOutOfScope(
        if (m_hResDll)
            FreeLibrary(m_hResDll);)

    size_t nEntries = 0;
    // fill in the std::map with all translatable entries

    if (!m_bQuiet)
        fwprintf(stdout, L"Extracting StringTable....");
    EnumResourceNames(m_hResDll, RT_STRING, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4Iu Strings\n", m_stringEntries.size());
    nEntries = m_stringEntries.size();

    if (!m_bQuiet)
        fwprintf(stdout, L"Extracting Dialogs........");
    EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4Iu Strings\n", m_stringEntries.size() - nEntries);
    nEntries = m_stringEntries.size();

    if (!m_bQuiet)
        fwprintf(stdout, L"Extracting Menus..........");
    EnumResourceNames(m_hResDll, RT_MENU, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4Iu Strings\n", m_stringEntries.size() - nEntries);
    nEntries = m_stringEntries.size();

    if (!m_bQuiet)
        fwprintf(stdout, L"Extracting Accelerators...");
    EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4Iu Accelerators\n", m_stringEntries.size() - nEntries);
    nEntries = m_stringEntries.size();

    // parse a probably existing file and update the translations which are
    // already done
    m_stringEntries.ParseFile(lpszPoFilePath, !bNoUpdate, m_bAdjustEOLs);

    // at last, save the new file
    if (!m_stringEntries.SaveFile(lpszPoFilePath, lpszHeaderFile))
        return FALSE;

    return TRUE;
}

BOOL CResModule::CreateTranslatedResources(LPCWSTR lpszSrcLangDllPath, LPCWSTR lpszDestLangDllPath, LPCWSTR lpszPoFilePath)
{
    if (!CopyFile(lpszSrcLangDllPath, lpszDestLangDllPath, FALSE))
        MYERROR;

    RemoveSignatures(lpszDestLangDllPath);

    int count = 0;
    do
    {
        m_hResDll = LoadLibraryEx(lpszSrcLangDllPath, nullptr, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_IGNORE_CODE_AUTHZ_LEVEL);
        if (!m_hResDll)
            Sleep(100);
        count++;
    } while (!m_hResDll && (count < 10));

    if (!m_hResDll)
        MYERROR;

    OnOutOfScope(
        if (m_hResDll)
            FreeLibrary(m_hResDll);)

        sDestFile = std::wstring(lpszDestLangDllPath);

    // get all translated strings
    if (!m_stringEntries.ParseFile(lpszPoFilePath, FALSE, m_bAdjustEOLs))
        MYERROR;
    m_bTranslatedStrings            = 0;
    m_bDefaultStrings               = 0;
    m_bTranslatedDialogStrings      = 0;
    m_bDefaultDialogStrings         = 0;
    m_bTranslatedMenuStrings        = 0;
    m_bDefaultMenuStrings           = 0;
    m_bTranslatedAcceleratorStrings = 0;
    m_bDefaultAcceleratorStrings    = 0;

    BOOL bRes = FALSE;
    count     = 0;
    do
    {
        m_hUpdateRes = BeginUpdateResource(sDestFile.c_str(), FALSE);
        if (!m_hUpdateRes)
            Sleep(100);
        count++;
    } while (!m_hUpdateRes && (count < 10));

    if (!m_hUpdateRes)
        MYERROR;

    if (!m_bQuiet)
        fwprintf(stdout, L"Translating StringTable...");
    bRes = EnumResourceNames(m_hResDll, RT_STRING, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedStrings, m_bDefaultStrings);

    if (!m_bQuiet)
        fwprintf(stdout, L"Translating Dialogs.......");
    bRes = EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedDialogStrings, m_bDefaultDialogStrings);

    if (!m_bQuiet)
        fwprintf(stdout, L"Translating Menus.........");
    bRes = EnumResourceNames(m_hResDll, RT_MENU, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedMenuStrings, m_bDefaultMenuStrings);

    if (!m_bQuiet)
        fwprintf(stdout, L"Translating Accelerators..");
    bRes = EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedAcceleratorStrings, m_bDefaultAcceleratorStrings);

    if (!m_bQuiet)
        fwprintf(stdout, L"Translating Ribbons.......");
    bRes = EnumResourceNames(m_hResDll, RT_RIBBON, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
    if (!m_bQuiet)
        fwprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedRibbonTexts, m_bDefaultRibbonTexts);
    bRes = TRUE;
    if (!EndUpdateResource(m_hUpdateRes, !bRes))
        MYERROR;

    AdjustCheckSum(sDestFile);

    return TRUE;
}

void CResModule::RemoveSignatures(LPCWSTR lpszDestLangDllPath)
{
    // Remove any signatures in the file:
    // if we don't remove it here, the signature will be invalid after
    // we modify this file, and the signtool.exe will throw an error and refuse to sign it again.
    auto hFile = CreateFile(lpszDestLangDllPath, FILE_READ_DATA | FILE_WRITE_DATA, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    DWORD certCount = 0;
    DWORD indices[100]{};
    ImageEnumerateCertificates(hFile, CERT_SECTION_TYPE_ANY, &certCount, indices, _countof(indices));

    for (DWORD i = 0; i < certCount; ++i)
        ImageRemoveCertificate(hFile, i);

    CloseHandle(hFile);
}

BOOL CResModule::ExtractString(LPCWSTR lpszType)
{
    HRSRC   hrsrc          = FindResource(m_hResDll, lpszType, RT_STRING);
    HGLOBAL hglStringTable = nullptr;
    LPWSTR  p              = nullptr;

    if (!hrsrc)
        MYERROR;
    hglStringTable = LoadResource(m_hResDll, hrsrc);

    OnOutOfScope(
        UnlockResource(hglStringTable);
        FreeResource(hglStringTable););

    if (!hglStringTable)
        MYERROR;
    p = static_cast<LPWSTR>(LockResource(hglStringTable));

    if (!p)
        MYERROR;
    /*  [Block of 16 strings.  The strings are Pascal style with a WORD
    length preceding the string.  16 strings are always written, even
    if not all slots are full.  Any slots in the block with no string
    have a zero WORD for the length.]
    */

    //first check how much memory we need
    LPWSTR pp = p;
    for (int i = 0; i < 16; ++i)
    {
        int len = GET_WORD(pp);
        pp++;
        std::wstring msgId                      = std::wstring(pp, len);
        wchar_t      buf[MAX_STRING_LENGTH * 2] = {0};
        wcscpy_s(buf, msgId.c_str());
        CUtils::StringExtend(buf);

        if (buf[0])
        {
            std::wstring str   = std::wstring(buf);
            auto         entry = m_stringEntries[str];
            InsertResourceIDs(RT_STRING, 0, entry, (reinterpret_cast<INT_PTR>(lpszType) - 1) * 16 + i, L"");
            if (wcschr(str.c_str(), '%'))
                entry.flag = L"#, c-format";
            m_stringEntries[str] = entry;
        }
        pp += len;
    }
    UnlockResource(hglStringTable);
    FreeResource(hglStringTable);
    return TRUE;
}

BOOL CResModule::ReplaceString(LPCWSTR lpszType, WORD wLanguage)
{
    HRSRC   hrsrc          = FindResourceEx(m_hResDll, RT_STRING, lpszType, wLanguage);
    HGLOBAL hglStringTable = nullptr;
    LPWSTR  p              = nullptr;

    if (!hrsrc)
        MYERROR;
    hglStringTable = LoadResource(m_hResDll, hrsrc);

    OnOutOfScope(
        UnlockResource(hglStringTable);
        FreeResource(hglStringTable););

    if (!hglStringTable)
        MYERROR;
    p = static_cast<LPWSTR>(LockResource(hglStringTable));

    if (!p)
        MYERROR;
    /*  [Block of 16 strings.  The strings are Pascal style with a WORD
    length preceding the string.  16 strings are always written, even
    if not all slots are full.  Any slots in the block with no string
    have a zero WORD for the length.]
*/

    //first check how much memory we need
    size_t nMem = 0;
    LPWSTR pp   = p;
    for (int i = 0; i < 16; ++i)
    {
        nMem++;
        size_t len = GET_WORD(pp);
        pp++;
        std::wstring msgid                      = std::wstring(pp, len);
        wchar_t      buf[MAX_STRING_LENGTH * 2] = {0};
        wcscpy_s(buf, msgid.c_str());
        CUtils::StringExtend(buf);
        msgid = std::wstring(buf);

        auto resEntry = m_stringEntries[msgid];
        wcscpy_s(buf, resEntry.msgStr.empty() ? msgid.c_str() : resEntry.msgStr.c_str());
        ReplaceWithRegex(buf);
        CUtils::StringCollapse(buf);
        size_t newLen = wcslen(buf);
        if (newLen)
            nMem += newLen;
        else
            nMem += len;
        pp += len;
    }

    auto newTable = std::make_unique<WORD[]>(nMem + (nMem % 2));
    ZeroMemory(newTable.get(), (nMem + (nMem % 2)) * sizeof(WORD));

    size_t index = 0;
    for (int i = 0; i < 16; ++i)
    {
        int len = GET_WORD(p);
        p++;
        std::wstring msgId                      = std::wstring(p, len);
        wchar_t      buf[MAX_STRING_LENGTH * 2] = {0};
        wcscpy_s(buf, msgId.c_str());
        CUtils::StringExtend(buf);
        msgId = std::wstring(buf);

        auto resEntry = m_stringEntries[msgId];
        wcscpy_s(buf, resEntry.msgStr.empty() ? msgId.c_str() : resEntry.msgStr.c_str());
        ReplaceWithRegex(buf);
        CUtils::StringCollapse(buf);
        size_t newlen = wcslen(buf);
        if (newlen)
        {
            newTable.get()[index++] = static_cast<WORD>(newlen);
            wcsncpy(reinterpret_cast<wchar_t*>(&newTable.get()[index]), buf, newlen);
            index += newlen;
            m_bTranslatedStrings++;
        }
        else
        {
            newTable.get()[index++] = static_cast<WORD>(len);
            if (len)
                wcsncpy(reinterpret_cast<wchar_t*>(&newTable.get()[index]), p, len);
            index += len;
            if (len)
                m_bDefaultStrings++;
        }
        p += len;
    }

    if (!UpdateResource(m_hUpdateRes, RT_STRING, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newTable.get(), static_cast<DWORD>(nMem + (nMem % 2)) * sizeof(WORD)))
        MYERROR;

    if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_STRING, lpszType, wLanguage, nullptr, 0)))
        MYERROR;

    return TRUE;
}

BOOL CResModule::ExtractMenu(LPCWSTR lpszType)
{
    HRSRC       hrsrc           = FindResource(m_hResDll, lpszType, RT_MENU);
    HGLOBAL     hglMenuTemplate = nullptr;
    WORD        version = 0, offset = 0;
    const WORD *p = nullptr, *p0 = nullptr;

    if (!hrsrc)
        MYERROR;

    hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

    if (!hglMenuTemplate)
        MYERROR;
    OnOutOfScope(FreeResource(hglMenuTemplate);)

    p = static_cast<const WORD*>(LockResource(hglMenuTemplate));

    if (!p)
        MYERROR;
    OnOutOfScope(UnlockResource(hglMenuTemplate);)
    // Standard MENU resource
    //struct MenuHeader {
    //  WORD   wVersion;           // Currently zero
    //  WORD   cbHeaderSize;       // Also zero
    //};

    // MENUEX resource
    //struct MenuExHeader {
    //    WORD wVersion;           // One
    //    WORD wOffset;
    //    DWORD dwHelpId;
    //};
    p0      = p;
    version = GET_WORD(p);

    p++;

    switch (version)
    {
        case 0:
        {
            offset = GET_WORD(p);
            p += offset;
            p++;
            if (!ParseMenuResource(p))
                MYERROR;
        }
        break;
        case 1:
        {
            offset = GET_WORD(p);
            p++;
            //dwHelpId = GET_DWORD(p);
            if (!ParseMenuExResource(p0 + offset))
                MYERROR;
        }
        break;
        default:
            MYERROR;
    }

    return TRUE;
}

BOOL CResModule::ReplaceMenu(LPCWSTR lpszType, WORD wLanguage)
{
    HRSRC   hrsrc           = FindResourceEx(m_hResDll, RT_MENU, lpszType, wLanguage);
    HGLOBAL hglMenuTemplate = nullptr;
    WORD    version = 0, offset = 0;
    LPWSTR  p  = nullptr;
    WORD*   p0 = nullptr;

    if (!hrsrc)
        MYERROR; //just the language wasn't found

    hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

    if (!hglMenuTemplate)
        MYERROR;

    p = static_cast<LPWSTR>(LockResource(hglMenuTemplate));

    if (!p)
        MYERROR;

    //struct MenuHeader {
    //  WORD   wVersion;           // Currently zero
    //  WORD   cbHeaderSize;       // Also zero
    //};

    // MENUEX resource
    //struct MenuExHeader {
    //    WORD wVersion;           // One
    //    WORD wOffset;
    //    DWORD dwHelpId;
    //};
    p0      = reinterpret_cast<WORD*>(p);
    version = GET_WORD(p);

    p++;

    switch (version)
    {
        case 0:
        {
            offset = GET_WORD(p);
            p += offset;
            p++;
            size_t nMem = 0;
            if (!CountMemReplaceMenuResource(reinterpret_cast<WORD*>(p), &nMem, nullptr))
                MYERROR;
            auto newMenu = std::make_unique<WORD[]>(nMem + (nMem % 2) + 2);
            ZeroMemory(newMenu.get(), (nMem + (nMem % 2) + 2) * sizeof(WORD));
            size_t index = 2; // MenuHeader has 2 WORDs zero
            if (!CountMemReplaceMenuResource(reinterpret_cast<WORD*>(p), &index, newMenu.get()))
                MYERROR;

            if (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newMenu.get(), static_cast<DWORD>(nMem + (nMem % 2) + 2) * sizeof(WORD)))
                MYERROR;

            if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, wLanguage, nullptr, 0)))
                MYERROR;
        }
        break;
        case 1:
        {
            offset = GET_WORD(p);
            p++;
            //dwHelpId = GET_DWORD(p);
            size_t nMem = 0;
            if (!CountMemReplaceMenuExResource(reinterpret_cast<WORD*>(p0 + offset), &nMem, nullptr))
                MYERROR;
            auto newMenu = std::make_unique<WORD[]>(nMem + (nMem % 2) + 4);
            ZeroMemory(newMenu.get(), (nMem + (nMem % 2) + 4) * sizeof(WORD));
            CopyMemory(newMenu.get(), p0, 2 * sizeof(WORD) + sizeof(DWORD));
            size_t index = 4; // MenuExHeader has 2 x WORD + 1 x DWORD
            if (!CountMemReplaceMenuExResource(reinterpret_cast<WORD*>(p0 + offset), &index, newMenu.get()))
                MYERROR;

            if (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newMenu.get(), static_cast<DWORD>(nMem + (nMem % 2) + 4) * sizeof(DWORD)))
                MYERROR;

            if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, wLanguage, nullptr, 0)))
                MYERROR;
        }
        break;
        default:
            MYERROR;
    }

    return TRUE;
}

const WORD* CResModule::ParseMenuResource(const WORD* res)
{
    WORD flags = 0;
    WORD id    = 0;

    //struct PopupMenuItem {
    //  WORD   fItemFlags;
    //  WCHAR  szItemText[];
    //};
    //struct NormalMenuItem {
    //  WORD   fItemFlags;
    //  WORD   wMenuID;
    //  WCHAR  szItemText[];
    //};

    do
    {
        flags = GET_WORD(res);
        res++;
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res); //normal menu item
            res++;
        }
        else
            id = static_cast<WORD>(-1); //popup menu item

        LPCWSTR str = reinterpret_cast<LPCWSTR>(res);
        size_t  l   = wcslen(str) + 1;
        res += l;

        if (flags & MF_POPUP)
        {
            wchar_t buf[MAX_STRING_LENGTH] = {0};
            wcscpy_s(buf, str);
            CUtils::StringExtend(buf);

            std::wstring wStr  = std::wstring(buf);
            auto         entry = m_stringEntries[wStr];
            if (id)
                InsertResourceIDs(RT_MENU, 0, entry, id, L" - PopupMenu");

            m_stringEntries[wStr] = entry;

            if ((res = ParseMenuResource(res)) == nullptr)
                return nullptr;
        }
        else if (id != 0)
        {
            wchar_t buf[MAX_STRING_LENGTH] = {0};
            wcscpy_s(buf, str);
            CUtils::StringExtend(buf);

            std::wstring wStr  = std::wstring(buf);
            auto         entry = m_stringEntries[wStr];
            InsertResourceIDs(RT_MENU, 0, entry, id, L" - Menu");

            wchar_t szTempBuf[1024] = {0};
            swprintf_s(szTempBuf, L"#: MenuEntry; ID:%u", id);
            MENUENTRY menuEntry;
            menuEntry.wID       = id;
            menuEntry.reference = szTempBuf;
            menuEntry.msgstr    = wStr;

            m_stringEntries[wStr] = entry;
            m_menuEntries[id]     = menuEntry;
        }
    } while (!(flags & MF_END));
    return res;
}

const WORD* CResModule::CountMemReplaceMenuResource(const WORD* res, size_t* wordcount, WORD* newMenu)
{
    WORD flags = 0;
    WORD id    = 0;

    //struct PopupMenuItem {
    //  WORD   fItemFlags;
    //  WCHAR  szItemText[];
    //};
    //struct NormalMenuItem {
    //  WORD   fItemFlags;
    //  WORD   wMenuID;
    //  WCHAR  szItemText[];
    //};

    do
    {
        flags = GET_WORD(res);
        res++;
        if (!newMenu)
            (*wordcount)++;
        else
            newMenu[(*wordcount)++] = flags;
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res); //normal menu item
            res++;
            if (!newMenu)
                (*wordcount)++;
            else
                newMenu[(*wordcount)++] = id;
        }
        else
            id = static_cast<WORD>(-1); //popup menu item

        if (flags & MF_POPUP)
        {
            ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;

            if ((res = CountMemReplaceMenuResource(res, wordcount, newMenu)) == nullptr)
                return nullptr;
        }
        else if (id != 0)
        {
            ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
        }
        else
        {
            if (newMenu)
                wcscpy(reinterpret_cast<wchar_t*>(&newMenu[(*wordcount)]), reinterpret_cast<LPCWSTR>(res));
            (*wordcount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
        }
    } while (!(flags & MF_END));
    return res;
}

const WORD* CResModule::ParseMenuExResource(const WORD* res)
{
    WORD bResInfo;

    //struct MenuExItem {
    //    DWORD dwType;
    //    DWORD dwState;
    //    DWORD menuId;
    //    WORD bResInfo;
    //    WCHAR szText[];
    //    DWORD dwHelpId; - Popup menu only
    //};

    do
    {
        DWORD dwType = GET_DWORD(res);
        res += 2;
        //dwState = GET_DWORD(res);
        res += 2;
        DWORD menuId = GET_DWORD(res);
        res += 2;
        bResInfo = GET_WORD(res);
        res++;

        LPCWSTR str = reinterpret_cast<LPCWSTR>(res);
        size_t  l   = wcslen(str) + 1;
        res += l;
        // Align to DWORD boundary
        res = AlignWORD(res);

        if (dwType & MFT_SEPARATOR)
            continue;

        if (bResInfo & 0x01)
        {
            // Popup menu - note this can also have a non-zero ID
            if (menuId == 0)
                menuId = static_cast<WORD>(-1);
            wchar_t buf[MAX_STRING_LENGTH] = {0};
            wcscpy_s(buf, str);
            CUtils::StringExtend(buf);

            std::wstring wstr  = std::wstring(buf);
            auto         entry = m_stringEntries[wstr];
            // Popup has a DWORD help entry on a DWORD boundary - skip over it
            res += 2;

            InsertResourceIDs(RT_MENU, 0, entry, menuId, L" - PopupMenuEx");
            wchar_t szTempBuf[1024] = {0};
            swprintf_s(szTempBuf, L"#: MenuExPopupEntry; ID:%lu", menuId);
            MENUENTRY menuEntry;
            menuEntry.wID                            = static_cast<WORD>(menuId);
            menuEntry.reference                      = szTempBuf;
            menuEntry.msgstr                         = wstr;
            m_stringEntries[wstr]                    = entry;
            m_menuEntries[static_cast<WORD>(menuId)] = menuEntry;

            if ((res = ParseMenuExResource(res)) == nullptr)
                return nullptr;
        }
        else if (menuId != 0)
        {
            wchar_t buf[MAX_STRING_LENGTH] = {0};
            wcscpy_s(buf, str);
            CUtils::StringExtend(buf);

            std::wstring wStr  = std::wstring(buf);
            auto         entry = m_stringEntries[wStr];
            InsertResourceIDs(RT_MENU, 0, entry, menuId, L" - MenuEx");

            wchar_t szTempBuf[1024] = {0};
            swprintf_s(szTempBuf, L"#: MenuExEntry; ID:%lu", menuId);
            MENUENTRY menuEntry;
            menuEntry.wID                            = static_cast<WORD>(menuId);
            menuEntry.reference                      = szTempBuf;
            menuEntry.msgstr                         = wStr;
            m_stringEntries[wStr]                    = entry;
            m_menuEntries[static_cast<WORD>(menuId)] = menuEntry;
        }
    } while (!(bResInfo & 0x80));
    return res;
}

const WORD* CResModule::CountMemReplaceMenuExResource(const WORD* res, size_t* wordCount, WORD* newMenu)
{
    WORD bResInfo;

    //struct MenuExItem {
    //    DWORD dwType;
    //    DWORD dwState;
    //    DWORD menuId;
    //    WORD bResInfo;
    //    WCHAR szText[];
    //    DWORD dwHelpId; - Popup menu only
    //};

    do
    {
        WORD* p0     = const_cast<WORD*>(res);
        DWORD dwType = GET_DWORD(res);
        res += 2;
        //dwState = GET_DWORD(res);
        res += 2;
        DWORD menuId = GET_DWORD(res);
        res += 2;
        bResInfo = GET_WORD(res);
        res++;

        if (newMenu)
            CopyMemory(&newMenu[*wordCount], p0, 7 * sizeof(WORD));

        (*wordCount) += 7;

        if (dwType & MFT_SEPARATOR)
        {
            // Align to DWORD
            (*wordCount)++;
            res++;
            continue;
        }

        if (bResInfo & 0x01)
        {
            ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordCount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
            // Align to DWORD
            res = AlignWORD(res);
            if ((*wordCount) & 0x01)
                (*wordCount)++;

            if (newMenu)
                CopyMemory(&newMenu[*wordCount], res, sizeof(DWORD)); // Copy Help ID

            res += 2;
            (*wordCount) += 2;

            if ((res = CountMemReplaceMenuExResource(res, wordCount, newMenu)) == nullptr)
                return nullptr;
        }
        else if (menuId != 0)
        {
            ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordCount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
        }
        else
        {
            if (newMenu)
                wcscpy(reinterpret_cast<wchar_t*>(&newMenu[(*wordCount)]), reinterpret_cast<LPCWSTR>(res));
            (*wordCount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
        }
        // Align to DWORD
        res = AlignWORD(res);
        if ((*wordCount) & 0x01)
            (*wordCount)++;
    } while (!(bResInfo & 0x80));
    return res;
}

BOOL CResModule::ExtractAccelerator(LPCWSTR lpszType)
{
    HRSRC       hrsrc       = FindResource(m_hResDll, lpszType, RT_ACCELERATOR);
    HGLOBAL     hglAccTable = nullptr;
    WORD        fFlags = 0, wAnsi = 0, wID = 0;
    const WORD* p = nullptr;
    bool        bEnd(false);

    if (!hrsrc)
        MYERROR;

    hglAccTable = LoadResource(m_hResDll, hrsrc);

    if (!hglAccTable)
        MYERROR;
    OnOutOfScope(FreeResource(hglAccTable);)

    p = static_cast<const WORD*>(LockResource(hglAccTable));

    if (!p)
        MYERROR;
    OnOutOfScope(UnlockResource(hglAccTable);)
    /*
    struct ACCELTABLEENTRY
    {
        WORD fFlags;        FVIRTKEY, FSHIFT, FCONTROL, FALT, 0x80 - Last in a table
        WORD wAnsi;         ANSI character
        WORD wId;           Keyboard accelerator passed to windows
        WORD padding;       # bytes added to ensure aligned to DWORD boundary
    };
    */

    do
    {
        fFlags = GET_WORD(p);
        p++;
        wAnsi = GET_WORD(p);
        p++;
        wID = GET_WORD(p);
        p++;
        p++; // Skip over padding

        if ((fFlags & 0x80) == 0x80)
        { // 0x80
            bEnd = true;
        }

        if ((wAnsi < 0x30) ||
            (wAnsi > 0x5A) ||
            (wAnsi >= 0x3A && wAnsi <= 0x40))
            continue;

        wchar_t buf[1024]  = {0};
        wchar_t buf2[1024] = {0};

        // include the menu ID in the msgid to make sure that 'duplicate'
        // accelerator keys are listed in the po-file.
        // without this, we would get entries like this:
        //#. Accelerator Entry for Menu ID:32809; '&Filter'
        //#. Accelerator Entry for Menu ID:57636; '&Find'
        //#: Corresponding Menu ID:32771; '&Find'
        //msgid "V C +F"
        //msgStr ""
        //
        // Since "filter" and "find" are most likely translated to words starting
        // with different letters, we need to have a separate accelerator entry
        // for each of those
        swprintf_s(buf, L"ID:%u:", wID);

        // EXACTLY 5 characters long "ACS+X"
        // V = Virtual key (or blank if not used)
        // A = Alt key     (or blank if not used)
        // C = Ctrl key    (or blank if not used)
        // S = Shift key   (or blank if not used)
        // X = upper case character
        // e.g. "V CS+Q" == Ctrl + Shift + 'Q'
        if ((fFlags & FVIRTKEY) == FVIRTKEY) // 0x01
            wcscat_s(buf, L"V");
        else
            wcscat_s(buf, L" ");

        if ((fFlags & FALT) == FALT) // 0x10
            wcscat_s(buf, L"A");
        else
            wcscat_s(buf, L" ");

        if ((fFlags & FCONTROL) == FCONTROL) // 0x08
            wcscat_s(buf, L"C");
        else
            wcscat_s(buf, L" ");

        if ((fFlags & FSHIFT) == FSHIFT) // 0x04
            wcscat_s(buf, L"S");
        else
            wcscat_s(buf, L" ");

        swprintf_s(buf2, L"%s+%c", buf, wAnsi);

        std::wstring wStr      = std::wstring(buf2);
        auto         aKeyEntry = m_stringEntries[wStr];

        wchar_t      szTempBuf[1024] = {0};
        std::wstring wMenu;
        auto         iter = m_menuEntries.find(wID);
        if (iter != m_menuEntries.end())
        {
            wMenu = iter->second.msgstr;
        }
        swprintf_s(szTempBuf, L"#. Accelerator Entry for Menu ID:%u; '%s'", wID, wMenu.c_str());
        aKeyEntry.automaticComments.push_back(std::wstring(szTempBuf));

        m_stringEntries[wStr] = aKeyEntry;
    } while (!bEnd);

    return TRUE;
}

BOOL CResModule::ReplaceAccelerator(LPCWSTR lpszType, WORD wLanguage)
{
    LPACCEL     lpaccelNew     = nullptr; // pointer to new accelerator table
    HACCEL      haccelOld      = nullptr; // handle to old accelerator table
    int         cAccelerators  = 0;       // number of accelerators in table
    HGLOBAL     hglAccTableNew = nullptr;
    const WORD* p              = nullptr;
    int         i              = 0;

    haccelOld = LoadAccelerators(m_hResDll, lpszType);

    if (!haccelOld)
        MYERROR;

    cAccelerators = CopyAcceleratorTable(haccelOld, nullptr, 0);

    lpaccelNew = static_cast<LPACCEL>(LocalAlloc(LPTR, cAccelerators * sizeof(ACCEL)));

    if (!lpaccelNew)
        MYERROR;
    OnOutOfScope(LocalFree(lpaccelNew););

    CopyAcceleratorTable(haccelOld, lpaccelNew, cAccelerators);

    // Find the accelerator that the user modified
    // and change its flags and virtual-key code
    // as appropriate.

    for (i = 0; i < cAccelerators; i++)
    {
        if ((lpaccelNew[i].key < 0x30) ||
            (lpaccelNew[i].key > 0x5A) ||
            (lpaccelNew[i].key >= 0x3A && lpaccelNew[i].key <= 0x40))
            continue;

        BYTE    xfVirt;
        wchar_t xkey       = {0};
        wchar_t buf[1024]  = {0};
        wchar_t buf2[1024] = {0};

        swprintf_s(buf, L"ID:%d:", lpaccelNew[i].cmd);

        // get original key combination
        if ((lpaccelNew[i].fVirt & FVIRTKEY) == FVIRTKEY) // 0x01
            wcscat_s(buf, L"V");
        else
            wcscat_s(buf, L" ");

        if ((lpaccelNew[i].fVirt & FALT) == FALT) // 0x10
            wcscat_s(buf, L"A");
        else
            wcscat_s(buf, L" ");

        if ((lpaccelNew[i].fVirt & FCONTROL) == FCONTROL) // 0x08
            wcscat_s(buf, L"C");
        else
            wcscat_s(buf, L" ");

        if ((lpaccelNew[i].fVirt & FSHIFT) == FSHIFT) // 0x04
            wcscat_s(buf, L"S");
        else
            wcscat_s(buf, L" ");

        swprintf_s(buf2, L"%s+%c", buf, lpaccelNew[i].key);

        // Is it there?
        auto pAkIter = m_stringEntries.find(buf2);
        if (pAkIter != m_stringEntries.end())
        {
            m_bTranslatedAcceleratorStrings++;
            xfVirt             = 0;
            xkey               = 0;
            std::wstring wtemp = pAkIter->second.msgStr;
            wtemp              = wtemp.substr(wtemp.find_last_of(':') + 1);
            if (wtemp.size() != 6)
                continue;
            if (wtemp.compare(0, 1, L"V") == 0)
                xfVirt |= FVIRTKEY;
            else if (wtemp.compare(0, 1, L" ") != 0)
                continue; // not a space - user must have made a mistake when translating
            if (wtemp.compare(1, 1, L"A") == 0)
                xfVirt |= FALT;
            else if (wtemp.compare(1, 1, L" ") != 0)
                continue; // not a space - user must have made a mistake when translating
            if (wtemp.compare(2, 1, L"C") == 0)
                xfVirt |= FCONTROL;
            else if (wtemp.compare(2, 1, L" ") != 0)
                continue; // not a space - user must have made a mistake when translating
            if (wtemp.compare(3, 1, L"S") == 0)
                xfVirt |= FSHIFT;
            else if (wtemp.compare(3, 1, L" ") != 0)
                continue; // not a space - user must have made a mistake when translating
            if (wtemp.compare(4, 1, L"+") == 0)
            {
                swscanf_s(wtemp.substr(5, 1).c_str(), L"%c", &xkey, 1);
                lpaccelNew[i].fVirt = xfVirt;
                lpaccelNew[i].key   = static_cast<DWORD>(xkey);
            }
        }
        else
            m_bDefaultAcceleratorStrings++;
    }

    // Create the new accelerator table
    hglAccTableNew = LocalAlloc(LPTR, cAccelerators * 4LL * sizeof(WORD));
    if (!hglAccTableNew)
        MYERROR;
    OnOutOfScope(LocalFree(hglAccTableNew););
    p = static_cast<WORD*>(hglAccTableNew);
    lpaccelNew[cAccelerators - 1].fVirt |= 0x80;
    for (i = 0; i < cAccelerators; i++)
    {
        memcpy(static_cast<void*>(const_cast<WORD*>(p)), &lpaccelNew[i].fVirt, 1);
        p++;
        memcpy(static_cast<void*>(const_cast<WORD*>(p)), &lpaccelNew[i].key, sizeof(WORD));
        p++;
        memcpy(static_cast<void*>(const_cast<WORD*>(p)), &lpaccelNew[i].cmd, sizeof(WORD));
        p++;
        p++;
    }

    if (!UpdateResource(m_hUpdateRes, RT_ACCELERATOR, lpszType,
                        (m_wTargetLang ? m_wTargetLang : wLanguage), hglAccTableNew /* haccelNew*/, cAccelerators * 4 * sizeof(WORD)))
    {
        MYERROR;
    }

    if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_ACCELERATOR, lpszType, wLanguage, nullptr, 0)))
    {
        MYERROR;
    }

    return TRUE;
}

BOOL CResModule::ExtractDialog(LPCWSTR lpszType)
{
    const WORD* lpDlg     = nullptr;
    const WORD* lpDlgItem = nullptr;
    DIALOGINFO  dlg{};
    DLGITEMINFO dlgItem{};
    WORD        bNumControls     = 0;
    HRSRC       hrsrc            = nullptr;
    HGLOBAL     hGlblDlgTemplate = nullptr;

    hrsrc = FindResource(m_hResDll, lpszType, RT_DIALOG);

    if (!hrsrc)
        MYERROR;

    hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);
    if (!hGlblDlgTemplate)
        MYERROR;

    lpDlg = static_cast<const WORD*>(LockResource(hGlblDlgTemplate));

    if (!lpDlg)
        MYERROR;

    lpDlgItem    = static_cast<const WORD*>(GetDialogInfo(lpDlg, &dlg));
    bNumControls = dlg.nbItems;

    if (dlg.caption)
    {
        wchar_t buf[MAX_STRING_LENGTH] = {0};
        wcscpy_s(buf, dlg.caption);
        CUtils::StringExtend(buf);

        std::wstring wStr  = std::wstring(buf);
        auto         entry = m_stringEntries[wStr];
        InsertResourceIDs(RT_DIALOG, reinterpret_cast<INT_PTR>(lpszType), entry, reinterpret_cast<INT_PTR>(lpszType), L"");

        m_stringEntries[wStr] = entry;
    }

    while (bNumControls-- != 0)
    {
        wchar_t szTitle[500] = {0};
        BOOL    bCode;

        lpDlgItem = GetControlInfo(const_cast<WORD*>(lpDlgItem), &dlgItem, dlg.dialogEx, &bCode);

        if (bCode == FALSE)
            wcsncpy_s(szTitle, dlgItem.windowName, _countof(szTitle) - 1);

        if (szTitle[0])
        {
            CUtils::StringExtend(szTitle);

            std::wstring wstr  = std::wstring(szTitle);
            auto         entry = m_stringEntries[wstr];
            InsertResourceIDs(RT_DIALOG, reinterpret_cast<INT_PTR>(lpszType), entry, dlgItem.id, L"");

            m_stringEntries[wstr] = entry;
        }
    }

    UnlockResource(hGlblDlgTemplate);
    FreeResource(hGlblDlgTemplate);
    return (TRUE);
}

BOOL CResModule::ReplaceDialog(LPCWSTR lpszType, WORD wLanguage)
{
    const WORD* lpDlg            = nullptr;
    HRSRC       hrsrc            = nullptr;
    HGLOBAL     hGlblDlgTemplate = nullptr;

    hrsrc = FindResourceEx(m_hResDll, RT_DIALOG, lpszType, wLanguage);

    if (!hrsrc)
        MYERROR;

    hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);

    if (!hGlblDlgTemplate)
        MYERROR;

    lpDlg = static_cast<WORD*>(LockResource(hGlblDlgTemplate));

    OnOutOfScope(
        UnlockResource(hGlblDlgTemplate);
        FreeResource(hGlblDlgTemplate););

    if (!lpDlg)
        MYERROR;

    size_t      nMem = 0;
    const WORD* p    = lpDlg;
    if (!CountMemReplaceDialogResource(p, &nMem, nullptr))
        MYERROR;
    auto newDialog = std::make_unique<WORD[]>(nMem + (nMem % 2));
    ZeroMemory(newDialog.get(), (nMem + (nMem % 2)) * sizeof(WORD));

    size_t index = 0;
    if (!CountMemReplaceDialogResource(lpDlg, &index, newDialog.get()))
        MYERROR;

    if (!UpdateResource(m_hUpdateRes, RT_DIALOG, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newDialog.get(), static_cast<DWORD>(nMem + (nMem % 2)) * sizeof(WORD)))
        MYERROR;

    if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_DIALOG, lpszType, wLanguage, nullptr, 0)))
        MYERROR;

    return TRUE;
}

const WORD* CResModule::GetDialogInfo(const WORD* pTemplate, LPDIALOGINFO lpDlgInfo)
{
    const WORD* p = static_cast<const WORD*>(pTemplate);

    lpDlgInfo->style = GET_DWORD(p);
    p += 2;

    if (lpDlgInfo->style == 0xffff0001) // DIALOGEX resource
    {
        lpDlgInfo->dialogEx = TRUE;
        lpDlgInfo->helpId   = GET_DWORD(p);
        p += 2;
        lpDlgInfo->exStyle = GET_DWORD(p);
        p += 2;
        lpDlgInfo->style = GET_DWORD(p);
        p += 2;
    }
    else
    {
        lpDlgInfo->dialogEx = FALSE;
        lpDlgInfo->helpId   = 0;
        lpDlgInfo->exStyle  = GET_DWORD(p);
        p += 2;
    }

    lpDlgInfo->nbItems = GET_WORD(p);
    p++;

    lpDlgInfo->x = GET_WORD(p);
    p++;

    lpDlgInfo->y = GET_WORD(p);
    p++;

    lpDlgInfo->cx = GET_WORD(p);
    p++;

    lpDlgInfo->cy = GET_WORD(p);
    p++;

    // Get the menu name

    switch (GET_WORD(p))
    {
        case 0x0000:
            lpDlgInfo->menuName = nullptr;
            p++;
            break;
        case 0xffff:
            lpDlgInfo->menuName = reinterpret_cast<LPCWSTR>(static_cast<WORD>(GET_WORD(p + 1)));
            p += 2;
            break;
        default:
            lpDlgInfo->menuName = reinterpret_cast<LPCWSTR>(p);
            p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
            break;
    }

    // Get the class name

    switch (GET_WORD(p))
    {
        case 0x0000:
            lpDlgInfo->className = static_cast<LPCWSTR>(MAKEINTATOM(32770));
            p++;
            break;
        case 0xffff:
            lpDlgInfo->className = reinterpret_cast<LPCWSTR>(static_cast<WORD>(GET_WORD(p + 1)));
            p += 2;
            break;
        default:
            lpDlgInfo->className = reinterpret_cast<LPCWSTR>(p);
            p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
            break;
    }

    // Get the window caption

    lpDlgInfo->caption = reinterpret_cast<LPCWSTR>(p);
    p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;

    // Get the font name

    if (lpDlgInfo->style & DS_SETFONT)
    {
        lpDlgInfo->pointSize = GET_WORD(p);
        p++;

        if (lpDlgInfo->dialogEx)
        {
            lpDlgInfo->weight = GET_WORD(p);
            p++;
            lpDlgInfo->italic = LOBYTE(GET_WORD(p));
            p++;
        }
        else
        {
            lpDlgInfo->weight = FW_DONTCARE;
            lpDlgInfo->italic = FALSE;
        }

        lpDlgInfo->faceName = reinterpret_cast<LPCWSTR>(p);
        p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
    }
    // First control is on DWORD boundary
    p = AlignWORD(p);

    return p;
}

const WORD* CResModule::GetControlInfo(const WORD* p, LPDLGITEMINFO lpDlgItemInfo, BOOL dialogEx, LPBOOL bIsID)
{
    if (dialogEx)
    {
        lpDlgItemInfo->helpId = GET_DWORD(p);
        p += 2;
        lpDlgItemInfo->exStyle = GET_DWORD(p);
        p += 2;
        lpDlgItemInfo->style = GET_DWORD(p);
        p += 2;
    }
    else
    {
        lpDlgItemInfo->helpId = 0;
        lpDlgItemInfo->style  = GET_DWORD(p);
        p += 2;
        lpDlgItemInfo->exStyle = GET_DWORD(p);
        p += 2;
    }

    lpDlgItemInfo->x = GET_WORD(p);
    p++;

    lpDlgItemInfo->y = GET_WORD(p);
    p++;

    lpDlgItemInfo->cx = GET_WORD(p);
    p++;

    lpDlgItemInfo->cy = GET_WORD(p);
    p++;

    if (dialogEx)
    {
        // ID is a DWORD for DIALOGEX
        lpDlgItemInfo->id = static_cast<WORD>(GET_DWORD(p));
        p += 2;
    }
    else
    {
        lpDlgItemInfo->id = GET_WORD(p);
        p++;
    }

    if (GET_WORD(p) == 0xffff)
    {
        GET_WORD(p + 1LL);

        p += 2;
    }
    else
    {
        lpDlgItemInfo->className = reinterpret_cast<LPCWSTR>(p);
        p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
    }

    if (GET_WORD(p) == 0xffff) // an integer ID?
    {
        *bIsID                    = TRUE;
        lpDlgItemInfo->windowName = reinterpret_cast<LPCWSTR>(static_cast<UINT_PTR>(GET_WORD(p + 1)));
        p += 2;
    }
    else
    {
        *bIsID                    = FALSE;
        lpDlgItemInfo->windowName = reinterpret_cast<LPCWSTR>(p);
        p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
    }

    if (GET_WORD(p))
    {
        lpDlgItemInfo->data = static_cast<LPVOID>(const_cast<WORD*>(p + 1));
        p += GET_WORD(p) / sizeof(WORD);
    }
    else
        lpDlgItemInfo->data = nullptr;

    p++;
    // Next control is on DWORD boundary
    p = AlignWORD(p);
    return p;
}

const WORD* CResModule::CountMemReplaceDialogResource(const WORD* res, size_t* wordCount, WORD* newDialog)
{
    BOOL  bEx   = FALSE;
    DWORD style = GET_DWORD(res);
    if (newDialog)
    {
        newDialog[(*wordCount)++] = GET_WORD(res++);
        newDialog[(*wordCount)++] = GET_WORD(res++);
    }
    else
    {
        res += 2;
        (*wordCount) += 2;
    }

    if (style == 0xffff0001) // DIALOGEX resource
    {
        bEx = TRUE;
        if (newDialog)
        {
            newDialog[(*wordCount)++] = GET_WORD(res++); //help id
            newDialog[(*wordCount)++] = GET_WORD(res++); //help id
            newDialog[(*wordCount)++] = GET_WORD(res++); //exStyle
            newDialog[(*wordCount)++] = GET_WORD(res++); //exStyle
            style                     = GET_DWORD(res);
            newDialog[(*wordCount)++] = GET_WORD(res++); //style
            newDialog[(*wordCount)++] = GET_WORD(res++); //style
        }
        else
        {
            res += 4;
            style = GET_DWORD(res);
            res += 2;
            (*wordCount) += 6;
        }
    }
    else
    {
        bEx = FALSE;
        if (newDialog)
        {
            newDialog[(*wordCount)++] = GET_WORD(res++); //exStyle
            newDialog[(*wordCount)++] = GET_WORD(res++); //exStyle
            //style = GET_DWORD(res);
            //newDialog[(*wordCount)++] = GET_WORD(res++);  //style
            //newDialog[(*wordCount)++] = GET_WORD(res++);  //style
        }
        else
        {
            res += 2;
            (*wordCount) += 2;
        }
    }

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res);
    WORD nbItems = GET_WORD(res);
    (*wordCount)++;
    res++;

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //x
    (*wordCount)++;
    res++;

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //y
    (*wordCount)++;
    res++;

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //cx
    (*wordCount)++;
    res++;

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //cy
    (*wordCount)++;
    res++;

    // Get the menu name

    switch (GET_WORD(res))
    {
        case 0x0000:
            if (newDialog)
                newDialog[(*wordCount)] = GET_WORD(res);
            (*wordCount)++;
            res++;
            break;
        case 0xffff:
            if (newDialog)
            {
                newDialog[(*wordCount)++] = GET_WORD(res++);
                newDialog[(*wordCount)++] = GET_WORD(res++);
            }
            else
            {
                (*wordCount) += 2;
                res += 2;
            }
            break;
        default:
            if (newDialog)
            {
                wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordCount)]), reinterpret_cast<LPCWSTR>(res));
            }
            (*wordCount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
            break;
    }

    // Get the class name

    switch (GET_WORD(res))
    {
        case 0x0000:
            if (newDialog)
                newDialog[(*wordCount)] = GET_WORD(res);
            (*wordCount)++;
            res++;
            break;
        case 0xffff:
            if (newDialog)
            {
                newDialog[(*wordCount)++] = GET_WORD(res++);
                newDialog[(*wordCount)++] = GET_WORD(res++);
            }
            else
            {
                (*wordCount) += 2;
                res += 2;
            }
            break;
        default:
            if (newDialog)
            {
                wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordCount)]), reinterpret_cast<LPCWSTR>(res));
            }
            (*wordCount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
            res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
            break;
    }

    // Get the window caption

    ReplaceStr(reinterpret_cast<LPCWSTR>(res), newDialog, wordCount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
    res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;

    // Get the font name

    if (style & DS_SETFONT)
    {
        if (newDialog)
            newDialog[(*wordCount)] = GET_WORD(res);
        res++;
        (*wordCount)++;

        if (bEx)
        {
            if (newDialog)
            {
                newDialog[(*wordCount)++] = GET_WORD(res++);
                newDialog[(*wordCount)++] = GET_WORD(res++);
            }
            else
            {
                res += 2;
                (*wordCount) += 2;
            }
        }

        if (newDialog)
            wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordCount)]), reinterpret_cast<LPCWSTR>(res));
        (*wordCount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
        res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
    }
    // First control is on DWORD boundary
    while ((*wordCount) % 2)
        (*wordCount)++;
    while (reinterpret_cast<UINT_PTR>(res) % 4)
        res++;

    while (nbItems--)
    {
        res = ReplaceControlInfo(res, wordCount, newDialog, bEx);
    }
    return res;
}

const WORD* CResModule::ReplaceControlInfo(const WORD* res, size_t* wordCount, WORD* newDialog, BOOL bEx)
{
    if (bEx)
    {
        if (newDialog)
        {
            newDialog[(*wordCount)++] = GET_WORD(res++); //helpid
            newDialog[(*wordCount)++] = GET_WORD(res++); //helpid
        }
        else
        {
            res += 2;
            (*wordCount) += 2;
        }
    }
    if (newDialog)
    {
        LONG* exStyle             = reinterpret_cast<LONG*>(&newDialog[(*wordCount)]);
        newDialog[(*wordCount)++] = GET_WORD(res++); //exStyle
        newDialog[(*wordCount)++] = GET_WORD(res++); //exStyle
        if (m_bRTL)
            *exStyle |= WS_EX_RTLREADING;
    }
    else
    {
        res += 2;
        (*wordCount) += 2;
    }

    if (newDialog)
    {
        newDialog[(*wordCount)++] = GET_WORD(res++); //style
        newDialog[(*wordCount)++] = GET_WORD(res++); //style
    }
    else
    {
        res += 2;
        (*wordCount) += 2;
    }

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //x
    res++;
    (*wordCount)++;

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //y
    res++;
    (*wordCount)++;

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //cx
    res++;
    (*wordCount)++;

    if (newDialog)
        newDialog[(*wordCount)] = GET_WORD(res); //cy
    res++;
    (*wordCount)++;

    if (bEx)
    {
        // ID is a DWORD for DIALOGEX
        if (newDialog)
        {
            newDialog[(*wordCount)++] = GET_WORD(res++);
            newDialog[(*wordCount)++] = GET_WORD(res++);
        }
        else
        {
            res += 2;
            (*wordCount) += 2;
        }
    }
    else
    {
        if (newDialog)
            newDialog[(*wordCount)] = GET_WORD(res);
        res++;
        (*wordCount)++;
    }

    if (GET_WORD(res) == 0xffff) //classID
    {
        if (newDialog)
        {
            newDialog[(*wordCount)++] = GET_WORD(res++);
            newDialog[(*wordCount)++] = GET_WORD(res++);
        }
        else
        {
            res += 2;
            (*wordCount) += 2;
        }
    }
    else
    {
        if (newDialog)
            wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordCount)]), reinterpret_cast<LPCWSTR>(res));
        (*wordCount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
        res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
    }

    if (GET_WORD(res) == 0xffff) // an integer ID?
    {
        if (newDialog)
        {
            newDialog[(*wordCount)++] = GET_WORD(res++);
            newDialog[(*wordCount)++] = GET_WORD(res++);
        }
        else
        {
            res += 2;
            (*wordCount) += 2;
        }
    }
    else
    {
        ReplaceStr(reinterpret_cast<LPCWSTR>(res), newDialog, wordCount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
        res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
    }

    if (newDialog)
        memcpy(&newDialog[(*wordCount)], res, (GET_WORD(res) + 1LL) * sizeof(WORD));
    (*wordCount) += (GET_WORD(res) + 1LL);
    res += (GET_WORD(res) + 1LL);
    // Next control is on DWORD boundary
    while ((*wordCount) % 2)
        (*wordCount)++;
    res = AlignWORD(res);

    return res;
}

BOOL CResModule::ExtractRibbon(LPCWSTR lpszType)
{
    HRSRC       hrsrc = FindResource(m_hResDll, lpszType, RT_RIBBON);
    HGLOBAL     hglRibbonTemplate;
    const BYTE* p;

    if (!hrsrc)
        MYERROR;

    hglRibbonTemplate = LoadResource(m_hResDll, hrsrc);
    if (!hglRibbonTemplate)
        MYERROR;
    OnOutOfScope(FreeResource(hglRibbonTemplate);)

    DWORD sizeres = SizeofResource(m_hResDll, hrsrc);

    if (!hglRibbonTemplate)
        MYERROR;

    p = static_cast<const BYTE*>(LockResource(hglRibbonTemplate));

    if (!p)
        MYERROR;
    OnOutOfScope(UnlockResource(hglRibbonTemplate);)

    // Resource consists of one single string
    // that is XML.

    // extract all <id><name>blah1</name><value>blah2</value></id><text>blah</text> elements

    const std::regex           regRevMatch("<ID><NAME>([^<]+)</NAME><VALUE>([^<]+)</VALUE></ID><TEXT>([^<]+)</TEXT>");
    std::string                ss = std::string(reinterpret_cast<const char*>(p), sizeres);
    const std::sregex_iterator end;
    for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatch); it != end; ++it)
    {
        std::wstring strIdNameVal = CUnicodeUtils::StdGetUnicode((*it)[1].str());
        strIdNameVal += L" - Ribbon name";

        std::wstring strIdVal = CUnicodeUtils::StdGetUnicode((*it)[2].str());

        std::wstring str = CUnicodeUtils::StdGetUnicode((*it)[3].str());

        auto entry = m_stringEntries[str];
        InsertResourceIDs(RT_RIBBON, 0, entry, std::stoi(strIdVal), strIdNameVal.c_str());
        if (wcschr(str.c_str(), '%'))
            entry.flag = L"#, c-format";
        m_stringEntries[str] = entry;
        m_bDefaultRibbonTexts++;
    }

    // extract all </ELEMENT_NAME><NAME>blahblah</NAME> elements

    const std::regex regRevMatchName("</ELEMENT_NAME><NAME>([^<]+)</NAME>");
    for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatchName); it != end; ++it)
    {
        std::wstring ret   = CUnicodeUtils::StdGetUnicode((*it)[1].str());
        auto         entry = m_stringEntries[ret];
        InsertResourceIDs(RT_RIBBON, 0, entry, reinterpret_cast<INT_PTR>(lpszType), L" - Ribbon element");
        if (wcschr(ret.c_str(), '%'))
            entry.flag = L"#, c-format";
        m_stringEntries[ret] = entry;
        m_bDefaultRibbonTexts++;
    }

    return TRUE;
}

BOOL CResModule::ReplaceRibbon(LPCWSTR lpszType, WORD wLanguage)
{
    HRSRC       hrsrc = FindResource(m_hResDll, lpszType, RT_RIBBON);
    HGLOBAL     hglRibbonTemplate;
    const BYTE* p;

    if (!hrsrc)
        MYERROR;

    hglRibbonTemplate = LoadResource(m_hResDll, hrsrc);

    if (!hglRibbonTemplate)
        MYERROR;
    OnOutOfScope(FreeResource(hglRibbonTemplate);)

    DWORD sizeres = SizeofResource(m_hResDll, hrsrc);

    p = static_cast<const BYTE*>(LockResource(hglRibbonTemplate));

    if (!p)
        MYERROR;
    OnOutOfScope(UnlockResource(hglRibbonTemplate);)

    std::string  ss  = std::string(reinterpret_cast<const char*>(p), sizeres);
    std::wstring ssw = CUnicodeUtils::StdGetUnicode(ss);

    const std::regex           regRevMatch("<TEXT>([^<]+)</TEXT>");
    const std::sregex_iterator end;
    for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatch); it != end; ++it)
    {
        std::wstring ret = CUnicodeUtils::StdGetUnicode((*it)[1].str());

        auto entry = m_stringEntries[ret];
        ret        = L"<TEXT>" + ret + L"</TEXT>";

        if (entry.msgStr.size())
        {
            size_t bufLen = entry.msgStr.size() + 10;
            auto   sBuf   = std::make_unique<wchar_t[]>(bufLen);
            wcscpy_s(sBuf.get(), bufLen, entry.msgStr.c_str());
            CUtils::StringCollapse(sBuf.get());
            ReplaceWithRegex(sBuf.get(), bufLen);
            std::wstring sreplace = L"<TEXT>";
            sreplace += sBuf.get();
            sreplace += L"</TEXT>";
            CUtils::SearchReplace(ssw, ret, sreplace);
            m_bTranslatedRibbonTexts++;
        }
        else
            m_bDefaultRibbonTexts++;
    }

    const std::regex regRevMatchName("</ELEMENT_NAME><NAME>([^<]+)</NAME>");
    for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatchName); it != end; ++it)
    {
        std::wstring ret = CUnicodeUtils::StdGetUnicode((*it)[1].str());

        auto entry = m_stringEntries[ret];
        ret        = L"</ELEMENT_NAME><NAME>" + ret + L"</NAME>";

        if (entry.msgStr.size())
        {
            size_t bufLen = entry.msgStr.size() + 10;
            auto   sBuf   = std::make_unique<wchar_t[]>(bufLen);
            wcscpy_s(sBuf.get(), bufLen, entry.msgStr.c_str());
            CUtils::StringCollapse(sBuf.get());
            ReplaceWithRegex(sBuf.get(), bufLen);
            std::wstring sreplace = L"</ELEMENT_NAME><NAME>";
            sreplace += sBuf.get();
            sreplace += L"</NAME>";
            CUtils::SearchReplace(ssw, ret, sreplace);
            m_bTranslatedRibbonTexts++;
        }
        else
            m_bDefaultRibbonTexts++;
    }

    auto buf                 = std::make_unique<char[]>(ssw.size() * 4 + 1);
    int  lengthIncTerminator = WideCharToMultiByte(CP_UTF8, 0, ssw.c_str(), -1, buf.get(), static_cast<int>(ssw.size()) * 4, nullptr, nullptr); //

    if (!UpdateResource(m_hUpdateRes, RT_RIBBON, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), buf.get(), lengthIncTerminator - 1))
        MYERROR;

    if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_RIBBON, lpszType, wLanguage, nullptr, 0)))
        MYERROR;

    return TRUE;
}

std::wstring CResModule::ReplaceWithRegex(WCHAR* pBuf, size_t bufferSize)
{
    for (const auto& t : m_stringEntries.m_regexes)
    {
        try
        {
            std::wregex e(std::get<0>(t), std::regex_constants::icase);
            auto        replaced = std::regex_replace(pBuf, e, std::get<1>(t));
            wcscpy_s(pBuf, bufferSize, replaced.c_str());
        }
        catch (std::exception&)
        {
        }
    }
    return pBuf;
}

std::wstring CResModule::ReplaceWithRegex(std::wstring& s)
{
    for (const auto& t : m_stringEntries.m_regexes)
    {
        try
        {
            std::wregex e(std::get<0>(t), std::regex_constants::icase);
            auto        replaced = std::regex_replace(s, e, std::get<1>(t));
            s                    = replaced;
        }
        catch (std::exception&)
        {
        }
    }
    return s;
}

BOOL CALLBACK CResModule::EnumResNameCallback(HMODULE /*hModule*/, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
{
    auto lpResModule = reinterpret_cast<CResModule*>(lParam);

    if (lpszType == RT_STRING)
    {
        if (IS_INTRESOURCE(lpszName))
        {
            if (!lpResModule->ExtractString(lpszName))
                return FALSE;
        }
    }
    else if (lpszType == RT_MENU)
    {
        if (IS_INTRESOURCE(lpszName))
        {
            if (!lpResModule->ExtractMenu(lpszName))
                return FALSE;
        }
    }
    else if (lpszType == RT_DIALOG)
    {
        if (IS_INTRESOURCE(lpszName))
        {
            if (!lpResModule->ExtractDialog(lpszName))
                return FALSE;
        }
    }
    else if (lpszType == RT_ACCELERATOR)
    {
        if (IS_INTRESOURCE(lpszName))
        {
            if (!lpResModule->ExtractAccelerator(lpszName))
                return FALSE;
        }
    }
    else if (lpszType == RT_RIBBON)
    {
        if (IS_INTRESOURCE(lpszName))
        {
            if (!lpResModule->ExtractRibbon(lpszName))
                return FALSE;
        }
    }

    return TRUE;
}

BOOL CALLBACK CResModule::EnumResNameWriteCallback(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
{
    auto lpResModule = reinterpret_cast<CResModule*>(lParam);
    return EnumResourceLanguages(hModule, lpszType, lpszName, reinterpret_cast<ENUMRESLANGPROCW>(&lpResModule->EnumResWriteLangCallback), lParam);
}

BOOL CALLBACK CResModule::EnumResWriteLangCallback(HMODULE /*hModule*/, LPCWSTR lpszType, LPWSTR lpszName, WORD wLanguage, LONG_PTR lParam)
{
    BOOL bRes        = FALSE;
    auto lpResModule = reinterpret_cast<CResModule*>(lParam);

    if (lpszType == RT_STRING)
    {
        bRes = lpResModule->ReplaceString(lpszName, wLanguage);
    }
    else if (lpszType == RT_MENU)
    {
        bRes = lpResModule->ReplaceMenu(lpszName, wLanguage);
    }
    else if (lpszType == RT_DIALOG)
    {
        bRes = lpResModule->ReplaceDialog(lpszName, wLanguage);
    }
    else if (lpszType == RT_ACCELERATOR)
    {
        bRes = lpResModule->ReplaceAccelerator(lpszName, wLanguage);
    }
    else if (lpszType == RT_RIBBON)
    {
        bRes = lpResModule->ReplaceRibbon(lpszName, wLanguage);
    }

    return bRes;
}

void CResModule::ReplaceStr(LPCWSTR src, WORD* dest, size_t* count, int* translated, int* def)
{
    wchar_t buf[MAX_STRING_LENGTH] = {0};
    wcscpy_s(buf, src);
    CUtils::StringExtend(buf);

    std::wstring wStr = std::wstring(buf);
    ReplaceWithRegex(buf);
    auto entry = m_stringEntries[wStr];
    if (!entry.msgStr.empty())
    {
        wcscpy_s(buf, entry.msgStr.c_str());
        ReplaceWithRegex(buf);
        CUtils::StringCollapse(buf);
        if (dest)
            wcscpy(reinterpret_cast<wchar_t*>(&dest[(*count)]), buf);
        (*count) += wcslen(buf) + 1;
        (*translated)++;
    }
    else
    {
        if (wcscmp(buf, wStr.c_str()))
        {
            if (dest)
                wcscpy(reinterpret_cast<wchar_t*>(&dest[(*count)]), buf);
            (*count) += wcslen(buf) + 1;
            (*translated)++;
        }
        else
        {
            if (dest)
                wcscpy(reinterpret_cast<wchar_t*>(&dest[(*count)]), src);
            (*count) += wcslen(src) + 1;
            if (wcslen(src))
                (*def)++;
        }
    }
}

static bool StartsWith(const std::string& heystacl, const char* needle)
{
    return heystacl.compare(0, strlen(needle), needle) == 0;
}

static bool StartsWith(const std::wstring& heystacl, const wchar_t* needle)
{
    return heystacl.compare(0, wcslen(needle), needle) == 0;
}

size_t CResModule::ScanHeaderFile(const std::wstring& filepath)
{
    size_t count = 0;

    // open the file and read the contents
    DWORD reqLen     = GetFullPathName(filepath.c_str(), 0, nullptr, nullptr);
    auto  wcfullPath = std::make_unique<wchar_t[]>(reqLen + 1LL);
    GetFullPathName(filepath.c_str(), reqLen, wcfullPath.get(), nullptr);
    std::wstring fullpath = wcfullPath.get();

    // first treat the file as ASCII and try to get the defines
    {
        std::ifstream fin(fullpath);
        std::string   fileLine;
        while (std::getline(fin, fileLine))
        {
            auto defpos = fileLine.find("#define");
            if (defpos != std::string::npos)
            {
                std::string text = fileLine.substr(defpos + 7);
                trim(text);
                auto spacepos = text.find(' ');
                if (spacepos == std::string::npos)
                    spacepos = text.find('\t');
                if (spacepos != std::string::npos)
                {
                    auto value = atol(text.substr(spacepos).c_str());
                    if (value == 0 && text.substr(spacepos).find("0x") != std::string::npos)
                        value = std::stoul(text.substr(spacepos), nullptr, 16);
                    text = text.substr(0, spacepos);
                    trim(text);
                    if (StartsWith(text, "IDS_"))
                    {
                        m_currentHeaderDataStrings[value] = CUnicodeUtils::StdGetUnicode(text);
                        ++count;
                    }
                    else if (StartsWith(text, "IDD_"))
                    {
                        m_currentHeaderDataDialogs[value] = CUnicodeUtils::StdGetUnicode(text);
                        ++count;
                    }
                    else if (StartsWith(text, "ID_"))
                    {
                        m_currentHeaderDataMenus[value] = CUnicodeUtils::StdGetUnicode(text);
                        ++count;
                    }
                    else if (StartsWith(text, "cmd"))
                    {
                        m_currentHeaderDataStrings[value] = CUnicodeUtils::StdGetUnicode(text);
                        ++count;
                    }
                    else if (text.find("_RESID") != std::string::npos)
                    {
                        m_currentHeaderDataStrings[value] = CUnicodeUtils::StdGetUnicode(text);
                        ++count;
                    }
                }
            }
        }
    }

    // now try the same with the file treated as utf16
    {
        // open as a byte stream
        std::ifstream wFin(fullpath, std::ios::binary);
        // apply BOM-sensitive UTF-16 facet
        //std::wifstream wfin(fullpath);
        std::string fileLine;
        while (std::getline(wFin, fileLine))
        {
            auto defpos = fileLine.find("#define");
            if (defpos != std::wstring::npos)
            {
                std::wstring text = CUnicodeUtils::StdGetUnicode(fileLine.substr(defpos + 7));
                trim(text);
                auto spacepos = text.find(' ');
                if (spacepos == std::wstring::npos)
                    spacepos = text.find('\t');
                if (spacepos != std::wstring::npos)
                {
                    auto value = _wtol(text.substr(spacepos).c_str());
                    if (value == 0 && text.substr(spacepos).find(L"0x") != std::wstring::npos)
                        value = std::stoul(text.substr(spacepos), nullptr, 16);
                    text = text.substr(0, spacepos);
                    trim(text);
                    if (StartsWith(text, L"IDS_"))
                    {
                        m_currentHeaderDataStrings[value] = text;
                        ++count;
                    }
                    else if (StartsWith(text, L"IDD_"))
                    {
                        m_currentHeaderDataDialogs[value] = text;
                        ++count;
                    }
                    else if (StartsWith(text, L"ID_"))
                    {
                        m_currentHeaderDataMenus[value] = text;
                        ++count;
                    }
                    else if (StartsWith(text, L"cmd"))
                    {
                        m_currentHeaderDataStrings[value] = text;
                        ++count;
                    }
                    else if (text.find(L"_RESID") != std::string::npos)
                    {
                        m_currentHeaderDataStrings[value] = text;
                        ++count;
                    }
                }
            }
        }
    }

    return count;
}

void CResModule::InsertResourceIDs(LPCWSTR lpType, INT_PTR mainId, TagResourceEntry& entry, INT_PTR id, LPCWSTR infoText)
{
    if (lpType == RT_DIALOG)
    {
        auto foundIt = m_currentHeaderDataDialogs.find(mainId);
        if (foundIt != m_currentHeaderDataDialogs.end())
            entry.resourceIDs.insert(L"Dialog " + foundIt->second + L": Control id " + NumToStr(id) + infoText);
        else
            entry.resourceIDs.insert(NumToStr(id) + infoText);
    }
    else if (lpType == RT_STRING)
    {
        auto foundIt = m_currentHeaderDataStrings.find(id);
        if (foundIt != m_currentHeaderDataStrings.end())
            entry.resourceIDs.insert(foundIt->second + infoText);
        else
            entry.resourceIDs.insert(NumToStr(id) + infoText);
    }
    else if (lpType == RT_MENU)
    {
        auto foundIt = m_currentHeaderDataMenus.find(id);
        if (foundIt != m_currentHeaderDataMenus.end())
            entry.resourceIDs.insert(foundIt->second + infoText);
        else
            entry.resourceIDs.insert(NumToStr(id) + infoText);
    }
    else if (lpType == RT_RIBBON && infoText && wcsstr(infoText, L"ID") == infoText)
        entry.resourceIDs.insert(infoText);
    else
        entry.resourceIDs.insert(NumToStr(id) + infoText);
}

bool CResModule::AdjustCheckSum(const std::wstring& resFile)
{
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    HANDLE hFileMapping = nullptr;
    PVOID  pBaseAddress = nullptr;

    try
    {
        hFile = CreateFile(resFile.c_str(), FILE_READ_DATA | FILE_WRITE_DATA, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            throw GetLastError();

        hFileMapping = CreateFileMapping(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (!hFileMapping)
            throw GetLastError();

        pBaseAddress = MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!pBaseAddress)
            throw GetLastError();

        auto fileSize = GetFileSize(hFile, nullptr);
        if (fileSize == INVALID_FILE_SIZE)
            throw GetLastError();
        DWORD dwChecksum  = 0;
        DWORD dwHeaderSum = 0;
        CheckSumMappedFile(pBaseAddress, fileSize, &dwHeaderSum, &dwChecksum);

        PIMAGE_DOS_HEADER pDOSHeader = static_cast<PIMAGE_DOS_HEADER>(pBaseAddress);
        if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
            throw GetLastError();

        PIMAGE_NT_HEADERS pNTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<PBYTE>(pBaseAddress) + pDOSHeader->e_lfanew);
        if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
            throw GetLastError();

        SetLastError(ERROR_SUCCESS);

        DWORD* pChecksum = &(pNTHeader->OptionalHeader.CheckSum);
        *pChecksum       = dwChecksum;

        UnmapViewOfFile(pBaseAddress);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
    }
    catch (...)
    {
        if (pBaseAddress)
            UnmapViewOfFile(pBaseAddress);
        if (hFileMapping)
            CloseHandle(hFileMapping);
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        return false;
    }

    return true;
}
