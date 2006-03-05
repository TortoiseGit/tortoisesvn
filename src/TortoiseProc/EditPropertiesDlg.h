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
#include "afxcmn.h"

// CEditPropertiesDlg dialog

class CEditPropertiesDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CEditPropertiesDlg)

public:
	CEditPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditPropertiesDlg();

	void	SetPathList(const CTSVNPathList& pathlist) {m_pathlist = pathlist;}
	void	Refresh();
	bool	HasChanged() {return m_bChanged;}

// Dialog Data
	enum { IDD = IDD_EDITPROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnNMCustomdrawEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedRemoveProps();
	afx_msg void OnBnClickedEditprops();

	DECLARE_MESSAGE_MAP()
private:
	static UINT PropsThreadEntry(LPVOID pVoid);
	UINT PropsThread();

protected:
	class PropValue
	{
	public:
		PropValue(void) : count(0), allthesamevalue(true) {};

		stdstring	value;
		stdstring	value_without_newlines;
		int			count;
		bool		allthesamevalue;
	};
	CTSVNPathList	m_pathlist;
	CListCtrl		m_propList;
	BOOL			m_bRecursive;
	bool			m_bChanged;
	volatile LONG	m_bThreadRunning;
	std::map<stdstring, PropValue>	m_properties;
};
