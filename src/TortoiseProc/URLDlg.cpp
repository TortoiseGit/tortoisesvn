// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015 - TortoiseSVN

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
#include "URLDlg.h"
#include "ControlsBridge.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CURLDlg, CResizableStandAloneDialog)
CURLDlg::CURLDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CURLDlg::IDD, pParent)
{
}

CURLDlg::~CURLDlg()
{
}

void CURLDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
}


BEGIN_MESSAGE_MAP(CURLDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()


BOOL CURLDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    BlockResize(DIALOG_BLOCKVERTICAL);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassOkCancel(this);

    m_URLCombo.SetURLHistory(true, false);
    m_URLCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS", L"url");

    CControlsBridge::AlignHorizontally(this, IDC_LABEL, IDC_URLCOMBO);

    AddAnchor(IDC_LABEL, TOP_LEFT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    EnableSaveRestore(L"URLDlg");

    // Now, after the combo size might have changed, select the proper string and
    // put focus into it so that if the text was too wide to be displayed with
    // the original size but it fits into the restored size it is no longer scrolled
    // in the edit box of the combo.
    m_URLCombo.SetCurSel(0);
    m_URLCombo.SetFocus();

    // if there is an url on the clipboard, use that url as the default.
    CAppUtils::AddClipboardUrlToWindow(m_URLCombo.GetSafeHwnd());

    return FALSE;
}

void CURLDlg::OnOK()
{
    if (m_URLCombo.IsWindowEnabled())
    {
        m_URLCombo.SaveHistory();
        m_url = m_URLCombo.GetString();
        UpdateData();
    }

    CResizableStandAloneDialog::OnOK();
}


