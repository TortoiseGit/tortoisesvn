// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2016, 2018, 2021 - TortoiseSVN

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
#include "SVNDiffOptions.h"

/**
 * Helper dialog to chose options for diffing
 */
class CDiffOptionsDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CDiffOptionsDlg)

public:
    CDiffOptionsDlg(CWnd* pParent = nullptr); // standard constructor
    ~CDiffOptionsDlg() override;

    enum
    {
        IDD = IDD_DIFFOPTIONS
    };

    void           SetDiffOptions(const SVNDiffOptions& opts);
    void           SetPrettyPrint(bool prettyprint) { m_bPrettyPrint = prettyprint; }
    SVNDiffOptions GetDiffOptions() const;
    CString        GetDiffOptionsString() const { return GetDiffOptions().GetOptionsString(); }
    bool           GetPrettyPrint() const { return !!m_bPrettyPrint; }

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()

private:
    BOOL m_bIgnoreEOLs;
    BOOL m_bIgnoreWhitespaces;
    BOOL m_bIgnoreAllWhitespaces;
    BOOL m_bPrettyPrint;
};
