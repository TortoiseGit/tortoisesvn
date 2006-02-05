////////////////////////////////////////////////////////////////////////////
//	File:		ReportCtrl.cpp
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
//	Version history
//
//	1.0.1	- Initial release.
//	1.1.0	- Changed copyright notice.
//			- Added RVN_LAYOUTCHANGED notification message.
//			- Fixed SB_THUMBPOSITION and SB_THUMBTRACK problems.
//			- Removed IDC_HEADERCTRL and IDC_REPORTCTRL definitions.
//			  Now hardcoded because conflicts with some implementations.
//			- Added SortAllColumns(), contributed by Roger Parkinson.
//			- Added Chris Hambleton's suggestion to sort on image.
//			  indices when the (sub)item does not have text.
//			- Fixed DeleteAllItems(), as suggested by Paul Leung.
//			- Fixed DeleteAllItems() focus problem, as suggested by Dmitry ??.
//			- Fixed DeleteItem(), as noted by Eugenio Ciceri.
//			- Added fixes and suggestions of Serge Weinstock:
//				- Fixed GetNextSelectedItem().
//				- Fixed "no items to show" position.
//				- Added support for WS_BORDER style.
//				- Added RVN_ITEMDELETED notification.
//			- Added mouse-wheel support.
//			- Added extended horizontal grid style and changed regular
//			  horizontal grid style.
//			- Fixed selection inversion using space key.
//			- Fixed focus selection using ctrl key.
//			- Fixed focus on deleted items.
//			- Added RVS_OWNERDATA style.
//			- Changed notification structure to facilitate
//			  RVS_OWNERDATA style.
//			- Changed hit test info structure to facilitate
//			  RVS_FOCUSSUBITEMS style.
//			- Added RVS_FOCUSSUBITEMS style to enable focus on individual
//			  subitems to facilitate subitem editing.
//			- Added dialog control ID to WPARAM of notification messages.
//			- Added subitem editing functionality.
//			- Added CReportEditCtrl edit control.
//			- Added CReportComboCtrl edit control.
//			- Added support for RVIM_TEXT on empty strings (required for
//			  editing).
//			- Added RVIS_READONLY state.
//			- Added rect member to RVHITTESTINFO structure.
//			- Added RVS_EXPANDSUBITEMS style.
//			- Added CReportTipCtrl, based on code by Zafir Anjum.
//			- Added background image support, based on code contributed by
//			  Ernest Laurentin.
//	1.1.1	- Added RVN_KEYDOWN notification.
//			- Fixed DeleteItem as suggested by Thomas Freudenberg
//			  (Luca; this time I tested the function :-).
//			- Removed use of GetBuffer() in CReportData::GetSubItem()
//			  to improve performance.
//			- Improved sorting performance by using quick sort instead
//			  of bubble sort (QuickSort based on Martin Ziacek's
//			  QArray implementation).
//			- Added item to row mapping for improved lookup performance.
//			- Fixed GetItemFromRow to retrieve ITEM in OWNERDATA style.
//			- Added item/row manipulation functions.
//			- Fixed column reordering and storage for invisible
//			  controls.
//			- Added GetItemHeight function.
//			- Added NMRVHEADER notification structure.
//			- Replaced RVN_COLUMNCLICK with RVN_HEADERCLICK. This
//			  new notification uses the new NMRVHEADER structure.
//			- Added MeasureItem function.
//			- Added RVN_DIVIDERDBLCLICK notification and code that
//			  implements optimal column sizing.
//			- Added right mouse button handling and RVN_ITEMRCLICK
//			  notification.
//			- Added item cache to optimize lookup of ITEM structures
//			  in RVS_OWNERDATA style.
//			- Fixed a bug related to creation of the control from
//			  from dialog templates.
//			- Fixed double RVN_ENDITEMEDIT messages in edit controls.
//			- Fixed edit cancelation on style changes.
//			- Fixed control keys in edit controls.
//			- Added AddItem operations.
//			- Added FindItem operation.
//			- Changed text string parameter of InsertItem to LPCTSTR.
//			- Changed behavior of edit row and RVS_FOCUSSUBITEMS style.
//			  The focus on the edit row is now always on individual columns,
//			  while the focus on other rows is only on individual columns
//			  if the RVS_FOCUSSUBITEMS style is used.
//			- Fixed a bug related to editing of edit row while not visible.
//			- Added BeginEdit functions to edit controls.
//			- Changed 'Column' handling function names in order to stop
//			  confusion about what is a column and what is a subitem.
//			  A column is a visible (or active) subitem.
//			- Added GetSubItemWidth and SetSubItemWidth functions, courtesy
//			  of Jonathan Kotas.
//			- Removed restriction to sort on columns only.
//			- Added Focus manipulation functions, suggested by Attila Hajdrik.
//			- Added subitem persistent style, as suggested by Attila Hajdrik.
//			- Fixed assertion on column invisible deactivation.
//			- Fixed several bugs related to subitem dynamic (de)activation and
//			  column manipulation.
//			- Added RVP_SORTTOOLTIP property to allow changing "Sort by"
//			  tooltip.
//			- Fixed pResult assignment in OnHdnItemClick.
//			- Added ResortItems function.
//			- Removed restriction for at least one visible subitem.
//			- Added GetActiveSubItemCount().
//			- Added flat style to CReportSubItemListCtrl
//			  (use WS_EX_STATICEDGE).
//			- Added subitem disable to CReportSubItemListCtrl.
//			- Added UndefineAllSubItems, as suggested by Brian Pollack.
//			- Added RVCF_SUBITEM_NOFOCUS, as suggested by Attila Hajdrik.
//			- Added RVP_NOTIFYMASK property for setting a mask on what
//			  notifications must be generated, as suggested by A. Hajdrik.
//			- Made Notify function virtual.
//			- Fixed OnVScroll thumb track for item count > 32767.
//			- Improved SelectRows performance by using list instead of
//			  itteration through all items to clear the previous selection.
//			- Fixed several glitches related to subitem activation.
//			- Added IsItemVisible().
//			- Fixed scrollbar assertion in SetItemCount().
//			- Improved performance of CReportData.
//			- Added PreviewHeight function to calculate the height of
//			  preview text.
//			- Made several implementation function virtual.
//			- Minor performance improvement by replacing printf with prepared
//			  fixed in New.
//	1.1.2	- Fixed MoveUp/MoveDown related selection bug.
//			- Fixed focus on item sort as suggested by Matija Jerkovic.
//			- Fixed GetProfile and improved GetItem as suggested by
//			  Eugenio Ciceri.
//			- Several fixes to CReportView as suggested by Eugenio Ciceri.
//			- Fixed ClearSelection, added InvertSelection SelectAll and
//			  SetSelection variants (SelectMatch and SelectLessEq) as
//			  contributed by Eugenio Ciceri.
//			- Fixed SelectRows as suggested by Matija Jerkovic.
//			- Fixed InsertItem and DeleteItem as suggested by Eugenio Ciceri.
//			- Fixed DeleteAllItems.
//			- Fixed grid property, thanks to Roland Kopetzky.
//			- Applied fixes to edit row as suggested by Alex.
//			- Added support for disabled style.
//			- Fixed obstructed bottom selected row redraw problem.
//			- Added GetColor, SetColor and DeleteAllColors methods.
//			- Fixed (ctrl+)page-up assertion in empty reports.
//			- Added UpdateWindow on strategic places to improve
//			  drawing on slower or heavy loaded systems.
//			- Fixed assertion in HitTest on edit row.
//			- Added "browse" button option to CReportEditCtrl.
//			- Added RVN_BUTTONCLICK notification.
//			- Added RVIS_OWNERDRAW state and RVN_ITEMDRAW notification
//			  to support owner drawn subitems.
//			- Changed default font to default GUI font.
//			- Added second PreviewHeight function to calculate the height of
//			  preview text from the text and an optional rectangle.
//			- Fixed a selection bug in SetItemCount().
//			- Fixed another selection bug in DeleteItem().
//			- Fixed FindItem for LPARAM as suggested by Armin Zürcher.
//	2.0.0	- Made scrolling with left or right cursor key dependent on
//			  focus subitems style.
//			- Fixed resource leak in empty list, as suggested by Florent
//			  Odelain.
//			- Fixed ResortItems and DeactivateSubItem bugs as suggested by
//			  Rafael Lombardi Santos.
//			- Fixed HitTest on reordered columns, reported by Trevor Ash.
//			- Fixed item rect returned in hitinfo structure for scrolled
//			  controls. This fixes incorrect positioning of tip windows.
//			- Added CReportHeaderCtrl to access CFlatHeaderCtrl's protected
//			  members.
//			- Added first column indent functions, to support hierarchy
//			  GUI elements.
//			- Fixed HitTest to retrieve correct item data.
//			- Added parameter to sort callback.
//			- Removed SortAllSubItems, because implementation was not
//			  maintained and buggy.
//			- Fixed IsItemVisible and added GetTopIndex(), PageUp() and
//			  PageDown() as suggested by Alina Kozlovsky.
//			- Added tree control features, both in preparation of group
//			  view mode as well as a standalone feature.
//			- Fixed subitem text drawing following uninitialized subitems.
//			- Changed CompareItems callback and functions to support
//			  separate subitems to enable tree view and group view sorting.
//			- Extended CompareItems to sort on checkboxes as suggested by
//			  Peter Lagerhem.
//			- Fixed offset of subitem tip for items with images or checks
//			  and text and related hittesting issues.
//			- Fixed NOHEADER style.
//			- Changed selected items and tree boxes in tree view mode visuals
//			  to match common control look and feel.
//			- Fixed default height setting for font size updates with images.
//			- Fixed cosmetic bug with RVS_SHOWHGRID style, as suggested by Matrix.
//			- Fixed SelectAll() for empty control as suggested by Dmitry Sazonov.
//			- Added GetStyle().
//			- Added RVN_HEADERRCLICK to support popup menus on header.
//			- Fixed tooltip double click and ALT key relay events by removing
//			  mouse capture.
//	2.0.1	- Fixed bug in DeleteItem for tree control mode.
//			- Fixed item expansion for single subitem hierarchy items.
//			- Added GetExpandedItemText to allow expanded items to show different
//			  text from the subitem text.
//			- Added support for radio button and disabled check marks and 
//			  radio buttons.
//			- Added RVP_ENABLEFLATCHECKMARK property, to control the visual style
//			  of check marks or radio buttons.
//			- Added support for check mark and radio button image list.
//			- Changed CurrentFocus() to GetCurrentFocus().
//			- Changed OnKillFocus to recognize all child windows.
//			- Extended MeasureItem function.
//			- Fixed GetNextItem() for RVTI_ROOT.
//			- Added support for SetRedraw suggested by Phil J Pearson.
//			- Optimized InsertItem performance by skipping GetRowFromItem
//			  when the focus is not on an reorderable row, as suggested by
//			  Phil J Pearson.
//			- Added disabled background function to CReportSubItemListCtrl.
//			- Added pre-create style passing to CReportView.
//			- Adjusted edit box position.
//			- Fixed a bug related to keydown messages in unfocused state.
//			- Fixed tip redraw problem and tip background color mismatch.
//			- Fixed kill tip on WM_KILLFOCUS, finally fixing click and double
//			  click on expanded subitems.
//			- Added GetSelectedItems method.
//			- Fixed a selection bug in ClearSelection.
//			- Made GetReportCtrlPtr virtual and changed CReportView to use
//			  overriden function to get a pointer to the embedded CReportCtrl.
//			  This allows control derived from CReportCtrl to be embedded in 
//			  CReportView.
//			- Added UpdateWindow to SetRedraw when re-enabling redrawing.
//			- Removed legacy definitions from header file.
//			- Added blended image support through state bits, focus and selection
//			  through the RVP_ENABLEIMAGEBLENDING and RVP_BLENDCOLOR properties.
//			- Added support for overlay images.
//			- Added Win2K tip fading to CReportTipCtrl.
//			- Fixed nFormat subitem member update in OnHdnItemChanged, as suggested
//			  by Sven Ritter.
//			- Added selective item cache flush to SetItemCount.
//			- Added GetItemString() function to retrieve an item as a string.
//			- Added clipboard Copy support and clipboard separator and indent
//			  properties.
//			- Fixed OnRvnEndItemEdit to not loose lParam, as suggested by
//			  Paul Hurley.
//			- Added GetSortSettings() to retrieve sorting settings.
//			- Made SelectRows virtual to allow owner data multiple selection
//			  management.
//			- Fixed a bug in DeleteItem for trees.
//			- Fixed various mouse button, keyboard selection/focus issues.
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ReportCtrl.h"

#include "MemDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REPORTDATA_MAX_NODATA	20

static TCHAR g_cSeparator = _T('|');
static TCHAR g_cCBSeparator = _T('\t');
static CString g_strCBIndent = _T("  ");

static TCHAR g_szNoData[REPORTDATA_MAX_NODATA];

static lpfnUpdateLayeredWindow g_lpfnUpdateLayeredWindow = NULL;
static lpfnSetLayeredWindowAttributes g_lpfnSetLayeredWindowAttributes = NULL;

#define IsShiftDown()	( (GetKeyState(VK_SHIFT) & (1 << (sizeof(SHORT)*8-1))) != 0   )
#define IsCtrlDown()	( (GetKeyState(VK_CONTROL) & (1 << (sizeof(SHORT)*8-1))) != 0 )


/////////////////////////////////////////////////////////////////////////////
// CReportData

CReportData::CReportData()
{
}

CReportData::~CReportData()
{
}

BOOL CReportData::New(INT iSubItems)
{
	LPTSTR lpsz = GetBuffer(iSubItems*REPORTDATA_MAX_NODATA);

	lpsz[0] = 0;

	for(INT i=0;i<iSubItems;i++)
	{
		_tcscat_s( lpsz, REPORTDATA_MAX_NODATA, g_szNoData );
		lpsz += _tcslen(g_szNoData);
	}
	ReleaseBuffer();

	return TRUE;
}

BOOL CReportData::GetSubItem(INT iSubItem, LPINT lpiImage, LPINT lpiOverlay, LPINT lpiCheck, LPINT lpiColor, LPTSTR lpszText, LPINT lpiTextMax)
{
	INT i, iPos, iText;

	for(i=0,iPos=0;i<iSubItem&&iPos>=0;i++,iPos++)
		iPos = Find(g_cSeparator, iPos);

	if(iPos<0)
		return FALSE;

	LPCTSTR lpsz = *this;
	lpsz = &lpsz[iPos];
	VERIFY(_stscanf_s(lpsz, _T("(%d,%d,%d,%d,%d)"), lpiImage, lpiOverlay, lpiCheck, lpiColor, &iText));

	if(iText < 0)
		*lpiTextMax = -1;

	if(*lpiImage == -1 && *lpiOverlay == -1 && *lpiCheck == -1 && *lpiColor == -1 && *lpiTextMax == -1)
		return FALSE;

	if(iText < 0)
		return TRUE;

	lpsz = _tcspbrk(lpsz, _T(")"))+1;
	if(lpsz && lpszText)
	{
		INT iTextSize=0;
		for(iTextSize=0;iTextSize<(*lpiTextMax)-1 && *lpsz!=g_cSeparator;iTextSize++)
			lpszText[iTextSize] = *lpsz++;

		lpszText[iTextSize] = 0;
	}

	return TRUE;
}

BOOL CReportData::SetSubItem(INT iSubItem, INT iImage, INT iOverlay, INT iCheck, INT iColor, LPCTSTR lpszText)
{
	if(!InsertSubItem(iSubItem, iImage, iOverlay, iCheck, iColor, lpszText))
		return FALSE;

	if(!DeleteSubItem(iSubItem+1))
		return FALSE;

	return TRUE;
}

BOOL CReportData::InsertSubItem(INT iSubItem, INT iImage, INT iOverlay, INT iCheck, INT iColor, LPCTSTR lpszText)
{
	INT i, iPos, iText;

	for(i=0,iPos=0;i<iSubItem&&iPos>=0;i++,iPos++)
		iPos = Find(g_cSeparator, iPos);

	if(iPos<0)
		return FALSE;

	if(lpszText == NULL)
	{
		lpszText = _T("");
		iText = -1;
	}
	else
		iText = (INT)_tcslen(lpszText);

	TCHAR sz[32+REPORTCTRL_MAX_TEXT];
	_stprintf_s(sz, 32+REPORTCTRL_MAX_TEXT, _T("(%d,%d,%d,%d,%d)%s%c"), iImage, iOverlay, iCheck, iColor, iText, lpszText, g_cSeparator);

	Insert(iPos, sz);
	return TRUE;
}

BOOL CReportData::DeleteSubItem(INT iSubItem)
{
	INT i, iPos1, iPos2;

	for(i=0,iPos1=0;i<iSubItem&&iPos1>=0;i++,iPos1++)
		iPos1 = Find(g_cSeparator, iPos1);

	if(iPos1<0)
		return FALSE;

	iPos2 = Find(g_cSeparator, iPos1);
	if(iPos2++<0)
		return FALSE;

	Delete(iPos1, iPos2-iPos1);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CReportView

IMPLEMENT_DYNCREATE(CReportView, CView)

CReportView::CReportView()
{
	m_bCreated = FALSE;
}

CReportView::~CReportView()
{
}

BOOL CReportView::PreCreateWindow(CREATESTRUCT& cs) 
{
	BOOL bResult = CView::PreCreateWindow(cs);

	m_wndReportCtrl.m_dwStyle = cs.style&0xFFFF;
	return bResult;
}

void CReportView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	if(!m_bCreated && GetReportCtrlPtr() == &m_wndReportCtrl)
	{
		m_wndReportCtrl.m_dwStyle |= WS_CHILD|WS_TABSTOP|WS_VISIBLE;

		CRect rect;
		GetClientRect(rect);

		if(m_wndReportCtrl.Create(m_wndReportCtrl.m_dwStyle, rect, this, 0) == NULL)
			AfxThrowMemoryException();
	}
	
	m_bCreated = TRUE;
}

BEGIN_MESSAGE_MAP(CReportView, CView)
	//{{AFX_MSG_MAP(CReportView)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportView drawing

void CReportView::OnDraw(CDC* /*pDC*/)
{
	;
}

/////////////////////////////////////////////////////////////////////////////
// CReportView diagnostics

#ifdef _DEBUG
void CReportView::AssertValid() const
{
	CView::AssertValid();
}

void CReportView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CReportView attributes

CReportCtrl& CReportView::GetReportCtrl()
{
	ASSERT(GetReportCtrlPtr() == &m_wndReportCtrl);	// Don't use this function
													// GetReportCtrlPtr is overridden.
	return m_wndReportCtrl;
}

CReportCtrl* CReportView::GetReportCtrlPtr()
{
	return &m_wndReportCtrl;
}

/////////////////////////////////////////////////////////////////////////////
// CReportView implementation

BOOL CReportView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CReportView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if(m_bCreated && GetReportCtrlPtr()->GetSafeHwnd() != NULL)
	{
		CRect rect;
		GetClientRect(rect);
		GetReportCtrlPtr()->MoveWindow(rect);
	}
}

void CReportView::OnSetFocus(CWnd* /*pOldWnd*/)
{
	if(m_bCreated && GetReportCtrlPtr()->GetSafeHwnd() != NULL)
		GetReportCtrlPtr()->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// CReportCtrl

IMPLEMENT_DYNCREATE(CReportCtrl, CWnd)

CReportCtrl::CReportCtrl() :
	m_pDropTargetHelper(NULL) ,
	m_cRefCount(0)
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!(::GetClassInfo(hInst, REPORTCTRL_CLASSNAME, &wndclass)))
	{
        wndclass.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
		wndclass.lpfnWndProc = ::DefWindowProc;
		wndclass.cbClsExtra = wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInst;
		wndclass.hIcon = NULL;
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = REPORTCTRL_CLASSNAME;

		if (!AfxRegisterClass(&wndclass))
			AfxThrowResourceException();
	}

	m_bSubclassFromCreate = FALSE;

	m_bDoubleBuffer = TRUE;
	m_bRedrawFlag = TRUE;
	m_iSpacing = 6;
	m_nRowsPerWheelNotch = GetMouseScrollLines();

	m_dwStyle = 0;

	m_bPreview = TRUE;
	m_bHorzScrollBar = TRUE;

	m_font.CreateStockObject( DEFAULT_GUI_FONT );

	m_pImageList = NULL;
	m_pCheckList = NULL;
	m_sizeImage.cx = 0;
	m_sizeImage.cy = 0;
	m_sizeCheck.cx = 8;
	m_sizeCheck.cy = 8;

	m_arrayColors.SetSize(0, 8);

	m_iGridStyle = PS_SOLID;
	m_nFrameControlStyle = DFCS_FLAT;
	m_nBlendStyle = RVP_BLEND_NONE;
	m_crBlendColor = RGB(0xff, 0xff, 0xff);

	m_strNoItems = _T("There are no items to show in this view.");
	m_strSortBy = _T("Sort by: %s");

	_stprintf_s(g_szNoData, REPORTDATA_MAX_NODATA, _T("(-1,-1,-1,-1,-1)%c"), g_cSeparator);

	m_iDefaultWidth = 200;
	m_iDefaultHeight = 10;
	m_iVirtualWidth = 0;
	m_iVirtualHeight = 0;

	m_arraySubItems.SetSize(0, 8);
	m_arrayItems.SetSize(0, 128);

	m_tiRoot.bOpen = TRUE;

	m_bEditValid = FALSE;

	m_bUseItemCacheMap = TRUE;
	for(UINT n=0;n<REPORTCTRL_MAX_CACHE;n++)
		m_aciCache[n].iItem = RVI_INVALID;

	m_bProcessKey = FALSE;

	m_bUpdateItemMap = FALSE;

	m_bColumnsReordered = FALSE;
	m_arrayColumns.SetSize(0, 8);

	m_bFocus = FALSE;
	m_iFocusRow = RVI_EDIT;
	m_iFocusColumn = -1;
	m_iSelectRow = 0;
	m_arrayRows.SetSize(0, 128);

	m_bIndentColumn = FALSE;
	m_bIndentGrey = FALSE;
	m_iIndentColumn = 0;
	m_iIndentColumnPending = -1;

	m_iEditItem = RVI_INVALID;
	m_iEditSubItem = -1;
	m_hEditWnd = NULL;

	m_lprsilc = NULL;
	m_lpfnrvc = NULL;
	m_lParamCompare = 0;

	m_uNotifyMask = RVNM_ALL;

	if(FAILED(CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
		IID_IDropTargetHelper,(LPVOID*)&m_pDropTargetHelper)))
		m_pDropTargetHelper = NULL;

	m_nLastToggledItem = -1;
}

CReportCtrl::~CReportCtrl()
{
	RevokeDragDrop(m_hWnd);
	m_wndHeader.DestroyWindow();
	m_wndTip.DestroyWindow();

	if(m_palette.m_hObject)
		m_palette.DeleteObject();
	if( m_bitmap.m_hObject != NULL )
		m_bitmap.DeleteObject();

    if(m_font.m_hObject)
		m_font.DeleteObject();
	if(m_fontBold.m_hObject)
		m_fontBold.DeleteObject();
}

BOOL CReportCtrl::Create()
{
	CRect rect(0, 0, 0, 0);
	DWORD dwStyle = HDS_HORZ|HDS_BUTTONS|HDS_FULLDRAG|HDS_DRAGDROP|CCS_TOP;
	if(!m_wndHeader.Create(dwStyle, rect, this, 0))
		return FALSE;

	if(!m_wndTip.Create(this))
		return FALSE;

	CWnd* pWnd = GetParent();
	if(pWnd)
	{
		CFont* pFont = pWnd->GetFont();
		if(pFont)
		{
			LOGFONT lf;
			pFont->GetLogFont(&lf);

			m_font.DeleteObject();
			m_font.CreateFontIndirect(&lf);
		}
	}

	OnSetFont((WPARAM)((HFONT)m_font), FALSE);
	m_wndHeader.SetFont(&m_font, FALSE);

	m_dwStyle = CWnd::GetStyle();

	GetClientRect(rect);
	Layout(rect.Width(), rect.Height());

	GetSysColors();
	RegisterDragDrop(m_hWnd, this);
	return TRUE;
}

BOOL CReportCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	m_bSubclassFromCreate = TRUE;

	if(!CWnd::Create(REPORTCTRL_CLASSNAME, NULL, dwStyle, rect, pParentWnd, nID, pContext))
		return FALSE;

	return Create();
}

void CReportCtrl::PreSubclassWindow()
{
	CWnd::PreSubclassWindow();

	if(!m_bSubclassFromCreate)
		if(!Create())
			AfxThrowMemoryException();
}

void CReportCtrl::OnDestroy()
{
	DeleteAllItems();
	CWnd::OnDestroy();
}

void CReportCtrl::OnTimer(UINT_PTR nIDEvent)
{
	if ( nIDEvent==REPORTCTRL_AUTOEXPAND_TIMERID )
	{
		KillTimer( REPORTCTRL_AUTOEXPAND_TIMERID );

		CPoint currentPoint;
		GetCursorPos(&currentPoint);
		ScreenToClient(&currentPoint);

		RVHITTESTINFO rvhti;
		rvhti.point = currentPoint;
		HitTest(&rvhti);
		HTREEITEM hItem = rvhti.hItem;

		if (hItem)
		{
			if(m_dwStyle&RVS_TREEMASK && rvhti.nFlags&(RVHT_ONITEM|RVHT_ONITEMTREEBOX))
			{
				if(!Notify(RVN_ITEMEXPANDING, rvhti.iItem, rvhti.iSubItem))
				{
					Expand((HTREEITEM)m_arrayItems[rvhti.iItem].lptiItem, RVE_TOGGLE);
					Notify(RVN_ITEMEXPANDED, rvhti.iItem, rvhti.iSubItem);
				}
			}
		}
	}
	else
	{
		CWnd::OnTimer(nIDEvent);
	}
}

BEGIN_MESSAGE_MAP(CReportCtrl, CWnd)
	//{{AFX_MSG_MAP(CReportCtrl)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_SETTINGCHANGE()
	ON_NOTIFY(HDN_ITEMCHANGED, 0, OnHdnItemChanged)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemClick)
	ON_NOTIFY(HDN_BEGINDRAG, 0, OnHdnBeginDrag)
	ON_NOTIFY(HDN_ENDDRAG, 0, OnHdnEndDrag)
	ON_NOTIFY(HDN_DIVIDERDBLCLICK, 0, OnHdnDividerDblClick)
	ON_NOTIFY(RVN_ENDITEMEDIT, 0, OnRvnEndItemEdit)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_MOUSEWHEEL()
	ON_WM_CHAR()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETFONT, OnSetFont)
    ON_MESSAGE(WM_GETFONT, OnGetFont)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportCtrl attributes

DWORD CReportCtrl::GetStyle() const
{
	return (CWnd::GetStyle()&0xFFFF0000)|m_dwStyle;
}

BOOL CReportCtrl::ModifyProperty(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case RVP_SPACING:
		m_iSpacing = (INT)lParam;
		break;

	case RVP_CHECK:
		m_sizeCheck.cx = LOWORD(lParam);
		m_sizeCheck.cy = HIWORD(lParam);
		break;

	case RVP_NOITEMTEXT:
		m_strNoItems = (LPCTSTR)lParam;
		break;

	case RVP_GRIDSTYLE:
		switch(lParam)
		{
		case RVP_GRIDSTYLE_DOT:		m_iGridStyle = PS_DOT; break;
		case RVP_GRIDSTYLE_DASH:	m_iGridStyle = PS_DASH; break;
		case RVP_GRIDSTYLE_SOLID:	m_iGridStyle = PS_SOLID; break;
		default:
			return FALSE;
		}
		break;

	case RVP_SORTTOOLTIP:
		m_strSortBy = (LPCTSTR)lParam;
		break;

	case RVP_NOTIFYMASK:
		m_uNotifyMask = (ULONG)lParam;
		break;

	case RVP_ENABLEITEMCACHEMAP:
		m_bUseItemCacheMap = (BOOL)lParam;
		break;

	case RVP_SEPARATOR:
		g_cSeparator = (TCHAR)lParam;
		_stprintf_s(g_szNoData, REPORTDATA_MAX_NODATA, _T("(-1,-1,-1,-1)%c"), g_cSeparator);
		break;

	case RVP_ENABLEPREVIEW:
		m_bPreview = (BOOL)lParam;
		break;

	case RVP_ENABLEHORZSCROLLBAR:
		m_bHorzScrollBar = (BOOL)lParam;
		break;

	case RVP_FRAMECONTROLSTYLE:
		switch(lParam)
		{
		case RVP_FCSTYLE_FLAT:		m_nFrameControlStyle = DFCS_FLAT; break;
		case RVP_FCSTYLE_MONO:		m_nFrameControlStyle = DFCS_MONO; break;
		case RVP_FCSTYLE_NORMAL:	m_nFrameControlStyle = 0; break;
		default: 
			return FALSE;
		}
		break;

	case RVP_ENABLEIMAGEBLENDING:
		m_nBlendStyle = (UINT)lParam;
		break;

	case RVP_BLENDCOLOR:
		m_crBlendColor = (COLORREF)lParam;
		break;

	case RVP_CBSEPARATOR:
		g_cCBSeparator = (TCHAR)lParam;
		break;

	case RVP_CBINDENT:
		g_strCBIndent = (LPCTSTR)lParam;
		break;

	default:
		return FALSE;
	}

	CRect rect;
	GetClientRect(rect);
	Layout(rect.Width(), rect.Height());

	return TRUE;
}

INT CReportCtrl::ActivateSubItem(INT iSubItem, INT iColumn)
{
	ASSERT(iSubItem>=0);
	ASSERT(iSubItem<m_arraySubItems.GetSize());	// Specify a valid subitem

	SUBITEM& subitem = m_arraySubItems[iSubItem];
	INT iResult = -1;

	if(GetColumnFromSubItem(iSubItem) < 0)
	{
		UnindentFirstColumn();

		try
		{
			HDITEM hdi;
			hdi.mask = HDI_FORMAT|HDI_WIDTH|HDI_LPARAM|HDI_ORDER;
			hdi.fmt = subitem.nFormat&RVCF_MASK;
			hdi.cxy = subitem.iWidth;
			hdi.iOrder = iColumn;
			hdi.lParam = (LPARAM)iSubItem;

			if(subitem.nFormat&RVCF_IMAGE)
			{
				hdi.mask |= HDI_IMAGE;
				hdi.iImage = subitem.iImage;
			}

			if(subitem.nFormat&RVCF_TEXT)
			{
				hdi.mask |= HDI_TEXT;
				hdi.pszText = subitem.strText.GetBuffer(0);
			}

			iResult = m_wndHeader.InsertItem((int)m_arrayColumns.GetSize(), &hdi);
			if(iResult >= 0)
			{
				m_iVirtualWidth += subitem.iWidth;

				HDITEMEX hditemex;
				hditemex.nStyle = (subitem.nFormat&RVCF_EX_MASK)>>16;
				hditemex.iMinWidth = subitem.iMinWidth;
				hditemex.iMaxWidth = subitem.iMaxWidth;

				if(hditemex.nStyle&HDF_EX_TOOLTIP)
					hditemex.strToolTip.Format(m_strSortBy, subitem.strText);

				m_wndHeader.SetItemEx(iResult, &hditemex);

				hdi.mask = HDI_WIDTH;
				m_wndHeader.GetItem(iResult, &hdi);
				subitem.iWidth = hdi.cxy;

				ReorderColumns();

				ScrollWindow(SB_HORZ, GetScrollPos32(SB_HORZ));
			}

			IndentFirstColumn();
		}
		catch(CMemoryException* e)
		{
			e->Delete();
			if(iResult >= 0)
				m_wndHeader.DeleteItem(iResult);
		}
	}

	return iResult;
}

