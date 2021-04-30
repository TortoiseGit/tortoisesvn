// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014-2015, 2017, 2020-2021 - TortoiseSVN

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
#include "PathEdit.h"
#include "CriticalSection.h"
#include "UserProperties.h"
#include "HintCtrl.h"
#include "ThemeControls.h"

/**
 * \ingroup TortoiseProc
 * dialog showing a list of properties of the files/folders specified with SetPathList().
 */
class CEditPropertiesDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CEditPropertiesDlg)

public:
    CEditPropertiesDlg(CWnd* pParent = nullptr); // standard constructor
    ~CEditPropertiesDlg() override;

    void SetPathList(const CTSVNPathList& pathlist) { m_pathList = pathlist; }
    void SetRevision(const SVNRev& rev) { m_revision = rev; }
    void Refresh();
    bool HasChanged() const { return m_bChanged; }

    void SetProjectProperties(ProjectProperties* pProps) { m_pProjectProperties = pProps; }
    void SetUUID(const CString& sUuid) { m_sUuid = sUuid; }
    void RevProps(bool bRevProps = false) { m_bRevProps = bRevProps; }
    void UrlIsFolder(bool bFolder) { m_bUrlIsFolder = bFolder; }
    void SetInitPropName(const std::wstring& pn) { m_propname = pn; }
    // Dialog Data
    enum
    {
        IDD = IDD_EDITPROPERTIES
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    void         OnOK() override;
    void         OnCancel() override;
    BOOL         PreTranslateMessage(MSG* pMsg) override;
    afx_msg void OnBnClickedHelp();
    afx_msg void OnNMCustomdrawEditproplist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedRemoveProps();
    afx_msg void OnBnClickedEditprops();
    afx_msg void OnLvnItemchangedEditproplist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMDblclkEditproplist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnHdnItemclickEditproplist(NMHDR* pNMHDR, LRESULT* pResult);
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

    void                ReadProperties(int first, int last);
    UINT                PropsThread();
    void                EditProps(bool bDefault, const std::string& propName = "", bool bAdd = false);
    void                RemoveProps();
    EditPropBase*       GetPropDialog(bool bDefault, const std::string& sName);
    void                FillListControl();
    static int CALLBACK SortCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

protected:
    async::CCriticalSection m_mutex;
    CTSVNPathList           m_pathList;
    CHintCtrl<CListCtrl>    m_propList;
    BOOL                    m_bRecursive;
    bool                    m_bChanged;
    bool                    m_bRevProps;
    bool                    m_bUrlIsFolder;
    volatile LONG           m_bThreadRunning;

    TProperties           m_properties;
    SVNRev                m_revision;
    CPathEdit             m_propPath;
    std::vector<UserProp> m_userProperties;
    CThemeMFCMenuButton   m_btnNew;
    CThemeMFCMenuButton   m_btnEdit;
    CMenu                 m_editMenu;
    CMenu                 m_newMenu;
    std::wstring          m_propname;

    CString            m_sUuid;
    ProjectProperties* m_pProjectProperties;
    bool               m_bCancelled;

    int  m_nSortedColumn;
    bool m_bAscending;
};

static UINT WM_AFTERTHREAD = RegisterWindowMessage(L"TORTOISESVN_AFTERTHREAD_MSG");
