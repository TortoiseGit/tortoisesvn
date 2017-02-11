// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2010, 2012, 2014-2015, 2017 - TortoiseSVN

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
#include "SimplePrompt.h"
#include "SVNConfig.h"

IMPLEMENT_DYNAMIC(CSimplePrompt, CStandAloneDialog)
CSimplePrompt::CSimplePrompt(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CSimplePrompt::IDD, pParent)
    , m_bSaveAuthentication(FALSE)
    , m_hParentWnd(NULL)
    , m_bHideAuthSaveCheck(false)
{
}

CSimplePrompt::~CSimplePrompt()
{
}

void CSimplePrompt::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_USEREDIT, m_sUsername);
    DDX_Text(pDX, IDC_PASSEDIT, m_sPassword);
    DDX_Check(pDX, IDC_SAVECHECK, m_bSaveAuthentication);
    DDX_Text(pDX, IDC_REALM, m_sRealm);
}


BEGIN_MESSAGE_MAP(CSimplePrompt, CStandAloneDialog)
END_MESSAGE_MAP()

BOOL CSimplePrompt::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_PASSEDIT);
    m_aeroControls.SubclassControl(this, IDC_SAVECHECK);
    m_aeroControls.SubclassOkCancel(this);

    BOOL bAllowAuthSave = (BOOL)(DWORD)CRegDWORD(L"Software\\TortoiseSVN\\AllowAuthSave", TRUE);
    DialogEnableWindow(IDC_SAVECHECK, bAllowAuthSave);
    if (bAllowAuthSave)
    {
        m_bSaveAuthentication = SVNConfig::Instance().ConfigGetBool(SVN_CONFIG_SECTION_AUTH, SVN_CONFIG_OPTION_STORE_PASSWORDS, true);
        CheckDlgButton(IDC_SAVECHECK, m_bSaveAuthentication ? BST_CHECKED : BST_UNCHECKED);
    }
    if (m_bHideAuthSaveCheck)
        GetDlgItem(IDC_SAVECHECK)->ShowWindow(SW_HIDE);

    GetDlgItem(IDC_USEREDIT)->SetFocus();
    if ((m_hParentWnd==NULL)&&(GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(m_hParentWnd));
    return FALSE;
}
