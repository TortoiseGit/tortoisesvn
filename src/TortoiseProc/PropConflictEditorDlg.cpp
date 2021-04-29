// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016-2018, 2020-2021 - TortoiseSVN

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
#include "PropConflictEditorDlg.h"
#include "CommonAppUtils.h"
#include "SVN.h"
#include "AppUtils.h"

CPropConflictEditorDlg::CPropConflictEditorDlg()
    : m_conflictInfo(nullptr)
    , m_choice(svn_client_conflict_option_undefined)
    , m_svn(nullptr)
    , m_bCancelled(false)
    , m_mergedCreationTime(0)
{
}

CPropConflictEditorDlg::~CPropConflictEditorDlg()
{
}

HRESULT CALLBACK CPropConflictEditorDlg::TaskDialogCallback(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
    CPropConflictEditorDlg *pThis = reinterpret_cast<CPropConflictEditorDlg *>(dwRefData);
    return pThis->OnNotify(hWnd, uNotification, wParam, lParam);
}

HRESULT CPropConflictEditorDlg::OnNotify(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM)
{
    switch (uNotification)
    {
        case TDN_DIALOG_CONSTRUCTED:
            return OnDialogConstructed(hWnd);
        case TDN_BUTTON_CLICKED:
            return OnButtonClicked(hWnd, static_cast<int>(wParam));
        case TDN_TIMER:
            return OnTimer(hWnd);
        case TDN_HELP:
            if (!CAppUtils::StartHtmlHelp(IDD_CONFLICTRESOLVE + 0x20000))
            {
                AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
            }
            break;
        default:
            break;
    }
    return S_OK;
}

void CPropConflictEditorDlg::AddCommandButton(int id, const CString &text)
{
    TASKDIALOG_BUTTON btn = {0};
    m_buttonTexts.push_back(text);
    btn.nButtonID     = id;
    btn.pszButtonText = m_buttonTexts.back().GetString();

    m_buttons.push_back(btn);
}

