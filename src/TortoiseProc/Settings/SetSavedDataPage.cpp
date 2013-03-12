// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013 - TortoiseSVN

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
#include "registry.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include "SetSavedDataPage.h"
#include "StringUtils.h"
#include "SettingsClearAuth.h"

IMPLEMENT_DYNAMIC(CSetSavedDataPage, ISettingsPropPage)

CSetSavedDataPage::CSetSavedDataPage()
    : ISettingsPropPage(CSetSavedDataPage::IDD)
    , m_maxLines(0)
{
    m_regMaxLines = CRegDWORD(_T("Software\\TortoiseSVN\\MaxLinesInLogfile"), 4000);
    m_maxLines = m_regMaxLines;
}

CSetSavedDataPage::~CSetSavedDataPage()
{
}

void CSetSavedDataPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLHISTCLEAR, m_btnUrlHistClear);
    DDX_Control(pDX, IDC_LOGHISTCLEAR, m_btnLogHistClear);
    DDX_Control(pDX, IDC_RESIZABLEHISTCLEAR, m_btnResizableHistClear);
    DDX_Control(pDX, IDC_AUTHHISTCLEAR, m_btnAuthHistClear);
    DDX_Control(pDX, IDC_AUTHHISTCLEARSELECT, m_btnAuthHistClearSelect);
    DDX_Control(pDX, IDC_REPOLOGCLEAR, m_btnRepoLogClear);
    DDX_Text(pDX, IDC_MAXLINES, m_maxLines);
    DDX_Control(pDX, IDC_ACTIONLOGSHOW, m_btnActionLogShow);
    DDX_Control(pDX, IDC_ACTIONLOGCLEAR, m_btnActionLogClear);
    DDX_Control(pDX, IDC_HOOKCLEAR, m_btnHookClear);
}

