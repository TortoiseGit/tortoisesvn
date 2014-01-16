// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "CopyDlg.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "registry.h"
#include "TSVNPath.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "HistoryDlg.h"
#include "SVNStatus.h"
#include "SVNProperties.h"
#include "BstrSafeVector.h"
#include "COMError.h"


IMPLEMENT_DYNAMIC(CCopyDlg, CResizableStandAloneDialog)
CCopyDlg::CCopyDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CCopyDlg::IDD, pParent)
    , m_URL(L"")
    , m_sLogMessage(L"")
    , m_sBugID(L"")
    , m_CopyRev(SVNRev::REV_HEAD)
    , m_bDoSwitch(false)
    , m_bMakeParents(false)
    , m_bSettingChanged(false)
    , m_bCancelled(false)
    , m_pThread(NULL)
    , m_pLogDlg(NULL)
    , m_bThreadRunning(0)
    , m_maxrev(0)
    , m_bmodified(false)
{
    m_columnbuf[0] = 0;
}

CCopyDlg::~CCopyDlg()
{
    delete m_pLogDlg;
}

void CCopyDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
    DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
    DDX_Text(pDX, IDC_BUGID, m_sBugID);
    DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
    DDX_Check(pDX, IDC_DOSWITCH, m_bDoSwitch);
    DDX_Check(pDX, IDC_MAKEPARENTS, m_bMakeParents);
    DDX_Control(pDX, IDC_FROMURL, m_FromUrl);
    DDX_Control(pDX, IDC_DESTURL, m_DestUrl);
    DDX_Control(pDX, IDC_EXTERNALSLIST, m_ExtList);
}


BEGIN_MESSAGE_MAP(CCopyDlg, CResizableStandAloneDialog)
    ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
    ON_MESSAGE(WM_TSVN_MAXREVFOUND, OnRevFound)
    ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_BN_CLICKED(IDC_BROWSEFROM, OnBnClickedBrowsefrom)
    ON_BN_CLICKED(IDC_COPYHEAD, OnBnClickedCopyhead)
    ON_BN_CLICKED(IDC_COPYREV, OnBnClickedCopyrev)
    ON_BN_CLICKED(IDC_COPYWC, OnBnClickedCopywc)
    ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
    ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
    ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCopyDlg::OnCbnEditchangeUrlcombo)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_EXTERNALSLIST, &CCopyDlg::OnLvnGetdispinfoExternalslist)
    ON_NOTIFY(LVN_KEYDOWN, IDC_EXTERNALSLIST, &CCopyDlg::OnLvnKeydownExternalslist)
    ON_NOTIFY(NM_CLICK, IDC_EXTERNALSLIST, &CCopyDlg::OnNMClickExternalslist)
    ON_REGISTERED_MESSAGE(CLinkControl::LK_LINKITEMCLICKED, &CCopyDlg::OnCheck)
    ON_BN_CLICKED(IDC_BUGTRAQBUTTON, &CCopyDlg::OnBnClickedBugtraqbutton)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_EXTERNALSLIST, &CCopyDlg::OnNMCustomdrawExternalslist)
    ON_EN_CHANGE(IDC_COPYREVTEXT, &CCopyDlg::OnEnChangeCopyrevtext)
    ON_WM_QUERYENDSESSION()
END_MESSAGE_MAP()


