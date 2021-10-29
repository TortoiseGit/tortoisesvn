// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2021 - TortoiseSVN
// Copyright (C) 2019 - TortoiseGit

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
#include "UnicodeUtils.h"
#include "RepositoryBrowser.h"
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
#include "Hooks.h"

IMPLEMENT_DYNAMIC(CCopyDlg, CResizableStandAloneDialog)
CCopyDlg::CCopyDlg(CWnd *pParent /*=NULL*/)
    : CResizableStandAloneDialog(CCopyDlg::IDD, pParent)
    , m_copyRev(SVNRev::REV_HEAD)
    , m_bDoSwitch(false)
    , m_bMakeParents(false)
    , m_pLogDlg(nullptr)
    , m_maxRev(0)
    , m_bModified(false)
    , m_bSettingChanged(false)
    , m_pThread(nullptr)
    , m_bCancelled(false)
    , m_bThreadRunning(0)
{
    m_columnBuf[0] = 0;
}

CCopyDlg::~CCopyDlg()
{
    delete m_pLogDlg;
}

void CCopyDlg::DoDataExchange(CDataExchange *pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_urlCombo);
    DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
    DDX_Text(pDX, IDC_BUGID, m_sBugID);
    DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
    DDX_Check(pDX, IDC_DOSWITCH, m_bDoSwitch);
    DDX_Check(pDX, IDC_MAKEPARENTS, m_bMakeParents);
    DDX_Control(pDX, IDC_FROMURL, m_fromUrl);
    DDX_Control(pDX, IDC_DESTURL, m_destUrl);
    DDX_Control(pDX, IDC_EXTERNALSLIST, m_extList);
    DDX_Control(pDX, IDC_CHECKALL, m_checkAll);
    DDX_Control(pDX, IDC_CHECKNONE, m_checkNone);
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
    m_extList.SetExtendedStyle(exStyle);
    SetWindowTheme(m_extList.GetSafeHwnd(), L"Explorer", nullptr);
    m_extList.ShowText(CString(MAKEINTRESOURCE(IDS_COPY_WAITFOREXTERNALS)));

    AdjustControlSize(IDC_COPYHEAD);
    AdjustControlSize(IDC_COPYREV);
    AdjustControlSize(IDC_COPYWC);
    AdjustControlSize(IDC_DOSWITCH);
    AdjustControlSize(IDC_MAKEPARENTS);

    CTSVNPath path(m_path);
    CString   sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, path.GetUIPathString(), sWindowTitle);

    m_history.SetMaxHistoryItems(static_cast<LONG>(CRegDWORD(L"Software\\TortoiseSVN\\MaxHistoryItems", 25)));

    SetRevision(m_copyRev);

    m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);

    if (SVN::PathIsURL(path))
    {
        DialogEnableWindow(IDC_COPYWC, FALSE);
        DialogEnableWindow(IDC_DOSWITCH, FALSE);
        SetDlgItemText(IDC_COPYSTARTLABEL, CString(MAKEINTRESOURCE(IDS_COPYDLG_FROMURL)));
    }

    SVN     svn;
    CString sUuid;
    m_repoRoot = svn.GetRepositoryRootAndUUID(path, true, sUuid);
    m_repoRoot.TrimRight('/');
    m_wcURL = svn.GetURLFromPath(path);
    if (m_wcURL.IsEmpty() || (!path.IsUrl() && !path.Exists()))
    {
        CString wrongURL = path.GetSVNPathString();
        CString temp;
        temp.Format(IDS_ERR_NOURLOFFILE, static_cast<LPCWSTR>(wrongURL));
        ::MessageBox(this->m_hWnd, temp, L"TortoiseSVN", MB_ICONERROR);
        this->EndDialog(IDCANCEL); //exit
    }
    m_urlCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoPaths\\" + sUuid, L"url");
    m_urlCombo.SetCurSel(0);
    CString relPath = m_wcURL.Mid(m_repoRoot.GetLength());
    if (!m_url.IsEmpty())
    {
        // allow the use of urls relative to the repo root
        if (m_url[0] != '^')
            relPath = m_url.Mid(m_repoRoot.GetLength());
        else
            relPath = m_url.Mid(1);
    }
    CTSVNPath r = CTSVNPath(relPath);
    relPath     = CPathUtils::PathUnescape(r.GetSVNPathString());
    relPath.Replace('\\', '/');
    m_urlCombo.AddString(relPath, 0);
    m_urlCombo.SelectString(-1, relPath);
    m_url = m_wcURL;
    OnCbnEditchangeUrlcombo();
    m_bSettingChanged = false;
    SetDlgItemText(IDC_FROMURL, m_wcURL);

    CString reg;
    reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", static_cast<LPCWSTR>(sUuid));
    m_history.Load(reg, L"logmsgs");

    m_projectProperties.ReadProps(m_path);
    if (CRegDWORD(L"Software\\TortoiseSVN\\AlwaysWarnIfNoIssue", FALSE))
        m_projectProperties.bWarnIfNoIssue = TRUE;

    m_cLogMessage.Init(m_projectProperties);
    m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());

    GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
    CBugTraqAssociations bugtraqAssociations;
    bugtraqAssociations.Load(m_projectProperties.GetProviderUUID(), m_projectProperties.sProviderParams);

    if (bugtraqAssociations.FindProvider(CTSVNPathList(m_path), &m_bugtraqAssociation))
    {
        CComPtr<IBugTraqProvider> pProvider;
        HRESULT                   hr = pProvider.CoCreateInstance(m_bugtraqAssociation.GetProviderClass());
        if (SUCCEEDED(hr))
        {
            m_bugTraqProvider = pProvider;
            ATL::CComBSTR temp;
            ATL::CComBSTR parameters;
            parameters.Attach(m_bugtraqAssociation.GetParameters().AllocSysString());
            hr = pProvider->GetLinkText(GetSafeHwnd(), parameters, &temp);
            if (SUCCEEDED(hr))
            {
                SetDlgItemText(IDC_BUGTRAQBUTTON, temp == 0 ? L"" : static_cast<LPCWSTR>(temp));
                GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(TRUE);
                GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
            }
        }

        GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
    }
    if (m_projectProperties.sMessage.IsEmpty())
    {
        GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
    }
    else
    {
        GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
        if (!m_projectProperties.sLabel.IsEmpty())
            SetDlgItemText(IDC_BUGIDLABEL, m_projectProperties.sLabel);
        GetDlgItem(IDC_BUGID)->SetFocus();
    }

    if (!m_sLogMessage.IsEmpty())
        m_cLogMessage.SetText(m_sLogMessage);
    else
        m_cLogMessage.SetText(m_projectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEBRANCH));

    OnEnChangeLogmessage();

    // line up all controls and adjust their sizes.
