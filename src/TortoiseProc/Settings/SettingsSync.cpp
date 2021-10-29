// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014-2015, 2019, 2021 - TortoiseSVN

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
#include "SettingsSync.h"
#include "StringUtils.h"
#include "SimpleIni.h"
#include "UnicodeUtils.h"
#include "SmartHandle.h"
#include "AppUtils.h"
#include "BrowseFolder.h"

// CSettingsSync dialog

IMPLEMENT_DYNAMIC(CSettingsSync, ISettingsPropPage)

CSettingsSync::CSettingsSync()
    : ISettingsPropPage(CSettingsSync::IDD)
    , m_regSyncPath(L"Software\\TortoiseSVN\\SyncPath")
    , m_regSyncPw(L"Software\\TortoiseSVN\\SyncPW")
    , m_regSyncAuth(L"Software\\TortoiseSVN\\SyncAuth")
    , m_bSyncAuth(FALSE)
{
}

CSettingsSync::~CSettingsSync()
{
}

void CSettingsSync::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_SYNCPW1, m_sPw1);
    DDX_Text(pDX, IDC_SYNCPW2, m_sPw2);
    DDX_Text(pDX, IDC_SYNCPATH, m_sSyncPath);
    DDX_Check(pDX, IDC_SYNCAUTH, m_bSyncAuth);
}

BEGIN_MESSAGE_MAP(CSettingsSync, ISettingsPropPage)
    ON_BN_CLICKED(IDC_SYNCBROWSE, &CSettingsSync::OnBnClickedSyncbrowse)
    ON_EN_CHANGE(IDC_SYNCPATH, &CSettingsSync::OnEnChange)
    ON_EN_CHANGE(IDC_SYNCPW1, &CSettingsSync::OnEnChange)
    ON_EN_CHANGE(IDC_SYNCPW2, &CSettingsSync::OnEnChange)
    ON_BN_CLICKED(IDC_LOAD, &CSettingsSync::OnBnClickedLoad)
    ON_BN_CLICKED(IDC_SAVE, &CSettingsSync::OnBnClickedSave)
    ON_BN_CLICKED(IDC_LOADFILE, &CSettingsSync::OnBnClickedLoadfile)
    ON_BN_CLICKED(IDC_SAVEFILE, &CSettingsSync::OnBnClickedSavefile)
    ON_BN_CLICKED(IDC_SYNCAUTH, &CSettingsSync::OnBnClickedSyncauth)
END_MESSAGE_MAP()

// CSettingsSync message handlers

BOOL CSettingsSync::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_sSyncPath = m_regSyncPath;
    auto pw     = CStringUtils::Decrypt(static_cast<LPCWSTR>(CString(m_regSyncPw)));
    m_sPw1      = pw.get();
    m_sPw2      = m_sPw1;

    m_bSyncAuth = m_regSyncAuth;

    UpdateData(FALSE);

    OnEnChange();

    return TRUE;
}

