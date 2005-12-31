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

#include "ResizableDialog.h"
#include "StateDlg.h"

/**
* \ingroup TortoiseProc
*
* A template which can be used as the base-class of dialogs which form the main dialog
* of a 'dialog-style application'
* Just provides the boiler-plate code for dealing with application icons
* 
*\remark Replace all references to CDialog or CResizableDialog in your dialog class with 
* either CResizableStandaloneDialog or CStandalongDialog, as appropriate
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
* \author Will Dean
*
* \par license
* This code is absolutely free to use and modify. The code is provided "as is" with
* no expressed or implied warranty. The author accepts no liability if it causes
* any damage to your social life, causes your pet to fall ill, *increases* your impotence
* or makes your car start emitting strange noises when you try to start it up.
* This code has many bugs, but it being free means I can't be bothered to fix them
*/
template <typename BaseType> class CStandAloneDialogTmpl : public BaseType
{
protected:
	CStandAloneDialogTmpl(UINT nIDTemplate, CWnd* pParentWnd = NULL) : BaseType(nIDTemplate, pParentWnd)
	{
		m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	}
	virtual BOOL OnInitDialog()
	{
		BaseType::OnInitDialog();

		// Set the icon for this dialog.  The framework does this automatically
		//  when the application's main window is not a dialog
		SetIcon(m_hIcon, TRUE);			// Set big icon
		SetIcon(m_hIcon, FALSE);		// Set small icon

		return FALSE;
	}

protected:
	afx_msg void OnPaint()
	{
		if (IsIconic())
		{
			CPaintDC dc(this); // device context for painting

			SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

			// Center icon in client rectangle
			int cxIcon = GetSystemMetrics(SM_CXICON);
			int cyIcon = GetSystemMetrics(SM_CYICON);
			CRect rect;
			GetClientRect(&rect);
			int x = (rect.Width() - cxIcon + 1) / 2;
			int y = (rect.Height() - cyIcon + 1) / 2;

			// Draw the icon
			dc.DrawIcon(x, y, m_hIcon);
		}
		else
		{
			BaseType::OnPaint();
		}
	}
private:
	HCURSOR OnQueryDragIcon()
	{
		return static_cast<HCURSOR>(m_hIcon);
	}

private:
	DECLARE_MESSAGE_MAP()
private:
	HICON m_hIcon;
};

// manually expand the MESSAGE_MAP macros here so we can use templates

template<typename BaseType>
const AFX_MSGMAP* CStandAloneDialogTmpl<BaseType>::GetMessageMap() const 
	{ return GetThisMessageMap(); } 

template<typename BaseType>
const AFX_MSGMAP* PASCAL CStandAloneDialogTmpl<BaseType>::GetThisMessageMap() 
{ 
	typedef CStandAloneDialogTmpl<BaseType> ThisClass;						   
	typedef BaseType TheBaseClass;					   
	static const AFX_MSGMAP_ENTRY _messageEntries[] =  
	{
		ON_WM_PAINT()
		ON_WM_QUERYDRAGICON()

		{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 } 
	}; 

	static const AFX_MSGMAP messageMap = 
	{ &TheBaseClass::GetThisMessageMap, &_messageEntries[0] }; 

	return &messageMap; 
}								  

typedef CStandAloneDialogTmpl<CResizableDialog> CResizableStandAloneDialog;
typedef CStandAloneDialogTmpl<CDialog> CStandAloneDialog;
typedef CStandAloneDialogTmpl<CStateDialog> CStateStandAloneDialog;