#define LINKSPACING 9
    RECT rc = AdjustControlSize(IDC_SELECTLABEL);
    rc.right -= 15; // AdjustControlSize() adds 20 pixels for the checkbox/radio button bitmap, but this is a label...
    rc = AdjustStaticSize(IDC_CHECKALL, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKNONE, rc, LINKSPACING);

    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+" + CString(CAppUtils::FindAcceleratorKey(this, IDC_INVISIBLE)));

    CAppUtils::SetAccProperty(m_urlCombo.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+" + CString(CAppUtils::FindAcceleratorKey(this, IDC_TOURLLABEL)));
    CAppUtils::SetAccProperty(GetDlgItem(IDC_FROMURL)->GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+" + CString(CAppUtils::FindAcceleratorKey(this, IDC_COPYSTARTLABEL)));
    CAppUtils::SetAccProperty(GetDlgItem(IDC_DESTURL)->GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+" + CString(CAppUtils::FindAcceleratorKey(this, IDC_DESTLABEL)));

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

    if ((m_pParentWnd == nullptr) && (GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"CopyDlg");

    m_bSettingChanged = false;
    if (!m_path.IsUrl())
    {
        // start a thread to obtain the highest revision number of the working copy
        // without blocking the dialog
        InterlockedExchange(&m_bThreadRunning, TRUE);
        if ((m_pThread = AfxBeginThread(FindRevThreadEntry, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED)) == nullptr)
        {
            InterlockedExchange(&m_bThreadRunning, FALSE);
            OnCantStartThread();
        }
        else
        {
            m_pThread->m_bAutoDelete = FALSE;
            m_pThread->ResumeThread();
        }
    }
    else
        m_extList.ShowText(CString(MAKEINTRESOURCE(IDS_COPY_NOEXTERNALSFORURLS)));

    SetTheme(CTheme::Instance().IsDarkTheme());

    return TRUE;
}

UINT CCopyDlg::FindRevThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return static_cast<CCopyDlg *>(pVoid)->FindRevThread();
}

UINT CCopyDlg::FindRevThread()
{
    m_bModified = false;
    if (!m_bCancelled)
    {
        // find the external properties
        SVNStatus            stats(&m_bCancelled);
        CTSVNPath            retPath;
        svn_client_status_t *s = nullptr;
        m_maxRev               = 0;
        m_externals.clear();
        s = stats.GetFirstFileStatus(m_path, retPath, false, svn_depth_unknown, true, true);
        if (s)
        {
            std::string sUuid;
            if (s->repos_uuid)
                sUuid = s->repos_uuid;
            while ((s) && (!m_bCancelled))
            {
                if (s->repos_uuid && sUuid.empty())
                    sUuid = s->repos_uuid;
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
                if (s->changed_rev > m_maxRev)
                    m_maxRev = s->changed_rev;
                if ((s->node_status != svn_wc_status_none) &&
                    (s->node_status != svn_wc_status_normal) &&
                    (s->node_status != svn_wc_status_external) &&
                    (s->node_status != svn_wc_status_unversioned) &&
                    (s->node_status != svn_wc_status_ignored))
                    m_bModified = true;

                s = stats.GetNextFileStatus(retPath);
            }
        }
        // important note:
        // recursive externals can not be tagged, because the external
        // itself is not copied, and therefore there is no defined target
        // on where to modify the svn:externals property
        if (!m_bCancelled)
            SendMessage(WM_TSVN_MAXREVFOUND);
    }
    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

void CCopyDlg::OnOK()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }
    m_bCancelled = true;
    // check if the status thread has already finished
    if ((m_pThread) && (m_bThreadRunning))
    {
        WaitForSingleObject(m_pThread->m_hThread, 1000);
        if (m_bThreadRunning)
        {
            // we gave the thread a chance to quit. Since the thread didn't
            // listen to us we have to kill it.
            TerminateThread(m_pThread->m_hThread, static_cast<DWORD>(-1));
            delete m_pThread;
            InterlockedExchange(&m_bThreadRunning, FALSE);
        }
    }

    CString id;
    GetDlgItemText(IDC_BUGID, id);
    CString sRevText;
    GetDlgItemText(IDC_COPYREVTEXT, sRevText);
    if (!m_projectProperties.CheckBugID(id))
    {
        ShowEditBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    m_sLogMessage = m_cLogMessage.GetText();
    if ((m_projectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_projectProperties.HasBugID(m_sLogMessage)))
    {
        CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK1)),
                            CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(200);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        if (taskDlg.DoModal(m_hWnd) != 100)
            return;
    }
    UpdateData(TRUE);

    if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
        m_copyRev = SVNRev(SVNRev::REV_HEAD);
    else if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYWC)
        m_copyRev = SVNRev(SVNRev::REV_WC);
    else
        m_copyRev = SVNRev(sRevText);

    if (!m_copyRev.IsValid())
    {
        ShowEditBalloon(IDC_COPYREVTEXT, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    CString comboUrl = m_urlCombo.GetWindowString();
    if (comboUrl.IsEmpty())
        comboUrl = m_urlCombo.GetString();
    auto comboUrlA = CUnicodeUtils::GetUTF8(comboUrl);
    auto comboUrlW = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(comboUrlA));

    if ((m_wcURL.CompareNoCase(comboUrlW) == 0) && (m_copyRev.IsHead()))
    {
        CString temp;
        temp.FormatMessage(IDS_ERR_COPYITSELF, static_cast<LPCWSTR>(m_wcURL), static_cast<LPCWSTR>(m_urlCombo.GetString()));
        ::MessageBox(this->m_hWnd, temp, L"TortoiseSVN", MB_ICONERROR);
        return;
    }

    m_urlCombo.SaveHistory();
    m_url = CPathUtils::CombineUrls(m_repoRoot, m_urlCombo.GetString());
    if (!CTSVNPath(m_url).IsValidOnWindows())
    {
        CString sInfo;
        sInfo.Format(IDS_WARN_NOVALIDPATH_TASK1, static_cast<LPCWSTR>(m_url));
        CTaskDialog taskDlg(sInfo,
                            CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK5)));
        taskDlg.SetDefaultCommandControl(200);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        if (taskDlg.DoModal(m_hWnd) != 100)
            return;
    }
    CStringUtils::WriteAsciiStringToClipboard(m_url);

    // now let the bugtraq plugin check the commit message
    CComPtr<IBugTraqProvider2> pProvider2 = nullptr;
    if (m_bugTraqProvider)
    {
        HRESULT hr = m_bugTraqProvider.QueryInterface(&pProvider2);
        if (SUCCEEDED(hr))
        {
            ATL::CComBSTR temp;
            ATL::CComBSTR sourceURL;
            sourceURL.Attach(m_wcURL.AllocSysString());
            ATL::CComBSTR parameters;
            parameters.Attach(m_bugtraqAssociation.GetParameters().AllocSysString());
            ATL::CComBSTR targetURL;
            targetURL.Attach(m_url.AllocSysString());
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
                CString sError = temp == 0 ? L"" : static_cast<LPCWSTR>(temp);
                if (!sError.IsEmpty())
                {
                    CAppUtils::ReportFailedHook(m_hWnd, sError);
                    return;
                }
            }
        }
    }

    CTSVNPathList checkedItems;
    checkedItems.AddPath(m_path);
    DWORD   exitCode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(m_path, m_projectProperties);
    if (CHooks::Instance().CheckCommit(m_hWnd, checkedItems, m_sLogMessage, exitCode, error))
    {
        if (exitCode)
        {
            CString sErrorMsg;
            sErrorMsg.Format(IDS_HOOK_ERRORMSG, static_cast<LPCWSTR>(error));

            CTaskDialog taskDlg(sErrorMsg,
                                CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
            taskDlg.SetDefaultCommandControl(100);
            taskDlg.SetMainIcon(TD_ERROR_ICON);
            bool retry = (taskDlg.DoModal(GetSafeHwnd()) == 100);

            if (retry)
                return;
        }
    }

    if (!m_sLogMessage.IsEmpty())
    {
        m_history.AddEntry(m_sLogMessage);
        m_history.Save();
    }

    m_sBugID.Trim();
    if (!m_sBugID.IsEmpty())
    {
        m_sBugID.Replace(L", ", L",");
        m_sBugID.Replace(L" ,", L",");
        CString sBugID = m_projectProperties.sMessage;
        sBugID.Replace(L"%BUGID%", m_sBugID);
        if (m_projectProperties.bAppend)
            m_sLogMessage += L"\n" + sBugID + L"\n";
        else
            m_sLogMessage = sBugID + L"\n" + m_sLogMessage;
        UpdateData(FALSE);
    }
    CResizableStandAloneDialog::OnOK();
}