BOOL CSettingsSync::OnApply()
{
    if (!ValidateInput())
        return FALSE;

    // if the path changed, reset the sync counter in the registry
    if (m_sSyncPath.CompareNoCase(static_cast<CString>(m_regSyncPath)))
    {
        CRegDWORD regCount(L"Software\\TortoiseSVN\\SyncCounter");
        regCount = 0;
    }

    auto pw = CStringUtils::Encrypt(m_sPw1);
    Store(pw, m_regSyncPw);
    Store(m_sSyncPath, m_regSyncPath);
    Store(m_bSyncAuth, m_regSyncAuth);

    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSettingsSync::OnBnClickedSyncbrowse()
{
    UpdateData();
    CBrowseFolder browseFolder;
    browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    CString strDir       = m_sSyncPath;
    if (browseFolder.Show(GetSafeHwnd(), strDir) == CBrowseFolder::OK)
    {
        m_sSyncPath = strDir;
        SetModified(TRUE);
        UpdateData(FALSE);
    }
}

void CSettingsSync::OnEnChange()
{
    UpdateData();
    DialogEnableWindow(IDC_SYNCPW1, !m_sSyncPath.IsEmpty());
    DialogEnableWindow(IDC_SYNCPW2, !m_sSyncPath.IsEmpty());
    DialogEnableWindow(IDC_LOAD, !m_sSyncPath.IsEmpty());
    DialogEnableWindow(IDC_SAVE, !m_sSyncPath.IsEmpty());
    SetModified(TRUE);
}

BOOL CSettingsSync::ValidateInput()
{
    UpdateData();

    if (m_sPw1 != m_sPw2)
    {
        // passwords don't match
        ShowEditBalloon(IDC_SYNCPW1, IDS_ERR_SYNCPW_NOMATCH, IDS_ERR_ERROR, TTI_ERROR);
        return FALSE;
    }
    if (PathFileExists(m_sSyncPath))
    {
        // check whether the settings file can be decrypted
        // using the specified password
        CSimpleIni iniFile;
        iniFile.SetMultiLine(true);
        {
            // open the file in read mode
            CAutoFile hFile = CreateFile(m_sSyncPath + L"\\tsvnsync.tsex", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile.IsValid())
            {
                // load the file
                LARGE_INTEGER fSize = {0};
                if (GetFileSizeEx(hFile, &fSize))
                {
                    auto  fileBuf   = std::make_unique<char[]>(static_cast<DWORD>(fSize.QuadPart));
                    DWORD bytesRead = 0;
                    if (ReadFile(hFile, fileBuf.get(), static_cast<DWORD>(fSize.QuadPart), &bytesRead, nullptr))
                    {
                        // decrypt the file contents
                        std::string encrypted = std::string(fileBuf.get(), bytesRead);
                        std::string passwordA = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sPw1));
                        std::string decrypted = CStringUtils::Decrypt(encrypted, passwordA);
                        if (decrypted.size() >= 3)
                        {
                            if (decrypted.substr(0, 3) == "***")
                            {
                                decrypted = decrypted.substr(3);
                                // pass the decrypted data to the ini file
                                iniFile.LoadFile(decrypted.c_str(), decrypted.size());
                                int       iniCount = _wtoi(iniFile.GetValue(L"sync", L"synccounter", L""));
                                CRegDWORD regCount(L"Software\\TortoiseSVN\\SyncCounter");
                                if (iniCount != 0)
                                {
                                    if (static_cast<int>(static_cast<DWORD>(regCount)) < iniCount)
                                    {
                                        CString sCmd = L" /command:sync /load" + GetHWndParam();
                                        CAppUtils::RunTortoiseProc(sCmd);
                                    }
                                }
                            }
                            else
                            {
                                // the file could not be decrypted using the specified password:
                                // either the password is wrong or the file is not a settings export
                                // ask the user whether to overwrite the file
                                CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK1)),
                                                    CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK2)),
                                                    L"TortoiseSVN",
                                                    0,
                                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                                taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK3)));
                                taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK4)));
                                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                                taskDlg.SetDefaultCommandControl(200);
                                taskDlg.SetMainIcon(TD_WARNING_ICON);
                                bool doIt = (taskDlg.DoModal(GetSafeHwnd()) == 100);
                                if (!doIt)
                                    return FALSE;
                            }
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}

BOOL CSettingsSync::OnKillActive()
{
    return ValidateInput();
}

void CSettingsSync::OnBnClickedLoad()
{
    ValidateInput();
    CString sCmd = L" /command:sync /load" + GetHWndParam();
    CAppUtils::RunTortoiseProc(sCmd);
}

void CSettingsSync::OnBnClickedSave()
{
    ValidateInput();
    CString sCmd = L" /command:sync /save" + GetHWndParam();
    CAppUtils::RunTortoiseProc(sCmd);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CSettingsSync::OnBnClickedLoadfile()
{
    CString sCmd = L" /command:sync /load /askforpath" + GetHWndParam();
    CAppUtils::RunTortoiseProc(sCmd);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CSettingsSync::OnBnClickedSavefile()
{
    CString sCmd = L" /command:sync /save /askforpath" + GetHWndParam();
    CAppUtils::RunTortoiseProc(sCmd);
}

CString CSettingsSync::GetHWndParam() const
{
    CString sCmd;
    sCmd.Format(L" /hwnd:%p", static_cast<void*>(GetSafeHwnd()));
    return sCmd;
}

void CSettingsSync::OnBnClickedSyncauth()
{
    SetModified(TRUE);
}
