// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2013 - TortoiseSVN

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
#include "EditPropsLocalHooks.h"
#include "UnicodeUtils.h"
#include <afxdialogex.h>


// CEditPropsLocalHooks dialog

IMPLEMENT_DYNAMIC(CEditPropsLocalHooks, CResizableStandAloneDialog)

CEditPropsLocalHooks::CEditPropsLocalHooks(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropsLocalHooks::IDD, pParent)
    , m_bWait(false)
    , m_bHide(false)
    , m_bEnforce(false)
{

}

CEditPropsLocalHooks::~CEditPropsLocalHooks()
{
}

void CEditPropsLocalHooks::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_HOOKCOMMANDLINE, m_sCommandLine);
    DDX_Check(pDX, IDC_WAITCHECK, m_bWait);
    DDX_Check(pDX, IDC_HIDECHECK, m_bHide);
    DDX_Check(pDX, IDC_ENFORCECHECK, m_bEnforce);
    DDX_Control(pDX, IDC_HOOKTYPECOMBO, m_cHookTypeCombo);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropsLocalHooks, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, &CEditPropsLocalHooks::OnBnClickedHelp)
END_MESSAGE_MAP()


// CEditPropsLocalHooks message handlers


BOOL CEditPropsLocalHooks::OnInitDialog()
{
    __super::OnInitDialog();

    SetDlgItemText(IDC_HOOKCMLABEL, CString(MAKEINTRESOURCE(IDS_EDITPROPS_LOCALHOOK_CMDLINELABEL)));
    // initialize the combo box with all the hook types we have
    int index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTCOMMIT)));
    m_cHookTypeCombo.SetItemData(index, start_commit_hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_CHECKCOMMIT)));
    m_cHookTypeCombo.SetItemData(index, check_commit_hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PRECOMMIT)));
    m_cHookTypeCombo.SetItemData(index, pre_commit_hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_MANUALPRECOMMIT)));
    m_cHookTypeCombo.SetItemData(index, manual_precommit);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTCOMMIT)));
    m_cHookTypeCombo.SetItemData(index, post_commit_hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTUPDATE)));
    m_cHookTypeCombo.SetItemData(index, start_update_hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PREUPDATE)));
    m_cHookTypeCombo.SetItemData(index, pre_update_hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTUPDATE)));
    m_cHookTypeCombo.SetItemData(index, post_update_hook);

    // the string consists of multiple lines, where one hook script is defined
    // as three lines:
    // line 1: command line to execute
    // line 2: 'true' or 'false' for waiting for the script to finish
    // line 3: 'show' or 'hide' on how to start the hook script
    hookcmd cmd;
    cmd.bShow = false;
    cmd.bWait = false;
    cmd.bEnforce = false;
    cmd.bApproved = false;
    hooktype htype = unknown_hook;
    CString temp;
    CString strhook = CUnicodeUtils::GetUnicode(m_PropValue.c_str());
    if (!strhook.IsEmpty())
    {
        int pos = 0;
        temp = strhook.Tokenize(_T("\n"), pos);
        htype = CHooks::GetHookType(temp);
        if (!temp.IsEmpty())
        {
            temp = strhook.Tokenize(_T("\n"), pos);
            cmd.commandline = temp;
            temp = strhook.Tokenize(_T("\n"), pos);
            if (!temp.IsEmpty())
            {
                cmd.bWait = (temp.CompareNoCase(_T("true"))==0);
                temp = strhook.Tokenize(_T("\n"), pos);
                if (!temp.IsEmpty())
                {
                    cmd.bShow = (temp.CompareNoCase(_T("show"))==0);

                    temp = strhook.Tokenize(_T("\n"), pos);
                    cmd.bEnforce = (temp.CompareNoCase(_T("enforce"))==0);
                }
            }
        }
    }

    if (htype == unknown_hook)
    {
        if (m_PropName.compare(PROJECTPROPNAME_STARTCOMMITHOOK)==0)
            htype = start_commit_hook;
        if (m_PropName.compare(PROJECTPROPNAME_CHECKCOMMITHOOK)==0)
            htype = pre_commit_hook;
        if (m_PropName.compare(PROJECTPROPNAME_PRECOMMITHOOK)==0)
            htype = pre_commit_hook;
        if (m_PropName.compare(PROJECTPROPNAME_MANUALPRECOMMITHOOK)==0)
            htype = manual_precommit;
        if (m_PropName.compare(PROJECTPROPNAME_POSTCOMMITHOOK)==0)
            htype = post_commit_hook;
        if (m_PropName.compare(PROJECTPROPNAME_STARTUPDATEHOOK)==0)
            htype = start_update_hook;
        if (m_PropName.compare(PROJECTPROPNAME_PREUPDATEHOOK)==0)
            htype = pre_update_hook;
        if (m_PropName.compare(PROJECTPROPNAME_POSTUPDATEHOOK)==0)
            htype = post_update_hook;
    }

    // preselect the right hook type in the combobox
    for (int i=0; i<m_cHookTypeCombo.GetCount(); ++i)
    {
        hooktype ht = (hooktype)m_cHookTypeCombo.GetItemData(i);
        if (ht == htype)
        {
            CString str;
            m_cHookTypeCombo.GetLBText(i, str);
            m_cHookTypeCombo.SelectString(i, str);
            break;
        }
    }

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || (!m_bFolder && !m_bMultiple) || m_bRemote ? SW_HIDE : SW_SHOW);

    m_sCommandLine = cmd.commandline;
    m_bWait = cmd.bWait;
    m_bHide = !cmd.bShow;
    m_bEnforce = cmd.bEnforce;
    m_tooltips.Create(this);
    UpdateData(FALSE);

    AddAnchor(IDC_HOOKTYPELABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_HOOKTYPECOMBO, TOP_RIGHT);
    AddAnchor(IDC_HOOKCMLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_HOOKCOMMANDLINE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_WAITCHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_HIDECHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_ENFORCECHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_PROPRECURSIVE, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    EnableSaveRestore(_T("EditPropsLocalHooks"));

    return TRUE;
}

