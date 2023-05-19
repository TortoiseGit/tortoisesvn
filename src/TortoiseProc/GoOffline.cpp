// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2021, 2023 - TortoiseSVN

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
#include "GoOffline.h"

// CGoOffline dialog

IMPLEMENT_DYNAMIC(CGoOffline, CDialog)

CGoOffline::CGoOffline(CWnd* pParent /*=NULL*/)
    : CDialog(CGoOffline::IDD, pParent)
    , selection(LogCache::ConnectionState::Online)
    , asDefault(false)
    , doRetry(false)
{
}

CGoOffline::~CGoOffline()
{
}

void CGoOffline::SetErrorMessage(const CString& errMsg)
{
    m_errMsg = errMsg;
}

void CGoOffline::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_ASDEFAULTOFFLINE, asDefault);
}

BEGIN_MESSAGE_MAP(CGoOffline, CDialog)
    ON_BN_CLICKED(IDOK, &CGoOffline::OnBnClickedOk)
    ON_BN_CLICKED(IDC_PERMANENTLYOFFLINE, &CGoOffline::OnBnClickedPermanentlyOffline)
    ON_BN_CLICKED(IDCANCEL, &CGoOffline::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_RETRY, &CGoOffline::OnBnClickedRetry)
END_MESSAGE_MAP()

BOOL CGoOffline::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetDlgItemText(IDC_ERRORMSG, m_errMsg);

    return TRUE;
}

// CGoOffline message handlers

void CGoOffline::OnBnClickedOk()
{
    selection = LogCache::ConnectionState::TempOffline;

    OnOK();
}

void CGoOffline::OnBnClickedPermanentlyOffline()
{
    selection = LogCache::ConnectionState::Offline;

    OnOK();
}

void CGoOffline::OnBnClickedCancel()
{
    selection = LogCache::ConnectionState::Online;

    OnCancel();
}

void CGoOffline::OnBnClickedRetry()
{
    doRetry   = true;
    selection = LogCache::ConnectionState::Online;

    OnCancel();
}
