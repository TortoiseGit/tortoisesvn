// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2012, 2014, 2020-2021 - TortoiseSVN

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
#include "RevGraphFilterDlg.h"

IMPLEMENT_DYNAMIC(CRevGraphFilterDlg, CStandAloneDialog)

CRevGraphFilterDlg::CRevGraphFilterDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CRevGraphFilterDlg::IDD, pParent)
    , m_removeSubTree(FALSE)
    , m_headRev(-1)
    , m_minRev(1)
    , m_maxRev(1)
{
}

CRevGraphFilterDlg::~CRevGraphFilterDlg()
{
}

void CRevGraphFilterDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_PATHFILTER, m_sFilterPaths);
    DDX_Check(pDX, IDC_REMOVESUBTREE, m_removeSubTree);
    DDX_Control(pDX, IDC_FROMSPIN, m_cFromSpin);
    DDX_Control(pDX, IDC_TOSPIN, m_cToSpin);
    DDX_Text(pDX, IDC_FROMREV, m_sFromRev);
    DDX_Text(pDX, IDC_TOREV, m_sToRev);
}

BEGIN_MESSAGE_MAP(CRevGraphFilterDlg, CStandAloneDialog)
    ON_NOTIFY(UDN_DELTAPOS, IDC_FROMSPIN, &CRevGraphFilterDlg::OnDeltaposFromspin)
    ON_NOTIFY(UDN_DELTAPOS, IDC_TOSPIN, &CRevGraphFilterDlg::OnDeltaposTospin)
END_MESSAGE_MAP()

BOOL CRevGraphFilterDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    m_cFromSpin.SetBuddy(GetDlgItem(IDC_FROMREV));
    m_cFromSpin.SetRange32(1, m_headRev);
    m_cFromSpin.SetPos32(m_minRev);

    m_cToSpin.SetBuddy(GetDlgItem(IDC_TOREV));
    m_cToSpin.SetRange32(1, m_headRev);
    m_cToSpin.SetPos32(m_maxRev);

    return TRUE;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRevGraphFilterDlg::OnDeltaposFromspin(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

    // the 'from' revision is about to be changed.
    // Since the 'from' revision must not be higher or the same as the 'to'
    // revision, we have to block this if the user tries to do that.
    if ((pNMUpDown->iPos + pNMUpDown->iDelta) >= m_cToSpin.GetPos32())
    {
        // block the change
        pNMUpDown->iDelta = 0;
    }
    *pResult = 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRevGraphFilterDlg::OnDeltaposTospin(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

    // the 'to' revision is about to be changed.
    // Since the 'to' revision must not be lower or the same as the 'from'
    // revision, we have to block this if the user tries to do that.
    if ((pNMUpDown->iPos + pNMUpDown->iDelta) <= m_cFromSpin.GetPos32())
    {
        // block the change
        pNMUpDown->iDelta = 0;
    }
    *pResult = 0;
}

void CRevGraphFilterDlg::GetRevisionRange(svn_revnum_t& minrev, svn_revnum_t& maxrev) const
{
    minrev = m_minRev;
    maxrev = m_maxRev;
}

void CRevGraphFilterDlg::SetRevisionRange(svn_revnum_t minrev, svn_revnum_t maxrev)
{
    m_minRev = minrev;
    m_maxRev = maxrev;
}

void CRevGraphFilterDlg::OnOK()
{
    m_minRev = m_cFromSpin.GetPos32();
    m_maxRev = m_cToSpin.GetPos32();
    CStandAloneDialog::OnOK();
}