void CEditPropsLocalHooks::OnBnClickedHelp()
{
    OnHelp();
}

void CEditPropsLocalHooks::OnCancel()
{
    CResizableStandAloneDialog::OnCancel();
}

void CEditPropsLocalHooks::OnOK()
{
    UpdateData();
    int cursel = m_cHookTypeCombo.GetCurSel();
    hooktype htype = unknown_hook;
    if (cursel != CB_ERR)
    {
        htype = (hooktype)m_cHookTypeCombo.GetItemData(cursel);
    }
    if (htype == unknown_hook)
    {
        m_tooltips.ShowBalloon(IDC_HOOKTYPECOMBO, IDS_ERR_NOHOOKTYPESPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    if (m_sCommandLine.IsEmpty())
    {
        ShowEditBalloon(IDC_HOOKCOMMANDLINE, IDS_ERR_NOHOOKCOMMANDPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    CString sHookType = CHooks::GetHookTypeString(htype);
    m_PropValue = CUnicodeUtils::StdGetUTF8((LPCWSTR)sHookType) + "\n";
    m_PropValue += CUnicodeUtils::StdGetUTF8((LPCWSTR)m_sCommandLine) + "\n";
    m_PropValue += m_bWait ? "true\n" : "false\n";
    m_PropValue += m_bHide ? "hide" : "show";
    m_PropValue += m_bEnforce ? "\nenforce" : "";

    switch (htype)
    {
    case start_commit_hook:
        m_PropName = PROJECTPROPNAME_STARTCOMMITHOOK;
        break;
    case check_commit_hook:
        m_PropName = PROJECTPROPNAME_CHECKCOMMITHOOK;
        break;
    case pre_commit_hook:
        m_PropName = PROJECTPROPNAME_PRECOMMITHOOK;
        break;
    case manual_precommit:
        m_PropName = PROJECTPROPNAME_MANUALPRECOMMITHOOK;
        break;
    case post_commit_hook:
        m_PropName = PROJECTPROPNAME_POSTCOMMITHOOK;
        break;
    case start_update_hook:
        m_PropName = PROJECTPROPNAME_STARTUPDATEHOOK;
        break;
    case pre_update_hook:
        m_PropName = PROJECTPROPNAME_PREUPDATEHOOK;
        break;
    case post_update_hook:
        m_PropName = PROJECTPROPNAME_POSTUPDATEHOOK;
        break;
    }
    m_bChanged = true;
    CResizableStandAloneDialog::OnOK();
}