void CCopyDlg::OnBnClickedBugtraqbutton()
{
    m_tooltips.Pop(); // hide the tooltips
    CString sMsg = m_cLogMessage.GetText();

    if (m_bugTraqProvider == nullptr)
        return;

    ATL::CComBSTR parameters;
    parameters.Attach(m_bugtraqAssociation.GetParameters().AllocSysString());
    ATL::CComBSTR commonRoot;
    commonRoot.Attach(m_path.GetWinPathString().AllocSysString());
    CBstrSafeVector pathList(1);
    pathList.PutElement(0, m_path.GetSVNPathString());

    ATL::CComBSTR originalMessage;
    originalMessage.Attach(sMsg.AllocSysString());
    ATL::CComBSTR temp;
    m_revProps.clear();

    // first try the IBugTraqProvider2 interface
    CComPtr<IBugTraqProvider2> pProvider2 = nullptr;
    HRESULT                    hr         = m_bugTraqProvider.QueryInterface(&pProvider2);
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
            m_cLogMessage.SetText(static_cast<LPCWSTR>(temp));
            BSTR *pbRevNames;
            BSTR *pbRevValues;

            HRESULT hr1 = SafeArrayAccessData(revPropNames, reinterpret_cast<void **>(&pbRevNames));
            if (SUCCEEDED(hr1))
            {
                HRESULT hr2 = SafeArrayAccessData(revPropValues, reinterpret_cast<void **>(&pbRevValues));
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
        CComPtr<IBugTraqProvider> pProvider = nullptr;
        hr                                  = m_bugTraqProvider.QueryInterface(&pProvider);
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
            m_cLogMessage.SetText(static_cast<LPCWSTR>(temp));
    }
    m_sLogMessage = m_cLogMessage.GetText();
    if (!m_projectProperties.sMessage.IsEmpty())
    {
        CString sBugID = m_projectProperties.FindBugID(m_sLogMessage);
        if (!sBugID.IsEmpty())
        {
            SetDlgItemText(IDC_BUGID, sBugID);
        }
    }

    m_cLogMessage.SetFocus();
}

