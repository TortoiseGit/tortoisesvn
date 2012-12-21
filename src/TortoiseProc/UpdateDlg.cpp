// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007,2009-2012 - TortoiseSVN

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
#include "UpdateDlg.h"
#include "registry.h"
#include "LogDialog\LogDlg.h"
#include "AppUtils.h"
#include "RepositoryBrowser.h"

IMPLEMENT_DYNAMIC(CUpdateDlg, CStandAloneDialog)
CUpdateDlg::CUpdateDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CUpdateDlg::IDD, pParent)
    , Revision(_T("HEAD"))
    , m_bNoExternals(FALSE)
    , m_bStickyDepth(TRUE)
    , m_pLogDlg(NULL)
    , m_depth(svn_depth_unknown)
{
}

CUpdateDlg::~CUpdateDlg()
{
    delete m_pLogDlg;
}

void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_REVNUM, m_sRevision);
    DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
    DDX_Check(pDX, IDC_STICKYDEPTH, m_bStickyDepth);
    DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
}


BEGIN_MESSAGE_MAP(CUpdateDlg, CStandAloneDialog)
    ON_BN_CLICKED(IDC_LOG, OnBnClickedShowLog)
    ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
    ON_EN_CHANGE(IDC_REVNUM, &CUpdateDlg::OnEnChangeRevnum)
    ON_CBN_SELCHANGE(IDC_DEPTH, &CUpdateDlg::OnCbnSelchangeDepth)
    ON_BN_CLICKED(IDC_SPARSE, &CUpdateDlg::OnBnClickedSparse)
END_MESSAGE_MAP()

BOOL CUpdateDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_GROUPMISC);
    m_aeroControls.SubclassOkCancel(this);

    AdjustControlSize(IDC_REVISION_HEAD);
    AdjustControlSize(IDC_REVISION_N);
    AdjustControlSize(IDC_NOEXTERNALS);

    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
    if (m_wcPath.Exists())
        m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EXCLUDE)));
    m_depthCombo.SetCurSel(0);

    CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_wcPath.GetUIPathString(), sWindowTitle);

    DialogEnableWindow(IDC_SPARSE, m_wcPath.Exists() && m_wcPath.IsDirectory());

    GetDlgItem(IDC_REVNUM)->SetFocus();
    if ((m_pParentWnd==NULL)&&(GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    return FALSE;
}

void CUpdateDlg::OnCancel()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    CStandAloneDialog::OnCancel();
}

void CUpdateDlg::OnOK()
{
    if (!UpdateData(TRUE))
        return; // don't dismiss dialog (error message already shown by MFC framework)

    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return;
    }

    Revision = SVNRev(m_sRevision);
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        Revision = SVNRev(_T("HEAD"));
    }
    if (!Revision.IsValid())
    {
        ShowEditBalloon(IDC_REVNUM, IDS_ERR_INVALIDREV, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    switch (m_depthCombo.GetCurSel())
    {
    case 0:
        m_depth = svn_depth_unknown;
        m_checkoutDepths.clear();
        break;
    case 1:
        m_depth = svn_depth_infinity;
        m_checkoutDepths.clear();
        break;
    case 2:
        m_depth = svn_depth_immediates;
        m_checkoutDepths.clear();
        break;
    case 3:
        m_depth = svn_depth_files;
        m_checkoutDepths.clear();
        break;
    case 4:
        m_depth = svn_depth_empty;
        m_checkoutDepths.clear();
        break;
    case 5:
        m_depth = svn_depth_exclude;
        m_checkoutDepths.clear();
        break;
    default:
        m_depth = svn_depth_empty;
        break;
    }

    UpdateData(FALSE);

    CStandAloneDialog::OnOK();
}

void CUpdateDlg::OnBnClickedShowLog()
{
    UpdateData(TRUE);
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
        return;
    if (!m_wcPath.IsEmpty())
    {
        delete m_pLogDlg;
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->m_wParam = 0;
        m_pLogDlg->SetParams(m_wcPath, SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, TRUE);
        m_pLogDlg->ContinuousSelection(true);
        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
    AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CUpdateDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
    CString temp;
    temp.Format(_T("%ld"), lParam);
    SetDlgItemText(IDC_REVNUM, temp);
    CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    return 0;
}

void CUpdateDlg::OnEnChangeRevnum()
{
    UpdateData();
    if (m_sRevision.IsEmpty())
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    else
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CUpdateDlg::OnCbnSelchangeDepth()
{
    if (m_depthCombo.GetCurSel() == 5)  // svn_depth_exclude
    {
        // for exclude depths, the update must be sticky or it will fail
        // because it would be a no-op.
        m_bStickyDepth = TRUE;
        CheckDlgButton(IDC_STICKYDEPTH, BST_CHECKED);
    }
}


void CUpdateDlg::OnBnClickedSparse()
{
    UpdateData();

    CString strURLs;

    CTSVNPathList paths;
    paths.LoadFromAsteriskSeparatedString (strURLs);

    SVN svn;
    CString strUrl = svn.GetURLFromPath(m_wcPath);

    CRepositoryBrowser browser(strUrl, SVNRev::REV_HEAD, this);
    browser.SetSparseCheckoutMode(m_wcPath);
    if (browser.DoModal() == IDOK)
    {
        m_checkoutDepths = browser.GetUpdateDepths();
        CString sCustomDepth = CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_CUSTOM));
        int customIndex = m_depthCombo.FindStringExact(-1, sCustomDepth);
        if (customIndex == CB_ERR)
        {
            customIndex = m_depthCombo.AddString(sCustomDepth);
        }
        m_depthCombo.SetCurSel(customIndex);
    }
}