BOOL CReportCtrl::DeactivateSubItem(INT iSubItem)
{
	ASSERT(iSubItem>=0);
	ASSERT(iSubItem<m_arraySubItems.GetSize());	// Specify a valid subitem

	INT iColumn = GetColumnFromSubItem(iSubItem);
	if(iColumn < 0)
		return FALSE;

	UnindentFirstColumn();

	BOOL bAscending;
	INT iSortColumn = m_wndHeader.GetSortColumn(&bAscending);
	if(iColumn == iSortColumn)
		m_wndHeader.SetSortColumn(-1, TRUE);
	else if(iColumn < iSortColumn)
		m_wndHeader.SetSortColumn(m_arrayColumns[iSortColumn-1], bAscending);

	HDITEM hdi;
	hdi.mask = HDI_WIDTH;
	m_wndHeader.GetItem(m_arrayColumns[iColumn], &hdi);

	BOOL bResult = m_wndHeader.DeleteItem(m_arrayColumns[iColumn]);

	m_iVirtualWidth -= hdi.cxy;
	ReorderColumns();

	ScrollWindow(SB_HORZ, GetScrollPos32(SB_HORZ));

	IndentFirstColumn();
	return bResult;
}

BOOL CReportCtrl::DeactivateAllSubItems()
{
	INT iSubItems = (INT)m_arraySubItems.GetSize();
	for(INT iSubItem=0;iSubItem<iSubItems;iSubItem++)
	{
		if(IsActiveSubItem(iSubItem))
		{
			if(!DeactivateSubItem(iSubItem))
				return FALSE;
		}
	}

	return TRUE;
}

BOOL CReportCtrl::IsActiveSubItem(INT iSubItem)
{
	INT iColumn = GetColumnFromSubItem(iSubItem);
	return iColumn<0 ? FALSE:TRUE;
}

INT CReportCtrl::GetActiveSubItemCount()
{
	return (INT)m_arrayColumns.GetSize();
}

INT CReportCtrl::GetSubItemWidth(INT iSubItem)
{
#ifdef DEBUG
	INT iSubItems = (INT)m_arraySubItems.GetSize();
	ASSERT(iSubItem <= iSubItems);
#endif
	try
	{
		INT iWidth = m_arraySubItems[iSubItem].iWidth;

		if(
			m_bIndentColumn &&
			GetColumnFromSubItem(iSubItem) == 0
		)
			iWidth -= m_iIndentColumn;

		return iWidth;
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		return -1;
	}
}

BOOL CReportCtrl::SetSubItemWidth(INT iSubItem, INT iWidth)
{
	BOOL bResult;

	UnindentFirstColumn();
	bResult = SetSubItemWidthImpl(iSubItem, iWidth);
	IndentFirstColumn();

	return bResult;
}

INT CReportCtrl::GetItemIndex(HTREEITEM hItem)
{
	ASSERT(m_dwStyle&RVS_TREEVIEW);
	if (hItem)
		return ((LPTREEITEM)hItem)->iItem;
	return 0;
}

HTREEITEM CReportCtrl::GetItemHandle(INT iItem)
{
	ASSERT(m_dwStyle&RVS_TREEVIEW);
	return iItem>=RVI_FIRST ? (HTREEITEM)m_arrayItems[iItem].lptiItem:NULL;
}

void CReportCtrl::SetTreeSubItem(HTREEITEM hItem, INT iSubItem)
{
	((LPTREEITEM)hItem)->iSubItem = iSubItem;
}

HTREEITEM CReportCtrl::GetNextItem(HTREEITEM hItem, UINT nCode)
{
	LPTREEITEM lpti = (LPTREEITEM)hItem;

	if( hItem == RVTI_ROOT )
		lpti = &m_tiRoot;

	switch(nCode)
	{
	case RVGN_ROOT:
		while(lpti->lptiParent != &m_tiRoot)
			lpti = lpti->lptiParent;
		break;
		
	case RVGN_NEXT:
		lpti = lpti->lptiSibling;
		break;

	case RVGN_PREVIOUS:
		if(lpti->lptiParent->lptiChildren != lpti)
		{
			LPTREEITEM lptiSibling = lpti->lptiParent->lptiChildren;
			ASSERT(lptiSibling != NULL);

			while(lptiSibling->lptiSibling != lpti)
			{
				lptiSibling = lptiSibling->lptiSibling;
				ASSERT(lptiSibling != NULL);
			}

			lpti = lptiSibling;
		}
		else
			lpti = NULL;
		break;

	case RVGN_PARENT:
		lpti = lpti->lptiParent != &m_tiRoot ? lpti->lptiParent:NULL;
		break;

	case RVGN_CHILD:
		lpti = lpti->lptiChildren;
		break;

	case RVGN_FOCUSED:
		{
			INT iFocusItem = GetItemFromRow(m_iFocusRow);
			if(iFocusItem >= RVI_FIRST)
				lpti = m_arrayItems[iFocusItem].lptiItem;
			else
				lpti = NULL;
		}
		break;

	default:
		lpti = NULL;
		break;
	}

	return (HTREEITEM)lpti;
}

BOOL CReportCtrl::GetItem(LPRVITEM lprvi)
{
	ASSERT(lprvi->iItem > RVI_INVALID);
	ASSERT(lprvi->iItem < GetItemCount());					// Specify item
	ASSERT(lprvi->iSubItem < m_arraySubItems.GetSize());	// Specify subitem

	if(
		(lprvi->iItem <= RVI_INVALID) ||
		(lprvi->iItem >= GetItemCount()) ||
		(lprvi->iSubItem >= m_arraySubItems.GetSize())
	)
		return FALSE;

	UINT nMask = lprvi->nMask;
	INT iTextMax = lprvi->iTextMax;
	ITEM& item = GetItemStruct(lprvi->iItem, lprvi->iSubItem, nMask);

	lprvi->nMask = 0;
	lprvi->iBkColor = item.iBkColor;
	if(lprvi->iBkColor >= 0)
		lprvi->nMask |= RVIM_BKCOLOR;

	lprvi->nPreview = item.nPreview;
	if(lprvi->nPreview > 0)
		lprvi->nMask |= RVIM_PREVIEW;

	lprvi->iIndent = item.iIndent;
	if(lprvi->iIndent >= 0)
		lprvi->nMask |= RVIM_INDENT;

	lprvi->lpszText = nMask&RVIM_TEXT ? lprvi->lpszText:NULL;
	item.rdData.GetSubItem(lprvi->iSubItem, &lprvi->iImage, &lprvi->iOverlay, &lprvi->iCheck, &lprvi->iTextColor, lprvi->lpszText, &iTextMax);
	if(lprvi->lpszText && iTextMax >= 0)
		lprvi->nMask |= RVIM_TEXT;

	if(lprvi->iImage >= 0)
		lprvi->nMask |= RVIM_IMAGE;
	if(lprvi->iOverlay >= 0)
		lprvi->nMask |= RVIM_OVERLAY;
	if(lprvi->iCheck >= 0)
		lprvi->nMask |= RVIM_CHECK;
	if(lprvi->iTextColor >= 0)
		lprvi->nMask |= RVIM_TEXTCOLOR;

	lprvi->nMask |= RVIM_STATE|RVIM_LPARAM|RVIM_PARAM64;
	lprvi->nState = item.nState;
	lprvi->lParam = item.lParam;
	lprvi->Param64 = item.Param64;

	return TRUE;
}

BOOL CReportCtrl::SetItem(LPRVITEM lprvi)
{
	ASSERT(!(lprvi->iItem != RVI_EDIT && m_dwStyle&RVS_OWNERDATA)); // Not supported when using this style
	if(m_dwStyle&RVS_OWNERDATA)
		return FALSE;

	m_wndTip.Hide();

	TCHAR szText[REPORTCTRL_MAX_TEXT];

	RVITEM rvi;
	rvi.iItem = lprvi->iItem;
	rvi.iSubItem = lprvi->iSubItem;
	rvi.lpszText = szText;
	rvi.iTextMax = REPORTCTRL_MAX_TEXT;
	rvi.nMask = RVIM_TEXT|RVIM_STATE;
	VERIFY(GetItem(&rvi));

	if(lprvi->nMask&RVIM_TEXT)
	{
		if(lprvi->lpszText != NULL)
		{
			_tcsncpy_s(rvi.lpszText, REPORTCTRL_MAX_TEXT, lprvi->lpszText, REPORTCTRL_MAX_TEXT-1);
			rvi.lpszText[REPORTCTRL_MAX_TEXT-1] = 0;
		}
		else
			rvi.lpszText = NULL;
	}
	else if(!(rvi.nMask&RVIM_TEXT))
		rvi.lpszText = NULL;

	if(lprvi->nMask&RVIM_TEXTCOLOR)
		rvi.iTextColor = lprvi->iTextColor;

	if(m_pImageList && (lprvi->nMask&RVIM_IMAGE))
	{
		ASSERT(m_pImageList != NULL && lprvi->iImage < m_pImageList->GetImageCount());
		rvi.iImage = lprvi->iImage;
	}

	if(m_pImageList && (lprvi->nMask&RVIM_OVERLAY))
	{
		ASSERT(m_pImageList != NULL && lprvi->iOverlay < m_pImageList->GetImageCount());
		rvi.iOverlay = lprvi->iOverlay;
	}

	if(lprvi->nMask&RVIM_CHECK)
		rvi.iCheck = lprvi->iCheck;

	if(lprvi->nMask&RVIM_BKCOLOR)
		rvi.iBkColor = lprvi->iBkColor;

	ASSERT(!(lprvi->iItem == RVI_EDIT && lprvi->nMask&RVIM_PREVIEW)); // Preview not supported on edit row
	if(lprvi->nMask&RVIM_PREVIEW)
		rvi.nPreview = lprvi->nPreview;

	if(lprvi->nMask&RVIM_INDENT)
		rvi.iIndent = lprvi->iIndent;

	// Note: focus, selection and tree-GUI elements cannot be changed through this function
	if(lprvi->nMask&RVIM_STATE)
	{
		rvi.nState &= RVIS_FOCUSED|RVIS_SELECTED|RVIS_TREEMASK;
		rvi.nState |= lprvi->nState&~(RVIS_FOCUSED|RVIS_SELECTED|RVIS_TREEMASK);
	}

	if(lprvi->nMask&RVIM_LPARAM)
		rvi.lParam = lprvi->lParam;

	if(lprvi->nMask&RVIM_PARAM64)
		rvi.Param64 = lprvi->Param64;

	ITEM& item = GetItemStruct(rvi.iItem, rvi.iSubItem);

	VERIFY(item.rdData.InsertSubItem(rvi.iSubItem, rvi.iImage, rvi.iOverlay, rvi.iCheck, rvi.iTextColor, rvi.lpszText));
	VERIFY(item.rdData.DeleteSubItem(rvi.iSubItem+1));

	item.iBkColor = rvi.iBkColor;
	item.nPreview = rvi.nPreview;
	item.iIndent = rvi.iIndent;
	item.nState = rvi.nState;
	item.lParam = rvi.lParam;
	item.Param64 = rvi.Param64;
	SetItemStruct(rvi.iItem, item);

	ScrollWindow(SB_VERT, GetScrollPos32(SB_VERT));

	if(m_bRedrawFlag == TRUE)
		RedrawItems(rvi.iItem);
	return TRUE;
}

INT CReportCtrl::GetItemText(INT iItem, INT iSubItem, LPTSTR lpszText, INT iLen)
{
	RVITEM rvi;
	rvi.nMask = RVIM_TEXT;
	rvi.iItem = iItem;
	rvi.iSubItem = iSubItem;
	rvi.lpszText = lpszText;
	rvi.iTextMax = iLen;
	return (GetItem(&rvi)&&(rvi.lpszText)) ? (INT)_tcslen(rvi.lpszText):0;
}

CString CReportCtrl::GetItemText(INT iItem, INT iSubItem)
{
	CString str;
	TCHAR szText[REPORTCTRL_MAX_TEXT];
	ZeroMemory(szText, sizeof(szText));

	if(GetItemText(iItem, iSubItem, szText, REPORTCTRL_MAX_TEXT))
		str = szText;

	return str;
}

BOOL CReportCtrl::SetItemText(INT iItem, INT iSubItem, LPCTSTR lpszText)
{
	RVITEM rvi;
	rvi.nMask = RVIM_TEXT;
	rvi.iItem = iItem;
	rvi.iSubItem = iSubItem;
	rvi.lpszText = (LPTSTR)lpszText;
	return SetItem(&rvi);
}

INT CReportCtrl::GetItemImage(INT iItem, INT iSubItem)
{
	RVITEM rvi;
	rvi.nMask = RVIM_IMAGE;
	rvi.iItem = iItem;
	rvi.iSubItem = iSubItem;
	return GetItem(&rvi) ? rvi.iImage:-1;
}

BOOL CReportCtrl::SetItemImage(INT iItem, INT iSubItem, INT iImage)
{
	RVITEM rvi;
	rvi.nMask = RVIM_IMAGE;
	rvi.iItem = iItem;
	rvi.iSubItem = iSubItem;
	rvi.iImage = iImage;
	return SetItem(&rvi);
}

INT CReportCtrl::GetItemCheck(INT iItem, INT iSubItem)
{
	RVITEM rvi;
	rvi.nMask = RVIM_CHECK;
	rvi.iItem = iItem;
	rvi.iSubItem = iSubItem;
	return GetItem(&rvi) ? rvi.iCheck:-1;
}

BOOL CReportCtrl::SetItemCheck(INT iItem, INT iSubItem, INT iCheck)
{
	RVITEM rvi;
	rvi.nMask = RVIM_CHECK;
	rvi.iItem = iItem;
	rvi.iSubItem = iSubItem;
	rvi.iCheck = iCheck;
	return SetItem(&rvi);
}

DWORD_PTR CReportCtrl::GetItemData(INT iItem)
{
	RVITEM rvi;
	rvi.nMask = RVIM_LPARAM;
	rvi.iItem = iItem;
	return GetItem(&rvi) ? rvi.lParam:0;
}

BOOL CReportCtrl::SetItemData(INT iItem, DWORD_PTR dwData)
{
	RVITEM rvi;
	rvi.nMask = RVIM_LPARAM;
	rvi.iItem = iItem;
	rvi.lParam = dwData;
	return SetItem(&rvi);
}

BOOL CReportCtrl::GetItemRect(INT iItem, INT iSubItem, LPRECT lpRect, UINT nCode)
{
	INT iRow = GetRowFromItem(iItem);
	INT iColumn = GetColumnFromSubItem(iSubItem);

	if( nCode == RVIR_TEXTNOCOLUMNS )
		iColumn = 0;

	if(!GetRowRect(iRow, iColumn, lpRect, nCode))
		return FALSE;

	RVITEM rvi;
	rvi.iItem = iItem;
	rvi.iSubItem = iSubItem;
	GetItem(&rvi);

	if(nCode != RVIR_BOUNDS && iColumn == 0 && rvi.nMask&RVIM_INDENT && rvi.iIndent >= 0)
	{
		lpRect->left += rvi.iIndent*m_iDefaultHeight;

		if(lpRect->left > lpRect->right)
			return FALSE;
	}

	switch(nCode)
	{
	case RVIR_BOUNDS:
		return TRUE;

	case RVIR_IMAGE:
		if(!(rvi.nMask&RVIM_IMAGE))
			return FALSE;

		lpRect->left += m_iSpacing;
		if(lpRect->left + m_sizeImage.cx > lpRect->right - m_iSpacing)
			return FALSE;

		lpRect->right = lpRect->left + m_sizeImage.cx;
		return TRUE;

	case RVIR_CHECK:
		if(!(rvi.nMask&RVIM_CHECK))
			return FALSE;

		lpRect->left += m_iSpacing + (rvi.nMask&RVIM_IMAGE ? m_sizeImage.cx+m_iSpacing:0);
		if(lpRect->left + m_sizeCheck.cx > lpRect->right - m_iSpacing)
			return FALSE;

		lpRect->right = lpRect->left + m_sizeCheck.cx;
		return TRUE;

	case RVIR_TEXT:
	case RVIR_TEXTNOCOLUMNS:
		lpRect->left += m_iSpacing +
			(rvi.nMask&RVIM_IMAGE ? m_sizeImage.cx+m_iSpacing:0) +
			(rvi.nMask&RVIM_CHECK ? m_sizeCheck.cx+m_iSpacing:0);

		if(lpRect->left > lpRect->right - m_iSpacing)
			return FALSE;

		lpRect->right -= m_iSpacing;
		return TRUE;

	default:
		return FALSE;
	}
}

BOOL CReportCtrl::MeasureItem(INT iItem, INT iSubItem, LPRECT lpRect, BOOL bTextOnly)
{
	lpRect->top = 0;
	lpRect->bottom = m_iDefaultHeight;
	lpRect->left = 0;
	lpRect->right = m_iDefaultWidth;

	if(iSubItem != -1)
	{
		TCHAR szText[REPORTCTRL_MAX_TEXT];

		RVITEM rvi;
		rvi.nMask = RVIM_TEXT;
		rvi.iItem = iItem;
		rvi.iSubItem = iSubItem;
		rvi.lpszText = szText;
		rvi.iTextMax = REPORTCTRL_MAX_TEXT;
		GetItem(&rvi);

		lpRect->right = m_iSpacing;
		if(bTextOnly == FALSE)
		{
			lpRect->right +=
				(rvi.nMask&RVIM_IMAGE ? m_sizeImage.cx+m_iSpacing:0) +
				(rvi.nMask&RVIM_CHECK ? m_sizeCheck.cx+m_iSpacing:0);
		}
		if(rvi.nMask&RVIM_TEXT)
		{
			CClientDC dc(this);
			CFont *pFontDC = dc.SelectObject(rvi.nState&RVIS_BOLD ? &m_fontBold:&m_font);
			lpRect->right += dc.GetTextExtent(rvi.lpszText, (int)_tcslen(rvi.lpszText)).cx + m_iSpacing;
			dc.SelectObject(pFontDC);
		}
	}
	return TRUE;
}

INT CReportCtrl::GetItemHeight()
{
	return m_iDefaultHeight;
}

void CReportCtrl::SetItemHeight(INT iHeight)
{
	m_iDefaultHeight = iHeight;

	CRect rect;
	GetClientRect(rect);
	Layout(rect.Width(), rect.Height());
}

CString CReportCtrl::GetItemString(INT iItem, TCHAR cSeparator)
{
	CString str;
	TCHAR szText[REPORTCTRL_MAX_TEXT+1];
	INT iColumn, iColumns = (INT)m_arrayColumns.GetSize();

	for(iColumn=0;iColumn<iColumns;iColumn++)
	{
		HDITEM hdi;
		hdi.mask = HDI_WIDTH|HDI_LPARAM;
		m_wndHeader.GetItem(m_arrayColumns[iColumn], &hdi);

		RVITEM rvi;
		rvi.iItem = iItem;
		rvi.iSubItem = (INT)hdi.lParam;

		rvi.nMask = RVIM_TEXT;
		rvi.lpszText = szText;
		rvi.iTextMax = REPORTCTRL_MAX_TEXT;

		VERIFY(GetItem(&rvi));

		if(rvi.nMask&RVIM_TEXT)
		{
			LPTSTR lpsz = rvi.lpszText;
			do
			{
				lpsz = _tcschr( lpsz, cSeparator );
				if( lpsz != NULL )
					*lpsz = _T(' ');
			} while( lpsz != NULL );

		}
		else if(rvi.nMask&RVIM_IMAGE)
		{
			_stprintf_s(szText, REPORTCTRL_MAX_TEXT, _T("%d"), rvi.iImage );	
		}
		else if(rvi.nMask&RVIM_CHECK)
		{
			_stprintf_s(szText, REPORTCTRL_MAX_TEXT, _T("%d"), rvi.iCheck );
		}
		else
			szText[0] = 0;

		if(m_dwStyle&RVS_TREEMASK && iColumn == 0 && rvi.nMask&RVIM_INDENT)
		{
			for(INT i=1;i<rvi.iIndent;i++)
				str += g_strCBIndent;

			HTREEITEM hItem = GetItemHandle(iItem);
			ASSERT(hItem != NULL);

			if(HasChildren(hItem))
				str += GetNextItem(hItem, RVGN_CHILD) != NULL ? _T('-'):_T('+');
			else
				str += _T(' ');
		}

		str += rvi.lpszText;

		str += iColumn < iColumns-1 ? cSeparator:_T('\n');
	}

	return str;
}

INT CReportCtrl::GetVisibleCount(BOOL bUnobstructed)
{
	return GetVisibleRows(bUnobstructed);
}

INT CReportCtrl::GetTopIndex()
{
	return GetScrollPos32(SB_VERT);
}

BOOL CReportCtrl::IsItemVisible(INT iItem, BOOL bUnobstructed)
{
	INT iFirst, iLast, iRow;

	iRow   = GetRowFromItem(iItem);
	iFirst = GetScrollPos32(SB_VERT);
	GetVisibleRows(bUnobstructed, &iFirst, &iLast);

	return (iRow>=iFirst && iRow<=iLast) ? TRUE:FALSE;
}

void CReportCtrl::PageUp() 
{
	INT iPos = GetTopIndex() - GetVisibleCount();
	ScrollWindow(SB_VERT, iPos<0 ? 0:iPos);
}

void CReportCtrl::PageDown() 
{
	INT iPos = GetTopIndex() + GetVisibleCount();
	ScrollWindow(SB_VERT, iPos);
}

INT CReportCtrl::GetItemCount()
{
	return m_dwStyle&RVS_OWNERDATA ? m_iVirtualHeight:(INT)m_arrayItems.GetSize();
}

INT CReportCtrl::GetItemCount(HTREEITEM hParent, BOOL bRecurse)
{
	INT iCount = 0;

	LPTREEITEM lptiSibling =
		hParent == RVTI_ROOT
		?
		m_tiRoot.lptiChildren:((LPTREEITEM)hParent)->lptiChildren;

	while(lptiSibling != NULL)
	{
		if(bRecurse == TRUE && lptiSibling->lptiChildren != NULL)
			iCount += GetItemCount((HTREEITEM)lptiSibling, bRecurse);

		lptiSibling = lptiSibling->lptiSibling;
		iCount ++;
	}

	return iCount;
}

void CReportCtrl::SetItemCount(INT iCount)
{
	ASSERT(m_dwStyle&RVS_OWNERDATA); // Only supported when using RVS_OWNERDATA style
	ASSERT(iCount >= 0);

	INT i;

	m_iVirtualHeight = iCount;
	m_iFocusRow = m_iFocusRow>=m_iVirtualHeight ? m_iVirtualHeight-1:m_iFocusRow;

	CList<INT, INT> list;
	while( !m_listSelection.IsEmpty() )
	{
		i = m_listSelection.GetHead();
		if(i < iCount)
			list.AddTail(i);

		m_listSelection.RemoveHead();
	}
	m_listSelection.AddTail(&list);

	for( i=0; i<REPORTCTRL_MAX_CACHE; i++ )
	{
		if( m_aciCache[i].iItem >= iCount )
			m_aciCache[i].iItem = RVI_INVALID;
	}

	RedrawRows(RVI_EDIT, RVI_LAST, TRUE);

	INT iPos = GetScrollPos32(SB_VERT);
	ScrollWindow(SB_VERT, iPos>iCount ? iCount : iPos);
}

INT CReportCtrl::GetFirstSelectedItem()
{
	return GetNextSelectedItem(RVI_INVALID);
}

INT CReportCtrl::GetNextSelectedItem(INT iItem)
{
	RVITEM rvi;
	rvi.nMask = RVIM_STATE;

	INT iItems = GetItemCount();

	for(rvi.iItem=iItem+1;rvi.iItem<iItems;rvi.iItem++)
		if(GetItem(&rvi) && rvi.nState&RVIS_SELECTED)
			return rvi.iItem;

	return RVI_INVALID;
}

INT CReportCtrl::GetSelectedCount()
{
	INT iItem = GetFirstSelectedItem(), iItems = 0;

	while(iItem != RVI_INVALID)
	{
		iItem = GetNextSelectedItem(iItem);
		iItems++;
	}

	return iItems;
}

INT CReportCtrl::GetSelectedItems(LPINT lpiItems, INT iMax)
{
	INT iItem = GetFirstSelectedItem(), iItems = 0;

	while(iItem != RVI_INVALID)
	{
		if(lpiItems != NULL && iItems < iMax)
			lpiItems[iItems] = iItem;

		iItem = GetNextSelectedItem(iItem);
		iItems++;
	}

	return iItems;
}

void CReportCtrl::ClearSelection()
{
	POSITION pos = m_listSelection.GetHeadPosition();
	while(pos != NULL)
	{
		INT iItem = m_listSelection.GetAt(pos);
		INT iRow = GetRowFromItem( iItem );

		SetState(iRow, GetState(iRow) & ~RVIS_SELECTED, RVIS_SELECTED);
		RedrawRows( iRow );

		m_listSelection.GetNext(pos);
	}
	m_listSelection.RemoveAll();
}

void CReportCtrl::InvertSelection()
{
	INT iRows = (INT)m_arrayRows.GetSize();
	INT iFocusRow = m_iFocusRow;
	SelectRows(RVI_FIRST, iRows-1, TRUE, TRUE, TRUE, FALSE);
	SelectRows(iFocusRow, iFocusRow, FALSE, FALSE, FALSE, FALSE);
}

void CReportCtrl::SelectAll()
{
	if( m_arrayRows.GetSize() )
	{
		ClearSelection();
		InvertSelection();
	}
}

void CReportCtrl::SetSelection(INT iItem, BOOL bKeepSelection)
{
	ASSERT(iItem > RVI_INVALID && iItem < GetItemCount());

	INT iRow = GetRowFromItem(iItem);
	SelectRows(iRow, iRow, TRUE, bKeepSelection, FALSE, FALSE);
}

void CReportCtrl::SetSelection(LPINT lpiItems, INT iCount, BOOL bKeepSelection)
{
	INT i, iRow;

	for(i=0;i<iCount;i++)
	{
		ASSERT(lpiItems[i] > RVI_INVALID && lpiItems[i] < GetItemCount());

		iRow = GetRowFromItem(lpiItems[i]);
		SelectRows(iRow, iRow, TRUE, bKeepSelection|(i != 0), FALSE, FALSE);
	}
}

void CReportCtrl::SetSelection(INT iSubItem, CString& strPattern, BOOL bKeepSelection)
{
	INT iRows = (INT)m_arrayRows.GetSize();

	if(bKeepSelection == FALSE)
		ClearSelection();

	for(INT iRow=0; iRow<iRows; iRow++)
	{
		INT iItem = GetItemFromRow(iRow);
		if(MatchString(GetItemText(iItem, iSubItem), strPattern))
			SelectRows(iRow, iRow, TRUE, TRUE, FALSE, FALSE);
	}
}

void CReportCtrl::SetSelectionLessEq(INT iSubItem, INT iMatch, BOOL bKeepSelection)
{
	RVITEM rvi;
	INT iRows = (INT)m_arrayRows.GetSize();

	if(bKeepSelection == FALSE)
		ClearSelection();

	rvi.iSubItem = iSubItem;
	rvi.nMask = RVIM_LPARAM;

	for(INT iRow=0; iRow<iRows; iRow++)
	{
		rvi.iItem = GetItemFromRow(iRow);
		if(GetItem(&rvi))
		{
			if(rvi.lParam <= iMatch)
				SelectRows(iRow, iRow, TRUE, TRUE, FALSE, FALSE);
		}
	}
}

void CReportCtrl::SetSelection(HTREEITEM hItem, BOOL bKeepSelection)
{
	SetSelection(((LPTREEITEM)hItem)->iItem, bKeepSelection);
}

void CReportCtrl::SetSelection(HTREEITEM* lphItems, INT iCount, BOOL bKeepSelection)
{
	INT i, iItem, iRow;

	for(i=0;i<iCount;i++)
	{
		iItem = ((LPTREEITEM)lphItems[i])->iItem;
		ASSERT(iItem > RVI_INVALID && iItem < GetItemCount());

		iRow = GetRowFromItem(iItem);
		SelectRows(iRow, iRow, TRUE, bKeepSelection|(i != 0), FALSE, FALSE);
	}
}

INT CReportCtrl::GetItemRow(INT iItem)
{
	ASSERT(!(m_dwStyle&RVS_OWNERDATA));
	return GetRowFromItem(iItem);
}

CArray<INT, INT>* CReportCtrl::GetItemRowArray()
{
	ASSERT(!(m_dwStyle&RVS_OWNERDATA));
	return &m_arrayRows;
}

BOOL CReportCtrl::SetItemRowArray(CArray<INT, INT>& arrayRows)
{
	if(m_dwStyle&RVS_OWNERDATA)
		return FALSE;

	ASSERT(m_arrayRows.GetSize() == arrayRows.GetSize());	// Must match
	if(m_arrayRows.GetSize() != arrayRows.GetSize())
		return FALSE;

	INT iFocusItem = GetItemFromRow(m_iFocusRow);

	m_arrayRows.Copy(arrayRows);
	m_bUpdateItemMap = TRUE;

	m_iFocusRow = GetRowFromItem(iFocusItem);

	Invalidate();
	return TRUE;
}

BOOL CReportCtrl::MoveUp(INT iRow)
{
	if(m_dwStyle&RVS_OWNERDATA)
		return FALSE;

	if(iRow <= RVI_FIRST)
		return FALSE;

	ASSERT(iRow < m_arrayRows.GetSize());

	INT iTemp = m_arrayRows[iRow-1];
	m_arrayRows[iRow-1] = m_arrayRows[iRow];
	m_arrayRows[iRow] = iTemp;

	if(m_iFocusRow == iRow)
		m_iFocusRow--;

	Invalidate();

	m_bUpdateItemMap = TRUE;
	return TRUE;
}

