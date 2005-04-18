// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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

#include "svn.h"
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
#include "TSVNPath.h"
#include "SplitterControl.h"
#include "afxwin.h"
#include "afxdtctl.h"

#define ID_COMPARE		1
#define ID_SAVEAS		2
#define ID_COMPARETWO	3
#define ID_UPDATE		4
#define ID_COPY			5
#define ID_REVERTREV	6
#define ID_GNUDIFF1		7
#define ID_GNUDIFF2		8
#define ID_FINDENTRY	9
#define ID_REVERT	   10
#define	ID_REFRESH	   11
#define ID_OPEN		   12
#define ID_REPOBROWSE  13
#define ID_DELETE	   14
#define ID_IGNORE	   15
#define	ID_LOG		   16
#define ID_POPPROPS	   17
#define ID_EDITAUTHOR  18
#define ID_EDITLOG     19

#define ID_DIFF			20
#define ID_EDITCONFLICT	21

#define LOGFILTER_ALL      1
#define LOGFILTER_MESSAGES 2
#define LOGFILTER_PATHS    3
#define LOGFILTER_AUTHORS  4


#define LOGFILTER_TIMER		101

/**
 * \ingroup TortoiseProc
 * Shows log messages of a single file or folder in a listbox. 
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.1
 * log messages have the modified/added/removed/moved files listed
 * at the bottom.
 * \version 1.0
 * first version
 *
 * \date 10-20-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CLogDlg : public CResizableStandAloneDialog, public SVN //CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CLogDlg)

public:
	CLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLogDlg();

// Dialog Data
	enum { IDD = IDD_LOGMESSAGE };

protected:
	//implement the virtual methods from SVN base class
	virtual BOOL Log(LONG rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies);
	virtual BOOL Cancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedGetall();
	afx_msg void OnNMDblclkLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedStatbutton();
	afx_msg void OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnClickedFiltericon();
	afx_msg void OnEnChangeSearchedit();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDtnDatetimechangeDateto(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDatetimechangeDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void	FillLogMessageCtrl(const CString& msg, LogChangedPathArray * paths);
	BOOL	StartDiff(const CTSVNPath& path1, LONG rev1, const CTSVNPath& path2, LONG rev2);
	void	DoDiffFromLog(int selIndex, long rev);

	DECLARE_MESSAGE_MAP()
public:
	void SetParams(const CTSVNPath& path, long startrev, long endrev, int limit, BOOL bStrict = FALSE);

private:
	static UINT LogThreadEntry(LPVOID pVoid);
	UINT LogThread();
	BOOL DiffPossible(LogChangedPath * changedpath, long rev);
	void EditAuthor(int index);
	void EditLogMessage(int index);
	void DoSizeV1(int delta);
	void DoSizeV2(int delta);
	void SetSplitterRange();
	void SetFilterCueText();
	BOOL IsEntryInDateRange(int i);
	
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
	CWnd *				m_pNotifyWindow;
	ProjectProperties	m_ProjectProperties;
	WORD				m_wParam;
private:
	CListCtrl			m_LogList;
	CListCtrl			m_LogMsgCtrl;
	CProgressCtrl		m_LogProgress;
	CTSVNPath			m_path;
	long				m_startrev;
	long				m_endrev;
	long				m_logcounter;
	BOOL				m_bCancelled;
	BOOL				m_bThreadRunning;
	BOOL				m_bStrict;
	CStringArray		m_arLogMessages;
	CStringArray		m_arShortLogMessages;
	CArray<LogChangedPathArray*, LogChangedPathArray*> m_arLogPaths;
	CDWordArray			m_arDates;
	CStringArray		m_arDateStrings;
	CStringArray		m_arAuthors;
	CDWordArray			m_arFileChanges;
	CDWordArray			m_arRevs;
	CDWordArray			m_arCopies;
	CDWordArray			m_arShownList;
	BOOL				m_hasWC;
	int					m_nSearchIndex;
	static const UINT	m_FindDialogMessage;
	CFindReplaceDialog *m_pFindDialog;
	CTSVNPathList		m_tempFileList;
	CFont				m_logFont;
	CString				m_sMessageBuf;
	CSplitterControl	m_wndSplitter1;
	CSplitterControl	m_wndSplitter2;
	CString				m_sFilterText;
	int					m_nSelectedFilter;
	bool				m_bNoDispUpdates;
	CDateTimeCtrl		m_DateFrom;
	CDateTimeCtrl		m_DateTo;
	DWORD				m_tFrom;
	DWORD				m_tTo;
	int					m_limit;
	int					m_limitcounter;
public:
	afx_msg void OnBnClickedNexthundred();
};
static UINT WM_REVSELECTED = RegisterWindowMessage(_T("TORTOISESVN_REVSELECTED_MSG"));

