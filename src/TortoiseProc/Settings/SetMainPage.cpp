// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015 - TortoiseSVN

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
#include "SetMainPage.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "SVNProgressDlg.h"
#include "../version.h"
#include "SetMainPage.h"
#include "SVN.h"
#include "SysInfo.h"
#include "Libraries.h"
#include "SmartHandle.h"

IMPLEMENT_DYNAMIC(CSetMainPage, ISettingsPropPage)
CSetMainPage::CSetMainPage()
    : ISettingsPropPage(CSetMainPage::IDD)
    , m_sTempExtensions(L"")
    , m_bLastCommitTime(FALSE)
    , m_bUseAero(TRUE)
    , m_dwLanguage(0)
{
    m_regLanguage = CRegDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033);
    CString temp(SVN_CONFIG_DEFAULT_GLOBAL_IGNORES);
    m_regExtensions = CRegString(L"Software\\Tigris.org\\Subversion\\Config\\miscellany\\global-ignores", temp);
    m_regLastCommitTime = CRegString(L"Software\\Tigris.org\\Subversion\\Config\\miscellany\\use-commit-times", L"");
    m_regUseAero = CRegDWORD(L"Software\\TortoiseSVN\\EnableDWMFrame", TRUE);
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
    m_dwLanguage = (DWORD)m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel());
    DDX_Text(pDX, IDC_TEMPEXTENSIONS, m_sTempExtensions);
    DDX_Check(pDX, IDC_COMMITFILETIMES, m_bLastCommitTime);
    DDX_Check(pDX, IDC_AERODWM, m_bUseAero);
}


BEGIN_MESSAGE_MAP(CSetMainPage, ISettingsPropPage)
    ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnModified)
    ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnModified)
    ON_BN_CLICKED(IDC_EDITCONFIG, OnBnClickedEditconfig)
    ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnModified)
    ON_BN_CLICKED(IDC_CHECKNEWERBUTTON, OnBnClickedChecknewerbutton)
    ON_BN_CLICKED(IDC_COMMITFILETIMES, OnModified)
    ON_BN_CLICKED(IDC_SOUNDS, OnBnClickedSounds)
    ON_BN_CLICKED(IDC_AERODWM, OnModified)
    ON_BN_CLICKED(IDC_CREATELIB, &CSetMainPage::OnBnClickedCreatelib)
END_MESSAGE_MAP()

BOOL CSetMainPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    EnableToolTips();

    m_sTempExtensions = m_regExtensions;
    m_dwLanguage = m_regLanguage;
    m_bUseAero = m_regUseAero;
    HIGHCONTRAST hc = { sizeof(HIGHCONTRAST) };
    SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, FALSE);
    BOOL bEnabled = FALSE;
    DialogEnableWindow(IDC_AERODWM, ((hc.dwFlags & HCF_HIGHCONTRASTON) == 0) && SUCCEEDED(DwmIsCompositionEnabled(&bEnabled)) && bEnabled);

    CString temp;
    temp = m_regLastCommitTime;
    m_bLastCommitTime = (temp.CompareNoCase(L"yes")==0);

    m_tooltips.AddTool(IDC_TEMPEXTENSIONSLABEL, IDS_SETTINGS_TEMPEXTENSIONS_TT);
    m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
    m_tooltips.AddTool(IDC_COMMITFILETIMES, IDS_SETTINGS_COMMITFILETIMES_TT);
    m_tooltips.AddTool(IDC_CREATELIB, IDS_SETTINGS_CREATELIB_TT);

    DialogEnableWindow(IDC_CREATELIB, SysInfo::Instance().IsWin7OrLater());

    // set up the language selecting combobox
    TCHAR buf[MAX_PATH] = { 0 };
    GetLocaleInfo(1033, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
    m_LanguageCombo.AddString(buf);
    m_LanguageCombo.SetItemData(0, 1033);
    CString path = CPathUtils::GetAppParentDirectory();
    path = path + L"Languages\\";
    CSimpleFileFind finder(path, L"*.dll");
    int langcount = 1;
    while (finder.FindNextFileNoDirectories())
    {
        CString file = finder.GetFilePath();
        CString filename = finder.GetFileName();
        if (filename.Left(12).CompareNoCase(L"TortoiseProc")==0)
        {
            CString sVer = _T(STRPRODUCTVER);
            sVer = sVer.Left(sVer.ReverseFind('.'));
            CString sFileVer = CPathUtils::GetVersionFromFile(file);
            sFileVer = sFileVer.Left(sFileVer.ReverseFind('.'));
            if (sFileVer.Compare(sVer)!=0)
                continue;
            CString sLoc = filename.Mid(12);
            sLoc = sLoc.Left(sLoc.GetLength()-4);   // cut off ".dll"
            if ((sLoc.Left(2) == L"32")&&(sLoc.GetLength() > 5))
                continue;
            DWORD loc = _tstoi(filename.Mid(12));
            GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
            CString sLang = buf;
            GetLocaleInfo(loc, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
            if (buf[0])
            {
                sLang += L" (";
                sLang += buf;
                sLang += L")";
            }
            m_LanguageCombo.AddString(sLang);
            m_LanguageCombo.SetItemData(langcount++, loc);
        }
    }

    for (int i=0; i<m_LanguageCombo.GetCount(); i++)
    {
        if (m_LanguageCombo.GetItemData(i) == m_dwLanguage)
            m_LanguageCombo.SetCurSel(i);
    }

    UpdateData(FALSE);
    return TRUE;
}

void CSetMainPage::OnModified()
{
    SetModified();
}

BOOL CSetMainPage::OnApply()
{
    UpdateData();
    Store (m_dwLanguage, m_regLanguage);
    if (m_sTempExtensions.Compare(CString(m_regExtensions)))
    {
        Store (m_sTempExtensions, m_regExtensions);
        m_restart = Restart_Cache;
    }
    Store ((m_bLastCommitTime ? L"yes" : L"no"), m_regLastCommitTime);
    Store (m_bUseAero, m_regUseAero);

    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSetMainPage::OnBnClickedEditconfig()
{
    SVN::EnsureConfigFile();

    PWSTR pszPath = NULL;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &pszPath) == S_OK)
    {
        CString path = pszPath;
        CoTaskMemFree(pszPath);

        path += L"\\Subversion\\config";
        CAppUtils::StartTextViewer(path);
    }
}

void CSetMainPage::OnBnClickedChecknewerbutton()
{
    TCHAR com[MAX_PATH + 100] = { 0 };
    GetModuleFileName(NULL, com, MAX_PATH);

    CCreateProcessHelper::CreateProcessDetached(com, L" /command:updatecheck /visible");
}

void CSetMainPage::OnBnClickedSounds()
{
    CAppUtils::LaunchApplication(L"RUNDLL32 Shell32,Control_RunDLL mmsys.cpl,,2", NULL, false);
}

void CSetMainPage::OnBnClickedCreatelib()
{
    CoInitialize(NULL);
    EnsureSVNLibrary();
    CoUninitialize();
}