BOOL CReportCtrl::MoveDown(INT iRow)
{
	if(m_dwStyle&RVS_OWNERDATA)
		return FALSE;

	if(iRow >= m_arrayRows.GetSize()-1)
		return FALSE;

	ASSERT(iRow >= RVI_FIRST);

	INT iTemp = m_arrayRows[iRow+1];
	m_arrayRows[iRow+1] = m_arrayRows[iRow];
	m_arrayRows[iRow] = iTemp;

	if(m_iFocusRow == iRow)
		m_iFocusRow++;

	Invalidate();

	m_bUpdateItemMap = TRUE;
	return TRUE;
}

BOOL CReportCtrl::SetImageList(CImageList* pImageList, CImageList* pCheckList)
{
	BOOL bResult = TRUE;
	IMAGEINFO info;

	m_pImageList = pImageList;
	m_wndHeader.SetImageList(pImageList);

	if(pImageList != NULL)
	{
		if(pImageList->GetImageInfo(0, &info))
		{
			m_sizeImage.cx = info.rcImage.right - info.rcImage.left;
			m_sizeImage.cy = info.rcImage.bottom - info.rcImage.top;

			m_iDefaultHeight = m_sizeImage.cy+1>m_iDefaultHeight ? m_sizeImage.cy+1:m_iDefaultHeight;
		}
		else
			bResult = FALSE;
	}

	m_pCheckList = pCheckList;

	if(pCheckList != NULL)
	{
		if(pCheckList->GetImageInfo(0, &info))
		{
			m_sizeCheck.cx = info.rcImage.right - info.rcImage.left;
			m_sizeCheck.cy = info.rcImage.bottom - info.rcImage.top;

			m_iDefaultHeight = m_sizeCheck.cy+1>m_iDefaultHeight ? m_sizeCheck.cy+1:m_iDefaultHeight;
		}
		else
			bResult = FALSE;
	}

	Invalidate();
	return bResult;
}

CImageList* CReportCtrl::GetImageList(void)
{
	return m_pImageList;
}

BOOL CReportCtrl::SetBkImage(UINT nIDResource)
{
	return SetBkImage( (LPCTSTR)((INT_PTR)nIDResource) );
}

BOOL CReportCtrl::SetBkImage(LPCTSTR lpszResourceName)
{
	if(m_bitmap.m_hObject != NULL)
		m_bitmap.DeleteObject();

	HBITMAP hBitmap = (HBITMAP)::LoadImage( AfxGetResourceHandle(),
			lpszResourceName, IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION );

	Invalidate();

	if(hBitmap == NULL)
		return FALSE;

	m_bitmap.Attach(hBitmap);

	BITMAP bitmap;
	m_bitmap.GetBitmap(&bitmap);
	m_sizeBitmap.cx = bitmap.bmWidth;
	m_sizeBitmap.cy = bitmap.bmHeight;

	BOOL bResult = CreatePalette();

	Invalidate();
	return bResult;
}

COLORREF CReportCtrl::GetColor(INT iIndex)
{
	return m_arrayColors[iIndex];
}

BOOL CReportCtrl::SetColor(INT iIndex, COLORREF crColor)
{
	if(iIndex<0  || iIndex>=m_arrayColors.GetSize())
		return FALSE;

	m_arrayColors.SetAt(iIndex, crColor);

	BOOL bResult = CreatePalette();

	Invalidate();
	return bResult;
}

BOOL CReportCtrl::HasFocus()
{
	return m_bFocus;
}

INT CReportCtrl::GetCurrentFocus(LPINT lpiColumn)
{
	if(lpiColumn != NULL)
		*lpiColumn = m_iFocusColumn;

	return m_iFocusRow;
}

BOOL CReportCtrl::AdjustFocus(INT iRow, INT iColumn)
{
	INT iSubItem = GetSubItemFromColumn(iColumn);
	if(iSubItem>=0 && m_arraySubItems[iSubItem].nFormat&RVCF_SUBITEM_NOFOCUS)
		return FALSE;

	if(
		iRow < RVI_INVALID || iRow > m_iVirtualHeight-1 ||
		iColumn < -1 || iColumn > m_arrayColumns.GetSize()-1
	)
		return FALSE;

	if(m_iFocusRow > RVI_INVALID && iRow != m_iFocusRow)
	{
		SetState(m_iFocusRow, 0, RVIS_FOCUSED);
		RedrawRows(m_iFocusRow);
	}

	m_iFocusColumn = iColumn;
	m_iFocusRow = iRow;
	if(iRow > RVI_INVALID)
	{
		SetState(iRow, RVIS_FOCUSED, RVIS_FOCUSED);
		RedrawRows(m_iFocusRow);
	}
	return TRUE;
}

BOOL CReportCtrl::HasChildren(HTREEITEM hItem)
{
	ASSERT(m_dwStyle&RVS_TREEMASK);
	return ((LPTREEITEM)hItem)->lptiChildren != NULL ? TRUE:FALSE;
}

BOOL CReportCtrl::IsChild(HTREEITEM hParent, HTREEITEM hItem)
{
	ASSERT(m_dwStyle&RVS_TREEMASK);
	return ((LPTREEITEM)hItem)->lptiParent == (LPTREEITEM)hParent ? TRUE:FALSE;
}

BOOL CReportCtrl::IsDescendent(HTREEITEM hParent, HTREEITEM hItem)
{
	ASSERT(m_dwStyle&RVS_TREEMASK);
	LPTREEITEM lptiAncestor = ((LPTREEITEM)hItem)->lptiParent;

	while(lptiAncestor != NULL)
	{
		if(lptiAncestor == (LPTREEITEM)hParent)
			return TRUE;

		lptiAncestor = lptiAncestor->lptiParent;
	}

	return FALSE;
}

CReportHeaderCtrl* CReportCtrl::GetHeaderCtrl()
{
	return m_wndHeader.GetSafeHwnd() != NULL ? &m_wndHeader:NULL;
}

BOOL CReportCtrl::SetReportSubItemListCtrl(CReportSubItemListCtrl* lprsilc)
{
	if(m_lprsilc != NULL)
	{
		m_wndHeader.ModifyProperty(FH_PROPERTY_DROPTARGET, NULL);
		m_lprsilc->SetReportCtrl(NULL);
	}

	m_lprsilc = lprsilc;
	if(m_lprsilc != NULL)
	{
		m_wndHeader.ModifyProperty(FH_PROPERTY_DROPTARGET, (LPARAM)m_lprsilc->m_hWnd);
		m_lprsilc->SetReportCtrl(this);
	}

	return TRUE;
}

CReportSubItemListCtrl* CReportCtrl::GetReportSubItemListCtrl()
{
	return m_lprsilc;
}

BOOL CReportCtrl::SetSortCallback(LPFNRVCOMPARE lpfnrvc, LPARAM lParam)
{
	m_lpfnrvc = lpfnrvc;
	m_lParamCompare = lParam;
	return TRUE;
}

LPFNRVCOMPARE CReportCtrl::GetSortCallback()
{
	return m_lpfnrvc;
}

BOOL CReportCtrl::GetSortSettings(INT *lpiSubItem, BOOL *lpbAscending)
{
	INT iColumn = m_wndHeader.GetSortColumn(lpbAscending);
	if(iColumn >= 0)
	{
		*lpiSubItem = GetSubItemFromColumn(iColumn);
		return TRUE;
	}

	*lpiSubItem = -1;
	return FALSE;
}

BOOL CReportCtrl::WriteProfile(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	CString str, strProfile;

	UnindentFirstColumn();

	if(m_bColumnsReordered)
		ReorderColumns();

	INT i;
	INT iSubItems = (INT)m_arraySubItems.GetSize();
	INT iColumns = (INT)m_arrayColumns.GetSize();

	strProfile.Format(_T("(%d,%d)"), iSubItems, iColumns);

	for(i=0;i<iSubItems;i++)
	{
		str.Format(_T(" %d"), m_arraySubItems[i].iWidth);
		strProfile += str;
	}

	for(i=0;i<iColumns;i++)
	{
		HDITEM hdi;
		hdi.mask = HDI_LPARAM;
		m_wndHeader.GetItem(m_arrayColumns[i], &hdi);

		str.Format(_T(" %d"), hdi.lParam);
		strProfile += str;
	}

	IndentFirstColumn();

	CWinApp* pApp = AfxGetApp();
	return pApp->WriteProfileString(lpszSection, lpszEntry, strProfile);
}

BOOL CReportCtrl::GetProfile(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
	CString str, strProfile;

	CWinApp* pApp = AfxGetApp();
	strProfile = pApp->GetProfileString(lpszSection, lpszEntry);
	if(strProfile.IsEmpty())
		return FALSE;

	UnindentFirstColumn();

	LPTSTR lpsz = strProfile.GetBuffer(0);

	INT i;
	INT iSubItems;
	INT iColumns;

	iSubItems = _tcstol(++lpsz, &lpsz, 10);
	iColumns = _tcstol(++lpsz, &lpsz, 10);

	if(iSubItems != m_arraySubItems.GetSize())
		return FALSE;

	if(iColumns == 0)
		return FALSE;

	for(i=0;i<iSubItems;i++)
		m_arraySubItems[i].iWidth = _tcstol(++lpsz, &lpsz, 10);

	for(i=0;i<iColumns;i++)
		ActivateSubItem(_tcstol(++lpsz, &lpsz, 10), i);

	ReorderColumns();

	//IndentFirstColumn();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CReportCtrl operations

BOOL CReportCtrl::ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	DWORD dwStyle = CWnd::GetStyle();

	ASSERT(!(dwRemove&(RVS_OWNERDATA|RVS_TREEVIEW)));	// Can't dynamically remove this styles
	ASSERT(!(dwAdd&(RVS_OWNERDATA|RVS_TREEVIEW)));		// Can't dynamically add this styles

	if(m_hEditWnd != NULL)
		EndEdit();

	if(CWnd::ModifyStyle(dwRemove, dwAdd, nFlags))
	{
		if(!(dwStyle&RVS_SHOWHGRID) && dwAdd&RVS_SHOWHGRID)
			m_iDefaultHeight++;

		if(dwStyle&RVS_SHOWHGRID && dwRemove&RVS_SHOWHGRID)
			m_iDefaultHeight--;

		if(dwRemove&RVS_FOCUSSUBITEMS)
			m_iFocusColumn = -1;

		m_dwStyle = CWnd::GetStyle();

		CRect rect;
		GetClientRect(rect);
		Layout(rect.Width(), rect.Height());

		return TRUE;
	}

	return FALSE;
}

INT CReportCtrl::DefineSubItem(INT iSubItem, LPRVSUBITEM lprvs, BOOL bUpdateList)
{
	INT i;
#ifdef DEBUG
	INT iSubItems = (INT)m_arraySubItems.GetSize();

	ASSERT(iSubItem <= iSubItems);
	ASSERT(lprvs->lpszText != NULL); // Must supply (descriptive) text for subitem selector
	ASSERT(_tcslen(lprvs->lpszText) < FLATHEADER_TEXT_MAX);
#endif

	try
	{
		SUBITEM subitem;

		subitem.nFormat = lprvs->nFormat;
		subitem.iWidth = lprvs->iWidth<0 ? m_iDefaultWidth:lprvs->iWidth;
		subitem.iMinWidth = lprvs->iMinWidth;
		subitem.iMaxWidth = lprvs->iMaxWidth;
		subitem.iImage = lprvs->nFormat&RVCF_IMAGE ? lprvs->iImage:0;
		subitem.strText = lprvs->lpszText;

		m_arraySubItems.InsertAt(iSubItem, subitem);

		HDITEM hdi;
		hdi.mask = HDI_LPARAM;

		INT iHeaderItems = (INT)m_arrayColumns.GetSize();
		for(i=0;i<iHeaderItems;i++)
			if(m_wndHeader.GetItem(m_arrayColumns[i], &hdi))
				if(hdi.lParam >= iSubItem)
				{
					hdi.lParam++;
					m_wndHeader.SetItem(m_arrayColumns[i], &hdi);
				}

		VERIFY(m_itemEdit.rdData.InsertSubItem(iSubItem, -1, -1, -1, -1, NULL));

		if(!(m_dwStyle&RVS_OWNERDATA))
		{
			INT iItems = (INT)m_arrayItems.GetSize();
			for(i=0;i<iItems;i++)
				VERIFY(m_arrayItems[i].rdData.InsertSubItem(iSubItem, -1, -1, -1, -1, NULL));
		}

		if(bUpdateList && m_lprsilc != NULL)
			m_lprsilc->UpdateList();

		return iSubItem;
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		return -1;
	}
}

BOOL CReportCtrl::RedefineSubItem(INT iSubItem, LPRVSUBITEM lprvs, BOOL bUpdateList)
{
#ifdef DEBUG
	INT iSubItems = (INT)m_arraySubItems.GetSize();

	ASSERT(iSubItem <= iSubItems);
	ASSERT(lprvs->lpszText != NULL); // Must supply (descriptive) text for subitem selector
	ASSERT(_tcslen(lprvs->lpszText) < FLATHEADER_TEXT_MAX);
#endif

	try
	{
		SUBITEM subitem;

		subitem.nFormat = lprvs->nFormat;
		subitem.iWidth = lprvs->iWidth<0 ? m_iDefaultWidth:lprvs->iWidth;
		subitem.iMinWidth = lprvs->iMinWidth;
		subitem.iMaxWidth = lprvs->iMaxWidth;
		subitem.iImage = lprvs->nFormat&RVCF_IMAGE ? lprvs->iImage:0;
		subitem.strText = lprvs->lpszText;

		m_arraySubItems.SetAt(iSubItem, subitem);

		if(bUpdateList && m_lprsilc != NULL)
			m_lprsilc->UpdateList();

		return SetSubItemWidth(iSubItem, lprvs->iWidth);
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		return FALSE;
	}
}

BOOL CReportCtrl::UndefineSubItem(INT iSubItem)
{
	ASSERT(iSubItem >= 0);
	ASSERT(iSubItem < m_arraySubItems.GetSize()); // Specify a valid subitem

	VERIFY(!DeactivateSubItem(iSubItem));
	m_arraySubItems.RemoveAt(iSubItem);

	HDITEM hdi;
	hdi.mask = HDI_LPARAM;

	INT i;
	INT iHeaderItems = (INT)m_arrayColumns.GetSize();
	for(i=0;i<iHeaderItems;i++)
		if(m_wndHeader.GetItem(m_arrayColumns[i], &hdi))
			if(hdi.lParam > iSubItem)
			{
				hdi.lParam--;
				m_wndHeader.SetItem(m_arrayColumns[i], &hdi);
			}

	VERIFY(m_itemEdit.rdData.DeleteSubItem(iSubItem));

	if(!(m_dwStyle&RVS_OWNERDATA))
	{
		INT iItems = (INT)m_arrayItems.GetSize();
		for(i=0;i<iItems;i++)
			VERIFY(m_arrayItems[i].rdData.DeleteSubItem(iSubItem));
	}

	if(m_lprsilc != NULL)
		m_lprsilc->UpdateList();

	return TRUE;
}

void CReportCtrl::UndefineAllSubItems()
{
     while (m_arraySubItems.GetSize() > 0)
          UndefineSubItem(0);
}

INT CReportCtrl::AddItem(LPCTSTR lpszText, INT iImage, INT iCheck, INT iTextColor)
{
	INT iItems = (INT)m_arrayItems.GetSize();
	return InsertItem(iItems, lpszText, iImage, iCheck, iTextColor);
}

INT CReportCtrl::AddItem(LPRVITEM lprvi)
{
	INT iItems = (INT)m_arrayItems.GetSize();

	if(lprvi == NULL)
	{
		INT iSubItem = 0, iSubItems = (INT)m_arraySubItems.GetSize();

		RVITEM rvi;
		TCHAR szText[REPORTCTRL_MAX_TEXT];

		rvi.iItem = RVI_EDIT;
		rvi.iSubItem = 0;
		rvi.nMask = RVIM_TEXT;
		rvi.lpszText = szText;
		rvi.iTextMax = REPORTCTRL_MAX_TEXT;
		VERIFY(GetItem(&rvi));

		rvi.iItem = iItems;
		INT iItem = InsertItem(&rvi);
		if(iItem != RVI_INVALID)
		{
			for(iSubItem = 1;iSubItem<iSubItems;iSubItem++)
			{
				rvi.iItem = RVI_EDIT;
				rvi.iSubItem = iSubItem;
				rvi.nMask = RVIM_TEXT;
				rvi.lpszText = szText;
				rvi.iTextMax = REPORTCTRL_MAX_TEXT;
				VERIFY(GetItem(&rvi));

				rvi.iItem = iItem;
				VERIFY(SetItem(&rvi));
			}
		}

		return iItem;
	}
	else
		return InsertItem(lprvi);
}

INT CReportCtrl::InsertItem(INT iItem, LPCTSTR lpszText, INT iImage, INT iCheck, INT iTextColor)
{
	RVITEM rvi;
	rvi.nMask = RVIM_TEXT;
	rvi.iItem = iItem;
	rvi.iSubItem = 0;
	rvi.lpszText = (LPTSTR)lpszText;

	rvi.iTextColor = iTextColor;
	rvi.iImage = iImage;
	rvi.iCheck = iCheck;

	if(iTextColor >= 0)
		rvi.nMask |= RVIM_TEXTCOLOR;

	if(iImage >= 0)
		rvi.nMask |= RVIM_IMAGE;

	if(iCheck >= 0)
		rvi.nMask |= RVIM_CHECK;

	return InsertItem(&rvi);
}

INT CReportCtrl::InsertItem(LPRVITEM lprvi)
{
	if(m_dwStyle&RVS_OWNERDATA || m_dwStyle&RVS_TREEVIEW )
		return RVI_INVALID;	// Use set SetItemCount() when using OWNERDATA style
							// Use tree item variants when using TREEVIEW style

	INT iItem = InsertItemImpl(lprvi);
	if(iItem != RVI_INVALID)
	{
		m_bUpdateItemMap = TRUE;
		m_iVirtualHeight++;

		if(m_iFocusRow >= RVI_FIRST && m_iFocusRow >= GetRowFromItem(iItem))
			m_iFocusRow++;
	}

	return iItem;
}

BOOL CReportCtrl::DeleteItem(INT iItem)
{
	if(m_dwStyle&RVS_OWNERDATA || m_dwStyle&RVS_TREEVIEW)
		return RVI_INVALID;	// Use set SetItemCount() when using OWNERDATA style
							// Use tree item variants when using TREEVIEW style
	if(iItem < RVI_FIRST)
		return FALSE;

	ASSERT(iItem <= m_arrayItems.GetSize());

	INT iRow = GetRowFromItem(iItem);
	BOOL bResult = DeleteItemImpl(iItem);

	m_bUpdateItemMap = TRUE;
	m_iVirtualHeight--;

	if(m_iFocusRow >= iRow)
		m_iFocusRow--;

	m_iFocusRow = m_iFocusRow >= m_iVirtualHeight ? m_iVirtualHeight-1 : m_iFocusRow;

	INT iRows = (INT)m_arrayRows.GetSize();
	INT iFirst = GetScrollPos32(SB_VERT), iLast;
	GetVisibleRows(TRUE, &iFirst, &iLast);

	if (iRow <= iFirst || iLast >= iRows - 1)
		GetVisibleRows(TRUE, &iFirst, &iLast, TRUE);

	ScrollWindow(SB_VERT, iFirst > RVI_INVALID ? iFirst : 0);

	return bResult;
}

HTREEITEM CReportCtrl::InsertItem(LPCTSTR lpszText, INT iImage, INT iCheck, INT iTextColor, HTREEITEM hParent, HTREEITEM hInsertAfter, INT iSubItem)
{
	RVITEM rvi;
	rvi.nMask = RVIM_TEXT;
	rvi.lpszText = (LPTSTR)lpszText;

	rvi.iTextColor = iTextColor;
	rvi.iImage = iImage;
	rvi.iCheck = iCheck;

	if(iTextColor >= 0)
		rvi.nMask |= RVIM_TEXTCOLOR;

	if(iImage >= 0)
		rvi.nMask |= RVIM_IMAGE;

	if(iCheck >= 0)
		rvi.nMask |= RVIM_CHECK;

	return InsertItem(&rvi, hParent, hInsertAfter, iSubItem);
}

HTREEITEM CReportCtrl::InsertItem(LPRVITEM lprvi, HTREEITEM hParent, HTREEITEM hInsertAfter, INT iSubItem)
{
	if(!(m_dwStyle&RVS_TREEVIEW))
		return NULL;	// TREEVIEW style must be set to use this style.

	LPTREEITEM lptiFocus = GetTreeFocus();

	LPTREEITEM lpti = new TREEITEM;
	if( lpti != NULL )
	{
		lprvi->iItem = (INT)m_arrayItems.GetSize();
		lprvi->iSubItem = 0;

		lpti->iItem = InsertItemImpl(lprvi);
		lpti->iSubItem = iSubItem;

		if(lpti->iItem != RVI_INVALID)
		{
			LPTREEITEM lptiParent = hParent==RVTI_ROOT ? &m_tiRoot:(LPTREEITEM)hParent;

			InsertTree(lprvi->iSubItem, lptiParent, (LPTREEITEM)hInsertAfter, lpti, TRUE);

			m_arrayItems[lpti->iItem].lptiItem = lpti;
			lpti->iIndent = hParent==RVTI_ROOT ? 1:lptiParent->iIndent+1;
			lpti->lptiParent = lptiParent;
		}
		else
		{
			delete lpti;
			lpti = NULL;
		}
	}

	BuildTree();
	SetTreeFocus(lptiFocus);

	return (HTREEITEM)lpti;
}

BOOL CReportCtrl::DeleteItem(HTREEITEM hItem)
{
	if(!(m_dwStyle&RVS_TREEVIEW))
		return FALSE;	// TREEVIEW style must be set to use this style.

	LPTREEITEM lptiFocus = GetTreeFocus();

	if(
		lptiFocus != NULL
		&& (IsDescendent(hItem, (HTREEITEM)lptiFocus) || (HTREEITEM)lptiFocus == hItem)
	) {
		if(lptiFocus->lptiSibling != NULL)
			lptiFocus = lptiFocus->lptiSibling;
		else if(lptiFocus->lptiParent != &m_tiRoot)
			lptiFocus = lptiFocus->lptiParent;
		else
			lptiFocus = NULL;
	}
//	else
//  if we get here then:
//  - the focused item isn't a descendent of the "to be deleted" item
//  - the focused item isn't the deleted item iself
//		lptiFocus = NULL; // then why o why was the focus being reset over here???

	BOOL bResult = DeleteItemImpl((LPTREEITEM)hItem, FALSE);

	BuildTree();
	SetTreeFocus(lptiFocus);

	return bResult;
}

BOOL CReportCtrl::DeleteAllItems()
{
	BOOL bResult = TRUE;

	if(m_dwStyle&RVS_OWNERDATA) // Use set SetItemCount() when using this style
		return FALSE;

	if(m_dwStyle&RVS_TREEMASK)
	{
		bResult = DeleteItemImpl(&m_tiRoot, TRUE);
	}
	else
	{
		RVITEM rvi;
		for (int iItem=0; iItem<m_arrayItems.GetSize(); iItem++)
		{
			rvi.nMask = RVIM_LPARAM;
			rvi.iItem = iItem;
			GetItem(&rvi);
			Notify(RVN_ITEMDELETED, iItem, 0, 0, rvi.lParam);
		}
	}

	m_arrayRows.RemoveAll();
	m_arrayItems.RemoveAll();
	m_listSelection.RemoveAll();

	m_bUpdateItemMap = TRUE;
	m_iVirtualHeight = 0;
	m_iFocusRow = RVI_INVALID;

	ScrollWindow(SB_VERT, 0);
	return bResult;
}

void CReportCtrl::RedrawItems(INT iFirst, INT iLast)
{
	CRect rect;

	if(iFirst == iLast || iLast == RVI_INVALID)
	{
		GetItemRect(iFirst, -1, rect);

		if(m_bPreview)
			rect.bottom += GetItemStruct(iFirst, -1, RVIM_PREVIEW).nPreview;

		rect.OffsetRect(GetScrollPos32(SB_HORZ), 0);
	}
	else
		rect = m_rectReport;

	InvalidateRect(rect);
}

BOOL CReportCtrl::EnsureVisible(INT iItem, BOOL bUnobstructed)
{
	INT iFirst = GetScrollPos32(SB_VERT), iLast;
	INT iRow = GetRowFromItem(iItem);
	if(iRow < RVI_FIRST)
		return FALSE;

	GetVisibleRows(bUnobstructed, &iFirst, &iLast);

	if(iRow<iFirst)
		ScrollWindow(SB_VERT, iRow);

	if(iRow>iLast)
	{
		iLast = iRow;
		GetVisibleRows(bUnobstructed, &iFirst, &iLast, TRUE);
		ScrollWindow(SB_VERT, iFirst);
	}

	return TRUE;
}

BOOL CReportCtrl::EnsureVisible(HTREEITEM hItem, BOOL bUnobstructed)
{
	LPTREEITEM lpti = (LPTREEITEM)hItem;
	while(lpti->lptiParent != &m_tiRoot)
	{
		Expand((HTREEITEM)lpti->lptiParent, RVE_EXPAND);
		lpti = lpti->lptiParent;
	}

	lpti = (LPTREEITEM)hItem;
	return EnsureVisible(lpti->iItem, bUnobstructed);
}

INT CReportCtrl::InsertColor(INT iIndex, COLORREF crColor)
{
	try
	{
		m_arrayColors.InsertAt(iIndex, crColor);
		if(CreatePalette())
			return iIndex;

		return -1;
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		return -1;
	}
}

BOOL CReportCtrl::DeleteColor(INT iIndex)
{
	if(iIndex >= m_arrayColors.GetSize())
		return FALSE;

	m_arrayColors.RemoveAt(iIndex);
	CreatePalette();

	return TRUE;
}

void CReportCtrl::DeleteAllColors()
{
	m_arrayColors.RemoveAll();
	CreatePalette();
}

INT CReportCtrl::HitTest(LPRVHITTESTINFO lprvhti)
{
	ASSERT(lprvhti);

	lprvhti->nFlags = 0;
	lprvhti->iItem = RVI_INVALID;
	lprvhti->iSubItem = -1;

	lprvhti->hItem = NULL;

	lprvhti->iRow = RVI_INVALID;
	lprvhti->iColumn = -1;

	CRect rectItem = m_rectEdit;
	rectItem.bottom -= GetSystemMetrics(SM_CYFIXEDFRAME)*2;

	if(
		rectItem.PtInRect(lprvhti->point) ||
		m_rectReport.PtInRect(lprvhti->point)
	) {
		INT iHPos = GetScrollPos32(SB_HORZ);
		INT iFirst = GetScrollPos32(SB_VERT), iLast;

		BOOL bCheckItem = FALSE;

		rectItem.left = -iHPos;
		rectItem.right = rectItem.left + m_iVirtualWidth;

		if(rectItem.PtInRect(lprvhti->point))
		{
			iFirst = RVI_EDIT;

			lprvhti->iItem = RVI_EDIT;
			lprvhti->nFlags |= RVHT_ONITEMEDIT;

			bCheckItem = TRUE;
		}
		else if(!bCheckItem && GetVisibleRows(FALSE, &iFirst, &iLast))
		{
			ITEM item;
			rectItem.SetRect(
				m_rectReport.left-iHPos,					m_rectReport.top,
				m_rectReport.left-iHPos+m_iVirtualWidth,	m_rectReport.top
			);

			for(;iFirst<=iLast;iFirst++)
			{
				GetItemFromRow(iFirst, item);
				rectItem.bottom += m_iDefaultHeight + (m_bPreview ? item.nPreview:0);

				if(rectItem.PtInRect(lprvhti->point))
					break;

				rectItem.top = rectItem.bottom;
			}

			if(iFirst<=iLast)
			{
				lprvhti->iItem = GetItemFromRow(iFirst);
				lprvhti->iRow = iFirst;

				if(m_dwStyle&RVS_TREEMASK && lprvhti->iItem>=RVI_FIRST)
					lprvhti->hItem = (HTREEITEM)m_arrayItems[lprvhti->iItem].lptiItem;

				if(m_bPreview && item.nPreview)
				{
					CRect rectPreview(rectItem);
					rectPreview.top += m_iDefaultHeight;

					if(rectPreview.PtInRect(lprvhti->point))
					{
						lprvhti->nFlags |= RVHT_ONITEMPREVIEW;
						return lprvhti->iItem;
					}
				}

				bCheckItem = TRUE;
			}
		}

		if(bCheckItem == TRUE)
		{
			bCheckItem = FALSE;

			if(
				m_dwStyle&RVS_TREEMASK &&
				m_arrayItems[lprvhti->iItem].lptiItem->iSubItem >= 0
			) {
				LPTREEITEM lpti = m_arrayItems[lprvhti->iItem].lptiItem;

				lprvhti->iSubItem = lpti->iSubItem;
				lprvhti->iColumn = 0;

				rectItem.OffsetRect(iHPos, 0);

				lprvhti->rect = rectItem;
				lprvhti->rect.right = lprvhti->rect.left + m_iVirtualWidth;
				lprvhti->rect.bottom = rectItem.top + m_iDefaultHeight;

				bCheckItem = TRUE;
			}
			else
			{
				INT iHeaderItem;
				INT iHeaderItems = (INT)m_arrayColumns.GetSize();

				HDITEM hdi;
				hdi.mask = HDI_WIDTH|HDI_LPARAM;

				rectItem.right = rectItem.left;

				for(iHeaderItem=0;iHeaderItem<iHeaderItems;iHeaderItem++)
				{
					CRect rect;
					m_wndHeader.GetItemRect(iHeaderItem, &rect);

					CPoint point = lprvhti->point;
					point.y = (rect.bottom - rect.top) / 2;
					point.x += iHPos;

					if(point.x>=rect.left && point.x<rect.right)
					{
						rectItem.left = rect.left;
						rectItem.right = rect.right;
						break;
					}
				}

				ASSERT(iHeaderItem < iHeaderItems);
				m_wndHeader.GetItem(iHeaderItem, &hdi);

				lprvhti->iSubItem = (INT)hdi.lParam;
				lprvhti->iColumn = GetColumnFromSubItem(lprvhti->iSubItem);

				if(iHeaderItem<iHeaderItems)
					bCheckItem = TRUE;
			}

			if(bCheckItem == TRUE)
			{
				RVITEM rvi;
				rvi.iItem = GetItemFromRow(iFirst);
				rvi.iSubItem = lprvhti->iSubItem;
				GetItem(&rvi);

				if(lprvhti->iColumn == 0 && rvi.nMask&RVIM_INDENT && rvi.iIndent >= 0)
				{
					rectItem.left += rvi.iIndent*m_iDefaultHeight;
					if(rvi.nState&RVIS_TREEMASK)
					{
						if(
							lprvhti->point.x >= rectItem.left - m_iDefaultHeight &&
							lprvhti->point.x < rectItem.left
						) {
							lprvhti->rect = rectItem;
							lprvhti->rect.left -= iHPos + m_iDefaultHeight;
							lprvhti->rect.right -= lprvhti->rect.left;
							lprvhti->rect.bottom = rectItem.top + m_iDefaultHeight;

							lprvhti->nFlags |= RVHT_ONITEMTREEBOX;
							return lprvhti->iItem;
						}
					}
				}

				rectItem.OffsetRect(-iHPos, 0);

				lprvhti->rect = rectItem;
				lprvhti->rect.bottom = rectItem.top + m_iDefaultHeight;

				rectItem.right = rectItem.left + m_iSpacing;
				if(rvi.nMask&RVIM_IMAGE)
				{
					rectItem.right += m_sizeImage.cx + m_iSpacing;
					if(lprvhti->point.x < rectItem.right)
					{
						lprvhti->nFlags |= RVHT_ONITEMIMAGE;
						return lprvhti->iItem;
					}
				}

				if(rvi.nMask&RVIM_CHECK)
				{
					rectItem.right +=  m_sizeCheck.cx + m_iSpacing;
					if(lprvhti->point.x < rectItem.right)
					{
						lprvhti->nFlags |= RVHT_ONITEMCHECK;
						return lprvhti->iItem;
					}
				}

				if(
					lprvhti->point.x >= rectItem.right &&
					lprvhti->point.x < lprvhti->rect.right - m_iSpacing
				) {
					lprvhti->nFlags |= RVHT_ONITEMTEXT;
					return lprvhti->iItem;
				}

				lprvhti->nFlags = RVHT_NOWHERE;
			}
			else
				lprvhti->nFlags = RVHT_NOWHERE;
		}
		else
			lprvhti->nFlags = RVHT_NOWHERE;
	}
	else
	{
		lprvhti->nFlags |= lprvhti->point.y<m_rectReport.top ? RVHT_ABOVE:0;
		lprvhti->nFlags |= lprvhti->point.y>m_rectReport.bottom ? RVHT_BELOW:0;
		lprvhti->nFlags |= lprvhti->point.x<m_rectReport.left ? RVHT_TOLEFT:0;
		lprvhti->nFlags |= lprvhti->point.x>m_rectReport.right ? RVHT_TORIGHT:0;
	}

	return lprvhti->iItem;
}

