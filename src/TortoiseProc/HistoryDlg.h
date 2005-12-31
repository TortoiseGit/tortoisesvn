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
#include "StandAloneDlg.h"


/**
 * \ingroup TortoiseProc
 * Dialog showing the log message history.
 */
class CHistoryDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CHistoryDlg)
public:
	CHistoryDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CHistoryDlg();

	/// Returns the text of the selected entry.
	CString GetSelectedText() const {return m_SelectedText;}
	/// Loads the history into the dialog.
	int LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix);
	/// Saves the history.
	bool SaveHistory();
	/// Adds a new string to the history list.
	bool AddString(const CString& sText);
	/// Sets the maximum number of items in the history. Default is 25.
	void SetMaxHistoryItems(int nMax) {m_nMaxHistoryItems = nMax;}
	/// Returns the number of items in the history.
	INT_PTR GetCount() const {return m_arEntries.GetCount(); }
// Dialog Data
	enum { IDD = IDD_HISTORYDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnLbnDblclkHistorylist();

	DECLARE_MESSAGE_MAP()
private:
	CString m_sSection;
	CString m_sKeyPrefix;
	CStringArray m_arEntries;
	CListBox m_List;
	CString m_SelectedText;
	int m_nMaxHistoryItems;
};
