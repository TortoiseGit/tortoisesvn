// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2011-2013, 2015. 2021 - TortoiseSVN

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
#include "TSVNPath.h"
#include "ResizableSheetEx.h"
#include "MergeWizardStart.h"
#include "MergeWizardTree.h"
#include "MergeWizardRevRange.h"
#include "MergeWizardOptions.h"
#include "AeroControls.h"

#define MERGEWIZARD_REVRANGE 0
#define MERGEWIZARD_TREE     1

class CMergeWizard : public CResizableSheetEx
{
    DECLARE_DYNAMIC(CMergeWizard)

public:
    CMergeWizard(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
    ~CMergeWizard() override;

protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog() override;

    afx_msg void    OnPaint();
    afx_msg BOOL    OnEraseBkgnd(CDC* pDC);
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnTaskbarButtonCreated(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void    OnCancel();

    CMergeWizardStart    m_page1;
    CMergeWizardTree     m_tree;
    CMergeWizardRevRange m_revRange;
    CMergeWizardOptions  m_options;

public:
    CTSVNPath m_wcPath;
    CString   m_url;
    CString   m_sUuid;
    int       m_nRevRangeMerge;

    CString          m_url1;
    CString          m_url2;
    SVNRev           m_startRev;
    SVNRev           m_endRev;
    SVNRev           m_pegRev;
    SVNRevRangeArray m_revRangeArray;
    BOOL             m_bReverseMerge;
    BOOL             m_bReintegrate;

    BOOL m_bRecordOnly;

    BOOL                         m_bIgnoreAncestry;
    svn_depth_t                  m_depth;
    BOOL                         m_bIgnoreEOL;
    svn_diff_file_ignore_space_t m_ignoreSpaces;
    BOOL                         m_bForce;
    BOOL                         m_bAllowMixed;

    void    SaveMode() const;
    LRESULT GetSecondPage() const;

private:
    bool            m_firstPageActivation;
    HICON           m_hIcon;
    AeroControlBase m_aeroControls;
};
