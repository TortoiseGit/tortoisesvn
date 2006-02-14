////////////////////////////////////////////////////////////////////////////
//	File:		ReportCtrl.h
//	Version:	2.0.1
//
//	Author:		Maarten Hoeben
//	E-mail:		hamster@xs4all.nl
//
//	Implementation of the CReportCtrl and associated classes.
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

#if !defined(AFX_REPORTCTRL_H__279B1CA0_D7F2_11D2_88D7_ABB23645F26D__INCLUDED_)
#define AFX_REPORTCTRL_H__279B1CA0_D7F2_11D2_88D7_ABB23645F26D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ReportCtrl.h : header file
//

#include "MemDC.h"
#include "FlatHeaderCtrl.h"

#define ID_HEADERCTRL	0
#define ID_REPORTCTRL	0

// Use this as the classname when inserting this control as a custom control
// in the MSVC++ dialog editor
#define REPORTCTRL_CLASSNAME		_T("MFCReportCtrl")
#define REPORTCTRL_MAX_TEXT			256
#define REPORTCTRL_MAX_CACHE		128

// Properties
#define RVP_SPACING				1
#define RVP_CHECK				2
#define RVP_NOITEMTEXT			3
#define RVP_GRIDSTYLE			4
#define RVP_SORTTOOLTIP			5
#define RVP_NOTIFYMASK			6
#define RVP_ENABLEITEMCACHEMAP	7
#define RVP_SEPARATOR			8
#define RVP_ENABLEPREVIEW		9
#define RVP_ENABLEHORZSCROLLBAR	10
#define RVP_FRAMECONTROLSTYLE	11
#define RVP_ENABLEIMAGEBLENDING	12
#define RVP_BLENDCOLOR			13
#define RVP_CBSEPARATOR			14
#define RVP_CBINDENT			15

#define RVP_GRIDSTYLE_DOT		0
#define RVP_GRIDSTYLE_DASH		1
#define RVP_GRIDSTYLE_SOLID		2

#define RVP_FCSTYLE_FLAT		0
#define RVP_FCSTYLE_MONO		1
#define RVP_FCSTYLE_NORMAL		2

#define RVP_BLEND_NONE			0
#define RVP_BLEND_SELECT		1
#define RVP_BLEND_FOCUS			2
#define RVP_BLEND_ALL			3

// Styles
#define RVS_SINGLESELECT		0x0001
#define RVS_SHOWSELALWAYS		0x0002
#define RVS_SHOWCOLORALWAYS		0x0004
#define RVS_SHOWCOLORALTERNATE	0x0008
#define RVS_SHOWHGRID			0x0010
#define RVS_SHOWVGRID			0x0020
#define RVS_NOHEADER			0x0040
#define RVS_NOSORT				0x0080
#define RVS_ALLOWCOLUMNREMOVAL	0x0100
#define RVS_SHOWEDITROW			0x0200
#define RVS_SHOWHGRIDEX			0x0400
#define RVS_OWNERDATA			0x0800
#define RVS_FOCUSSUBITEMS		0x1000
#define RVS_EXPANDSUBITEMS		0x2000
#define RVS_GROUPVIEW			0x4000
#define RVS_TREEVIEW			0x8000
#define RVS_TREEMASK			0xc000

// Column Format
#define RVCF_LEFT				HDF_LEFT
#define RVCF_RIGHT				HDF_RIGHT
#define RVCF_CENTER				HDF_CENTER
#define RVCF_TEXT				HDF_STRING
#define RVCF_IMAGE				HDF_IMAGE

#define	RVCF_JUSTIFYMASK		HDF_JUSTIFYMASK
#define RVCF_MASK				0x0000ffff

#define RVCF_EX_AUTOWIDTH		(HDF_EX_AUTOWIDTH<<16)
#define RVCF_EX_INCLUDESORT		(HDF_EX_INCLUDESORT<<16)
#define RVCF_EX_FIXEDWIDTH		(HDF_EX_FIXEDWIDTH<<16)
#define RVCF_EX_TOOLTIP			(HDF_EX_TOOLTIP<<16)
#define RVCF_EX_PERSISTENT		(HDF_EX_PERSISTENT<<16)
#define RVCF_EX_MASK			0x00ff0000

#define RVCF_SUBITEM_IMAGE		0x01000000
#define RVCF_SUBITEM_CHECK		0x02000000
#define RVCF_SUBITEM_NOFOCUS	0x04000000
#define RVCF_SUBITEM_MASK		0xff000000

typedef struct _RVSUBITEM
{
    UINT	nFormat;
    INT		iWidth;
	INT		iMinWidth;
	INT		iMaxWidth;
    INT		iImage;

    LPCTSTR	lpszText;

	_RVSUBITEM() : nFormat(RVCF_LEFT|RVCF_TEXT), iWidth(-1), iMinWidth(0), iMaxWidth(0), iImage(0), lpszText(NULL) {};

} RVSUBITEM, FAR* LPRVSUBITEM;

