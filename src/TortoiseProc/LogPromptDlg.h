// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

#include "StandAloneDlg.h"
#include "Balloon.h"
#include "SVNStatusListCtrl.h"
#include "ProjectProperties.h"
#include "HistoryDlg.h"
#include "Registry.h"
#include "SciEdit.h"
#include "SplitterControl.h"

#define ENDDIALOGTIMER 100
#define REFRESHTIMER   101


/**
 * \ingroup TortoiseProc
 * Dialog to enter log messages used in a commit.
 */
class CLogPromptDlg : public CResizableStandAloneDialog, public CSciEditContextMenuInterface // CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CLogPromptDlg)

public:
	CLogPromptDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLogPromptDlg();

	// CSciEditContextMenuInterface
	virtual void		InsertMenuItems(CMenu& mPopup, int& nCmd);
	virtual bool		HandleMenuItemClick(int cmd, CSciEdit * pSciEdit);

private:
	static UINT StatusThreadEntry(LPVOID pVoid);
	UINT StatusThread();
	void UpdateOKButton();

// Dialog Data
	enum { IDD = IDD_LOGPROMPT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedShowunversioned();
	afx_msg void OnBnClickedHistory();
	afx_msg void OnEnChangeLogmessage();
	afx_msg LRESULT OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	afx_msg LRESULT OnAutoListReady(WPARAM, LPARAM);
	afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLvnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	void Refresh();
	void GetAutocompletionList();
	void ScanFile(const CString& sFilePath, const CString& sRegex, REGEX_FLAGS rflags);
	void DoSize(int delta);
	void SetSplitterRange();

	DECLARE_MESSAGE_MAP()


public:
	CTSVNPathList		m_pathList;
	BOOL				m_bRecursive;
	CSciEdit			m_cLogMessage;
	CString				m_sLogMessage;
	BOOL				m_bKeepLocks;
	CString				m_sBugID;

private:
	CWinThread*			m_pThread;
	CAutoCompletionList	m_autolist;
	CSVNStatusListCtrl	m_ListCtrl;
	BOOL				m_bShowUnversioned;
	volatile LONG		m_bBlock;
	volatile LONG		m_bThreadRunning;
	volatile LONG		m_bRunThread;
	CBalloon			m_tooltips;
	CRegDWORD			m_regAddBeforeCommit;
	ProjectProperties	m_ProjectProperties;
	CButton				m_SelectAll;
	CString				m_sWindowTitle;
	static UINT			WM_AUTOLISTREADY;
	int					m_nPopupPasteListCmd;
	CHistoryDlg			m_HistoryDlg;
	bool				m_bCancelled;
	CSplitterControl	m_wndSplitter;
};
