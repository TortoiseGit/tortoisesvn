// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "svn.h"
#include "promptdlg.h"

#include "Registry.h"
#include "ResizableDialog.h"

#define ID_COMPARE		1
#define ID_SAVEAS		2
#define ID_COMPARETWO	3
#define ID_UPDATE		4

/**
 * \ingroup TortoiseProc
 * Shows log messages of a single file in a listbox. Used by the property sheet handler of TortoiseShell.
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
 * 
 * \todo add possibility to specify how many log entries are shown
 * \todo show the log messages in a text edit control below the listcontrol
 *	     when the user selects a specific entry to avoid an overloaded
 *       listcontrol
 *
 * \bug 
 *
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
	virtual BOOL Log(LONG rev, CString author, CString date, CString message, CString cpaths);
	virtual BOOL Cancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	HICON m_hIcon;

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMClickLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL OnInitDialog();
	void SetParams(CString path, long startrev = 0, long endrev = -1, BOOL hasWC = TRUE);

public:
	CListCtrl	m_LogList;
	CString		m_path;
	long		m_startrev;
	long		m_endrev;
	long		m_logcounter;
	BOOL		m_bCancelled;
private:
	HANDLE		m_hThread;
	CString		m_sLogMsgCtrl;
	CStringArray m_arLogMessages;
	CDWordArray m_arRevs;
	BOOL		m_hasWC;
public:
	afx_msg void OnNMRclickLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	virtual void OnOK();
public:
	afx_msg void OnLvnGetInfoTipLoglist(NMHDR *pNMHDR, LRESULT *pResult);
};

DWORD WINAPI LogThread(LPVOID pVoid);
