// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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

// CSetProgsAdvDlg dialog

class CSetProgsAdvDlg : public CDialog
{
	DECLARE_DYNAMIC(CSetProgsAdvDlg)

public:
	CSetProgsAdvDlg(const CString& type, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSetProgsAdvDlg();
	/**
	 * Loads the tools from the registry.
	 */
	void LoadData();
	/**
	 * Saves the changed tools to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	int AddExtension(const CString& ext, const CString& tool);
	int FindExtension(const CString& ext);
	void EnableBtns();

// Dialog Data
	enum { IDD = IDD_SETTINGSPROGSADV };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedAddtool();
	afx_msg void OnBnClickedEdittool();
	afx_msg void OnBnClickedRemovetool();
	afx_msg void OnNMClickToollistctrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkToollistctrl(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

private:
	CBalloon	m_tooltips;				///< needed to display tooltips
	CString		m_sType;				///< tool type ("Diff" or "Merge")
	CRegKey		m_regToolKey;			///< registry key where the tools are stored
	CListCtrl	m_ToolListCtrl;			///< list control used for viewing and editing
	CRegDWORD	m_regDontConvertBase;	///< registry value for the "Don't Convert" flag
	BOOL		m_bDontConvertBase;		///< don't convert files when diffing agains BASE

	typedef std::map<CString,CString> TOOL_MAP;
	TOOL_MAP	m_Tools;				///< internal storage of all tools
	bool		m_ToolsValid;			///< true if m_Tools was ever read
};
