////////////////////////////////////////////////////////////////////////////
//	File:		ReportOptionsCtrl.h
//	Version:	1.0.0
//
//	Author:		Maarten Hoeben
//	E-mail:		hamster@xs4all.nl
//
//	Implementation of the CReportOptionsCtrl and associated classes.
//
//	This code may be used in compiled form in any way you desire. This
//	file may be redistributed unmodified by any means PROVIDING it is 
//	not sold for profit without the authors written consent, and 
//	providing that this notice and the authors name and all copyright 
//	notices remains intact. 
//
//	An email letting me know how you are using it would be nice as well. 
//
//	This file is provided "as is" with no expressed or implied warranty.
//	The author accepts no liability for any damage/loss of business that
//	this product may cause.
//
////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_REPORTOPTIONSCTRL_H__BE1B9300_0F0C_11D5_8C72_0800460803C2__INCLUDED_)
#define AFX_REPORTOPTIONSCTRL_H__BE1B9300_0F0C_11D5_8C72_0800460803C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ReportCtrl.h"

// Use this as the classname when inserting this control as a custom control
// in the MSVC++ dialog editor
#define REPORTOPTIONSCTRL_CLASSNAME		_T("MFCReportOptionsCtrl")

// Embedded control IDs
#define REPORTOPTIONSCTRL_CONTROL_ID	2

// Notifications
#define ROCN_ITEMCHANGED	(RVN_LAST-1U)
#define ROCN_CHECKCHANGED	(RVN_LAST-2U)
#define ROCN_RADIOCHANGED	(RVN_LAST-3U)

typedef struct _NMREPORTOPTIONCTRL
{
    NMHDR		hdr;

	HTREEITEM	hParent;
	HTREEITEM	hChild;

	INT			iIndex;

} NMREPORTOPTIONCTRL, FAR* LPNMREPORTOPTIONCTRL;

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsEdit window

class CReportOptionsEdit : public CEdit
{
	DECLARE_DYNCREATE(CReportOptionsEdit)

// Construction
public:
	CReportOptionsEdit();

// Attributes
public:
	BOOL m_bIgnoreLostFocus;
	HTREEITEM m_hItem;

// Operations
public:

// Overrides
public:
	virtual ~CReportOptionsEdit();

	virtual int GetWindowStyle() { return WS_VISIBLE|WS_CHILD|ES_LEFT|ES_AUTOHSCROLL; };
	virtual int GetWidth() { return 100; };

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportOptionsEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportOptionsEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCombo window

class CReportOptionsCombo : public CComboBox
{
	DECLARE_DYNCREATE(CReportOptionsCombo)

// Construction
public:
	CReportOptionsCombo();

// Attributes
public:
	BOOL m_bIgnoreLostFocus;
	HTREEITEM m_hItem;

// Operations
public:

// Overrides
public:
	virtual ~CReportOptionsCombo();

	virtual int GetWindowStyle() { return WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST; };
	virtual int GetWidth() { return 100; };
	virtual int GetHeight() { return 100; };

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportOptionsCombo)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportOptionsCombo)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCtrl window

class CReportOptionsCtrl : public CReportCtrl  
{
	DECLARE_DYNCREATE(CReportOptionsCtrl)

// Construction
public:
	CReportOptionsCtrl();

// Attributes
public:
	BOOL IsGroup(HTREEITEM hItem);
	BOOL IsCheckBox(HTREEITEM hItem);
	BOOL IsRadioButton(HTREEITEM hItem);
	BOOL IsEditable(HTREEITEM hItem);

	BOOL IsDisabled(HTREEITEM hItem);

	BOOL SetCheckBox(HTREEITEM hItem, BOOL bCheck);
	BOOL GetCheckBox(HTREEITEM hItem, BOOL& bCheck);

