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
 * Settings page for the icon overlays.
 *
 * \par requirements
 * win95 or later\n
 * winNT4 or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 04-14-2003
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
class CSetOverlayPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetOverlayPage)

public:
	CSetOverlayPage();   // standard constructor
	virtual ~CSetOverlayPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	UINT GetIconID() {return IDI_SET_OVERLAYS;}

// Dialog Data
	enum { IDD = IDD_SETTINGSOVERLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

private:
	BOOL			m_bOnlyExplorer;
	BOOL			m_bRemovable;
	BOOL			m_bNetwork;
	BOOL			m_bFixed;
	BOOL			m_bCDROM;
	BOOL			m_bRAM;
	BOOL			m_bUnknown;
	CRegDWORD		m_regOnlyExplorer;
	CRegDWORD		m_regDriveMaskRemovable;
	CRegDWORD		m_regDriveMaskRemote;
	CRegDWORD		m_regDriveMaskFixed;
	CRegDWORD		m_regDriveMaskCDROM;
	CRegDWORD		m_regDriveMaskRAM;
	CRegDWORD		m_regDriveMaskUnknown;
	CBalloon		m_tooltips;
	BOOL			m_bInitialized;
	CRegString		m_regExcludePaths;
	CString			m_sExcludePaths;
	CRegString		m_regIncludePaths;
	CString			m_sIncludePaths;
	CRegDWORD		m_regRecursive;
	BOOL			m_bRecursive;

	BOOL			m_bModified;

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedRemovable();
	afx_msg void OnBnClickedNetwork();
	afx_msg void OnBnClickedFixed();
	afx_msg void OnBnClickedCdrom();
	afx_msg void OnBnClickedRam();
	afx_msg void OnBnClickedUnknown();
	virtual BOOL OnApply();
	afx_msg void OnBnClickedOnlyexplorer();
	afx_msg void OnEnChangeExcludepaths();
	afx_msg void OnEnChangeIncludepaths();
	afx_msg void OnBnClickedRecursivecheck();
};
