#pragma once

#include "afxtempl.h"
#include "svnstatus.h"
#include "Registry.h"
#include "ResizableDialog.h"
#include "afxcmn.h"


// CChangedDlg dialog

class CChangedDlg : public CResizableDialog, public SVNStatus, public SVN
{
	DECLARE_DYNAMIC(CChangedDlg)

public:
	CChangedDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CChangedDlg();

	void AddEntry(CString file, svn_wc_status_t * status); 
// Dialog Data
	enum { IDD = IDD_CHANGEDFILES };

protected:
	virtual void			DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void			OnPaint();
	afx_msg HCURSOR			OnQueryDragIcon();
	afx_msg void			OnNMRclickChangedlist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL			OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
	HICON			m_hIcon;
	HANDLE			m_hThread;

public:
	CListCtrl		m_FileListCtrl;
	CString			m_path;
	CDWordArray		m_arWCStatus;
	CDWordArray		m_arRepoStatus;
	CDWordArray		m_arRevisions;
	CStringArray	m_arPaths;
};

DWORD WINAPI ChangedStatusThread(LPVOID pVoid);
