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
#include "StandAloneDlg.h"
#include "Registry.h"


// CSetOverlayIcons dialog

class CSetOverlayIcons : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetOverlayIcons)

public:
	CSetOverlayIcons();   // standard constructor
	virtual ~CSetOverlayIcons();

	/**
	* Saves the changed settings to the registry.
	* \remark If the dialog is closed/dismissed without calling
	* this method first then all settings the user made must be
	* discarded!
	*/
	void SaveData();

	UINT GetIconID() {return IDI_ICONSET;}

// Dialog Data
	enum { IDD = IDD_OVERLAYICONS };

protected:
	virtual void			DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL			OnInitDialog();
	afx_msg void			OnBnClickedListradio();
	afx_msg void			OnBnClickedSymbolradio();
	afx_msg void			OnCbnSelchangeIconsetcombo();

	void					ShowIconSet(bool bSmallIcons);
	void					AddFileTypeGroup(CString sFileType, bool bSmallIcons);
	DECLARE_MESSAGE_MAP()
protected:
	int				m_selIndex;
	CString			m_sIconSet;
	CComboBox		m_cIconSet;
	CListCtrl		m_cIconList;

	CString			m_sIconPath;
	CString			m_sOriginalIconSet;
	CString			m_sNormal;
	CString			m_sModified;
	CString			m_sConflicted;
	CString			m_sReadOnly;
	CString			m_sDeleted;
	CString			m_sAdded;
	CString			m_sLocked;
	CImageList		m_ImageList;
	CImageList		m_ImageListBig;

	CRegString		m_regInSubversion;
	CRegString		m_regModified;
	CRegString		m_regConflicted;
	CRegString		m_regReadOnly;
	CRegString		m_regDeleted;
	CRegString		m_regLocked;
	CRegString		m_regAdded;
public:
	virtual BOOL OnApply();
};
