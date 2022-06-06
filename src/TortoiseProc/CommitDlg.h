// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013, 2015, 2021-2022 - TortoiseSVN

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
#include "SVNStatusListCtrl.h"
#include "ProjectProperties.h"
#include "RegHistory.h"
#include "registry.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "LinkControl.h"
#include "HyperLink.h"
#include "PathWatcher.h"
#include "BugTraqAssociations.h"
#include "../IBugTraqProvider/IBugTraqProvider_h.h"
#include "PathEdit.h"

#define ENDDIALOGTIMER 100
#define REFRESHTIMER   101

/**
 * \ingroup TortoiseProc
 * Dialog to enter log messages used in a commit.
 */
class CCommitDlg : public CResizableStandAloneDialog
    , public CSciEditContextMenuInterface
{
    DECLARE_DYNAMIC(CCommitDlg)

public:
    CCommitDlg(CWnd* pParent = nullptr); // standard constructor
    ~CCommitDlg() override;

    // CSciEditContextMenuInterface
    void InsertMenuItems(CMenu& mPopup, int& nCmd) override;
    bool HandleMenuItemClick(int cmd, CSciEdit* pSciEdit) override;
    void HandleSnippet(int type, const CString& text, CSciEdit* pSciEdit) override;

private:
    static UINT StatusThreadEntry(LPVOID pVoid);
    UINT        StatusThread();
    void        UpdateOKButton();
    void        OnComError(HRESULT hr) const;

    // Dialog Data
    enum
    {
        IDD = IDD_COMMITDLG
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    // extracted from OnInitDialog
    void ShowBalloonInCaseOfError() const;
    void AddDirectoriesToPathWatcher();
    void AdjustDialogSizeAndPanes();
    void CenterWindowWhenLaunchedFromExplorer();
    void SaveDialogAndLogMessageControlRectangles();
    void AddAnchorsToFacilitateResizing();
    void LineupControlsAndAdjustSizes();
    void AdjustControlSizes();
    void SetCommitWindowTitleAndEnableStatus();
    void SetupLogMessageDefaultText();
    void ShowOrHideBugIdAndLabelControls();
    void HideBugIdAndLabel() const;
    void RestoreBugIdAndLabelFromProjectProperties();
    void SetupBugTraqControlsIfConfigured();
    void HideAndDisableBugTraqButton() const;
    void SetupToolTips();
    void SetControlAccessibilityProperties();
    void InitializeLogMessageControl();
    void InitializeListControl();
    void SubclassControls();
    void ReadPersistedDialogSettings();

    void            OnOK() override;
    void            OnCancel() override;
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    LRESULT         DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
    afx_msg void    OnBnClickedShowexternals();
    afx_msg void    OnBnClickedHelp();
    afx_msg void    OnBnClickedShowunversioned();
    afx_msg void    OnBnClickedHistory();
    afx_msg void    OnBnClickedBugtraqbutton();
    afx_msg void    OnBnClickedLog();
    afx_msg void    OnEnChangeLogmessage();
    afx_msg void    OnStnClickedExternalwarning();
    afx_msg BOOL    OnQueryEndSession();
    afx_msg LRESULT OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM);
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
    afx_msg LRESULT OnSVNStatusListCtrlCheckChanged(WPARAM, LPARAM);
    afx_msg LRESULT OnSVNStatusListCtrlChangelistChanged(WPARAM count, LPARAM);
    afx_msg LRESULT OnCheck(WPARAM count, LPARAM);
    afx_msg LRESULT OnAutoListReady(WPARAM, LPARAM);
    afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnSize(UINT nType, int cx, int cy);
    afx_msg void    OnBnClickedRunhook();
    void            Refresh();
    void            StartStatusThread();
    void            StopStatusThread();
    void            GetAutocompletionList(std::map<CString, int>& autolist);
    void            ScanFile(std::map<CString, int>& autolist, const CString& sFilePath, const CString& sRegex, const CString& sExt) const;
    void            DoSize(int delta);
    void            SetSplitterRange();
    void            SaveSplitterPos() const;
    void            ParseRegexFile(const CString& sFile, std::map<CString, CString>& mapRegex) const;
    void            UpdateCheckLinks();
    void            VersionCheck();

    DECLARE_MESSAGE_MAP()

public:
    ProjectProperties                               m_projectProperties;
    CTSVNPathList                                   m_pathList;
    CTSVNPathList                                   m_updatedPathList;
    CTSVNPathList                                   m_selectedPathList;
    CTSVNPathList                                   m_checkedPathList;
    BOOL                                            m_bRecursive;
    bool                                            m_bUnchecked;
    CSciEdit                                        m_cLogMessage;
    CString                                         m_sLogMessage;
    std::map<CString, CString>                      m_revProps;
    BOOL                                            m_bKeepLocks;
    CString                                         m_sBugID;
    CString                                         m_sChangeList;
    BOOL                                            m_bKeepChangeList;
    INT_PTR                                         m_itemsCount;
    bool                                            m_bSelectFilesForCommit;
    CComPtr<IBugTraqProvider>                       m_bugTraqProvider;
    std::map<CString, std::tuple<CString, CString>> m_restorePaths;

private:
    CWinThread*                m_pThread;
    std::map<CString, CString> m_snippet;
    CSVNStatusListCtrl         m_listCtrl;
    BOOL                       m_bShowUnversioned;
    BOOL                       m_bShowExternals;
    volatile LONG              m_bBlock;
    volatile LONG              m_bThreadRunning;
    volatile LONG              m_bRunThread;
    CRegDWORD                  m_regAddBeforeCommit;
    CRegDWORD                  m_regKeepChangelists;
    CRegDWORD                  m_regShowExternals;
    CString                    m_sWindowTitle;
    // ReSharper disable once CppInconsistentNaming
    static UINT         WM_AUTOLISTREADY;
    int                 m_nPopupPasteListCmd;
    CRegHistory         m_history;
    bool                m_bCancelled;
    CSplitterControl    m_wndSplitter;
    CRect               m_dlgOrigRect;
    CRect               m_logMsgOrigRect;
    CPathWatcher        m_pathWatcher;
    CLinkControl        m_checkAll;
    CLinkControl        m_checkNone;
    CLinkControl        m_checkUnversioned;
    CLinkControl        m_checkVersioned;
    CLinkControl        m_checkAdded;
    CLinkControl        m_checkDeleted;
    CLinkControl        m_checkModified;
    CLinkControl        m_checkFiles;
    CLinkControl        m_checkDirectories;
    CPathEdit           m_commitTo;
    CBugTraqAssociation m_bugtraqAssociation;
    CHyperLink          m_cUpdateLink;
};