BOOL CCopyDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_EXTGROUP);
    m_aeroControls.SubclassControl(this, IDC_DOSWITCH);
    m_aeroControls.SubclassControl(this, IDC_MAKEPARENTS);
    m_aeroControls.SubclassOkCancelHelp(this);
    m_bCancelled = false;

    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES;
    m_ExtList.SetExtendedStyle(exStyle);
    SetWindowTheme(m_ExtList.GetSafeHwnd(), L"Explorer", NULL);
    m_ExtList.ShowText(CString(MAKEINTRESOURCE(IDS_COPY_WAITFOREXTERNALS)));

    AdjustControlSize(IDC_COPYHEAD);
    AdjustControlSize(IDC_COPYREV);
    AdjustControlSize(IDC_COPYWC);
    AdjustControlSize(IDC_DOSWITCH);
    AdjustControlSize(IDC_MAKEPARENTS);

    CTSVNPath path(m_path);
    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, path.GetUIPathString(), sWindowTitle);

    m_History.SetMaxHistoryItems((LONG)CRegDWORD(L"Software\\TortoiseSVN\\MaxHistoryItems", 25));

    SetRevision(m_CopyRev);

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);

    if (SVN::PathIsURL(path))
    {
        DialogEnableWindow(IDC_COPYWC, FALSE);
        DialogEnableWindow(IDC_DOSWITCH, FALSE);
        SetDlgItemText(IDC_COPYSTARTLABEL, CString(MAKEINTRESOURCE(IDS_COPYDLG_FROMURL)));
    }

    SVN svn;
    CString sUUID;
    m_repoRoot = svn.GetRepositoryRootAndUUID(path, true, sUUID);
    m_repoRoot.TrimRight('/');
    m_wcURL = svn.GetURLFromPath(path);
    if (m_wcURL.IsEmpty() || (!path.IsUrl() && !path.Exists()))
    {
        CString Wrong_URL=path.GetSVNPathString();
        CString temp;
        temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)Wrong_URL);
        ::MessageBox(this->m_hWnd, temp, L"TortoiseSVN", MB_ICONERROR);
        this->EndDialog(IDCANCEL);      //exit
    }
    m_URLCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoPaths\\"+sUUID, L"url");
    m_URLCombo.SetCurSel(0);
    CString relPath = m_wcURL.Mid(m_repoRoot.GetLength());
    if (!m_URL.IsEmpty())
        relPath = m_URL.Mid(m_repoRoot.GetLength());
    CTSVNPath r = CTSVNPath(relPath);
    relPath = r.GetUIPathString();
    relPath.Replace('\\', '/');
    m_URLCombo.AddString(relPath, 0);
    m_URLCombo.SelectString(-1, relPath);
    m_URL = m_wcURL;
    SetDlgItemText(IDC_DESTURL, CPathUtils::CombineUrls(m_repoRoot, relPath));
    SetDlgItemText(IDC_FROMURL, m_wcURL);

    CString reg;
    reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", (LPCTSTR)sUUID);
    m_History.Load(reg, L"logmsgs");

    m_ProjectProperties.ReadProps(m_path);
    if (CRegDWORD(L"Software\\TortoiseSVN\\AlwaysWarnIfNoIssue", FALSE))
        m_ProjectProperties.bWarnIfNoIssue = TRUE;

    m_cLogMessage.Init(m_ProjectProperties);
    m_cLogMessage.SetFont((CString)CRegString(L"Software\\TortoiseSVN\\LogFontName", L"Courier New"), (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogFontSize", 8));

    GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
    CBugTraqAssociations bugtraq_associations;
    bugtraq_associations.Load(m_ProjectProperties.GetProviderUUID(), m_ProjectProperties.sProviderParams);

    if (bugtraq_associations.FindProvider(CTSVNPathList(m_path), &m_bugtraq_association))
    {
        CComPtr<IBugTraqProvider> pProvider;
        HRESULT hr = pProvider.CoCreateInstance(m_bugtraq_association.GetProviderClass());
        if (SUCCEEDED(hr))
        {
            m_BugTraqProvider = pProvider;
            ATL::CComBSTR temp;
            ATL::CComBSTR parameters;
            parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
            hr = pProvider->GetLinkText(GetSafeHwnd(), parameters, &temp);
            if (SUCCEEDED(hr))
            {
                SetDlgItemText(IDC_BUGTRAQBUTTON, temp == 0 ? L"" : temp);
                GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(TRUE);
                GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
            }
        }

        GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
    }
    if (m_ProjectProperties.sMessage.IsEmpty())
    {
        GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
    }
    else
    {
        GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
        if (!m_ProjectProperties.sLabel.IsEmpty())
            SetDlgItemText(IDC_BUGIDLABEL, m_ProjectProperties.sLabel);
        GetDlgItem(IDC_BUGID)->SetFocus();
    }

    if (!m_sLogMessage.IsEmpty())
        m_cLogMessage.SetText(m_sLogMessage);
    else
        m_cLogMessage.SetText(m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEBRANCH));

    OnEnChangeLogmessage();

    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKALL);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKNONE);

    // line up all controls and adjust their sizes.