BOOL CSetSavedDataPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    // find out how many log messages and URLs we've stored
    int nLogHistWC = 0;
    INT_PTR nLogHistMsg = 0;
    int nUrlHistWC = 0;
    INT_PTR nUrlHistItems = 0;
    int nLogHistRepo = 0;
    CRegistryKey regloghist(_T("Software\\TortoiseSVN\\History"));
    CStringList loghistlist;
    regloghist.getSubKeys(loghistlist);
    for (POSITION pos = loghistlist.GetHeadPosition(); pos != NULL; )
    {
        CString sHistName = loghistlist.GetNext(pos);
        if (sHistName.Left(6).CompareNoCase(_T("commit"))==0)
        {
            nLogHistWC++;
            CRegistryKey regloghistwc(_T("Software\\TortoiseSVN\\History\\")+sHistName);
            CStringList loghistlistwc;
            regloghistwc.getValues(loghistlistwc);
            nLogHistMsg += loghistlistwc.GetCount();
        }
        else
        {
            // repoURLs
            CStringList urlhistlistmain;
            CStringList urlhistlistmainvalues;
            CRegistryKey regurlhistlist(_T("Software\\TortoiseSVN\\History\\repoURLS"));
            regurlhistlist.getSubKeys(urlhistlistmain);
            regurlhistlist.getValues(urlhistlistmainvalues);
            nUrlHistItems += urlhistlistmainvalues.GetCount();
            for (POSITION urlpos = urlhistlistmain.GetHeadPosition(); urlpos != NULL; )
            {
                CString sWCUID = urlhistlistmain.GetNext(urlpos);
                nUrlHistWC++;
                CStringList urlhistlistwc;
                CRegistryKey regurlhistlistwc(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sWCUID);
                regurlhistlistwc.getValues(urlhistlistwc);
                nUrlHistItems += urlhistlistwc.GetCount();
            }
        }
    }

    // find out how many dialog sizes / positions we've stored
    INT_PTR nResizableDialogs = 0;
    CRegistryKey regResizable(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState"));
    CStringList resizablelist;
    regResizable.getValues(resizablelist);
    nResizableDialogs += resizablelist.GetCount();

    // find out how many auth data we've stored
    int nSimple = 0;
    int nSSL = 0;
    int nUsername = 0;

    CString sFile;
    bool bIsDir = false;

    TCHAR pathbuf[MAX_PATH] = {0};
    if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathbuf)==S_OK)
    {
        _tcscat_s(pathbuf, _T("\\Subversion\\auth\\"));
        CString sSimple = CString(pathbuf) + _T("svn.simple");
        CString sSSL = CString(pathbuf) + _T("svn.ssl.server");
        CString sUsername = CString(pathbuf) + _T("svn.username");
        CDirFileEnum simpleenum(sSimple);
        while (simpleenum.NextFile(sFile, &bIsDir))
            nSimple++;
        CDirFileEnum sslenum(sSSL);
        while (sslenum.NextFile(sFile, &bIsDir))
            nSSL++;
        CDirFileEnum userenum(sUsername);
        while (userenum.NextFile(sFile, &bIsDir))
            nUsername++;
    }

    CRegistryKey regCerts(L"Software\\TortoiseSVN\\CAPIAuthz");
    CStringList certList;
    regCerts.getValues(certList);
    int nCapi = (int)certList.GetCount();

    CDirFileEnum logenum(CPathUtils::GetAppDataDirectory()+_T("logcache"));
    while (logenum.NextFile(sFile, &bIsDir))
        nLogHistRepo++;
    // the "Repositories.dat" is not a cache file
    nLogHistRepo--;

    BOOL bActionLog = PathFileExists(CPathUtils::GetAppDataDirectory() + _T("logfile.txt"));

    INT_PTR nHooks = 0;
    CRegistryKey regHooks(_T("Software\\TortoiseSVN\\approvedhooks"));
    CStringList hookslist;
    regHooks.getValues(hookslist);
    nHooks += hookslist.GetCount();


    DialogEnableWindow(&m_btnLogHistClear, nLogHistMsg || nLogHistWC);
    DialogEnableWindow(&m_btnUrlHistClear, nUrlHistItems || nUrlHistWC);
    DialogEnableWindow(&m_btnResizableHistClear, nResizableDialogs > 0);
    DialogEnableWindow(&m_btnAuthHistClear, nSimple || nSSL || nUsername || nCapi);
    DialogEnableWindow(&m_btnAuthHistClearSelect, nSimple || nSSL || nUsername || nCapi);
    DialogEnableWindow(&m_btnRepoLogClear, nLogHistRepo >= 0);
    DialogEnableWindow(&m_btnActionLogClear, bActionLog);
    DialogEnableWindow(&m_btnActionLogShow, bActionLog);
    DialogEnableWindow(&m_btnHookClear, nHooks > 0);

    EnableToolTips();

    m_tooltips.Create(this);
    CString sTT;
    sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_LOGHIST_TT, nLogHistMsg, nLogHistWC);
    m_tooltips.AddTool(IDC_LOGHISTORY, sTT);
    m_tooltips.AddTool(IDC_LOGHISTCLEAR, sTT);
    sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_URLHIST_TT, nUrlHistItems, nUrlHistWC);
    m_tooltips.AddTool(IDC_URLHISTORY, sTT);
    m_tooltips.AddTool(IDC_URLHISTCLEAR, sTT);
    sTT.Format(IDS_SETTINGS_SAVEDDATA_RESIZABLE_TT, nResizableDialogs);
    m_tooltips.AddTool(IDC_RESIZABLEHISTORY, sTT);
    m_tooltips.AddTool(IDC_RESIZABLEHISTCLEAR, sTT);
    sTT.FormatMessage(IDS_SETTINGS_SAVEDDATA_AUTH_TT, nSimple, nSSL+nCapi, nUsername);
    m_tooltips.AddTool(IDC_AUTHHISTORY, sTT);
    m_tooltips.AddTool(IDC_AUTHHISTCLEAR, sTT);
    sTT.Format(IDS_SETTINGS_SAVEDDATA_REPOLOGHIST_TT, nLogHistRepo);
    m_tooltips.AddTool(IDC_REPOLOG, sTT);
    m_tooltips.AddTool(IDC_REPOLOGCLEAR, sTT);
    sTT.LoadString(IDS_SETTINGS_SHOWACTIONLOG_TT);
    m_tooltips.AddTool(IDC_ACTIONLOGSHOW, sTT);
    sTT.LoadString(IDS_SETTINGS_MAXACTIONLOGLINES_TT);
    m_tooltips.AddTool(IDC_MAXLINES, sTT);
    sTT.LoadString(IDS_SETTINGS_CLEARACTIONLOG_TT);
    m_tooltips.AddTool(IDC_ACTIONLOGCLEAR, sTT);
    sTT.Format(IDS_SETTINGS_CLEARHOOKS_TT, nHooks);
    m_tooltips.AddTool(IDC_HOOKCLEAR, sTT);

    return TRUE;
}

BOOL CSetSavedDataPage::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CSetSavedDataPage, ISettingsPropPage)
    ON_BN_CLICKED(IDC_URLHISTCLEAR, &CSetSavedDataPage::OnBnClickedUrlhistclear)
    ON_BN_CLICKED(IDC_LOGHISTCLEAR, &CSetSavedDataPage::OnBnClickedLoghistclear)
    ON_BN_CLICKED(IDC_RESIZABLEHISTCLEAR, &CSetSavedDataPage::OnBnClickedResizablehistclear)
    ON_BN_CLICKED(IDC_AUTHHISTCLEAR, &CSetSavedDataPage::OnBnClickedAuthhistclear)
    ON_BN_CLICKED(IDC_REPOLOGCLEAR, &CSetSavedDataPage::OnBnClickedRepologclear)
    ON_BN_CLICKED(IDC_ACTIONLOGSHOW, &CSetSavedDataPage::OnBnClickedActionlogshow)
    ON_BN_CLICKED(IDC_ACTIONLOGCLEAR, &CSetSavedDataPage::OnBnClickedActionlogclear)
    ON_EN_CHANGE(IDC_MAXLINES, OnModified)
    ON_BN_CLICKED(IDC_HOOKCLEAR, &CSetSavedDataPage::OnBnClickedHookclear)
    ON_BN_CLICKED(IDC_AUTHHISTCLEARSELECT, &CSetSavedDataPage::OnBnClickedAuthhistclearselect)
