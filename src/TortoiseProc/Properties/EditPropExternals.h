// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2013, 2015, 2017, 2021 - TortoiseSVN

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
#pragma once
#include <afxcmn.h>
#include "EditPropBase.h"
#include "StandAloneDlg.h"
#include "SVNExternals.h"
#include "HintCtrl.h"

class CEditPropExternals : public CResizableStandAloneDialog
    , public EditPropBase
{
    DECLARE_DYNAMIC(CEditPropExternals)

public:
    CEditPropExternals(CWnd *pParent = nullptr); // standard constructor
    ~CEditPropExternals() override;

    enum
    {
        IDD = IDD_EDITPROPEXTERNALS
    };

protected:
    void DoDataExchange(CDataExchange *pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedEdit();
    afx_msg void OnBnClickedRemove();
    afx_msg void OnBnClickedFindhead();
    afx_msg void OnLvnGetdispinfoExternalslist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnItemchangedExternalslist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnHdnItemclickExternalslist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclkExternalslist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    INT_PTR DoModal() override { return CResizableStandAloneDialog::DoModal(); }

    static bool SortCompare(const SVNExternal &data1, const SVNExternal &data2);

private:
    CHintCtrl<CListCtrl> m_extList;
    SVNExternals         m_externals;
    wchar_t              m_columnBuf[MAX_PATH];
    CTSVNPath            m_url;
    CTSVNPath            m_repoRoot;
    static int           m_nSortedColumn;
    static bool          m_bAscending;
};
