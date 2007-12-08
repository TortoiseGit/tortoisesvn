// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "Balloon.h"
#include "Registry.h"
#include "ILogReceiver.h"

class CProgressDlg;

/**
 * \ingroup TortoiseProc
 * Settings page to configure miscellaneous stuff. 
 */
class CSetLogCache 
    : public ISettingsPropPage
    , private ILogReceiver
{
	DECLARE_DYNAMIC(CSetLogCache)

public:
	CSetLogCache();
	virtual ~CSetLogCache();
	
	UINT GetIconID() {return IDI_DIALOGS;}

// Dialog Data
	enum { IDD = IDD_SETTINGSLOGCACHE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnChanged();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();

	afx_msg void OnBnClickedDetails();
	afx_msg void OnBnClickedUpdate();
	afx_msg void OnBnClickedExport();
	afx_msg void OnBnClickedDelete();

	afx_msg LRESULT OnRefeshRepositoryList (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
private:
	CBalloon		m_tooltips;

	CRegDWORD		m_regEnableLogCaching;
	BOOL			m_bEnableLogCaching;
    CRegDWORD		m_regDefaultConnectionState;
    CRegDWORD		m_regMaxHeadAge;
	DWORD			m_dwMaxHeadAge;

	CComboBox       m_cDefaultConnectionState;
	CListCtrl       m_cRepositoryList;

    /// current repository list

    typedef std::map<CString, CString> TURLs;
    typedef TURLs::const_iterator IT;
    TURLs           urls;

    CString GetSelectedUUID();
    void FillRepositoryList();

    static UINT WorkerThread(LPVOID pVoid);

    /// used by cache update

    CProgressDlg*   progress;
    svn_revnum_t    headRevision;

    void ReceiveLog ( LogChangedPathArray* changes
	                , svn_revnum_t rev
                    , const StandardRevProps* stdRevProps
                    , UserRevPropArray* userRevProps
                    , bool mergesFollow);
};
