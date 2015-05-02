// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014-2015 - TortoiseSVN

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
#include "PathUtils.h"
#include "AppUtils.h"
#include "PasswordDlg.h"
#include "SelectFileFilter.h"
#include "TempFile.h"
#include "ProjectProperties.h"
#include "SVNAuthData.h"

#define TSVN_SYNC_VERSION       1
#define TSVN_SYNC_VERSION_STR   L"1"

// registry entries
std::vector<CString> regUseArray = {
    L"TortoiseMerge\\*",
    L"TortoiseMerge\\History\\Find\\*",
    L"TortoiseOverlays\\*",
    L"TortoiseSVN\\*",
    L"TortoiseSVN\\Colors\\*",
    L"TortoiseSVN\\History\\**",
    L"TortoiseSVN\\History\\repoPaths\\**",
    L"TortoiseSVN\\History\\repoURLS\\**",
    L"TortoiseSVN\\LogCache\\*",
    L"TortoiseSVN\\Merge\\*",
    L"TortoiseSVN\\RevisionGraph\\*",
    L"TortoiseSVN\\Servers\\global\\**",
    L"TortoiseSVN\\StatusColumns\\*",
};

std::vector<CString> regUseLocalArray = {
    L"TortoiseSVN\\DiffTools\\*",
    L"TortoiseSVN\\MergeTools\\*",
    L"TortoiseSVN\\StatusColumns\\*",
    L"Tigris.org\\Subversion\\Config\\**",
    L"Tigris.org\\Subversion\\Servers\\**",
};

std::vector<CString> regBlockArray = {
    L"checknewerweek",
    L"configdir",
    L"currentversion",
    L"*debug*",
    L"defaultcheckoutpath",
    L"diff",
    L"erroroccurred",
    L"historyhintshown",
    L"hooks",
    L"lastcheckoutpath",
    L"merge",
    L"mergewcurl",
    L"monitorfirststart",
    L"newversion",
    L"newversionlink",
    L"newversiontext",
    L"nocontextpaths",
    L"scintilladirect2d",
    L"synccounter",
    L"synclast",
    L"syncpath",
    L"syncpw",
    L"tblamepos",
    L"tblamesize",
    L"tblamestate",
    L"udiffpagesetup*",
    L"*windowpos",
};

std::vector<CString> regBlockLocalArray = {
    L"configdir",
    L"hooks",
    L"scintilladirect2d",
};