#define LINKSPACING 9
    RECT rc = AdjustControlSize(IDC_SELECTLABEL);
    rc.right -= 15; // AdjustControlSize() adds 20 pixels for the checkbox/radio button bitmap, but this is a label...
    rc = AdjustStaticSize(IDC_CHECKALL, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKNONE, rc, LINKSPACING);

    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+"+CString(CAppUtils::FindAcceleratorKey(this, IDC_INVISIBLE)));

    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKALL)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);

    CAppUtils::SetAccProperty(m_URLCombo.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+"+CString(CAppUtils::FindAcceleratorKey(this, IDC_TOURLLABEL)));
    CAppUtils::SetAccProperty(GetDlgItem(IDC_FROMURL)->GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+"+CString(CAppUtils::FindAcceleratorKey(this, IDC_COPYSTARTLABEL)));
    CAppUtils::SetAccProperty(GetDlgItem(IDC_DESTURL)->GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+"+CString(CAppUtils::FindAcceleratorKey(this, IDC_DESTLABEL)));

    AddAnchor(IDC_REPOGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_COPYSTARTLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FROMURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_TOURLLABEL, TOP_LEFT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_DESTLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DESTURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MSGGROUP, TOP_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_HISTORY, TOP_LEFT);
    AddAnchor(IDC_BUGTRAQBUTTON, TOP_LEFT);
    AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
    AddAnchor(IDC_BUGID, TOP_RIGHT);
    AddAnchor(IDC_INVISIBLE, TOP_RIGHT);
    AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_FROMGROUP, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_COPYHEAD, MIDDLE_LEFT);
    AddAnchor(IDC_COPYREV, MIDDLE_LEFT);
    AddAnchor(IDC_COPYREVTEXT, MIDDLE_RIGHT);
    AddAnchor(IDC_BROWSEFROM, MIDDLE_RIGHT);
    AddAnchor(IDC_COPYWC, MIDDLE_LEFT);
    AddAnchor(IDC_EXTGROUP, MIDDLE_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTLABEL, MIDDLE_LEFT);
    AddAnchor(IDC_CHECKALL, MIDDLE_LEFT);
    AddAnchor(IDC_CHECKNONE, MIDDLE_LEFT);
    AddAnchor(IDC_EXTERNALSLIST, MIDDLE_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_DOSWITCH, BOTTOM_LEFT);
    AddAnchor(IDC_MAKEPARENTS, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    if ((m_pParentWnd==NULL)&&(GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"CopyDlg");

    m_bSettingChanged = false;
    if (!m_path.IsUrl())
    {
        // start a thread to obtain the highest revision number of the working copy
        // without blocking the dialog
        if ((m_pThread = AfxBeginThread(FindRevThreadEntry, this))==NULL)
        {
            OnCantStartThread();
        }
    }

    return TRUE;
}

UINT CCopyDlg::FindRevThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CCopyDlg*)pVoid)->FindRevThread();
}

