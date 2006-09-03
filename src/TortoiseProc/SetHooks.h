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
#include "afxcmn.h"


// CSetHooks dialog

class CSetHooks : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetHooks)

public:
	CSetHooks();   // standard constructor
	virtual ~CSetHooks();

	/**
	 * Saves the changed settings to the registry.
	 * returns 0 if no restart is needed for the changes to take effect
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	int SaveData();

	UINT GetIconID() {return IDI_HOOK;}

// Dialog Data
	enum { IDD = IDD_SETTINGSHOOKS };

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedRemovebutton();
	afx_msg void OnBnClickedEditbutton();
	afx_msg void OnBnClickedAddbutton();
	afx_msg void OnLvnItemchangedHooklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkHooklist(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

	void			RebuildHookList();
protected:
	CListCtrl m_cHookList;
};
