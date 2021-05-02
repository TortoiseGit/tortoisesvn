// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2014, 2021 - TortoiseSVN

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
    , m_startRev(L"HEAD")
    , m_endRev(L"HEAD")
    , m_bAllowWCRevs(true)
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
    DDX_Control(pDX, IDC_REVDATE, m_dateFrom);
    DDX_Control(pDX, IDC_REVDATE2, m_dateTo);
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

    if (m_startRev.IsHead())
    {
        CheckRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE, IDC_REVRANGE_HEAD);
    }
    else
    {
        CString sRev;
        if (m_startRev.IsDate())
        {
            CheckRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE, IDC_REVRANGE_DATE);
            sRev = m_startRev.GetDateString();
        }
        else
        {
            CheckRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE, IDC_REVRANGE_REV);
            sRev.Format(L"%ld", static_cast<LONG>(m_startRev));
        }
        if (!sRev.IsEmpty())
            SetDlgItemText(IDC_REVNUM, sRev);
    }
    if (m_endRev.IsHead())
    {
        CheckRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2, IDC_REVRANGE_HEAD2);
    }
    else
    {
        CString sRev;
        if (m_endRev.IsDate())
        {
            CheckRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2, IDC_REVRANGE_DATE2);
            sRev = m_endRev.GetDateString();
        }
        else
        {
            CheckRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2, IDC_REVRANGE_REV2);
            sRev.Format(L"%ld", static_cast<LONG>(m_endRev));
        }
        if (!sRev.IsEmpty())
            SetDlgItemText(IDC_REVNUM2, sRev);
    }

    m_dateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS | MCS_NOTODAY | MCS_NOTRAILINGDATES | MCS_NOSELCHANGEONNAV);
    m_dateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS | MCS_NOTODAY | MCS_NOTRAILINGDATES | MCS_NOSELCHANGEONNAV);

    if ((m_pParentWnd == nullptr) && (GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    GetDlgItem(IDC_REVNUM)->SetFocus();
    return FALSE;
}

void CRevisionRangeDlg::OnOK()
{
    if (!UpdateData(TRUE))
        return; // don't dismiss dialog (error message already shown by MFC framework)

    m_startRev = SVNRev(m_sStartRevision);
    if (GetCheckedRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE) == IDC_REVRANGE_HEAD)
    {
        m_startRev       = SVNRev(L"HEAD");
        m_sStartRevision = L"HEAD";
    }
    if (GetCheckedRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE) == IDC_REVRANGE_DATE)
    {
        CTime cTime;
        m_dateFrom.GetTime(cTime);
        try
        {
            CTime time(cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(), 0, 0, 0);
            m_sStartRevision = time.FormatGmt(L"{%Y-%m-%d}");
            m_startRev       = SVNRev(m_sStartRevision);
        }
        catch (CAtlException)
        {
        }
    }
    if ((!m_startRev.IsValid()) || ((!m_bAllowWCRevs) && (m_startRev.IsPrev() || m_startRev.IsCommitted() || m_startRev.IsBase())))
    {
        ShowEditBalloon(IDC_REVNUM, m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    m_endRev = SVNRev(m_sEndRevision);
    if (GetCheckedRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2) == IDC_REVRANGE_HEAD2)
    {
        m_endRev       = SVNRev(L"HEAD");
        m_sEndRevision = L"HEAD";
    }
    if (GetCheckedRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2) == IDC_REVRANGE_DATE2)
    {
        CTime cTime;
        m_dateTo.GetTime(cTime);
        try
        {
            CTime time(cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(), 23, 59, 59);
            m_sEndRevision = time.FormatGmt(L"{%Y-%m-%d}");
            m_endRev       = SVNRev(m_sEndRevision);
        }
        catch (CAtlException)
        {
        }
    }
    if ((!m_endRev.IsValid()) || ((!m_bAllowWCRevs) && (m_endRev.IsPrev() || m_endRev.IsCommitted() || m_endRev.IsBase())))
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
        CheckRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE, IDC_REVRANGE_HEAD);
    }
    else
    {
        CheckRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE, IDC_REVRANGE_REV);
    }
}

void CRevisionRangeDlg::OnEnChangeRevnum2()
{
    CString sText;
    GetDlgItemText(IDC_REVNUM2, sText);
    if (sText.IsEmpty())
    {
        CheckRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2, IDC_REVRANGE_HEAD2);
    }
    else
    {
        CheckRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2, IDC_REVRANGE_REV2);
    }
}

void CRevisionRangeDlg::OnDtnDatetimechangeDateto(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    CheckRadioButton(IDC_REVRANGE_HEAD2, IDC_REVRANGE_DATE2, IDC_REVRANGE_DATE2);

    *pResult = 0;
}

void CRevisionRangeDlg::OnDtnDatetimechangeDatefrom(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    CheckRadioButton(IDC_REVRANGE_HEAD, IDC_REVRANGE_DATE, IDC_REVRANGE_DATE);

    *pResult = 0;
}