BOOL CReportCtrl::ResortItems()
{
	INT iColumn;
	INT iSubItem;
	BOOL bAscending;

	iColumn = m_wndHeader.GetSortColumn(&bAscending);
	iSubItem = GetSubItemFromColumn(iColumn);
	if(iSubItem >= 0)
		return SortItems(iSubItem, bAscending);
	else
		return FALSE;
}

BOOL CReportCtrl::SortItems(INT iSubItem, BOOL bAscending)
{
	if(m_dwStyle&RVS_OWNERDATA)
		return FALSE;	// Can't sort if data is managed by owner

	INT iColumn = GetColumnFromSubItem(iSubItem);
	if(iColumn < 0)
		return FALSE;

	m_wndHeader.SetSortColumn(m_arrayColumns[iColumn], bAscending);

	if(m_dwStyle&RVS_TREEMASK)
	{
		LPTREEITEM lptiFocus = GetTreeFocus();

		TreeSort(iSubItem, &m_tiRoot, bAscending, TRUE);

		BuildTree();
		SetTreeFocus(lptiFocus);
	}
	else
	{
		INT iRows = (INT)m_arrayRows.GetSize();
		if(!iRows)
			return FALSE;

		INT iFocusItem = GetItemFromRow(m_iFocusRow);

		QuickSort(iSubItem, 0, iRows-1, bAscending);

		m_bUpdateItemMap = TRUE;

		if(iFocusItem>=0)
		{
			m_iFocusRow = GetRowFromItem(iFocusItem);

			INT iFirst = m_iFocusRow, iLast;
			GetVisibleRows(TRUE, &iFirst, &iLast);
			GetVisibleRows(TRUE, &iFirst, &iLast, TRUE);

			ScrollWindow(SB_VERT, iFirst);
		}
	}

	Invalidate();
	return TRUE;
}

INT CReportCtrl::CompareItems(LPRVITEM lprvi1, LPRVITEM lprvi2)
{
	ASSERT(!(m_dwStyle&RVS_OWNERDATA)); // Can't sort if data is managed by owner

	if((!_tcslen(lprvi1->lpszText)) && (!_tcslen(lprvi2->lpszText)))
	{
		if((lprvi1->nMask&RVIM_CHECK || lprvi2->nMask&RVIM_CHECK))
		{
			if(lprvi1->iCheck < lprvi2->iCheck)
				return -1;
			else if(lprvi1->iCheck > lprvi2->iCheck)
				return 1;
			else
				return 0;
		}

		if(lprvi1->iImage < lprvi2->iImage)
			return -1;
		else if(lprvi1->iImage > lprvi2->iImage)
			return 1;
		else
			return 0;
		}
	else
		return _tcscmp(lprvi1->lpszText, lprvi2->lpszText);
}

INT CReportCtrl::CompareItems(INT iItem1, INT iSubItem1, INT iItem2, INT iSubItem2)
{
	ASSERT(!(m_dwStyle&RVS_OWNERDATA)); // Can't sort if data is managed by owner

	if(m_lpfnrvc == NULL)
	{
		RVITEM rvi1, rvi2;

		TCHAR szText1[REPORTCTRL_MAX_TEXT], szText2[REPORTCTRL_MAX_TEXT];

		rvi1.nMask = RVIM_TEXT;
		rvi1.iItem = iItem1;
		rvi1.iSubItem = iSubItem1;
		rvi1.lpszText = szText1;
		rvi1.iTextMax = REPORTCTRL_MAX_TEXT;
		VERIFY(GetItem(&rvi1));

		rvi2.nMask = RVIM_TEXT;
		rvi2.iItem = iItem2;
		rvi2.iSubItem = iSubItem2;
		rvi2.lpszText = szText2;
		rvi2.iTextMax = REPORTCTRL_MAX_TEXT;
		VERIFY(GetItem(&rvi2));

		if(!(rvi1.nMask&RVIM_TEXT || rvi2.nMask&RVIM_TEXT))
		{
			if((rvi1.nMask&RVIM_CHECK || rvi2.nMask&RVIM_CHECK))
			{
				if(rvi1.iCheck < rvi2.iCheck)
					return -1;
				else if(rvi1.iCheck > rvi2.iCheck)
					return 1;
				else
					return 0;
			}

			if(rvi1.iImage < rvi2.iImage)
				return -1;
			else if(rvi1.iImage > rvi2.iImage)
				return 1;
			else
				return 0;
		}
		else
			return _tcscmp(szText1, szText2);
	}
	else
		return m_lpfnrvc(iItem1, iSubItem1, iItem2, iSubItem2, m_lParamCompare);
}

INT CReportCtrl::FindItem(LPRVFINDINFO lprvfi, INT iSubItem, INT iStart)
{
	if(m_dwStyle&RVS_OWNERDATA)
		return FALSE; // Can't find data if data is managed by owner

	INT iItems = (INT)m_arrayItems.GetSize();
	INT iItem = iStart + (lprvfi->nFlags&RVFI_UP ? -1:1);

	RVITEM rvi;
	TCHAR szText[REPORTCTRL_MAX_TEXT];
	rvi.lpszText = szText;
	rvi.iTextMax = REPORTCTRL_MAX_TEXT;

	BOOL bMatch;

	while(iItem != iStart)
	{
		if(iItem >= iItems)
		{
			if(lprvfi->nFlags&RVFI_WRAP)
				iItem = RVI_EDIT;
			else
				break;
		}

		if(iItem < RVI_EDIT)
		{
			if(lprvfi->nFlags&RVFI_WRAP)
				iItem = iItems-1;
			else
				break;
		}

		rvi.nMask = RVIM_TEXT|RVIM_IMAGE|RVIM_CHECK|RVIM_LPARAM;
		rvi.iItem = iItem;
		rvi.iSubItem = iSubItem;
		VERIFY(GetItem(&rvi));

		bMatch = TRUE;
		if(lprvfi->nFlags&RVFI_TEXT && rvi.nMask&RVIM_TEXT)
		{
			INT iMatch;
			if(lprvfi->nFlags&RVFI_PARTIAL)
				iMatch = _tcsncmp(lprvfi->lpszText, rvi.lpszText, _tcslen(lprvfi->lpszText));
			else
				iMatch = _tcscmp(lprvfi->lpszText, rvi.lpszText);

			bMatch &= !iMatch ? TRUE:FALSE;
		}
		else
			if(lprvfi->nFlags&RVFI_TEXT)
				bMatch = FALSE;

		if(lprvfi->nFlags&RVFI_IMAGE && rvi.nMask&RVIM_IMAGE)
			bMatch &= lprvfi->iImage == rvi.iImage ? TRUE:FALSE;
		else
			if(lprvfi->nFlags&RVFI_IMAGE)
				bMatch = FALSE;

		if(lprvfi->nFlags&RVFI_CHECK && rvi.nMask&RVIM_CHECK)
			bMatch &= lprvfi->iCheck == rvi.iCheck ? TRUE:FALSE;
		else
			if(lprvfi->nFlags&RVFI_CHECK)
				bMatch = FALSE;

		if(lprvfi->nFlags&RVFI_LPARAM && rvi.nMask&RVIM_LPARAM)
              bMatch &= lprvfi->lParam == rvi.lParam ? TRUE:FALSE;
        else
              if(lprvfi->nFlags&RVFI_LPARAM)
                    bMatch = FALSE;

		if(bMatch)
			return iItem;

		iItem += lprvfi->nFlags&RVFI_UP ? -1:1;
	}

	return RVI_INVALID;
}

BOOL CReportCtrl::Expand(HTREEITEM hItem, UINT nCode)
{
	if(!(m_dwStyle&RVS_TREEMASK))
		return FALSE;

	LPTREEITEM lpti = (LPTREEITEM)hItem;
	switch(nCode)
	{
	case RVE_COLLAPSE:	lpti->bOpen = FALSE; break;
	case RVE_EXPAND:	lpti->bOpen = TRUE; break;
	case RVE_TOGGLE:	lpti->bOpen = !lpti->bOpen; break;
	default:			return FALSE;
	}

	SetState(m_iFocusRow, 0, RVIS_FOCUSED);
	INT iFocusItem = GetItemFromRow(m_iFocusRow);

	BuildTree();

	if(iFocusItem >= RVI_FIRST)
	{
		LPTREEITEM lptiAncestor = GetVisibleAncestor(lpti);
		ASSERT(lptiAncestor != &m_tiRoot && lptiAncestor != NULL);

		m_iFocusRow = GetRowFromItem(lptiAncestor->iItem);
		ASSERT(m_iFocusRow != RVI_INVALID);

		SetState(m_iFocusRow, RVIS_FOCUSED, RVIS_FOCUSED);
		DeselectDescendents(lptiAncestor);
	}

	INT iFirst = GetScrollPos32(SB_VERT), iLast;
	GetVisibleRows(TRUE, &iFirst, &iLast);

	if(m_iFocusRow <= iFirst || iLast >= m_iFocusRow - 1)
		GetVisibleRows(TRUE, &iFirst, &iLast, TRUE);

	ScrollWindow(SB_VERT, iFirst > RVI_INVALID ? iFirst : 0);
	return TRUE;
}

BOOL CReportCtrl::ExpandAll(HTREEITEM hItem, UINT nCode, INT iLevels)
{
	if(!(m_dwStyle&RVS_TREEMASK))
		return FALSE;

	LPTREEITEM lpti = (LPTREEITEM)hItem;;

	if (nCode == RVE_TOGGLE)
	{
		if (lpti->bOpen)
			nCode = RVE_COLLAPSE;
		else
			nCode = RVE_EXPAND;
	}
	
	if(hItem == RVTI_ROOT)
	{
		lpti = &m_tiRoot;
	}
	else
	{
		Expand(hItem, nCode);

		lpti = (LPTREEITEM)hItem;
	}

	return ExpandAllImpl(hItem, nCode, iLevels, 0);
}

void CReportCtrl::FlushCache(INT iItem)
{
	if(!(m_dwStyle&RVS_OWNERDATA || m_bUseItemCacheMap))
		return;

	CacheDelete(iItem);
}

void CReportCtrl::Copy()
{
	CString str;

	if (OpenClipboard() == FALSE || ::EmptyClipboard() == FALSE)
		return;

	HGLOBAL	hMem = NULL, h;

	CArray<INT, INT> arraySelection;
	
	SelectionToSortedArray(arraySelection);

	INT i, iSize = (INT)arraySelection.GetSize();
	UINT nData = 0;

	for(i=0;i<iSize;i++)
	{
		INT iRow = arraySelection[i];
		INT iItem = GetItemFromRow(iRow);
		ASSERT(iItem > RVI_INVALID);

		str = GetItemString( iItem, g_cCBSeparator );

		ASSERT(str.GetLength() > 0 );

		if( hMem == NULL )
			h = ::GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT, str.GetLength()*(sizeof(TCHAR)+1) );
		else
			h = ::GlobalReAlloc( hMem, nData + str.GetLength()*(sizeof(TCHAR)+1), GMEM_MOVEABLE|GMEM_DDESHARE );

		if( h == NULL )
			break;

		hMem = h;

		LPBYTE lpx = (LPBYTE)::GlobalLock(hMem);
		ASSERT(lpx != NULL);

		memcpy(lpx + nData, (LPCTSTR)str, str.GetLength()*sizeof(TCHAR));

		::GlobalUnlock(hMem);

		nData += str.GetLength()*sizeof(TCHAR);
		((LPTSTR)lpx)[nData/sizeof(TCHAR)] = 0;
	}

	if(hMem != NULL)
		::SetClipboardData(CF_TEXT, hMem);

	::CloseClipboard();
}

void CReportCtrl::Cut()
{
}

void CReportCtrl::Paste()
{
}

/////////////////////////////////////////////////////////////////////////////
// CReportCtrl misc

INT CReportCtrl::PreviewHeight(CFont* pFont, UINT nLines)
{
	INT iHeight = -1;

	ASSERT(pFont != NULL);

	CDC* pDC = GetDC();
	if(pDC)
	{
		CFont* pDCFont = pDC->SelectObject(pFont);
		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);
		pDC->SelectObject(pDCFont);
		ReleaseDC(pDC);

		iHeight = (tm.tmHeight + tm.tmExternalLeading)*nLines;
	}

	return iHeight;
}

INT CReportCtrl::PreviewHeight(CFont* pFont, LPCTSTR lpszText, LPRECT lpRect)
{
	INT iHeight = -1;

	ASSERT(pFont != NULL);

	CDC* pDC = GetDC();
	if(pDC)
	{
		CRect rect( 0, 0, m_iVirtualWidth, 0);
		if(lpRect != NULL)
			rect = *lpRect;

		pDC->SelectObject(pFont);
		iHeight = pDC->DrawText( lpszText, -1, rect, DT_NOPREFIX|DT_CALCRECT|DT_EXPANDTABS|DT_WORDBREAK|DT_NOCLIP );
		ReleaseDC(pDC);
	}

	return iHeight;
}

/////////////////////////////////////////////////////////////////////////////
// CReportCtrl implementation

void CReportCtrl::GetSysColors()
{
	m_crBackground = ::GetSysColor(COLOR_WINDOW);
	m_crBkSelected = ::GetSysColor(COLOR_HIGHLIGHT);
	m_crBkSelectedNoFocus = ::GetSysColor(COLOR_BTNFACE);
	m_crText = ::GetSysColor(COLOR_WINDOWTEXT);
	m_crTextSelected = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_crTextSelectedNoFocus = ::GetSysColor(COLOR_WINDOWTEXT);
	m_crGrid = ::GetSysColor(COLOR_BTNFACE);
	m_cr3DFace = ::GetSysColor(COLOR_3DFACE);
	m_cr3DLight = ::GetSysColor(COLOR_3DLIGHT);
	m_cr3DShadow = ::GetSysColor(COLOR_3DSHADOW);
	m_cr3DHiLight = ::GetSysColor(COLOR_3DHILIGHT);
	m_cr3DDkShadow = ::GetSysColor(COLOR_3DDKSHADOW);
}

UINT CReportCtrl::GetMouseScrollLines()
{
	UINT nScrollLines = 3; // Reasonable default
	HKEY hKey;

	if(RegOpenKeyEx(
		HKEY_CURRENT_USER,  _T("Control Panel\\Desktop"),
		0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS
	) {
		TCHAR szData[128];
		DWORD dwKeyDataType;
		DWORD dwDataBufSize = sizeof(szData);

		if(RegQueryValueEx(
			hKey, _T("WheelScrollLines"), NULL, &dwKeyDataType,
			(LPBYTE) &szData, &dwDataBufSize) == ERROR_SUCCESS
		)
			nScrollLines = _tcstoul(szData, NULL, 10);

		RegCloseKey(hKey);
	}

	return nScrollLines;
}

BOOL CReportCtrl::CreatePalette()
{
	if(m_palette.m_hObject)
		m_palette.DeleteObject();

	INT iUserColors = (INT)m_arrayColors.GetSize();
	INT iBitmapColors = 0;

	DIBSECTION ds;
	BITMAPINFOHEADER &bmInfo = ds.dsBmih;

	if( (HBITMAP)m_bitmap != NULL )
	{
		m_bitmap.GetObject( sizeof(ds), &ds );
		iBitmapColors = bmInfo.biClrUsed ? bmInfo.biClrUsed : 1 << bmInfo.biBitCount;
	}

	INT iColors = iUserColors + iBitmapColors;

	if( !iColors )
		return FALSE;

	CClientDC cdc(NULL);
	if( iColors <= 256 )
	{
		UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * iColors);
		LOGPALETTE *pLP = (LOGPALETTE *) new BYTE[nSize];

		pLP->palVersion = 0x300;
		pLP->palNumEntries = (WORD)iColors;

		INT i;
		for( i=0;i<iUserColors;i++ )
		{
			pLP->palPalEntry[i].peRed   = GetRValue(m_arrayColors[i]);
			pLP->palPalEntry[i].peGreen = GetGValue(m_arrayColors[i]);
			pLP->palPalEntry[i].peBlue  = GetBValue(m_arrayColors[i]);
			pLP->palPalEntry[i].peFlags = 0;
		}

		if( iBitmapColors > 0 )
		{
			// Create the palette
			RGBQUAD *pRGB = new RGBQUAD[iBitmapColors];

			CDC dc;
			dc.CreateCompatibleDC(&cdc);

			dc.SelectObject( &m_bitmap );
			::GetDIBColorTable( dc, 0, iColors, pRGB );

			for( INT i=0;i<iBitmapColors;i++ )
			{
				pLP->palPalEntry[iUserColors+i].peRed = pRGB[i].rgbRed;
				pLP->palPalEntry[iUserColors+i].peGreen = pRGB[i].rgbGreen;
				pLP->palPalEntry[iUserColors+i].peBlue = pRGB[i].rgbBlue;
				pLP->palPalEntry[iUserColors+i].peFlags = 0;
			}

			delete[] pRGB;
		}

		m_palette.CreatePalette( pLP );

		delete[] pLP;
	}
	else
		m_palette.CreateHalftonePalette( &cdc );

	return TRUE;
}

BOOL CReportCtrl::NotifyHdr(LPNMRVHEADER lpnmrvhdr, UINT nCode, INT iHdrItem)
{
	lpnmrvhdr->hdr.hwndFrom = GetSafeHwnd();
	lpnmrvhdr->hdr.idFrom = GetDlgCtrlID();
	lpnmrvhdr->hdr.code = nCode;

	HDITEM hdi;
	hdi.mask = HDI_LPARAM;
	hdi.lParam = -1;
	m_wndHeader.GetItem(iHdrItem, &hdi);

	lpnmrvhdr->iSubItem = (INT)hdi.lParam;

	return Notify((LPNMREPORTVIEW)lpnmrvhdr);
}

BOOL CReportCtrl::Notify(UINT nCode, INT iItem, INT iSubItem, UINT nState, LPARAM lParam)
{
	NMREPORTVIEW nmrv;
	nmrv.hdr.hwndFrom = GetSafeHwnd();
	nmrv.hdr.idFrom = GetDlgCtrlID();
	nmrv.hdr.code = nCode;

	nmrv.iItem = iItem;
	nmrv.iSubItem = iSubItem;

	if(m_dwStyle&RVS_TREEMASK && iItem>=RVI_FIRST)
		nmrv.hItem = (HTREEITEM)m_arrayItems[iItem].lptiItem;

	nmrv.nState = nState;

	nmrv.lParam = lParam;

	return Notify(&nmrv);
}

BOOL CReportCtrl::Notify(UINT nCode, UINT nKeys, LPRVHITTESTINFO lprvhti)
{
	NMREPORTVIEW nmrv;
	ZeroMemory(&nmrv, sizeof(NMREPORTVIEW));
	nmrv.hdr.hwndFrom = GetSafeHwnd();
	nmrv.hdr.idFrom = GetDlgCtrlID();
	nmrv.hdr.code = nCode;

	nmrv.nKeys = nKeys;
	nmrv.point = lprvhti->point;
	nmrv.nFlags = lprvhti->nFlags;

	nmrv.iItem = lprvhti->iItem;
	nmrv.iSubItem = lprvhti->iSubItem;

	if(m_dwStyle&RVS_TREEMASK && lprvhti->iItem>=RVI_FIRST)
		nmrv.hItem = (HTREEITEM)m_arrayItems[lprvhti->iItem].lptiItem;

	if(lprvhti->iItem >= 0 && (!(m_dwStyle&RVS_OWNERDATA)))
		nmrv.lParam = GetItemStruct(lprvhti->iItem, -1).lParam;

	return Notify(&nmrv);
}

BOOL CReportCtrl::Notify(LPNMREPORTVIEW lpnmrv)
{
	INT iCode = 0-(lpnmrv->hdr.code-RVN_FIRST);

	if(m_uNotifyMask&(1<<iCode))
	{
		CWnd* pWnd = GetParent();
		if(pWnd)
			return !!pWnd->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)lpnmrv);
	}

	return FALSE;
}

void CReportCtrl::Layout(INT cx, INT cy)
{
	HDLAYOUT hdl;
	WINDOWPOS wpos;

	hdl.prc = &m_rectReport;
	hdl.pwpos = &wpos;

	m_rectReport.SetRect(0, 0, cx, cy);

	if(IsWindow(m_wndHeader.GetSafeHwnd()))
	{
		m_wndTip.Hide();

		VERIFY(m_wndHeader.SendMessage(HDM_LAYOUT, 0, (LPARAM)&hdl));
		if(m_dwStyle&RVS_NOHEADER)
		{
			m_rectHeader.SetRect(wpos.x, 0, wpos.x+wpos.cx, 0);
			m_rectReport.top = m_rectHeader.bottom;;
		}
		else
			m_rectHeader.SetRect(wpos.x, wpos.y, wpos.x+wpos.cx, wpos.y+wpos.cy);

		m_rectTop = m_rectHeader;

		if(m_dwStyle&RVS_SHOWEDITROW)
		{
			UINT nFrameHeight = GetSystemMetrics(SM_CYFIXEDFRAME);
			m_rectTop.bottom += m_iDefaultHeight + 2*nFrameHeight;
			m_rectReport.top += m_iDefaultHeight + 2*nFrameHeight;

			m_rectEdit = m_rectTop;
			m_rectEdit.top = m_rectHeader.bottom;
		}
		else
			m_rectEdit.SetRectEmpty();

		ScrollWindow(SB_HORZ, GetScrollPos32(SB_HORZ));
		ScrollWindow(SB_VERT, GetScrollPos32(SB_VERT));
	}
}

CReportCtrl::LPITEM CReportCtrl::CacheLookup(INT iItem)
{
	if(iItem == RVI_EDIT)
		return m_bEditValid ? &m_itemEdit:NULL;

	UINT nIndex = iItem%REPORTCTRL_MAX_CACHE;

	if(m_aciCache[nIndex].iItem == iItem)
		return &m_aciCache[nIndex].item;

	return NULL;
}

CReportCtrl::LPITEM CReportCtrl::CacheAdd(INT iItem)
{
	if(iItem == RVI_EDIT)
	{
		m_bEditValid = TRUE;
		return &m_itemEdit;
	}

	UINT nIndex = iItem%REPORTCTRL_MAX_CACHE;

	m_aciCache[nIndex].iItem = iItem;
	return &m_aciCache[nIndex].item;
}

void CReportCtrl::CacheDelete(INT iItem)
{
	if(iItem == RVI_INVALID)
	{
		for(UINT n=0;n<REPORTCTRL_MAX_CACHE;n++)
			m_aciCache[n].iItem = RVI_INVALID;
	}
	else if(iItem == RVI_EDIT)
	{
		m_bEditValid = FALSE;
	}
	else
	{
		UINT nIndex = iItem%REPORTCTRL_MAX_CACHE;

		if(m_aciCache[nIndex].iItem == iItem)
			m_aciCache[nIndex].iItem = RVI_INVALID;
	}
}

void CReportCtrl::BuildTree()
{
	m_iVirtualHeight = 0;
	m_arrayRows.RemoveAll();

	BuildTree(&m_tiRoot);

	m_bUpdateItemMap = TRUE;
}

void CReportCtrl::BuildTree(LPTREEITEM lpti)
{
	LPTREEITEM lptiSibling = lpti->lptiChildren;

	while(lptiSibling != NULL)
	{
		INT iRow = (INT)m_arrayRows.Add(lptiSibling->iItem);
		m_iVirtualHeight++;

		ITEM& item = GetItemStruct(lptiSibling->iItem, lptiSibling->iItem, RVIM_STATE|RVIM_INDENT);
		item.nState &= ~RVIS_TREEMASK;
		item.iIndent = lptiSibling->iIndent;
		if(lptiSibling->lptiChildren != NULL)
		{
			item.nState |= lptiSibling->bOpen ? RVIS_TREEOPENED:RVIS_TREECLOSED;
			SetItemStruct(lptiSibling->iItem, item);
			SetState(iRow, item.nState&RVIS_TREEMASK, RVIS_TREEMASK);

			if(lptiSibling->bOpen)
				BuildTree((LPTREEITEM)lptiSibling);
		}

		lptiSibling = lptiSibling->lptiSibling;
	}
}

void CReportCtrl::InsertTree(INT iSubItem, LPTREEITEM lptiParent, LPTREEITEM lptiInsertAfter, LPTREEITEM lpti, BOOL bAscending)
{
	LPTREEITEM lptiSibling = lptiParent->lptiChildren;

	if(lptiSibling == NULL)
	{
		ASSERT(lptiParent != NULL);
		lptiParent->lptiChildren = lpti;
	}
	else
	{
		switch((ULONG_PTR)lptiInsertAfter)
		{
		case RVTI_FIRST:
			lpti->lptiSibling = lptiSibling;
			lptiParent->lptiChildren = lpti;
			break;

		case RVTI_LAST:
			while(lptiSibling->lptiSibling != NULL)
				lptiSibling = lptiSibling->lptiSibling;
			lptiSibling->lptiSibling = lpti;
			break;

		case RVTI_SORT:
			{
				INT iSubItem1 = lpti->iSubItem<0 ? iSubItem:lpti->iSubItem;
				INT iSubItem2 = lptiSibling->iSubItem<0 ? iSubItem:lptiSibling->iSubItem;

				if(
					(bAscending && CompareItems(lpti->iItem, iSubItem1, lptiSibling->iItem, iSubItem2) <= 0) ||
					(!bAscending && CompareItems(lpti->iItem, iSubItem1, lptiSibling->iItem, iSubItem2) >= 0)
				) {
					lpti->lptiSibling = lptiSibling;
					lptiParent->lptiChildren = lpti;
					break;
				}
				else
				{
					while(lptiSibling->lptiSibling != NULL)
					{
						iSubItem1 = lpti->iSubItem<0 ? iSubItem:lpti->iSubItem;
						iSubItem2 = lptiSibling->iSubItem<0 ? iSubItem:lptiSibling->iSubItem;

						if(
							(bAscending && CompareItems(lpti->iItem, iSubItem1, lptiSibling->lptiSibling->iItem, iSubItem2) <= 0) ||
							(!bAscending && CompareItems(lpti->iItem, iSubItem1, lptiSibling->lptiSibling->iItem, iSubItem2) >= 0)
						)
							break;

						lptiSibling = lptiSibling->lptiSibling;
					}

					lptiInsertAfter = lptiSibling;
				}
			}
			//
			// Intentional
			//

		default:
			lpti->lptiSibling = lptiInsertAfter->lptiSibling;
			lptiInsertAfter->lptiSibling = lpti;
			break;
		}
	}
}

BOOL CReportCtrl::ExpandAllImpl(HTREEITEM hRoot, UINT nCode, INT iLevels, INT iDeep)
{
	HTREEITEM hItem = GetNextItem(hRoot, RVGN_CHILD);

	iDeep++;
	if(iLevels >= 0 && iDeep <= iLevels)
		return TRUE;

	while(hItem != NULL)
	{
		Expand(hItem, nCode);
		if(ExpandAllImpl(hItem, nCode, iLevels, iDeep) == FALSE)
			return FALSE;

		hItem = GetNextItem(hItem, RVGN_NEXT);
	}

	return TRUE;
}

INT CReportCtrl::InsertItemImpl(LPRVITEM lprvi)
{
	ASSERT(lprvi->iItem >= 0);	// Use SetItem to set Edit row item
	ASSERT(lprvi->iItem <= m_arrayItems.GetSize());
	ASSERT(lprvi->iSubItem < m_arraySubItems.GetSize());

	INT iRow = -1;
	BOOL bInserted1 = FALSE, bInserted2 = FALSE;

	try
	{
		ITEM item;
		item.rdData.New((INT)m_arraySubItems.GetSize());

		m_arrayItems.InsertAt(lprvi->iItem, item); bInserted1 = TRUE;
		VERIFY(SetItem(lprvi));

		if(!(m_dwStyle&RVS_TREEMASK))
		{
			iRow = lprvi->iItem;
			m_arrayRows.InsertAt(iRow, INT_MIN); bInserted2 = TRUE;

			INT iItems = (INT)m_arrayRows.GetSize();
			for(INT i=0;i<iItems;i++)
				if(m_arrayRows[i] >= iRow)
					m_arrayRows[i]++;

			m_arrayRows[iRow] = lprvi->iItem;
		}

		ScrollWindow(SB_VERT, GetScrollPos32(SB_VERT));

		CList<INT, INT> list;

		POSITION pos = m_listSelection.GetHeadPosition();
		while(pos != NULL)
		{
			INT iSelectedItem = m_listSelection.GetAt(pos);

			if(iSelectedItem >= lprvi->iItem)
				list.AddTail(iSelectedItem + 1);
			else
				list.AddTail(iSelectedItem);

			m_listSelection.GetNext(pos);
		}

		m_listSelection.RemoveAll();
		m_listSelection.AddTail(&list);

		return lprvi->iItem;
	}
	catch(CMemoryException* e)
	{
		if(bInserted1)
			m_arrayItems.RemoveAt(lprvi->iItem);
		if(bInserted2)
			m_arrayRows.RemoveAt(iRow);

		e->Delete();
		return RVI_INVALID;
	}
}

