// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
	virtual BOOL Log(LONG rev, CString author, CString date, CString message, CString& cpaths);
	virtual BOOL Cancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnNMClickLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTipLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	void	FillLogMessageCtrl(CString msg);
	BOOL	StartDiff(CString path1, LONG rev1, CString path2, LONG rev2);

	DECLARE_MESSAGE_MAP()
public:
	void SetParams(CString path, long startrev = 0, long endrev = -1, BOOL hasWC = TRUE);

public:
	CListCtrl	m_LogList;
	CListCtrl	m_LogMsgCtrl;
	CString		m_path;
	long		m_startrev;
	long		m_endrev;
	long		m_logcounter;
	BOOL		m_bCancelled;
	BOOL		m_bShowedAll;
private:
	HICON		m_hIcon;
	HANDLE		m_hThread;
	CStringArray m_arLogMessages;
	CDWordArray m_arFileListStarts;
	CDWordArray m_arRevs;
	BOOL		m_hasWC;
	int			m_nSearchIndex;
	static const UINT m_FindDialogMessage;
	CFindReplaceDialog *m_pFindDialog;
	CStringArray	m_templist;
public:
//	afx_msg void OnLvnItemActivateLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangingLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedGetall();
protected:
	virtual void OnCancel();
public:
	afx_msg void OnNMDblclkLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult);
};

DWORD WINAPI LogThread(LPVOID pVoid);
