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
#include "afxwin.h"

#include "ColourPickerXP.h"
#include "Colors.h"

/**
 * \ingroup TortoiseProc
 * Settings property page to set custom colors used in TortoiseSVN
 */
class CSettingsColors : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingsColors)

public:
	CSettingsColors();
	virtual ~CSettingsColors();

	void SaveData();

	UINT GetIconID() {return IDI_LOOKANDFEEL;}

// Dialog Data
	enum { IDD = IDD_SETTINGSCOLORS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnColorChanged(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private:
	CColourPickerXP m_cConflict;
	CColourPickerXP m_cAdded;
	CColourPickerXP m_cDeleted;
	CColourPickerXP m_cMerged;
	CColourPickerXP m_cModified;
	CColourPickerXP m_cAddedNode;
	CColourPickerXP m_cDeletedNode;
	CColourPickerXP m_cRenamedNode;
	CColourPickerXP m_cReplacedNode;
	CColors			m_Colors;
	
	bool bInit;
public:
	afx_msg void OnBnClickedRestore();
};
