// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2021 - TortoiseSVN

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
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "PropDlg.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CPropDlg, CResizableStandAloneDialog)
CPropDlg::CPropDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CPropDlg::IDD, pParent)
    , m_rev(SVNRev::REV_WC)
    , m_hThread(nullptr)
{
}

CPropDlg::~CPropDlg()
{
}

void CPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROPERTYLIST, m_propList);
}

BEGIN_MESSAGE_MAP(CPropDlg, CResizableStandAloneDialog)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CPropDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_PROPERTYLIST, IDC_PROPERTYLIST, IDC_PROPERTYLIST, IDC_PROPERTYLIST);
    m_aeroControls.SubclassControl(this, IDOK);

    m_propList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_propList.DeleteAllItems();
    int c = m_propList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_propList.DeleteColumn(c--);
    CString temp;
    temp.LoadString(IDS_PROPPROPERTY);
    m_propList.InsertColumn(0, temp);
    temp.LoadString(IDS_PROPVALUE);
    m_propList.InsertColumn(1, temp);
    m_propList.SetRedraw(false);
    setProplistColumnWidth();
    m_propList.SetRedraw(false);

    DialogEnableWindow(IDOK, FALSE);
    if (AfxBeginThread(PropThreadEntry, this) == nullptr)
    {
        OnCantStartThread();
    }

    AddAnchor(IDC_PROPERTYLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_CENTER);
    EnableSaveRestore(L"PropDlg");
    return TRUE;
}

void CPropDlg::OnCancel()
{
    if (GetDlgItem(IDOK)->IsWindowEnabled())
        CResizableStandAloneDialog::OnCancel();
}

void CPropDlg::OnOK()
{
    if (GetDlgItem(IDOK)->IsWindowEnabled())
        CResizableStandAloneDialog::OnOK();
}

UINT CPropDlg::PropThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CPropDlg*>(pVoid)->PropThread();
}

UINT CPropDlg::PropThread()
{
    SVNProperties props(m_path, m_rev, false, false);

    m_propList.SetRedraw(false);
    int row = 0;
    for (int i = 0; i < props.GetCount(); ++i)
    {
        CString name = CUnicodeUtils::StdGetUnicode(props.GetItemName(i)).c_str();
        CString val;
        val = CUnicodeUtils::GetUnicode(props.GetItemValue(i).c_str());

        int nFound = -1;
        do
        {
            nFound = val.FindOneOf(L"\r\n");
            m_propList.InsertItem(row, name);
            if (nFound >= 0)
                m_propList.SetItemText(row++, 1, val.Left(nFound));
            else
                m_propList.SetItemText(row++, 1, val);
            val = val.Mid(nFound);
            val.Trim();
            name.Empty();
        } while (!val.IsEmpty() && (nFound >= 0));
    }
    setProplistColumnWidth();

    m_propList.SetRedraw(true);
    DialogEnableWindow(IDOK, TRUE);
    return 0;
}

BOOL CPropDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if ((GetDlgItem(IDOK)->IsWindowEnabled()) || (IsCursorOverWindowBorder()))
    {
        HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
        SetCursor(hCur);
        return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
    }
    HCURSOR hCur = LoadCursor(nullptr, IDC_WAIT);
    SetCursor(hCur);
    return TRUE;
}

void CPropDlg::setProplistColumnWidth()
{
    const int maxCol = m_propList.GetHeaderCtrl()->GetItemCount() - 1;
    for (int col = 0; col <= maxCol; col++)
    {
        m_propList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }
}