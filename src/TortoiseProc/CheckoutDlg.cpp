// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2020-2021 - TortoiseSVN

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
#include "CheckoutDlg.h"
#include "RepositoryBrowser.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "AppUtils.h"
#include "SVNInfo.h"

IMPLEMENT_DYNAMIC(CCheckoutDlg, CResizableStandAloneDialog)
CCheckoutDlg::CCheckoutDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CCheckoutDlg::IDD, pParent)
    , m_bAutoCreateTargetName(false)
    , m_bBlockMessages(false)
    , m_revision(L"HEAD")
    , m_bNoExternals(CRegDWORD(L"Software\\TortoiseSVN\\noext"))
    , m_bIndependentWCs(FALSE)
    , m_pLogDlg(nullptr)
    , m_depth(svn_depth_unknown)
    , m_blockPathAdjustments(FALSE)
    , m_standardCheckout(true)
    , m_parentExists(false)
{
}

CCheckoutDlg::~CCheckoutDlg()
{
    delete m_pLogDlg;
}

void CCheckoutDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_urlCombo);
    DDX_Control(pDX, IDC_REVISION_NUM, m_editRevision);
    DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
    DDX_Text(pDX, IDC_REVISION_NUM, m_sRevision);
    DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strCheckoutDirectory);
    DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
    DDX_Check(pDX, IDC_INDEPENDENTWCS, m_bIndependentWCs);
    DDX_Control(pDX, IDC_CHECKOUTDIRECTORY, m_cCheckoutEdit);
    DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
}

BEGIN_MESSAGE_MAP(CCheckoutDlg, CResizableStandAloneDialog)
    ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
    ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
    ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_BN_CLICKED(IDC_SHOW_LOG, OnBnClickedShowlog)
    ON_EN_CHANGE(IDC_REVISION_NUM, &CCheckoutDlg::OnEnChangeRevisionNum)
    ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCheckoutDlg::OnCbnEditchangeUrlcombo)
    ON_CBN_SELCHANGE(IDC_DEPTH, &CCheckoutDlg::OnCbnSelchangeDepth)
    ON_BN_CLICKED(IDC_SPARSE, &CCheckoutDlg::OnBnClickedSparse)
END_MESSAGE_MAP()

void CCheckoutDlg::UpdateURLsFromCombo()
{
    // read URLs from combo

    CString text;
    m_urlCombo.GetWindowText(text);
    m_urLs.LoadFromAsteriskSeparatedString(text);

    // update "independent w/cs" option

    bool multiSelect = m_urLs.GetCount() > 1;
    DialogEnableWindow(IDC_INDEPENDENTWCS, multiSelect);
    if (!multiSelect)
        m_bIndependentWCs = FALSE;

    if (!m_bAutoCreateTargetName)
        return;

    // find out what to use as the checkout directory name

    if ((!m_sCheckoutDirOrig.IsEmpty()) && !m_blockPathAdjustments)
    {
        CString name = CAppUtils::GetProjectNameFromURL(m_urLs.GetCommonRoot().GetSVNPathString());
        if (!name.IsEmpty() && CPathUtils::GetFileNameFromPath(m_strCheckoutDirectory).CompareNoCase(name))
        {
            m_strCheckoutDirectory = m_sCheckoutDirOrig.TrimRight('\\') + '\\' + name;
        }
    }

    if ((m_strCheckoutDirectory.IsEmpty()) && !m_blockPathAdjustments)
    {
        CRegString lastCheckoutPath = CRegString(L"Software\\TortoiseSVN\\History\\lastCheckoutPath");
        m_strCheckoutDirectory      = lastCheckoutPath;
        if (m_strCheckoutDirectory.GetLength() <= 2)
            m_strCheckoutDirectory += L"\\";
    }
    m_strCheckoutDirectory.Replace(L":\\\\", L":\\");

    // update UI

    UpdateData(FALSE);
    DialogEnableWindow(IDOK, !m_strCheckoutDirectory.IsEmpty());
}