BOOL CReportCtrl::DeleteItemImpl(INT iItem)
{
	RVITEM rvi;
	rvi.nMask = RVIM_LPARAM;
	rvi.iItem = iItem;
	GetItem(&rvi);
	Notify(RVN_ITEMDELETED, iItem, 0, 0, rvi.lParam);

	INT i, iItems = (INT)m_arrayItems.GetSize();

	if(m_dwStyle&RVS_TREEMASK)
	{
		for(i=0;i<iItems;i++)
		{
			LPTREEITEM lpti = m_arrayItems[i].lptiItem;

			ASSERT(i == lpti->iItem);
			if(lpti->iItem > iItem)
				lpti->iItem--;
		}
	}
	m_arrayItems.RemoveAt(iItem);

	if(!(m_dwStyle&RVS_TREEMASK))
	{
		for(i=0;i<m_arrayRows.GetSize();i++)
			if (m_arrayRows[i] == iItem)
				m_arrayRows.RemoveAt(i);

		INT iRows = (INT)m_arrayRows.GetSize();
		for(i=0;i<iRows;i++)
			if(m_arrayRows[i] > iItem)
				m_arrayRows[i]--;
	}

	POSITION pos = m_listSelection.Find(iItem);
	if(pos != NULL)
		m_listSelection.RemoveAt(pos);

	CList<INT, INT> list;
	pos = m_listSelection.GetHeadPosition();
	while(pos != NULL)
	{
		INT iSelectedItem = m_listSelection.GetAt(pos);

		if(iSelectedItem >= iItem)
			list.AddTail(iSelectedItem - 1);
		else
			list.AddTail(iSelectedItem);

		m_listSelection.GetNext(pos);
	}

	m_listSelection.RemoveAll();
	m_listSelection.AddTail(&list);

	return TRUE;
}

BOOL CReportCtrl::DeleteItemImpl(LPTREEITEM lpti, BOOL bChildrenOnly)
{
	if (lpti == NULL)
		return FALSE;
	LPTREEITEM lptiSibling = lpti->lptiChildren;

	while(lptiSibling != NULL)
	{
		if(lptiSibling->lptiChildren != NULL)
			DeleteItemImpl(lptiSibling, TRUE);

		DeleteItemImpl(lptiSibling->iItem);

		LPTREEITEM lptiChild = lptiSibling;
		lptiSibling = lptiSibling->lptiSibling;
		delete lptiChild;
	}

	lpti->lptiChildren = NULL;

	if(!bChildrenOnly)
	{
		DeleteItemImpl(lpti->iItem);

		lptiSibling = (LPTREEITEM)GetNextItem((HTREEITEM)lpti, RVGN_PREVIOUS);

		if(lptiSibling != NULL)
			lptiSibling->lptiSibling = lpti->lptiSibling;
		else if(lpti->lptiParent->lptiChildren == lpti)
			lpti->lptiParent->lptiChildren = lpti->lptiSibling;

		delete lpti;
	}

	return TRUE;
}

CReportCtrl::ITEM& CReportCtrl::GetItemStruct(INT iItem, INT iSubItem, UINT nMask)
{
	if(m_dwStyle&RVS_OWNERDATA)
	{
		INT iSubItems = (INT)m_arraySubItems.GetSize();

		TCHAR szText[REPORTCTRL_MAX_TEXT+1];
		memset(szText, 0, sizeof(szText));

		LPITEM lpItem = CacheLookup(iItem);
		if(m_bUseItemCacheMap && lpItem != NULL)
		{
			if(iSubItem < 0)
				return *lpItem;

			INT iImage, iOverlay, iCheck, iColor, iTextMax;
			if(lpItem->rdData.GetSubItem(iSubItem, &iImage, &iOverlay, &iCheck, &iColor, szText, &iTextMax))
				return *lpItem;
		}
		else
		{
			VERIFY((lpItem = CacheAdd(iItem))!=0);
			lpItem->rdData.New(iSubItems);
		}

		VERIFY(LoadItemData(lpItem, iItem, iSubItem, nMask));
		return *lpItem;
	}

	return iItem == RVI_EDIT ? m_itemEdit:m_arrayItems[iItem];
}

void CReportCtrl::SetItemStruct(INT iItem, ITEM& item)
{
	if(m_dwStyle&RVS_OWNERDATA)
		return;

	if(iItem == RVI_EDIT)
		m_itemEdit = item;
	else
		m_arrayItems[iItem] = item;
}

BOOL CReportCtrl::LoadItemData(LPITEM lpItem, INT iItem, INT iSubItem, UINT nMask)
{
	CWnd* pWnd = GetParent();
	if(pWnd)
	{
		TCHAR szText[REPORTCTRL_MAX_TEXT+1];
		memset(szText, 0, sizeof(szText));

		NMRVITEMCALLBACK nmrvic;
		nmrvic.hdr.hwndFrom = GetSafeHwnd();
		nmrvic.hdr.idFrom = GetDlgCtrlID();
		nmrvic.hdr.code = RVN_ITEMCALLBACK;

		nmrvic.item.iItem = iItem;
		nmrvic.item.iSubItem = iSubItem;
		nmrvic.item.nMask = nMask;

		if(iSubItem >= 0)
		{
			nmrvic.item.nMask |= RVIM_TEXT;
			nmrvic.item.lpszText = szText;
			nmrvic.item.iTextMax = REPORTCTRL_MAX_TEXT;
		}

		Notify((LPNMREPORTVIEW)&nmrvic);

		INT iColor = nmrvic.item.nMask&RVIM_TEXTCOLOR ? nmrvic.item.iTextColor:-1;
		INT iImage = nmrvic.item.nMask&RVIM_IMAGE ? nmrvic.item.iImage:-1;
		INT iOverlay = nmrvic.item.nMask&RVIM_OVERLAY ? nmrvic.item.iOverlay:-1;
		INT iCheck = nmrvic.item.nMask&RVIM_CHECK ? nmrvic.item.iCheck:-1;

		if(iSubItem >= 0)
			lpItem->rdData.SetSubItem(iSubItem, iImage, iOverlay, iCheck, iColor, szText);

		lpItem->iBkColor =	nmrvic.item.nMask&RVIM_BKCOLOR ? nmrvic.item.iBkColor:-1;
		lpItem->nPreview =	nmrvic.item.nMask&RVIM_PREVIEW ? nmrvic.item.nPreview:0;
		lpItem->nState =	nmrvic.item.nMask&RVIM_STATE ? nmrvic.item.nState:0;
		lpItem->iIndent =	nmrvic.item.nMask&RVIM_INDENT ? nmrvic.item.iIndent:-1;
		lpItem->lParam =	nmrvic.item.nMask&RVIM_LPARAM ? nmrvic.item.lParam:-1;
		lpItem->Param64 =	nmrvic.item.nMask&RVIM_PARAM64 ? nmrvic.item.Param64:-1;

		return TRUE;
	}

	return FALSE;
}

INT CReportCtrl::GetItemFromRow(INT iRow)
{
	if(m_dwStyle&RVS_OWNERDATA)
		return iRow;

	if(iRow == RVI_INVALID)
		return RVI_INVALID;

	return iRow == RVI_EDIT ? RVI_EDIT:m_arrayRows[iRow];
}

INT CReportCtrl::GetItemFromRow(INT iRow, ITEM& item)
{
	if(iRow == RVI_INVALID)
		return RVI_INVALID;

	INT iItem = iRow;
	if(!(m_dwStyle&RVS_OWNERDATA))
		iItem = GetItemFromRow(iItem);

	item = GetItemStruct(iItem, -1);
	return iItem;
}

INT CReportCtrl::GetRowFromItem(INT iItem)
{
	if(iItem == RVI_EDIT || m_dwStyle&RVS_OWNERDATA)
		return iItem;

	INT iRows = (INT)m_arrayRows.GetSize(), iRow;
	if(iRows > 0)
	{
		if(m_bUpdateItemMap)
		{
			for(iRow=0;iRow<iRows;iRow++)
				m_mapItemToRow[m_arrayRows[iRow]] = iRow;

			m_bUpdateItemMap = FALSE;
		}

		if(m_mapItemToRow.Lookup(iItem, iRow))
			return iRow;
	}

	return RVI_INVALID;
}

INT CReportCtrl::GetSubItemFromColumn(INT iColumn)
{
	if(iColumn < 0)
		return -1;

	HDITEM hdi;
	hdi.mask = HDI_WIDTH|HDI_LPARAM;
	m_wndHeader.GetItem(m_arrayColumns[iColumn], &hdi);

	return (INT)hdi.lParam;
}

INT CReportCtrl::GetColumnFromSubItem(INT iSubItem)
{
	if(iSubItem < 0)
		return -1;

	INT iColumn, iColumns = (INT)m_arrayColumns.GetSize();
	for(iColumn=0;iColumn<iColumns;iColumn++)
	{
		HDITEM hdi;
		hdi.mask = HDI_WIDTH|HDI_LPARAM;
		m_wndHeader.GetItem(m_arrayColumns[iColumn], &hdi);

		if(hdi.lParam == iSubItem)
			return iColumn;
	}

	return -1;
}

CReportCtrl::LPTREEITEM CReportCtrl::GetVisibleAncestor(LPTREEITEM lpti)
{
	BOOL bExit = FALSE;
	LPTREEITEM lptiAncestor = GetVisibleAncestor(lpti, &bExit);

	return bExit ? lptiAncestor:lpti;
}

CReportCtrl::LPTREEITEM CReportCtrl::GetVisibleAncestor(LPTREEITEM lpti, LPBOOL lpbExit)
{
	if(lpti->lptiParent == &m_tiRoot)
		return lpti;

	LPTREEITEM lptiAncestor = GetVisibleAncestor(lpti->lptiParent, lpbExit);

	if(*lpbExit == TRUE)
		return lptiAncestor;
	
	if(lptiAncestor->bOpen == FALSE)
	{
		*lpbExit = TRUE;
		return lptiAncestor;
	}

	return lpti;
}

BOOL CReportCtrl::SetSubItemWidthImpl(INT iSubItem, INT iWidth)
{
#ifdef DEBUG
	INT iSubItems = (INT)m_arraySubItems.GetSize();
	ASSERT(iSubItem <= iSubItems);
#endif

	try
	{
		iWidth = iWidth<0 ? m_iDefaultWidth:iWidth;

		SUBITEM subitem;
		subitem = m_arraySubItems.GetAt(iSubItem);

		iWidth = iWidth<subitem.iMinWidth ? subitem.iMinWidth:iWidth;
		iWidth = iWidth>subitem.iMaxWidth && subitem.iMaxWidth > 0 ? subitem.iMaxWidth:iWidth;

		INT iOldWidth = subitem.iWidth;

		subitem.iWidth = iWidth;
		m_arraySubItems.SetAt(iSubItem, subitem);

		INT iColumn = GetColumnFromSubItem(iSubItem);
		if(iColumn < 0)
			return TRUE;

		HDITEM hditem;
		hditem.mask = HDI_WIDTH;
		if(!m_wndHeader.GetItem(m_arrayColumns[iColumn], &hditem))
			return FALSE;

		m_iVirtualWidth += iWidth - iOldWidth;

		hditem.cxy = iWidth;
		m_wndHeader.SetItem(m_arrayColumns[iColumn], &hditem);

		return TRUE;
	}
	catch(CMemoryException* e)
	{
		e->Delete();
		return FALSE;
	}
}

void CReportCtrl::GetExpandedItemText(LPRVITEM /*lprvi*/)
{
}

void CReportCtrl::SetState(INT iRow, UINT nState, UINT nMask)
{
	ASSERT(iRow >= RVI_EDIT);

	if(iRow == RVI_EDIT)
	{
		m_itemEdit.nState &= ~nMask;
		m_itemEdit.nState |= nState&nMask;
	}
	else if(!(m_dwStyle&RVS_OWNERDATA))
	{
		m_arrayItems[m_arrayRows[iRow]].nState &= ~nMask;
		m_arrayItems[m_arrayRows[iRow]].nState |= nState&nMask;
	}

	FlushCache(iRow);
}

UINT CReportCtrl::GetState(INT iRow)
{
	RVITEM rvi;
	rvi.iItem = GetItemFromRow(iRow);
	rvi.nMask = RVIM_STATE;
	GetItem(&rvi);

	ASSERT(rvi.nMask&RVIM_STATE);	// Need to return state of the item
	return rvi.nState;
}

CReportCtrl::LPTREEITEM CReportCtrl::GetTreeFocus()
{
	INT iFocusItem = GetItemFromRow(m_iFocusRow);

	if(iFocusItem >= RVI_EDIT)
		SetState(m_iFocusRow, 0, RVIS_FOCUSED);

	m_iFocusRow = RVI_INVALID;
	if (iFocusItem < m_arrayItems.GetCount())
		return iFocusItem >= RVI_FIRST ? m_arrayItems[iFocusItem].lptiItem:NULL;
	return NULL;
}

void CReportCtrl::SetTreeFocus(LPTREEITEM lptiFocus)
{
	INT iFirst = GetScrollPos32(SB_VERT), iLast;

	if(lptiFocus != NULL)
	{
		m_iFocusRow = GetRowFromItem(lptiFocus->iItem);
		if( m_iFocusRow == RVI_INVALID )
		{
			ScrollWindow(SB_VERT, 0);
			return;
		}
	}
	else
		m_iFocusRow = RVI_EDIT;

	SetState(m_iFocusRow, RVIS_FOCUSED, RVIS_FOCUSED);

	GetVisibleRows(TRUE, &iFirst, &iLast);

	if(m_iFocusRow <= iFirst || iLast >= m_iFocusRow - 1)
		GetVisibleRows(TRUE, &iFirst, &iLast, TRUE);

	ScrollWindow(SB_VERT, iFirst > RVI_INVALID ? iFirst : 0);
}

void CReportCtrl::DeselectDescendents(LPTREEITEM lptiParent)
{
	LPTREEITEM lptiSibling = lptiParent->lptiChildren;

	while(lptiSibling != NULL)
	{
		if(lptiSibling->lptiChildren != NULL)
			DeselectDescendents(lptiSibling);

		POSITION pos = m_listSelection.Find(lptiSibling->iItem);
		if(pos != NULL)
		{
			m_arrayItems[lptiSibling->iItem].nState &= ~RVIS_SELECTED;
			m_listSelection.RemoveAt(pos);
		}

		lptiSibling = lptiSibling->lptiSibling;
	}
}

void CReportCtrl::ReorderColumns()
{
	LPINT lpi;
	INT iColumns = m_wndHeader.GetItemCount();

	if(iColumns)
	{
		m_iFocusColumn = m_iFocusColumn >= iColumns ? iColumns-1:m_iFocusColumn;
		INT iSubItem = m_arrayColumns.GetSize() ? GetSubItemFromColumn(m_iFocusColumn):-1;

		m_arrayColumns.SetSize(iColumns, 8);
		lpi = m_arrayColumns.GetData();
		if(!m_wndHeader.GetOrderArray(lpi, iColumns))
			return;

		if(iSubItem > 0)
			m_iFocusColumn = GetColumnFromSubItem(iSubItem);
		else
			m_iFocusColumn = -1;
	}
	else
	{
		m_arrayColumns.RemoveAll();
		m_iFocusColumn = -1;
	}

	if(m_lprsilc != NULL)
		m_lprsilc->UpdateList();

	m_bColumnsReordered = FALSE;
}

BOOL CReportCtrl::GetRowRect(INT iRow, INT iColumn, LPRECT lpRect, UINT nCode)
{
	// Select a visible sub-item
	if(iColumn < 0 && nCode != RVIR_BOUNDS)
		return FALSE;

	INT iFirst, iLast;

	if(iRow != RVI_EDIT)
	{
		iFirst = GetScrollPos32(SB_VERT);

		GetVisibleRows(FALSE, &iFirst, &iLast);

		// Select a visible item
		if(iFirst > iRow || iLast < iRow)
			return FALSE;

		*lpRect = m_rectReport;
		for(;iFirst<iRow;iFirst++)
			lpRect->top +=
				m_iDefaultHeight +
				(m_bPreview ? GetItemStruct(GetItemFromRow(iFirst), -1).nPreview:0);
	}
	else
		*lpRect = m_rectEdit;

	lpRect->bottom = lpRect->top + m_iDefaultHeight;

	INT iPos = GetScrollPos32(SB_HORZ);

	lpRect->left -= iPos;
	lpRect->right -= iPos;

	INT iLeft = lpRect->left;

	if(iColumn < 0)
		return TRUE;

	HDITEM hdi;
	hdi.mask = HDI_WIDTH|HDI_LPARAM;
	for(iFirst=0;iFirst<iColumn;iFirst++)
	{
		m_wndHeader.GetItem(m_arrayColumns[iFirst], &hdi);
		lpRect->left += hdi.cxy;
	}

	m_wndHeader.GetItem(m_arrayColumns[iColumn], &hdi);

	if(nCode == RVIR_TEXTNOCOLUMNS)
		lpRect->right = iLeft + m_iVirtualWidth;
	else
		lpRect->right = lpRect->left + hdi.cxy;

	return TRUE;
}

INT CReportCtrl::GetVisibleRows(BOOL bUnobstructed, LPINT lpiFirst, LPINT lpiLast, BOOL bReverse)
{
	INT iHeight = m_rectReport.Height();
	INT iRows = 0;

	if(!bReverse)
	{
		INT iRow;
		INT iMaxRows = m_iVirtualHeight;

		if(lpiFirst)
			iRow = *lpiFirst;
		else
			iRow = GetScrollPos32(SB_VERT);

		for(;iRow<iMaxRows&&iHeight>0;iRow++,iRows++)
		{
			iHeight -=
				m_iDefaultHeight +
				(m_bPreview ? GetItemStruct(GetItemFromRow(iRow), -1).nPreview:0);

			if(bUnobstructed && iHeight<=0)
				break;
		}

		if(lpiLast)
			*lpiLast = iRow-1;
	}
	else
	{
		ASSERT(lpiFirst);
		ASSERT(lpiLast);

		for(*lpiFirst=*lpiLast;*lpiFirst>=0&&iHeight>0;*lpiFirst-=1,iRows++)
		{
			iHeight -=
				m_iDefaultHeight +
				(m_bPreview ? GetItemStruct(GetItemFromRow(*lpiFirst), -1).nPreview:0);

			if(bUnobstructed && iHeight<=0)
				break;
		}

		*lpiFirst += 1;
	}

	return iRows;
}

void CReportCtrl::SelectRows(INT iFirst, INT iLast, BOOL bSelect, BOOL bKeepSelection, BOOL bInvert, BOOL bNotify)
{
	INT i;

	if(m_dwStyle&RVS_SINGLESELECT)
	{
		iLast = iFirst;
		bKeepSelection = FALSE;
	}

	if(m_iFocusRow > RVI_INVALID && iFirst != m_iFocusRow)
	{
		SetState(m_iFocusRow, 0, RVIS_FOCUSED);
		RedrawRows(m_iFocusRow);
	}

	m_iFocusRow = iFirst;
	SetState(iFirst, RVIS_FOCUSED, RVIS_FOCUSED);

	if(iFirst > iLast)
	{
		iLast += iFirst;
		iFirst = iLast - iFirst;
		iLast -= iFirst;
	}

	if(bSelect)
	{
		for(i=iFirst;i<=iLast;i++)
		{
			INT iItem = GetItemFromRow(i);
			UINT nOldState, nNewState;

			nOldState = nNewState = GetState(i);

			if(bInvert)
				nNewState ^= RVIS_SELECTED;
			else
				nNewState |= RVIS_SELECTED;

			if(nNewState != nOldState)
			{
				if(bNotify && Notify(RVN_SELECTIONCHANGING, iItem, -1, nNewState))
					continue;

				POSITION pos = m_listSelection.Find(iItem);
				if(nNewState&RVIS_SELECTED)
				{
					if(pos == NULL)
						m_listSelection.AddTail(iItem);
				}
				else
					m_listSelection.RemoveAt(pos);

				SetState(i, nNewState, RVIS_SELECTED);

				if(bNotify)
					Notify(RVN_SELECTIONCHANGED, iItem, -1, nNewState);
			}
		}

		RedrawRows(iFirst, iLast);
	}
	else
		RedrawRows(iFirst);

	if(!bKeepSelection && bSelect)
	{
		CList<INT, INT> list;

		POSITION pos = m_listSelection.GetHeadPosition();
		while(pos != NULL)
		{
			INT iItem = m_listSelection.GetAt(pos);
			INT i = GetRowFromItem(iItem);
			UINT nOldState, nNewState;

			nOldState = nNewState = GetState(i);

			if((i<iFirst || i>iLast) && nOldState&RVIS_SELECTED)
			{
				nNewState &= ~RVIS_SELECTED;

				if(nNewState != nOldState)
				{
					if(bNotify && Notify(RVN_SELECTIONCHANGING, iItem, -1, nNewState))
						continue;

					SetState(i, nNewState, RVIS_SELECTED);

					if(bNotify)
						Notify(RVN_SELECTIONCHANGED, iItem, -1, nNewState);

					RedrawRows(i);
				}
			}
			else
				list.AddTail(iItem);

			m_listSelection.GetNext(pos);
		}

		m_listSelection.RemoveAll();
		m_listSelection.AddTail(&list);
	}

	UpdateWindow();
}

INT CReportCtrl::GetScrollPos32(INT iBar, BOOL bGetTrackPos)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);

	INT iMax = iBar==SB_HORZ ? m_iVirtualWidth:m_iVirtualHeight;

	if(bGetTrackPos)
	{
		if(GetScrollInfo(iBar, &si, SIF_TRACKPOS))
			return min(si.nTrackPos, iMax);
	}
	else
	{
		if(GetScrollInfo(iBar, &si, SIF_POS))
			return min(si.nPos, iMax);
	}

	return 0;
}

BOOL CReportCtrl::SetScrollPos32(INT iBar, INT iPos, BOOL bRedraw)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_POS;
    si.nPos   = iPos;

    return SetScrollInfo(iBar, &si, bRedraw);
}

void CReportCtrl::ScrollWindow(INT iBar, INT iPos)
{
	if(m_rectReport.Width()<=0 || m_rectReport.Height()<=0)
		return;

	m_wndTip.Hide();

	if(iBar == SB_HORZ)
	{
		SCROLLINFO si;
		INT iWidth = m_rectReport.Width();

		si.cbSize = sizeof(SCROLLINFO);
		si.fMask  = SIF_PAGE|SIF_RANGE|SIF_POS;
		si.nPage = iWidth;
		si.nMin = 0;
		si.nMax = iWidth<m_iVirtualWidth ? m_iVirtualWidth-1:0;
		si.nPos = iPos;
		SetScrollInfo(SB_HORZ, &si, TRUE);

		INT x = GetScrollPos32(SB_HORZ);
		INT cx = iWidth<m_iVirtualWidth ? m_iVirtualWidth:iWidth;

		VERIFY(m_wndHeader.SetWindowPos(&wndTop, -x, m_rectHeader.top, cx, m_rectHeader.Height(), m_dwStyle&RVS_NOHEADER ? SWP_HIDEWINDOW:SWP_SHOWWINDOW));
	}

	if(iBar == SB_VERT)
	{
		ASSERT(iPos >= 0 && iPos<=m_iVirtualHeight);

		INT iFirst = iPos, iLast;
		GetVisibleRows(TRUE, &iFirst, &iLast);

		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask  = SIF_PAGE|SIF_RANGE|SIF_POS;
		si.nPage = (iLast - iFirst + 1);
		si.nMin = 0;
		si.nMax = (iLast - iFirst + 1)<m_iVirtualHeight ? m_iVirtualHeight-1:0;
		si.nPos = iFirst;
		SetScrollInfo(SB_VERT, &si, TRUE);
	}

	if(m_dwStyle&RVS_SHOWEDITROW)
		InvalidateRect(m_rectEdit);

	InvalidateRect(m_rectReport);

	//
	// May solve slow redrawing problems
	//
	// UpdateWindow();
}

void CReportCtrl::EnsureVisibleColumn(INT iColumn)
{
	if(iColumn > -1)
	{
		INT iWidth = m_rectReport.Width();
		INT iPos = GetScrollPos(SB_HORZ);
		INT iOffset = 0;

		HDITEM hdi;
		hdi.mask = HDI_WIDTH|HDI_LPARAM;

		INT i = 0;
		for(i=0;i<iColumn;i++)
		{
			m_wndHeader.GetItem(m_arrayColumns[i], &hdi);
			iOffset += hdi.cxy;
		}

		m_wndHeader.GetItem(m_arrayColumns[i], &hdi);

		if(iOffset + hdi.cxy > iPos+iWidth || iOffset < iPos)
			ScrollWindow(SB_HORZ, iOffset);
	}
}

void CReportCtrl::SetIndentFirstColumn(INT iIndent)
{
	UnindentFirstColumn();
	m_iIndentColumn = iIndent;
	IndentFirstColumn();
}

void CReportCtrl::IndentFirstColumn()
{
	if(m_arrayColumns.GetSize() > 0 && m_iIndentColumn > 0)
	{
		ASSERT(m_bIndentColumn == FALSE);

		INT iSubItem = GetSubItemFromColumn(0);
		ASSERT(iSubItem >= 0);

		SUBITEM& subitem = m_arraySubItems[iSubItem];
		subitem.iMinWidth += m_iIndentColumn;
		subitem.iMaxWidth += subitem.iMaxWidth>0 ? m_iIndentColumn:0;

		SetSubItemWidthImpl(iSubItem, subitem.iWidth+m_iIndentColumn);

		m_wndHeader.SetMinMax(
			m_wndHeader.OrderToIndex(0),
			subitem.iMinWidth,
			subitem.iMaxWidth
		);

		m_bIndentColumn = TRUE;
	}
}

void CReportCtrl::UnindentFirstColumn()
{
	if(m_bIndentColumn && m_iIndentColumn>=0)
	{
		INT iSubItem = GetSubItemFromColumn(0);
		ASSERT(iSubItem >= 0);

		SUBITEM& subitem = m_arraySubItems[iSubItem];
		subitem.iMaxWidth -= subitem.iMaxWidth>0 ? m_iIndentColumn:0;
		subitem.iMinWidth -= m_iIndentColumn;

		SetSubItemWidthImpl(
			iSubItem,
			(subitem.iMinWidth <= 0 && subitem.iWidth-m_iIndentColumn < 20)
			?
			20:subitem.iWidth-m_iIndentColumn
		);

		m_iIndentColumnPending =
			m_iIndentColumnPending >= 0
			?
			m_iIndentColumnPending:m_wndHeader.OrderToIndex(0);

		m_wndHeader.SetMinMax(
			m_iIndentColumnPending,
			subitem.iMinWidth,
			subitem.iMaxWidth
		);
	}

	m_bIndentColumn = FALSE;
	m_iIndentColumnPending = -1;
}

void CReportCtrl::SetRedraw(BOOL bRedraw)
{
	CWnd::SetRedraw(bRedraw);
	m_bRedrawFlag = bRedraw;

	if(bRedraw == TRUE)
		UpdateWindow();
}

void CReportCtrl::RedrawRows(INT iFirst, INT iLast, BOOL bUpdate)
{
	CRect rect;

	if(iFirst == iLast || iLast == RVI_INVALID)
	{
		if(iFirst == RVI_INVALID && iLast == RVI_INVALID)
			return;

		GetRowRect(iFirst, -1, rect);

		if(m_bPreview)
			rect.bottom += GetItemStruct(GetItemFromRow(iFirst), -1, RVIM_PREVIEW).nPreview;

		rect.OffsetRect(GetScrollPos32(SB_HORZ), 0);
	}
	else
		rect = m_rectReport;

	InvalidateRect(rect);

	if(bUpdate)
		UpdateWindow();
}

void CReportCtrl::SelectionToSortedArray(CArray<INT, INT>& arrayRows)
{
	INT iPos = 0, iSize;
	POSITION pos = m_listSelection.GetHeadPosition();
	while( pos != NULL )
	{
		INT iRow = GetRowFromItem(m_listSelection.GetAt(pos));

		iSize = (INT)arrayRows.GetSize();
		if(iSize)
		{
			if(arrayRows[iPos] <= iRow)
			{
				while(iPos<iSize && arrayRows[iPos] <= iRow)
					iPos++;
			}
			else
			{
				while( iPos>0 && arrayRows[iPos-1] > iRow)
					iPos--;
			}
		}

		arrayRows.InsertAt(iPos, iRow);

		m_listSelection.GetNext(pos);
	}
}