END_MESSAGE_MAP()

void CSetSavedDataPage::OnBnClickedUrlhistclear()
{
    CRegistryKey reg(_T("Software\\TortoiseSVN\\History\\repoURLS"));
    reg.removeKey();
    DialogEnableWindow(&m_btnUrlHistClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_URLHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_URLHISTORY));
}

void CSetSavedDataPage::OnBnClickedLoghistclear()
{
    CRegistryKey reg(_T("Software\\TortoiseSVN\\History"));
    CStringList histlist;
    reg.getSubKeys(histlist);
    for (POSITION pos = histlist.GetHeadPosition(); pos != NULL; )
    {
        CString sHist = histlist.GetNext(pos);
        if (sHist.Left(6).CompareNoCase(_T("commit"))==0)
        {
            CRegistryKey regkey(_T("Software\\TortoiseSVN\\History\\")+sHist);
            regkey.removeKey();
        }
    }

    DialogEnableWindow(&m_btnLogHistClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedResizablehistclear()
{
    CRegistryKey reg(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState"));
    reg.removeKey();
    DialogEnableWindow(&m_btnResizableHistClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedHookclear()
{
    CRegistryKey reg(_T("Software\\TortoiseSVN\\approvedhooks"));
    reg.removeKey();
    DialogEnableWindow(&m_btnHookClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_HOOKCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_HOOKS));
}


void CSetSavedDataPage::OnBnClickedAuthhistclear()
{
    CRegStdString auth = CRegStdString(_T("Software\\TortoiseSVN\\Auth\\"));
    auth.removeKey();
    TCHAR pathbuf[MAX_PATH] = {0};
    if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathbuf)==S_OK)
    {
        _tcscat_s(pathbuf, _T("\\Subversion\\auth"));
        pathbuf[_tcslen(pathbuf)+1] = 0;
        DeleteViaShell(pathbuf, IDS_SETTINGS_DELFILE);
    }
    SHDeleteKey(HKEY_CURRENT_USER, L"Software\\TortoiseSVN\\CAPIAuthz");
    DialogEnableWindow(&m_btnAuthHistClear, false);
    DialogEnableWindow(&m_btnAuthHistClearSelect, false);
    m_tooltips.DelTool(GetDlgItem(IDC_AUTHHISTCLEAR));
    m_tooltips.DelTool(GetDlgItem(IDC_AUTHHISTORY));
}

void CSetSavedDataPage::OnBnClickedRepologclear()
{
    CString path = CPathUtils::GetAppDataDirectory()+_T("logcache");
    TCHAR pathbuf[MAX_PATH] = {0};
    _tcscpy_s(pathbuf, (LPCTSTR)path);
    pathbuf[_tcslen(pathbuf)+1] = 0;

    DeleteViaShell(pathbuf, IDS_SETTINGS_DELCACHE);

    DialogEnableWindow(&m_btnRepoLogClear, false);
    m_tooltips.DelTool(GetDlgItem(IDC_REPOLOG));
    m_tooltips.DelTool(GetDlgItem(IDC_REPOLOGCLEAR));
}

void CSetSavedDataPage::OnBnClickedActionlogshow()
{
    CString logfile = CPathUtils::GetAppDataDirectory() + _T("logfile.txt");
    CAppUtils::StartTextViewer(logfile);
}

void CSetSavedDataPage::OnBnClickedActionlogclear()
{
    CString logfile = CPathUtils::GetAppDataDirectory() + _T("logfile.txt");
    DeleteFile(logfile);
    DialogEnableWindow(&m_btnActionLogClear, false);
    DialogEnableWindow(&m_btnActionLogShow, false);
}

void CSetSavedDataPage::OnModified()
{
    SetModified();
}

BOOL CSetSavedDataPage::OnApply()
{
    Store (m_maxLines,  m_regMaxLines);
    return ISettingsPropPage::OnApply();
}

void CSetSavedDataPage::DeleteViaShell(LPCTSTR path, UINT progressText)
{
    CString p(path);
    p += L"||";
    int len = p.GetLength();
    std::unique_ptr<TCHAR[]> buf(new TCHAR[len+2]);
    wcscpy_s(buf.get(), len+2, p);
    CStringUtils::PipesToNulls(buf.get(), len);

    CString progText(MAKEINTRESOURCE(progressText));
    SHFILEOPSTRUCT fileop;
    fileop.hwnd = m_hWnd;
    fileop.wFunc = FO_DELETE;
    fileop.pFrom = buf.get();
    fileop.pTo = NULL;
    fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION;
    fileop.lpszProgressTitle = progText;
    SHFileOperation(&fileop);
}

void CSetSavedDataPage::OnBnClickedAuthhistclearselect()
{
    CSettingsClearAuth dlg;
    dlg.DoModal();
}