// Item Masks
#define RVIM_TEXT			0x0001
#define RVIM_TEXTCOLOR		0x0002
#define RVIM_IMAGE			0x0004
#define RVIM_CHECK			0x0008
#define RVIM_BKCOLOR		0x0010
#define RVIM_PREVIEW		0x0020
#define RVIM_STATE			0x0040
#define RVIM_LPARAM			0x0080
#define RVIM_INDENT			0x0100
#define RVIM_OVERLAY		0x0200
#define RVIM_PARAM64		0x0400

// Item Index
#define RVI_INVALID		-2
#define RVI_EDIT		-1
#define RVI_FIRST		0
#define RVI_LAST		INT_MAX

// Tree Item
#define RVTI_ROOT		TVI_ROOT
#define RVTI_FIRST		TVI_FIRST
#define RVTI_LAST		TVI_LAST
#define RVTI_SORT		TVI_SORT

// Item State
#define RVIS_FOCUSED		0x0001
#define RVIS_SELECTED		0x0002
#define RVIS_BOLD			0x0004
#define RVIS_READONLY		0x0008
#define RVIS_OWNERDRAW		0x0010
#define RVIS_TREECLOSED		0x0020
#define RVIS_TREEOPENED		0x0040
#define RVIS_BLEND25		0x0080
#define RVIS_BLEND50		0x0100

#define RVIS_TREEMASK		0x0060
#define RVIS_BLENDMASK		0x0180

typedef struct _RVITEM
{
	UINT		nMask;

	INT			iItem;
	INT			iSubItem;

	LPTSTR		lpszText;
	INT			iTextMax;
	INT			iTextColor;

	INT			iImage;
	INT			iCheck;

	INT			iBkColor;
	UINT		nPreview;
	INT			iIndent;

	UINT		nState;

	LPARAM		lParam;
	UINT64		Param64;

	INT			iOverlay;

	_RVITEM() : nMask(0), iItem(RVI_INVALID), iSubItem(-1), lpszText(NULL), iTextMax(0), iTextColor(-1), iImage(-1), iCheck(-1), iBkColor(-1), nPreview(0), iIndent(-1), nState(0), lParam(0), Param64(0), iOverlay(0) {};

} RVITEM, FAR* LPRVITEM;

// Hit Test
#define RVHT_NOWHERE			0x0001
#define RVHT_ONITEMIMAGE		0x0002
#define RVHT_ONITEMCHECK		0x0004
#define RVHT_ONITEMTEXT			0x0008
#define RVHT_ONITEMSUBITEM		(RVHT_ONITEMIMAGE|RVHT_ONITEMCHECK|RVHT_ONITEMTEXT)
#define RVHT_ONITEMPREVIEW		0x0010
#define RVHT_ONITEMTREEBOX		0x0020
#define RVHT_ONITEM				(RVHT_ONITEMIMAGE|RVHT_ONITEMCHECK|RVHT_ONITEMTEXT|RVHT_ONITEMPREVIEW)
#define RVHT_ONITEMEDIT			0x0040

#define RVHT_ABOVE				0x0100
#define RVHT_BELOW				0x0200
#define RVHT_TORIGHT			0x0400
#define RVHT_TOLEFT				0x0800

typedef struct _RVHITTESTINFO
{
	POINT		point;
	UINT		nFlags;

	HTREEITEM	hItem;
	INT			iItem;
	INT			iSubItem;

	INT			iRow;
	INT			iColumn;

	RECT		rect;

} RVHITTESTINFO, FAR* LPRVHITTESTINFO;

// Find Item
typedef struct _RVFINDINFO
{
    UINT		nFlags;

    LPCTSTR		lpszText;
	INT			iImage;
	INT			iCheck;
    LPARAM		lParam;

} RVFINDINFO, FAR* LPRVFINDINFO;

#define RVFI_TEXT		0x0001
#define RVFI_PARTIAL	0x0002
#define RVFI_IMAGE		0x0004
#define RVFI_CHECK		0x0008
#define RVFI_LPARAM		0x0010
#define RVFI_WRAP		0x0100
#define RVFI_UP			0x0200

// Expand Item
#define RVE_COLLAPSE	0
#define RVE_EXPAND		1
#define RVE_TOGGLE		2

// Get Next Item
#define RVGN_ROOT				0x0000
#define RVGN_NEXT				0x0001
#define RVGN_PREVIOUS			0x0002
#define RVGN_PARENT				0x0003
#define RVGN_CHILD				0x0004
#define RVGN_FOCUSED			0x0005

// Notifications
#define RVN_FIRST				(0U-2048U)
#define RVN_LAST				(0U-2067U)

#define RVN_ITEMDRAWPREVIEW		(0U-2048U)
#define RVN_ITEMCLICK			(0U-2049U)
#define RVN_ITEMDBCLICK			(0U-2050U)
#define RVN_SELECTIONCHANGING	(0U-2051U)
#define RVN_SELECTIONCHANGED	(0U-2052U)
#define RVN_HEADERCLICK			(0U-2053U)
#define RVN_LAYOUTCHANGED		(0U-2054U)
#define RVN_ITEMDELETED			(0U-2055U)
#define RVN_ITEMCALLBACK		(0U-2056U)
#define RVN_BEGINITEMEDIT		(0U-2057U)
#define RVN_ENDITEMEDIT			(0U-2058U)
#define RVN_KEYDOWN				(0U-2059U)
#define RVN_DIVIDERDBLCLICK		(0U-2060U)
#define RVN_ITEMRCLICK			(0U-2061U)
#define RVN_BUTTONCLICK			(0U-2062U)
#define RVN_ITEMDRAW			(0U-2063U)
#define RVN_ITEMEXPANDING		(0U-2064U)
#define RVN_ITEMEXPANDED		(0U-2065U)
#define RVN_HEADERRCLICK		(0U-2066U)
#define RVN_ITEMRCLICKUP		(0U-2067U)

