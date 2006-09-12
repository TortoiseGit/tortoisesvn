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
#include "TortoiseProc.h"
#include "Registry.h"
#include ".\historydlg.h"


IMPLEMENT_DYNAMIC(CHistoryDlg, CResizableStandAloneDialog)
CHistoryDlg::CHistoryDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CHistoryDlg::IDD, pParent),
	m_nMaxHistoryItems(25)
{
}

CHistoryDlg::~CHistoryDlg()
{
}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HISTORYLIST, m_List);
}


BEGIN_MESSAGE_MAP(CHistoryDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_LBN_DBLCLK(IDC_HISTORYLIST, OnLbnDblclkHistorylist)
END_MESSAGE_MAP()

bool CHistoryDlg::AddString(const CString& sText)
{
	if (sText.IsEmpty())
		return false;

	if ((!m_sSection.IsEmpty())&&(!m_sKeyPrefix.IsEmpty()))
	{
		// refresh the history from the registry
		LoadHistory(m_sSection, m_sKeyPrefix);
	}

	for (int i=0; i<m_arEntries.GetCount(); ++i)
	{
		if (sText.Compare(m_arEntries[i])==0)
		{
			m_arEntries.RemoveAt(i);
			m_arEntries.InsertAt(0, sText);
			return false;
		}
	}
	m_arEntries.InsertAt(0, sText);
	return true;
}

int CHistoryDlg::LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix)
{
	if (lpszSection == NULL || lpszKeyPrefix == NULL || *lpszSection == '\0')
		return -1;

	m_arEntries.RemoveAll();

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
		{
			m_arEntries.Add(sText);
		}
	} while (!sText.IsEmpty() && n < m_nMaxHistoryItems);
	return m_arEntries.GetCount();
}

bool CHistoryDlg::SaveHistory()
{
	if (m_sSection.IsEmpty())
		return false;

	// save history to registry
	int nMax = min(m_arEntries.GetCount(), m_nMaxHistoryItems + 1);
	for (int n = 0; n < nMax; n++)
	{
		CString sKey;
		sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
		CRegString regkey = CRegString(sKey);
		regkey = m_arEntries.GetAt(n);
	}
	// remove items exceeding the max number of history items
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
	return true;
}

void CHistoryDlg::OnBnClickedOk()
{
	int pos = m_List.GetCurSel();
	if (pos != LB_ERR)
	{
		m_SelectedText = m_arEntries[pos];
	}
	else
		m_SelectedText.Empty();
	OnOK();
}

BOOL CHistoryDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// calculate and set listbox width
	CDC* pDC=m_List.GetDC();
	CSize itemExtent;
	int horizExtent = 1;
	for (int i=0; i<m_arEntries.GetCount(); ++i)
	{
		CString sEntry = m_arEntries[i];
		sEntry.Replace(_T("\r"), _T(""));
		sEntry.Replace('\n', ' ');
		m_List.AddString(sEntry);
		itemExtent=pDC->GetTextExtent(sEntry);
		horizExtent=max(horizExtent, itemExtent.cx+5);
	}
	m_List.SetHorizontalExtent(horizExtent);
	ReleaseDC(pDC); 

	AddAnchor(IDC_HISTORYLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	EnableSaveRestore(_T("HistoryDlg"));
	m_List.SetFocus();
	return FALSE;
}

void CHistoryDlg::OnLbnDblclkHistorylist()
{
	int pos = m_List.GetCurSel();
	if (pos != LB_ERR)
	{
		m_SelectedText = m_arEntries[pos];
		OnOK();
	}
	else
		m_SelectedText.Empty();
}