	BOOL SetRadioButton(HTREEITEM hParent, INT iIndex);
	BOOL SetRadioButton(HTREEITEM hItem);
	BOOL GetRadioButton(HTREEITEM hParent, INT& iIndex, HTREEITEM& hCheckItem);
	BOOL GetRadioButton(HTREEITEM hItem, BOOL& bCheck);

	BOOL SetGroupEnable(HTREEITEM hParent, BOOL bEnable);
	BOOL GetGroupEnable(HTREEITEM hParent, BOOL& bEnable);

	BOOL SetCheckBoxEnable(HTREEITEM hItem, BOOL bEnable);
	BOOL GetCheckBoxEnable(HTREEITEM hItem, BOOL& bEnable);

	BOOL SetRadioButtonEnable(HTREEITEM hItem, BOOL bEnable);
	BOOL GetRadioButtonEnable(HTREEITEM hItem, BOOL& bEnable);

	CString GetEditText(HTREEITEM hItem);
	void SetEditText(HTREEITEM hItem, LPCTSTR lpszText);

	CString GetComboText(HTREEITEM hItem);
	void SetComboText(HTREEITEM hItem, LPCTSTR lpszText);

// Operations
public:
	HTREEITEM InsertItem(UINT nID, INT iImage, HTREEITEM hParent = RVTI_ROOT);
	HTREEITEM InsertItem(LPCTSTR lpszItem, INT iImage, HTREEITEM hParent = RVTI_ROOT);

	HTREEITEM InsertGroup(UINT nID, INT iImage, HTREEITEM hParent = RVTI_ROOT);
	HTREEITEM InsertGroup(LPCTSTR lpszItem, INT iImage, HTREEITEM hParent = RVTI_ROOT);

	HTREEITEM InsertCheckBox(UINT nID, HTREEITEM hParent, BOOL bCheck = TRUE);
	HTREEITEM InsertCheckBox(LPCTSTR lpszItem, HTREEITEM hParent, BOOL bCheck = TRUE);

	HTREEITEM InsertRadioButton(UINT nID, HTREEITEM hParent, BOOL bCheck = TRUE);
	HTREEITEM InsertRadioButton(LPCTSTR lpszItem, HTREEITEM hParent, BOOL bCheck = TRUE);

	BOOL AddEditBox(HTREEITEM hItem, LPCTSTR lpszText, CRuntimeClass* lpRuntimeClass = NULL);

	BOOL AddComboBox(HTREEITEM hItem, LPCTSTR lpszText, CRuntimeClass* lpRuntimeClass = NULL);

	void DeleteItem(HTREEITEM hItem);
	void SelectItem(HTREEITEM hItem);

// Overrides
public:
	virtual ~CReportOptionsCtrl();

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportOptionsCtrl)
	//}}AFX_VIRTUAL

// Implementation
protected:
	HTREEITEM m_hControl;
	CString m_strControl;

	CReportOptionsEdit* m_lpEdit;
	CReportOptionsCombo* m_lpCombo;

	BOOL Create();
	virtual BOOL CreateRuntimeClass(HTREEITEM hItem);
	virtual BOOL DeleteRuntimeClass(BOOL bUpdateFromControl, BOOL bNotify);

	BOOL GetCheck(HTREEITEM hItem, LPRVITEM lprvi);
	BOOL GetRadio(HTREEITEM hItem, LPRVITEM lprvi);

	INT VerifyText(HTREEITEM hItem);
	void UpdateText(INT iItem, LPCTSTR lpszText);
	CString StripText(INT iItem);

	virtual BOOL Notify(LPNMREPORTVIEW lpnmrv);
	virtual BOOL NotifyParent(UINT nCode, HTREEITEM hParent, HTREEITEM hChild, INT iIndex);

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportOptionsCtrl)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LRESULT OnCloseControl(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // !defined(AFX_REPORTOPTIONSCTRL_H__BE1B9300_0F0C_11D5_8C72_0800460803C2__INCLUDED_)