// Notification masks
#define RVNM_ITEMDRAWPREVIEW	0x00000001
#define RVNM_ITEMCLICK			0x00000002
#define RVNM_ITEMDBCLICK		0x00000004
#define RVNM_SELECTIONCHANGING	0x00000008
#define RVNM_SELECTIONCHANGED	0x00000010
#define RVNM_HEADERCLICK		0x00000020
#define RVNM_LAYOUTCHANGED		0x00000040
#define RVNM_ITEMDELETED		0x00000080
#define RVNM_ITEMCALLBACK		0x00000100
#define RVNM_BEGINITEMEDIT		0x00000200
#define RVNM_ENDITEMEDIT		0x00000400
#define RVNM_KEYDOWN			0x00000800
#define RVNM_DIVIDERDBLCLICK	0x00001000
#define RVNM_ITEMRCLICK			0x00002000
#define RVNM_BUTTONCLICK		0x00004000
#define RVNM_ITEMDRAW			0x00008000
#define RVNM_ITEMEXPANDING		0x00010000
#define RVNM_ITEMEXPANDED		0x00020000
#define RVNM_HEADERRCLICK		0x00040000
#define RVNM_ITEMRCLICKUP		0x00080000
#define RVNM_ALL				0xFFFFFFFF

typedef struct _NMRVDRAWPREVIEW
{
    NMHDR		hdr;

	HTREEITEM	hItem;
    INT			iItem;
	UINT		nState;

	HDC			hDC;
	RECT		rect;

    LPARAM		lParam;

} NMRVDRAWPREVIEW, FAR* LPNMRVDRAWPREVIEW;

typedef struct _NMREPORTVIEW
{
    NMHDR		hdr;

	UINT		nKeys;
    POINT		point;
	UINT		nFlags;

	HTREEITEM	hItem;
    INT			iItem;
    INT			iSubItem;

	UINT		nState;

    LPARAM		lParam;

} NMREPORTVIEW, FAR* LPNMREPORTVIEW;

typedef struct _NMRVHEADER
{
	NMHDR		hdr;

	INT			iSubItem;
	UINT		nWidth;

} NMRVHEADER, FAR* LPNMRVHEADER;

typedef struct _NMRVITEMCALLBACK
{
	NMHDR		hdr;
	RVITEM		item;
} NMRVITEMCALLBACK, FAR* LPNMRVITEMCALLBACK;

typedef INT (CALLBACK* LPFNRVCOMPARE)(INT iItem1, INT iSubItem1, INT iItem2, INT iSubItem2, LPARAM lParam);

typedef struct _NMRVITEMEDIT
{
	NMHDR		hdr;

	HTREEITEM	hItem;
	INT			iItem;
	INT			iSubItem;

	BOOL		bButton;

	HWND		hWnd;
	RECT		rect;

	UINT		nKey;
	LPTSTR		lpszText;

	LPARAM		lParam;

} NMRVITEMEDIT, FAR* LPNMRVITEMEDIT;

typedef struct _NMRVITEMDRAW
{
    NMHDR		hdr;

	LPRVITEM	lprvi;

	HDC			hDC;
	RECT		rect;

} NMRVITEMDRAW, FAR* LPNMRVITEMDRAW;

// Item Rect
#define RVIR_BOUNDS			0
#define RVIR_IMAGE			1
#define RVIR_CHECK			2
#define RVIR_TEXT			3
#define RVIR_TEXTNOCOLUMNS	4

class CReportCtrl;
class CReportSubItemListCtrl;

/////////////////////////////////////////////////////////////////////////////
// Global utilities

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#define ULW_COLORKEY			0x00000001
#define ULW_ALPHA               0x00000002
#endif

typedef BOOL (WINAPI *lpfnUpdateLayeredWindow)(
	HWND hWnd, HDC hdcDst, POINT *pptDst, SIZE *psize, HDC hdcSrc,
	POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags
);

typedef BOOL (WINAPI *lpfnSetLayeredWindowAttributes)(
	HWND hwnd, COLORREF crKey, BYTE xAlpha, DWORD dwFlags
);

BOOL InitLayeredWindows(void);

BOOL MatchString(const CString& strString, const CString& strPattern);

/////////////////////////////////////////////////////////////////////////////
// CReportEditCtrl window