void CCopyDlg::OnComError(HRESULT hr) const
{
    COMError ce(hr);
    CString  sErr;
    sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, static_cast<LPCWSTR>(m_bugtraqAssociation.GetProviderName()), ce.GetMessageAndDescription().c_str());
    ::MessageBox(m_hWnd, sErr, L"TortoiseSVN", MB_ICONERROR);
}

void CCopyDlg::OnBnClickedBrowse()
{
    m_tooltips.Pop(); // hide the tooltips
    SVNRev rev = SVNRev::REV_HEAD;

    CAppUtils::BrowseRepository(m_repoRoot, m_urlCombo, this, rev);
}

void CCopyDlg::OnBnClickedHelp()
{
    m_tooltips.Pop(); // hide the tooltips
    OnHelp();
}

void CCopyDlg::OnCancel()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }
    m_bCancelled = true;
    // check if the status thread has already finished
    if ((m_pThread) && (m_bThreadRunning))
    {
        WaitForSingleObject(m_pThread->m_hThread, 1000);
        if (m_bThreadRunning)
        {
            // we gave the thread a chance to quit. Since the thread didn't
            // listen to us we have to kill it.
            TerminateThread(m_pThread->m_hThread, static_cast<DWORD>(-1));
            delete m_pThread;
            InterlockedExchange(&m_bThreadRunning, FALSE);
        }
    }
    if (m_projectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEBRANCH).Compare(m_cLogMessage.GetText()) != 0)
    {
        CString sMsg = m_cLogMessage.GetText();
        if (!sMsg.IsEmpty())
        {
            m_history.AddEntry(sMsg);
            m_history.Save();
        }
    }
    CResizableStandAloneDialog::OnCancel();
}

