// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "EditPropExternals.h"
#include "EditPropExternalsValue.h"
#include "SVN.h"

IMPLEMENT_DYNAMIC(CEditPropExternals, CResizableStandAloneDialog)

CEditPropExternals::CEditPropExternals(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropExternals::IDD, pParent)
    , EditPropBase()
{

}

CEditPropExternals::~CEditPropExternals()
{
}

void CEditPropExternals::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EXTERNALSLIST, m_ExtList);
}


BEGIN_MESSAGE_MAP(CEditPropExternals, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_ADD, &CEditPropExternals::OnBnClickedAdd)
    ON_BN_CLICKED(IDC_EDIT, &CEditPropExternals::OnBnClickedEdit)
    ON_BN_CLICKED(IDC_REMOVE, &CEditPropExternals::OnBnClickedRemove)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_EXTERNALSLIST, &CEditPropExternals::OnLvnGetdispinfoExternalslist)
    ON_NOTIFY(NM_DBLCLK, IDC_EXTERNALSLIST, &CEditPropExternals::OnNMDblclkExternalslist)
    ON_BN_CLICKED(IDHELP, &CEditPropExternals::OnBnClickedHelp)
END_MESSAGE_MAP()


BOOL CEditPropExternals::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_EXTERNALSLIST);
    m_aeroControls.SubclassControl(this, IDC_ADD);
    m_aeroControls.SubclassControl(this, IDC_EDIT);
    m_aeroControls.SubclassControl(this, IDC_REMOVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    ATLASSERT(m_pathList.GetCount() == 1);

    SVN svn;
    m_url = CTSVNPath(svn.GetURLFromPath(m_pathList[0]));
    m_repoRoot = CTSVNPath(svn.GetRepositoryRoot(m_url));

    m_externals.Add(m_pathList[0], m_PropValue, false);

    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    m_ExtList.SetExtendedStyle(exStyle);

    SetWindowTheme(m_ExtList.GetSafeHwnd(), L"Explorer", NULL);

    m_ExtList.SetRedraw(false);
    m_ExtList.DeleteAllItems();

    int c = ((CHeaderCtrl*)(m_ExtList.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_ExtList.DeleteColumn(c--);
    CString temp;
    temp.LoadString(IDS_STATUSLIST_COLFILE);
    m_ExtList.InsertColumn(0, temp);
    temp.LoadString(IDS_STATUSLIST_COLURL);
    m_ExtList.InsertColumn(1, temp);
    temp.LoadString(IDS_STATUSLIST_COLREVISION);
    m_ExtList.InsertColumn(2, temp);
    m_ExtList.SetItemCountEx((int)m_externals.size());

    CRect rect;
    m_ExtList.GetClientRect(&rect);
    m_ExtList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    int cx = (rect.Width()-80-m_ExtList.GetColumnWidth(0));
    m_ExtList.SetColumnWidth(1, cx);
    m_ExtList.SetColumnWidth(2, 80);

    m_ExtList.SetRedraw(true);

    AddAnchor(IDC_EXTERNALSLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_ADD, BOTTOM_LEFT);
    AddAnchor(IDC_EDIT, BOTTOM_LEFT);
    AddAnchor(IDC_REMOVE, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);

    return TRUE;
}

void CEditPropExternals::OnOK()
{
    UpdateData();
    m_bChanged = true;
    m_PropValue = m_externals.GetValue(m_pathList[0]);

    CResizableStandAloneDialog::OnOK();
}

void CEditPropExternals::OnBnClickedAdd()
{
    CEditPropExternalsValue dlg;
    dlg.SetURL(m_url);
    dlg.SetRepoRoot(m_repoRoot);
    if (dlg.DoModal() == IDOK)
    {
        SVNExternal ext = dlg.GetExternal();
        ext.path = m_pathList[0];
        m_externals.push_back(ext);
        m_ExtList.SetItemCountEx((int)m_externals.size());
        m_ExtList.Invalidate();
    }
}

void CEditPropExternals::OnBnClickedEdit()
{
    if (m_ExtList.GetSelectedCount() == 1)
    {
        POSITION pos = m_ExtList.GetFirstSelectedItemPosition();
        size_t selIndex = m_ExtList.GetNextSelectedItem(pos);
        SVNExternal ext = m_externals[selIndex];
        if (ext.revision.kind == svn_opt_revision_unspecified)
            ext.revision = ext.origrevision;

        CEditPropExternalsValue dlg;
        dlg.SetURL(m_url);
        dlg.SetRepoRoot(m_repoRoot);
        dlg.SetExternal(ext);
        if (dlg.DoModal() == IDOK)
        {
            ext = dlg.GetExternal();
            m_externals[selIndex] = ext;
            m_ExtList.Invalidate();
        }
    }
}

void CEditPropExternals::OnBnClickedRemove()
{
    POSITION pos = m_ExtList.GetFirstSelectedItemPosition();
    size_t selIndex = m_ExtList.GetNextSelectedItem(pos);
    m_externals.erase(m_externals.begin() + selIndex);
    m_ExtList.SetItemCountEx((int)m_externals.size());
    m_ExtList.Invalidate();
}

void CEditPropExternals::OnNMDblclkExternalslist(NMHDR * pNMHDR, LRESULT *pResult)
{
    // a double click on an entry in the revision list has happened
    LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) pNMHDR;
    *pResult = 0;

    if (lpnmitem->iItem < (int)m_externals.size())
    {
        SVNExternal ext = m_externals[lpnmitem->iItem];
        if (ext.revision.kind == svn_opt_revision_unspecified)
            ext.revision = ext.origrevision;

        CEditPropExternalsValue dlg;
        dlg.SetURL(m_url);
        dlg.SetRepoRoot(m_repoRoot);
        dlg.SetExternal(ext);
        if (dlg.DoModal() == IDOK)
        {
            ext = dlg.GetExternal();
            m_externals[lpnmitem->iItem] = ext;
            m_ExtList.Invalidate();
        }
    }
}

void CEditPropExternals::OnLvnGetdispinfoExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    *pResult = 0;

    if (pDispInfo)
    {
        if (pDispInfo->item.iItem < (int)m_externals.size())
        {
            SVNExternal ext = m_externals[pDispInfo->item.iItem];
            if (pDispInfo->item.mask & LVIF_TEXT)
            {
                switch (pDispInfo->item.iSubItem)
                {
                case 0: // folder or file
                    {
                        lstrcpyn(m_columnbuf, ext.targetDir, pDispInfo->item.cchTextMax);
                    }
                    break;
                case 1: // url
                    {
                        lstrcpyn(m_columnbuf, ext.url, pDispInfo->item.cchTextMax);
                        SVNRev peg(ext.pegrevision);
                        if (peg.IsValid() && !peg.IsHead())
                        {
                            _tcscat_s(m_columnbuf, _countof(m_columnbuf), _T("@"));
                            _tcscat_s(m_columnbuf, _countof(m_columnbuf), peg.ToString());
                        }
                        int cWidth = m_ExtList.GetColumnWidth(1);
                        cWidth = max(14, cWidth-14);
                        CDC * pDC = m_ExtList.GetDC();
                        if (pDC != NULL)
                        {
                            CFont * pFont = pDC->SelectObject(m_ExtList.GetFont());
                            PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
                            pDC->SelectObject(pFont);
                            ReleaseDC(pDC);
                        }
                    }
                    break;
                case 2: // revision
                    if ((ext.revision.kind == svn_opt_revision_number) && (ext.revision.value.number >= 0))
                        _stprintf_s(m_columnbuf, MAX_PATH, _T("%ld"), ext.revision.value.number);
                    else
                        m_columnbuf[0] = 0;
                    break;

                default:
                    m_columnbuf[0] = 0;
                }
                pDispInfo->item.pszText = m_columnbuf;
            }
        }
    }
}

void CEditPropExternals::OnBnClickedHelp()
{
    OnHelp();
}