bool CCheckoutDlg::IsStandardCheckout()
{
    return (m_urLs.GetCount() == 1) && !SVNInfo::IsFile(m_urLs[0], m_revision);
}

void CCheckoutDlg::SetRevision(const SVNRev& rev)
{
    if (rev.IsHead() || !rev.IsValid())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
        CString sRev;
        sRev.Format(L"%ld", static_cast<LONG>(rev));
        SetDlgItemText(IDC_REVISION_NUM, sRev);
    }
}

BOOL CCheckoutDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_GROUPBOTTOM);
    m_aeroControls.SubclassOkCancelHelp(this);

    AdjustControlSize(IDC_NOEXTERNALS);
    AdjustControlSize(IDC_REVISION_HEAD);
    AdjustControlSize(IDC_REVISION_N);

    m_sCheckoutDirOrig = m_strCheckoutDirectory;

    CString sUrlSave = m_urLs.CreateAsteriskSeparatedString();
    m_urlCombo.SetURLHistory(true, true);
    m_bAutoCreateTargetName = FALSE;
    m_urlCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS", L"url");
    m_bAutoCreateTargetName = !(PathIsDirectoryEmpty(m_sCheckoutDirOrig) || !PathFileExists(m_sCheckoutDirOrig));
    m_urlCombo.SetCurSel(0);

    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
    m_depthCombo.SetCurSel(0);

    // set radio buttons according to the revision
    SetRevision(m_revision);

    m_editRevision.SetWindowText(L"");

    if (!sUrlSave.IsEmpty())
    {
        SetDlgItemText(IDC_CHECKOUTDIRECTORY, m_sCheckoutDirOrig);
        m_urlCombo.SetWindowText(sUrlSave);
    }
    else
    {
        // if there is an url on the clipboard, use that url as the default.
        CAppUtils::AddClipboardUrlToWindow(m_urlCombo.GetSafeHwnd());
    }

    m_tooltips.AddTool(IDC_CHECKOUTDIRECTORY, IDS_CHECKOUT_TT_DIR);
    m_tooltips.AddTool(IDC_INDEPENDENTWCS, IDS_CHECKOUT_TT_MULTIWC);

    SHAutoComplete(GetDlgItem(IDC_CHECKOUTDIRECTORY)->m_hWnd, SHACF_FILESYSTEM);

    if (!m_revision.IsHead())
    {
        CString temp;
        temp.Format(L"%ld", static_cast<LONG>(m_revision));
        m_editRevision.SetWindowText(temp);
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    }

    UpdateURLsFromCombo();

    CAppUtils::SetAccProperty(m_urlCombo.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+" + CString(CAppUtils::FindAcceleratorKey(this, IDC_URLOFREPO)));
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKOUTDIRECTORY)->GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+" + CString(CAppUtils::FindAcceleratorKey(this, IDC_EXPORT_CHECKOUTDIR)));

    AddAnchor(IDC_GROUPTOP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLOFREPO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_EXPORT_CHECKOUTDIR, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_CHECKOUTDIRECTORY, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_CHECKOUTDIRECTORY_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_INDEPENDENTWCS, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_GROUPMIDDLE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DEPTH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_NOEXTERNALS, TOP_LEFT);
    AddAnchor(IDC_SPARSE, TOP_RIGHT);
    AddAnchor(IDC_GROUPBOTTOM, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
    AddAnchor(IDC_REVISION_N, TOP_LEFT);
    AddAnchor(IDC_REVISION_NUM, TOP_LEFT);
    AddAnchor(IDC_SHOW_LOG, TOP_RIGHT);
    AddAnchor(IDOK, TOP_RIGHT);
    AddAnchor(IDCANCEL, TOP_RIGHT);
    AddAnchor(IDHELP, TOP_RIGHT);

    // prevent resizing vertically
    CRect rect;
    GetWindowRect(&rect);
    CSize size;
    size.cx = MAXLONG;
    size.cy = rect.Height();
    SetMaxTrackSize(size);

    SetTheme(CTheme::Instance().IsDarkTheme());

    if ((m_pParentWnd == nullptr) && (GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"CheckoutDlg");
    return TRUE;
}

void CCheckoutDlg::OnCancel()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    CResizableStandAloneDialog::OnCancel();
}

