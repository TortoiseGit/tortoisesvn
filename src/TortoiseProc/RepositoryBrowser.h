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
#include "repositorytree.h"
#include "ResizableDialog.h"


#define ID_POPSAVEAS		1
#define ID_POPSHOWLOG		2
#define	ID_POPOPEN			3

/**
 * \ingroup TortoiseProc
 * Dialog to browse a repository.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 02-07-2003
 *
 * \author Tim Kemp
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
class CRepositoryBrowser : public CResizableDialog
{
	DECLARE_DYNAMIC(CRepositoryBrowser)

public:
	CRepositoryBrowser(const CString& strUrl, CWnd* pParent = NULL);   // standard constructor
	virtual ~CRepositoryBrowser();

// Dialog Data
	enum { IDD = IDD_REPOSITORY_BROWSER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	HICON m_hIcon;

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	CRepositoryTree m_treeRepository;
	virtual BOOL OnInitDialog();
	afx_msg void OnTvnSelchangedReposTree(NMHDR *pNMHDR, LRESULT *pResult);
	CString m_strUrl;
	afx_msg void OnNMRclickReposTree(NMHDR *pNMHDR, LRESULT *pResult);
};
