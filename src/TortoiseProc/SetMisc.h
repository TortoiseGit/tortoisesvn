#pragma once


// CSetMisc dialog

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
#include "..\\Utils\\Balloon.h"
#include "Registry.h"

class CSetMisc : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetMisc)

public:
	CSetMisc();
	virtual ~CSetMisc();
	/**
	  * Saves the changed settings to the registry.
	  * \remark If the dialog is closed/dismissed without calling
	  * this method first then all settings the user made must be
	  * discarded!
	  */
	void SaveData();
	
	UINT GetIconID() {return IDI_MISC;}

// Dialog Data
	enum { IDD = IDD_SETTINGSMISC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnChanged();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
private:
	CBalloon		m_tooltips;

	BOOL			m_bUnversionedRecurse;
	CRegDWORD		m_regUnversionedRecurse;
};
