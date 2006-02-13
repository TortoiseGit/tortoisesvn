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
#include "RevisionGraph.h"
#include "ProgressDlg.h"
#include "Colors.h"
#include "RevisionGraphWnd.h"

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
class CRevisionGraphDlg : public CDialog
{
	DECLARE_DYNAMIC(CRevisionGraphDlg)
public:
	CRevisionGraphDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevisionGraphDlg();
	enum { IDD = IDD_REVISIONGRAPH };


	CProgressDlg* 	m_pProgress;
	void			SetPath(const CString& sPath) {m_Graph.m_sPath = sPath;}
protected:
	bool			m_bFetchLogs;
	bool			m_bShowAll;
	bool			m_bArrangeByPath;
	float			m_fZoomFactor;
	
	HACCEL			m_hAccel;

	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	ProgressCallback(CString text, CString text2, DWORD done, DWORD total);
	virtual BOOL	OnInitDialog();
	virtual void	OnCancel();
	virtual void	OnOK();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void	OnViewZoomin();
	afx_msg void	OnViewZoomout();
	afx_msg void	OnViewZoom100();
	afx_msg void	OnViewZoomAll();
	afx_msg void	OnMenuexit();
	afx_msg void	OnMenuhelp();
	afx_msg void	OnViewCompareheadrevisions();
	afx_msg void	OnViewComparerevisions();
	afx_msg void	OnViewUnifieddiff();
	afx_msg void	OnViewUnifieddiffofheadrevisions();
	afx_msg void	OnViewShowallrevisions();
	afx_msg void	OnViewArrangedbypath();
	afx_msg void	OnFileSavegraphas();
	afx_msg void	OnChangeZoom();

	DECLARE_MESSAGE_MAP()

	void			GetGraphRect(LPRECT rect);
	void			UpdateStatusBar();

private:
	static UINT		WorkerThread(LPVOID pVoid);
	CRevisionGraphWnd	m_Graph;
	CStatusBarCtrl		m_StatusBar;
	CRevGraphToolBar	m_ToolBar;
protected:
};
