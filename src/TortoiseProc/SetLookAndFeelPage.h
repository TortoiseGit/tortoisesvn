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

#include "Balloon.h"
#include "Registry.h"

/**
 * \ingroup TortoiseProc
 * Settings page look and feel.
 */
class CSetLookAndFeelPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetLookAndFeelPage)

public:
	CSetLookAndFeelPage();
	virtual ~CSetLookAndFeelPage();
	/**
	 * Saves the changed settings to the registry.
	 * returns 0 if no restart is needed for the changes to take effect
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	int SaveData();

	UINT GetIconID() {return IDI_MISC;}

// Dialog Data
	enum { IDD = IDD_SETTINGSLOOKANDFEEL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnBnClickedOnlyexplorer();
	afx_msg void OnLvnItemchangedMenulist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnChange();

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

private:
	void InsertItem(UINT nTextID, UINT nIconID, DWORD dwFlags);

	CBalloon		m_tooltips;
	BOOL			m_bInitialized;
	CRegDWORD		m_regTopmenu;

	CImageList		m_imgList;
	CListCtrl		m_cMenuList;
	BOOL			m_bModified;
	DWORD			m_topmenu;
	
	CRegDWORD		m_regSimpleContext;
	BOOL			m_bSimpleContext;
	
	CRegDWORD		m_regOwnerDrawn;
	int				m_OwnerDrawn;
};