void CCheckoutDlg::OnOK()
{
    if (m_bBlockMessages)
        return;
    m_bBlockMessages = true;
    if (!UpdateData(TRUE))
    {
        m_bBlockMessages = false;
        return; // don't dismiss dialog (error message already shown by MFC framework)
    }
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        m_bBlockMessages = false;
        return;
    }

    // require a syntactically valid target path

    if (m_strCheckoutDirectory.IsEmpty())
    {
        m_bBlockMessages = false;
        return; //don't dismiss the dialog
    }

    CTSVNPath checkoutDirectory;
    if (::PathIsRelative(m_strCheckoutDirectory))
    {
        checkoutDirectory = CTSVNPath(sOrigCwd);
        checkoutDirectory.AppendPathString(L"\\" + m_strCheckoutDirectory);
        m_strCheckoutDirectory = checkoutDirectory.GetWinPathString();
    }
    else
        checkoutDirectory = CTSVNPath(m_strCheckoutDirectory);
    if (!checkoutDirectory.IsValidOnWindows())
    {
        ShowEditBalloon(IDC_CHECKOUTDIRECTORY, IDS_ERR_NOVALIDPATH, IDS_ERR_ERROR, TTI_ERROR);
        m_bBlockMessages = false;
        return;
    }

    // require a source revision
    m_revision = GetSelectedRevision();
    if (!m_revision.IsValid())
    {
        ShowEditBalloon(IDC_REVISION_NUM, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        m_bBlockMessages = false;
        return;
    }

    // require a syntactically valid source path

    bool bAutoCreateTargetName = m_bAutoCreateTargetName;
    m_bAutoCreateTargetName    = false;
    m_urlCombo.SaveHistory();

    m_urLs.LoadFromAsteriskSeparatedString(m_urlCombo.GetString());

    for (INT_PTR i = 0; i < m_urLs.GetCount(); ++i)
        if (!m_urLs[i].IsUrl())
        {
            m_tooltips.ShowBalloon(IDC_URLCOMBO, IDS_ERR_MUSTBEURL, IDS_ERR_ERROR, TTI_ERROR);
            m_bAutoCreateTargetName = bAutoCreateTargetName;
            m_bBlockMessages        = false;
            return;
        }

    // decode depth info

    switch (m_depthCombo.GetCurSel())
    {
        case 0:
            m_depth = svn_depth_infinity;
            m_checkoutDepths.clear();
            break;
        case 1:
            m_depth = svn_depth_immediates;
            m_checkoutDepths.clear();
            break;
        case 2:
            m_depth = svn_depth_files;
            m_checkoutDepths.clear();
            break;
        case 3:
            m_depth = svn_depth_empty;
            m_checkoutDepths.clear();
            break;
        default:
            m_depth = svn_depth_unknown;
            break;
    }

    // require the target path to be actually valid
    // - depending on whether it is a file

    m_standardCheckout = IsStandardCheckout();
    if (!m_standardCheckout)
    {
        CTSVNPath targetPath(m_strCheckoutDirectory);

        // don't try to overwrite existing folders with a file

        if (!PathFileExists(m_strCheckoutDirectory) || !targetPath.IsDirectory())
        {
            // the parent must exist

            targetPath             = targetPath.GetContainingDirectory();
            m_strCheckoutDirectory = targetPath.GetWinPathString();

            CPathUtils::MakeSureDirectoryPathExists(targetPath.GetWinPath());
        }

        // is it already a w/c for the directory we want?

        CString parentURL = m_urLs.GetCommonRoot().GetSVNPathString();

        SVNInfo            info;
        const SVNInfoData* infoData = info.GetFirstFileInfo(targetPath, SVNRev(), SVNRev());
        // exists with matching URL?

        m_parentExists = (infoData != nullptr) && (infoData->url == parentURL);
        if (!m_parentExists)
        {
            // trying to c/o into an existing, non-empty folder?

            if (!PathIsDirectoryEmpty(targetPath.GetWinPath()))
            {
                CString message;
                message.Format(CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY)), targetPath.GetWinPath());
                CTaskDialog taskDlg(message,
                                    CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK3)));
                taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK4)));
                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskDlg.SetDefaultCommandControl(2);
                taskDlg.SetMainIcon(TD_WARNING_ICON);
                bool doIt = (taskDlg.DoModal(GetExplorerHWND()) == 100);

                if (!doIt)
                {
                    m_bAutoCreateTargetName = bAutoCreateTargetName;
                    m_bBlockMessages        = false;
                    return; //don't dismiss the dialog
                }
            }
        }
    }
    else
    {
        // our default is that the target is a directory.
        // If it doesn't exist yet, we create it and it should be empty.

        if (!PathFileExists(m_strCheckoutDirectory))
        {
            CPathUtils::MakeSureDirectoryPathExists(m_strCheckoutDirectory);
        }
        if (!PathIsDirectoryEmpty(m_strCheckoutDirectory))
        {
            CString message;
            message.Format(CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY)), static_cast<LPCWSTR>(m_strCheckoutDirectory));
            CTaskDialog taskDlg(message,
                                CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY_TASK4)));
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetDefaultCommandControl(2);
            taskDlg.SetMainIcon(TD_WARNING_ICON);
            bool doIt = (taskDlg.DoModal(GetExplorerHWND()) == 100);

            if (!doIt)
            {
                m_bAutoCreateTargetName = bAutoCreateTargetName;
                m_bBlockMessages        = false;
                return; //don't dismiss the dialog
            }
        }
    }

    // store state info & close dialog

    UpdateData(FALSE);
    CRegString lastCheckoutPath(L"Software\\TortoiseSVN\\History\\lastCheckoutPath");
    lastCheckoutPath = m_strCheckoutDirectory.Left(m_strCheckoutDirectory.ReverseFind('\\'));

    if (m_depth == svn_depth_infinity)
    {
        CRegDWORD regNoExt(L"Software\\TortoiseSVN\\noext");
        regNoExt = m_bNoExternals;
    }

    CResizableStandAloneDialog::OnOK();
}

