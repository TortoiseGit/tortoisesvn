// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2011-2012, 2015, 2021 - TortoiseSVN

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
#include "SettingsPropPage.h"
#include "ILogReceiver.h"

class CProgressDlg;

/**
 * \ingroup TortoiseProc
 * Settings page to configure miscellaneous stuff.
 */
class CSettingsLogCaches
    : public ISettingsPropPage
    , private ILogReceiver
{
    DECLARE_DYNAMIC(CSettingsLogCaches)

public:
    CSettingsLogCaches();
    ~CSettingsLogCaches() override;

    UINT GetIconID() override { return IDI_CACHELIST; }

    // update cache list

    BOOL OnSetActive() override;

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSLOGCACHELIST
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;
    BOOL OnKillActive() override;
    BOOL OnQueryCancel() override;

    afx_msg void OnBnClickedDetails();
    afx_msg void OnBnClickedUpdate();
    afx_msg void OnBnClickedExport();
    afx_msg void OnBnClickedDelete();

    afx_msg LRESULT OnRefeshRepositoryList(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnNMDblclkRepositorylist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnItemchangedRepositorylist(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
private:
    CListCtrl m_cRepositoryList;

    /// current repository list

    std::multimap<CString, CString>             repos;
    std::multimap<CString, CString>::value_type GetSelectedRepo();
    void                                        FillRepositoryList();

    static UINT WorkerThread(LPVOID pVoid);

    volatile LONG m_bThreadRunning;

    /// used by cache update

    CProgressDlg* progress;
    svn_revnum_t  headRevision;

    void ReceiveLog(TChangedPaths* changes, svn_revnum_t rev, const StandardRevProps* stdRevProps, UserRevPropArray* userRevProps, const MergeInfo* mergeInfo) override;
};
