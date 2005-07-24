////////////////////////////////////////////////////////////////////////////
//	File:		ReportOptionsCtrl.cpp
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
//	The ideas behind this control and its interface are based on the
//  CTreeOptionsCtrl and associated classes by PJ Naughther. The control
//	is derived from CReportCtrl and demonstrates the power of the
//  CReportControl when deriving new behavior.
//
//	Version history
//
//	1.0.0	- Initial release.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ReportOptionsCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CReportOptionsCtrl, CEdit)

CReportOptionsCtrl::CReportOptionsCtrl()
{
	// Register the window class if it has not already been registered.
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!(::GetClassInfo(hInst, REPORTOPTIONSCTRL_CLASSNAME, &wndclass)))
	{
        wndclass.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
		wndclass.lpfnWndProc = ::DefWindowProc;
		wndclass.cbClsExtra = wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInst;
		wndclass.hIcon = NULL;
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = REPORTOPTIONSCTRL_CLASSNAME;

		if (!AfxRegisterClass(&wndclass))
			AfxThrowResourceException();
	}

	m_hControl = NULL;
	m_lpEdit = NULL;
	m_lpCombo = NULL;
}

CReportOptionsCtrl::~CReportOptionsCtrl()
{
}

BEGIN_MESSAGE_MAP(CReportOptionsCtrl, CReportCtrl)
	//{{AFX_MSG_MAP(CReportOptionsCtrl)
	ON_WM_KEYDOWN()
	ON_MESSAGE(WM_USER, OnCloseControl)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCtrl attributes

BOOL CReportOptionsCtrl::IsGroup(HTREEITEM hItem)
{
	if(HasChildren(hItem))
		return TRUE;

	return FALSE;
}

BOOL CReportOptionsCtrl::IsCheckBox(HTREEITEM hItem)
{
	RVITEM rvi;
	return GetCheck(hItem, &rvi);
}

BOOL CReportOptionsCtrl::IsRadioButton(HTREEITEM hItem)
{
	RVITEM rvi;
	return GetRadio(hItem, &rvi);
}

BOOL CReportOptionsCtrl::IsEditable(HTREEITEM hItem)
{
	INT iItem = GetItemIndex(hItem);
	if(iItem != RVI_INVALID && GetItemData(iItem) != 0)
		return TRUE;

	return FALSE;
}

BOOL CReportOptionsCtrl::IsDisabled(HTREEITEM hItem)
{
	RVITEM rvi;
	if(GetCheck(hItem, &rvi))
		return rvi.iCheck&2 ? TRUE:FALSE;

	if(GetRadio(hItem, &rvi))
		return rvi.iCheck&2 ? TRUE:FALSE;

	return FALSE;
}

