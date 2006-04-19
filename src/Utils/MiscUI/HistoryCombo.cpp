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
#include "stdafx.h"
#include "HistoryCombo.h"
#include "SysImageList.h"
#include "registry.h"

#define MAX_HISTORY_ITEMS 25

CHistoryCombo::CHistoryCombo(BOOL bAllowSortStyle /*=FALSE*/ )
{
	m_nMaxHistoryItems = MAX_HISTORY_ITEMS;
	m_bAllowSortStyle = bAllowSortStyle;
	m_bURLHistory = FALSE;
	m_bPathHistory = FALSE;
}

CHistoryCombo::~CHistoryCombo()
{
}

BOOL CHistoryCombo::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!m_bAllowSortStyle)  //turn off CBS_SORT style
		cs.style &= ~CBS_SORT;
	cs.style |= CBS_AUTOHSCROLL;
	return CComboBoxEx::PreCreateWindow(cs);
}

BOOL CHistoryCombo::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		int nVirtKey = (int) pMsg->wParam;
		if (nVirtKey == VK_RETURN)
			return OnReturnKeyPressed();
	}

	return CComboBoxEx::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CHistoryCombo, CComboBoxEx)
	//{{AFX_MSG_MAP(CHistoryCombo)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CHistoryCombo::AddString(CString str, INT_PTR pos)
{
	if (str.IsEmpty())
		return -1;
	
	COMBOBOXEXITEM cbei;
	ZeroMemory(&cbei, sizeof cbei);
	cbei.mask = CBEIF_TEXT;

	if (pos < 0)
        cbei.iItem = GetCount();
	else
		cbei.iItem = pos;

	str.Trim(_T(" "));
	CString combostring = str;
	combostring.Replace('\r', ' ');
	combostring.Replace('\n', ' ');
	cbei.pszText = const_cast<LPTSTR>(combostring.GetString());

	if (m_bURLHistory)
	{
		cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(str);
		if (cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex())
		{
			if (str.Left(5) == _T("http:"))
				cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(_T(".html"));
			else if (str.Left(6) == _T("https:"))
				cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(_T(".shtml"));
			else if (str.Left(5) == _T("file:"))
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
			else if (str.Left(4) == _T("svn:"))
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
			else if (str.Left(8) == _T("svn+ssh:"))
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
		}
		cbei.iSelectedImage = cbei.iImage;
		cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	}
	if (m_bPathHistory)
	{
		cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(str);
		if (cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex())
		{
			cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
		}
		cbei.iSelectedImage = cbei.iImage;
		cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	}
	int nRet = InsertItem(&cbei);
	if (nRet >= 0)
		m_arEntries.InsertAt(nRet, str);

	//search the Combo for another string like this
	//and delete it if one is found
	str.Trim();
	int nIndex = FindStringExact(0, combostring);
	if (nIndex != -1 && nIndex != nRet)
	{
		DeleteItem(nIndex);
		m_arEntries.RemoveAt(nIndex);
	}

	//truncate list to m_nMaxHistoryItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
	{
		DeleteItem(m_nMaxHistoryItems);
		m_arEntries.RemoveAt(m_nMaxHistoryItems);
	}

	SetCurSel(nRet);
	return nRet;
}

CString CHistoryCombo::LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix)
{
	if (lpszSection == NULL || lpszKeyPrefix == NULL || *lpszSection == '\0')
		return _T("");

	m_sSection = lpszSection;
	m_sKeyPrefix = lpszKeyPrefix;

	int n = 0;
	CString sText;
	do
	{
		//keys are of form <lpszKeyPrefix><entrynumber>
		CString sKey;
		sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n++);
		sText = CRegString(sKey);
		if (!sText.IsEmpty())
			AddString(sText);
	} while (!sText.IsEmpty() && n < m_nMaxHistoryItems);

	SetCurSel(-1);

	ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, 0);

	// need to resize the control for correct display
	CRect rect;
	GetWindowRect(rect);
	GetParent()->ScreenToClient(rect);
	MoveWindow(rect.left, rect.top, rect.Width(), 100);

	return sText;
}

