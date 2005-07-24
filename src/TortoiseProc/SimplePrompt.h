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
#include "resource.h"

// CSimplePrompt dialog

class CSimplePrompt : public CDialog
{
	DECLARE_DYNAMIC(CSimplePrompt)

public:
	CSimplePrompt(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSimplePrompt();

// Dialog Data
	enum { IDD = IDD_SIMPLEPROMPT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CString		m_sUsername;
	CString		m_sPassword;
	CString		m_sRealm;
	BOOL		m_bSaveAuthentication;
	HWND		m_hParentWnd;

};
