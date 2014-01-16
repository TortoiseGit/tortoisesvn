// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "registry.h"
#include "HistoryDlg.h"


IMPLEMENT_DYNAMIC(CHistoryDlg, CResizableStandAloneDialog)
CHistoryDlg::CHistoryDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CHistoryDlg::IDD, pParent)
    , m_history(NULL)
{
}

CHistoryDlg::~CHistoryDlg()
{
}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_HISTORYLIST, m_List);
    DDX_Control(pDX, IDC_SEARCHEDIT, m_cFilter);
}


BEGIN_MESSAGE_MAP(CHistoryDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_LBN_DBLCLK(IDC_HISTORYLIST, OnLbnDblclkHistorylist)
    ON_WM_KEYDOWN()
    ON_EN_CHANGE(IDC_SEARCHEDIT, &CHistoryDlg::OnEnChangeSearchedit)
    ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_CANCELCLICKED, &CHistoryDlg::OnClickedCancelFilter)
END_MESSAGE_MAP()


void CHistoryDlg::OnBnClickedOk()
{
    int pos = m_List.GetCurSel();
    if (pos != LB_ERR)
    {
        int index = (int)m_List.GetItemData(pos);
        m_SelectedText = m_history->GetEntry(index);
    }
    else
        m_SelectedText.Empty();
    OnOK();
}

BOOL CHistoryDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
    m_cFilter.SetValidator(this);

    UpdateMessageList();

    AddAnchor(IDC_FILTERLABEL, TOP_LEFT);
    AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_HISTORYLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    EnableSaveRestore(L"HistoryDlg");
    m_List.SetFocus();
    return FALSE;
}

void CHistoryDlg::OnLbnDblclkHistorylist()
{
    int pos = m_List.GetCurSel();
    if (pos != LB_ERR)
    {
        int index = (int)m_List.GetItemData(pos);
        m_SelectedText = m_history->GetEntry(index);
        OnOK();
    }
    else
        m_SelectedText.Empty();
}

BOOL CHistoryDlg::PreTranslateMessage(MSG* pMsg)
{
    if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_DELETE))
    {
        int pos = m_List.GetCurSel();
        if (pos != LB_ERR)
        {
            m_List.DeleteString(pos);
            m_List.SetCurSel(min(pos, m_List.GetCount() - 1));
            int index = (int)m_List.GetItemData(pos);
            m_history->RemoveEntry(index);
            m_history->Save();
            return TRUE;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CHistoryDlg::OnEnChangeSearchedit()
{
    UpdateMessageList();
}

void CHistoryDlg::UpdateMessageList()
{
    CString sFilter;
    GetDlgItemText(IDC_SEARCHEDIT, sFilter);
    int pos = 0;
    CString temp;
    std::vector<CString> tokens;
    for(;;)
    {
        temp = sFilter.Tokenize(L" ", pos);
        if(temp.IsEmpty())
        {
            break;
        }
        tokens.push_back(temp);
    }

    m_List.ResetContent();

    // calculate and set listbox width
    CDC* pDC=m_List.GetDC();
    CSize itemExtent;
    int horizExtent = 1;
    for (size_t i = 0; i < m_history->GetCount(); ++i)
    {
        CString sEntry = m_history->GetEntry(i);
        CString sLowerEntry = sEntry;
        sLowerEntry.MakeLower();
        bool match = true;
        for (const auto& filter : tokens)
        {
            if (sLowerEntry.Find(filter) < 0)
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            sEntry.Remove('\r');
            sEntry.Replace('\n', ' ');
            int index = m_List.AddString(sEntry);
            m_List.SetItemData(index, i);
            itemExtent = pDC->GetTextExtent(sEntry);
            horizExtent = max(horizExtent, itemExtent.cx+5);
        }
    }
    m_List.SetHorizontalExtent(horizExtent);
    ReleaseDC(pDC);
}

bool CHistoryDlg::Validate( LPCTSTR /*string*/ )
{
    return true;
}

LRESULT CHistoryDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    UpdateMessageList();
    return 0;
}