void CHistoryCombo::SaveHistory()
{
	if (m_sSection.IsEmpty())
		return;

	//add the current item to the history
	CString sCurItem;
	GetWindowText(sCurItem);
	sCurItem.Trim();
	if (!sCurItem.IsEmpty())
		AddString(sCurItem, 0);
	//save history to registry/inifile
	int nMax = min(GetCount(), m_nMaxHistoryItems + 1);
	for (int n = 0; n < nMax; n++)
	{
		CString sKey;
		sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
		CRegString regkey = CRegString(sKey);
		regkey = m_arEntries.GetAt(n);
	}
	//remove items exceeding the max number of history items
	for (int n = nMax; ; n++)
	{
		CString sKey;
		sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
		CRegString regkey = CRegString(sKey);
		CString sText = regkey;
		if (sText.IsEmpty())
			break;
		regkey.removeValue(); // remove entry
	}
}

void CHistoryCombo::ClearHistory(BOOL bDeleteRegistryEntries/*=TRUE*/)
{
	ResetContent();
	if (! m_sSection.IsEmpty() && bDeleteRegistryEntries)
	{
		//remove profile entries
		CString sKey;
		for (int n = 0; ; n++)
		{
			sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
			CRegString regkey = CRegString(sKey);
			CString sText = regkey;
			if (sText.IsEmpty())
				break;
			regkey.removeValue(); // remove entry
		}
	}
}

void CHistoryCombo::SetURLHistory(BOOL bURLHistory)
{
	m_bURLHistory = bURLHistory;

	if (m_bURLHistory)
	{
		HWND hwndEdit;
		// use for ComboEx
		hwndEdit = (HWND)::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0);
		if (NULL == hwndEdit)
		{
			// Try the unofficial way of getting the edit control CWnd*
			CWnd* pWnd = this->GetDlgItem(1001);
			if(pWnd)
			{
				hwndEdit = pWnd->GetSafeHwnd();
			}
		} // if (NULL == hwndEdit) 
		if (hwndEdit)
			SHAutoComplete(hwndEdit, SHACF_URLALL);
	} // if (bUseShellURLHistory) 

	SetImageList(&SYS_IMAGE_LIST());
}

void CHistoryCombo::SetPathHistory(BOOL bPathHistory)
{
	m_bPathHistory = bPathHistory;

	if (m_bPathHistory)
	{
		HWND hwndEdit;
		// use for ComboEx
		hwndEdit = (HWND)::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0);
		if (NULL == hwndEdit)
		{
			//if not, try the old standby
			if(hwndEdit==NULL)
			{
				CWnd* pWnd = this->GetDlgItem(1001);
				if(pWnd)
				{
					hwndEdit = pWnd->GetSafeHwnd();
				}
			} // if(hwndEdit==NULL) 
		} // if (NULL == hwndEdit) 
		if (hwndEdit)
			SHAutoComplete(hwndEdit, SHACF_FILESYSTEM);
	} // if (bUseShellURLHistory) 

	SetImageList(&SYS_IMAGE_LIST());
}

void CHistoryCombo::SetMaxHistoryItems(int nMaxItems)
{
	m_nMaxHistoryItems = nMaxItems;

	//truncate list to nMaxItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
		DeleteString(m_nMaxHistoryItems);
}

CString CHistoryCombo::GetString() const
{
	CString str;
	int sel;
	sel = GetCurSel();
	if (sel == CB_ERR)
	{
		GetWindowText(str);
		return str;
	}
	if ((m_bURLHistory)||(m_bPathHistory))
	{
		//URL and path history comboboxes are editable, so get
		//the string directly from the combobox
		GetLBText(sel, str.GetBuffer(GetLBTextLen(sel)));
		str.ReleaseBuffer();
		return str;
	}
	return m_arEntries.GetAt(sel);
}




