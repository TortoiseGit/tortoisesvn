// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2012 - TortoiseSVN

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
#include "afxcmn.h"
#include "EditPropBase.h"
#include "StandAloneDlg.h"
#include "SVNExternals.h"


class CEditPropExternals : public CResizableStandAloneDialog, public EditPropBase
{
    DECLARE_DYNAMIC(CEditPropExternals)

public:
    CEditPropExternals(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEditPropExternals();

    enum { IDD = IDD_EDITPROPEXTERNALS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedEdit();
    afx_msg void OnBnClickedRemove();
    afx_msg void OnBnClickedFindhead();
    afx_msg void OnLvnGetdispinfoExternalslist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnItemchangedExternalslist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclkExternalslist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    INT_PTR DoModal() override { return CResizableStandAloneDialog::DoModal(); }

private:
    CListCtrl       m_ExtList;
    SVNExternals    m_externals;
    TCHAR           m_columnbuf[MAX_PATH];
    CTSVNPath       m_url;
    CTSVNPath       m_repoRoot;
};