class CReportEditCtrl : public CEdit
{
public:
	CReportEditCtrl(INT iItem, INT iSubItem, BOOL bButton = TRUE);

// Operations
public:
	void BeginEdit(UINT nKey);
	void EndEdit();

// Overrides
public:
	virtual ~CReportEditCtrl();

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportEditCtrl)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PostNcDestroy();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bEndEdit;

	INT m_iItem;
	INT m_iSubItem;

	BOOL m_bButton;
	CButton m_wndButton;

	UINT m_nLastKey;

	//{{AFX_MSG(CReportEditCtrl)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg UINT OnGetDlgCode();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportComboCtrl window

class CReportComboCtrl : public CComboBox
{
public:
	CReportComboCtrl(INT iItem, INT iSubItem);

// Operations
public:
	void BeginEdit(UINT nKey);
	void EndEdit();

// Overrides
public:
	virtual ~CReportComboCtrl();

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportComboCtrl)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bEndEdit;

	INT m_iItem;
	INT m_iSubItem;

	UINT m_nLastKey;

	//{{AFX_MSG(CReportComboCtrl)
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKillfocus();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportTipCtrl window

#define REPORTTIPCTRL_CLASSNAME _T("MFCReportTipCtrl")

#define REPORTTIPCTRL_FADE_TIMERID 1

#define REPORTTIPCTRL_FADETIME		500
#define REPORTTIPCTRL_FADESTEP		10
#define REPORTTIPCTRL_FADETIMEOUT	100

class CReportTipCtrl : public CWnd
{
// Construction
public:
	CReportTipCtrl();

	BOOL Create(CReportCtrl *pReportCtrl);

// Attributes
public:

// Operations
public:
	BOOL Show(CRect rectTitle, LPCTSTR lpszText, CFont* pFont = NULL);
	void Hide();

// Overrides
public:
	virtual ~CReportTipCtrl();

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportTipCtrl)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CReportCtrl *m_pReportCtrl;

	BOOL m_bLayeredWindows;
	BYTE m_nAlpha;

	CRect m_rectText;
	CRect m_rectTip;
    DWORD m_dwLastLButtonDown;
    DWORD m_dwDblClickMsecs;

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportTipCtrl)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportHeaderCtrl window

class CReportHeaderCtrl : public CFlatHeaderCtrl
{
// Construction
public:
	CReportHeaderCtrl();

// Attributes
public:
	void SetMinMax(INT iPos, INT iMin, INT iMax);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportHeaderCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CReportHeaderCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportHeaderCtrl)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportData storage class

class CReportData : public CString
{
public:
	CReportData();
	~CReportData();

	BOOL New(INT iSubItems);

	BOOL GetSubItem(INT iSubItem, LPINT lpiImage, LPINT lpiOverlay, LPINT lpiCheck, LPINT lpiColor, LPTSTR lpszText, LPINT lpiTextMax);
	BOOL SetSubItem(INT iSubItem, INT iImage, INT iOverlay, INT iCheck, INT iColor, LPCTSTR lpszText);

	BOOL InsertSubItem(INT iSubItem, INT iImage, INT iOverlay, INT iCheck, INT iColor, LPCTSTR lpszText);
	BOOL DeleteSubItem(INT iSubItem);
};

/////////////////////////////////////////////////////////////////////////////
// CReportCtrl window

#define REPORTCTRL_AUTOEXPAND_TIMERID 2
#define REPORTCTRL_AUTOSCROLL_TIMERID 3

#define REPORTCTRL_AUTOEXPAND 2000
#define REPORTCTRL_SCROLL 50

class CReportCtrl : public CWnd, public IDropTarget
{
	friend class CReportView;
	friend class CReportHeaderCtrl;
	friend class CReportTipCtrl;
	friend class CReportSubItemListCtrl;

// Construction
public:
	DECLARE_DYNCREATE(CReportCtrl)
	CReportCtrl();

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

// Attributes
public:
	DWORD GetStyle() const;

	BOOL ModifyProperty(WPARAM wParam, LPARAM lParam);

	void SetIndentFirstColumn(INT iIndent);

	INT ActivateSubItem(INT iSubItem, INT iColumn = 0);
	BOOL DeactivateSubItem(INT iSubItem);
	BOOL DeactivateAllSubItems();

	BOOL IsActiveSubItem(INT iSubItem);
	INT GetActiveSubItemCount();

	INT GetSubItemWidth(INT iSubItem);
	BOOL SetSubItemWidth(INT iSubItem, INT iWidth = -1);

	INT GetItemIndex(HTREEITEM hItem);
	HTREEITEM GetItemHandle(INT iItem);

	void SetTreeSubItem(HTREEITEM hItem, INT iSubItem = -1);
	HTREEITEM GetNextItem(HTREEITEM hItem, UINT nCode);

	BOOL GetItem(LPRVITEM lprvi);
	BOOL SetItem(LPRVITEM lprvi);

	INT GetItemText(INT iItem, INT iSubItem, LPTSTR lpszText, INT iLen);
	CString GetItemText(INT iItem, INT iSubItem);
	BOOL SetItemText(INT iItem, INT iSubItem, LPCTSTR lpszText);

	INT GetItemImage(INT iItem, INT iSubItem);
	BOOL SetItemImage(INT iItem, INT iSubItem, INT iImage);