UINT CCopyDlg::FindRevThread()
{
    InterlockedExchange(&m_bThreadRunning, TRUE);
    m_bmodified = false;
    if (!m_bCancelled)
    {
        // find the external properties
        SVNStatus stats(&m_bCancelled);
        CTSVNPath retPath;
        svn_client_status_t * s = NULL;
        m_maxrev = 0;
        s = stats.GetFirstFileStatus(m_path, retPath, false, svn_depth_unknown, true, true);
        if (s)
        {
            std::string sUUID;
            if (s->repos_uuid)
                sUUID = s->repos_uuid;
            while ((s) && (!m_bCancelled))
            {
                if (s->repos_uuid && sUUID.empty())
                    sUUID = s->repos_uuid;
                if (s->kind == svn_node_dir)
                {
                    // read the props of this dir and find out if it has svn:external props
                    SVNProperties props(retPath, SVNRev::REV_WC, false, false);
                    for (int i = 0; i < props.GetCount(); ++i)
                    {
                        if (props.GetItemName(i).compare(SVN_PROP_EXTERNALS) == 0)
                        {
                            m_externals.Add(retPath, props.GetItemValue(i), true);
                        }
                    }
                }
                if (s->changed_rev > m_maxrev)
                    m_maxrev = s->changed_rev;
                if ( (s->node_status != svn_wc_status_none) &&
                    (s->node_status != svn_wc_status_normal) &&
                    (s->node_status != svn_wc_status_external) &&
                    (s->node_status != svn_wc_status_unversioned) &&
                    (s->node_status != svn_wc_status_ignored))
                    m_bmodified = true;

                s = stats.GetNextFileStatus(retPath);
            }
            // now go through all externals and scan those as well,
            // as long as they are from the same repository and therefore
            // they can be committed with the main commit.
            std::set<CTSVNPath> exts;
            stats.GetExternals(exts);
            for (auto i: exts)
            {
                ScanWC(i, sUUID);
            }
        }
        if (!m_bCancelled)
            SendMessage(WM_TSVN_MAXREVFOUND);
    }
    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

void CCopyDlg::ScanWC( const CTSVNPath& path, const std::string& sUUID )
{
    // find the external properties
    SVNStatus stats(&m_bCancelled);
    CTSVNPath retPath;
    svn_client_status_t * s = stats.GetFirstFileStatus(path, retPath, false, svn_depth_unknown, true, true);
    if (s == nullptr)
        return;
    if (s->file_external)
        return;
    if (s->repos_uuid && sUUID.compare(s->repos_uuid))
        return;
    while ((s) && (!m_bCancelled))
    {
        if (s->repos_uuid && sUUID.compare(s->repos_uuid))
            return;
        if (s->kind == svn_node_dir)
        {
            // read the props of this dir and find out if it has svn:external props
            SVNProperties props(retPath, SVNRev::REV_WC, false, false);
            for (int i = 0; i < props.GetCount(); ++i)
            {
                if (props.GetItemName(i).compare(SVN_PROP_EXTERNALS) == 0)
                {
                    m_externals.Add(retPath, props.GetItemValue(i), true);
                }
            }
        }
        if (s->changed_rev > m_maxrev)
            m_maxrev = s->changed_rev;
        if ( (s->node_status != svn_wc_status_none) &&
            (s->node_status != svn_wc_status_normal) &&
            (s->node_status != svn_wc_status_external) &&
            (s->node_status != svn_wc_status_unversioned) &&
            (s->node_status != svn_wc_status_ignored))
            m_bmodified = true;

        s = stats.GetNextFileStatus(retPath);
    }
    std::set<CTSVNPath> exts;
    stats.GetExternals(exts);
    for (auto i: exts)
    {
        ScanWC(i, sUUID);
    }
}

void CCopyDlg::OnOK()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }
    m_bCancelled = true;
    // check if the status thread has already finished
    if ((m_pThread)&&(m_bThreadRunning))
    {
        WaitForSingleObject(m_pThread->m_hThread, 1000);
        if (m_bThreadRunning)
        {
            // we gave the thread a chance to quit. Since the thread didn't
            // listen to us we have to kill it.
            TerminateThread(m_pThread->m_hThread, (DWORD)-1);
            InterlockedExchange(&m_bThreadRunning, FALSE);
        }
    }

    CString id;
    GetDlgItemText(IDC_BUGID, id);
    CString sRevText;
    GetDlgItemText(IDC_COPYREVTEXT, sRevText);
    if (!m_ProjectProperties.CheckBugID(id))
    {
        ShowEditBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    m_sLogMessage = m_cLogMessage.GetText();
    if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
    {
        if (CTaskDialog::IsSupported())
        {
            CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK1)),
                                        CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK2)),
                                        L"TortoiseSVN",
                                        0,
                                        TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK4)));
            taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskdlg.SetDefaultCommandControl(2);
            taskdlg.SetMainIcon(TD_WARNING_ICON);
            if (taskdlg.DoModal(m_hWnd) != 1)
                return;
        }
        else
        {
            if (TSVNMessageBox(this->m_hWnd, IDS_COMMITDLG_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
                return;
        }
    }
    UpdateData(TRUE);

    if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
        m_CopyRev = SVNRev(SVNRev::REV_HEAD);
    else if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYWC)
        m_CopyRev = SVNRev(SVNRev::REV_WC);
    else
        m_CopyRev = SVNRev(sRevText);

    if (!m_CopyRev.IsValid())
    {
        ShowEditBalloon(IDC_COPYREVTEXT, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    CString combourl = m_URLCombo.GetWindowString();
    if (combourl.IsEmpty())
        combourl = m_URLCombo.GetString();

    if ((m_wcURL.CompareNoCase(combourl)==0)&&(m_CopyRev.IsHead()))
    {
        CString temp;
        temp.FormatMessage(IDS_ERR_COPYITSELF, (LPCTSTR)m_wcURL, (LPCTSTR)m_URLCombo.GetString());
        ::MessageBox(this->m_hWnd, temp, L"TortoiseSVN", MB_ICONERROR);
        return;
    }

    m_URLCombo.SaveHistory();
    m_URL = CPathUtils::CombineUrls(m_repoRoot, m_URLCombo.GetString());
    if (!CTSVNPath(m_URL).IsValidOnWindows())
    {
        if (CTaskDialog::IsSupported())
        {
            CString sInfo;
            sInfo.Format(IDS_WARN_NOVALIDPATH_TASK1, (LPCTSTR)m_URL);
            CTaskDialog taskdlg(sInfo,
                                CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK4)));
            taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskdlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK5)));
            taskdlg.SetDefaultCommandControl(2);
            taskdlg.SetMainIcon(TD_WARNING_ICON);
            if (taskdlg.DoModal(m_hWnd) != 1)
                return;
        }
        else
        {
            if (MessageBox(CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH)), CString(MAKEINTRESOURCE(IDS_APPNAME)), MB_ICONINFORMATION|MB_YESNO) != IDYES)
                return;
        }
    }
    CStringUtils::WriteAsciiStringToClipboard(m_URL);

    // now let the bugtraq plugin check the commit message
    CComPtr<IBugTraqProvider2> pProvider2 = NULL;
    if (m_BugTraqProvider)
    {
        HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
        if (SUCCEEDED(hr))
        {
            ATL::CComBSTR temp;
            ATL::CComBSTR sourceURL;
            sourceURL.Attach(m_wcURL.AllocSysString());
            ATL::CComBSTR parameters;
            parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
            ATL::CComBSTR targetURL;
            targetURL.Attach(m_URL.AllocSysString());
            ATL::CComBSTR commitMessage;
            commitMessage.Attach(m_sLogMessage.AllocSysString());
            CBstrSafeVector pathList(1);
            pathList.PutElement(0, m_path.GetSVNPathString());

            hr = pProvider2->CheckCommit(GetSafeHwnd(), parameters, sourceURL, targetURL, pathList, commitMessage, &temp);
            if (FAILED(hr))
            {
                OnComError(hr);
            }
            else
            {
                CString sError = temp == 0 ? L"" : temp;
                if (!sError.IsEmpty())
                {
                    CAppUtils::ReportFailedHook(m_hWnd, sError);
                    return;
                }
            }
        }
    }

    if (!m_sLogMessage.IsEmpty())
    {
        m_History.AddEntry(m_sLogMessage);
        m_History.Save();
    }

    m_sBugID.Trim();
    if (!m_sBugID.IsEmpty())
    {
        m_sBugID.Replace(L", ", L",");
        m_sBugID.Replace(L" ,", L",");
        CString sBugID = m_ProjectProperties.sMessage;
        sBugID.Replace(L"%BUGID%", m_sBugID);
        if (m_ProjectProperties.bAppend)
            m_sLogMessage += L"\n" + sBugID + L"\n";
        else
            m_sLogMessage = sBugID + L"\n" + m_sLogMessage;
        UpdateData(FALSE);
    }
    CResizableStandAloneDialog::OnOK();
}

