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

#include "ResizableWndState.h"

/**
* \ingroup Utils
*
* A base class for non-resizable dialogs to make them remember their last
* position/size.
* 
*
* \par requirements
* win95 or later
* winNT4 or later
* MFC
*
* \version 1.0
* first version
*
* \date Jan-2005
*
* \author Stefan Kueng
*/
class CStateDialog : public CDialog, public CResizableWndState
{
public:
	CStateDialog();
	CStateDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	CStateDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	virtual ~CStateDialog();

private:
	// flags
	BOOL m_bEnableSaveRestore;
	BOOL m_bRectOnly;

	// internal status
	CString m_sSection;			// section name (identifies a parent window)

protected:
	// section to use in app's profile
	void EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly = FALSE);

	virtual CWnd* GetResizableWnd() const
	{
		// make the layout know its parent window
		return CWnd::FromHandle(m_hWnd);
	};
	
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()

};

