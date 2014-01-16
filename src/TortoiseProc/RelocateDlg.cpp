// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014 - TortoiseSVN

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
#include "RelocateDlg.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CRelocateDlg, CResizableStandAloneDialog)
CRelocateDlg::CRelocateDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CRelocateDlg::IDD, pParent)
    , m_sToUrl(L"")
    , m_sFromUrl(L"")
    , m_bIncludeExternals(FALSE)
    , m_height(0)
{
}

CRelocateDlg::~CRelocateDlg()
{
}

void CRelocateDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TOURL, m_URLCombo);
    DDX_Control(pDX, IDC_FROMURL, m_FromUrl);
    DDX_Check(pDX, IDC_INCLUDEEXTERNALS, m_bIncludeExternals);
}


BEGIN_MESSAGE_MAP(CRelocateDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_WM_SIZING()
END_MESSAGE_MAP()

BOOL CRelocateDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_INCLUDEEXTERNALS);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_URLCombo.SetURLHistory(true, true);
    m_URLCombo.SetCurSel(0);

    RECT rect;
    GetWindowRect(&rect);
    m_height = rect.bottom - rect.top;

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sWindowTitle);

    AddAnchor(IDC_FROMURLLABEL, TOP_LEFT);
    AddAnchor(IDC_FROMURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_TOURLLABEL, TOP_LEFT);
    AddAnchor(IDC_TOURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_DWM, TOP_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    SetDlgItemText(IDC_FROMURL, m_sFromUrl);
    m_URLCombo.SetWindowText(m_sFromUrl);
    if ((m_pParentWnd==NULL)&&(GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"RelocateDlg");
    return TRUE;
}

void CRelocateDlg::OnOK()
{
    UpdateData(TRUE);
    m_sToUrl = m_URLCombo.GetString();
    UpdateData(FALSE);

    CResizableStandAloneDialog::OnOK();
}

void CRelocateDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CRelocateDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
    // don't allow the dialog to be changed in height
    switch (fwSide)
    {
    case WMSZ_BOTTOM:
    case WMSZ_BOTTOMLEFT:
    case WMSZ_BOTTOMRIGHT:
        pRect->bottom = pRect->top + m_height;
        break;
    case WMSZ_TOP:
    case WMSZ_TOPLEFT:
    case WMSZ_TOPRIGHT:
        pRect->top = pRect->bottom - m_height;
        break;
    }
    CResizableStandAloneDialog::OnSizing(fwSide, pRect);
}
