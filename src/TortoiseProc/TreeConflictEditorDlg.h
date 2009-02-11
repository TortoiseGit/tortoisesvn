// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseSVN

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

#include "TSVNPath.h"
#include "ProgressDlg.h"

class CTreeConflictEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CTreeConflictEditorDlg)

public:
	CTreeConflictEditorDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTreeConflictEditorDlg();

	void SetConflictInfoText(const CString& info) {m_sConflictInfo = info;}
	void SetResolveTexts(const CString& usetheirs, const CString& usemine) {m_sUseTheirs = usetheirs; m_sUseMine = usemine;}
	void SetPath(const CTSVNPath& path) {m_path = path;}
	void SetConflictSources(svn_wc_conflict_version_t * left, svn_wc_conflict_version_t * right) {src_left = left; src_right = right;}
	void SetConflictAction(svn_wc_conflict_action_t action) {conflict_action = action;}
	void SetConflictReason(svn_wc_conflict_reason_t reason) {conflict_reason = reason;}

// Dialog Data
	enum { IDD = IDD_TREECONFLICTEDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	static UINT StatusThreadEntry(LPVOID pVoid);
	UINT StatusThread();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedResolveusingtheirs();
	afx_msg void OnBnClickedResolveusingmine();

	/// called after the thread has finished
	LRESULT OnAfterThread(WPARAM /*wParam*/, LPARAM /*lParam*/);

private:
	CProgressDlg		m_progressDlg;

	volatile LONG 		m_bThreadRunning;
	CString				m_sConflictInfo;
	CString				m_sUseTheirs;
	CString				m_sUseMine;
	CTSVNPath			m_path;
	CTSVNPath			m_copyfromPath;
	svn_wc_conflict_version_t *	src_left;
	svn_wc_conflict_version_t *	src_right;
	svn_wc_conflict_reason_t conflict_reason;
	svn_wc_conflict_action_t conflict_action;
public:
	afx_msg void OnBnClickedShowlog();
};

static UINT WM_AFTERTHREAD = RegisterWindowMessage(_T("TORTOISESVN_AFTERTHREAD_MSG"));
