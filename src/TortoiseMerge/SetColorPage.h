// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

#include "ColourPickerXP.h"
#include "registry.h"


/**
 * \ingroup TortoiseMerge
 * Color settings page
 */
class CSetColorPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetColorPage)

public:
	CSetColorPage();
	virtual ~CSetColorPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	BOOL	m_bReloadNeeded;
// Dialog Data
	enum { IDD = IDD_SETCOLORPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	afx_msg LRESULT OnSelEndOK(WPARAM lParam, LPARAM wParam);
	DECLARE_MESSAGE_MAP()

protected:
	BOOL m_bInit;
	CRegDWORD		m_regInlineAdded;
	CRegDWORD		m_regInlineRemoved;
	CRegDWORD		m_regModifiedBackground;
	CColourPickerXP m_cBkNormal;
	CColourPickerXP m_cBkRemoved;
	CColourPickerXP m_cBkAdded;
	CColourPickerXP m_cBkInlineAdded;
	CColourPickerXP m_cBkInlineRemoved;
	CColourPickerXP m_cBkEmpty;
	CColourPickerXP m_cBkConflict;
	CColourPickerXP m_cBkConflictResolved;
	CColourPickerXP m_cBkModified;
};
