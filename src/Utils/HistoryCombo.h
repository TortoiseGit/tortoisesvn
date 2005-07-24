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

/**
 * \ingroup Utils
 * Extends the CComboBoxEx class with a history of entered
 * values. An example of such a combobox is the Start/Run
 * dialog which lists the programs you used last in a combobox.
 * To use this class do the following:
 * -# add both files HistoryCombo.h and HistoryCombo.cpp to your project.
 * -# add a ComboBoxEx to your dialog
 * -# create a variable for the ComboBox of type control
 * -# change the type of the created variable from CComboBoxEx to
 *    CHistoryCombo
 * -# in your OnInitDialog() call SetURLHistory(TRUE) if your ComboBox
 *    contains URLs
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
 */
class CHistoryCombo : public CComboBoxEx
{
// Construction
public:
	CHistoryCombo(BOOL bAllowSortStyle = FALSE);
	virtual ~CHistoryCombo();

// Operations
public:
	/**
	 * Adds the string \a str to both the combobox and the history.
	 * If \a pos is specified, insert the string at the specified
	 * position, otherwise add it to the end of the list.
	 */
	int AddString(CString str, INT_PTR pos = -1);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHistoryCombo)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
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
	 * When \a bURLHistory is TRUE, treat the combo box entries
	 * as URLs. This activates Shell URL auto completion and
	 * the display of special icons in front of the combobox
	 * entries. Default is FALSE.
	 */
	void SetURLHistory(BOOL bURLHistory);
	/**
	 * When \a bPathHistory is TRUE, treat the combo box entries
	 * as Paths. This activates Shell Path auto completion and
	 * the display of special icons in front of the combobox
	 * entries. Default is FALSE.
	 */
	void SetPathHistory(BOOL bPathHistory);
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
	CString GetString() const;

protected:
	/**
	 * Will be called whenever the return key is pressed while the
	 * history combo has the input focus. A derived class may implement
	 * a special behaviour for the return key by overriding this method.
	 * It must return true to prevent the default processing for the
	 * return key. The default implementation returns false.
	 */
	virtual bool OnReturnKeyPressed() { return false; }

protected:
	CStringArray m_arEntries;
	CString m_sSection;
	CString m_sKeyPrefix;
	int m_nMaxHistoryItems;
	BOOL m_bAllowSortStyle;
	BOOL m_bURLHistory;
	BOOL m_bPathHistory;

	DECLARE_MESSAGE_MAP()
};



