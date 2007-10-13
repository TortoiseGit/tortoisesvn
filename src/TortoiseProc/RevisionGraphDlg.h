// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "RevisionGraph.h"
#include "ProgressDlg.h"
#include "Colors.h"
#include "RevisionGraphWnd.h"
#include "StandAloneDlg.h"

#define	MAX_TT_LENGTH			10000

/**
 * \ingroup TortoiseProc
 * Helper class extending CToolBar, needed only to have the toolbar include
 * a combobox.
 */
class CRevGraphToolBar : public CToolBar
{
public:
	CComboBoxEx		m_ZoomCombo;
};

/**
 * \ingroup TortoiseProc
 * A dialog showing a revision graph.
 *
 * The analyzation of the log data is done in the child class CRevisionGraph,
 * the drawing is done in the member class CRevisionGraphWnd
 * Here, we handle window messages.
 */
class CRevisionGraphDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRevisionGraphDlg)
public:
	CRevisionGraphDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevisionGraphDlg();
	enum { IDD = IDD_REVISIONGRAPH };


	void			SetPath(const CString& sPath) {m_Graph.m_sPath = sPath;}
	void			UpdateZoomBox();
	float			m_fZoomFactor;
protected:
	bool			m_bFetchLogs;
    CRevisionGraph::SOptions m_options;
	char			m_szTip[MAX_TT_LENGTH+1];
	wchar_t			m_wszTip[MAX_TT_LENGTH+1];

	CString			m_sFilter;
	
	HACCEL			m_hAccel;

	BOOL			InitializeToolbar();

	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnInitDialog();
	virtual void	OnCancel();
	virtual void	OnOK();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void	OnViewFilter();
	afx_msg void	OnViewZoomin();
	afx_msg void	OnViewZoomout();
	afx_msg void	OnViewZoom100();
	afx_msg void	OnViewZoomAll();
	afx_msg void	OnViewCompareheadrevisions();
	afx_msg void	OnViewComparerevisions();
	afx_msg void	OnViewUnifieddiff();
	afx_msg void	OnViewUnifieddiffofheadrevisions();
	afx_msg void	OnViewShowallrevisions();
	afx_msg void	OnViewArrangedbypath();
	afx_msg void	OnViewTopDown();
	afx_msg void    OnViewShowHEAD();
	afx_msg void    OnViewExactCopySource();
	afx_msg void    OnViewSplitBranches();
	afx_msg void    OnViewFoldTags();
	afx_msg void    OnViewReduceCrosslines();
	afx_msg void	OnViewShowoverview();
	afx_msg void	OnFileSavegraphas();
	afx_msg void	OnMenuexit();
	afx_msg void	OnMenuhelp();
	afx_msg void	OnChangeZoom();
	afx_msg BOOL	OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

	void			SetOption(int controlID, bool option);
    void            OnToggleOption(int controlID, bool& option);

    void			GetGraphRect(LPRECT rect);
	void			UpdateStatusBar();

private:
	static UINT		WorkerThread(LPVOID pVoid);
	CRevisionGraphWnd	m_Graph;
	CStatusBarCtrl		m_StatusBar;
	CRevGraphToolBar	m_ToolBar;
};
