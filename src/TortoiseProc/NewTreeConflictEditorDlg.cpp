// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseSVN

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
#include "NewTreeConflictEditorDlg.h"
#include "CommonAppUtils.h"
#include "TortoiseProc.h"
#include "SVN.h"

CNewTreeConflictEditorDlg::CNewTreeConflictEditorDlg()
    : m_conflictInfo(NULL)
    , m_choice(svn_client_conflict_option_undefined)
    , m_bCancelled(false)
{
}

CNewTreeConflictEditorDlg::~CNewTreeConflictEditorDlg()
{
}

HRESULT CALLBACK CNewTreeConflictEditorDlg::TaskDialogCallback(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
    CNewTreeConflictEditorDlg *pThis = reinterpret_cast<CNewTreeConflictEditorDlg*>(dwRefData);
    return pThis->OnNotify(hWnd, uNotification, wParam, lParam);
}

HRESULT CNewTreeConflictEditorDlg::OnNotify(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM)
{
    switch (uNotification)
    {
    case TDN_DIALOG_CONSTRUCTED:
        return OnDialogConstructed(hWnd);
    case TDN_BUTTON_CLICKED:
        return OnButtonClicked(hWnd, (int) wParam);
    default:
        return S_OK;
    }
}

void CNewTreeConflictEditorDlg::AddCommandButton(int id, const CString & text)
{
    TASKDIALOG_BUTTON btn = { 0 };
    m_buttonTexts.push_back(text);
    btn.nButtonID = id;
    btn.pszButtonText = m_buttonTexts.back().GetString();

    m_buttons.push_back(btn);
}

HRESULT CNewTreeConflictEditorDlg::OnDialogConstructed(HWND hWnd)
{
    CCommonAppUtils::MarkWindowAsUnpinnable(hWnd);

    HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    ::SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    ::SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    return S_OK;
}

HRESULT CNewTreeConflictEditorDlg::OnButtonClicked(HWND hWnd, int id)
{
    for (SVNConflictOptions::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
        svn_client_conflict_option_id_t optionId = (*it)->GetId();
        if (optionId + 100 == id)
        {
            SVN svn;
            if (!svn.ResolveTreeConflict(*m_conflictInfo, *it->get()))
            {
                svn.ShowErrorDialog(hWnd);
                return S_FALSE;
            }
            m_choice = optionId;
            return S_OK;
        }
    }

    return S_OK;
}

// The following function uses non-localized strings, because it should become
// part of SVN 1.10 API
static CString GetConflictOptionTitle(svn_client_conflict_option_id_t id)
{
    switch (id)
    {
    case svn_client_conflict_option_postpone:
        return L"Postpone";
    case svn_client_conflict_option_accept_current_wc_state:
        return L"Mark as resolved";

    // Options for local move vs incoming edit on update.
    case svn_client_conflict_option_update_move_destination:
        return L"Update move destination";
    case svn_client_conflict_option_update_any_moved_away_children:
        return L"Update any moved-away children";

    // Options for incoming add vs local add.
    case svn_client_conflict_option_incoming_add_ignore:
        return L"Ignore incoming addition";

    // Options for incoming file add vs local file add upon merge.
    case svn_client_conflict_option_incoming_added_file_text_merge:
        return L"Merge the files";
    case svn_client_conflict_option_incoming_added_file_replace:
        return L"Replace with incoming change";
    case svn_client_conflict_option_incoming_added_file_replace_and_merge:
        return L"Replace my file with incoming file and merge the files";

    // Options for incoming dir add vs local dir add or obstruction.
    case svn_client_conflict_option_incoming_added_dir_merge:
        return L"Merge the directories";
    case svn_client_conflict_option_incoming_added_dir_replace:
        return L"Delete my directory and replace it with incoming directory";
    case svn_client_conflict_option_incoming_added_dir_replace_and_merge:
        return L"replace my directory with incoming directory and merge";

    // Options for incoming delete vs any
    case svn_client_conflict_option_incoming_delete_ignore:
        return L"Ignore incoming deletion";
    case svn_client_conflict_option_incoming_delete_accept:
        return L"Accept incoming deletion";

    // Options for incoming move vs local edit
    case svn_client_conflict_option_incoming_move_file_text_merge:
    case svn_client_conflict_option_incoming_move_dir_merge:
            return L"Move and merge";
    }

    ATLASSERT(FALSE);
    CString str;
    str.Format(L"%d", id);
    return str;
}

void CNewTreeConflictEditorDlg::DoModal(HWND parent)
{
    CTSVNPath path = m_conflictInfo->GetPath();
    CString sDialogTitle;
    sDialogTitle.LoadString(IDS_PROC_EDIT_TREE_CONFLICTS);
    sDialogTitle = CCommonAppUtils::FormatWindowTitle(path.GetUIPathString(), sDialogTitle);

    if (!m_conflictInfo->GetTreeResolutionOptions(m_options))
    {
        m_conflictInfo->ShowErrorDialog(parent);
    }

    CString sMainInstruction = m_conflictInfo->GetIncomingChangeSummary();
    CString sContent = m_conflictInfo->GetLocalChangeSummary();

    int button;

    for (SVNConflictOptions::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
        svn_client_conflict_option_id_t id = (*it)->GetId();

        CString optDescription((*it)->GetDescription());
        optDescription.SetAt(0, towupper(optDescription[0]));
        CString optTitle = GetConflictOptionTitle(id);

        AddCommandButton(100 + id, optTitle + L"\n" + optDescription);
    }

    TASKDIALOGCONFIG taskConfig = {0};
    taskConfig.cbSize = sizeof(taskConfig);
    taskConfig.hwndParent = parent;
    taskConfig.dwFlags = TDF_USE_COMMAND_LINKS;
    taskConfig.lpCallbackData = (LONG_PTR) this;
    taskConfig.pfCallback = TaskDialogCallback;
    taskConfig.pszWindowTitle = sDialogTitle;
    taskConfig.pszMainInstruction = sMainInstruction;
    taskConfig.pszContent = sContent;
    taskConfig.pButtons = &m_buttons.front();
    taskConfig.cButtons = (int) m_buttons.size();
    taskConfig.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    TaskDialogIndirect(&taskConfig, &button, NULL, NULL);
    if (button == IDCANCEL)
    {
        m_bCancelled = true;
    }
}