void CReportCtrl::DrawCtrl(CDC* pDC)
{
	CRect rectClip;
	if (pDC->GetClipBox(&rectClip) == ERROR)
		return;

	INT iHPos = GetScrollPos32(SB_HORZ);
	INT iVPos = GetScrollPos32(SB_VERT);

	DrawBkgnd(pDC, rectClip, m_dwStyle&WS_DISABLED ? m_cr3DFace:m_crBackground);

	CPen pen(m_iGridStyle, 1, m_crGrid);
	CPen* pPen = pDC->SelectObject(&pen);

	CFont* pFont = pDC->SelectObject(GetFont());

	pDC->SetBkMode(TRANSPARENT);

	CRect rect;
	CRect rectRow(m_rectReport.left-iHPos, m_rectReport.top, m_rectReport.left-iHPos+m_iVirtualWidth, m_rectReport.top);

	GetClientRect( rect );
	rect.right = rect.left + m_iDefaultHeight;
	rect.OffsetRect( -iHPos, 0 );
	if(
		m_iIndentColumn > 0 && m_bIndentGrey &&
		rectClip.left < rect.right && rect.right > 0
	)
		pDC->FillSolidRect( rect, m_cr3DFace );
	
	TCHAR szText[REPORTCTRL_MAX_TEXT];
	RVITEM rvi;
	rvi.nMask = RVIM_TEXT;
	rvi.lpszText = szText;
	rvi.iTextMax = REPORTCTRL_MAX_TEXT;

	NMRVDRAWPREVIEW nmrvdp;
	nmrvdp.hdr.hwndFrom = GetSafeHwnd();
	nmrvdp.hdr.idFrom = GetDlgCtrlID();
	nmrvdp.hdr.code = RVN_ITEMDRAWPREVIEW;
	nmrvdp.hDC = pDC->m_hDC;

	INT iRows = m_iVirtualHeight;
	INT iColumns = (INT)m_arrayColumns.GetSize();

	if(m_bColumnsReordered)
	{
		UnindentFirstColumn();
		ReorderColumns();
		IndentFirstColumn();
	}

	if(m_dwStyle&RVS_SHOWEDITROW)
	{
		GetClientRect(rect);
		rect.bottom = m_rectEdit.bottom;
		rect.top = rect.bottom - GetSystemMetrics(SM_CYFIXEDFRAME)*2;
		pDC->FillSolidRect(rect, m_cr3DFace);
		pDC->Draw3dRect(rect, m_cr3DHiLight, m_cr3DDkShadow);

		rvi.iItem = RVI_EDIT;
		rvi.nMask = RVIM_TEXT;
		GetItem(&rvi);

		rect = m_rectEdit;
		rect.bottom -= GetSystemMetrics(SM_CYFIXEDFRAME)*2;
		rect.left -= iHPos;

		DrawRow(pDC, rect, rectClip, RVI_EDIT, &rvi, FALSE);

		if(m_bFocus && m_hEditWnd == NULL && m_iFocusRow == RVI_EDIT && m_iFocusColumn < 0)
		{
			if(m_dwStyle&(RVS_SHOWHGRID|RVS_SHOWVGRID))
				rect.DeflateRect(1, 0, 1, 0);

			pDC->SetBkColor(m_dwStyle&WS_DISABLED ? m_cr3DFace:m_crBackground);
			pDC->SetTextColor(m_crText);
			pDC->DrawFocusRect(rect);
		}
	}

	if(!iRows)
	{
		rectRow.top += 2;
		rectRow.bottom = rectRow.top + m_iDefaultHeight;

		GetClientRect(rect);
		rect.top = rectRow.top;
		rect.bottom = rectRow.bottom;

		if(rectRow.Width() < rect.Width())
			rect = rectRow;

		if(!m_strNoItems.IsEmpty())
		{
			pDC->SetTextColor(m_crText);
			pDC->DrawText(m_strNoItems, rect, DT_NOPREFIX|DT_CENTER|DT_END_ELLIPSIS);
		}
	}
	else
	{
		INT iRow = iVPos;
		while(iRow<iRows && rectRow.top<rectClip.bottom)
		{
			rvi.iItem = GetItemFromRow(iRow);
			rvi.nMask = RVIM_TEXT;
			rvi.lpszText = szText;
			VERIFY(GetItem(&rvi));

			rectRow.left = m_rectReport.left-iHPos;
			rectRow.bottom = rectRow.top + m_iDefaultHeight + (m_bPreview ? rvi.nPreview:0);
			if(rectRow.bottom >= rectClip.top)
			{
				if(rvi.iIndent >= 0)
				{
					rectRow.left += rvi.iIndent*m_iDefaultHeight;
					rectRow.right = max(rectRow.left, rectRow.right);
				}

				DrawRow(pDC, rectRow, rectClip, iRow, &rvi, iRow&1);

				if(m_bPreview && rvi.nMask&RVIM_PREVIEW && rvi.nPreview)
				{
					nmrvdp.iItem = rvi.iItem;
					nmrvdp.nState = rvi.nState;
					nmrvdp.rect = rectRow;
					nmrvdp.rect.top += m_iDefaultHeight;
					nmrvdp.lParam = rvi.lParam;

					Notify((LPNMREPORTVIEW)&nmrvdp);
				}

				if(m_dwStyle&RVS_SHOWHGRID)
				{
					pDC->MoveTo(rectRow.left, rectRow.bottom-1);
					pDC->LineTo(rectRow.right, rectRow.bottom-1);
				}

				if(m_bFocus && m_hEditWnd == NULL && m_iFocusRow == iRow && (!(m_dwStyle&RVS_FOCUSSUBITEMS) || m_iFocusColumn < 0))
				{
					if(m_dwStyle&(RVS_SHOWHGRID|RVS_SHOWVGRID))
						rectRow.DeflateRect(1, 0, 1, 1);

					pDC->SetBkColor(m_dwStyle&WS_DISABLED ? m_cr3DFace:m_crBackground);
					pDC->SetTextColor(m_crText);
					pDC->DrawFocusRect(rectRow);

					if(m_dwStyle&(RVS_SHOWHGRID|RVS_SHOWVGRID))
						rectRow.InflateRect(1, 0, 1, 1);
				}
			}

			rectRow.top = rectRow.bottom;
			iRow++;
		}

		rectRow.top = m_rectReport.top;
		rectRow.bottom =
			m_rectReport.bottom<rectRow.bottom
			?
			m_rectReport.bottom:rectRow.bottom;

		if(m_dwStyle&(RVS_SHOWHGRID|RVS_SHOWVGRID))
		{
			if(m_dwStyle&RVS_SHOWHGRIDEX)
			{
				pDC->MoveTo(rectRow.left, m_rectReport.top);
				pDC->LineTo(rectRow.left, m_rectReport.bottom);

				pDC->MoveTo(rectRow.right-1, m_rectReport.top);
				pDC->LineTo(rectRow.right-1, m_rectReport.bottom);
			}
			else
			{
				pDC->MoveTo(rectRow.left, rectRow.top);
				pDC->LineTo(rectRow.left, rectRow.bottom);

				pDC->MoveTo(rectRow.right-1, rectRow.top);
				pDC->LineTo(rectRow.right-1, rectRow.bottom);
			}
		}

		if(m_dwStyle&RVS_SHOWHGRID && m_dwStyle&RVS_SHOWHGRIDEX && iRow>=iRows)
		{
			while(rectRow.bottom<=rectClip.bottom)
			{
				pDC->MoveTo(rectRow.left, rectRow.bottom-1);
				pDC->LineTo(rectRow.right, rectRow.bottom-1);

				rectRow.bottom += m_iDefaultHeight;
			}
		}

		if(m_dwStyle&RVS_SHOWVGRID)
		{
			for(INT iColumn=0;iColumn<iColumns&&rectRow.left<rectClip.right;iColumn++)
			{
				HDITEM hdi;
				hdi.mask = HDI_WIDTH;
				m_wndHeader.GetItem(m_arrayColumns[iColumn], &hdi);

				rectRow.left += hdi.cxy;

				pDC->MoveTo(rectRow.left-1, rectRow.top);
				pDC->LineTo(rectRow.left-1, rectRow.bottom);
			}
		}
	}

	pDC->SelectObject(pFont);
	pDC->SelectObject(pPen);

	pen.DeleteObject();
}

void CReportCtrl::DrawRow(CDC* pDC, CRect rectRow, CRect rectClip, INT iRow, LPRVITEM lprvi, BOOL bAlternate)
{
	INT iColumns = (INT)m_arrayColumns.GetSize();

	// You have to insert at least 1 color to use color alternate style.
	ASSERT(!(m_dwStyle&RVS_SHOWCOLORALTERNATE && !m_arrayColors.GetSize()));

	ASSERT(lprvi->lpszText != NULL);
	LPTSTR lpszText = lprvi->lpszText;

	COLORREF crItem, crBackground = m_dwStyle&WS_DISABLED ? m_cr3DFace:m_crBackground;
	crItem = (m_dwStyle&RVS_SHOWCOLORALTERNATE && bAlternate) ? m_arrayColors[0]:crBackground;
	crItem = lprvi->iBkColor>=0 ? m_arrayColors[lprvi->iBkColor]:crItem;
	if(lprvi->nState&RVIS_SELECTED)
	{
		if(m_bFocus)
			crItem = m_crBkSelected;
		else
			if(m_dwStyle&RVS_SHOWSELALWAYS)
				crItem = m_crBkSelectedNoFocus;
	}

	if(m_bitmap.m_hObject == NULL || crItem != crBackground || iRow == RVI_EDIT)
	{
		pDC->FillSolidRect(rectRow, crItem);
		pDC->SetBkColor(crItem);
	}

	if(lprvi->nState&RVIS_BOLD)
		pDC->SelectObject(&m_fontBold);

	CRect rectItem(rectRow.left, rectRow.top, rectRow.left, rectRow.top+m_iDefaultHeight);
	for(INT iColumn=0;iColumn<iColumns&&rectItem.left<rectClip.right;iColumn++)
	{
		HDITEM hdi;
		hdi.mask = HDI_WIDTH|HDI_LPARAM;
		m_wndHeader.GetItem(m_arrayColumns[iColumn], &hdi);

		lprvi->iSubItem = (INT)hdi.lParam;

		rectItem.right = rectItem.left + hdi.cxy -
			(
				iColumn == 0 && lprvi->iIndent >= 0
				?
				lprvi->iIndent*m_iDefaultHeight
				:
				0
			);

		if(rectItem.right > rectClip.left)
		{
			lprvi->nMask = RVIM_TEXT;
			lprvi->lpszText = lpszText;
			VERIFY(GetItem(lprvi));

			COLORREF crText = lprvi->nMask&RVIM_TEXTCOLOR ? m_arrayColors[lprvi->iTextColor]:m_crText;
			if(lprvi->nState&RVIS_SELECTED)
			{
				if(m_bFocus)
					crText = (m_dwStyle&RVS_SHOWCOLORALWAYS && lprvi->nMask&RVIM_TEXTCOLOR) ? crText:m_crTextSelected;
				else
					if(m_dwStyle&RVS_SHOWSELALWAYS)
						crText = lprvi->nMask&RVIM_TEXTCOLOR && m_dwStyle&RVS_SHOWCOLORALWAYS ? crText:m_crTextSelectedNoFocus;
			}

			if(m_iFocusRow == iRow && m_iFocusColumn == iColumn && (m_iFocusRow == RVI_EDIT || m_dwStyle&RVS_FOCUSSUBITEMS))
			{
				pDC->FillSolidRect(rectItem, crBackground);
				crText = m_crText;
			}

			pDC->SetTextColor(crText);

			if(iColumn == 0 && lprvi->iIndent >= 0)
			{
				if(lprvi->nState&RVIS_TREEMASK)
				{
					rectItem.left -= m_iDefaultHeight;
					DrawTreeBox(pDC, rectItem, lprvi->nState&RVIS_TREEOPENED);
					rectItem.left += m_iDefaultHeight;
				}

				if(!(m_dwStyle&RVS_OWNERDATA))
				{
					LPTREEITEM lpti = m_arrayItems[lprvi->iItem].lptiItem;
					if(lpti != NULL )
					{
						if(lpti->iSubItem >= 0)
						{
							rectItem.right = rectRow.right;

							lprvi->iSubItem = lpti->iSubItem;
							lprvi->nMask = RVIM_TEXT;
							lprvi->lpszText = lpszText;
							VERIFY(GetItem(lprvi));

							rectItem.DeflateRect(m_iSpacing, 0);
							DrawItem(pDC, rectItem, lprvi);
							rectItem.InflateRect(m_iSpacing, 0);
							break;
						}
					}
				}
			}

			rectItem.DeflateRect(m_iSpacing, 0);
			DrawItem(pDC, rectItem, lprvi);
			rectItem.InflateRect(m_iSpacing, 0);

			if(m_bFocus && m_hEditWnd == NULL && m_iFocusRow == iRow && m_iFocusColumn == iColumn && (m_iFocusRow == RVI_EDIT || m_dwStyle&RVS_FOCUSSUBITEMS))
			{
				if(m_dwStyle&(RVS_SHOWHGRID|RVS_SHOWVGRID))
					rectItem.DeflateRect(1, 0, 1, 1);

				pDC->SetBkColor(crBackground);
				pDC->SetTextColor(m_crText);
				pDC->DrawFocusRect(rectItem);

				if(m_dwStyle&(RVS_SHOWHGRID|RVS_SHOWVGRID))
					rectItem.InflateRect(1, 0, 1, 1);
			}
		}
		rectItem.left = rectItem.right;
	}

	if(lprvi->nState&RVIS_BOLD)
		pDC->SelectObject(GetFont());
}

void CReportCtrl::DrawItem(CDC* pDC, CRect rect, LPRVITEM lprvi)
{
	BOOL bResult = FALSE;

	if(lprvi->nState&RVIS_OWNERDRAW)
	{
		NMRVITEMDRAW nmrvid;
		nmrvid.hdr.hwndFrom = GetSafeHwnd();
		nmrvid.hdr.idFrom = GetDlgCtrlID();
		nmrvid.hdr.code = RVN_ITEMDRAW;

		nmrvid.lprvi = lprvi;

		nmrvid.hDC = pDC->GetSafeHdc();
		nmrvid.rect = rect;

		bResult = Notify((LPNMREPORTVIEW)&nmrvid);
	}

	if( !bResult )
	{
		INT iWidth = 0;

		rect.left += ((iWidth = DrawImage(pDC, rect, lprvi))!=0) ? iWidth+m_iSpacing : 0;
		if(lprvi->nMask&RVIM_IMAGE && !iWidth)
			return;
		rect.left += ((iWidth = DrawCheck(pDC, rect, lprvi))!=0) ? iWidth+m_iSpacing : 0;
		if(lprvi->nMask&RVIM_CHECK && !iWidth)
			return;
		DrawText(pDC, rect, lprvi);
	}
}

INT CReportCtrl::DrawTreeBox(CDC* pDC, CRect rectBox, BOOL bOpen)
{
	if(rectBox.right < rectBox.left)
		return rectBox.right;

	rectBox.right = rectBox.left + rectBox.Height();
	CPoint point = rectBox.CenterPoint();

	CRect rect;
	rect.SetRect(point, point);
	rect.InflateRect(4, 4, 5, 5);

	pDC->FillSolidRect(rect, m_cr3DShadow);
	rect.DeflateRect(1, 1);
	pDC->FillSolidRect(rect, m_crBackground);

	CPen pen(PS_SOLID, 1, m_crText);
	CPen* pPen = pDC->SelectObject(&pen);

	pDC->MoveTo(point.x-2, point.y);
	pDC->LineTo(point.x+3, point.y);

	if(!bOpen)
	{
		pDC->MoveTo(point.x, point.y-2);
		pDC->LineTo(point.x, point.y+3);
	}

	pDC->SelectObject(pPen);
	return rectBox.right;
}

INT CReportCtrl::DrawImage(CDC* pDC, CRect rect, LPRVITEM lprvi)
{
	CImageList* pImageList = GetImageList();
	INT iWidth = 0;

	if(pImageList && (lprvi->nMask&RVIM_IMAGE))
	{
		ASSERT(pImageList);
		ASSERT(lprvi->iImage>=0 && lprvi->iImage<pImageList->GetImageCount());

		if(rect.Width()>0)
		{
			POINT point;
			UINT nStyle = ILD_NORMAL;
			COLORREF crColor = 0;

			point.y = rect.CenterPoint().y - (m_sizeImage.cy>>1) + 1;
			point.x = rect.left;

			switch( m_nBlendStyle )
			{
			case RVP_BLEND_FOCUS:
				nStyle = lprvi->nState&RVIS_FOCUSED ? ILD_FOCUS:ILD_NORMAL;
				crColor = m_crBackground;
				break;

			case RVP_BLEND_SELECT:
				nStyle = lprvi->nState&RVIS_SELECTED ? ILD_SELECTED:ILD_NORMAL;
				crColor = pDC->GetBkColor();
				break;

			case RVP_BLEND_ALL:
				nStyle = lprvi->nState&RVIS_FOCUSED ? ILD_FOCUS:ILD_NORMAL;
				crColor = m_crBackground;
				if( lprvi->nState&RVIS_SELECTED )
				{
					nStyle = lprvi->nState&RVIS_SELECTED ? ILD_SELECTED:ILD_NORMAL;
					crColor = pDC->GetBkColor();
				}
				break;

			default:
				if( lprvi->nState&RVIS_BLENDMASK )
				{
					nStyle = lprvi->nState&RVIS_BLEND25 ? ILD_BLEND25:ILD_BLEND50;
					crColor = m_crBlendColor;
				}
				break;
			}

			SIZE size;
			size.cx = rect.Width()<m_sizeImage.cx ? rect.Width():m_sizeImage.cx;
			size.cy = m_sizeImage.cy;
			pImageList->DrawIndirect(
				pDC, lprvi->iImage, point, size, CPoint(0, 0),
				nStyle, SRCCOPY, CLR_NONE, crColor
			);

			if(lprvi->nMask&RVIM_OVERLAY)
				pImageList->DrawIndirect(
					pDC, lprvi->iOverlay, point, size, CPoint(0, 0),
					nStyle, SRCCOPY, CLR_NONE, crColor
				);

			iWidth = m_sizeImage.cx;
		}
	}
	else
		iWidth = m_arraySubItems[lprvi->iSubItem].nFormat&RVCF_SUBITEM_IMAGE ? m_sizeImage.cx:0;

	return iWidth;
}

INT CReportCtrl::DrawCheck(CDC* pDC, CRect rect, LPRVITEM lprvi)
{
	INT iWidth = 0;

	if(lprvi->nMask&RVIM_CHECK)
	{
		if(rect.Width()>m_sizeCheck.cx)
		{
			if( m_pCheckList == NULL )
			{
				rect.top = rect.CenterPoint().y - (m_sizeCheck.cy>>1);
				rect.left = rect.left;
				rect.bottom = rect.top + m_sizeCheck.cx;
				rect.right = rect.left + m_sizeCheck.cy;

				UINT nState = m_nFrameControlStyle;
				BOOL bDisable = FALSE;

				switch(lprvi->iCheck)
				{
				case 2: bDisable = TRUE; // Intentional
				case 0:	nState |= DFCS_BUTTONCHECK; break;
				case 3: bDisable = TRUE; // Intentional
				case 1:	nState |= DFCS_BUTTONCHECK|DFCS_CHECKED; break;

				case 6: bDisable = TRUE; // Intentional
				case 4:	nState |= DFCS_BUTTONRADIO; break;
				case 7: bDisable = TRUE; // Intentional
				case 5:	nState |= DFCS_BUTTONRADIO|DFCS_CHECKED; break;
				}

				pDC->DrawFrameControl(rect, DFC_BUTTON, nState);

				CPen pen(PS_SOLID, 2, RGB(0xFF, 0, 0));
				CPen* pPen = pDC->SelectObject(&pen);

				if(bDisable == TRUE)
				{
					rect.DeflateRect(1, 1);
					if( lprvi->iCheck >= 4 )
						rect.OffsetRect(1, 0);

					pDC->MoveTo(rect.left, rect.top);
					pDC->LineTo(rect.right-1, rect.bottom-1);
					pDC->MoveTo(rect.right-1, rect.top);
					pDC->LineTo(rect.left, rect.bottom-1);

					pDC->SelectObject(pPen);
				}
			}
			else
			{
				POINT point;

				point.y = rect.CenterPoint().y - (m_sizeCheck.cy>>1) + 1;
				point.x = rect.left;

				SIZE size;
				size.cx = rect.Width()<m_sizeCheck.cx ? rect.Width():m_sizeCheck.cx;
				size.cy = m_sizeCheck.cy;
				m_pCheckList->DrawIndirect(pDC, lprvi->iCheck, point, size, CPoint(0, 0));
			}

			iWidth = m_sizeCheck.cx;
		}
	}
	else
		iWidth = m_arraySubItems[lprvi->iSubItem].nFormat&RVCF_SUBITEM_CHECK ? m_sizeCheck.cx:0;

	return iWidth;
}

INT CReportCtrl::DrawText(CDC* pDC, CRect rect, LPRVITEM lprvi)
{
	CSize size;

	if(rect.Width()>0 && lprvi->nMask&RVIM_TEXT)
	{
		size = pDC->GetTextExtent(lprvi->lpszText);

		switch(m_arraySubItems[lprvi->iSubItem].nFormat&HDF_JUSTIFYMASK)
		{
		case HDF_LEFT:
		case HDF_LEFT|HDF_RTLREADING:
			pDC->DrawText(lprvi->lpszText, -1, rect, DT_NOPREFIX|DT_LEFT|DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);
			break;
		case HDF_CENTER:
		case HDF_CENTER|HDF_RTLREADING:
			pDC->DrawText(lprvi->lpszText, -1, rect, DT_NOPREFIX|DT_CENTER|DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);
			break;
		case HDF_RIGHT:
		case HDF_RIGHT|HDF_RTLREADING:
			pDC->DrawText(lprvi->lpszText, -1, rect, DT_NOPREFIX|DT_RIGHT|DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);
			break;
		}
	}

	size.cx = rect.Width()>size.cx ? size.cx:rect.Width();
	return size.cx>0 ? size.cx:0;
}

BOOL CReportCtrl::DrawBkgnd(CDC* pDC, CRect rect, COLORREF crBackground)
{
	if(m_bitmap.m_hObject != NULL)
	{
		CDC dc;
		CRgn rgnBitmap;
		CRect rectBitmap;

		dc.CreateCompatibleDC(pDC);
		dc.SelectObject(&m_bitmap);

		rect.IntersectRect(rect, m_rectReport);

		INT iHPos = GetScrollPos32(SB_HORZ);
		rectBitmap.top = rect.top - rect.top%m_sizeBitmap.cy;
		rectBitmap.bottom = rect.top + m_sizeBitmap.cy;
		rectBitmap.left = rect.left - (rect.left+iHPos)%m_sizeBitmap.cx;
		rectBitmap.right = rect.left + m_sizeBitmap.cx;

		INT x, y;
		for(x = rectBitmap.left; x < rect.right; x += m_sizeBitmap.cx)
			for(y = rectBitmap.top; y < rect.bottom; y += m_sizeBitmap.cy)
				pDC->BitBlt(x, y, m_sizeBitmap.cx, m_sizeBitmap.cy, &dc, 0, 0, SRCCOPY);

		return TRUE;
	}

	pDC->FillSolidRect(rect, crBackground);
	return FALSE;
}

BOOL CReportCtrl::BeginEdit(INT iRow, INT iColumn, UINT nKey)
{
	if(iRow == RVI_INVALID || iColumn < 0)
		return FALSE;

	if(iRow == RVI_EDIT && !(m_dwStyle&RVS_SHOWEDITROW))
		return FALSE;

	TCHAR szText[REPORTCTRL_MAX_TEXT];

	m_iEditItem = GetItemFromRow(iRow);
	m_iEditSubItem = GetSubItemFromColumn(iColumn);

	// Make sure that the item to edit actually exists
	ASSERT(m_iEditItem != RVI_INVALID && m_iEditSubItem != -1);

	RVITEM rvi;
	rvi.iItem = m_iEditItem;
	rvi.iSubItem = m_iEditSubItem;
	rvi.nMask = RVIM_TEXT|RVIM_STATE|RVIM_LPARAM;
	rvi.lpszText = szText;
	rvi.iTextMax = REPORTCTRL_MAX_TEXT;
	GetItem(&rvi);

	if(rvi.nState&RVIS_READONLY)
		return FALSE;

	NMRVITEMEDIT nmrvie;
	memset(&nmrvie, 0, sizeof(nmrvie));

	nmrvie.hdr.hwndFrom = GetSafeHwnd();
	nmrvie.hdr.idFrom = GetDlgCtrlID();
	nmrvie.hdr.code = RVN_BEGINITEMEDIT;

	nmrvie.iItem = m_iEditItem;
	nmrvie.iSubItem = m_iEditSubItem;

	nmrvie.hWnd = NULL;
	if(!GetItemRect(m_iEditItem, m_iEditSubItem, &nmrvie.rect))
		return FALSE;

	nmrvie.nKey = nKey;
	if(rvi.nMask&RVIM_TEXT)
		nmrvie.lpszText = szText;

	nmrvie.lParam = rvi.lParam;

	BOOL bResult = FALSE;
	if(GetParent() != NULL)
		bResult = Notify((LPNMREPORTVIEW)&nmrvie);

	if(!bResult)
	{
		if(!(rvi.nMask&RVIM_TEXT))
			return FALSE;

		if(!GetItemRect(m_iEditItem, m_iEditSubItem, &nmrvie.rect, RVIR_TEXT))
			return FALSE;

		nmrvie.rect.top += 2;

		CReportEditCtrl* pWnd= new CReportEditCtrl(m_iEditItem, m_iEditSubItem, nmrvie.bButton);
		if(pWnd == NULL)
			return FALSE;

		if(!pWnd->Create(
			WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
			nmrvie.rect,
			this,
			0)
		) {
			delete pWnd;
			return FALSE;
		}

		pWnd->SetFont(
			rvi.nMask&RVIM_STATE && rvi.nState&RVIS_BOLD
			?
			&m_fontBold:&m_font
		);
		pWnd->SetWindowText(szText);
		pWnd->BeginEdit(nKey);

		m_hEditWnd = pWnd->GetSafeHwnd();
	}
	else
	{
		if(nmrvie.hWnd == NULL)
			return FALSE;

		m_hEditWnd = nmrvie.hWnd;
	}

	if(m_hEditWnd != NULL)
		::SetFocus(m_hEditWnd);

	return TRUE;
}

void CReportCtrl::EndEdit(BOOL bUpdate, LPNMRVITEMEDIT lpnmrvie)
{
	if(!(m_dwStyle&RVS_OWNERDATA) && bUpdate)
	{
		if(lpnmrvie == NULL)
		{
			TCHAR szText[REPORTCTRL_MAX_TEXT];

			INT i = ::GetWindowText(m_hEditWnd, szText, REPORTCTRL_MAX_TEXT-1);
			szText[REPORTCTRL_MAX_TEXT-1] = 0;

			if(i || !::GetLastError())
				SetItemText(m_iEditItem, m_iEditSubItem, szText);
		}
		else
			SetItemText(m_iEditItem, m_iEditSubItem, lpnmrvie->lpszText);
	}

	if(IsWindow(m_hEditWnd))
		::PostMessage(m_hEditWnd, WM_CLOSE, 0, 0);
	m_hEditWnd = NULL;

	CWnd* pWnd = GetFocus();
	if(pWnd)
	{
		if(pWnd->GetSafeHwnd() != m_hWnd)
			m_bFocus = FALSE;
	}

	RedrawRows(m_iFocusRow);
}

void CReportCtrl::QuickSort(INT iColumn, INT iLow, INT iHigh, BOOL bAscending)
{
	INT i, j;

	i = iHigh;
	j = iLow;

	INT iRow = m_arrayRows[((INT) ((iLow+iHigh) / 2))];

	do
	{
		if(bAscending)
		{
			while(CompareItems(m_arrayRows[j], iColumn, iRow, iColumn) < 0) j++;
			while(CompareItems(m_arrayRows[i], iColumn, iRow, iColumn) > 0) i--;
		}
		else
		{
			while(CompareItems(m_arrayRows[j], iColumn, iRow, iColumn) > 0) j++;
			while(CompareItems(m_arrayRows[i], iColumn, iRow, iColumn) < 0) i--;
		}

		if(i >= j)
		{
			if (i != j)
			{
				INT iTemp;

				iTemp = m_arrayRows[i];
				m_arrayRows[i] = m_arrayRows[j];
				m_arrayRows[j] = iTemp;
			}

			i--;
			j++;
		}
	} while(j <= i);

	if(iLow < i) QuickSort(iColumn, iLow, i, bAscending);
	if(j < iHigh) QuickSort(iColumn, j, iHigh, bAscending);
}

void CReportCtrl::TreeSort(INT iSubItem, LPTREEITEM lptiParent, BOOL bAscending, BOOL bTraverse)
{
	TREEITEM ti;

	LPTREEITEM lpti, lptiSibling = lptiParent->lptiChildren;
	while(lptiSibling != NULL)
	{
		lpti = lptiSibling;
		lptiSibling = lptiSibling->lptiSibling;

		lpti->lptiSibling = NULL;

		if(bTraverse && lpti->lptiChildren != NULL)
			TreeSort(iSubItem, lpti, bAscending);

		InsertTree(iSubItem, &ti, (LPTREEITEM)RVTI_SORT, lpti, bAscending);
	}

	lptiParent->lptiChildren = ti.lptiChildren;
}

/////////////////////////////////////////////////////////////////////////////
// CReportCtrl message handlers

BOOL CReportCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	SetCursor(::LoadCursor(NULL, IDC_ARROW));
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CReportCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	m_bFocus = TRUE;
	Invalidate();
}

void CReportCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	if(CWnd::IsChild(pNewWnd) == FALSE)
		m_bFocus = FALSE;

	if( m_bFocus == FALSE && pNewWnd != NULL && pNewWnd->GetSafeHwnd() != m_wndTip.GetSafeHwnd())
		m_wndTip.Hide();

	Invalidate();
}

void CReportCtrl::OnSysColorChange()
{
	CWnd::OnSysColorChange();
	GetSysColors();
}

void CReportCtrl::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CWnd::OnSettingChange(uFlags, lpszSection);

	m_nRowsPerWheelNotch = GetMouseScrollLines();
}

BOOL CReportCtrl::OnQueryNewPalette()
{
	Invalidate();
	return CWnd::OnQueryNewPalette();
}

void CReportCtrl::OnPaletteChanged(CWnd* pFocusWnd)
{
	CWnd::OnPaletteChanged(pFocusWnd);

	if(pFocusWnd->GetSafeHwnd() != GetSafeHwnd())
		Invalidate();
}

void CReportCtrl::OnSize(UINT nType, int cx, int cy)
{
	if(m_hEditWnd != NULL)
		EndEdit();

	CWnd::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(rect);

	Layout(rect.Width(), rect.Height());
}

void CReportCtrl::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	CWnd::OnWindowPosChanging(lpwndpos);
	
	if(!m_bHorzScrollBar)
	{
		ShowScrollBar( SB_HORZ, FALSE );
		ModifyStyle( WS_HSCROLL, 0, SWP_DRAWFRAME );
	}
}

LRESULT CReportCtrl::OnGetFont(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return (LRESULT)m_font.m_hObject;
}

LRESULT CReportCtrl::OnSetFont(WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = Default();

	CFont *pFont = CFont::FromHandle((HFONT)wParam);
	if(pFont)
	{
		LOGFONT lf;
		pFont->GetLogFont(&lf);

		m_font.DeleteObject();
		m_font.CreateFontIndirect(&lf);

		lf.lfWeight = FW_BOLD;
		m_fontBold.DeleteObject();
		m_fontBold.CreateFontIndirect(&lf);
	}

	CDC* pDC = GetDC();
	if (pDC)
	{
		CFont* pFont = pDC->SelectObject(&m_font);
		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);
		pDC->SelectObject(pFont);
		ReleaseDC(pDC);

		INT iDefaultHeight; 
		iDefaultHeight = tm.tmHeight + tm.tmExternalLeading + 2;
		iDefaultHeight += m_dwStyle&RVS_SHOWHGRID ? 1:0;

		if( m_pImageList != NULL )
			m_iDefaultHeight = max(iDefaultHeight, m_sizeImage.cy+1);
		else
			m_iDefaultHeight = iDefaultHeight;

		m_sizeCheck.cx = m_iDefaultHeight-2;
		m_sizeCheck.cy = m_iDefaultHeight-2;
	}

	if(::IsWindow(GetSafeHwnd()) && lParam)
	{
		CRect rect;
		GetClientRect(rect);

		Layout(rect.Width(), rect.Height());
	}

	return lResult;
}


