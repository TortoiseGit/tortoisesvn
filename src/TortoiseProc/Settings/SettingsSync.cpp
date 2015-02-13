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
#include "TortoiseProc.h"
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
    , m_sPW1(_T(""))
    , m_sPW2(_T(""))
    , m_sSyncPath(_T(""))
    , m_regSyncPW(L"Software\\TortoiseSVN\\SyncPW")
    , m_regSyncPath(L"Software\\TortoiseSVN\\SyncPath")
    , m_bSyncAuth(FALSE)
    , m_regSyncAuth(L"Software\\TortoiseSVN\\SyncAuth")
{
}

CSettingsSync::~CSettingsSync()
{
}

void CSettingsSync::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_SYNCPW1, m_sPW1);
    DDX_Text(pDX, IDC_SYNCPW2, m_sPW2);
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
    auto pw = CStringUtils::Decrypt((LPCWSTR)CString(m_regSyncPW));
    m_sPW1 = pw.get();
    m_sPW2 = m_sPW1;

    m_bSyncAuth = m_regSyncAuth;

    UpdateData(FALSE);

    OnEnChange();

    return TRUE;
}

BOOL CSettingsSync::OnApply()
{
    if (!ValidateInput())
        return FALSE;

    auto pw = CStringUtils::Encrypt(m_sPW1);
    Store(pw, m_regSyncPW);
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
    CString strDir = m_sSyncPath;
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

    if (m_sPW1 != m_sPW2)
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
            CAutoFile hFile = CreateFile(m_sSyncPath + L"\\tsvnsync.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
                        std::string encrypted = std::string(filebuf.get(), bytesread);
                        std::string passworda = CUnicodeUtils::GetUTF8(m_sPW1);
                        std::string decrypted = CStringUtils::Decrypt(encrypted, passworda);
                        if (decrypted.size() >= 3)
                        {
                            if (decrypted.substr(0, 3) == "***")
                            {
                                decrypted = decrypted.substr(3);
                                // pass the decrypted data to the ini file
                                iniFile.LoadFile(decrypted.c_str(), decrypted.size());
                                int inicount = _wtoi(iniFile.GetValue(L"sync", L"synccounter", L""));
                                CRegDWORD regCount(L"Software\\TortoiseSVN\\SyncCounter");
                                if (inicount != 0)
                                {
                                    if (int(DWORD(regCount)) < inicount)
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
                                CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK1)),
                                                    CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK2)),
                                                    L"TortoiseSVN",
                                                    0,
                                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
                                taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK3)));
                                taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_WARN_SYNCPWWRONG_TASK4)));
                                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                                taskdlg.SetDefaultCommandControl(2);
                                taskdlg.SetMainIcon(TD_WARNING_ICON);
                                bool doIt = (taskdlg.DoModal(GetSafeHwnd()) == 1);
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


void CSettingsSync::OnBnClickedLoadfile()
{
    CString sCmd = L" /command:sync /load /askforpath" + GetHWndParam();
    CAppUtils::RunTortoiseProc(sCmd);
}


void CSettingsSync::OnBnClickedSavefile()
{
    CString sCmd = L" /command:sync /save /askforpath" + GetHWndParam();
    CAppUtils::RunTortoiseProc(sCmd);
}

CString CSettingsSync::GetHWndParam() const
{
    CString sCmd;
    sCmd.Format(L" /hwnd:%p", (void*)GetSafeHwnd());
    return sCmd;
}


void CSettingsSync::OnBnClickedSyncauth()
{
    SetModified(TRUE);
}
