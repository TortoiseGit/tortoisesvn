// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014 - TortoiseSVN

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
#include "SyncCommand.h"
#include "registry.h"
#include "TSVNPath.h"
#include "SimpleIni.h"
#include "SmartHandle.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"


// registry entries
std::vector<CString> regArray = {
    L"AllowAuthSave",
    L"AllowUnversionedObstruction",
    L"AlwaysExtendedMenu",
    L"AutoCompleteMinChars",
    L"AutocompleteRemovesExtensions",
    L"BlockPeggedExternals",
    L"BlockStatus",
    L"CacheTrayIcon",
    L"ColumnsEveryWhere",
    L"ConfigDir",
    L"CtrlEnter",
    L"DialogTitles",
    L"DiffBlamesWithTortoiseMerge",
    L"DlgStickySize",
    L"FixCaseRenames",
    L"FullRowSelect",
    L"GroupTaskbarIconsPerRepo",
    L"GroupTaskbarIconsPerRepoOverlay",
    L"HideExternalInfo",
    L"IncludeExternals",
    L"LogFindCopyFrom",
    L"LogMultiRevFormat",
    L"LogStatusCheck",
    L"MergeLogSeparator",
    L"NumDiffWarning",
    L"OldVersionCheck",
    L"OutOfDateRetry",
    L"RepoBrowserTrySVNParentPath",
    L"ScintillaDirect2D",
    L"ShellMenuAccelerators",
    L"ShowContextMenuIcons",
    L"ShowAppContextMenuIcons",
    L"StyleCommitMessages",
    L"UpdateCheckURL",
    L"VersionCheck",

    L"EnableDragContextMenu",
    L"ShowContextMenuIcons",
    L"CacheType",
    L"RecursiveOverlay",
    L"FolderOverlay",
    L"DriveMaskRemote",
    L"DriveMaskFixed",
    L"DriveMaskCDROM",
    L"DriveMaskRemovable",
    L"DriveMaskFloppy",
    L"DriveMaskRAM",
    L"DriveMaskUnknown",
    L"ShellMenuAccelerators",
    L"UnversionedAsModified",
    L"IgnoreOnCommitIgnored",
    L"HideMenusForUnversionedItems",
    L"GetLockTop",
    L"ShowExcludedFoldersAsNormal",
    L"AlwaysExtendedMenu",
    L"ContextMenuEntries",
    L"ContextMenuEntrieshigh",
    L"ContextMenuEntriesMaskLow",
    L"ContextMenuEntriesMaskHigh",
    L"ContextMenuEntriesMaskLow",
    L"ContextMenuEntriesMaskHigh",
    L"LanguageID",
    L"BlockStatus",
    L"ColumnsEveryWhere",
    L"NoContextPaths",

    L"LogDateFormat",
    L"UseSystemLocaleForDates",
    L"ShowLockDlg",
    L"MaxHistoryItems",
    L"LogFontName",
    L"LogFontSize",
    L"SelectFilesForCommit",
    L"TextBlame",
    L"BlameIncludeMerge",
    L"DefaultCheckoutUrl",
    L"noext",
    L"RevertWithRecycleBin",
    L"CleanupRefreshShell",
    L"CleanupExternals",
    L"CleanupFixTimeStamps",
    L"CleanupVacuum",
    L"IncludeExternals",
    L"OutOfDateRetry",
    L"IncompleteReopen",
    L"UnversionedRecurse",
    L"BlockPeggedExternals",
    L"RevisionGraph\\BranchPattern",
    L"AutocompleteParseTimeout",
    L"AutocompleteRemovesExtensions",
    L"VersionCheck",
    L"AddBeforeCommit",
    L"ShowExternals",
    L"MaxHistoryItems",
    L"KeepChangeLists",
    L"AlwaysWarnIfNoIssue",
    L"CommitReopen",
    L"NumberOfLogs",
    L"LastLogStrict",
    L"UseRegexFilter",
    L"MaxBugIDColWidth",
    L"FilterCaseSensitively",
    L"LogMultiRevFormat",
    L"LogShowPaths",
    L"ShowAllEntry",
    L"StyleCommitMessages",
    L"LogFindCopyFrom",
    L"LogStatusCheck",
    L"LogHidePaths",
    L"DiffByDoubleClickInLog",
    L"StatAuthorsCaseSensitive",
    L"StatSortByCommitCount",
    L"LastViewedStatsPage",
    L"MergeWizardMode",
    L"EnableDWMFrame",

    L"Colors\\*",
    L"History\\*",
    L"History\\repoURLS\\*",
    L"History\\repoPaths\\*",
    L"LogCache\\*",
    L"Servers\\global\\*",
    L"StatusColumns\\*",
};