BOOL CReportCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CReportCtrl::OnNcCalcSize(BOOL /*bCalcValidRects*/, NCCALCSIZE_PARAMS FAR* lpncsp)
{
	LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);

	if(lStyle & WS_BORDER)
		::InflateRect(&lpncsp->rgrc[0], -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));

	Default();
}

void CReportCtrl::OnNcPaint()
{
	Default();

	LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
	if(lStyle & WS_BORDER)
	{
		CWindowDC dc(this);
		CRect rcBounds;
		GetWindowRect(rcBounds);
		ScreenToClient(rcBounds);
		rcBounds.OffsetRect(-rcBounds.left, -rcBounds.top);
		dc.DrawEdge(rcBounds, EDGE_SUNKEN, BF_RECT);
	}
}

void CReportCtrl::OnPaint()
{
	CPaintDC dc(this);

	CPalette* pPalette = NULL;
	if(dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
	{
		pPalette = dc.SelectPalette(&m_palette, FALSE);
		dc.RealizePalette();
	}

	dc.ExcludeClipRect(m_rectHeader);

    if(m_bDoubleBuffer)
    {
        CMemDC MemDC(&dc);
        DrawCtrl(&MemDC);

		if(m_hEditWnd != NULL)
		{
			RECT rect;
			::GetWindowRect(m_hEditWnd, &rect);

			ScreenToClient(&rect);
			dc.ExcludeClipRect(&rect);
		}
    }
    else
        DrawCtrl(&dc);

	if(pPalette && dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		dc.SelectPalette(pPalette, FALSE);
}

void CReportCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* /*pScrollBar*/)
{
	INT iScrollPos = GetScrollPos32(SB_HORZ);
	INT iScroll;

	if(m_hEditWnd != NULL)
		EndEdit();

	switch (nSBCode)
	{
	case SB_LINERIGHT:
		iScroll = min(m_iVirtualWidth-(m_rectReport.Width()+iScrollPos), m_rectReport.Width()>>3);
		ScrollWindow(SB_HORZ, iScrollPos + iScroll);
		break;

	case SB_LINELEFT:
		iScroll = min(iScrollPos, m_rectReport.Width()>>3);
		ScrollWindow(SB_HORZ, iScrollPos - iScroll);
		break;

	case SB_PAGERIGHT:
		iScrollPos = min(m_iVirtualWidth, iScrollPos + m_rectReport.Width());
		ScrollWindow(SB_HORZ, iScrollPos);
		break;

	case SB_PAGELEFT:
		iScrollPos = max(0, iScrollPos - m_rectReport.Width());
		ScrollWindow(SB_HORZ, iScrollPos);
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		iScrollPos = nPos;
		ScrollWindow(SB_HORZ, iScrollPos);
		break;

	case SB_RIGHT:
		ScrollWindow(SB_HORZ, m_iVirtualWidth);
		break;

	case SB_LEFT:
		ScrollWindow(SB_HORZ, 0);
		break;

	default:
		break;
	}
}

void CReportCtrl::OnVScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	INT iFirst = GetScrollPos32(SB_VERT), iLast;
	INT iItems = GetVisibleRows(TRUE, &iFirst, &iLast);

	if(m_hEditWnd != NULL)
		EndEdit();

	switch(nSBCode)
	{
	case SB_LINEUP:
		if(iFirst>0)
			iFirst--;
		break;

	case SB_LINEDOWN:
		if(iFirst+iItems < m_iVirtualHeight)
			iFirst++;
		break;

	case SB_PAGEUP:
		GetVisibleRows(TRUE, &iLast, &iFirst, TRUE);
		iFirst = iLast-1;
		iFirst = iFirst<0 ? 0:iFirst;
		break;

	case SB_PAGEDOWN:
		iFirst += iItems;
		GetVisibleRows(TRUE, &iFirst, &iLast);
		GetVisibleRows(TRUE, &iFirst, &iLast, TRUE);
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		SetScrollPos32(SB_VERT, GetScrollPos32(SB_VERT, TRUE));

		iFirst = GetScrollPos32(SB_VERT);
		GetVisibleRows(TRUE, &iFirst, &iLast);
		GetVisibleRows(TRUE, &iFirst, &iLast, TRUE);
		break;

	case SB_TOP:
		iFirst = 0;
		break;

	case SB_BOTTOM:
		iLast = m_iVirtualHeight-1;
		GetVisibleRows(TRUE, &iFirst, &iLast, TRUE);
		break;

	default:
		return;
	}

	ScrollWindow(SB_VERT, iFirst);
}

void CReportCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	RVHITTESTINFO rvhti;
	rvhti.point = point;

	HitTest(&rvhti);

	m_lastLClickPos = point;

	if(m_dwStyle&RVS_TREEMASK && rvhti.nFlags&RVHT_ONITEMTREEBOX)
	{
		if(!Notify(RVN_ITEMEXPANDING, rvhti.iItem, rvhti.iSubItem))
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
				ExpandAll((HTREEITEM)m_arrayItems[rvhti.iItem].lptiItem, RVE_TOGGLE);
			else
				Expand((HTREEITEM)m_arrayItems[rvhti.iItem].lptiItem, RVE_TOGGLE);

			Notify(RVN_ITEMEXPANDED, rvhti.iItem, rvhti.iSubItem);
			return;
		}
	}

	if(Notify(RVN_ITEMCLICK, nFlags, &rvhti))
	{
		CWnd::OnLButtonDown(nFlags, point);
		return;
	}

	INT iFocusRow = m_iFocusRow, iFocusColumn = m_iFocusColumn;

	INT iSubItem = GetSubItemFromColumn(rvhti.iColumn);
	if(
		m_dwStyle&RVS_FOCUSSUBITEMS &&
		iSubItem >= 0 &&
		!(m_arraySubItems[iSubItem].nFormat&RVCF_SUBITEM_NOFOCUS)
	)
		m_iFocusColumn = rvhti.iColumn;
	else
		m_iFocusColumn = -1;
	
	if(rvhti.iItem != RVI_INVALID)
	{
		INT iRow = GetRowFromItem(rvhti.iItem);
		ASSERT(iRow != RVI_INVALID);

		switch(nFlags&(MK_CONTROL|MK_SHIFT))
		{
		case MK_CONTROL:
			SelectRows(iRow, iRow, TRUE, TRUE, TRUE);
			m_iSelectRow = iRow;
			break;

		case MK_SHIFT:
			SelectRows(iRow, m_iSelectRow, TRUE);
			break;

		case MK_CONTROL|MK_SHIFT:
			SelectRows(iRow, m_iSelectRow, TRUE, TRUE);
			m_iSelectRow = iRow;
			break;

		default:
			SelectRows(iRow, iRow, TRUE);
			m_iSelectRow = iRow;

			if(m_iFocusRow == iFocusRow && m_iFocusColumn == iFocusColumn && (m_iFocusRow == RVI_EDIT || m_dwStyle&RVS_FOCUSSUBITEMS))
				BeginEdit(m_iFocusRow, m_iFocusColumn, VK_LBUTTON);
			break;
		}
	}
}

void CReportCtrl::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	m_lastLClickPos = CPoint(0, 0);
}

void CReportCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	RVHITTESTINFO rvhti;
	ZeroMemory(&rvhti, sizeof(RVHITTESTINFO));
	rvhti.point = point;

	HitTest(&rvhti);
	
	m_lastRClickPos = point;

	if(Notify(RVN_ITEMRCLICK, nFlags, &rvhti))
	{
		CWnd::OnRButtonDown(nFlags, point);
		return;
	}

	if(rvhti.iItem != RVI_INVALID)
	{
		INT iRow = GetRowFromItem(rvhti.iItem);
		ASSERT(iRow != RVI_INVALID);

		if(!(nFlags&(MK_CONTROL|MK_SHIFT)))
		{
			RVITEM rvi;
			ZeroMemory(&rvi, sizeof rvi);
			rvi.nMask = RVIM_STATE;
			rvi.iItem = rvhti.iItem;
			if (GetItem(&rvi) && !(rvi.nState&RVIS_SELECTED))
			{
				SelectRows(iRow, iRow, TRUE);
				m_iSelectRow = iRow;
			}
		}
	}
}

void CReportCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	SetFocus();

	RVHITTESTINFO rvhti;
	ZeroMemory(&rvhti, sizeof(RVHITTESTINFO));
	rvhti.point = point;

	HitTest(&rvhti);

	m_lastRClickPos = CPoint(0, 0);

	if(Notify(RVN_ITEMRCLICKUP, nFlags, &rvhti))
	{
		CWnd::OnRButtonUp(nFlags, point);
	}
}

void CReportCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	RVHITTESTINFO rvhti;
	rvhti.point = point;

	HitTest(&rvhti);

	if(Notify(RVN_ITEMDBCLICK, nFlags, &rvhti))
	{
		CWnd::OnLButtonDblClk(nFlags, point);
		return;
	}

	if(m_dwStyle&RVS_TREEMASK)
	{
		if(
			rvhti.iItem >= RVI_FIRST &&
			m_arrayItems[rvhti.iItem].lptiItem->lptiChildren != NULL &&
			!Notify(RVN_ITEMEXPANDING, rvhti.iItem, rvhti.iSubItem)
		) {
			Expand((HTREEITEM)m_arrayItems[rvhti.iItem].lptiItem, RVE_TOGGLE);
			Notify(RVN_ITEMEXPANDED, rvhti.iItem, rvhti.iSubItem);
		}
	}
}

UINT CReportCtrl::OnGetDlgCode()
{
	return DLGC_WANTARROWS;
}

void CReportCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	INT iFirst = GetScrollPos32(SB_VERT), iLast;
	GetVisibleRows(TRUE, &iFirst, &iLast);

	NMREPORTVIEW nmrv;
	nmrv.hdr.hwndFrom = GetSafeHwnd();
	nmrv.hdr.idFrom = GetDlgCtrlID();
	nmrv.hdr.code = RVN_KEYDOWN;

	nmrv.nKeys = nChar;
	nmrv.nFlags = nFlags;

	nmrv.iItem = GetItemFromRow(m_iFocusRow);
	nmrv.iSubItem = GetSubItemFromColumn(m_iFocusColumn);

	if(nmrv.iItem != RVI_INVALID && (!(m_dwStyle&RVS_OWNERDATA)))
		nmrv.lParam = GetItemStruct(nmrv.iItem, -1).lParam;

	m_bProcessKey = FALSE;

	if(!Notify(&nmrv))
	{
		switch (nChar)
		{
		case VK_SPACE:
			SelectRows(m_iFocusRow, m_iFocusRow, TRUE, TRUE, IsCtrlDown() ? TRUE:FALSE);
			return;

		case VK_LEFT:
			if(m_dwStyle&RVS_TREEMASK && m_iFocusColumn == -1)
			{
				INT iFocusItem = GetItemFromRow(m_iFocusRow);
				if(iFocusItem >= RVI_FIRST)
				{
					LPTREEITEM lpti = m_arrayItems[iFocusItem].lptiItem;
					if(lpti->lptiChildren != NULL && lpti->bOpen == TRUE)
					{
						if(!Notify(RVN_ITEMEXPANDING, lpti->iItem, lpti->iSubItem))
						{
							Expand((HTREEITEM)lpti, RVE_COLLAPSE);
							Notify(RVN_ITEMEXPANDED, lpti->iItem, lpti->iSubItem);
						}
						break;
					}
					else
					{
						if(lpti->lptiParent != &m_tiRoot)
						{
							lpti = lpti->lptiParent;

							INT iFocusRow = GetRowFromItem(lpti->iItem);
							ASSERT(iFocusRow != RVI_INVALID);

							SelectRows(iFocusRow, iFocusRow);
							EnsureVisible(lpti->iItem);
							break;
						}
						else if(m_dwStyle&RVS_SHOWEDITROW)
						{
							SelectRows(RVI_EDIT, RVI_EDIT);
							break;
						}
					}
				}
			}

			if(m_dwStyle&RVS_FOCUSSUBITEMS)
			{
				if(m_iFocusColumn > 0)
				{
					m_iFocusColumn--;

					while(m_iFocusColumn >= 0)
					{
						INT iSubItem = GetSubItemFromColumn(m_iFocusColumn);
						if(iSubItem >= 0 && !(m_arraySubItems[iSubItem].nFormat&RVCF_SUBITEM_NOFOCUS))
							break;

						m_iFocusColumn--;
					}
				}
				else
					m_iFocusColumn = -1;

				EnsureVisibleColumn(m_iFocusColumn);

				iFirst = GetRowFromItem(m_iFocusRow);
				RedrawRows(iFirst);
			}
			else
			{
				INT iScrollPos = GetScrollPos32(SB_HORZ);
				INT iScroll = min(iScrollPos, m_rectReport.Width()>>3);

				ScrollWindow(SB_HORZ, iScrollPos - iScroll);
			}
			return;

		case VK_RIGHT:
			if(m_dwStyle&RVS_TREEMASK && m_iFocusColumn == -1)
			{
				INT iFocusItem = GetItemFromRow(m_iFocusRow);
				if(iFocusItem >= RVI_FIRST)
				{
					LPTREEITEM lpti = m_arrayItems[iFocusItem].lptiItem;
					if(lpti->lptiChildren != NULL)
					{
						if(lpti->bOpen == FALSE)
						{
							if(!Notify(RVN_ITEMEXPANDING, lpti->iItem, lpti->iSubItem))
							{
								Expand((HTREEITEM)lpti, RVE_EXPAND);
								Notify(RVN_ITEMEXPANDED, lpti->iItem, lpti->iSubItem);
								return;
							}
							break;
						}
						else
						{
							lpti = lpti->lptiChildren;

							INT iFocusRow = GetRowFromItem(lpti->iItem);
							ASSERT(iFocusRow != RVI_INVALID);

							SelectRows(iFocusRow, iFocusRow);
							EnsureVisible(lpti->iItem);
							break;
						}
					}
				}
			}

			if(m_dwStyle&RVS_FOCUSSUBITEMS)
			{
				if(m_iFocusColumn < m_arrayColumns.GetSize()-1)
				{
					INT iFocusColumn = m_iFocusColumn;

					m_iFocusColumn++;

					while(m_iFocusColumn < m_arrayColumns.GetSize())
					{
						INT iSubItem = GetSubItemFromColumn(m_iFocusColumn);
						if(iSubItem >= 0 && !(m_arraySubItems[iSubItem].nFormat&RVCF_SUBITEM_NOFOCUS))
							break;

						m_iFocusColumn++;

						if(m_iFocusColumn == m_arrayColumns.GetSize())
						{
							m_iFocusColumn = iFocusColumn;
							break;
						}
					}
				}
				else
					m_iFocusColumn = (INT)m_arrayColumns.GetSize()-1;

				EnsureVisibleColumn(m_iFocusColumn);

				iFirst = GetRowFromItem(m_iFocusRow);
				RedrawRows(iFirst);
			}
			else
			{
				INT iScrollPos = GetScrollPos32(SB_HORZ);
				INT iScroll = min(m_iVirtualWidth-(m_rectReport.Width()+iScrollPos), m_rectReport.Width()>>3);

				ScrollWindow(SB_HORZ, iScrollPos + iScroll);
			}
			return;

		case VK_DOWN:
			if(m_iFocusRow<m_iVirtualHeight-1)
			{
				if(
					m_iFocusRow == RVI_INVALID &&
					!(m_dwStyle&RVS_SHOWEDITROW) &&
					m_iVirtualHeight > 0
				)
					m_iFocusRow = RVI_EDIT;

				if(IsCtrlDown())
				{
					SelectRows(m_iFocusRow+1, m_iFocusRow+1, FALSE);
					m_iSelectRow = m_iFocusRow;
				}
				else if(!IsShiftDown())
					{
						SelectRows(m_iFocusRow+1, m_iFocusRow+1, TRUE);
						m_iSelectRow = m_iFocusRow;
					}
					else
						SelectRows(m_iFocusRow+1, m_iSelectRow, TRUE);

				EnsureVisible(GetItemFromRow(m_iFocusRow));
			}
			return;

		case VK_UP:
			if(
				(m_dwStyle&RVS_SHOWEDITROW && m_iFocusRow > RVI_EDIT) ||
				m_iFocusRow > RVI_FIRST
			) {
				if(IsCtrlDown())
				{
					SelectRows(m_iFocusRow-1, m_iFocusRow-1, FALSE);
					m_iSelectRow = m_iFocusRow;
				}
				else if(!IsShiftDown())
					{
						SelectRows(m_iFocusRow-1, m_iFocusRow-1, TRUE);
						m_iSelectRow = m_iFocusRow;
					}
					else
						SelectRows(m_iFocusRow-1, m_iSelectRow, TRUE);

				EnsureVisible(GetItemFromRow(m_iFocusRow));
			}
			return;

		case VK_NEXT:
			if(m_iFocusRow == iLast)
			{
				SendMessage(WM_VSCROLL, SB_PAGEDOWN, 0);

				iFirst = GetScrollPos32(SB_VERT);
				GetVisibleRows(TRUE, &iFirst, &iLast);
			}

			if(IsCtrlDown())
			{
				SelectRows(iLast, iLast, FALSE);
				m_iSelectRow = m_iFocusRow;
				return;
			}
			else if(!IsShiftDown())
				{
					iFirst = iLast;
					m_iSelectRow = iLast;
				}
				else
					iFirst = m_iSelectRow;

			SelectRows(iLast, iFirst, TRUE);
			return;

		case VK_PRIOR:
			if(m_iFocusRow == iFirst)
			{
				SendMessage(WM_VSCROLL, SB_PAGEUP, 0);

				iFirst = GetScrollPos32(SB_VERT);
			}

			iFirst = iLast<iFirst ? iLast:iFirst;

			if(IsCtrlDown())
			{
				SelectRows(iFirst, iFirst, FALSE);
				m_iSelectRow = m_iFocusRow;
				return;
			}
			else if(!IsShiftDown())
				{
					iLast = iFirst;
					m_iSelectRow = iFirst;
				}
				else
					iLast = m_iSelectRow;

			SelectRows(iFirst, iLast, TRUE);
			return;

		case VK_HOME:
			if(m_iFocusRow>0)
			{
				SendMessage(WM_VSCROLL, SB_TOP, 0);

				if(!IsShiftDown())
					m_iSelectRow = 0;

				SelectRows(0, m_iSelectRow, IsCtrlDown()?FALSE:TRUE);
			}
			break;

		case VK_END:
			if(m_iFocusRow<m_iVirtualHeight)
			{
				SendMessage(WM_VSCROLL, SB_BOTTOM, 0);

				if(!IsShiftDown())
					m_iSelectRow = m_iVirtualHeight-1;

				SelectRows(m_iVirtualHeight-1, m_iSelectRow, IsCtrlDown()?FALSE:TRUE);
			}
			break;

		case VK_ESCAPE:
			break;

		default:
			m_bProcessKey = !IsCtrlDown() ? TRUE:FALSE;
			break;
		}
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CReportCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_bProcessKey && m_iFocusColumn != -1 && (m_iFocusRow == RVI_EDIT || m_dwStyle&RVS_FOCUSSUBITEMS))
		if(BeginEdit(m_iFocusRow, m_iFocusColumn, nChar))
			return;

	CWnd::OnChar(nChar, nRepCnt, nFlags);
}

BOOL CReportCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	int i;

    // A m_nRowsPerWheelNotch value less than 0 indicates that the mouse
    // wheel scrolls whole pages, not just lines.
	if (m_nRowsPerWheelNotch == -1)
    {
        int iPagesScrolled = zDelta / 120;

        if (iPagesScrolled > 0)
            for (i=0;i<iPagesScrolled;i++)
                PostMessage(WM_VSCROLL, SB_PAGEUP, 0);
        else
            for (i=0;i>iPagesScrolled;i--)
                PostMessage(WM_VSCROLL, SB_PAGEDOWN, 0);
    }
    else
    {
        int iRowsScrolled = (int)m_nRowsPerWheelNotch * zDelta / 120;

        if (iRowsScrolled>0)
            for (i=0;i<iRowsScrolled;i++)
                PostMessage(WM_VSCROLL, SB_LINEUP, 0);
        else
            for (i=0;i>iRowsScrolled;i--)
                PostMessage(WM_VSCROLL, SB_LINEDOWN, 0);
    }

    return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CReportCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	BOOL bHide = TRUE;

	if ((nFlags & MK_RBUTTON)&&(!((m_lastRClickPos.x == 0)&&(m_lastRClickPos.y == 0))))
	{
		// check if we have to start a drag operation
		if (abs(m_lastRClickPos.x - point.x) > GetSystemMetrics(SM_CXDRAG))
		{
			OnBeginDrag();
			m_lastRClickPos = CPoint(0, 0);
		}
		else if (abs(m_lastRClickPos.y - point.y) > GetSystemMetrics(SM_CYDRAG))
		{
			OnBeginDrag();
			m_lastRClickPos = CPoint(0, 0);
		}
	}
	else
		m_lastRClickPos = CPoint(0, 0);
	if ((nFlags & MK_LBUTTON)&&(!((m_lastLClickPos.x == 0)&&(m_lastLClickPos.y == 0))))
	{
		// check if we have to start a drag operation
		if (abs(m_lastLClickPos.x - point.x) > GetSystemMetrics(SM_CXDRAG))
		{
			OnBeginDrag();
			m_lastLClickPos = CPoint(0, 0);
		}
		else if (abs(m_lastLClickPos.y - point.y) > GetSystemMetrics(SM_CYDRAG))
		{
			OnBeginDrag();
			m_lastLClickPos = CPoint(0, 0);
		}
	}
	else
		m_lastLClickPos = CPoint(0, 0);

	if(m_bFocus && !nFlags && m_dwStyle&RVS_EXPANDSUBITEMS && m_hEditWnd == NULL)
	{
		RVHITTESTINFO rvhti;
		rvhti.point = point;

		if(HitTest(&rvhti) != RVI_INVALID)
		{
			TCHAR szText[REPORTCTRL_MAX_TEXT];

			RVITEM rvi;
			rvi.iItem = rvhti.iItem;
			rvi.iSubItem = rvhti.iSubItem;
			rvi.lpszText = szText;
			rvi.iTextMax = REPORTCTRL_MAX_TEXT;
			rvi.nMask = RVIM_TEXT|RVIM_STATE;
			GetItem(&rvi);

			if(rvhti.nFlags&RVHT_ONITEMTEXT && rvi.nMask&RVIM_TEXT && _tcslen(rvi.lpszText))
			{
				CRect rect;

				if(
					m_dwStyle&RVS_TREEMASK &&
					m_arrayItems[rvhti.iItem].lptiItem->iSubItem >= 0
				)
					GetItemRect(rvhti.iItem, rvhti.iSubItem, rect, RVIR_TEXTNOCOLUMNS);
				else
					GetItemRect(rvhti.iItem, rvhti.iSubItem, rect, RVIR_TEXT);

				GetExpandedItemText(&rvi);

				bHide = !m_wndTip.Show(
					rect,
					rvi.lpszText,
					rvi.nState&RVIS_BOLD ? &m_fontBold:&m_font
				);
			}
		}
	}

	if( m_wndTip.IsWindowVisible() && bHide )
		m_wndTip.Hide();

	CWnd::OnMouseMove(nFlags, point);
}

void CReportCtrl::OnHdnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPHDITEM lphdi = (LPHDITEM)((LPNMHEADER)pNMHDR)->pitem;

	if(m_hEditWnd != NULL)
		EndEdit();

	HDITEM hdi;
	hdi.mask = HDI_WIDTH|HDI_ORDER|HDI_LPARAM;
	m_wndHeader.GetItem(((LPNMHEADER)pNMHDR)->iItem, &hdi);

	if(lphdi->mask&HDI_FORMAT)
		m_arraySubItems[hdi.lParam].nFormat =
			(m_arraySubItems[hdi.lParam].nFormat & ~RVCF_JUSTIFYMASK) |
			(lphdi->fmt & HDF_JUSTIFYMASK);

	if(lphdi->mask&HDI_WIDTH)
	{
		m_iVirtualWidth += lphdi->cxy - m_arraySubItems[hdi.lParam].iWidth;
		ASSERT(m_iVirtualWidth >= 0);

		m_arraySubItems[hdi.lParam].iWidth = lphdi->cxy;
	}

	if(m_wndHeader.IsAutoSizing() == FALSE)
		ScrollWindow(SB_HORZ, GetScrollPos32(SB_HORZ));

	Notify(RVN_LAYOUTCHANGED, RVI_INVALID, (INT)hdi.lParam);

	*pResult = FALSE;
}

void CReportCtrl::OnHdnItemClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	if(m_hEditWnd != NULL)
		EndEdit();

	NMRVHEADER nmrvhdr;
	if(!NotifyHdr(&nmrvhdr, RVN_HEADERCLICK, ((LPNMHEADER)pNMHDR)->iItem))
	{
		if(!(m_dwStyle&(RVS_NOSORT|RVS_OWNERDATA)))
		{
			BOOL bAscending;

			INT iOld = m_wndHeader.GetSortColumn(&bAscending);
			INT iNew = ((LPNMHEADER)pNMHDR)->iItem;

			if(iNew == iOld)
				bAscending = !bAscending;
			else
				bAscending = FALSE;

			SortItems(nmrvhdr.iSubItem, bAscending);
		}
	}

	*pResult = FALSE;
}

void CReportCtrl::OnHdnBeginDrag(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	//LPNMHEADER lpnmhdr = (LPNMHEADER)pNMHDR;

	if(m_hEditWnd != NULL)
		EndEdit();

	pResult = FALSE;
}

void CReportCtrl::OnHdnEndDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMHEADER lpnmhdr = (LPNMHEADER)pNMHDR;

	INT iDropResult = m_wndHeader.GetDropResult();
	if(iDropResult == FHDR_DROPPED || iDropResult == FHDR_ONTARGET)
	{
		if(m_dwStyle&RVS_ALLOWCOLUMNREMOVAL)
			DeactivateSubItem((INT)lpnmhdr->pitem->lParam);

		*pResult = TRUE;
	}
	else
	{
		m_bColumnsReordered = TRUE;
		m_iIndentColumnPending = m_wndHeader.OrderToIndex(0);
		Invalidate();

		*pResult = FALSE;
	}
}

void CReportCtrl::OnHdnDividerDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	if(m_hEditWnd != NULL)
		EndEdit();

	NMRVHEADER nmrvhdr;
	if(
		!NotifyHdr(&nmrvhdr, RVN_DIVIDERDBLCLICK, ((LPNMHEADER)pNMHDR)->iItem) &&
		!(m_dwStyle&RVS_OWNERDATA) &&
		!(m_arraySubItems[nmrvhdr.iSubItem].nFormat&RVCF_EX_FIXEDWIDTH)
	) {
		INT iItems = (INT)m_arrayItems.GetSize();
		if(iItems > 0)
		{
			INT iWidth = 0;
			RECT rect;

			for(INT iItem=0;iItem<iItems;iItem++)
			{
				if(MeasureItem(iItem, nmrvhdr.iSubItem, &rect))
				{
					INT cx = rect.right - rect.left;
					iWidth = cx > iWidth ? cx:iWidth;
				}
			}

			HDITEM hdi;
			hdi.mask = HDI_WIDTH;
			hdi.cxy = iWidth;
			m_wndHeader.SetItem(((LPNMHEADER)pNMHDR)->iItem, &hdi);
		}
	}

	*pResult = FALSE;
}

void CReportCtrl::OnRvnEndItemEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMRVITEMEDIT lpnmrvie = (LPNMRVITEMEDIT)pNMHDR;

	ASSERT(m_iEditItem == lpnmrvie->iItem);
	ASSERT(m_iEditSubItem == lpnmrvie->iSubItem);

	NMRVITEMEDIT nmrvie;
	memset(&nmrvie, 0, sizeof(nmrvie));

	nmrvie.hdr.hwndFrom = GetSafeHwnd();
	nmrvie.hdr.idFrom = GetDlgCtrlID();
	nmrvie.hdr.code = RVN_ENDITEMEDIT;

	nmrvie.iItem = m_iEditItem;
	nmrvie.iSubItem = m_iEditSubItem;

	nmrvie.hWnd = lpnmrvie->hWnd;

	nmrvie.nKey = lpnmrvie->nKey;
	nmrvie.lpszText = lpnmrvie->lpszText;

	nmrvie.lParam = lpnmrvie->lParam;

	BOOL bResult = FALSE;
	if(GetParent() != NULL)
		bResult = Notify((LPNMREPORTVIEW)&nmrvie);

	if(bResult)
		EndEdit(FALSE);
	else
		EndEdit(nmrvie.nKey != VK_ESCAPE ? TRUE:FALSE, &nmrvie);

	pResult = FALSE;
}

HRESULT CReportCtrl::QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;
	if (IID_IUnknown==riid || IID_IDropTarget==riid)
		*ppvObject=this;

	if (*ppvObject != NULL)
	{
		((LPUNKNOWN)*ppvObject)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG CReportCtrl::Release()
{
	m_cRefCount--;
	return m_cRefCount;
}

HRESULT CReportCtrl::DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	if(pDataObj == NULL)
		return E_INVALIDARG;

	if(m_pDropTargetHelper)
		m_pDropTargetHelper->DragEnter(m_hWnd, pDataObj, (LPPOINT)&pt, *pdwEffect);
	
	m_pDropDataObj = pDataObj;

	if (pdwEffect)
	{
		RVHITTESTINFO rvhti;
		rvhti.point.x = pt.x;
		rvhti.point.y = pt.y;
		ScreenToClient(&rvhti.point);
		HitTest(&rvhti);
		*pdwEffect = DROPEFFECT_NONE;
		if ((rvhti.iItem >= 0)&&(rvhti.iSubItem >= 0))
		{
			*pdwEffect = OnDrag(rvhti.iItem, rvhti.iSubItem, pDataObj, grfKeyState);
			if (*pdwEffect != DROPEFFECT_NONE)
			{
				// Select the target
				SetSelection(rvhti.iItem);
				ATLTRACE("CReportCtrl::DragEnter Item %d, Subitem %d\n", rvhti.iItem, rvhti.iSubItem);
			}
		}
	}
	return S_OK;
}