HRESULT CPropConflictEditorDlg::OnDialogConstructed(HWND hWnd)
{
    CCommonAppUtils::MarkWindowAsUnpinnable(hWnd);

    HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    ::SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
    ::SendMessage(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    ::SendMessage(hWnd, TDM_ENABLE_BUTTON, 100 + svn_client_conflict_option_merged_text, 0);
    return S_OK;
}

HRESULT CPropConflictEditorDlg::OnButtonClicked(HWND hWnd, int id)
{
    if (id == 1000)
    {
        // Edit conflicts
        CTSVNPath theirs, mine, base;
        m_conflictInfo->GetPropValFiles(m_propName, m_merged, base, theirs, mine);
        m_mergedCreationTime = m_merged.GetLastWriteTime();
        ::SendMessage(hWnd, TDM_ENABLE_BUTTON, 100 + svn_client_conflict_option_merged_text, 0);

        CString n1, n2, n3, n4;
        n1.Format(IDS_DIFF_PROP_BASENAME, static_cast<LPCWSTR>(m_propName));
        n2.Format(IDS_DIFF_PROP_REMOTENAME, static_cast<LPCWSTR>(m_propName));
        n3.Format(IDS_DIFF_PROP_WCNAME, static_cast<LPCWSTR>(m_propName));
        n4.Format(IDS_DIFF_PROP_MERGENAME, static_cast<LPCWSTR>(m_propName));

        CAppUtils::MergeFlags flags;
        flags.AlternativeTool((GetKeyState(VK_SHIFT) & 0x8000) != 0);
        flags.PreventSVNResolve(true);
        CAppUtils::StartExtMerge(flags,
                                 base, theirs, mine, m_merged, true, n1, n1, n3, n4, m_propName);
        return S_FALSE;
    }
    for (SVNConflictOptions::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
        svn_client_conflict_option_id_t optionId = (*it)->GetId();
        int                             buttonID = 100 + optionId;

        if (buttonID == id)
        {
            if (optionId == svn_client_conflict_option_merged_text)
            {
                (*it)->SetMergedPropValFile(m_merged, m_options.GetPool());
            }
            if (m_svn)
            {
                if (!m_svn->ResolvePropConflict(*m_conflictInfo, m_propName, *it->get()))
                {
                    m_svn->ShowErrorDialog(hWnd);
                    return S_FALSE;
                }
            }
            else
            {
                SVN svn;
                if (!svn.ResolvePropConflict(*m_conflictInfo, m_propName, *it->get()))
                {
                    svn.ShowErrorDialog(hWnd);
                    return S_FALSE;
                }
            }
            m_choice = optionId;
            return S_OK;
        }
    }

    return S_OK;
}

HRESULT CPropConflictEditorDlg::OnTimer(HWND hWnd)
{
    if ((m_mergedCreationTime > 0) && m_merged.Exists() && (m_merged.GetLastWriteTime(true) > m_mergedCreationTime))
    {
        ::SendMessage(hWnd, TDM_ENABLE_BUTTON, 100 + svn_client_conflict_option_merged_text, 1);
        m_mergedCreationTime = 0;
    }

    return S_OK;
}

void CPropConflictEditorDlg::DoModal(HWND parent, int index)
{
    auto    path = m_conflictInfo->GetPath().GetFileOrDirectoryName();
    CString sDialogTitle;
    sDialogTitle.LoadString(IDS_PROC_EDIT_PROP_CONFLICTS);
    sDialogTitle = CCommonAppUtils::FormatWindowTitle(path, sDialogTitle);

    if (!m_conflictInfo->GetPropResolutionOptions(m_options))
    {
        m_conflictInfo->ShowErrorDialog(parent);
    }

    m_propName = m_conflictInfo->GetPropConflictName(index);
    CString sMainInstruction;
    sMainInstruction.Format(IDS_EDITCONFLICT_PROP_MAININSTRUCTION, static_cast<LPCWSTR>(m_propName));
    CString sContent = m_conflictInfo->GetPropDescription();
    sContent += L"\n" + m_conflictInfo->GetPath().GetUIPathString();
    CString sDetailedInfo = CString(MAKEINTRESOURCE(IDS_EDITCONFLICT_PROP_DIFF)) + m_conflictInfo->GetPropDiff(m_propName);

    for (SVNConflictOptions::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
        CString optLabel = (*it)->GetLabel();

        CString optDescription((*it)->GetDescription());
        optDescription.SetAt(0, towupper(optDescription[0]));

        int buttonID = 100 + (*it)->GetId();
        AddCommandButton(buttonID, optLabel + L"\n" + optDescription);
    }

    AddCommandButton(1000, CString(MAKEINTRESOURCE(IDS_EDITCONFLICT_PROP_EDITCMD)));

    svn_client_conflict_option_id_t recommendedOptionId = m_conflictInfo->GetRecommendedOptionId();
    int                             defaultButtonID     = 0;

    if (recommendedOptionId != svn_client_conflict_option_unspecified)
    {
        SVNConflictOption *recommendedOption = m_options.FindOptionById(recommendedOptionId);

        if (recommendedOption)
        {
            defaultButtonID = 100 + recommendedOption->GetId();
        }
    }

    int              button;
    TASKDIALOGCONFIG taskConfig       = {0};
    taskConfig.cbSize                 = sizeof(taskConfig);
    taskConfig.hwndParent             = parent;
    taskConfig.dwFlags                = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_EXPAND_FOOTER_AREA | TDF_CALLBACK_TIMER;
    taskConfig.lpCallbackData         = reinterpret_cast<LONG_PTR>(this);
    taskConfig.pfCallback             = TaskDialogCallback;
    taskConfig.pszWindowTitle         = sDialogTitle;
    taskConfig.pszMainInstruction     = sMainInstruction;
    taskConfig.pszExpandedInformation = sDetailedInfo;
    taskConfig.pszContent             = sContent;
    taskConfig.pButtons               = &m_buttons.front();
    taskConfig.cButtons               = static_cast<int>(m_buttons.size());
    taskConfig.nDefaultButton         = defaultButtonID;
    taskConfig.dwCommonButtons        = TDCBF_CANCEL_BUTTON;
    TaskDialogIndirect(&taskConfig, &button, nullptr, nullptr);
    if (button == IDCANCEL)
    {
        m_bCancelled = true;
    }
}
