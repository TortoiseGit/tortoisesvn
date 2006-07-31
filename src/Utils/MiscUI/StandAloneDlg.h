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

/**
 * \ingroup TortoiseProc
 *
 * A template which can be used as the base-class of dialogs which form the main dialog
 * of a 'dialog-style application'
 * Just provides the boiler-plate code for dealing with application icons
 * 
 * \remark Replace all references to CDialog or CResizableDialog in your dialog class with 
 * either CResizableStandaloneDialog, CStandalongDialog or CStateStandAloneDialog, as appropriate
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
	/**
	 * Wrapper around the CWnd::EnableWindow() method, but
	 * makes sure that a control that has the focus is not disabled
	 * before the focus is passed on to the next control.
	 */
	BOOL DialogEnableWindow(UINT nID, BOOL bEnable)
	{
		CWnd * pwndDlgItem = GetDlgItem(nID);
		if (pwndDlgItem == NULL)
			return FALSE;
		if (bEnable)
			return pwndDlgItem->EnableWindow(bEnable);
		if (GetFocus() == pwndDlgItem)
		{
			SendMessage(WM_NEXTDLGCTL, 0, FALSE);
		}
		return pwndDlgItem->EnableWindow(bEnable);
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

class CStateDialog : public CDialog, public CResizableWndState
{
public:
	CStateDialog() {m_bEnableSaveRestore = false;}
	CStateDialog(UINT /*nIDTemplate*/, CWnd* /*pParentWnd = NULL*/) {m_bEnableSaveRestore = false;}
	CStateDialog(LPCTSTR /*lpszTemplateName*/, CWnd* /*pParentWnd = NULL*/) {m_bEnableSaveRestore = false;}
	virtual ~CStateDialog();

private:
	// flags
	bool m_bEnableSaveRestore;
	bool m_bRectOnly;

	// internal status
	CString m_sSection;			// section name (identifies a parent window)

protected:
	// section to use in app's profile
	void EnableSaveRestore(LPCTSTR pszSection, bool bRectOnly = FALSE)
	{
		m_sSection = pszSection;

		m_bEnableSaveRestore = true;
		m_bRectOnly = bRectOnly;

		// restore immediately
		LoadWindowRect(pszSection, bRectOnly);
	};

	virtual CWnd* GetResizableWnd() const
	{
		// make the layout know its parent window
		return CWnd::FromHandle(m_hWnd);
	};

	afx_msg void OnDestroy()
	{
		if (m_bEnableSaveRestore)
			SaveWindowRect(m_sSection, m_bRectOnly);
		CDialog::OnDestroy();
	};

	DECLARE_MESSAGE_MAP()

	//BEGIN_MESSAGE_MAP(CStateDialog, CDialog)
	//	ON_WM_DESTROY()
	//END_MESSAGE_MAP();

};
typedef CStandAloneDialogTmpl<CResizableDialog> CResizableStandAloneDialog;
typedef CStandAloneDialogTmpl<CDialog> CStandAloneDialog;
typedef CStandAloneDialogTmpl<CStateDialog> CStateStandAloneDialog;
