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
#include "TextConflictEditorDlg.h"
#include "CommonAppUtils.h"
#include "SVN.h"
#include "AppUtils.h"
#include "StringUtils.h"

CTextConflictEditorDlg::CTextConflictEditorDlg()
    : m_conflictInfo(nullptr)
    , m_choice(svn_client_conflict_option_undefined)
    , m_svn(nullptr)
    , m_bCancelled(false)
    , m_mergedCreationTime(0)
{
}

CTextConflictEditorDlg::~CTextConflictEditorDlg()
{
}

HRESULT CALLBACK CTextConflictEditorDlg::TaskDialogCallback(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
    CTextConflictEditorDlg *pThis = reinterpret_cast<CTextConflictEditorDlg *>(dwRefData);
    return pThis->OnNotify(hWnd, uNotification, wParam, lParam);
}

HRESULT CTextConflictEditorDlg::OnNotify(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM)
{
    switch (uNotification)
    {
        case TDN_DIALOG_CONSTRUCTED:
            return OnDialogConstructed(hWnd);
        case TDN_BUTTON_CLICKED:
            return OnButtonClicked(hWnd, static_cast<int>(wParam));
        case TDN_TIMER:
            return OnTimer(hWnd, wParam);
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

void CTextConflictEditorDlg::AddCommandButton(int id, const CString &text)
{
    TASKDIALOG_BUTTON btn = {0};
    m_buttonTexts.push_back(text);
    btn.nButtonID     = id;
    btn.pszButtonText = m_buttonTexts.back().GetString();

    m_buttons.push_back(btn);
}

HRESULT CTextConflictEditorDlg::OnDialogConstructed(HWND hWnd)
{
    CCommonAppUtils::MarkWindowAsUnpinnable(hWnd);

    HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    ::SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
    ::SendMessage(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
    ::SendMessage(hWnd, TDM_ENABLE_BUTTON, 100 + svn_client_conflict_option_merged_text, 0);
    return S_OK;
}

HRESULT CTextConflictEditorDlg::OnButtonClicked(HWND hWnd, int id)
{
    if (id == 1000)
    {
        // Edit conflicts
        CTSVNPath theirs, mine, base;
        m_merged = m_conflictInfo->GetPath();
        m_conflictInfo->GetTextContentFiles(base, theirs, mine);
        ::SendMessage(hWnd, TDM_ENABLE_BUTTON, 100 + svn_client_conflict_option_merged_text, 0);
        m_mergedCreationTime = m_merged.GetLastWriteTime(true);

        CString filename, n1, n2, n3, n4;
        filename = m_merged.GetUIFileOrDirectoryName();
        n1.Format(IDS_DIFF_BASENAME, static_cast<LPCWSTR>(filename));
        n2.Format(IDS_DIFF_REMOTENAME, static_cast<LPCWSTR>(filename));
        n3.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(filename));
        n4.Format(IDS_DIFF_MERGEDNAME, static_cast<LPCWSTR>(filename));

        CAppUtils::MergeFlags flags;
        flags.AlternativeTool((GetKeyState(VK_SHIFT) & 0x8000) != 0);
        flags.PreventSVNResolve(true);
        CAppUtils::StartExtMerge(flags,
                                 base, theirs, mine, m_merged, true, n1, n2, n3, n4, filename);
        return S_FALSE;
    }
    for (SVNConflictOptions::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
        svn_client_conflict_option_id_t optionId = (*it)->GetId();
        int                             buttonID = 100 + optionId;

        if (buttonID == id)
        {
            if (m_svn)
            {
                if (!m_svn->ResolveTextConflict(*m_conflictInfo, *it->get()))
                {
                    m_svn->ShowErrorDialog(hWnd);
                    return S_FALSE;
                }
            }
            else
            {
                SVN svn;
                if (!svn.ResolveTextConflict(*m_conflictInfo, *it->get()))
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

HRESULT CTextConflictEditorDlg::OnTimer(HWND hWnd, WPARAM wParam)
{
    // timer triggers every 200ms, but we don't want to check
    // that frequently. So use the tickcount.
    if (wParam < 500)
        return S_OK;
    if ((m_mergedCreationTime > 0) && m_merged.Exists(true) && (m_merged.GetLastWriteTime() > m_mergedCreationTime))
    {
        ::SendMessage(hWnd, TDM_ENABLE_BUTTON, 100 + svn_client_conflict_option_merged_text, 1);
        m_mergedCreationTime = 0;
    }

    return S_FALSE; // reset tick count
}

void CTextConflictEditorDlg::DoModal(HWND parent)
{
    auto    path = m_conflictInfo->GetPath().GetFileOrDirectoryName();
    CString sDialogTitle;
    sDialogTitle.LoadString(IDS_PROC_EDIT_TEXT_CONFLICTS);
    sDialogTitle = CCommonAppUtils::FormatWindowTitle(path, sDialogTitle);

    if (!m_conflictInfo->GetTextResolutionOptions(m_options))
    {
        m_conflictInfo->ShowErrorDialog(parent);
    }

    CString sMainInstruction;
    if (m_conflictInfo->IsBinary())
        sMainInstruction.Format(IDS_EDITCONFLICT_TEXT_BINARY_MAININSTRUCTION, static_cast<LPCWSTR>(path));
    else
        sMainInstruction.Format(IDS_EDITCONFLICT_TEXT_MAININSTRUCTION, static_cast<LPCWSTR>(path));
    sMainInstruction = CStringUtils::LinesWrap(sMainInstruction, 80, true, true);
    CString sContent = CStringUtils::WordWrap(m_conflictInfo->GetPath().GetUIPathString(), 80, true, true, 4);

    for (SVNConflictOptions::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
        CString optLabel = (*it)->GetLabel();

        CString optDescription((*it)->GetDescription());
        optDescription.SetAt(0, towupper(optDescription[0]));

        int buttonID = 100 + (*it)->GetId();
        AddCommandButton(buttonID, optLabel + L"\n" + optDescription);
    }

    AddCommandButton(1000, CString(MAKEINTRESOURCE(IDS_EDITCONFLICT_TEXT_EDITCMD)));

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
    TASKDIALOGCONFIG taskConfig   = {0};
    taskConfig.cbSize             = sizeof(taskConfig);
    taskConfig.hwndParent         = parent;
    taskConfig.dwFlags            = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_CALLBACK_TIMER;
    taskConfig.lpCallbackData     = reinterpret_cast<LONG_PTR>(this);
    taskConfig.pfCallback         = TaskDialogCallback;
    taskConfig.pszWindowTitle     = sDialogTitle;
    taskConfig.pszMainInstruction = sMainInstruction;
    taskConfig.pszContent         = sContent;
    taskConfig.pButtons           = &m_buttons.front();
    taskConfig.cButtons           = static_cast<int>(m_buttons.size());
    taskConfig.nDefaultButton     = defaultButtonID;
    taskConfig.dwCommonButtons    = TDCBF_CANCEL_BUTTON;
    TaskDialogIndirect(&taskConfig, &button, nullptr, nullptr);
    if (button == IDCANCEL)
    {
        m_bCancelled = true;
    }
}
