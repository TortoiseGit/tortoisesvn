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

#include "SVNStatus.h"

#define WMU_SHELLNOTIFY (WM_APP + 1)

typedef struct 
{
	DWORD dwItem1;
	DWORD dwItem2;
} SHNOTIFYSTRUCT;


/**
 * \ingroup TortoiseProc
 * Helper dialog for watching changes in the filesystem. As soon
 * as a change is monitored the corresponding action (delete/rename)
 * is done in the subversion working copy.
 * The dialog itself is never shown - it is only used to catch windows messages.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 11-01-2002
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class CWatcherDlg : public CDialog, public SVN
{
	DECLARE_DYNAMIC(CWatcherDlg)

public:
	CWatcherDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWatcherDlg();

// Dialog Data
	enum { IDD = IDD_WATCHER };

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg LRESULT OnShellNotify(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	CString GetPathFromPIDL(DWORD pidl);
protected:
	virtual void PostNcDestroy();
private:
	UINT	m_uID;
	CString m_before;
	CString m_after;
};
