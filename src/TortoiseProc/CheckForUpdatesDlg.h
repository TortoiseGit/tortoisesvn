// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2021 - TortoiseSVN

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
#include "HyperLink.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog class, which checks if there are updated version of TortoiseSVN
 * available.
 */
class CCheckForUpdatesDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CCheckForUpdatesDlg)

public:
    CCheckForUpdatesDlg(CWnd* pParent = nullptr); // standard constructor
    ~CCheckForUpdatesDlg() override;

    enum
    {
        IDD = IDD_CHECKFORUPDATES
    };

protected:
    afx_msg void OnStnClickedCheckresult();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    void         OnOK() override;
    void         OnCancel() override;

    DECLARE_MESSAGE_MAP()

private:
    static UINT CheckThreadEntry(LPVOID pVoid);
    UINT        CheckThread();

public:
    BOOL m_bThreadRunning;
    BOOL m_bShowInfo;
    BOOL m_bVisible;

private:
    CString    m_sUpdateDownloadLink; ///< Where to send a user looking to download a update
    CHyperLink m_link;
};
