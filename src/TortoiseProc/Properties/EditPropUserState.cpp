// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2013 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "EditPropUserState.h"
#include <afxdialogex.h>
#include "AppUtils.h"
#include "UnicodeUtils.h"


// EditPropUserState dialog

IMPLEMENT_DYNAMIC(EditPropUserState, CStandAloneDialog)

EditPropUserState::EditPropUserState(CWnd* pParent, const UserProp * p)
    : CStandAloneDialog(EditPropUserState::IDD, pParent)
    , EditPropBase()
    , m_userprop(p)
{

}

EditPropUserState::~EditPropUserState()
{
}

void EditPropUserState::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO, m_combo);
    DDX_Text(pDX, IDC_LABEL, m_sLabel);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(EditPropUserState, CStandAloneDialog)
END_MESSAGE_MAP()


// EditPropUserState message handlers


BOOL EditPropUserState::OnInitDialog()
{
    __super::OnInitDialog();

    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    AdjustControlSize(IDC_PROPRECURSIVE);

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(!m_bFolder || m_bMultiple || (m_bFolder && !m_userprop->file));
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || (!m_bFolder && !m_bMultiple) || m_bRemote ? SW_HIDE : SW_SHOW);

    if (m_userprop->stateEntries.size() <= 3)
    {
        // use the radio buttons
        int index = 0;
        bool checked = false;
        for (auto it = m_userprop->stateEntries.cbegin(); it != m_userprop->stateEntries.cend(); ++it)
        {
            switch (index)
            {
            case 0:     // IDC_RADIO1
                GetDlgItem(IDC_RADIO1)->ShowWindow(SW_SHOW);
                SetDlgItemText(IDC_RADIO1, std::get<1>(*it));
                AdjustControlSize(IDC_RADIO1);
                if (m_PropValue.compare(CUnicodeUtils::GetUTF8(std::get<0>(*it)))==0)
                {
                    CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
                    checked = true;
                }
                if (!checked)
                {
                    if (std::get<0>(*it).Compare(m_userprop->stateDefaultVal)==0)
                        CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
                }
                break;
            case 1:     // IDC_RADIO2
                GetDlgItem(IDC_RADIO2)->ShowWindow(SW_SHOW);
                SetDlgItemText(IDC_RADIO2, std::get<1>(*it));
                AdjustControlSize(IDC_RADIO2);
                if (m_PropValue.compare(CUnicodeUtils::GetUTF8(std::get<0>(*it)))==0)
                {
                    CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO2);
                    checked = true;
                }
                if (!checked)
                {
                    if (std::get<0>(*it).Compare(m_userprop->stateDefaultVal)==0)
                        CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO2);
                }
                break;
            case 2:     // IDC_RADIO3
                GetDlgItem(IDC_RADIO3)->ShowWindow(SW_SHOW);
                SetDlgItemText(IDC_RADIO3, std::get<1>(*it));
                AdjustControlSize(IDC_RADIO3);
                if (m_PropValue.compare(CUnicodeUtils::GetUTF8(std::get<0>(*it)))==0)
                {
                    CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);
                    checked = true;
                }
                if (!checked)
                {
                    if (std::get<0>(*it).Compare(m_userprop->stateDefaultVal)==0)
                        CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);
                }
                break;
            }
            index++;
        }
    }
    else
    {
        // use the combo list box
        m_combo.ShowWindow(SW_SHOW);
        CString selString;
        CString selDefString;
        for (auto it = m_userprop->stateEntries.cbegin(); it != m_userprop->stateEntries.cend(); ++it)
        {
            int index = m_combo.AddString(std::get<1>(*it));
            m_combo.SetItemData(index, (DWORD_PTR)&std::get<0>(*it));
            if (m_PropValue.compare(CUnicodeUtils::GetUTF8(std::get<0>(*it)))==0)
                selString = std::get<1>(*it);
            if (std::get<0>(*it).Compare(m_userprop->stateDefaultVal)==0)
                selDefString = std::get<1>(*it);
        }
        if (!selString.IsEmpty())
            m_combo.SelectString(0, selString);
        else if (!selDefString.IsEmpty())
            m_combo.SelectString(0, selDefString);
    }
    m_sLabel = m_userprop->labelText;

    CString sWindowTitle = m_userprop->propName;
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    if (m_bFolder && m_userprop->file)
    {
        // for folders, the file properties can only be set recursively
        m_bRecursive = TRUE;
    }

    UpdateData(false);

    return TRUE;
}


void EditPropUserState::OnOK()
{
    UpdateData();

    if (m_userprop->stateEntries.size() <= 3)
    {
        int checkedRadio = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3);
        int index = 0;
        for (auto it = m_userprop->stateEntries.cbegin(); it != m_userprop->stateEntries.cend(); ++it)
        {
            switch (index)
            {
            case 0:
                if (checkedRadio == IDC_RADIO1)
                {
                    m_PropValue = CUnicodeUtils::GetUTF8(std::get<0>(*it));
                    m_bChanged = true;
                }
                break;
            case 1:
                if (checkedRadio == IDC_RADIO2)
                {
                    m_PropValue = CUnicodeUtils::GetUTF8(std::get<0>(*it));
                    m_bChanged = true;
                }
                break;
            case 2:
                if (checkedRadio == IDC_RADIO3)
                {
                    m_PropValue = CUnicodeUtils::GetUTF8(std::get<0>(*it));
                    m_bChanged = true;
                }
            }
            index++;
        }
    }
    else
    {
        int sel = m_combo.GetCurSel();
        if (sel != CB_ERR)
        {
            CString * pVal = (CString*)m_combo.GetItemData(sel);
            m_PropValue = CUnicodeUtils::GetUTF8(*pVal);
        }
        else
            m_PropValue = CUnicodeUtils::GetUTF8(m_userprop->stateDefaultVal);
    }

    m_bChanged = true;

    __super::OnOK();
}
