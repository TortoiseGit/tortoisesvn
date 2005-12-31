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
#include "SVNStatusListCtrl.h"

/**
 * \ingroup TortoiseProc
 * a simple dialog to show the user all unversioned
 * files below a specified folder.
 *
 * \par requirements
 * win95 or later\n
 * winNT4 or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 03-06-2003
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CAddDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CAddDlg)

public:
	CAddDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAddDlg();

// Dialog Data
	enum { IDD = IDD_ADD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHelp();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

private:
	static UINT AddThreadEntry(LPVOID pVoid);
	UINT AddThread();

	DECLARE_MESSAGE_MAP()

public:
	CTSVNPathList	m_pathList;

private:
	CSVNStatusListCtrl	m_addListCtrl;
	volatile bool	m_bThreadRunning;
	CButton			m_SelectAll;
};