bool SyncCommand::Execute()
{
    bool bRet = false;
    CRegString rSyncPath(L"Software\\TortoiseSVN\\SyncPath");
    CTSVNPath syncPath = CTSVNPath(CString(rSyncPath));
    CTSVNPath syncFolder = syncPath;
    CRegDWORD regCount(L"Software\\TortoiseSVN\\SyncCounter");
    CRegDWORD regSyncAuth(L"Software\\TortoiseSVN\\SyncAuth");
    bool bSyncAuth = DWORD(regSyncAuth) != 0;
    if (!cmdLinePath.IsEmpty())
        syncPath = cmdLinePath;
    if (syncPath.IsEmpty() && !parser.HasKey(L"askforpath"))
    {
        return false;
    }
    syncPath.AppendPathString(L"tsvnsync.tsex");

    BOOL bWithLocals = FALSE;
    if (parser.HasKey(L"askforpath"))
    {
        // ask for the path first, then for the password
        // this is used for a manual import/export
        CString path;
        bool bGotPath = FileOpenSave(path, bWithLocals, !!parser.HasKey(L"load"), GetExplorerHWND());
        if (bGotPath)
        {
            syncPath = CTSVNPath(path);
            if (!parser.HasKey(L"load") && syncPath.GetFileExtension().IsEmpty())
                syncPath.AppendRawString(L".tsex");
        }
        else
            return false;
    }


    CSimpleIni iniFile;
    iniFile.SetMultiLine(true);
    SVNAuthData authData;

    CAutoRegKey hMainKey;
    RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\TortoiseSVN", 0, KEY_READ, hMainKey.GetPointer());
    FILETIME filetime = { 0 };
    RegQueryInfoKey(hMainKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &filetime);

    bool bCloudIsNewer = false;
    if (!parser.HasKey(L"save"))
    {
        // open the file in read mode
        CAutoFile hFile = CreateFile(syncPath.GetWinPathString(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile.IsValid())
        {
            // load the file
            LARGE_INTEGER fsize = { 0 };
            if (GetFileSizeEx(hFile, &fsize))
            {
                auto filebuf = std::make_unique<char[]>(fsize.QuadPart);
                DWORD bytesread = 0;
                if (ReadFile(hFile, filebuf.get(), DWORD(fsize.QuadPart), &bytesread, NULL))
                {
                    // decrypt the file contents
                    std::string encrypted;
                    if (bytesread > 0)
                        encrypted = std::string(filebuf.get(), bytesread);
                    CRegString regPW(L"Software\\TortoiseSVN\\SyncPW");
                    CString password;
                    if (parser.HasKey(L"askforpath") && parser.HasKey(L"load"))
                    {
                        INT_PTR dlgret = 0;
                        bool bPasswordMatches = true;
                        do
                        {
                            bPasswordMatches = true;
                            CPasswordDlg passDlg(CWnd::FromHandle(GetExplorerHWND()));
                            passDlg.m_bForSave = !!parser.HasKey(L"save");
                            dlgret = passDlg.DoModal();
                            password = passDlg.m_sPW1;
                            if ((dlgret == IDOK) && (parser.HasKey(L"load")))
                            {
                                std::string passworda = CUnicodeUtils::StdGetUTF8((LPCWSTR)password);
                                std::string decrypted = CStringUtils::Decrypt(encrypted, passworda);
                                if ((decrypted.size() < 3) || (decrypted.substr(0, 3) != "***"))
                                {
                                    bPasswordMatches = false;
                                }
                            }
                        } while ((dlgret == IDOK) && !bPasswordMatches);
                        if (dlgret != IDOK)
                            return false;
                    }
                    else
                    {
                        auto passwordbuf = CStringUtils::Decrypt(CString(regPW));
                        if (passwordbuf.get())
                        {
                            password = passwordbuf.get();
                        }
                        else
                        {
                            // password does not match or it couldn't be read from
                            // the registry!
                            // 
                            TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_SYNC_WRONGPASSWORD), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
                            CString sCmd = L" /command:settings /page:21";
                            CAppUtils::RunTortoiseProc(sCmd);
                            return false;
                        }
                    }
                    std::string passworda = CUnicodeUtils::StdGetUTF8((LPCWSTR)password);
                    std::string decrypted = CStringUtils::Decrypt(encrypted, passworda);
                    if (decrypted.size() >= 3)
                    {
                        if (decrypted.substr(0, 3) == "***")
                        {
                            decrypted = decrypted.substr(3);
                            // pass the decrypted data to the ini file
                            iniFile.LoadFile(decrypted.c_str(), decrypted.size());
                            int inicount = _wtoi(iniFile.GetValue(L"sync", L"synccounter", L""));
                            if (inicount != 0)
                            {
                                if (int(DWORD(regCount)) < inicount)
                                {
                                    bCloudIsNewer = true;
                                    regCount = inicount;
                                }
                            }

                            // load the auth data, but do not overwrite already stored auth data!
                            if (bSyncAuth)
                                authData.ImportAuthData(syncFolder.GetWinPathString(), password);
                        }
                        else
                        {
                            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error decrypting, password may be wrong\n");
                            return false;
                        }
                    }
                }
            }
        }
        else
        {
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error opening file %s, Error %u\n", syncPath.GetWinPath(), GetLastError());
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
                return false;
        }
    }

    if (parser.HasKey(L"load"))
        bCloudIsNewer = true;
    if (parser.HasKey(L"save"))
        bCloudIsNewer = false;

    bool bHaveChanges = false;

    if (bWithLocals || parser.HasKey(L"local"))
    {
        // remove all blocks that are allowed for local exports
        for (const auto& allow : regBlockLocalArray)
        {
            regBlockArray.erase(std::remove(regBlockArray.begin(), regBlockArray.end(), allow), regBlockArray.end());
        }
    }
    // go through all registry values and update either the registry
    // or the ini file, depending on which is newer
    for (const auto& regname : regUseArray)
    {
        bool bChanges = HandleRegistryKey(regname, iniFile, bCloudIsNewer);
        bHaveChanges = bHaveChanges || bChanges;
    }
    if (bWithLocals || parser.HasKey(L"local"))
    {
        for (const auto& regname : regUseLocalArray)
        {
            bool bChanges = HandleRegistryKey(regname, iniFile, bCloudIsNewer);
            bHaveChanges = bHaveChanges || bChanges;
        }
    }

    if (bCloudIsNewer)
    {
        CString regpath = L"Software\\";

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
    {
        // sync project monitor settings
        CString sDataFilePath = CPathUtils::GetAppDataDirectory();
        sDataFilePath += L"\\MonitoringData.ini";
        CSimpleIni monitorIni;
        monitorIni.SetMultiLine(true);
        if (bCloudIsNewer)
        {
            CSimpleIni origMonitorIni;
            origMonitorIni.SetMultiLine(true);
            origMonitorIni.LoadFile(sDataFilePath);

            CSimpleIni::TNamesDepend keys;
            iniFile.GetAllKeys(L"ini_monitor", keys);
            for (const auto& k : keys)
            {
                CString sKey = k;
                CString sSection = sKey.Left(sKey.Find('.'));
                sKey = sKey.Mid(sKey.Find('.') + 1);
                if (sKey.CompareNoCase(L"name") == 0)
                {
                    // make sure the non-synced values are still used
                    monitorIni.SetValue(sSection, L"lastchecked", origMonitorIni.GetValue(sSection, L"lastchecked", L"0"));
                    monitorIni.SetValue(sSection, L"lastcheckedrobots", origMonitorIni.GetValue(sSection, L"lastcheckedrobots", L"0"));
                    monitorIni.SetValue(sSection, L"lastHEAD", origMonitorIni.GetValue(sSection, L"lastHEAD", L"0"));
                    monitorIni.SetValue(sSection, L"UnreadItems", origMonitorIni.GetValue(sSection, L"UnreadItems", L"0"));
                    monitorIni.SetValue(sSection, L"unreadFirst", origMonitorIni.GetValue(sSection, L"unreadFirst", L"0"));
                    monitorIni.SetValue(sSection, L"WCPathOrUrl", origMonitorIni.GetValue(sSection, L"WCPathOrUrl", L""));
                }
                CString sValue = CString(iniFile.GetValue(L"ini_monitor", k, L""));
                if ((sKey.Compare(L"username") == 0) || (sKey.Compare(L"password") == 0))
                {
                    sValue = CStringUtils::Encrypt(sValue);
                }
                monitorIni.SetValue(sSection, sKey, sValue);
            }
            FILE * pFile = NULL;
            errno_t err = 0;
            int retrycount = 5;
            CString sTempfile = CTempFiles::Instance().GetTempFilePathString();
            do
            {
                err = _tfopen_s(&pFile, sTempfile, L"wb");
                if ((err == 0) && pFile)
                {
                    monitorIni.SaveFile(pFile);
                    err = fclose(pFile);
                }
                if (err)
                {
                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error saving %s, retrycount %d\n", (LPCWSTR)sTempfile, retrycount);
                    Sleep(500);
                }
            } while (err && retrycount--);
            if (err == 0)
            {
                if (!CopyFile(sTempfile, sDataFilePath, FALSE))
                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error copying %s to %s, Error %u\n", (LPCWSTR)sTempfile, (LPCWSTR)sDataFilePath, GetLastError());
                else
                {
                    // now send a message to a possible running monitor to force it
                    // to reload the ini file. Otherwise it would overwrite the ini
                    // file without using the synced data!
                    HWND hWnd = FindWindow(NULL, CString(MAKEINTRESOURCE(IDS_MONITOR_DLGTITLE)));
                    if (hWnd)
                    {
                        UINT TSVN_COMMITMONITOR_RELOADINI = RegisterWindowMessage(L"TSVNCommitMonitor_ReloadIni");
                        PostMessage(hWnd, TSVN_COMMITMONITOR_RELOADINI, 0, 0);
                    }
                }
            }
        }
        else
        {
            CSimpleIni::TNamesDepend mitems;
            if (PathFileExists(sDataFilePath))
            {
                int retrycount = 5;
                SI_Error err = SI_OK;
                do
                {
                    err = monitorIni.LoadFile(sDataFilePath);
                    if (err == SI_FILE)
                    {
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error loading %s, retrycount %d\n", (LPCWSTR)sDataFilePath, retrycount);
                        Sleep(500);
                    }
                } while ((err == SI_FILE) && retrycount--);

                if (err == SI_FILE)
                {
                    return false;
                }
                monitorIni.GetAllSections(mitems);
            }

            for (const auto& mitem : mitems)
            {
                CString sSection = mitem;
                CString Name = monitorIni.GetValue(mitem, L"Name", L"");
                if (!Name.IsEmpty())
                {
                    CString newval = monitorIni.GetValue(mitem, L"WCPathOrUrl", L"");
                    iniFile.SetValue(L"ini_monitor", sSection + L".Name", Name);
                    CString oldval = iniFile.GetValue(L"ini_monitor", sSection + L".WCPathOrUrl", L"");
                    bHaveChanges |= ((newval != oldval) && (!oldval.IsEmpty()));
                    // only save monitored working copies if local settings are included, or
                    // if the monitored path is an url.
                    // Don't save paths to working copies for non-local stores
                    if (bWithLocals || newval.IsEmpty() || !PathIsDirectory(newval))
                        iniFile.SetValue(L"ini_monitor", sSection + L".WCPathOrUrl", newval);

                    newval = monitorIni.GetValue(mitem, L"interval", L"5");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".interval", L"0");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".interval", newval);

                    newval = monitorIni.GetValue(mitem, L"minminutesinterval", L"0");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".minminutesinterval", L"0");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".minminutesinterval", newval);

                    newval = CStringUtils::Decrypt(monitorIni.GetValue(mitem, L"username", L"")).get();
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".username", L"");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".username", newval);

                    newval = CStringUtils::Decrypt(monitorIni.GetValue(mitem, L"password", L"")).get();
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".password", L"");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".password", newval);

                    newval = monitorIni.GetValue(mitem, L"MsgRegex", L"");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".MsgRegex", L"");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".MsgRegex", newval);

                    newval = monitorIni.GetValue(mitem, L"ignoreauthors", L"");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".ignoreauthors", L"");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".ignoreauthors", newval);

                    newval = monitorIni.GetValue(mitem, L"parentTreePath", L"");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".parentTreePath", L"");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".parentTreePath", newval);

                    newval = monitorIni.GetValue(mitem, L"uuid", L"");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".uuid", L"");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".uuid", newval);

                    newval = monitorIni.GetValue(mitem, L"root", L"");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".root", L"");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".root", newval);

                    ProjectProperties ProjProps;
                    ProjProps.LoadFromIni(monitorIni, sSection);
                    ProjProps.SaveToIni(iniFile, L"ini_monitor", sSection + L".pp_");
                }
                else if (sSection.CompareNoCase(L"global") == 0)
                {
                    CString newval = monitorIni.GetValue(mitem, L"PlaySound", L"1");
                    CString oldval = iniFile.GetValue(L"ini_monitor", sSection + L".PlaySound", L"1");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".PlaySound", newval);

                    newval = monitorIni.GetValue(mitem, L"ShowNotifications", L"1");
                    oldval = iniFile.GetValue(L"ini_monitor", sSection + L".ShowNotifications", L"1");
                    bHaveChanges |= newval != oldval;
                    iniFile.SetValue(L"ini_monitor", sSection + L".ShowNotifications", newval);
                }
            }
        }
    }

    {
        // sync TortoiseMerge regex filters
        CSimpleIni regexIni;
        regexIni.SetMultiLine(true);
        CString sDataFilePath = CPathUtils::GetAppDataDirectory();
        sDataFilePath += L"\\regexfilters.ini";

        if (bCloudIsNewer)
        {
            CSimpleIni origRegexIni;

            if (PathFileExists(sDataFilePath))
            {
                int retrycount = 5;
                SI_Error err = SI_OK;
                do
                {
                    err = origRegexIni.LoadFile(sDataFilePath);
                    if (err == SI_FILE)
                    {
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error loading %s, retrycount %d\n", (LPCWSTR)sDataFilePath, retrycount);
                        Sleep(500);
                    }
                } while ((err == SI_FILE) && retrycount--);

                if (err == SI_FILE)
                {
                    return false;
                }
            }

            CSimpleIni::TNamesDepend keys;
            iniFile.GetAllKeys(L"ini_tmergeregex", keys);
            for (const auto& k : keys)
            {
                CString sKey = k;
                CString sSection = sKey.Left(sKey.Find('.'));
                sKey = sKey.Mid(sKey.Find('.') + 1);
                CString sValue = CString(iniFile.GetValue(L"ini_tmergeregex", k, L""));
                regexIni.SetValue(sSection, sKey, sValue);
            }
            FILE * pFile = NULL;
            errno_t err = 0;
            int retrycount = 5;
            CString sTempfile = CTempFiles::Instance().GetTempFilePathString();
            do
            {
                err = _tfopen_s(&pFile, sTempfile, L"wb");
                if ((err == 0) && pFile)
                {
                    regexIni.SaveFile(pFile);
                    err = fclose(pFile);
                }
                if (err)
                {
                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error saving %s, retrycount %d\n", (LPCWSTR)sTempfile, retrycount);
                    Sleep(500);
                }
            } while (err && retrycount--);
            if (err == 0)
            {
                if (!CopyFile(sTempfile, sDataFilePath, FALSE))
                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error copying %s to %s, Error: %u\n", (LPCWSTR)sTempfile, (LPCWSTR)sDataFilePath, GetLastError());
            }
        }
        else
        {
            if (PathFileExists(sDataFilePath))
            {
                int retrycount = 5;
                SI_Error err = SI_OK;
                do
                {
                    err = regexIni.LoadFile(sDataFilePath);
                    if (err == SI_FILE)
                    {
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error loading %s, retrycount %d\n", (LPCWSTR)sDataFilePath, retrycount);
                        Sleep(500);
                    }
                } while ((err == SI_FILE) && retrycount--);

                if (err == SI_FILE)
                {
                    return false;
                }
            }

            CSimpleIni::TNamesDepend mitems;
            regexIni.GetAllSections(mitems);
            for (const auto& mitem : mitems)
            {
                CString sSection = mitem;

                CString newval = regexIni.GetValue(mitem, L"regex", L"");
                CString oldval = iniFile.GetValue(L"ini_tmergeregex", sSection + L".regex", L"");
                bHaveChanges |= newval != oldval;
                iniFile.SetValue(L"ini_tmergeregex", sSection + L".regex", newval);

                newval = regexIni.GetValue(mitem, L"replace", L"5");
                oldval = iniFile.GetValue(L"ini_tmergeregex", sSection + L".replace", L"0");
                bHaveChanges |= newval != oldval;
                iniFile.SetValue(L"ini_tmergeregex", sSection + L".replace", newval);
            }
        }
    }


    if (bHaveChanges)
    {
        iniFile.SetValue(L"sync", L"version", TSVN_SYNC_VERSION_STR);
        DWORD count = regCount;
        ++count;
        regCount = count;
        CString tmp;
        tmp.Format(L"%lu", count);
        iniFile.SetValue(L"sync", L"synccounter", tmp);

        // save the ini file
        std::string iniData;
        iniFile.SaveString(iniData);
        iniData = "***" + iniData;
        // encrypt the string

        CString password;
        if (parser.HasKey(L"askforpath"))
        {
            CPasswordDlg passDlg(CWnd::FromHandle(GetExplorerHWND()));
            passDlg.m_bForSave = true;
            if (passDlg.DoModal() != IDOK)
                return false;
            password = passDlg.m_sPW1;
        }
        else
        {
            CRegString regPW(L"Software\\TortoiseSVN\\SyncPW");
            auto passwordbuf = CStringUtils::Decrypt(CString(regPW));
            if (passwordbuf.get())
            {
                password = passwordbuf.get();
            }
        }

        std::string passworda = CUnicodeUtils::StdGetUTF8((LPCWSTR)password);

        std::string encrypted = CStringUtils::Encrypt(iniData, passworda);
        CPathUtils::MakeSureDirectoryPathExists(syncPath.GetContainingDirectory().GetWinPathString());
        CString sTempfile = CTempFiles::Instance().GetTempFilePathString();
        CAutoFile hFile = CreateFile(sTempfile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile.IsValid())
        {
            DWORD written = 0;
            if (WriteFile(hFile, encrypted.c_str(), DWORD(encrypted.size()), &written, NULL))
            {
                if (hFile.CloseHandle())
                {
                    if (!CopyFile(sTempfile, syncPath.GetWinPath(), FALSE))
                    {
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error copying %s to %s, Error: %u\n", (LPCWSTR)sTempfile, syncPath.GetWinPath(), GetLastError());
                    }
                    else
                        bRet = true;
                }
                else
                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error closing file %s, Error: %u\n", (LPCWSTR)sTempfile, GetLastError());
            }
            else
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error writing to file %s, Error: %u\n", (LPCWSTR)sTempfile, GetLastError());
        }
        else
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error creating file %s for writing, Error: %u\n", (LPCWSTR)sTempfile, GetLastError());

        if (bSyncAuth)
        {
            // now save all auth data
            CPathUtils::MakeSureDirectoryPathExists(syncFolder.GetWinPathString() + L"\\auth");
            CPathUtils::MakeSureDirectoryPathExists(syncFolder.GetWinPathString() + L"\\auth\\svn.simple");
            CPathUtils::MakeSureDirectoryPathExists(syncFolder.GetWinPathString() + L"\\auth\\svn.ssl.client-passphrase");
            CPathUtils::MakeSureDirectoryPathExists(syncFolder.GetWinPathString() + L"\\auth\\svn.ssl.server");
            CPathUtils::MakeSureDirectoryPathExists(syncFolder.GetWinPathString() + L"\\auth\\svn.username");
            authData.ExportAuthData(syncFolder.GetWinPathString(), password);
        }
    }


    return bRet;
}