BOOL CCopyDlg::PreTranslateMessage(MSG *pMsg)
{
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
    m_tooltips.Pop(); // hide the tooltips
    UpdateData(TRUE);
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
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
        m_pLogDlg->m_wParam        = 0;
        m_pLogDlg->m_pNotifyWindow = this;
    }
    AfxGetApp()->DoWaitCursor(-1);
}

void CCopyDlg::OnEnChangeLogmessage()
{
    CString sTemp = m_cLogMessage.GetText();
    DialogEnableWindow(IDOK, sTemp.GetLength() >= m_projectProperties.nMinLogSize);
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
    m_tooltips.Pop(); // hide the tooltips
    m_bSettingChanged = true;
}

void CCopyDlg::OnBnClickedCopyrev()
{
    m_tooltips.Pop(); // hide the tooltips
    m_bSettingChanged = true;
}

void CCopyDlg::OnBnClickedCopywc()
{
    m_tooltips.Pop(); // hide the tooltips
    m_bSettingChanged = true;
}

void CCopyDlg::OnBnClickedHistory()
{
    m_tooltips.Pop(); // hide the tooltips
    SVN         svn;
    CHistoryDlg historyDlg;
    historyDlg.SetHistory(m_history);
    if (historyDlg.DoModal() == IDOK)
    {
        CString sMsg = historyDlg.GetSelectedText();

        if (sMsg.Compare(m_cLogMessage.GetText().Left(sMsg.GetLength())) != 0)
        {
            CString sBugID = m_projectProperties.FindBugID(sMsg);
            if ((!sBugID.IsEmpty()) && ((GetDlgItem(IDC_BUGID)->IsWindowVisible())))
            {
                SetDlgItemText(IDC_BUGID, sBugID);
            }
            if (m_projectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEBRANCH).Compare(m_cLogMessage.GetText()) != 0)
                m_cLogMessage.InsertText(sMsg, !m_cLogMessage.GetText().IsEmpty());
            else
                m_cLogMessage.SetText(sMsg);
        }
        DialogEnableWindow(IDOK, m_projectProperties.nMinLogSize <= m_cLogMessage.GetText().GetLength());
    }
}

