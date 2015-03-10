// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014-2015 - TortoiseSVN

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
#include "EditPropBase.h"
#include "SVNRev.h"
#include "ProjectProperties.h"
#include "tstring.h"
#include "PathEdit.h"
#include "CriticalSection.h"
#include "UserProperties.h"

/**
 * \ingroup TortoiseProc
 * dialog showing a list of properties of the files/folders specified with SetPathList().
 */
class CEditPropertiesDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CEditPropertiesDlg)

public:
    CEditPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEditPropertiesDlg();

    void    SetPathList(const CTSVNPathList& pathlist) {m_pathlist = pathlist;}
    void    SetRevision(const SVNRev& rev) {m_revision = rev;}
    void    Refresh();
    bool    HasChanged() {return m_bChanged;}

    void    SetProjectProperties(ProjectProperties * pProps) {m_pProjectProperties = pProps;}
    void    SetUUID(const CString& sUUID) {m_sUUID = sUUID;}
    void    RevProps(bool bRevProps = false) {m_bRevProps = bRevProps;}
    void    UrlIsFolder(bool bFolder) {m_bUrlIsFolder = bFolder;}
    void    SetInitPropName(const std::wstring& pn) { m_propname = pn; }
// Dialog Data
    enum { IDD = IDD_EDITPROPERTIES };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnBnClickedHelp();
    afx_msg void OnNMCustomdrawEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedRemoveProps();
    afx_msg void OnBnClickedEditprops();
    afx_msg void OnLvnItemchangedEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclkEditproplist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedSaveprop();
    afx_msg void OnBnClickedAddprops();
    afx_msg void OnBnClickedExport();
    afx_msg void OnBnClickedImport();
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    /// called after the thread has finished
    LRESULT OnAfterThread(WPARAM /*wParam*/, LPARAM /*lParam*/);

    DECLARE_MESSAGE_MAP()
private:
    static UINT PropsThreadEntry(LPVOID pVoid);

    void ReadProperties (int first, int last);
    UINT PropsThread();
    void EditProps(bool bDefault, const std::string& propName = "", bool bAdd = false);
    void RemoveProps();
    EditPropBase * GetPropDialog(bool bDefault, const std::string& sName);

protected:

    async::CCriticalSection m_mutex;
    CTSVNPathList   m_pathlist;
    CListCtrl       m_propList;
    BOOL            m_bRecursive;
    bool            m_bChanged;
    bool            m_bRevProps;
    bool            m_bUrlIsFolder;
    volatile LONG   m_bThreadRunning;

    TProperties     m_properties;
    SVNRev          m_revision;
    CPathEdit       m_PropPath;
    std::vector<UserProp>   m_userProperties;
    CMFCMenuButton  m_btnNew;
    CMFCMenuButton  m_btnEdit;
    CMenu           m_editMenu;
    CMenu           m_newMenu;
    std::wstring    m_propname;

    CString         m_sUUID;
    ProjectProperties * m_pProjectProperties;
    bool            m_bCancelled;
};

static UINT WM_AFTERTHREAD = RegisterWindowMessage(L"TORTOISESVN_AFTERTHREAD_MSG");