HRESULT CReportCtrl::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	if(m_pDropTargetHelper)
		m_pDropTargetHelper->DragOver((LPPOINT)&pt, *pdwEffect);
	if (pdwEffect)
	{
		RVHITTESTINFO rvhti;
		rvhti.point.x = pt.x;
		rvhti.point.y = pt.y;
		ScreenToClient(&rvhti.point);
		HitTest(&rvhti);
		*pdwEffect = DROPEFFECT_NONE;
		if ((rvhti.iItem >= 0)&&(rvhti.iSubItem >= 0))
		{
			*pdwEffect = OnDrag(rvhti.iItem, rvhti.iSubItem, m_pDropDataObj, grfKeyState);
			if (*pdwEffect != DROPEFFECT_NONE)
			{
				// Select the target
				SetSelection(rvhti.iItem);
				ATLTRACE("CReportCtrl::DragOver Item %d, Subitem %d\n", rvhti.iItem, rvhti.iSubItem);
			}
		}
	}

	CPoint currentPoint;
	GetCursorPos(&currentPoint);
	ScreenToClient(&currentPoint);

	RVHITTESTINFO rvhti;
	rvhti.point = currentPoint;
	INT nToggledItem = HitTest(&rvhti);

	if (nToggledItem==-1)
	{
		m_nLastToggledItem = -1;
	}
	else if (nToggledItem!=m_nLastToggledItem)
	{
		SetTimer(REPORTCTRL_AUTOEXPAND_TIMERID, REPORTCTRL_AUTOEXPAND,NULL);
		m_nLastToggledItem = nToggledItem;
	}

	return S_OK;
}

HRESULT CReportCtrl::DragLeave()
{
	m_pDropDataObj = NULL;
	if(m_pDropTargetHelper)
		m_pDropTargetHelper->DragLeave();

	KillTimer(REPORTCTRL_AUTOEXPAND_TIMERID);

	return S_OK;
}

HRESULT CReportCtrl::Drop(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	if (pDataObj == NULL)
		return E_INVALIDARG;	

	if(m_pDropTargetHelper)
		m_pDropTargetHelper->Drop(pDataObj, (LPPOINT)&pt, *pdwEffect);

	if (pdwEffect)
	{
		RVHITTESTINFO rvhti;
		rvhti.point.x = pt.x;
		rvhti.point.y = pt.y;
		ScreenToClient(&rvhti.point);
		HitTest(&rvhti);
		*pdwEffect = DROPEFFECT_NONE;
		if ((rvhti.iItem >= 0)&&(rvhti.iSubItem >= 0))
		{
			*pdwEffect = OnDrag(rvhti.iItem, rvhti.iSubItem, pDataObj, grfKeyState);
			if (*pdwEffect != DROPEFFECT_NONE)
			{
				// Select the target
				SetSelection(rvhti.iItem);
				ATLTRACE("CReportCtrl::Drop Item %d, Subitem %d\n", rvhti.iItem, rvhti.iSubItem);
				OnDrop(rvhti.iItem, rvhti.iSubItem, pDataObj, grfKeyState);
			}
		}
	}
	*pdwEffect = DROPEFFECT_NONE;
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CReportSubItemListCtrl

CReportSubItemListCtrl::CReportSubItemListCtrl()
{
	m_pReportCtrl = NULL;

	m_iSubItem = -1;
	m_pDragWnd = NULL;
}

CReportSubItemListCtrl::~CReportSubItemListCtrl()
{
}


BEGIN_MESSAGE_MAP(CReportSubItemListCtrl, CDragListBox)
	//{{AFX_MSG_MAP(CReportSubItemListCtrl)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportSubItemListCtrl attributes

BOOL CReportSubItemListCtrl::SetReportCtrl(CReportCtrl* pReportCtrl)
{
	if(pReportCtrl == NULL)
		return FALSE;

	ASSERT_KINDOF(CReportCtrl, pReportCtrl);

	m_pReportCtrl = pReportCtrl;
	ResetContent();

	return TRUE;
}

CReportCtrl* CReportSubItemListCtrl::GetReportCtrl()
{
	return m_pReportCtrl;
}

/////////////////////////////////////////////////////////////////////////////
// CReportSubItemListCtrl operations

BOOL CReportSubItemListCtrl::UpdateList()
{
	INT iSubItem, iSubItems = (INT)m_pReportCtrl->m_arraySubItems.GetSize();

	ResetContent();

	for(iSubItem=0;iSubItem<iSubItems;iSubItem++)
	{
		if(!m_pReportCtrl->IsActiveSubItem(iSubItem) && Include(iSubItem))
		{
			INT iItem = AddString(m_pReportCtrl->m_arraySubItems[iSubItem].strText);
			SetItemData(iItem, iSubItem);
		}
	}

	return TRUE;
}

BOOL CReportSubItemListCtrl::Include(INT /*iSubItem*/)
{
	return TRUE;
}

BOOL CReportSubItemListCtrl::Disable(INT /*iSubItem*/)
{
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CReportSubItemListCtrl message handlers

void CReportSubItemListCtrl::PreSubclassWindow()
{
	CDragListBox::PreSubclassWindow();

	SetItemHeight(0, GetItemHeight(0)+2);
}

void CReportSubItemListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	CRect rcItem(lpDrawItemStruct->rcItem);

	if(GetCount() > 0)
	{
		BOOL bDisable = Disable((INT)lpDrawItemStruct->itemData);

		if(GetExStyle()&WS_EX_STATICEDGE)
		{
			pDC->Draw3dRect(rcItem, m_pReportCtrl->m_cr3DHiLight, m_pReportCtrl->m_cr3DDkShadow);
			rcItem.DeflateRect(1, 1);
			pDC->FillSolidRect(rcItem, m_pReportCtrl->m_cr3DFace);
			rcItem.DeflateRect(1, 1);
		}
		else
		{
			pDC->DrawFrameControl(rcItem, DFC_BUTTON, DFCS_BUTTONPUSH);
			rcItem.DeflateRect(2, 2);
		}

		pDC->SetBkMode(TRANSPARENT);

		if(lpDrawItemStruct->itemState&ODS_SELECTED)
		{
			CBrush brush(::GetSysColor(COLOR_3DSHADOW));
			pDC->FillRect(rcItem, &brush);
			pDC->SetTextColor(bDisable ? ::GetSysColor(COLOR_BTNFACE) : ::GetSysColor(COLOR_3DHIGHLIGHT));
		}
		else
		{
			if(bDisable)
			{
				pDC->SetTextColor(::GetSysColor(COLOR_3DHIGHLIGHT));

				CRect rect = rcItem;

				rect.left   += 1;
				rect.top    += 1;
				rect.right  += 1;
				rect.bottom += 1;

				pDC->DrawText(
					m_pReportCtrl->m_arraySubItems[lpDrawItemStruct->itemData].strText,
					-1,
					rect,
					DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_VCENTER|DT_END_ELLIPSIS|DT_LEFT);

				pDC->SetTextColor(::GetSysColor(COLOR_3DSHADOW));
			}
			else
				pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		}

		pDC->DrawText(
			m_pReportCtrl->m_arraySubItems[lpDrawItemStruct->itemData].strText,
			-1,
			rcItem,
			DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP|DT_VCENTER|DT_END_ELLIPSIS|DT_LEFT);
	}
	else
		pDC->FillSolidRect(rcItem, ::GetSysColor(COLOR_WINDOW));
}

BOOL CReportSubItemListCtrl::BeginDrag(CPoint pt)
{
	if(GetCount() <= 0)
		return FALSE;

	INT iItem = ItemFromPt(pt);
	if(iItem >= 0)
	{
		m_iSubItem = (INT)GetItemData(iItem);

		if(Disable(m_iSubItem))
		{
			m_iSubItem = -1;
			return FALSE;
		}

		GetClientRect(m_rcDragWnd);
		m_rcDragWnd.bottom = m_rcDragWnd.top + GetItemHeight(0);

		_tcscpy_s(m_szSubItemText, 80, m_pReportCtrl->m_arraySubItems[m_iSubItem].strText);

		m_hdiSubItem.mask = HDI_WIDTH|HDI_TEXT|HDI_FORMAT;
		m_hdiSubItem.cxy = m_rcDragWnd.Width();
		m_hdiSubItem.pszText = m_szSubItemText;
		m_hdiSubItem.cchTextMax = sizeof(m_szSubItemText);
		m_hdiSubItem.fmt = HDF_STRING|HDF_LEFT;

		m_pDragWnd = new CFHDragWnd;
		if(m_pDragWnd)
			m_pDragWnd->Create(m_rcDragWnd, &m_pReportCtrl->m_wndHeader, -2, &m_hdiSubItem);

		GetWindowRect(m_rcDropTarget1);
		m_pReportCtrl->m_wndHeader.GetWindowRect(m_rcDropTarget2);
	}

	m_iDropIndex = -1;

	return TRUE;
}

UINT CReportSubItemListCtrl::Dragging(CPoint pt)
{
	CPoint point = pt;
	point.Offset(-(m_rcDragWnd.Width()>>1), -(m_rcDragWnd.Height()>>1));

	if(m_pDragWnd != NULL)
		m_pDragWnd->SetWindowPos(
			&wndTop,
			point.x, point.y,
 			0, 0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE
		);

	if(m_rcDropTarget1.PtInRect(pt))
		return DL_MOVECURSOR;

	m_iDropIndex = (INT)m_pReportCtrl->m_wndHeader.SendMessage(HDM_SETHOTDIVIDER, TRUE, MAKELONG(pt.x, pt.y));

	if(m_rcDropTarget2.PtInRect(pt))
		return DL_MOVECURSOR;

	return DL_STOPCURSOR;
}

void CReportSubItemListCtrl::CancelDrag(CPoint /*pt*/)
{
	m_pReportCtrl->m_wndHeader.SendMessage(HDM_SETHOTDIVIDER, FALSE, -1);

	if(m_pDragWnd != NULL)
	{
		m_pDragWnd->DestroyWindow();
		m_pDragWnd = NULL;
	}

	m_iSubItem = -1;
}

void CReportSubItemListCtrl::Dropped(INT /*iSrcIndex*/, CPoint /*pt*/)
{
	m_pReportCtrl->m_wndHeader.SendMessage(HDM_SETHOTDIVIDER, FALSE, -1);

	if(m_pDragWnd != NULL)
	{
		m_pDragWnd->DestroyWindow();
		m_pDragWnd = NULL;
	}

	if(m_iSubItem >= 0)
	{
		if(!m_pReportCtrl->GetActiveSubItemCount())
			m_pReportCtrl->ActivateSubItem(m_iSubItem);
		else
			if(m_iDropIndex >= 0)
				m_pReportCtrl->ActivateSubItem(m_iSubItem, m_iDropIndex);
	}

	m_iSubItem = -1;
}

BOOL CReportSubItemListCtrl::OnEraseBkgnd(CDC* pDC) 
{
	if(!IsWindowEnabled())
	{
		CRect rect;
		GetClientRect( rect );

		pDC->FillSolidRect(rect, ::GetSysColor(COLOR_BTNFACE));
		return TRUE;
	}
	else
		return 	CDragListBox::OnEraseBkgnd(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// CReportEditCtrl

CReportEditCtrl::CReportEditCtrl(INT iItem, INT iSubItem, BOOL bButton)
{
	m_bEndEdit = FALSE;

	m_iItem = iItem;
	m_iSubItem = iSubItem;

	m_bButton = bButton;

	m_nLastKey = VK_RETURN;
}

CReportEditCtrl::~CReportEditCtrl()
{
}

BOOL CReportEditCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	if(m_bButton)
	{
		if(cs.cx > cs.cy+10)
			cs.cx -= cs.cy;
		else
			m_bButton = FALSE;
	}

	return CEdit::PreCreateWindow(cs);
}

int CReportEditCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	if(m_bButton)
	{
		CRect rect(
			lpCreateStruct->x + lpCreateStruct->cx,
			lpCreateStruct->y,
			lpCreateStruct->x + lpCreateStruct->cx + lpCreateStruct->cy,
			lpCreateStruct->y + lpCreateStruct->cy
		);
		CWnd *pParent = GetParent();
		ASSERT(pParent != NULL);

		if(!m_wndButton.Create(_T("..."), WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, rect, pParent, 0))
			return -1;

		m_wndButton.SetFont(pParent->GetFont());
	}

	return 0;
}


void CReportEditCtrl::PostNcDestroy()
{
	CEdit::PostNcDestroy();
	delete this;
}

BEGIN_MESSAGE_MAP(CReportEditCtrl, CEdit)
	//{{AFX_MSG_MAP(CReportEditCtrl)
	ON_WM_KILLFOCUS()
	ON_WM_GETDLGCODE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportEditCtrl operations

void CReportEditCtrl::BeginEdit(UINT nKey)
{
	CString str;
	GetWindowText(str);

	switch (nKey)
	{
	case VK_LBUTTON:
	case VK_RETURN:
		SetSel(str.GetLength(), -1);
		break;
	case VK_BACK:
		SetSel(str.GetLength(), -1);
		SendMessage(WM_CHAR, nKey);
		break;
	case VK_TAB:
	case VK_DOWN:
	case VK_UP:
	case VK_RIGHT:
	case VK_LEFT:
	case VK_NEXT:
	case VK_PRIOR:
	case VK_HOME:
	case VK_SPACE:
	case VK_END:
		SetSel(0,-1);
		break;
	default:
		SetSel(0,-1);
		SendMessage(WM_CHAR, nKey);
		break;
	}
}

void CReportEditCtrl::EndEdit()
{
    CString str;

    if(m_bEndEdit)
        return;

    m_bEndEdit = TRUE;
    GetWindowText(str);

	NMRVITEMEDIT nmrvie;
	memset(&nmrvie, 0, sizeof(nmrvie));

    nmrvie.hdr.hwndFrom = GetSafeHwnd();
    nmrvie.hdr.idFrom = GetDlgCtrlID();
    nmrvie.hdr.code = RVN_ENDITEMEDIT;

	nmrvie.iItem = m_iItem;
	nmrvie.iSubItem = m_iSubItem;

	nmrvie.bButton = m_bButton;

	nmrvie.hWnd = nmrvie.hdr.hwndFrom;

	nmrvie.nKey = m_nLastKey;
	nmrvie.lpszText = (LPTSTR)(LPCTSTR)str;

    CWnd* pWnd = GetOwner();
    if(pWnd != NULL)
        pWnd->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmrvie);

    PostMessage(WM_CLOSE, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CReportEditCtrl message handlers

void CReportEditCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	if(m_bButton)
	{
		if(pNewWnd->GetSafeHwnd() == m_wndButton.GetSafeHwnd())
		{
			BOOL bResult = FALSE;
			CString str;

			NMRVITEMEDIT nmrvie;
			memset(&nmrvie, 0, sizeof(nmrvie));

			CReportCtrl* pWnd = DYNAMIC_DOWNCAST(CReportCtrl, GetOwner());
			if(pWnd != NULL)
			{
				GetWindowText(str);

				nmrvie.hdr.hwndFrom = pWnd->GetSafeHwnd();
				nmrvie.hdr.idFrom = pWnd->GetDlgCtrlID();
				nmrvie.hdr.code = RVN_BUTTONCLICK;

				nmrvie.iItem = m_iItem;
				nmrvie.iSubItem = m_iSubItem;

				nmrvie.bButton = m_bButton;

				nmrvie.hWnd = GetSafeHwnd();

				VERIFY((nmrvie.lpszText = str.GetBuffer(REPORTCTRL_MAX_TEXT))!=0);

				bResult = pWnd->Notify((LPNMREPORTVIEW)&nmrvie);

				str.ReleaseBuffer();
			}

			if(bResult == TRUE)
				SetWindowText(nmrvie.lpszText);

			SetFocus();
			return;
		}
	}

	EndEdit();
}

UINT CReportEditCtrl::OnGetDlgCode()
{
    return DLGC_WANTALLKEYS;
}

BOOL CReportEditCtrl::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN &&
	   (pMsg->wParam == VK_TAB || pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
	) {
		m_nLastKey = (UINT)pMsg->wParam;

		GetParent()->SetFocus();
		return TRUE;
	}

    if(pMsg->message == WM_SYSCHAR)
        return TRUE;

	return CEdit::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CReportComboCtrl

CReportComboCtrl::CReportComboCtrl(INT iItem, INT iSubItem)
{
	m_bEndEdit = FALSE;

	m_iItem = iItem;
	m_iSubItem = iSubItem;

	m_nLastKey = VK_RETURN;
}

CReportComboCtrl::~CReportComboCtrl()
{
}

void CReportComboCtrl::PostNcDestroy()
{
	CComboBox::PostNcDestroy();
	delete this;
}

BEGIN_MESSAGE_MAP(CReportComboCtrl, CComboBox)
	//{{AFX_MSG_MAP(CReportComboCtrl)
	ON_WM_GETDLGCODE()
	ON_CONTROL_REFLECT(CBN_KILLFOCUS, OnKillfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportComboCtrl operations

void CReportComboCtrl::BeginEdit(UINT nKey)
{
	CString str;
	GetWindowText(str);

	if(GetStyle()&CBS_DROPDOWN)
	{
		switch (nKey)
		{
		case VK_LBUTTON:
		case VK_RETURN:
			break;
		default:
			PostMessage(WM_CHAR, nKey);
			break;
		}
	}
}

void CReportComboCtrl::EndEdit()
{
    CString str;

    if(m_bEndEdit)
        return;

    m_bEndEdit = TRUE;
    GetWindowText(str);

	NMRVITEMEDIT nmrvie;
	memset(&nmrvie, 0, sizeof(nmrvie));

    nmrvie.hdr.hwndFrom = GetSafeHwnd();
    nmrvie.hdr.idFrom = GetDlgCtrlID();
    nmrvie.hdr.code = RVN_ENDITEMEDIT;

	nmrvie.iItem = m_iItem;
	nmrvie.iSubItem = m_iSubItem;

	nmrvie.hWnd = nmrvie.hdr.hwndFrom;

	nmrvie.nKey = m_nLastKey;
	nmrvie.lpszText = (LPTSTR)(LPCTSTR)str;

    CWnd* pWnd = GetOwner();
    if(pWnd != NULL)
        pWnd->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmrvie);

    PostMessage(WM_CLOSE, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CReportComboCtrl message handlers

UINT CReportComboCtrl::OnGetDlgCode()
{
    return DLGC_WANTALLKEYS;
}

BOOL CReportComboCtrl::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN &&
	   (pMsg->wParam == VK_TAB || pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
	) {
		m_nLastKey = (UINT)pMsg->wParam;

		GetParent()->SetFocus();
		return TRUE;
	}

    if (pMsg->message == WM_SYSCHAR)
        return TRUE;

	return CComboBox::PreTranslateMessage(pMsg);
}

void CReportComboCtrl::OnKillfocus()
{
	EndEdit();
}

/////////////////////////////////////////////////////////////////////////////
// CReportTipCtrl

CReportTipCtrl::CReportTipCtrl()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!(::GetClassInfo(hInst, REPORTTIPCTRL_CLASSNAME, &wndcls)))
	{
		wndcls.style = CS_SAVEBITS |CS_GLOBALCLASS;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)(COLOR_INFOBK+1);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = REPORTTIPCTRL_CLASSNAME;
		if(!AfxRegisterClass(&wndcls))
			AfxThrowResourceException();
	}

	m_bLayeredWindows = InitLayeredWindows();
	m_nAlpha = 0;

    m_dwLastLButtonDown = ULONG_MAX;
    m_dwDblClickMsecs = GetDoubleClickTime();

	m_pReportCtrl = NULL;
}

CReportTipCtrl::~CReportTipCtrl()
{
}


BEGIN_MESSAGE_MAP(CReportTipCtrl, CWnd)
	//{{AFX_MSG_MAP(CReportTipCtrl)
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CReportTipCtrl message handlers

static BOOL Is_Win2000_Or_Later()
{
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof osvi);
	osvi.dwOSVersionInfoSize = sizeof osvi;

	if (!GetVersionEx((OSVERSIONINFO *)&osvi)) 
		return FALSE;

	return osvi.dwMajorVersion >= 5;
}

BOOL CReportTipCtrl::Create(CReportCtrl* pReportCtrl)
{
	ASSERT_VALID(pReportCtrl);

	m_pReportCtrl = pReportCtrl;

	DWORD dwExStyle = WS_EX_TOPMOST|WS_EX_TOOLWINDOW;
	if (Is_Win2000_Or_Later())			// WS_EX_LAYERED only supported on W2K
		dwExStyle |= WS_EX_LAYERED;		// or later. Creation fails on NT4!

	return CreateEx(
		dwExStyle,
		REPORTTIPCTRL_CLASSNAME, NULL,
		WS_BORDER|WS_POPUP,
		CRect(0, 0, 0, 0),
		pReportCtrl, 0, NULL
	);
}

BOOL CReportTipCtrl::Show(CRect rectText, LPCTSTR lpszText, CFont* pFont)
{
	BOOL bResult = FALSE;

	ASSERT(::IsWindow(GetSafeHwnd()));

	if(
		rectText.IsRectEmpty() ||
		GetFocus() == NULL
	) {
		Hide();
		return FALSE;
	}

	if( rectText != m_rectText )
	{
		Invalidate();
		UpdateWindow();

		m_rectTip.top = -1;
		m_rectTip.bottom = rectText.Height() + 1;
		m_rectTip.left = -1;
		m_rectTip.right = rectText.Width()+1;
		m_rectTip.InflateRect(m_pReportCtrl->m_iSpacing, 0);
		m_pReportCtrl->ClientToScreen(rectText);

		CClientDC dc(this);
		pFont = pFont == NULL ? m_pReportCtrl->GetFont():pFont;
		CFont *pFontDC = dc.SelectObject(pFont);

		CRect rectDisplay = rectText;
		CSize size = dc.GetTextExtent(lpszText, (int)_tcslen(lpszText));
		rectDisplay.right = rectDisplay.left + size.cx;

		if(rectDisplay.right > rectText.right)
		{
			rectDisplay.InflateRect(m_pReportCtrl->m_iSpacing, 0);

			SetWindowPos(
				&wndTop,
				rectDisplay.left, rectDisplay.top,
				rectDisplay.Width(), rectDisplay.Height(),
				SWP_NOOWNERZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE
			);

			dc.SetBkMode(TRANSPARENT);
			dc.TextOut(m_pReportCtrl->m_iSpacing-1, 1, lpszText);

			if(m_bLayeredWindows)
			{
				UpdateWindow();

				if(m_nAlpha < 255)
					SetTimer(REPORTTIPCTRL_FADE_TIMERID, REPORTTIPCTRL_FADETIME/(255/REPORTTIPCTRL_FADESTEP), NULL);

				g_lpfnSetLayeredWindowAttributes(
					GetSafeHwnd(),
					RGB(0xFF, 0, 0xFF),
					m_nAlpha,
					ULW_ALPHA
				);
			}

			bResult = TRUE;
		}
		else
			Hide();

		m_rectText = rectText;

		dc.SelectObject(pFontDC);
	}
	else
		Hide();

	return bResult;
}

void CReportTipCtrl::Hide()
{
	if (!::IsWindow(GetSafeHwnd()))
        return;

	if(m_bLayeredWindows)
	{
		m_nAlpha = 255;
		SetTimer( REPORTTIPCTRL_FADE_TIMERID, REPORTTIPCTRL_FADETIMEOUT, NULL );
	}

	if(IsWindowVisible())
		ShowWindow(SW_HIDE);
}

void CReportTipCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if(!m_rectTip.PtInRect(point))
	{
		Hide();

		ClientToScreen(&point);
		CWnd *pWnd = WindowFromPoint(point);
		if(pWnd == this)
			pWnd = m_pReportCtrl;

		INT iHitTest = (INT)pWnd->SendMessage(
			WM_NCHITTEST,
			0,
			MAKELONG(point.x,point.y)
		);

		if(iHitTest == HTCLIENT)
		{
			pWnd->ScreenToClient(&point);
			pWnd->PostMessage(
				WM_MOUSEMOVE,
				nFlags,
				MAKELONG(point.x,point.y)
			);
		}
		else
		{
		   pWnd->PostMessage(
			   WM_NCMOUSEMOVE,
			   iHitTest,
				MAKELONG(point.x,point.y)
			);
		}
	}
}

BOOL CReportTipCtrl::PreTranslateMessage(MSG* pMsg)
{
    DWORD dwTick = 0;
    BOOL bDoubleClick = FALSE;

    CWnd *pWnd;
	INT iHitTest;

	switch (pMsg->message)
	{
	case WM_LBUTTONDOWN:

        dwTick = GetTickCount();
        bDoubleClick = ((dwTick - m_dwLastLButtonDown) <= m_dwDblClickMsecs);
        m_dwLastLButtonDown = dwTick;

		// Notice fall-through

	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:

		{
		POINTS points = MAKEPOINTS(pMsg->lParam);
		POINT  point;
		point.x = points.x;
		point.y = points.y;

		ClientToScreen(&point);
		pWnd = WindowFromPoint(point);

		if(pWnd == this)
			pWnd = m_pReportCtrl;

		iHitTest = (INT)pWnd->SendMessage(
			WM_NCHITTEST,
			0,
			MAKELONG(point.x, point.y)
		);

		if(iHitTest == HTCLIENT)
		{
			pWnd->ScreenToClient(&point);
			pMsg->lParam = MAKELONG(point.x,point.y);
		}
		else
		{
			switch (pMsg->message)
			{
			case WM_LBUTTONDOWN: pMsg->message = WM_NCLBUTTONDOWN; break;
			case WM_RBUTTONDOWN: pMsg->message = WM_NCRBUTTONDOWN; break;
			case WM_MBUTTONDOWN: pMsg->message = WM_NCMBUTTONDOWN; break;
			}

			pMsg->wParam = iHitTest;
			pMsg->lParam = MAKELONG(point.x,point.y);
		}

        Hide();

        pWnd->PostMessage(
			bDoubleClick ? WM_LBUTTONDBLCLK : pMsg->message,
			pMsg->wParam,
			pMsg->lParam
		);
		return TRUE;
		}

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:

        Hide();
		m_pReportCtrl->PostMessage(
			pMsg->message,
			pMsg->wParam,
			pMsg->lParam
		);
		return TRUE;
	}

	if(GetFocus() == NULL)
	{
        Hide();
		return TRUE;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CReportTipCtrl::OnTimer(UINT_PTR nIDEvent) 
{
	if(nIDEvent == REPORTTIPCTRL_FADE_TIMERID )
	{
		ASSERT(m_bLayeredWindows == TRUE);
		if(m_nAlpha < 255)
		{
			m_nAlpha += REPORTTIPCTRL_FADESTEP;
			if( m_nAlpha >= 255 )
			{
				m_nAlpha = 255;
				KillTimer(REPORTTIPCTRL_FADE_TIMERID);
			}

			g_lpfnSetLayeredWindowAttributes(
				GetSafeHwnd(),
				RGB(0xFF, 0, 0xFF),
				m_nAlpha,
				ULW_ALPHA
			);
		}
		else
		{
			m_nAlpha = 0;
			KillTimer(REPORTTIPCTRL_FADE_TIMERID);
		}
	}

	CWnd::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
// Global utilities
//

BOOL InitLayeredWindows()
{
	if( g_lpfnUpdateLayeredWindow == NULL )
	{
		HMODULE hUser32 = GetModuleHandle(_T("USER32.DLL"));

		g_lpfnUpdateLayeredWindow =
			(lpfnUpdateLayeredWindow)GetProcAddress
			(
				hUser32,
				"UpdateLayeredWindow"
			);

		g_lpfnSetLayeredWindowAttributes =
			(lpfnSetLayeredWindowAttributes)GetProcAddress
			(
				hUser32,
				"SetLayeredWindowAttributes"
			);

		return g_lpfnUpdateLayeredWindow != NULL ? TRUE:FALSE;
	}

	return TRUE;
}

BOOL MatchString(const CString& strString, const CString& strPattern)
{
	CString strStr = strString;
	CString strPat = strPattern;
	strStr.MakeLower();
	strPat.MakeLower();

	BOOL bStopSearch = FALSE;

	while(!strString.IsEmpty() && !strPat.IsEmpty() && !bStopSearch)
		if(strPat[0] == _T('*'))
		{
			strPat = strPat.Right(strPat.GetLength() - 1);

			if(strPat.IsEmpty())
				return TRUE;

			CString strBuf = strPat;

			INT iWild = strBuf.FindOneOf(_T("*?"));

			if(iWild != -1)
				strBuf = strBuf.Left(iWild);

			INT iFound = strStr.Find(strBuf);

			if(iFound == -1)
				return FALSE;

			if(iWild == -1)
				return TRUE;

			strStr = strStr.Right(strStr.GetLength() - iFound);
		}
		else
		{
			bStopSearch = strStr[0] != strPat[0] && strPat[0] != _T('?');

			if(!bStopSearch)
			{
				strStr = strStr.Right(strStr.GetLength() - 1);
				strPat = strPat.Right(strPat.GetLength() - 1);
			}
		}

	bStopSearch = bStopSearch || !strStr.IsEmpty() && strPat.IsEmpty() || strStr.IsEmpty() && !strPat.IsEmpty();

	return !bStopSearch || bStopSearch && !strPat.IsEmpty() && strPat[0] == _T('*');
}

/////////////////////////////////////////////////////////////////////////////
// CReportHeaderCtrl

CReportHeaderCtrl::CReportHeaderCtrl()
{
}

CReportHeaderCtrl::~CReportHeaderCtrl()
{
}


BEGIN_MESSAGE_MAP(CReportHeaderCtrl, CFlatHeaderCtrl)
	//{{AFX_MSG_MAP(CReportHeaderCtrl)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportHeaderCtrl attributes

void CReportHeaderCtrl::SetMinMax(INT iPos, INT iMin, INT iMax)
{
	ASSERT(iPos < m_arrayHdrItemEx.GetSize());
	m_arrayHdrItemEx[iPos].iMinWidth = iMin;
	m_arrayHdrItemEx[iPos].iMaxWidth = iMax;
};

/////////////////////////////////////////////////////////////////////////////
// CReportHeaderCtrl messages

void CReportHeaderCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CReportCtrl* pReportCtrl = DYNAMIC_DOWNCAST(CReportCtrl, GetParent());
	if( pReportCtrl != NULL )
	{
		HDHITTESTINFO hdhti;
		hdhti.pt = point;
		
		INT iItem = (INT)::SendMessage(GetSafeHwnd(), HDM_HITTEST, 0, (LPARAM)(&hdhti));

		NMRVHEADER nmrvhdr;
		if(pReportCtrl->NotifyHdr(&nmrvhdr, RVN_HEADERRCLICK, iItem))
			return;
	}

	CFlatHeaderCtrl::OnRButtonDown(nFlags, point);
}
