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
#include "afxwin.h"


/**
 * \ingroup TortoiseProc
 * Shows a dialog to prompt the user for an URL of a branch and a revision
 * number to switch the working copy to. Also has a checkbox to
 * specify the current branch instead of a different branch url and
 * one checkbox to specify the newest revision.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 11-08-2002
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
 * \todo the dialog looks ugly now. This needs definitely an improvement!
 *
 * \bug 
 *
 */
class CSwitchDlg : public CDialog
{
	DECLARE_DYNAMIC(CSwitchDlg)

public:
	CSwitchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSwitchDlg();

// Dialog Data
	enum { IDD = IDD_SWITCH };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	CString m_path;
	CString m_URL;
	CString m_rev;
	afx_msg void OnEnUpdateRev();
	afx_msg void OnBnClickedNewest();
	BOOL m_IsNewest;
	CEdit m_revctrl;
	afx_msg void OnBnClickedBrowse();
};
