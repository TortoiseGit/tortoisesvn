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
//
#pragma once
#include "afxcmn.h"


// CSetMenuPage dialog

class CSetMenuPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetMenuPage)

public:
	CSetMenuPage();
	virtual ~CSetMenuPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	virtual BOOL OnApply();

// Dialog Data
	enum { IDD = IDD_SETTINGSMENU };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	void InsertItem(UINT nTextID, UINT nIconID, DWORD dwFlags);
	DECLARE_MESSAGE_MAP()

	afx_msg void OnLvnItemchangedMenulist(NMHDR *pNMHDR, LRESULT *pResult);

	CImageList m_imgList;
	CListCtrl m_cMenuList;
	BOOL m_bModified;
	DWORD m_topmenu;
};
