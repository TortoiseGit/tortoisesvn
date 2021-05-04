// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013, 2015, 2021 - TortoiseSVN

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
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "RegHistory.h"
#include "SciEdit.h"
#include "TSVNPath.h"
#include "SVNRev.h"
#include "SVN.h"
#include "PathEdit.h"
#include "SVNExternals.h"
#include "HintCtrl.h"
#include "LinkControl.h"
#include "BugTraqAssociations.h"
#include "../IBugTraqProvider/IBugTraqProvider_h.h"

#define WM_TSVN_MAXREVFOUND (WM_APP + 1)

/// forward declarations

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Prompts the user for the required information needed for a copy command.
 * The required information is a single URL to copy the current URL of the
 * working copy to.
 */
class CCopyDlg : public CResizableStandAloneDialog
    , public SVN
{
    DECLARE_DYNAMIC(CCopyDlg)

public:
    CCopyDlg(CWnd* pParent = nullptr); // standard constructor
    ~CCopyDlg() override;

    SVNExternals GetExternalsToTag() const { return m_externals; }

    // Dialog Data
    enum
    {
        IDD = IDD_COPY
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL            OnInitDialog() override;
    void            OnOK() override;
    void            OnCancel() override;
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    afx_msg LRESULT OnRevFound(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnBnClickedBrowse();
    afx_msg void    OnBnClickedHelp();
    afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnBnClickedBrowsefrom();
    afx_msg void    OnBnClickedCopyhead();
    afx_msg void    OnBnClickedCopyrev();
    afx_msg void    OnBnClickedCopywc();
    afx_msg void    OnBnClickedHistory();
    afx_msg void    OnEnChangeLogmessage();
    afx_msg void    OnCbnEditchangeUrlcombo();
    afx_msg void    OnNMCustomdrawExternalslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetdispinfoExternalslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnKeydownExternalslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMClickExternalslist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedBugtraqbutton();
    afx_msg LRESULT OnCheck(WPARAM count, LPARAM);
    afx_msg void    OnEnChangeCopyrevtext();
    afx_msg BOOL    OnQueryEndSession();
    DECLARE_MESSAGE_MAP()

    virtual BOOL Cancel() override { return m_bCancelled; }
    void         SetRevision(const SVNRev& rev);
    void         ToggleCheckbox(int index);
    void         OnComError(HRESULT hr) const;

public:
    CString                    m_url;
    CTSVNPath                  m_path;
    CString                    m_sLogMessage;
    SVNRev                     m_copyRev;
    BOOL                       m_bDoSwitch;
    BOOL                       m_bMakeParents;
    std::map<CString, CString> m_revProps;

private:
    CLogDlg*                  m_pLogDlg;
    CSciEdit                  m_cLogMessage;
    CFont                     m_logFont;
    ProjectProperties         m_projectProperties;
    CString                   m_sBugID;
    CString                   m_repoRoot;
    CHistoryCombo             m_urlCombo;
    CString                   m_wcURL;
    CButton                   m_butBrowse;
    CRegHistory               m_history;
    CPathEdit                 m_fromUrl;
    CPathEdit                 m_destUrl;
    CHintCtrl<CListCtrl>      m_extList;
    wchar_t                   m_columnBuf[MAX_PATH];
    CBugTraqAssociation       m_bugtraqAssociation;
    CComPtr<IBugTraqProvider> m_bugTraqProvider;

    svn_revnum_t  m_maxRev;
    bool          m_bModified;
    bool          m_bSettingChanged;
    static UINT   FindRevThreadEntry(LPVOID pVoid);
    UINT          FindRevThread();
    CWinThread*   m_pThread;
    bool          m_bCancelled;
    volatile LONG m_bThreadRunning;
    SVNExternals  m_externals;
    CLinkControl  m_checkAll;
    CLinkControl  m_checkNone;
};