// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "RevisionDlg.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "LogDialog\LogDlg.h"

IMPLEMENT_DYNAMIC(CRevisionDlg, CStandAloneDialog)
CRevisionDlg::CRevisionDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CRevisionDlg::IDD, pParent)
    , SVNRev(_T("HEAD"))
    , m_bAllowWCRevs(true)
{
}

CRevisionDlg::~CRevisionDlg()
{
}

void CRevisionDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_REVNUM, m_sRevision);
}


BEGIN_MESSAGE_MAP(CRevisionDlg, CStandAloneDialog)
    ON_EN_CHANGE(IDC_REVNUM, OnEnChangeRevnum)
    ON_BN_CLICKED(IDC_LOG, &CRevisionDlg::OnBnClickedLog)
    ON_BN_CLICKED(IDC_REVISION_N, &CRevisionDlg::OnBnClickedRevisionN)
END_MESSAGE_MAP()

BOOL CRevisionDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_REVGROUP);
    m_aeroControls.SubclassOkCancel(this);

    if (IsHead())
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    }
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
        CString sRev;
        if (IsDate())
            sRev = GetDateString();
        else
            sRev.Format(_T("%ld"), (LONG)(*this));
        SetDlgItemText(IDC_REVNUM, sRev);
    }
    if (!m_logPath.IsEmpty())
        GetDlgItem(IDC_LOG)->ShowWindow(SW_SHOW);

    if ((m_pParentWnd==NULL)&&(GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    GetDlgItem(IDC_REVNUM)->SetFocus();
    return FALSE;
}

void CRevisionDlg::OnOK()
{
    if (!UpdateData(TRUE))
        return; // don't dismiss dialog (error message already shown by MFC framework)

    SVNRev::Create(m_sRevision);
    // if head revision, set revision as -1
    if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
    {
        SVNRev::Create(_T("HEAD"));
        m_sRevision = _T("HEAD");
    }
    if ((!IsValid())||((!m_bAllowWCRevs)&&(IsPrev() || IsCommitted() || IsBase())))
    {
        ShowEditBalloon(IDC_REVNUM, m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    UpdateData(FALSE);

    CStandAloneDialog::OnOK();
}

void CRevisionDlg::OnEnChangeRevnum()
{
    CString sText;
    GetDlgItemText(IDC_REVNUM, sText);
    if (sText.IsEmpty())
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
    }
    else
    {
        CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
    }
}

void CRevisionDlg::SetLogPath(const CTSVNPath& path, const SVNRev& rev /* = SVNRev::REV_HEAD */)
{
    m_logPath = path;
    m_logRev = rev;
}

void CRevisionDlg::OnBnClickedLog()
{
    CLogDlg dlg;
    dlg.SetParams (m_logPath, SVNRev::REV_HEAD, m_logRev, 1);
    dlg.SetSelect (true);
    if (dlg.DoModal() == IDOK)
    {
        m_logRev = dlg.GetSelectedRevRanges().GetHighestRevision();
        SetDlgItemText (IDC_REVNUM, m_logRev.ToString());
    }
}

void CRevisionDlg::OnBnClickedRevisionN()
{
    GetDlgItem(IDC_REVNUM)->SetFocus();
}
