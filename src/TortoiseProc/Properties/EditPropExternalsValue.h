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
#include "StandAloneDlg.h"
#include "SVNExternals.h"
#include "HistoryCombo.h"
#include "SVNRev.h"
#include "../LogDialog/LogDlg.h"



class CEditPropExternalsValue : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CEditPropExternalsValue)

public:
    CEditPropExternalsValue(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEditPropExternalsValue();

    void         SetURL(const CTSVNPath& url) { m_URL = url; }
    void         SetRepoRoot(const CTSVNPath& root) { m_RepoRoot = root; }
    void         SetExternal(const SVNExternal& ext) { m_External = ext; }
    SVNExternal  GetExternal() { return m_External; }
    void         SetPathList(const CTSVNPathList& list) { m_pathList = list; }

    enum { IDD = IDD_EDITPROPEXTERNALSVALUE };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

    DECLARE_MESSAGE_MAP()

    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedShowLog();
    afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
    afx_msg void OnEnChangeRevisionNum();
    afx_msg void OnBnClickedHelp();

private:
    CTSVNPath       m_URL;
    CTSVNPath       m_RepoRoot;
    SVNExternal     m_External;
    CLogDlg *       m_pLogDlg;
    int             m_height;

    CHistoryCombo   m_URLCombo;
    CString         m_sRevision;
    CString         m_sWCPath;
    CString         m_sPegRev;
    CTSVNPathList   m_pathList;
};
