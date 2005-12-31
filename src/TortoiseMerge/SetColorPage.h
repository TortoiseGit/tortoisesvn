// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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

#include "ColourPickerXP.h"

// CSetColorPage dialog

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
	CColourPickerXP m_cBkNormal;
	CColourPickerXP m_cBkRemoved;
	CColourPickerXP m_cBkAdded;
	CColourPickerXP m_cBkWhitespaces;
	CColourPickerXP m_cBkWhitespaceDiff;
	CColourPickerXP m_cBkEmpty;
	CColourPickerXP m_cBkConflict;
};
