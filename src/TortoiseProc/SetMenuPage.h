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
//
#pragma once


// CSetMenuPage dialog

class CSetMenuPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetMenuPage)

public:
	CSetMenuPage();
	virtual ~CSetMenuPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

// Dialog Data
	enum { IDD = IDD_SETTINGSMENU };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	BOOL m_bMenu1;
	BOOL m_bMenu2;
	BOOL m_bMenu3;
	BOOL m_bMenu4;
	BOOL m_bMenu5;
	BOOL m_bMenu6;
	BOOL m_bMenu7;
	BOOL m_bMenu8;
	BOOL m_bMenu9;
	BOOL m_bMenu10;
	BOOL m_bMenu11;
	BOOL m_bMenu12;
	BOOL m_bMenu13;
	BOOL m_bMenu14;
	BOOL m_bMenu15;
	BOOL m_bMenu16;
	BOOL m_bMenu17;
	BOOL m_bMenu18;
	BOOL m_bMenu19;
	BOOL m_bMenu20;
	BOOL m_bMenu21;
	BOOL m_bMenu22;
	BOOL m_bMenu23;
	BOOL m_bMenu24;
public:
	afx_msg void OnBnClickedMenu1();
	afx_msg void OnBnClickedMenu2();
	afx_msg void OnBnClickedMenu3();
	afx_msg void OnBnClickedMenu4();
	afx_msg void OnBnClickedMenu5();
	afx_msg void OnBnClickedMenu6();
	afx_msg void OnBnClickedMenu7();
	afx_msg void OnBnClickedMenu8();
	afx_msg void OnBnClickedMenu9();
	afx_msg void OnBnClickedMenu10();
	afx_msg void OnBnClickedMenu11();
	afx_msg void OnBnClickedMenu12();
	afx_msg void OnBnClickedMenu13();
	afx_msg void OnBnClickedMenu14();
	afx_msg void OnBnClickedMenu15();
	afx_msg void OnBnClickedMenu16();
	afx_msg void OnBnClickedMenu17();
	afx_msg void OnBnClickedMenu18();
	afx_msg void OnBnClickedMenu19();
	afx_msg void OnBnClickedMenu20();
	afx_msg void OnBnClickedMenu21();
	afx_msg void OnBnClickedMenu22();
	afx_msg void OnBnClickedMenu23();
	afx_msg void OnBnClickedMenu24();
	virtual BOOL OnApply();
};
