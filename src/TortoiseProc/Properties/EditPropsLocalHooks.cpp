// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2021 - TortoiseSVN

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
#include "EditPropsLocalHooks.h"
#include "UnicodeUtils.h"
#include "Hooks.h"

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
    m_cHookTypeCombo.SetItemData(index, Start_Commit_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_CHECKCOMMIT)));
    m_cHookTypeCombo.SetItemData(index, Check_Commit_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PRECOMMIT)));
    m_cHookTypeCombo.SetItemData(index, Pre_Commit_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_MANUALPRECOMMIT)));
    m_cHookTypeCombo.SetItemData(index, Manual_Precommit);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTCOMMIT)));
    m_cHookTypeCombo.SetItemData(index, Post_Commit_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTUPDATE)));
    m_cHookTypeCombo.SetItemData(index, Start_Update_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PREUPDATE)));
    m_cHookTypeCombo.SetItemData(index, Pre_Update_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTUPDATE)));
    m_cHookTypeCombo.SetItemData(index, Post_Update_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PRELOCK)));
    m_cHookTypeCombo.SetItemData(index, Pre_Lock_Hook);
    index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTLOCK)));
    m_cHookTypeCombo.SetItemData(index, Post_Lock_Hook);

    // the string consists of multiple lines, where one hook script is defined
    // as three lines:
    // line 1: command line to execute
    // line 2: 'true' or 'false' for waiting for the script to finish
    // line 3: 'show' or 'hide' on how to start the hook script
    HookCmd cmd;
    cmd.bShow        = false;
    cmd.bWait        = false;
    cmd.bEnforce     = false;
    cmd.bApproved    = false;
    HookType hType   = Unknown_Hook;
    CString  strHook = CUnicodeUtils::GetUnicode(m_propValue.c_str());
    if (!strHook.IsEmpty())
    {
        int     pos  = 0;
        CString temp = strHook.Tokenize(L"\n", pos);
        hType        = CHooks::GetHookType(temp);
        if (!temp.IsEmpty())
        {
            temp            = strHook.Tokenize(L"\n", pos);
            cmd.commandline = temp;
            temp            = strHook.Tokenize(L"\n", pos);
            if (!temp.IsEmpty())
            {
                cmd.bWait = (temp.CompareNoCase(L"true") == 0);
                temp      = strHook.Tokenize(L"\n", pos);
                if (!temp.IsEmpty())
                {
                    cmd.bShow = (temp.CompareNoCase(L"show") == 0);

                    temp         = strHook.Tokenize(L"\n", pos);
                    cmd.bEnforce = (temp.CompareNoCase(L"enforce") == 0);
                }
            }
        }
    }

    if (hType == Unknown_Hook)
    {
        if (m_propName.compare(PROJECTPROPNAME_STARTCOMMITHOOK) == 0)
            hType = Start_Commit_Hook;
        if (m_propName.compare(PROJECTPROPNAME_CHECKCOMMITHOOK) == 0)
            hType = Pre_Commit_Hook;
        if (m_propName.compare(PROJECTPROPNAME_PRECOMMITHOOK) == 0)
            hType = Pre_Commit_Hook;
        if (m_propName.compare(PROJECTPROPNAME_MANUALPRECOMMITHOOK) == 0)
            hType = Manual_Precommit;
        if (m_propName.compare(PROJECTPROPNAME_POSTCOMMITHOOK) == 0)
            hType = Post_Commit_Hook;
        if (m_propName.compare(PROJECTPROPNAME_STARTUPDATEHOOK) == 0)
            hType = Start_Update_Hook;
        if (m_propName.compare(PROJECTPROPNAME_PREUPDATEHOOK) == 0)
            hType = Pre_Update_Hook;
        if (m_propName.compare(PROJECTPROPNAME_POSTUPDATEHOOK) == 0)
            hType = Post_Update_Hook;
        if (m_propName.compare(PROJECTPROPNAME_PRELOCKHOOK) == 0)
            hType = Pre_Lock_Hook;
        if (m_propName.compare(PROJECTPROPNAME_POSTLOCKHOOK) == 0)
            hType = Post_Lock_Hook;
    }

    // preselect the right hook type in the combobox
    for (int i = 0; i < m_cHookTypeCombo.GetCount(); ++i)
    {
        auto ht = static_cast<HookType>(m_cHookTypeCombo.GetItemData(i));
        if (ht == hType)
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
    m_bWait        = cmd.bWait;
    m_bHide        = !cmd.bShow;
    m_bEnforce     = cmd.bEnforce;
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
    EnableSaveRestore(L"EditPropsLocalHooks");

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
    int      curSel = m_cHookTypeCombo.GetCurSel();
    HookType hType  = Unknown_Hook;
    if (curSel != CB_ERR)
    {
        hType = static_cast<HookType>(m_cHookTypeCombo.GetItemData(curSel));
    }
    if (hType == Unknown_Hook)
    {
        m_tooltips.ShowBalloon(IDC_HOOKTYPECOMBO, IDS_ERR_NOHOOKTYPESPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    if (m_sCommandLine.IsEmpty())
    {
        ShowEditBalloon(IDC_HOOKCOMMANDLINE, IDS_ERR_NOHOOKCOMMANDPECIFIED, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    CString sHookType = CHooks::GetHookTypeString(hType);
    m_propValue       = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(sHookType)) + "\n";
    m_propValue += CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_sCommandLine)) + "\n";
    m_propValue += m_bWait ? "true\n" : "false\n";
    m_propValue += m_bHide ? "hide" : "show";
    m_propValue += m_bEnforce ? "\nenforce" : "";

    switch (hType)
    {
        case Start_Commit_Hook:
            m_propName = PROJECTPROPNAME_STARTCOMMITHOOK;
            break;
        case Check_Commit_Hook:
            m_propName = PROJECTPROPNAME_CHECKCOMMITHOOK;
            break;
        case Pre_Commit_Hook:
            m_propName = PROJECTPROPNAME_PRECOMMITHOOK;
            break;
        case Manual_Precommit:
            m_propName = PROJECTPROPNAME_MANUALPRECOMMITHOOK;
            break;
        case Post_Commit_Hook:
            m_propName = PROJECTPROPNAME_POSTCOMMITHOOK;
            break;
        case Start_Update_Hook:
            m_propName = PROJECTPROPNAME_STARTUPDATEHOOK;
            break;
        case Pre_Update_Hook:
            m_propName = PROJECTPROPNAME_PREUPDATEHOOK;
            break;
        case Post_Update_Hook:
            m_propName = PROJECTPROPNAME_POSTUPDATEHOOK;
            break;
        case Pre_Lock_Hook:
            m_propName = PROJECTPROPNAME_PRELOCKHOOK;
            break;
        case Post_Lock_Hook:
            m_propName = PROJECTPROPNAME_POSTLOCKHOOK;
            break;
        case Unknown_Hook:
            break;
        case Pre_Connect_Hook:
            break;
        default:
            break;
    }
    m_bChanged = true;
    CResizableStandAloneDialog::OnOK();
}