	INT GetItemCheck(INT iItem, INT iSubItem);
	BOOL SetItemCheck(INT iItem, INT iSubItem, INT iCheck = -1);

	DWORD_PTR GetItemData(INT iItem);
	BOOL SetItemData(INT iItem, DWORD_PTR dwData);

	BOOL GetItemRect(INT iItem, INT iSubItem, LPRECT lpRect, UINT nCode = RVIR_BOUNDS);
	BOOL MeasureItem(INT iItem, INT iSubItem, LPRECT lpRect, BOOL bTextOnly = FALSE);

	INT GetItemHeight();
	void SetItemHeight(INT iHeight);

	INT GetVisibleCount(BOOL bUnobstructed = TRUE);
	INT GetTopIndex();
	BOOL IsItemVisible(INT iItem, BOOL bUnobstructed = TRUE);
	void PageUp();
	void PageDown();

	INT GetItemCount();
	INT GetItemCount(HTREEITEM hItem, BOOL bRecurse = FALSE);
	void SetItemCount(INT iCount);

	INT GetFirstSelectedItem();
	INT GetNextSelectedItem(INT iItem);
	INT GetSelectedCount();
	INT GetSelectedItems(LPINT lpiItems, INT iMax);

	void ClearSelection();
	void InvertSelection();
	void SelectAll();
	void SetSelection(INT iItem, BOOL bKeepSelection = FALSE);
	void SetSelection(LPINT lpiItems, INT iCount, BOOL bKeepSelection = FALSE);
	void SetSelection(INT iSubItem, CString& strPattern, BOOL bKeepSelection);
	void SetSelectionLessEq(INT iSubItem, INT iMatch, BOOL bKeepSelection);

	void SetSelection(HTREEITEM hItem, BOOL bKeepSelection = FALSE);
	void SetSelection(HTREEITEM* lphItems, INT iCount, BOOL bKeepSelection = FALSE);

	INT GetItemRow(INT iItem);
	CArray<INT, INT>* GetItemRowArray();
	BOOL SetItemRowArray(CArray<INT, INT>& arrayRows);

	BOOL MoveUp(INT iRow);
	BOOL MoveDown(INT iRow);

	BOOL SetImageList(CImageList* pImageList, CImageList* pCheckList = NULL);
	CImageList* GetImageList();

	BOOL SetBkImage(UINT nIDResource);
	BOOL SetBkImage(LPCTSTR lpszResourceName);

	COLORREF GetColor(INT iIndex);
	BOOL SetColor(INT iIndex, COLORREF crColor);

	BOOL HasFocus();
	INT GetCurrentFocus(LPINT lpiColumn = NULL);
	BOOL AdjustFocus(INT iRow, INT iColumn = -1);

	BOOL HasChildren(HTREEITEM hItem);
	BOOL IsChild(HTREEITEM hParent, HTREEITEM hItem);
	BOOL IsDescendent(HTREEITEM hParent, HTREEITEM hItem);

	CReportHeaderCtrl* GetHeaderCtrl();

	BOOL SetReportSubItemListCtrl(CReportSubItemListCtrl* lprsilc);
	CReportSubItemListCtrl* GetReportSubItemListCtrl();

	BOOL SetSortCallback(LPFNRVCOMPARE lpfnrvc, LPARAM lParam);
	LPFNRVCOMPARE GetSortCallback();

	BOOL GetSortSettings(INT *lpiSubItem, BOOL *lpbAscending);

	BOOL WriteProfile(LPCTSTR lpszSection, LPCTSTR lpszEntry);
	BOOL GetProfile(LPCTSTR lpszSection, LPCTSTR lpszEntry);

// Operations
public:
	BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);

	INT DefineSubItem(INT iSubItem, LPRVSUBITEM lprvs, BOOL bUpdateList = FALSE);
	BOOL RedefineSubItem(INT iSubItem, LPRVSUBITEM lprvs, BOOL bUpdateList = TRUE);
	BOOL UndefineSubItem(INT iSubItem);
	void UndefineAllSubItems();

	INT AddItem(LPCTSTR lpszText, INT iImage = -1, INT iCheck = -1, INT iTextColor = -1);
	INT AddItem(LPRVITEM lprvi = NULL);

	INT InsertItem(INT iItem, LPCTSTR lpszText, INT iImage = -1, INT iCheck = -1, INT iTextColor = -1);
	INT InsertItem(LPRVITEM lprvi);
	BOOL DeleteItem(INT iItem);

	HTREEITEM InsertItem(LPCTSTR lpszText, INT iImage = -1, INT iCheck = -1, INT iTextColor = -1, HTREEITEM hParent = RVTI_ROOT, HTREEITEM hInsertAfter = RVTI_LAST, INT iSubItem = -1);
	HTREEITEM InsertItem(LPRVITEM lprvi, HTREEITEM hParent, HTREEITEM hInsertAfter = RVTI_LAST, INT iSubItem = -1);
	BOOL DeleteItem(HTREEITEM hItem);

	BOOL DeleteAllItems();

	void SetRedraw(BOOL bRedraw = TRUE);
	void RedrawItems(INT iFirst, INT iLast = RVI_INVALID);
	BOOL EnsureVisible(INT iItem, BOOL bUnobstructed = TRUE);
	BOOL EnsureVisible(HTREEITEM hItem, BOOL bUnobstructed = TRUE);

	INT InsertColor(INT iIndex, COLORREF crColor);
	BOOL DeleteColor(INT iIndex);
	void DeleteAllColors();

	INT HitTest(LPRVHITTESTINFO lprvhti);

	BOOL ResortItems();
	BOOL SortItems(INT iSubItem, BOOL bAscending);

	INT FindItem(LPRVFINDINFO lprvfi, INT iSubItem, INT iStart = RVI_INVALID);

	BOOL Expand(HTREEITEM hItem, UINT nCode);
	BOOL ExpandAll(HTREEITEM hItem, UINT nCode, INT iLevels = -1);

	void FlushCache(INT iItem = RVI_INVALID);

	void Copy();
	void Cut();
	void Paste();

