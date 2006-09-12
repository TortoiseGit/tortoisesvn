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
 * Settings page to configure how the icon overlays and the cache should
 * behave.
 */
class CSetOverlayPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetOverlayPage)

public:
	CSetOverlayPage();
	virtual ~CSetOverlayPage();
	/**
	 * Saves the changed settings to the registry.
	 * returns 0 if no restart is needed for the changes to take effect
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	int SaveData();

	UINT GetIconID() {return IDI_SET_OVERLAYS;}

// Dialog Data
	enum { IDD = IDD_SETTINGSOVERLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnChange();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

private:
	BOOL			m_bOnlyExplorer;
	BOOL			m_bRemovable;
	BOOL			m_bFloppy;
	BOOL			m_bNetwork;
	BOOL			m_bFixed;
	BOOL			m_bCDROM;
	BOOL			m_bRAM;
	BOOL			m_bUnknown;
	BOOL			m_bUnversionedAsModified;
	CRegDWORD		m_regOnlyExplorer;
	CRegDWORD		m_regDriveMaskRemovable;
	CRegDWORD		m_regDriveMaskFloppy;
	CRegDWORD		m_regDriveMaskRemote;
	CRegDWORD		m_regDriveMaskFixed;
	CRegDWORD		m_regDriveMaskCDROM;
	CRegDWORD		m_regDriveMaskRAM;
	CRegDWORD		m_regDriveMaskUnknown;
	CRegDWORD		m_regUnversionedAsModified;
	CBalloon		m_tooltips;
	BOOL			m_bInitialized;
	CRegString		m_regExcludePaths;
	CString			m_sExcludePaths;
	CRegString		m_regIncludePaths;
	CString			m_sIncludePaths;
	CRegDWORD		m_regCacheType;
	DWORD			m_dwCacheType;

	BOOL			m_bModified;
};