bool SyncCommand::HandleRegistryKey(const CString& regname, CSimpleIni& iniFile, bool bCloudIsNewer)
{
    CAutoRegKey hKey;
    CAutoRegKey hKeyKey;
    DWORD regtype = 0;
    DWORD regsize = 0;
    CString sKeyPath = L"Software";
    CString sValuePath = regname;
    CString sIniKeyName = regname;
    CString sRegname = regname;
    CString sValue;
    bool bHaveChanges = false;
    if (regname.Find('\\') >= 0)
    {
        // handle values in sub-keys
        sKeyPath = L"Software\\" + regname.Left(regname.ReverseFind('\\'));
        sValuePath = regname.Mid(regname.ReverseFind('\\') + 1);
    }
    if (RegOpenKeyEx(HKEY_CURRENT_USER, sKeyPath, 0, KEY_READ, hKey.GetPointer()) == ERROR_SUCCESS)
    {
        bool bEnum = false;
        bool bEnumKeys = false;
        int index = 0;
        int keyindex = 0;
        // an asterisk means: use all values inside the specified key
        if (sValuePath == L"*")
            bEnum = true;
        if (sValuePath == L"**")
        {
            bEnumKeys = true;
            bEnum = true;
            RegOpenKeyEx(HKEY_CURRENT_USER, sKeyPath, 0, KEY_READ, hKeyKey.GetPointer());
        }
        do
        {
            if (bEnumKeys)
            {
                bEnum = true;
                index = 0;
                wchar_t cKeyName[MAX_PATH] = { 0 };
                DWORD cLen = _countof(cKeyName);
                if (RegEnumKeyEx(hKeyKey, keyindex, cKeyName, &cLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                {
                    bEnumKeys = false;
                    break;
                }
                ++keyindex;
                sKeyPath = L"Software\\" + regname.Left(regname.ReverseFind('\\')) + L"\\" + cKeyName + L"\\";
                sRegname = regname.Left(regname.ReverseFind('\\')) + L"\\" + cKeyName + L"\\";
                hKey.CloseHandle();
                if (RegOpenKeyEx(HKEY_CURRENT_USER, sKeyPath, 0, KEY_READ, hKey.GetPointer()) != ERROR_SUCCESS)
                {
                    bEnumKeys = false;
                    break;
                }
            }
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
                    CString sValueLower = sValuePath;
                    sValueLower.MakeLower();
                    bool bIgnore = false;
                    for (const auto& ignore : regBlockArray)
                    {
                        if (wcswildcmp(ignore, sValueLower))
                        {
                            bIgnore = true;
                            break;
                        }
                    }
                    if (bIgnore)
                        continue;
                    sIniKeyName = sRegname.Left(sRegname.ReverseFind('\\'));
                    if (sIniKeyName.IsEmpty())
                        sIniKeyName = sValuePath;
                    else
                        sIniKeyName += L"\\" + sValuePath;
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
                                            sValue.Format(L"%lu", value);
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
        } while (bEnumKeys);
    }
    return bHaveChanges;
}

bool SyncCommand::FileOpenSave(CString& path, BOOL& bWithLocals, bool bOpen, HWND hwndOwner)
{
    HRESULT hr;
    bWithLocals = FALSE;
    // Create a new common save file dialog
    CComPtr<IFileDialog> pfd = NULL;

    hr = pfd.CoCreateInstance(bOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER);
    if (SUCCEEDED(hr))
    {
        // Set the dialog options
        DWORD dwOptions;
        if (SUCCEEDED(hr = pfd->GetOptions(&dwOptions)))
        {
            if (bOpen)
                hr = pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
            else
            {
                hr = pfd->SetOptions(dwOptions | FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
                hr = pfd->SetFileName(CPathUtils::GetFileNameFromPath(path));
            }
        }

        // Set a title
        if (SUCCEEDED(hr))
        {
            CString temp;
            temp.LoadString(IDS_SYNC_SETTINGSFILE);
            CStringUtils::RemoveAccelerators(temp);
            pfd->SetTitle(temp);
        }

        CSelectFileFilter fileFilter(IDS_EXPORTFILTER);
        hr = pfd->SetFileTypes(fileFilter.GetCount(), fileFilter);

        if (!bOpen)
        {
            CComPtr<IFileDialogCustomize> pfdCustomize;
            hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdCustomize));
            if (SUCCEEDED(hr))
            {
                pfdCustomize->AddCheckButton(101, CString(MAKEINTRESOURCE(IDS_SYNC_INCLUDELOCAL)), FALSE);
            }
        }

        // Show the save/open file dialog
        if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(hwndOwner)))
        {
            // Get the selection from the user
            CComPtr<IShellItem> psiResult = NULL;
            hr = pfd->GetResult(&psiResult);
            if (SUCCEEDED(hr))
            {
                PWSTR pszPath = NULL;
                hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                if (SUCCEEDED(hr))
                {
                    path = CString(pszPath);
                    if (!bOpen)
                    {
                        CComPtr<IFileDialogCustomize> pfdCustomize;
                        hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdCustomize));
                        if (SUCCEEDED(hr))
                        {
                            pfdCustomize->GetCheckButtonState(101, &bWithLocals);
                        }
                    }
                    return true;
                }
            }
        }
    }
    return false;
}
