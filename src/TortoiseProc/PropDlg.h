#pragma once
#include "afxcmn.h"
#include "ResizableDialog.h"


// CPropDlg dialog

class CPropDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CPropDlg)

public:
	CPropDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPropDlg();

// Dialog Data
	enum { IDD = IDD_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

	HANDLE		m_hThread;
public:
	CListCtrl	m_proplist;
	CString		m_sPath;
	SVNRev		m_rev;
};

DWORD WINAPI PropThread(LPVOID pVoid);
