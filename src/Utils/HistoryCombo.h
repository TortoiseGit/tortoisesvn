// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#pragma once

#ifndef __AFXADV_H__
#include "afxadv.h" // needed for CRecentFileList
#endif 

/**
 * \ingroup Utils
 * Extends the CComboBox class with a history of entered
 * values. An example of such a combobox is the Start/Run
 * dialog which lists the programs you used last in a combobox.
 * To use this class do the following:
 * -# add both files HistoryCombo.h and HistoryCombo.cpp to your project.
 * -# add a ComboBox to your dialog
 * -# create a variable for the ComboBox of type control
 * -# change the type of the created variable from CComboBox to
 *   CHistoryCombo
 * -# in your OnInitDialog() call the LoadHistory() method
 * -# in your OnOK() or somewhere similar call the SaveHistory() method
 * 
 * thats it. 
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 01-26-2003
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class CHistoryCombo : public CComboBox
{
// Construction
public:
	CHistoryCombo(BOOL bAllowSortStyle = FALSE);

// Operations
public:
	/**
	 * Overloaded from base class. It adds the string not only
	 * to the combobox but also to the history.
	 */
	int AddString(LPCTSTR lpszString);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHistoryCombo)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	/**
	 * Clears the history in the registry/inifile and the ComboBox.
	 * \param bDeleteRegistryEntries if this value is true then the registry key
	 * itself is deleted.
	 */
	void ClearHistory(BOOL bDeleteRegistryEntries = TRUE);
	/**
	 * Sets the maximum numbers of entries in the history list.
	 * If the history is larger as \em nMaxItems then the last
	 * items in the history are deleted.
	 */
	void SetMaxHistoryItems(int nMaxItems);
	/**
	 * Saves the history to the registry/inifile.
	 * \remark if you haven't called LoadHistory() before this method
	 * does nothing!
	 */
	void SaveHistory();
	/**
	 * Loads the history from the registry/inifile and fills in the
	 * ComboBox.
	 * \param lpszSection a section name where to put the entries, e.g. "lastloadedfiles"
	 * \param lpszKeyPrefix a prefix to use for the history entries in registry/inifiles. E.g. "file" or "entry"
	 */
	CString LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix);

	/**
	 * Returns the string in the combobox which is either selected or the user has entered.
	 */
	CString GetString();

	virtual ~CHistoryCombo();

protected:
	CString m_sSection;
	CString m_sKeyPrefix;
	int m_nMaxHistoryItems;
	BOOL m_bAllowSortStyle;


	DECLARE_MESSAGE_MAP()
};