void CCopyDlg::OnBnClickedBugtraqbutton()
{
    m_tooltips.Pop();   // hide the tooltips
    CString sMsg = m_cLogMessage.GetText();

    if (m_BugTraqProvider == NULL)
        return;

    ATL::CComBSTR parameters;
    parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
    ATL::CComBSTR commonRoot;
    commonRoot.Attach(m_path.GetWinPathString().AllocSysString());
    CBstrSafeVector pathList(1);
    pathList.PutElement(0, m_path.GetSVNPathString());

    ATL::CComBSTR originalMessage;
    originalMessage.Attach(sMsg.AllocSysString());
    ATL::CComBSTR temp;
    m_revProps.clear();

    // first try the IBugTraqProvider2 interface
    CComPtr<IBugTraqProvider2> pProvider2 = NULL;
    HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
    if (SUCCEEDED(hr))
    {
        ATL::CComBSTR repositoryRoot;
        repositoryRoot.Attach(m_wcURL.AllocSysString());
        ATL::CComBSTR bugIDOut;
        GetDlgItemText(IDC_BUGID, m_sBugID);
        ATL::CComBSTR bugID;
        bugID.Attach(m_sBugID.AllocSysString());
        CBstrSafeVector revPropNames;
        CBstrSafeVector revPropValues;
        if (FAILED(hr = pProvider2->GetCommitMessage2(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, originalMessage, bugID, &bugIDOut, &revPropNames, &revPropValues, &temp)))
        {
            OnComError(hr);
        }
        else
        {
            if (bugIDOut)
            {
                m_sBugID = bugIDOut;
                SetDlgItemText(IDC_BUGID, m_sBugID);
            }
            m_cLogMessage.SetText((LPCTSTR)temp);
            BSTR HUGEP *pbRevNames;
            BSTR HUGEP *pbRevValues;

            HRESULT hr1 = SafeArrayAccessData(revPropNames, (void HUGEP**)&pbRevNames);
            if (SUCCEEDED(hr1))
            {
                HRESULT hr2 = SafeArrayAccessData(revPropValues, (void HUGEP**)&pbRevValues);
                if (SUCCEEDED(hr2))
                {
                    if (revPropNames->rgsabound->cElements == revPropValues->rgsabound->cElements)
                    {
                        for (ULONG i = 0; i < revPropNames->rgsabound->cElements; i++)
                        {
                            m_revProps[pbRevNames[i]] = pbRevValues[i];
                        }
                    }
                    SafeArrayUnaccessData(revPropValues);
                }
                SafeArrayUnaccessData(revPropNames);
            }
        }
    }
    else
    {
        // if IBugTraqProvider2 failed, try IBugTraqProvider
        CComPtr<IBugTraqProvider> pProvider = NULL;
        hr = m_BugTraqProvider.QueryInterface(&pProvider);
        if (FAILED(hr))
        {
            OnComError(hr);
            return;
        }

        if (FAILED(hr = pProvider->GetCommitMessage(GetSafeHwnd(), parameters, commonRoot, pathList, originalMessage, &temp)))
        {
            OnComError(hr);
        }
        else
            m_cLogMessage.SetText((LPCTSTR)temp);
    }
    m_sLogMessage = m_cLogMessage.GetText();
    if (!m_ProjectProperties.sMessage.IsEmpty())
    {
        CString sBugID = m_ProjectProperties.FindBugID(m_sLogMessage);
        if (!sBugID.IsEmpty())
        {
            SetDlgItemText(IDC_BUGID, sBugID);
        }
    }

    m_cLogMessage.SetFocus();
}