bool SyncCommand::Execute()
{
    CRegString rSyncPath(L"Software\\TortoiseSVN\\SyncPath");
    CTSVNPath syncPath = CTSVNPath(CString(rSyncPath));

    if (syncPath.IsEmpty())
    {
        // show the settings dialog so the user can enter a sync path
        //return false;
    }

    {
        CRegString regPW(L"Software\\TortoiseSVN\\SyncPW");
        auto encrypted = CStringUtils::Encrypt(L"testpassword");
        regPW = encrypted;
    }

    CSimpleIni iniFile;

    CAutoRegKey hMainKey;
    RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\TortoiseSVN", 0, KEY_READ, hMainKey.GetPointer());
    FILETIME filetime = { 0 };
    RegQueryInfoKey(hMainKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &filetime);


    bool bCloudIsNewer = false;
    {
        // open the file in read mode
        CAutoFile hFile = CreateFile(syncPath.GetWinPathString() + L"\\tsvnsync.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile.IsValid())
        {
            // compare the file times
            FILETIME ftCreate, ftAccess, ftWrite;
            if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
            {
                if (CompareFileTime(&filetime, &ftWrite) < 0)
                    bCloudIsNewer = true;
            }
            // load the file
            LARGE_INTEGER fsize = { 0 };
            if (GetFileSizeEx(hFile, &fsize))
            {
                auto filebuf = std::make_unique<char[]>(fsize.QuadPart);
                DWORD bytesread = 0;
                if (ReadFile(hFile, filebuf.get(), DWORD(fsize.QuadPart), &bytesread, NULL))
                {
                    // decrypt the file contents
                    std::string encrypted = std::string(filebuf.get(), bytesread);
                    CRegString regPW(L"Software\\TortoiseSVN\\SyncPW");
                    auto password = CStringUtils::Decrypt(CString(regPW));
                    std::string passworda = CUnicodeUtils::StdGetUTF8(password.get());
                    std::string decrypted = CStringUtils::Decrypt(encrypted, passworda);
                    // pass the decrypted data to the ini file
                    iniFile.LoadFile(decrypted.c_str(), decrypted.size());
                }
            }
        }
    }

    bool bHaveChanges = false;
    CString sValue;

    // go through all registry values and update either the registry
    // or the ini file, depending on which is newer
    for (const auto& regname : regArray)
    {
        CAutoRegKey hKey;
        DWORD regtype = 0;
        DWORD regsize = 0;
        CString sKeyPath = L"Software\\TortoiseSVN";
        CString sValuePath = regname;
        CString sIniKeyName = regname;
        if (regname.Find('\\') >= 0)
        {
            // handle values in sub-keys
            sKeyPath = L"Software\\TortoiseSVN\\" + regname.Left(regname.ReverseFind('\\'));
            sValuePath = regname.Mid(regname.ReverseFind('\\') + 1);
        }
        if (RegOpenKeyEx(HKEY_CURRENT_USER, sKeyPath, 0, KEY_READ, hKey.GetPointer()) == ERROR_SUCCESS)
        {
            bool bEnum = false;
            int index = 0;
            // an asterisk means: use all values inside the specified key
            if (sValuePath == L"*")
                bEnum = true;
            do 
            {
                if (bEnum)
                {
                    // start enumerating all values
                    wchar_t cValueName[MAX_PATH] = { 0 };
                    DWORD cLen = _countof(cValueName);
                    if (RegEnumValue(hKey, index, cValueName, &cLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                    {
                        bEnum = false;
                        break;
                    }
                    ++index;
                    sValuePath = cValueName;
                    sIniKeyName = regname.Left(regname.ReverseFind('\\')) + L"\\" + sValuePath;
                }
                if (RegQueryValueEx(hKey, sValuePath, NULL, &regtype, NULL, &regsize) == ERROR_SUCCESS)
                {
                    if (regtype != 0)
                    {
                        auto regbuf = std::make_unique<BYTE[]>(regsize);
                        if (RegQueryValueEx(hKey, sValuePath, NULL, &regtype, regbuf.get(), &regsize) == ERROR_SUCCESS)
                        {
                            switch (regtype)
                            {
                                case REG_DWORD:
                                {
                                    DWORD value = *(DWORD*)regbuf.get();
                                    sValue = iniFile.GetValue(L"registry_dword", sIniKeyName);
                                    DWORD nValue = DWORD(_wtol(sValue));
                                    if (nValue != value)
                                    {
                                        if (bCloudIsNewer)
                                        {
                                            RegSetValueEx(hKey, sValuePath, NULL, regtype, (BYTE *)&nValue, sizeof(nValue));
                                        }
                                        else
                                        {
                                            bHaveChanges = true;
                                            sValue.Format(L"%ld", value);
                                            iniFile.SetValue(L"registry_dword", sIniKeyName, sValue);
                                        }
                                    }
                                    if (bCloudIsNewer)
                                        iniFile.Delete(L"registry_dword", sIniKeyName);
                                }
                                    break;
                                case REG_QWORD:
                                {
                                    QWORD value = *(QWORD*)regbuf.get();
                                    sValue = iniFile.GetValue(L"registry_qword", sIniKeyName);
                                    QWORD nValue = QWORD(_wtoi64(sValue));
                                    if (nValue != value)
                                    {
                                        if (bCloudIsNewer)
                                        {
                                            RegSetValueEx(hKey, sValuePath, NULL, regtype, (BYTE *)&nValue, sizeof(nValue));
                                        }
                                        else
                                        {
                                            bHaveChanges = true;
                                            sValue.Format(L"%I64d", value);
                                            iniFile.SetValue(L"registry_qword", sIniKeyName, sValue);
                                        }
                                    }
                                    if (bCloudIsNewer)
                                        iniFile.Delete(L"registry_qword", sIniKeyName);
                                }
                                    break;
                                case REG_EXPAND_SZ:
                                case REG_MULTI_SZ:
                                case REG_SZ:
                                {
                                    sValue = (LPCWSTR)regbuf.get();
                                    CString iniValue = iniFile.GetValue(L"registry_string", sIniKeyName);
                                    if (iniValue != sValue)
                                    {
                                        if (bCloudIsNewer)
                                        {
                                            RegSetValueEx(hKey, sValuePath, NULL, regtype, (BYTE *)(LPCWSTR)iniValue, (iniValue.GetLength() + 1)*sizeof(WCHAR));
                                        }
                                        else
                                        {
                                            bHaveChanges = true;
                                            iniFile.SetValue(L"registry_string", sIniKeyName, sValue);
                                        }
                                    }
                                    if (bCloudIsNewer)
                                        iniFile.Delete(L"registry_string", sIniKeyName);
                                }
                                    break;
                            }
                        }
                    }
                }
            } while (bEnum);
        }
    }

    if (bCloudIsNewer)
    {
        CString regpath = L"Software\\TortoiseSVN\\";

        CSimpleIni::TNamesDepend keys;
        iniFile.GetAllKeys(L"registry_dword", keys);
        for (const auto& k : keys)
        {
            CRegDWORD reg(regpath + k);
            reg = _wtol(iniFile.GetValue(L"registry_dword", k, L""));
        }

        keys.clear();
        iniFile.GetAllKeys(L"registry_qword", keys);
        for (const auto& k : keys)
        {
            CRegQWORD reg(regpath + k);
            reg = _wtoi64(iniFile.GetValue(L"registry_qword", k, L""));
        }

        keys.clear();
        iniFile.GetAllKeys(L"registry_string", keys);
        for (const auto& k : keys)
        {
            CRegString reg(regpath + k);
            reg = CString(iniFile.GetValue(L"registry_string", k, L""));
        }
    }

    if (bHaveChanges)
    {
        // save the ini file
        std::string iniData;
        iniFile.SaveString(iniData);
        // encrypt the string

        CRegString regPW(L"Software\\TortoiseSVN\\SyncPW");
        auto password = CStringUtils::Decrypt(CString(regPW));
        std::string passworda = CUnicodeUtils::StdGetUTF8(password.get());

        std::string encrypted = CStringUtils::Encrypt(iniData, passworda);

        CAutoFile hFile = CreateFile(syncPath.GetWinPathString() + L"\\tsvnsync.ini", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile.IsValid())
        {
            DWORD written = 0;
            WriteFile(hFile, encrypted.c_str(), DWORD(encrypted.size()), &written, NULL);
        }

        return true;
    }
    return false;
}
