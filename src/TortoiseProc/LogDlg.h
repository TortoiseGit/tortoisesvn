// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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

#pragma once
#include "afxcmn.h"
#include "afxtempl.h"
#include "afxdlgs.h"
#include "svn.h"
#include "promptdlg.h"
#include "ProjectProperties.h"
#include "Registry.h"
#include "ResizableDialog.h"

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

#define ID_DIFF			20

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
class CLogDlg : public CResizableDialog, public SVN
{
	DECLARE_DYNAMIC(CLogDlg)

public:
	CLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLogDlg();

// Dialog Data
	enum { IDD = IDD_LOGMESSAGE };

protected:
	//implement the virtual methods from SVN base class
	virtual BOOL Log(LONG rev, const CString& author, const CString& date, const CString& message, const CString& cpaths);
	virtual BOOL Cancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnPaint();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedGetall();
	afx_msg void OnNMDblclkLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void	FillLogMessageCtrl(const CString& msg, const CString& paths);
	BOOL	StartDiff(CString path1, LONG rev1, CString path2, LONG rev2);
	void	DoDiffFromLog(int selIndex, long rev);

	DECLARE_MESSAGE_MAP()
public:
	void SetParams(CString path, long startrev = 0, long endrev = -1, BOOL bStrict = FALSE);

public:
	CWnd *		m_pNotifyWindow;
	WORD		m_wParam;
	CListCtrl	m_LogList;
	CListCtrl	m_LogMsgCtrl;
	CProgressCtrl m_LogProgress;
	CString		m_path;
	long		m_startrev;
	long		m_endrev;
	long		m_logcounter;
	BOOL		m_bCancelled;
	BOOL		m_bShowedAll;
	BOOL		m_bThreadRunning;
	BOOL		m_bStrict;
	BOOL		m_bGotRevisions;
	ProjectProperties m_ProjectProperties;
private:
	HICON		m_hIcon;
	HANDLE		m_hThread;
	CStringArray m_arLogMessages;
	CStringArray m_arLogPaths;
	CDWordArray m_arRevs;
	BOOL		m_hasWC;
	BOOL		m_bGotAllPressed;
	int			m_nSearchIndex;
	static const UINT m_FindDialogMessage;
	CFindReplaceDialog *m_pFindDialog;
	CStringArray	m_templist;
	CFont		m_logFont;
	CString		m_sMessageBuf;
};

DWORD WINAPI LogThread(LPVOID pVoid);