BOOL CReportOptionsCtrl::SetCheckBox(HTREEITEM hItem, BOOL bCheck)
{
	RVITEM rvi;
	if(GetCheck(hItem, &rvi) == TRUE)
	{
		rvi.iCheck = (rvi.iCheck&~1);
		rvi.iCheck |= (bCheck ? 1:0);

		rvi.nMask = RVIM_CHECK;
		VERIFY(SetItem(&rvi));
		return TRUE;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::GetCheckBox(HTREEITEM hItem, BOOL& bCheck)
{
	RVITEM rvi;

	if(GetCheck(hItem, &rvi) == TRUE)
	{
		bCheck = rvi.iCheck&1 ? TRUE:FALSE;
		return TRUE;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::SetRadioButton(HTREEITEM hParent, INT iIndex)
{
	INT i = 0;
	HTREEITEM hItem = GetNextItem(hParent, RVGN_CHILD);

	while(hItem != NULL && i<iIndex)
	{
		hItem = GetNextItem(hItem, RVGN_NEXT);
		i++;
	}

	i = 0;
	hItem = GetNextItem(hParent, RVGN_CHILD);

	while(hItem != NULL)
	{
		RVITEM rvi;
		if(GetRadio(hItem, &rvi) == TRUE)
		{
			if(i == iIndex)
				rvi.iCheck |= 1;
			else
				rvi.iCheck &= ~1;

			rvi.nMask = RVIM_CHECK;
			VERIFY(SetItem(&rvi));
		}

		hItem = GetNextItem(hItem, RVGN_NEXT);
		i++;
	}

	return TRUE;
}

BOOL CReportOptionsCtrl::SetRadioButton(HTREEITEM hItem)
{
	INT iIndex = -1;
	HTREEITEM hParent = GetNextItem(hItem, RVGN_PARENT);

	ASSERT(hItem != NULL);
	while(hItem != NULL)
	{
		hItem = GetNextItem(hItem, RVGN_PREVIOUS);
		iIndex++;
	}

	return SetRadioButton(hParent, iIndex);
}

BOOL CReportOptionsCtrl::GetRadioButton(HTREEITEM hParent, INT& iIndex, HTREEITEM& /*hCheckItem*/)
{
	HTREEITEM hItem = GetNextItem(hParent, RVGN_CHILD);

	iIndex = 0;
	while(hItem != NULL)
	{
		RVITEM rvi;
		if(GetRadio(hItem, &rvi) == TRUE)
		{
			if(rvi.iCheck&1)
				return TRUE;
		}

		hItem = GetNextItem(hItem, RVGN_NEXT);
		iIndex++;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::GetRadioButton(HTREEITEM hItem, BOOL& bCheck)
{
	RVITEM rvi;
	if(GetRadio(hItem, &rvi) == TRUE)
	{
		bCheck = rvi.iCheck&1 ? TRUE:FALSE;
		return TRUE;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::SetGroupEnable(HTREEITEM hParent, BOOL bEnable)
{
	HTREEITEM hItem = GetNextItem(hParent, RVGN_CHILD);

	if(hItem == NULL)
		return FALSE;

	while(hItem != NULL)
	{
		SetGroupEnable(hItem, bEnable);
		SetCheckBoxEnable(hItem, bEnable);
		SetRadioButtonEnable(hItem, bEnable);

		hItem = GetNextItem(hItem, RVGN_NEXT);
	}

	return TRUE;
}

BOOL CReportOptionsCtrl::GetGroupEnable(HTREEITEM hParent, BOOL& bEnable)
{
	HTREEITEM hItem = GetNextItem(hParent, RVGN_CHILD);

	if(hItem == NULL)
		return FALSE;

	bEnable = TRUE;

	while(hItem != NULL && bEnable == TRUE)
	{
		GetGroupEnable(hItem, bEnable);
		if(bEnable == FALSE)
			return TRUE;

		GetCheckBoxEnable(hItem, bEnable);
		if(bEnable == FALSE)
			return TRUE;

		GetRadioButtonEnable(hItem, bEnable);
		if(bEnable == FALSE)
			return TRUE;

		hItem = GetNextItem(hItem, bEnable);
	}

	return TRUE;
}

BOOL CReportOptionsCtrl::SetCheckBoxEnable(HTREEITEM hItem, BOOL bEnable)
{
	RVITEM rvi;
	if(GetCheck(hItem, &rvi) == TRUE)
	{
		rvi.iCheck = (rvi.iCheck&~2)|(bEnable ? 0:2);
		rvi.nMask = RVIM_CHECK;
		return SetItem(&rvi);
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::GetCheckBoxEnable(HTREEITEM hItem, BOOL& bEnable)
{
	RVITEM rvi;
	if(GetCheck(hItem, &rvi) == TRUE)
	{
		bEnable = rvi.iCheck&2 ? FALSE:TRUE;
		return TRUE;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::SetRadioButtonEnable(HTREEITEM hItem, BOOL bEnable)
{
	RVITEM rvi;
	if(GetRadio(hItem, &rvi) == TRUE)
	{
		rvi.iCheck = (rvi.iCheck&~2)|(bEnable ? 0:2);
		rvi.nMask = RVIM_CHECK;
		return SetItem(&rvi);
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::GetRadioButtonEnable(HTREEITEM hItem, BOOL& bEnable)
{
	RVITEM rvi;
	if(GetRadio(hItem, &rvi) == TRUE)
	{
		bEnable = rvi.iCheck&2 ? FALSE:TRUE;
		return TRUE;
	}

	return FALSE;
}

CString CReportOptionsCtrl::GetEditText(HTREEITEM hItem)
{
	ASSERT(IsEditable(hItem) == TRUE);

	INT iItem = GetItemIndex(hItem);
	ASSERT(iItem != RVI_INVALID);

	CString str = GetItemText(iItem, 0);

	INT iOffset = str.Find(_T(": "));
	if(iOffset >= 0)
		str.Delete(0, iOffset+2);
	else
		str.Empty();

	return str;
}

void CReportOptionsCtrl::SetEditText(HTREEITEM hItem, LPCTSTR lpszText)
{
	INT iItem = GetItemIndex(hItem);
	ASSERT(iItem != RVI_INVALID);

	UpdateText(iItem, lpszText);
}

CString CReportOptionsCtrl::GetComboText(HTREEITEM hItem)
{
	return GetEditText(hItem);
}

void CReportOptionsCtrl::SetComboText(HTREEITEM hItem, LPCTSTR lpszText)
{
	SetEditText(hItem, lpszText);
}

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCtrl operations

HTREEITEM CReportOptionsCtrl::InsertItem(UINT nID, INT iImage, HTREEITEM hParent)
{
	CString str;
	str.LoadString(nID);

	return InsertItem(str, iImage, hParent);
}

HTREEITEM CReportOptionsCtrl::InsertItem(LPCTSTR lpszItem, INT iImage, HTREEITEM hParent)
{
	return CReportCtrl::InsertItem(lpszItem, iImage, -1, -1, hParent);
}

HTREEITEM CReportOptionsCtrl::InsertGroup(UINT nID, INT iImage, HTREEITEM hParent)
{
	CString str;
	str.LoadString(nID);

	return InsertGroup(str, iImage, hParent);
}

HTREEITEM CReportOptionsCtrl::InsertGroup(LPCTSTR lpszItem, INT iImage, HTREEITEM hParent)
{
	return CReportCtrl::InsertItem(lpszItem, iImage, -1, -1, hParent);
}

HTREEITEM CReportOptionsCtrl::InsertCheckBox(UINT nID, HTREEITEM hParent, BOOL bCheck)
{
	CString str;
	str.LoadString(nID);

	return InsertCheckBox(str, hParent, bCheck);
}

HTREEITEM CReportOptionsCtrl::InsertCheckBox(LPCTSTR lpszItem, HTREEITEM hParent, BOOL bCheck)
{
	return CReportCtrl::InsertItem(lpszItem, -1, bCheck ? 1:0, -1, hParent);
}

HTREEITEM CReportOptionsCtrl::InsertRadioButton(UINT nID, HTREEITEM hParent, BOOL bCheck)
{
	CString str;
	str.LoadString(nID);

	return InsertRadioButton(str, hParent, bCheck);
}

HTREEITEM CReportOptionsCtrl::InsertRadioButton(LPCTSTR lpszItem, HTREEITEM hParent, BOOL bCheck)
{
	return CReportCtrl::InsertItem(lpszItem, -1, 4 + (bCheck ? 1:0), -1, hParent);
}

BOOL CReportOptionsCtrl::AddEditBox(HTREEITEM hItem, LPCTSTR lpszText, CRuntimeClass* lpRuntimeClass)
{
	INT iItem = VerifyText(hItem);
	if(iItem != RVI_INVALID && !GetItemData(iItem))
	{
		if(lpRuntimeClass == NULL)
			lpRuntimeClass = RUNTIME_CLASS(CReportOptionsEdit);

		if(SetItemData(iItem, (DWORD_PTR)lpRuntimeClass) == TRUE)
		{
			UpdateText(iItem, lpszText);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::AddComboBox(HTREEITEM hItem, LPCTSTR lpszText, CRuntimeClass* lpRuntimeClass)
{
	INT iItem = VerifyText(hItem);
	if(iItem != RVI_INVALID && !GetItemData(iItem))
	{
		if(lpRuntimeClass == NULL)
			lpRuntimeClass = RUNTIME_CLASS(CReportOptionsCombo);

		if(SetItemData(iItem, (DWORD_PTR)lpRuntimeClass) == TRUE)
		{
			UpdateText(iItem, lpszText);
			return TRUE;
		}
	}

	return FALSE;
}

void CReportOptionsCtrl::DeleteItem(HTREEITEM hItem)
{
	VERIFY(CReportCtrl::DeleteItem(hItem) == TRUE);
}

void CReportOptionsCtrl::SelectItem(HTREEITEM hItem)
{
	SetSelection(hItem);
}

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCtrl implementation

BOOL CReportOptionsCtrl::Create()
{
	if(CReportCtrl::Create() == TRUE)
	{
		ASSERT(m_dwStyle&RVS_TREEVIEW);
		ModifyStyle(0, RVS_SINGLESELECT|RVS_NOHEADER|RVS_NOSORT);

		CRect rect;
		GetClientRect(rect);

		RVSUBITEM rvs;
		rvs.lpszText = _T("");
		rvs.iWidth = rect.Width();
		DefineSubItem(0, &rvs);
		ActivateSubItem(0, 0);

		return TRUE;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::CreateRuntimeClass(HTREEITEM hItem)
{
	INT iItem = GetItemIndex(hItem);
	ASSERT(iItem != RVI_INVALID);

	CFont* pFont = GetFont();
	CRuntimeClass* pRuntimeClass = (CRuntimeClass*)GetItemData(iItem);

	m_strControl = GetEditText(hItem);
	CString strItem = StripText(iItem);

	if(pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CReportOptionsEdit)))
	{
		ASSERT(m_lpEdit == NULL);

		VERIFY((m_lpEdit = (CReportOptionsEdit*)pRuntimeClass->CreateObject())!=0);
		ASSERT(m_lpEdit->IsKindOf(RUNTIME_CLASS(CReportOptionsEdit)));

		SetItemText(iItem, 0, strItem);

		CRect rect;
		VERIFY(MeasureItem(iItem, 0, rect, TRUE));
		INT iWidth = rect.Width();
		VERIFY(GetItemRect(iItem, 0, rect, RVIR_TEXT));

		rect.left += iWidth - ::GetSystemMetrics(SM_CXEDGE)*2;
		rect.right = rect.left + m_lpEdit->GetWidth();

		m_lpEdit->m_hItem = hItem;

		m_lpEdit->CreateEx(WS_EX_CLIENTEDGE, _T("Edit"), m_strControl, m_lpEdit->GetWindowStyle(), rect, this, REPORTOPTIONSCTRL_CONTROL_ID);
		m_lpEdit->SetFont(pFont);
		m_lpEdit->SetSel(0, -1);
		m_lpEdit->SetFocus();

		m_hControl = hItem;
		return TRUE;
	}

	if(pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CReportOptionsCombo)))
	{
		ASSERT(m_lpCombo == NULL);

		VERIFY((m_lpCombo = (CReportOptionsCombo*)pRuntimeClass->CreateObject())!=0);
		ASSERT(m_lpCombo->IsKindOf(RUNTIME_CLASS(CReportOptionsCombo)));

		SetItemText(iItem, 0, strItem);

		CRect rect;
		VERIFY(MeasureItem(iItem, 0, rect, TRUE));
		INT iWidth = rect.Width();
		VERIFY(GetItemRect(iItem, 0, rect, RVIR_TEXT));

		rect.left += iWidth - ::GetSystemMetrics(SM_CXEDGE)*2;
		rect.right = rect.left + m_lpCombo->GetWidth();
		rect.bottom += m_lpCombo->GetHeight();

		m_lpCombo->m_hItem = hItem;

		m_lpCombo->Create(m_lpCombo->GetWindowStyle(), rect, this, REPORTOPTIONSCTRL_CONTROL_ID);
		m_lpCombo->SetFont(pFont);
		m_lpCombo->SelectString(-1, m_strControl);
		m_lpCombo->SetFocus();

		m_hControl = hItem;
		return TRUE;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::DeleteRuntimeClass(BOOL bUpdateFromControl, BOOL /*bNotify*/)
{
	if(m_hControl == NULL)
		return FALSE;

	HTREEITEM hParent = GetNextItem(m_hControl, RVGN_PARENT);

	if(m_lpEdit != NULL)
	{
		ASSERT(m_hControl != NULL);

		if(bUpdateFromControl == TRUE)
		{
			m_lpEdit->GetWindowText(m_strControl);
			NotifyParent(ROCN_ITEMCHANGED, hParent, m_hControl, -1);
		}

		m_lpEdit->m_bIgnoreLostFocus = TRUE;
		m_lpEdit->DestroyWindow();
		delete m_lpEdit;
		m_lpEdit = NULL;

		INT iItem = GetItemIndex(m_hControl);
		ASSERT(iItem != RVI_INVALID);

		UpdateText(iItem, m_strControl);

		m_hControl = NULL;
		return TRUE;
	}
	else if(m_lpCombo != NULL)
	{
		ASSERT(m_hControl != NULL);

		if(bUpdateFromControl == TRUE)
		{
			m_lpCombo->GetWindowText(m_strControl);
			NotifyParent(ROCN_ITEMCHANGED, hParent, m_hControl, -1);
		}

		m_lpCombo->m_bIgnoreLostFocus = TRUE;
		m_lpCombo->DestroyWindow();
		delete m_lpCombo;
		m_lpCombo= NULL;

		INT iItem = GetItemIndex(m_hControl);
		ASSERT(iItem != RVI_INVALID);

		UpdateText(iItem, m_strControl);

		m_hControl = NULL;
		return TRUE;
	}

	return FALSE;
}

BOOL CReportOptionsCtrl::GetCheck(HTREEITEM hItem, LPRVITEM lprvi)
{
	VERIFY((lprvi->iItem = GetItemIndex(hItem)) != RVI_INVALID);
	lprvi->iSubItem = 0;

	if(GetItem(lprvi) == TRUE && lprvi->iCheck >= 0 && lprvi->iCheck <= 3)
		return TRUE;

	return FALSE;
}

BOOL CReportOptionsCtrl::GetRadio(HTREEITEM hItem, LPRVITEM lprvi)
{
	VERIFY((lprvi->iItem = GetItemIndex(hItem)) != RVI_INVALID);
	lprvi->iSubItem = 0;

	if(GetItem(lprvi) == TRUE && lprvi->iCheck >= 4 && lprvi->iCheck <= 7)
		return TRUE;

	return FALSE;
}

INT CReportOptionsCtrl::VerifyText(HTREEITEM hItem)
{
	TCHAR sz[REPORTCTRL_MAX_TEXT];

	RVITEM rvi;
	rvi.iItem = GetItemIndex(hItem);
	rvi.iSubItem = 0;
	rvi.lpszText = sz;
	rvi.iTextMax = REPORTCTRL_MAX_TEXT;
	rvi.nMask = RVIM_TEXT;
	if(GetItem(&rvi) == TRUE && rvi.nMask&RVIM_TEXT)
	{
		if(_tcschr(sz, _T(':')) == NULL)
			return rvi.iItem;
	}

	return RVI_INVALID;
}

void CReportOptionsCtrl::UpdateText(INT iItem, LPCTSTR lpszText)
{
	CString str = StripText(iItem);
	str += _T(": ");
	str += lpszText;
	VERIFY(SetItemText(iItem, 0, str));
}

CString CReportOptionsCtrl::StripText(INT iItem)
{
	CString str = GetItemText(iItem, 0);
	INT iOffset = str.Find(_T(": "));
	if(iOffset >= 0)
		str.Delete(iOffset, str.GetLength() - iOffset);

	return str;
}

BOOL CReportOptionsCtrl::Notify(LPNMREPORTVIEW lpnmrv)
{
	switch(lpnmrv->hdr.code)
	{
	case RVN_ITEMCLICK:
		if(lpnmrv->iItem >= RVI_FIRST)
		{
			DeleteRuntimeClass(FALSE, FALSE);

			HTREEITEM hItem = lpnmrv->hItem;
			HTREEITEM hParent = GetNextItem(hItem, RVGN_PARENT);

			if(IsCheckBox(hItem))
			{
				BOOL bCheck;
				if(GetCheckBox(hItem, bCheck) == TRUE)
				{
					ASSERT(hParent != NULL);
					if(IsDisabled(hItem) == FALSE && SetCheckBox(hItem, !bCheck) == TRUE)
						NotifyParent(ROCN_CHECKCHANGED, hParent, hItem, !bCheck);
				}
			}
			else if(IsRadioButton(hItem))
			{
				ASSERT(hParent != NULL);
				if(IsDisabled(hItem) == FALSE && SetRadioButton(hItem) == TRUE)
				{
					INT iIndex;
					GetRadioButton(hParent, iIndex, hItem);
					NotifyParent(ROCN_RADIOCHANGED, hParent, hItem, iIndex);
				}
			}
		}
		break;

	case RVN_ITEMDBCLICK:
		if(lpnmrv->iItem >= RVI_FIRST)
		{
			if(IsEditable(lpnmrv->hItem))
			{
				CreateRuntimeClass(lpnmrv->hItem);
			}
		}
		break;

	case RVN_KEYDOWN:
		break;

	default:
		DeleteRuntimeClass(FALSE, FALSE);
		break;
	}

	return CReportCtrl::Notify(lpnmrv);
}

BOOL CReportOptionsCtrl::NotifyParent(UINT nCode, HTREEITEM hParent, HTREEITEM hChild, INT iIndex)
{
	BOOL bResult = FALSE;

	NMREPORTOPTIONCTRL nmroc;
	nmroc.hdr.hwndFrom = GetSafeHwnd();
	nmroc.hdr.idFrom = GetDlgCtrlID();
	nmroc.hdr.code = nCode;

	nmroc.hParent = hParent;
	nmroc.hChild = hChild;
	nmroc.iIndex = iIndex;

	CWnd* pWnd = GetParent();
	if(pWnd != NULL)
		bResult = !!pWnd->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmroc);

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCtrl messages

void CReportOptionsCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(nChar == VK_SPACE)
	{
		DeleteRuntimeClass(FALSE, FALSE);

		INT iItem = GetItemFromRow(GetCurrentFocus());
		if(iItem >= RVI_FIRST)
		{
			HTREEITEM hItem = GetItemHandle(iItem);
			HTREEITEM hParent = GetNextItem(hItem, RVGN_PARENT);

			if(IsEditable(hItem))
			{
				CreateRuntimeClass(hItem);
			}
			else if(IsCheckBox(hItem))
			{
				BOOL bCheck;
				if(GetCheckBox(hItem, bCheck) == TRUE)
				{
					ASSERT(hParent != NULL);
					if(IsDisabled(hItem) == FALSE && SetCheckBox(hItem, !bCheck) == TRUE)
						NotifyParent(ROCN_CHECKCHANGED, hParent, hItem, !bCheck);
				}
			}
			else if(IsRadioButton(hItem))
			{
				ASSERT(hParent != NULL);
				if(IsDisabled(hItem) == FALSE && SetRadioButton(hItem) == TRUE)
				{
					INT iIndex;
					GetRadioButton(hParent, iIndex, hItem);
					NotifyParent(ROCN_RADIOCHANGED, hParent, hItem, iIndex);
				}
			}
		}
	}
	
	CReportCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

LRESULT CReportOptionsCtrl::OnCloseControl(WPARAM wParam, LPARAM /*lParam*/)
{
	if(m_hControl != NULL)
		DeleteRuntimeClass(wParam == VK_ESCAPE ? FALSE:TRUE, TRUE);

	return 0;
}

void CReportOptionsCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	DeleteRuntimeClass(FALSE, FALSE);

	CReportCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CReportOptionsCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	DeleteRuntimeClass(FALSE, FALSE);
	
	CReportCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsEdit

IMPLEMENT_DYNCREATE(CReportOptionsEdit, CEdit)

CReportOptionsEdit::CReportOptionsEdit()
{
	m_bIgnoreLostFocus = FALSE;
}

CReportOptionsEdit::~CReportOptionsEdit()
{
}


BEGIN_MESSAGE_MAP(CReportOptionsEdit, CEdit)
	//{{AFX_MSG_MAP(CReportOptionsEdit)
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsEdit message handlers

BOOL CReportOptionsEdit::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN &&
	   (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
	) {
		CReportOptionsCtrl* pWnd = DYNAMIC_DOWNCAST(CReportOptionsCtrl, GetParent());
		ASSERT(pWnd != NULL);

		pWnd->SendMessage(WM_USER, pMsg->wParam, (LPARAM)m_hWnd);
		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}

void CReportOptionsEdit::OnKillFocus(CWnd* /*pNewWnd*/) 
{
	if(m_bIgnoreLostFocus == FALSE)
	{
		CReportOptionsCtrl* pWnd = DYNAMIC_DOWNCAST(CReportOptionsCtrl, GetParent());
		ASSERT(pWnd != NULL);

		pWnd->SendMessage(WM_USER, 0, (LPARAM)m_hWnd);
	}

	m_bIgnoreLostFocus = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCombo

IMPLEMENT_DYNCREATE(CReportOptionsCombo, CComboBox)

CReportOptionsCombo::CReportOptionsCombo()
{
	m_bIgnoreLostFocus = FALSE;
}

CReportOptionsCombo::~CReportOptionsCombo()
{
}


BEGIN_MESSAGE_MAP(CReportOptionsCombo, CComboBox)
	//{{AFX_MSG_MAP(CReportOptionsCombo)
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportOptionsCombo message handlers

BOOL CReportOptionsCombo::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN &&
	   (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
	) {
		CReportOptionsCtrl* pWnd = DYNAMIC_DOWNCAST(CReportOptionsCtrl, GetParent());
		ASSERT(pWnd != NULL);

		pWnd->SendMessage(WM_USER, pMsg->wParam, (LPARAM)m_hWnd);
		return TRUE;
	}
	
	return CComboBox::PreTranslateMessage(pMsg);
}

void CReportOptionsCombo::OnKillFocus(CWnd* /*pNewWnd*/) 
{
	if(m_bIgnoreLostFocus == FALSE)
	{
		CReportOptionsCtrl* pWnd = DYNAMIC_DOWNCAST(CReportOptionsCtrl, GetParent());
		ASSERT(pWnd != NULL);

		pWnd->SendMessage(WM_USER, 0, (LPARAM)m_hWnd);
	}

	m_bIgnoreLostFocus = FALSE;
}
