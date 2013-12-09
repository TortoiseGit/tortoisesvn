// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2010, 2013 - TortoiseSVN

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
#include "ProjectProperties.h"
#include "SciEdit.h"
#include "BugTraqAssociations.h"
#include "../IBugTraqProvider/IBugTraqProvider_h.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog to let the user enter a log/commit message.
 */
class CInputLogDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CInputLogDlg)

public:
    CInputLogDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CInputLogDlg();

    void SetProjectProperties(ProjectProperties * pProps, const CStringA& sAction) { m_pProjectProperties = pProps; m_sSVNAction = sAction; }
    void SetPathList(const CTSVNPathList& pl) { m_pathlist = pl; }
    void SetRootPath(const CTSVNPath& p) { m_rootpath = p; }
    void SetUUID(const CString& sUUID) {m_sUUID = sUUID;}
    void SetActionText(const CString& sAction) {m_sActionText = sAction;}
    void SetTitleText(const CString& sTitle) { m_sTitleText = sTitle; }
    void SetCheckText(const CString& sCheck) { m_sCheckText = sCheck; }
    void SetLogText(const CString& sLog) { m_sLogMsg = sLog; }
    int GetCheck() { return m_iCheck; }
    CString GetLogMessage() {return m_sLogMsg;}

    std::map<CString, CString> m_revProps;

protected:
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnOK();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    afx_msg void OnEnChangeLogmessage();
    afx_msg void OnBnClickedHistory();
    afx_msg void OnBnClickedBugtraqbutton();
    DECLARE_MESSAGE_MAP()

// Dialog Data
    enum { IDD = IDD_INPUTLOGDLG };

private:
    void        UpdateOKButton();
    void        OnComError(HRESULT hr);


private:
    CSciEdit            m_cInput;
    CString             m_sBugID;
    ProjectProperties * m_pProjectProperties;
    CComPtr<IBugTraqProvider> m_BugTraqProvider;
    CBugTraqAssociation m_bugtraq_association;
    CTSVNPathList       m_pathlist;
    CTSVNPath           m_rootpath;
    CStringA            m_sSVNAction;
    CFont               m_logFont;
    CString             m_sLogMsg;
    CString             m_sUUID;
    CString             m_sActionText;
    CString             m_sTitleText;
    CString             m_sCheckText;
    int                 m_iCheck;
    bool                m_bLock;
};
