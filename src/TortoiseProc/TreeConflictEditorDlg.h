// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2013, 2015 - TortoiseSVN

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

#include "StandAloneDlg.h"
#include "TSVNPath.h"
#include "ProgressDlg.h"
#include "SVNRev.h"
#include "PathEdit.h"

class CTreeConflictEditorDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CTreeConflictEditorDlg)

public:
    CTreeConflictEditorDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTreeConflictEditorDlg();

    void SetPath(const CTSVNPath& path) {m_path = path;}
    void SetConflictSources(const svn_wc_conflict_version_t * left, const svn_wc_conflict_version_t * right);
    void SetConflictLeftSources(const CString& url, const CString& path, const SVNRev& rev, svn_node_kind_t nodekind);
    void SetConflictRightSources(const CString& url, const CString& path, const SVNRev& rev, svn_node_kind_t nodekind);
    void SetConflictAction(svn_wc_conflict_action_t action) {conflict_action = action;}
    void SetConflictReason(svn_wc_conflict_reason_t reason) {conflict_reason = reason;}
    void SetConflictOperation(svn_wc_operation_t operation) {conflict_operation = operation;}
    void SetKind(svn_node_kind_t k) {kind = k;}
    void SetInteractive(bool bInteractive = true) {m_bInteractive = bInteractive;}

    svn_wc_conflict_choice_t GetResult() {return m_choice;}
    bool IsCancelled() const {return m_bCancelled;}

// Dialog Data
    enum { IDD = IDD_TREECONFLICTEDITOR };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

    afx_msg void OnBnClickedResolveusingtheirs();
    afx_msg void OnBnClickedResolveusingmine();
    afx_msg void OnBnClickedShowlog();
    afx_msg void OnBnClickedBranchlog();
    afx_msg void OnBnClickedHelp();
    afx_msg void OnBnClickedPostponeAll();
    afx_msg void OnBnClickedAbort();

private:
    CString GetShowLogCmd(const CTSVNPath &logPath, const CString &sFile);

    svn_wc_conflict_choice_t    m_theirsChoice;
    svn_wc_conflict_choice_t    m_mineChoice;
    CTSVNPath                   m_path;
    CTSVNPath                   m_copyfromPath;
    bool                        m_bInteractive;
    svn_wc_conflict_reason_t    conflict_reason;
    svn_wc_conflict_action_t    conflict_action;
    svn_wc_operation_t          conflict_operation;
    svn_node_kind_t             kind;
    svn_wc_conflict_choice_t    m_choice;
    bool                        m_bCancelled;


    CString                     src_right_version_url;
    CString                     src_right_version_path;
    SVNRev                      src_right_version_rev;
    svn_node_kind_t             src_right_version_kind;
    CString                     src_left_version_url;
    CString                     src_left_version_path;
    SVNRev                      src_left_version_rev;
    svn_node_kind_t             src_left_version_kind;
    CPathEdit                   src_leftedit;
    CPathEdit                   src_rightedit;
};
