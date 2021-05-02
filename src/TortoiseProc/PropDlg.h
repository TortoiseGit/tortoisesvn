// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2010, 2017, 2021 - TortoiseSVN

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
#include "SVNRev.h"
#include "HintCtrl.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog which shows revision properties.
 */
class CPropDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CPropDlg)

public:
    CPropDlg(CWnd* pParent = nullptr);
    ~CPropDlg() override;

    enum
    {
        IDD = IDD_PROPERTIES
    };

private:
    static UINT PropThreadEntry(LPVOID pVoid);
    UINT        PropThread();
    void        setProplistColumnWidth();

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnCancel() override;
    void OnOK() override;

    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

    DECLARE_MESSAGE_MAP()

public:
    CTSVNPath m_path;
    SVNRev    m_rev;

private:
    HANDLE               m_hThread;
    CHintCtrl<CListCtrl> m_propList;
};