void CCopyDlg::OnComError( HRESULT hr )
{
    COMError ce(hr);
    CString sErr;
    sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), ce.GetMessageAndDescription().c_str());
    ::MessageBox(m_hWnd, sErr, L"TortoiseSVN", MB_ICONERROR);
}

void CCopyDlg::OnBnClickedBrowse()
{
    m_tooltips.Pop();   // hide the tooltips
    SVNRev rev = SVNRev::REV_HEAD;

    CAppUtils::BrowseRepository(m_repoRoot, m_URLCombo, this, rev);
}

void CCopyDlg::OnBnClickedHelp()
{
    m_tooltips.Pop();   // hide the tooltips
    OnHelp();
}

void CCopyDlg::OnCancel()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }
    m_bCancelled = true;
    // check if the status thread has already finished
    if ((m_pThread)&&(m_bThreadRunning))
    {
        WaitForSingleObject(m_pThread->m_hThread, 1000);
        if (m_bThreadRunning)
        {
            // we gave the thread a chance to quit. Since the thread didn't
            // listen to us we have to kill it.
            TerminateThread(m_pThread->m_hThread, (DWORD)-1);
            InterlockedExchange(&m_bThreadRunning, FALSE);
        }
    }
    if (m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEBRANCH).Compare(m_cLogMessage.GetText()) != 0)
    {
        CString sMsg = m_cLogMessage.GetText();
        if (!sMsg.IsEmpty())
        {
            m_History.AddEntry(sMsg);
            m_History.Save();
        }
    }
    CResizableStandAloneDialog::OnCancel();
}

BOOL CCopyDlg::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);

    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_RETURN:
            if (OnEnterPressed())
                return TRUE;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCopyDlg::OnBnClickedBrowsefrom()
{
    m_tooltips.Pop();   // hide the tooltips
    UpdateData(TRUE);
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
        return;
    AfxGetApp()->DoWaitCursor(1);
    //now show the log dialog
    if (!m_wcURL.IsEmpty())
    {
        delete m_pLogDlg;
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetParams(CTSVNPath(m_wcURL), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, TRUE);
        m_pLogDlg->SetSelect(true);
        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
        m_pLogDlg->m_wParam = 0;
        m_pLogDlg->m_pNotifyWindow = this;
    }
    AfxGetApp()->DoWaitCursor(-1);
}

void CCopyDlg::OnEnChangeLogmessage()
{
    CString sTemp = m_cLogMessage.GetText();
    DialogEnableWindow(IDOK, sTemp.GetLength() >= m_ProjectProperties.nMinLogSize);
}

LPARAM CCopyDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
    CString temp;
    temp.Format(L"%Id", lParam);
    SetDlgItemText(IDC_COPYREVTEXT, temp);
    CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
    return 0;
}

void CCopyDlg::OnBnClickedCopyhead()
{
    m_tooltips.Pop();   // hide the tooltips
    m_bSettingChanged = true;
}

void CCopyDlg::OnBnClickedCopyrev()
{
    m_tooltips.Pop();   // hide the tooltips
    m_bSettingChanged = true;
}

void CCopyDlg::OnBnClickedCopywc()
{
    m_tooltips.Pop();   // hide the tooltips
    m_bSettingChanged = true;
}

void CCopyDlg::OnBnClickedHistory()
{
    m_tooltips.Pop();   // hide the tooltips
    SVN svn;
    CHistoryDlg historyDlg;
    historyDlg.SetHistory(m_History);
    if (historyDlg.DoModal()==IDOK)
    {
        if (historyDlg.GetSelectedText().Compare(m_cLogMessage.GetText().Left(historyDlg.GetSelectedText().GetLength()))!=0)
        {
            if (m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEBRANCH).Compare(m_cLogMessage.GetText())!=0)
                m_cLogMessage.InsertText(historyDlg.GetSelectedText(), !m_cLogMessage.GetText().IsEmpty());
            else
                m_cLogMessage.SetText(historyDlg.GetSelectedText());
        }
        DialogEnableWindow(IDOK, m_ProjectProperties.nMinLogSize <= m_cLogMessage.GetText().GetLength());
    }
}

