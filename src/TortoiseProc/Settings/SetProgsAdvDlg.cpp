// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2014-2015, 2021 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "ToolAssocDlg.h"
#include "SetProgsAdvDlg.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CSetProgsAdvDlg, CResizableStandAloneDialog)
CSetProgsAdvDlg::CSetProgsAdvDlg(const CString& type, CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CSetProgsAdvDlg::IDD, pParent)
    , m_sType(type)
    , m_regToolKey(L"Software\\TortoiseSVN\\" + type + L"Tools")
    , m_toolsValid(false)
{
}

CSetProgsAdvDlg::~CSetProgsAdvDlg()
{
}

void CSetProgsAdvDlg::LoadData()
{
    if (!m_toolsValid)
    {
        m_tools.clear();

        CStringList values;
        if (m_regToolKey.getValues(values))
        {
            for (POSITION pos = values.GetHeadPosition(); pos != nullptr;)
            {
                CString ext  = values.GetNext(pos);
                m_tools[ext] = CRegString(m_regToolKey.m_path + L"\\" + ext);
            }
        }

        m_toolsValid = true;
    }
}

int CSetProgsAdvDlg::SaveData()
{
    if (m_toolsValid)
    {
        // Remove all registry values which are no longer in the list
        CStringList values;
        if (m_regToolKey.getValues(values))
        {
            for (POSITION pos = values.GetHeadPosition(); pos != nullptr;)
            {
                CString ext = values.GetNext(pos);
                if (m_tools.find(ext) == m_tools.end())
                {
                    CRegString toRemove(m_regToolKey.m_path + L"\\" + ext);
                    toRemove.removeValue();
                }
            }
        }

        // Add or update new or changed values
        for (auto it = m_tools.begin(); it != m_tools.end(); ++it)
        {
            CString    ext      = it->first;
            CString    newValue = it->second;
            CRegString regValue(m_regToolKey.m_path + L"\\" + ext);
            if (regValue != newValue)
                regValue = newValue;
        }
    }
    return 0;
}

void CSetProgsAdvDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TOOLLISTCTRL, m_toolListCtrl);

    if (pDX->m_bSaveAndValidate)
    {
        m_tools.clear();
        int count = m_toolListCtrl.GetItemCount();
        for (int i = 0; i < count; i++)
        {
            CString ext   = m_toolListCtrl.GetItemText(i, 0);
            CString value = m_toolListCtrl.GetItemText(i, 1);
            m_tools[ext]  = value;
        }
    }
    else
    {
        m_toolListCtrl.DeleteAllItems();
        for (auto it = m_tools.begin(); it != m_tools.end(); ++it)
        {
            CString ext   = it->first;
            CString value = it->second;
            AddExtension(ext, value);
        }
    }
}

BEGIN_MESSAGE_MAP(CSetProgsAdvDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_ADDTOOL, OnBnClickedAddtool)
    ON_BN_CLICKED(IDC_REMOVETOOL, OnBnClickedRemovetool)
    ON_BN_CLICKED(IDC_EDITTOOL, OnBnClickedEdittool)
    ON_NOTIFY(NM_DBLCLK, IDC_TOOLLISTCTRL, OnNMDblclkToollistctrl)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_TOOLLISTCTRL, &CSetProgsAdvDlg::OnLvnItemchangedToollistctrl)
    ON_BN_CLICKED(IDC_RESTOREDEFAULTS, &CSetProgsAdvDlg::OnBnClickedRestoredefaults)
END_MESSAGE_MAP()

BOOL CSetProgsAdvDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_GROUP);
    m_aeroControls.SubclassOkCancel(this);

    m_toolListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_toolListCtrl.DeleteAllItems();
    int c = m_toolListCtrl.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_toolListCtrl.DeleteColumn(c--);

    SetWindowTheme(m_toolListCtrl.GetSafeHwnd(), L"Explorer", nullptr);

    CString temp;
    temp.LoadString(IDS_PROGS_EXTCOL);
    m_toolListCtrl.InsertColumn(0, temp);
    temp.LoadString(IDS_PROGS_TOOLCOL);
    m_toolListCtrl.InsertColumn(1, temp);

    m_toolListCtrl.SetRedraw(FALSE);
    int minCol = 0;
    int maxCol = m_toolListCtrl.GetHeaderCtrl()->GetItemCount() - 1;
    for (int col = minCol; col <= maxCol; col++)
    {
        m_toolListCtrl.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }
    m_toolListCtrl.SetRedraw(TRUE);

    temp.LoadString(m_sType == L"Diff" ? IDS_DLGTITLE_ADV_DIFF : IDS_DLGTITLE_ADV_MERGE);
    SetWindowText(temp);

    LoadData();
    UpdateData(FALSE);
    EnableBtns();

    AddAnchor(IDC_GROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_TOOLLISTCTRL, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_ADDTOOL, BOTTOM_LEFT);
    AddAnchor(IDC_EDITTOOL, BOTTOM_LEFT);
    AddAnchor(IDC_REMOVETOOL, BOTTOM_LEFT);
    AddAnchor(IDC_RESTOREDEFAULTS, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);

    return TRUE;
}

