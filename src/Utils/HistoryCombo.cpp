// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "stdafx.h"
#include "HistoryCombo.h"


#define MAX_HISTORY_ITEMS 10

CHistoryCombo::CHistoryCombo(BOOL bAllowSortStyle /*=FALSE*/ )
{
	m_nMaxHistoryItems = MAX_HISTORY_ITEMS;
	m_bAllowSortStyle = bAllowSortStyle;
}

CHistoryCombo::~CHistoryCombo()
{
}

BOOL CHistoryCombo::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!m_bAllowSortStyle)  //turn off CBS_SORT style
		cs.style &= ~CBS_SORT;
	return CComboBox::PreCreateWindow(cs);
}

BEGIN_MESSAGE_MAP(CHistoryCombo, CComboBox)
	//{{AFX_MSG_MAP(CHistoryCombo)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CHistoryCombo::AddString(LPCTSTR lpszString)
{
	if (m_sSection.IsEmpty() || m_sKeyPrefix.IsEmpty())
		return CComboBox::AddString(lpszString);

	int nRet = -1;
	//add the string
	CString sString(lpszString);
	sString.Trim(_T(" "));
	nRet = CComboBox::InsertString(0, sString);
	//search zhe Combo for another string like this
	//and delete it if one is found
	int nIndex = FindStringExact(0, sString);
	if (nIndex != -1 && nIndex != 0)
		DeleteString(nIndex);

	//truncate list to m_nMaxHistoryItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
	{
		DeleteString(m_nMaxHistoryItems);
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
	CWinApp* pApp = AfxGetApp();

	int n = 0;
	CString sText;
	do
	{
		//keys are of form <lpszKeyPrefix><entrynumber>
		CString sKey;
		sKey.Format(_T("%s%d"), m_sKeyPrefix, n++);
		sText = pApp->GetProfileString(m_sSection, sKey);
		if (!sText.IsEmpty())
			CComboBox::AddString(sText);
	} while (!sText.IsEmpty() && n < m_nMaxHistoryItems);

	SetCurSel(0);

	return sText;
}

void CHistoryCombo::SaveHistory()
{
	if (m_sSection.IsEmpty())
		return;

	CWinApp* pApp = AfxGetApp();
	ASSERT(pApp);

	//add the current item to the history
	CString sCurItem;
	GetWindowText(sCurItem);
	sCurItem.Trim();
	if (!sCurItem.IsEmpty())
		AddString(sCurItem);
	//save history to registry/inifile
	int nMax = min(GetCount(), m_nMaxHistoryItems + 1);
	for (int n = 0; n < nMax; n++)
	{
		CString sKey;
		sKey.Format(_T("%s%d"), m_sKeyPrefix, n);
		CString sText;
		GetLBText(n, sText);
		pApp->WriteProfileString(m_sSection, sKey, sText);
	}
	//remove items exceeding the max number of history items
	for (n = nMax; ; n++)
	{
		CString sKey;
		sKey.Format(_T("%s%d"), m_sKeyPrefix, n);
		CString sText = pApp->GetProfileString(m_sSection, sKey);
		if (sText.IsEmpty())
			break;
		pApp->WriteProfileString(m_sSection, sKey, NULL); // remove entry
	}
}

void CHistoryCombo::ClearHistory(BOOL bDeleteRegistryEntries/*=TRUE*/)
{
	ResetContent();
	if (! m_sSection.IsEmpty() && bDeleteRegistryEntries)
	{
		//remove profile entries
		CWinApp* pApp = AfxGetApp();
		ASSERT(pApp);
		CString sKey;
		for (int n = 0; ; n++)
		{
			sKey.Format(_T("%s%d"), m_sKeyPrefix, n);
			CString sText = pApp->GetProfileString(m_sSection, sKey);
			if (sText.IsEmpty())
				break;
			pApp->WriteProfileString(m_sSection, sKey, NULL); // remove entry
		}
	}
}

void CHistoryCombo::SetMaxHistoryItems(int nMaxItems)
{
	m_nMaxHistoryItems = nMaxItems;

	//truncate list to nMaxItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
		DeleteString(m_nMaxHistoryItems);
}

CString CHistoryCombo::GetString()
{
	CString str;
	int sel;
	sel = GetCurSel();
	GetLBText(sel, str.GetBuffer(GetLBTextLen(sel)));
	str.ReleaseBuffer();
	return str;
}




