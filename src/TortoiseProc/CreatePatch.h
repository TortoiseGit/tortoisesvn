#pragma once
#include "StandAloneDlg.h"
#include "SVNStatusListCtrl.h"


// CCreatePatch dialog

class CCreatePatch : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCreatePatch)

public:
	CCreatePatch(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCreatePatch();

// Dialog Data
	enum { IDD = IDD_CREATEPATCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHelp();

	DECLARE_MESSAGE_MAP()

private:
	static UINT PatchThreadEntry(LPVOID pVoid);
	UINT PatchThread();

private:
	CSVNStatusListCtrl	m_PatchList;
	bool				m_bThreadRunning;
	CButton				m_SelectAll;
public:
	CTSVNPathList	m_pathList;
	CTSVNPathList		m_filesToRevert;
};