LPARAM CCopyDlg::OnRevFound(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // we have found the highest last-committed revision
    // in the working copy
    if ((!m_bSettingChanged)&&(m_maxrev != 0)&&(!m_bCancelled))
    {
        // we only change the setting automatically if the user hasn't done so
        // already him/herself, if the highest revision is valid. And of course,
        // if the thread hasn't been stopped forcefully.
        if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
        {
            if (m_bmodified)
            {
                // the working copy has local modifications.
                // show a warning balloon if the user has selected HEAD as the
                // source revision
                m_tooltips.ShowBalloon(IDC_COPYHEAD, IDS_WARN_COPYHEADWITHLOCALMODS, IDS_WARN_WARNING, TTI_WARNING);
            }
            else
            {
                // and of course, we only change it if the radio button for a REPO-to-REPO copy
                // is enabled for HEAD and if there are no local modifications
                CString temp;
                temp.Format(L"%ld", m_maxrev);
                SetDlgItemText(IDC_COPYREVTEXT, temp);
                CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
            }
        }
    }


    if (m_externals.size())
        m_ExtList.ClearText();
    else
        m_ExtList.ShowText(CString(MAKEINTRESOURCE(IDS_COPY_NOEXTERNALSFOUND)));

    m_ExtList.SetRedraw(false);
    m_ExtList.DeleteAllItems();

    int c = ((CHeaderCtrl*)(m_ExtList.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_ExtList.DeleteColumn(c--);
    CString temp;
    temp.LoadString(IDS_STATUSLIST_COLFILE);
    m_ExtList.InsertColumn(0, temp);
    temp.LoadString(IDS_STATUSLIST_COLURL);
    m_ExtList.InsertColumn(1, temp);
    temp.LoadString(IDS_STATUSLIST_COLREVISION);
    m_ExtList.InsertColumn(2, temp, LVCFMT_RIGHT);
    temp.LoadString(IDS_COPYEXTLIST_COLTAGGED);
    m_ExtList.InsertColumn(3, temp, LVCFMT_RIGHT);
    m_ExtList.SetItemCountEx((int)m_externals.size());

    CRect rect;
    m_ExtList.GetClientRect(&rect);
    int cx = (rect.Width()-180)/2;
    m_ExtList.SetColumnWidth(0, cx);
    m_ExtList.SetColumnWidth(1, cx);
    m_ExtList.SetColumnWidth(2, 80);
    m_ExtList.SetColumnWidth(3, 80);

    m_ExtList.SetRedraw(true);
    DialogEnableWindow(IDC_EXTERNALSLIST, true);

    return 0;
}

void CCopyDlg::SetRevision(const SVNRev& rev)
{
    if (rev.IsHead())
    {
        CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYHEAD);
    }
    else if (rev.IsWorking())
    {
        CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYWC);
    }
    else
    {
        CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
        CString temp;
        temp.Format(L"%ld", (LONG)rev);
        SetDlgItemText(IDC_COPYREVTEXT, temp);
    }
}

void CCopyDlg::OnEnChangeCopyrevtext()
{
    CString sText;
    GetDlgItemText(IDC_COPYREVTEXT, sText);
    if (sText.IsEmpty())
    {
        CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYHEAD);
    }
    else
    {
        CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
    }
}

void CCopyDlg::OnCbnEditchangeUrlcombo()
{
    m_bSettingChanged = true;
    SetDlgItemText(IDC_DESTURL, CPathUtils::CombineUrls(m_repoRoot, m_URLCombo.GetWindowString()));
}

void CCopyDlg::OnNMCustomdrawExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_ExtList.HasText())
        return;

    // Draw externals that are already tagged to a specific revision in gray.
    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        *pResult = CDRF_NOTIFYSUBITEMDRAW;

        COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
        if (pLVCD->nmcd.dwItemSpec < (UINT_PTR)m_externals.size())
        {
            SVNExternal ext = m_externals[pLVCD->nmcd.dwItemSpec];
            if ((ext.origrevision.kind == svn_opt_revision_number)||(ext.origrevision.kind == svn_opt_revision_date))
                crText = GetSysColor(COLOR_GRAYTEXT);
        }
        // Store the color back in the NMLVCUSTOMDRAW struct.
        pLVCD->clrText = crText;
    }
}

