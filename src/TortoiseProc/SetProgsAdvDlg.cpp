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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "ToolAssocDlg.h"
#include "SetProgsAdvDlg.h"


// CSetProgsAdvDlg dialog

IMPLEMENT_DYNAMIC(CSetProgsAdvDlg, CDialog)
CSetProgsAdvDlg::CSetProgsAdvDlg(const CString& type, CWnd* pParent /*=NULL*/)
	: CDialog(CSetProgsAdvDlg::IDD, pParent)
	, m_sType(type)
	, m_regToolKey(_T("Software\\TortoiseSVN\\") + type + _T("Tools"))
	, m_ToolsValid(false)
{
}

CSetProgsAdvDlg::~CSetProgsAdvDlg()
{
}

void CSetProgsAdvDlg::LoadData()
{
	if (!m_ToolsValid)
	{
		m_Tools.clear();

		CStringList values;
		if (m_regToolKey.getValues(values))
		{
			for (POSITION pos = values.GetHeadPosition(); pos != NULL; )
			{
				CString ext = values.GetNext(pos);
				m_Tools[ext] = CRegString(m_regToolKey.m_path + _T("\\") + ext);
			}
		}

		m_ToolsValid = true;
	}
}

void CSetProgsAdvDlg::SaveData()
{
	if (m_ToolsValid)
	{
		// Remove all registry values which are no longer in the list
		CStringList values;
		if (m_regToolKey.getValues(values))
		{
			for (POSITION pos = values.GetHeadPosition(); pos != NULL; )
			{
				CString ext = values.GetNext(pos);
				if (m_Tools.find(ext) == m_Tools.end())
				{
					CRegString to_remove(m_regToolKey.m_path + _T("\\") + ext);
					to_remove.removeValue();
				}
			}
		}

		// Add or update new or changed values
		for (TOOL_MAP::iterator it = m_Tools.begin(); it != m_Tools.end() ; it++)
		{
			CString ext = it->first;
			CString new_value = it->second;
			CRegString reg_value(m_regToolKey.m_path + _T("\\") + ext);
			if (reg_value != new_value)
				reg_value = new_value;
		}
	}
}

void CSetProgsAdvDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOOLLISTCTRL, m_ToolListCtrl);

	if (pDX->m_bSaveAndValidate)
	{
		m_Tools.clear();
		int count = m_ToolListCtrl.GetItemCount();
		for (int i = 0; i < count; i++)
		{
			CString ext = m_ToolListCtrl.GetItemText(i, 0);
			CString value = m_ToolListCtrl.GetItemText(i, 1);
			m_Tools[ext] = value;
		}
	}
	else
	{
		m_ToolListCtrl.DeleteAllItems();
		for (TOOL_MAP::iterator it = m_Tools.begin(); it != m_Tools.end() ; it++)
		{
			CString ext = it->first;
			CString value = it->second;
			AddExtension(ext, value);
		}
	}
}


BEGIN_MESSAGE_MAP(CSetProgsAdvDlg, CDialog)
	ON_BN_CLICKED(IDC_ADDTOOL, OnBnClickedAddtool)
	ON_BN_CLICKED(IDC_REMOVETOOL, OnBnClickedRemovetool)
	ON_BN_CLICKED(IDC_EDITTOOL, OnBnClickedEdittool)
	ON_NOTIFY(NM_CLICK, IDC_TOOLLISTCTRL, OnNMClickToollistctrl)
	ON_NOTIFY(NM_DBLCLK, IDC_TOOLLISTCTRL, OnNMDblclkToollistctrl)
END_MESSAGE_MAP()


BOOL CSetProgsAdvDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ToolListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_ToolListCtrl.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ToolListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ToolListCtrl.DeleteColumn(c--);

	CString temp;
	temp.LoadString(IDS_PROGS_EXTCOL);
	m_ToolListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGS_TOOLCOL);
	m_ToolListCtrl.InsertColumn(1, temp);

	m_ToolListCtrl.SetRedraw(FALSE);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_ToolListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_ToolListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_ToolListCtrl.SetRedraw(TRUE);

	temp.LoadString(m_sType == _T("Diff") ? IDS_DLGTITLE_ADV_DIFF : IDS_DLGTITLE_ADV_MERGE);
	SetWindowText(temp);

	LoadData();
	UpdateData(FALSE);
	EnableBtns();
	return TRUE;
}

int CSetProgsAdvDlg::AddExtension(const CString& ext, const CString& tool)
{
	// Note: list control automatically sorts entries
	int index = m_ToolListCtrl.InsertItem(0, ext);
	if (index >= 0)
	{
		m_ToolListCtrl.SetItemText(index, 1, tool);
	}
	return index;
}

int CSetProgsAdvDlg::FindExtension(const CString& ext)
{
	int count = m_ToolListCtrl.GetItemCount();

	for (int i = 0; i < count; i++)
	{
		if (m_ToolListCtrl.GetItemText(i, 0) == ext)
			return i;
	}

	return -1;
}

void CSetProgsAdvDlg::EnableBtns()
{
	bool enable_btns = m_ToolListCtrl.GetSelectionMark() >= 0;
	GetDlgItem(IDC_EDITTOOL)->EnableWindow(enable_btns);
	GetDlgItem(IDC_REMOVETOOL)->EnableWindow(enable_btns);
}


// CSetProgsPage message handlers

void CSetProgsAdvDlg::OnBnClickedAddtool()
{
	CToolAssocDlg dlg(m_sType, true);;
	dlg.m_sExtension = _T("");
	dlg.m_sTool = _T("");
	if (dlg.DoModal() == IDOK)
	{
		int index = AddExtension(dlg.m_sExtension, dlg.m_sTool);
		m_ToolListCtrl.SetItemState(index, UINT(-1), LVIS_SELECTED|LVIS_FOCUSED);
	}

	EnableBtns();
	m_ToolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnBnClickedEdittool()
{
	int selected = m_ToolListCtrl.GetSelectionMark();
	if (selected >= 0)
	{
		CToolAssocDlg dlg(m_sType, false);;
		dlg.m_sExtension = m_ToolListCtrl.GetItemText(selected, 0);
		dlg.m_sTool = m_ToolListCtrl.GetItemText(selected, 1);
		if (dlg.DoModal() == IDOK)
		{
			if (m_ToolListCtrl.DeleteItem(selected))
			{
				selected = AddExtension(dlg.m_sExtension, dlg.m_sTool);
				m_ToolListCtrl.SetItemState(selected, UINT(-1), LVIS_SELECTED|LVIS_FOCUSED);
			}
		}
	}

	EnableBtns();
	m_ToolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnBnClickedRemovetool()
{
	int selected = m_ToolListCtrl.GetSelectionMark();
	if (selected >= 0)
	{
		m_ToolListCtrl.SetRedraw(FALSE);
		if (m_ToolListCtrl.DeleteItem(selected))
		{
			if (m_ToolListCtrl.GetItemCount() <= selected)
				--selected;
			if (selected >= 0)
				m_ToolListCtrl.SetItemState(selected, UINT(-1), LVIS_SELECTED|LVIS_FOCUSED);
		}
		m_ToolListCtrl.SetRedraw(TRUE);
	}

	EnableBtns();
	m_ToolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnNMClickToollistctrl(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	EnableBtns();
	*pResult = 0;
}

void CSetProgsAdvDlg::OnNMDblclkToollistctrl(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	OnBnClickedEdittool();
	*pResult = 0;
}