void CCheckoutDlg::OnBnClickedBrowse()
{
    m_tooltips.Pop(); // hide the tooltips
    UpdateData();

    SVNRev rev = GetSelectedRevisionOrHead();
    if (CAppUtils::BrowseRepository(m_urlCombo, this, rev, true))
    {
        SetRevision(rev);
        UpdateURLsFromCombo();
    }
}

void CCheckoutDlg::OnBnClickedCheckoutdirectoryBrowse()
{
    m_tooltips.Pop(); // hide the tooltips
    //
    // Create a folder browser dialog. If the user selects OK, we should update
    // the local data members with values from the controls, copy the checkout
    // directory from the browse folder, then restore the local values into the
    // dialog controls.
    //
    CBrowseFolder browseFolder;
    browseFolder.m_style         = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    CString strCheckoutDirectory = m_strCheckoutDirectory;
    if (browseFolder.Show(GetSafeHwnd(), strCheckoutDirectory) == CBrowseFolder::OK)
    {
        UpdateData(TRUE);
        m_strCheckoutDirectory  = strCheckoutDirectory;
        m_sCheckoutDirOrig      = m_strCheckoutDirectory;
        m_bAutoCreateTargetName = !(PathIsDirectoryEmpty(m_sCheckoutDirOrig) || !PathFileExists(m_sCheckoutDirOrig));
        UpdateData(FALSE);
        DialogEnableWindow(IDOK, !m_strCheckoutDirectory.IsEmpty());
    }
}

