#pragma once
#include "afxcmn.h"
#include "sciedit.h"
#include "StandAloneDlg.h"
#include "SVNStatusListCtrl.h"
#include "ProjectProperties.h"
#include "SciEdit.h"
#include "Registry.h"


// CLockDlg dialog

class CLockDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CLockDlg)

public:
	CLockDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLockDlg();


private:
	static UINT StatusThreadEntry(LPVOID pVoid);
	UINT StatusThread();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnChangeLockmessage();
	afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	void Refresh();

	DECLARE_MESSAGE_MAP()

// Dialog Data
	enum { IDD = IDD_LOCK };
public:
	CString				m_sLockMessage;
	BOOL				m_bStealLocks;
	CTSVNPathList		m_pathList;

private:
	CWinThread*			m_pThread;
	BOOL				m_bBlock;
	CSVNStatusListCtrl	m_cFileList;
	CSciEdit			m_cEdit;
	ProjectProperties	m_ProjectProperties;
};
