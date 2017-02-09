// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016-2017 - TortoiseSVN

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

#pragma once

#include "SVNConflictInfo.h"
#include "SVN.h"

class CPropConflictEditorDlg
{
public:
    CPropConflictEditorDlg();
    ~CPropConflictEditorDlg();

    void DoModal(HWND parent, int index);
    void SetConflictInfo(SVNConflictInfo * conflictInfo) { m_conflictInfo = conflictInfo; }
    void SetSVNContext(SVN * svn) { m_svn = svn; }

    svn_client_conflict_option_id_t GetResult() { return m_choice; }
    bool IsCancelled() const { return m_bCancelled; }
private:
    static HRESULT CALLBACK TaskDialogCallback(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);
    HRESULT OnDialogConstructed(HWND hWnd);
    HRESULT OnButtonClicked(HWND hWnd, int id);
    HRESULT OnTimer(HWND hWnd);
    HRESULT OnNotify(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam);

    void AddCommandButton(int id, const CString & text);

    SVNConflictInfo * m_conflictInfo;
    SVNConflictOptions m_options;
    svn_client_conflict_option_id_t m_choice;
    SVN * m_svn;
    bool m_bCancelled;
    CString m_propName;
    CTSVNPath m_merged;
    __int64 m_mergedCreationTime;

    std::vector<TASKDIALOG_BUTTON> m_buttons;
    std::deque<CString> m_buttonTexts;
};