void CCheckoutDlg::OnEnChangeCheckoutdirectory()
{
    UpdateData(TRUE);
    DialogEnableWindow(IDOK, !m_strCheckoutDirectory.IsEmpty());
}

void CCheckoutDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CCheckoutDlg::OnBnClickedShowlog()
{
    m_tooltips.Pop(); // hide the tooltips
    UpdateData(TRUE);
    m_urLs.LoadFromAsteriskSeparatedString(m_urlCombo.GetString());
    if ((m_pLogDlg) && (m_pLogDlg->IsWindowVisible()))
        return;
    AfxGetApp()->DoWaitCursor(1);
    //now show the log dialog for working copy
    if (m_urLs.GetCount() > 0)
    {
        delete m_pLogDlg;
        m_pLogDlg = nullptr; // otherwise if the next line throws pointer will be left invalid
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetParams(m_urLs.GetCommonRoot(), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1);
        m_pLogDlg->m_wParam = 1;
        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
    AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CCheckoutDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
    CString temp;
    temp.Format(L"%Id", lParam);
    SetDlgItemText(IDC_REVISION_NUM, temp);
    CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    return 0;
}

void CCheckoutDlg::OnEnChangeRevisionNum()
{
    UpdateData();
    if (m_sRevision.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CCheckoutDlg::OnCbnEditchangeUrlcombo()
{
    // find out what to use as the checkout directory name
    UpdateData();
    UpdateURLsFromCombo();
}

void CCheckoutDlg::OnCbnSelchangeDepth()
{
    bool bOmitExternals = false;
    switch (m_depthCombo.GetCurSel())
    {
        case 0:
            //svn_depth_infinity
            bOmitExternals = false;
            break;
        default:
            bOmitExternals = true;
            break;
    }
    m_bNoExternals = bOmitExternals;
    UpdateData(FALSE);
    GetDlgItem(IDC_NOEXTERNALS)->EnableWindow(!bOmitExternals);
}

void CCheckoutDlg::OnBnClickedSparse()
{
    m_tooltips.Pop(); // hide the tooltips
    UpdateData();
    SVNRev rev = GetSelectedRevisionOrHead();

    CString strUrLs;
    m_urlCombo.GetWindowText(strUrLs);
    if (strUrLs.IsEmpty())
        strUrLs = m_urlCombo.GetString();
    strUrLs.Replace('\\', '/');
    strUrLs.Replace(L"%", L"%25");

    CTSVNPathList paths;
    paths.LoadFromAsteriskSeparatedString(strUrLs);

    CString            strUrl = paths.GetCommonRoot().GetSVNPathString();
    CRepositoryBrowser browser(strUrl, rev, this);
    browser.SetSparseCheckoutMode(CTSVNPath());
    if (browser.DoModal() == IDOK)
    {
        m_checkoutDepths     = browser.GetCheckoutDepths();
        CString sCustomDepth = CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_CUSTOM));
        int     customIndex  = m_depthCombo.FindStringExact(-1, sCustomDepth);
        if (customIndex == CB_ERR)
        {
            customIndex = m_depthCombo.AddString(sCustomDepth);
        }
        m_depthCombo.SetCurSel(customIndex);
    }
}

SVNRev CCheckoutDlg::GetSelectedRevision() const
{
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
        return SVNRev(L"HEAD");

    return SVNRev(m_sRevision);
}

SVNRev CCheckoutDlg::GetSelectedRevisionOrHead() const
{
    SVNRev rev = GetSelectedRevision();
    if (rev.IsValid())
        return rev;
    return SVNRev(SVNRev::REV_HEAD);
}