// Misc
public:
	INT PreviewHeight(CFont* pFont, UINT nLines);
	INT PreviewHeight(CFont* pFont, LPCTSTR lpszText, LPRECT lpRect = NULL);

// IDropTarget
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(/* [in] */ REFIID riid,
					/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG STDMETHODCALLTYPE AddRef( void) { ATLTRACE("CReportCtrl::AddRef\n"); return ++m_cRefCount; }
	ULONG STDMETHODCALLTYPE Release( void);

	HRESULT STDMETHODCALLTYPE DragEnter(
			/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ DWORD __RPC_FAR *pdwEffect);
	HRESULT STDMETHODCALLTYPE DragOver( 
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ DWORD __RPC_FAR *pdwEffect);
	HRESULT STDMETHODCALLTYPE DragLeave(void);    
	HRESULT STDMETHODCALLTYPE Drop(
			/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
			/* [in] */ DWORD grfKeyState,
			/* [in] */ POINTL pt,
			/* [out][in] */ DWORD __RPC_FAR *pdwEffect);


// Overrides
public:
	virtual ~CReportCtrl();

	virtual CString GetItemString(INT iItem, TCHAR cSeparator = _T('\t'));

	virtual void DrawItem(CDC* pDC, CRect rect, LPRVITEM lprvi);
	virtual BOOL DrawBkgnd(CDC* pDC, CRect rect, COLORREF crBackground);

	virtual INT CompareItems(LPRVITEM lprvi1, LPRVITEM lprvi2);
	virtual INT CompareItems(INT iItem1, INT iSubItem1, INT iItem2, INT iSubItem2);

	virtual BOOL BeginEdit(INT iRow, INT iColumn, UINT nKey);
	virtual void EndEdit(BOOL bUpdate = TRUE, LPNMRVITEMEDIT lpnmrvie = NULL);

	virtual BOOL Notify(LPNMREPORTVIEW lpnmrv);
	
	virtual DROPEFFECT OnDrag(int /*iItem*/, int /*iSubItem*/, IDataObject * /*pDataObj*/, DWORD /*grfKeyState*/) {return DROPEFFECT_NONE;}
	virtual void OnDrop(int /*iItem*/, int /*iSubItem*/, IDataObject * /*pDataObj*/, DWORD /*grfKeyState*/) {return;}

	virtual void OnBeginDrag() {return;}

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Structures
protected:

	typedef struct structSUBITEM
	{
		UINT nFormat;
		INT iWidth;
		INT iMinWidth;
		INT iMaxWidth;
		INT iImage;

		CString strText;

		structSUBITEM() : nFormat(RVCF_LEFT), iWidth(-1), iMinWidth(0), iMaxWidth(0), iImage(0) {};

	} SUBITEM, FAR* LPSUBITEM;

	typedef struct structTREEITEM
	{
		struct structTREEITEM* lptiParent;
		struct structTREEITEM* lptiChildren;
		struct structTREEITEM* lptiSibling;

		INT	iItem;
		INT iSubItem;

		BOOL bOpen;
		INT iIndent;

		structTREEITEM() :
			lptiParent(NULL),
			lptiChildren(NULL),
			lptiSibling(NULL),
			iItem(RVI_INVALID),
			iSubItem(-1),
			bOpen(FALSE),
			iIndent(0)
			{};

	} TREEITEM, FAR* LPTREEITEM;

	typedef struct structITEM
	{
		CReportData rdData;
		INT iBkColor;
		UINT nPreview;
		INT iIndent;
		UINT nState;
		LPARAM lParam;
		UINT64 Param64;

		LPTREEITEM lptiItem;

		structITEM() :
			iBkColor(-1),
			nPreview(0),
			iIndent(-1),
			nState(0),
			lParam(0),

			lptiItem(NULL)
			{};

		structITEM(const structITEM& item)
		{
			rdData = item.rdData;

			iBkColor = item.iBkColor;
			nPreview = item.nPreview;
			iIndent = item.iIndent;
			nState = item.nState;
			lParam = item.lParam;

			lptiItem = item.lptiItem;
		}

		structITEM& operator = (structITEM& item)
		{
			rdData = item.rdData;

			iBkColor = item.iBkColor;
			nPreview = item.nPreview;
			iIndent = item.iIndent;
			nState = item.nState;
			lParam = item.lParam;

			lptiItem = item.lptiItem;

			return *this;
		}

	} ITEM, FAR* LPITEM;

	typedef struct structCACHEITEM
	{
		INT		iItem;
		ITEM	item;

	} CACHEITEM, FAR* LPCACHEITEM;

// Implementation
protected:
	DWORD m_cRefCount;
	struct IDropTargetHelper *m_pDropTargetHelper;
	IDataObject * m_pDropDataObj;
	
	BOOL m_bSubclassFromCreate;

	BOOL m_bDoubleBuffer;
	BOOL m_bRedrawFlag;
	INT m_iSpacing;
	UINT m_nRowsPerWheelNotch;

	DWORD m_dwStyle;
	DWORD m_dwStyleEx;

	BOOL m_bPreview;
	BOOL m_bHorzScrollBar;

	CFont m_font;
	CFont m_fontBold;

	CImageList* m_pImageList;
	CImageList* m_pCheckList;
	SIZE m_sizeImage;
	SIZE m_sizeCheck;

	COLORREF m_crBackground;
	COLORREF m_crBkSelected;
	COLORREF m_crBkSelectedNoFocus;
	COLORREF m_crText;
	COLORREF m_crTextSelected;
	COLORREF m_crTextSelectedNoFocus;
	COLORREF m_crGrid;
	COLORREF m_cr3DFace;
	COLORREF m_cr3DLight;
	COLORREF m_cr3DShadow;
	COLORREF m_cr3DHiLight;
	COLORREF m_cr3DDkShadow;

	CArray<COLORREF, COLORREF> m_arrayColors;
	CPalette m_palette;

	INT m_iGridStyle;
	UINT m_nFrameControlStyle;
	UINT m_nBlendStyle;
	COLORREF m_crBlendColor;

	CString m_strNoItems;
	CString m_strSortBy;

	CRect m_rectTop;
	CRect m_rectHeader;
	CRect m_rectEdit;
	CRect m_rectReport;

	INT m_iDefaultWidth, m_iDefaultHeight;
	INT m_iVirtualWidth, m_iVirtualHeight;

	CArray<SUBITEM, SUBITEM&> m_arraySubItems;
	CArray<ITEM, ITEM&> m_arrayItems;

	TREEITEM m_tiRoot;

	BOOL m_bEditValid;
	ITEM m_itemEdit;

	BOOL m_bUseItemCacheMap;
	CACHEITEM m_aciCache[REPORTCTRL_MAX_CACHE];

	CList<INT, INT> m_listSelection;

	BOOL m_bColumnsReordered;
	CArray<INT, INT> m_arrayColumns;

	BOOL m_bFocus;
	INT m_iFocusRow;
	INT m_iFocusColumn;
	INT m_iSelectRow;
	CArray<INT, INT> m_arrayRows;

	BOOL m_bIndentColumn;
	BOOL m_bIndentGrey;
	INT m_iIndentColumn;
	INT m_iIndentColumnPending;

	BOOL m_bProcessKey;

	BOOL m_bUpdateItemMap;
	CMap<INT, INT, INT, INT> m_mapItemToRow;

	INT m_iEditItem;
	INT m_iEditSubItem;
	HWND m_hEditWnd;

	LPFNRVCOMPARE m_lpfnrvc;
	LPARAM m_lParamCompare;

	CReportHeaderCtrl m_wndHeader;
	CReportTipCtrl m_wndTip;
	CReportSubItemListCtrl* m_lprsilc;

	CBitmap m_bitmap;
	SIZE m_sizeBitmap;

	ULONG m_uNotifyMask;
	
	CPoint m_lastRClickPos;
	CPoint m_lastLClickPos;

	INT m_nLastToggledItem;

	UINT m_nAutoscrollTimerticks;
	
	virtual BOOL Create();

	virtual void GetSysColors();
	virtual UINT GetMouseScrollLines();
	BOOL CreatePalette();

	BOOL NotifyHdr(LPNMRVHEADER lpnmrvhdr, UINT nCode, INT iHdrItem);
	BOOL Notify(UINT nCode, INT iItem = RVI_INVALID, INT iSubItem = -1, UINT nState = 0, LPARAM lParam = 0);
	BOOL Notify(UINT nCode, UINT nKeys, LPRVHITTESTINFO lprvhti);

	void Layout(INT cx, INT cy);

	virtual LPITEM CacheLookup(INT iItem);
	virtual LPITEM CacheAdd(INT iItem);
	virtual void CacheDelete(INT iItem = RVI_INVALID);

	void BuildTree();
	void BuildTree(LPTREEITEM lpti);

	void InsertTree(INT iSubItem, LPTREEITEM lptiParent, LPTREEITEM lptiInsertAfter, LPTREEITEM lpti, BOOL bAscending);

	BOOL ExpandAllImpl(HTREEITEM hRoot, UINT nCode, INT iLevels, INT iDeep);

	INT InsertItemImpl(LPRVITEM lprvi);
	BOOL DeleteItemImpl(INT iItem);
	BOOL DeleteItemImpl(LPTREEITEM lpti, BOOL bChildrenOnly);

	virtual ITEM& GetItemStruct(INT iItem, INT iSubItem, UINT nMask = 0);
	virtual void SetItemStruct(INT iItem, ITEM& item);
	virtual BOOL LoadItemData(LPITEM lpItem, INT iItem, INT iSubItem, UINT nMask);

	INT GetItemFromRow(INT iRow);
	INT GetItemFromRow(INT iRow, ITEM& item);
	INT GetRowFromItem(INT iItem);
	INT GetSubItemFromColumn(INT iColumn);
	INT GetColumnFromSubItem(INT iSubItem);

	LPTREEITEM GetVisibleAncestor(LPTREEITEM lpti);
	LPTREEITEM GetVisibleAncestor(LPTREEITEM lpti, LPBOOL lpbExit);

	BOOL SetSubItemWidthImpl(INT iSubItem, INT iWidth);

	virtual void GetExpandedItemText(LPRVITEM lprvi);

	void SetState(INT iRow, UINT nState, UINT nMask);
	UINT GetState(INT iRow);

	LPTREEITEM GetTreeFocus();
	void SetTreeFocus(LPTREEITEM lptiFocus);

	void DeselectDescendents(LPTREEITEM lptiParent);

	void ReorderColumns();

	BOOL GetRowRect(INT iRow, INT iColumn, LPRECT lpRect, UINT nCode = RVIR_BOUNDS);
	INT GetVisibleRows(BOOL bUnobstructed = TRUE, LPINT lpiFirst = NULL, LPINT lpiLast = NULL, BOOL bReverse = FALSE);
	virtual void SelectRows(INT iFirst, INT iLast, BOOL bSelect = TRUE, BOOL bKeepSelection = FALSE, BOOL bInvert = FALSE, BOOL bNotify = TRUE);

    INT GetScrollPos32(INT iBar, BOOL bGetTrackPos = FALSE);
    BOOL SetScrollPos32(INT iBar, INT nPos, BOOL bRedraw = TRUE);
	void ScrollWindow(INT iBar, INT iPos);

	void EnsureVisibleColumn(INT iColumn);

	void IndentFirstColumn();
	void UnindentFirstColumn();

	void RedrawRows(INT iFirst, INT iLast = RVI_INVALID, BOOL bUpdate = FALSE);

	void SelectionToSortedArray(CArray<INT, INT>& arraySelection);

	virtual void DrawCtrl(CDC* pDC);
	virtual void DrawRow(CDC* pDC, CRect rectRow, CRect rectClip, INT iRow, LPRVITEM lprvi, BOOL bAlternate);
	virtual INT DrawTreeBox(CDC* pDC, CRect rectBox, BOOL bOpen);
	virtual INT DrawImage(CDC* pDC, CRect rect, LPRVITEM lprvi);
	virtual INT DrawCheck(CDC* pDC, CRect rect, LPRVITEM lprvi);
	virtual INT DrawText(CDC* pDC, CRect rect, LPRVITEM lprvi);

	virtual void QuickSort(INT iSubItem, INT iLow, INT iHigh, BOOL bAscending);
	virtual void TreeSort(INT iSubItem, LPTREEITEM lptiParent, BOOL bAscending, BOOL bTraverse = TRUE);

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportCtrl)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSysColorChange();
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnHdnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnItemClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnEndDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnDividerDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRvnEndItemEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnQueryNewPalette();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
    afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetFont(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportView

class CReportView : public CView
{
protected:
	DECLARE_DYNCREATE(CReportView)
	CReportView();           // protected constructor used by dynamic creation

// Attributes
public:
	CReportCtrl& GetReportCtrl();
	virtual CReportCtrl* GetReportCtrlPtr();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CReportView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	BOOL m_bCreated;
	CReportCtrl m_wndReportCtrl;

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CReportSubItemListCtrl window

class CReportSubItemListCtrl : public CDragListBox
{
// Construction
public:
	CReportSubItemListCtrl();
	~CReportSubItemListCtrl();

// Attributes
public:
	BOOL SetReportCtrl(CReportCtrl* pReportCtrl);
	CReportCtrl* GetReportCtrl();

// Operations
public:
	BOOL UpdateList();

// Overrides
	virtual BOOL Include(INT iSubItem);
	virtual BOOL Disable(INT iSubItem);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReportSubItemListCtrl)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
protected:
	CReportCtrl* m_pReportCtrl;

	INT m_iSubItem;
	HDITEM m_hdiSubItem;
	TCHAR m_szSubItemText[FLATHEADER_TEXT_MAX];

	CFHDragWnd* m_pDragWnd;
	CRect m_rcDragWnd;
	CRect m_rcDropTarget1;
	CRect m_rcDropTarget2;

	INT m_iDropIndex;

	BOOL BeginDrag(CPoint pt);
	UINT Dragging(CPoint pt);
	void CancelDrag(CPoint pt);
	void Dropped(INT iSrcIndex, CPoint pt);

	// Generated message map functions
protected:
	//{{AFX_MSG(CReportSubItemListCtrl)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPORTCTRL_H__279B1CA0_D7F2_11D2_88D7_ABB23645F26D__INCLUDED_)