void CCopyDlg::OnLvnGetdispinfoExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    *pResult = 0;

    if (m_ExtList.HasText())
        return;

    if (pDispInfo)
    {
        if (pDispInfo->item.iItem < (int)m_externals.size())
        {
            SVNExternal ext = m_externals[pDispInfo->item.iItem];
            if (pDispInfo->item.mask & LVIF_INDENT)
            {
                pDispInfo->item.iIndent = 0;
            }
            if (pDispInfo->item.mask & LVIF_IMAGE)
            {
                pDispInfo->item.mask |= LVIF_STATE;
                pDispInfo->item.stateMask = LVIS_STATEIMAGEMASK;

                if (ext.adjust)
                {
                    //Turn check box on
                    pDispInfo->item.state = INDEXTOSTATEIMAGEMASK(2);
                }
                else
                {
                    //Turn check box off
                    pDispInfo->item.state = INDEXTOSTATEIMAGEMASK(1);
                }
            }
            if (pDispInfo->item.mask & LVIF_TEXT)
            {
                switch (pDispInfo->item.iSubItem)
                {
                case 0: // folder
                    {
                        CTSVNPath p = ext.path;
                        p.AppendPathString(ext.targetDir);
                        lstrcpyn(m_columnbuf, p.GetWinPath(), min(MAX_PATH-2, pDispInfo->item.cchTextMax));
                        int cWidth = m_ExtList.GetColumnWidth(0);
                        cWidth = max(28, cWidth-28);
                        CDC * pDC = m_ExtList.GetDC();
                        if (pDC != NULL)
                        {
                            CFont * pFont = pDC->SelectObject(m_ExtList.GetFont());
                            PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
                            pDC->SelectObject(pFont);
                            ReleaseDC(pDC);
                        }
                    }
                    break;
                case 1: // url
                    {
                        lstrcpyn(m_columnbuf, ext.url, min(MAX_PATH-2, pDispInfo->item.cchTextMax));
                        SVNRev peg(ext.pegrevision);
                        if (peg.IsValid() && !peg.IsHead())
                        {
                            wcscat_s(m_columnbuf, L"@");
                            wcscat_s(m_columnbuf, peg.ToString());
                        }
                        int cWidth = m_ExtList.GetColumnWidth(1);
                        cWidth = max(14, cWidth-14);
                        CDC * pDC = m_ExtList.GetDC();
                        if (pDC != NULL)
                        {
                            CFont * pFont = pDC->SelectObject(m_ExtList.GetFont());
                            PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
                            pDC->SelectObject(pFont);
                            ReleaseDC(pDC);
                        }
                    }
                    break;
                case 2: // revision
                    m_columnbuf[0] = 0;
                    if ((ext.revision.kind == svn_opt_revision_number) && (ext.revision.value.number >= 0))
                        swprintf_s(m_columnbuf, L"%ld", ext.revision.value.number);
                    break;
                case 3: // tagged
                    m_columnbuf[0] = 0;
                    if (ext.origrevision.kind == svn_opt_revision_number)
                        swprintf_s(m_columnbuf, L"%ld", ext.origrevision.value.number);
                    else if (ext.origrevision.kind == svn_opt_revision_date)
                    {
                        SVNRev r(ext.origrevision);
                        wcscpy_s(m_columnbuf, (LPCTSTR)r.ToString());
                    }
                    break;
                default:
                    m_columnbuf[0] = 0;
                }
                pDispInfo->item.pszText = m_columnbuf;
            }
        }
    }
}

void CCopyDlg::OnLvnKeydownExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
    *pResult = 0;

    if (m_ExtList.HasText())
        return;

    if (pLVKeyDow->wVKey == VK_SPACE)
    {
        int nFocusedItem = m_ExtList.GetNextItem(-1, LVNI_FOCUSED);
        if (nFocusedItem >= 0)
            ToggleCheckbox(nFocusedItem);
    }
}

void CCopyDlg::OnNMClickExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    *pResult = 0;

    if (m_ExtList.HasText())
        return;

    UINT flags = 0;
    CPoint point(pNMItemActivate->ptAction);
    // Make the hit test...
    int item = m_ExtList.HitTest(point, &flags);

    if (item != -1)
    {
        // We hit one item... did we hit state image (check box)?
        if( (flags & LVHT_ONITEMSTATEICON) != 0)
        {
            ToggleCheckbox(item);
        }
    }
}

void CCopyDlg::ToggleCheckbox(int index)
{
    SVNExternal ext = m_externals[index];
    ext.adjust = !ext.adjust;
    m_externals[index] = ext;
    m_ExtList.RedrawItems(index, index);
    m_ExtList.UpdateWindow();
}

LRESULT CCopyDlg::OnCheck(WPARAM wnd, LPARAM)
{
    HWND hwnd = (HWND)wnd;
    bool check = false;
    if (hwnd == GetDlgItem(IDC_CHECKALL)->GetSafeHwnd())
        check = true;

    for (std::vector<SVNExternal>::iterator it = m_externals.begin(); it != m_externals.end(); ++it)
    {
        it->adjust = check;
    }
    m_ExtList.RedrawItems(0, m_ExtList.GetItemCount());
    m_ExtList.UpdateWindow();

    return 0;
}

BOOL CCopyDlg::OnQueryEndSession()
{
    if (!__super::OnQueryEndSession())
        return FALSE;

    OnCancel();

    return TRUE;
}
