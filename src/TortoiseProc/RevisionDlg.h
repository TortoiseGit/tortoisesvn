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


// For base class
#include "SVNRev.h"
#include "StandAloneDlg.h"

/**
 * \ingroup TortoiseProc
 * A simple dialog box asking the user for a revision number.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 01-27-2003
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
class CRevisionDlg : public CDialog, public SVNRev
{
	DECLARE_DYNAMIC(CRevisionDlg)

public:
	CRevisionDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevisionDlg();

// Dialog Data
	enum { IDD = IDD_REVISION };
	CString GetEnteredRevisionString() {return m_sRevision;}
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnEnChangeRevnum();

	DECLARE_MESSAGE_MAP()

	CString m_sRevision;
};
