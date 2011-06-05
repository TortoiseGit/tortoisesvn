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
#include "AppUtils.h"
#include "RevisionRangeDlg.h"


IMPLEMENT_DYNAMIC(CRevisionRangeDlg, CStandAloneDialog)

CRevisionRangeDlg::CRevisionRangeDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CRevisionRangeDlg::IDD, pParent)
    , m_bAllowWCRevs(true)
    , m_StartRev(_T("HEAD"))
    , m_EndRev(_T("HEAD"))
{
}

CRevisionRangeDlg::~CRevisionRangeDlg()
{
}

void CRevisionRangeDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_REVNUM, m_sStartRevision);
    DDX_Text(pDX, IDC_REVNUM2, m_sEndRevision);
    DDX_Control(pDX, IDC_REVDATE, m_DateFrom);
    DDX_Control(pDX, IDC_REVDATE2, m_DateTo);
}


BEGIN_MESSAGE_MAP(CRevisionRangeDlg, CStandAloneDialog)
    ON_EN_CHANGE(IDC_REVNUM, OnEnChangeRevnum)
    ON_EN_CHANGE(IDC_REVNUM2, OnEnChangeRevnum2)
    ON_NOTIFY(DTN_DATETIMECHANGE, IDC_REVDATE, OnDtnDatetimechangeDatefrom)
    ON_NOTIFY(DTN_DATETIMECHANGE, IDC_REVDATE2, OnDtnDatetimechangeDateto)
END_MESSAGE_MAP()

BOOL CRevisionRangeDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_ENDREVGROUP);
    m_aeroControls.SubclassOkCancel(this);

    if (m_StartRev.IsHead())
    {
        CheckRadioButton(IDC_NEWEST, IDC_DATEREV, IDC_NEWEST);
    }
    else
    {
        CString sRev;
        if (m_StartRev.IsDate())
        {
            CheckRadioButton(IDC_NEWEST, IDC_DATEREV, IDC_DATEREV);
            sRev = m_StartRev.GetDateString();
        }
        else
        {
            CheckRadioButton(IDC_NEWEST, IDC_DATEREV, IDC_REVISION_N);
            sRev.Format(_T("%ld"), (LONG)(m_StartRev));
        }
        if (!sRev.IsEmpty())
            SetDlgItemText(IDC_REVNUM, sRev);
    }
    if (m_EndRev.IsHead())
    {
        CheckRadioButton(IDC_NEWEST2, IDC_DATEREV2, IDC_NEWEST2);
    }
    else
    {
        CString sRev;
        if (m_EndRev.IsDate())
        {
            CheckRadioButton(IDC_NEWEST2, IDC_DATEREV2, IDC_DATEREV2);
            sRev = m_EndRev.GetDateString();
        }
        else
        {
            CheckRadioButton(IDC_NEWEST2, IDC_DATEREV2, IDC_REVISION_N2);
            sRev.Format(_T("%ld"), (LONG)(m_EndRev));
        }
        if (!sRev.IsEmpty())
            SetDlgItemText(IDC_REVNUM2, sRev);
    }

    m_DateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);
    m_DateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);

    if ((m_pParentWnd==NULL)&&(GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    GetDlgItem(IDC_REVNUM)->SetFocus();
    return FALSE;
}

void CRevisionRangeDlg::OnOK()
{
    if (!UpdateData(TRUE))
        return; // don't dismiss dialog (error message already shown by MFC framework)

    m_StartRev = SVNRev(m_sStartRevision);
    if (GetCheckedRadioButton(IDC_NEWEST, IDC_DATEREV) == IDC_NEWEST)
    {
        m_StartRev = SVNRev(_T("HEAD"));
        m_sStartRevision = _T("HEAD");
    }
    if (GetCheckedRadioButton(IDC_NEWEST, IDC_DATEREV) == IDC_DATEREV)
    {
        CTime _time;
        m_DateFrom.GetTime(_time);
        try
        {
            CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 0, 0, 0);
            m_sStartRevision = time.FormatGmt(_T("{%Y-%m-%d}"));
            m_StartRev = SVNRev(m_sStartRevision);
        }
        catch (CAtlException)
        {
        }
    }
    if ((!m_StartRev.IsValid())||((!m_bAllowWCRevs)&&(m_StartRev.IsPrev() || m_StartRev.IsCommitted() || m_StartRev.IsBase())))
    {
        ShowEditBalloon(IDC_REVNUM, m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    m_EndRev = SVNRev(m_sEndRevision);
    if (GetCheckedRadioButton(IDC_NEWEST2, IDC_DATEREV2) == IDC_NEWEST2)
    {
        m_EndRev = SVNRev(_T("HEAD"));
        m_sEndRevision = _T("HEAD");
    }
    if (GetCheckedRadioButton(IDC_NEWEST2, IDC_DATEREV2) == IDC_DATEREV2)
    {
        CTime _time;
        m_DateTo.GetTime(_time);
        try
        {
            CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 23, 59, 59);
            m_sEndRevision = time.FormatGmt(_T("{%Y-%m-%d}"));
            m_EndRev = SVNRev(m_sEndRevision);
        }
        catch (CAtlException)
        {
        }
    }
    if ((!m_EndRev.IsValid())||((!m_bAllowWCRevs)&&(m_EndRev.IsPrev() || m_EndRev.IsCommitted() || m_EndRev.IsBase())))
    {
        ShowEditBalloon(IDC_REVNUM2, m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    UpdateData(FALSE);

    CStandAloneDialog::OnOK();
}

void CRevisionRangeDlg::OnEnChangeRevnum()
{
    CString sText;
    GetDlgItemText(IDC_REVNUM, sText);
    if (sText.IsEmpty())
    {
        CheckRadioButton(IDC_NEWEST, IDC_DATEREV, IDC_NEWEST);
    }
    else
    {
        CheckRadioButton(IDC_NEWEST, IDC_DATEREV, IDC_REVISION_N);
    }
}

void CRevisionRangeDlg::OnEnChangeRevnum2()
{
    CString sText;
    GetDlgItemText(IDC_REVNUM2, sText);
    if (sText.IsEmpty())
    {
        CheckRadioButton(IDC_NEWEST2, IDC_DATEREV2, IDC_NEWEST2);
    }
    else
    {
        CheckRadioButton(IDC_NEWEST2, IDC_DATEREV2, IDC_REVISION_N2);
    }
}

void CRevisionRangeDlg::OnDtnDatetimechangeDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    CheckRadioButton(IDC_NEWEST2, IDC_DATEREV2, IDC_DATEREV2);

    *pResult = 0;
}

void CRevisionRangeDlg::OnDtnDatetimechangeDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    CheckRadioButton(IDC_NEWEST, IDC_DATEREV, IDC_DATEREV);

    *pResult = 0;
}