LPARAM CCopyDlg::OnRevFound(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // we have found the highest last-committed revision
    // in the working copy
    if ((!m_bSettingChanged) && (m_maxRev != 0) && (!m_bCancelled))
    {
        // we only change the setting automatically if the user hasn't done so
        // already him/herself, if the highest revision is valid. And of course,
        // if the thread hasn't been stopped forcefully.
        if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
        {
            if (m_bModified)
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
                temp.Format(L"%ld", m_maxRev);
                SetDlgItemText(IDC_COPYREVTEXT, temp);
                CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
            }
        }
    }

    if (m_externals.size())
        m_extList.ClearText();
    else
        m_extList.ShowText(CString(MAKEINTRESOURCE(IDS_COPY_NOEXTERNALSFOUND)));

    m_extList.SetRedraw(false);
    m_extList.DeleteAllItems();

    int c = m_extList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_extList.DeleteColumn(c--);
    CString temp;
    temp.LoadString(IDS_STATUSLIST_COLFILE);
    m_extList.InsertColumn(0, temp);
    temp.LoadString(IDS_STATUSLIST_COLURL);
    m_extList.InsertColumn(1, temp);
    temp.LoadString(IDS_COPYEXTLIST_COLTAGGED);
    m_extList.InsertColumn(2, temp, LVCFMT_RIGHT);
    m_extList.SetItemCountEx(static_cast<int>(m_externals.size()));

    CRect rect;
    m_extList.GetClientRect(&rect);
    int cx = (rect.Width() - 180) / 2;
    m_extList.SetColumnWidth(0, cx);
    m_extList.SetColumnWidth(1, cx);
    m_extList.SetColumnWidth(2, 80);

    m_extList.SetRedraw(true);
    DialogEnableWindow(IDC_EXTERNALSLIST, true);

    return 0;
}

void CCopyDlg::SetRevision(const SVNRev &rev)
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
        temp.Format(L"%ld", static_cast<LONG>(rev));
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
    auto urlA         = CUnicodeUtils::GetUTF8(m_urlCombo.GetWindowString());
    auto urlW         = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(urlA));
    SetDlgItemText(IDC_DESTURL, CPathUtils::CombineUrls(m_repoRoot, urlW));
}

void CCopyDlg::OnNMCustomdrawExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW *pLVCD = reinterpret_cast<NMLVCUSTOMDRAW *>(pNMHDR);
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_extList.HasText())
        return;

    // Draw externals that are already tagged to a specific revision in gray.
    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYSUBITEMDRAW;

        COLORREF crText = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
        if (pLVCD->nmcd.dwItemSpec < static_cast<UINT_PTR>(m_externals.size()))
        {
            SVNExternal ext = m_externals[pLVCD->nmcd.dwItemSpec];
            if ((ext.origRevision.kind == svn_opt_revision_number) || (ext.origRevision.kind == svn_opt_revision_date))
                crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
        }
        // Store the color back in the NMLVCUSTOMDRAW struct.
        pLVCD->clrText = crText;
    }
}