int CSetProgsAdvDlg::AddExtension(const CString& ext, const CString& tool)
{
    // Note: list control automatically sorts entries
    int index = m_toolListCtrl.InsertItem(0, ext);
    if (index >= 0)
    {
        m_toolListCtrl.SetItemText(index, 1, tool);
    }
    return index;
}

int CSetProgsAdvDlg::FindExtension(const CString& ext) const
{
    int count = m_toolListCtrl.GetItemCount();

    for (int i = 0; i < count; i++)
    {
        if (m_toolListCtrl.GetItemText(i, 0) == ext)
            return i;
    }

    return -1;
}

void CSetProgsAdvDlg::EnableBtns() const
{
    bool enableBtns = m_toolListCtrl.GetSelectedCount() > 0;
    GetDlgItem(IDC_EDITTOOL)->EnableWindow(enableBtns);
    GetDlgItem(IDC_REMOVETOOL)->EnableWindow(enableBtns);
}

// CSetProgsPage message handlers

void CSetProgsAdvDlg::OnBnClickedAddtool()
{
    CToolAssocDlg dlg(m_sType, true);
    if (dlg.DoModal() == IDOK)
    {
        int index = AddExtension(dlg.m_sExtension, dlg.m_sTool);
        m_toolListCtrl.SetItemState(index, static_cast<UINT>(-1), LVIS_SELECTED | LVIS_FOCUSED);
        m_toolListCtrl.SetSelectionMark(index);
    }

    EnableBtns();
    m_toolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnBnClickedEdittool()
{
    int selected = m_toolListCtrl.GetSelectionMark();
    if (selected >= 0)
    {
        CToolAssocDlg dlg(m_sType, false);
        dlg.m_sExtension = m_toolListCtrl.GetItemText(selected, 0);
        dlg.m_sTool      = m_toolListCtrl.GetItemText(selected, 1);
        if (dlg.DoModal() == IDOK)
        {
            if (m_toolListCtrl.DeleteItem(selected))
            {
                selected = AddExtension(dlg.m_sExtension, dlg.m_sTool);
                m_toolListCtrl.SetItemState(selected, static_cast<UINT>(-1), LVIS_SELECTED | LVIS_FOCUSED);
                m_toolListCtrl.SetSelectionMark(selected);
            }
        }
    }

    EnableBtns();
    m_toolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnBnClickedRemovetool()
{
    int selected = m_toolListCtrl.GetSelectionMark();
    if (selected >= 0)
    {
        m_toolListCtrl.SetRedraw(FALSE);
        if (m_toolListCtrl.DeleteItem(selected))
        {
            if (m_toolListCtrl.GetItemCount() <= selected)
                --selected;
            if (selected >= 0)
            {
                m_toolListCtrl.SetItemState(selected, static_cast<UINT>(-1), LVIS_SELECTED | LVIS_FOCUSED);
                m_toolListCtrl.SetSelectionMark(selected);
            }
        }
        m_toolListCtrl.SetRedraw(TRUE);
    }

    EnableBtns();
    m_toolListCtrl.SetFocus();
}

void CSetProgsAdvDlg::OnNMDblclkToollistctrl(NMHDR* /* pNMHDR */, LRESULT* pResult)
{
    OnBnClickedEdittool();
    *pResult = 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CSetProgsAdvDlg::OnLvnItemchangedToollistctrl(NMHDR* /* pNMHDR */, LRESULT* pResult)
{
    EnableBtns();

    *pResult = 0;
}

void CSetProgsAdvDlg::OnBnClickedRestoredefaults()
{
    CAppUtils::SetupDiffScripts(true, m_sType);
    m_toolsValid = FALSE;
    LoadData();
    UpdateData(FALSE);
    EnableBtns();
}