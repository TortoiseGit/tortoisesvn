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

#include "SetMainPage.h"
#include "SetProxyPage.h"
#include "SetOverlayPage.h"
#include "SettingsProgsDiff.h"
#include "SettingsProgsMerge.h"
#include "SettingsProgsUniDiff.h"
#include "SetOverlayIcons.h"
#include "SetLookAndFeelPage.h"
#include "SetDialogs.h"
#include "SettingsColors.h"
#include "SetMisc.h"
#include "TreePropSheet/TreePropSheet.h"

using namespace TreePropSheet;

/**
 * \ingroup TortoiseProc
 * This is the container for all settings pages. A setting page is
 * a class derived from CPropertyPage with an additional method called
 * SaveData(). The SaveData() method is used by the dialog to save
 * the settings the user has made - if that method is not called then
 * it means that the changes are discarded! Each settings page has
 * to make sure that no changes are saved outside that method.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 11-28-2002
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
class CSettings : public CTreePropSheet
{
	DECLARE_DYNAMIC(CSettings)
private:
	/**
	 * Adds all pages to this Settings-Dialog.
	 */
	void AddPropPages();
	/**
	 * Removes the pages and frees up memory.
	 */
	void RemovePropPages();

private:
	CSetMainPage *			m_pMainPage;
	CSetProxyPage *			m_pProxyPage;
	CSetOverlayPage *		m_pOverlayPage;
	CSetOverlayIcons *		m_pOverlaysPage;
	CSettingsProgsDiff*		m_pProgsDiffPage;
	CSettingsProgsMerge *	m_pProgsMergePage;
	CSettingsProgsUniDiff * m_pProgsUniDiffPage;
	CSetLookAndFeelPage *	m_pLookAndFeelPage;
	CSetDialogs *			m_pDialogsPage;
	CSettingsColors *		m_pColorsPage;
	CSetMisc *				m_pMiscPage;

	HICON					m_hIcon;
public:
	CSettings(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CSettings();

	/**
	 * Calls the SaveData()-methods of each of the settings pages.
	 */
	void SaveData();
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
};