void CCopyDlg::OnLvnGetdispinfoExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO *>(pNMHDR);
    *pResult                = 0;

    if (m_extList.HasText())
        return;

    if (pDispInfo)
    {
        if (pDispInfo->item.iItem < static_cast<int>(m_externals.size()))
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
                        lstrcpyn(m_columnBuf, p.GetWinPath(), min(MAX_PATH - 2, pDispInfo->item.cchTextMax - 1));
                        int cWidth = m_extList.GetColumnWidth(0);
                        cWidth     = max(28, cWidth - 28);
                        CDC *pDC   = m_extList.GetDC();
                        if (pDC != nullptr)
                        {
                            CFont *pFont = pDC->SelectObject(m_extList.GetFont());
                            PathCompactPath(pDC->GetSafeHdc(), m_columnBuf, cWidth);
                            pDC->SelectObject(pFont);
                            ReleaseDC(pDC);
                        }
                    }
                    break;
                    case 1: // url
                    {
                        lstrcpyn(m_columnBuf, ext.url, min(MAX_PATH - 2, pDispInfo->item.cchTextMax - 1));
                        SVNRev peg(ext.pegRevision);
                        if (peg.IsValid() && !peg.IsHead())
                        {
                            wcscat_s(m_columnBuf, L"@");
                            wcscat_s(m_columnBuf, peg.ToString());
                        }
                        int cWidth = m_extList.GetColumnWidth(1);
                        cWidth     = max(14, cWidth - 14);
                        CDC *pDC   = m_extList.GetDC();
                        if (pDC != nullptr)
                        {
                            CFont *pFont = pDC->SelectObject(m_extList.GetFont());
                            PathCompactPath(pDC->GetSafeHdc(), m_columnBuf, cWidth);
                            pDC->SelectObject(pFont);
                            ReleaseDC(pDC);
                        }
                    }
                    break;
                    case 2: // tagged
                        m_columnBuf[0] = 0;
                        if (ext.origRevision.kind == svn_opt_revision_number)
                            swprintf_s(m_columnBuf, L"%ld", ext.origRevision.value.number);
                        else if (ext.origRevision.kind == svn_opt_revision_date)
                        {
                            SVNRev r(ext.origRevision);
                            wcscpy_s(m_columnBuf, static_cast<LPCWSTR>(r.ToString()));
                        }
                        break;
                    default:
                        m_columnBuf[0] = 0;
                }
                pDispInfo->item.pszText = m_columnBuf;
            }
        }
    }
}

void CCopyDlg::OnLvnKeydownExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVKEYDOWN pLvKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
    *pResult                 = 0;

    if (m_extList.HasText())
        return;

    if (pLvKeyDown->wVKey == VK_SPACE)
    {
        int nFocusedItem = m_extList.GetNextItem(-1, LVNI_FOCUSED);
        if (nFocusedItem >= 0)
            ToggleCheckbox(nFocusedItem);
    }
}

void CCopyDlg::OnNMClickExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    *pResult                         = 0;

    if (m_extList.HasText())
        return;

    UINT   flags = 0;
    CPoint point(pNMItemActivate->ptAction);
    // Make the hit test...
    int item = m_extList.HitTest(point, &flags);

    if (item != -1)
    {
        // We hit one item... did we hit state image (check box)?
        if ((flags & LVHT_ONITEMSTATEICON) != 0)
        {
            ToggleCheckbox(item);
        }
    }
}

void CCopyDlg::ToggleCheckbox(int index)
{
    SVNExternal ext    = m_externals[index];
    ext.adjust         = !ext.adjust;
    m_externals[index] = ext;
    m_extList.RedrawItems(index, index);
    m_extList.UpdateWindow();
}

LRESULT CCopyDlg::OnCheck(WPARAM wnd, LPARAM)
{
    HWND hwnd  = reinterpret_cast<HWND>(wnd);
    bool check = false;
    if (hwnd == GetDlgItem(IDC_CHECKALL)->GetSafeHwnd())
        check = true;

    for (std::vector<SVNExternal>::iterator it = m_externals.begin(); it != m_externals.end(); ++it)
    {
        it->adjust = check;
    }
    m_extList.RedrawItems(0, m_extList.GetItemCount());
    m_extList.UpdateWindow();

    return 0;
}

BOOL CCopyDlg::OnQueryEndSession()
{
    if (!__super::OnQueryEndSession())
        return FALSE;

    OnCancel();

    return TRUE;
}
