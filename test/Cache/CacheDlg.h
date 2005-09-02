// CacheDlg.h : header file
//

#pragma once
#include "TSVNPath.h"

// CCacheDlg dialog
class CCacheDlg : public CDialog
{
// Construction
public:
	CCacheDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_CACHE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedWatchtestbutton();

	DECLARE_MESSAGE_MAP()

	CString m_sRootPath;
	CStringArray m_filelist;
	HANDLE m_hPipe;
	OVERLAPPED m_Overlapped;
	HANDLE m_hEvent;
	CComCriticalSection m_critSec;
	static UINT TestThreadEntry(LPVOID pVoid);
	UINT TestThread();
	void ClosePipe();
	bool EnsurePipeOpen();
	bool GetStatusFromRemoteCache(const CTSVNPath& Path, bool bRecursive);
	void RemoveFromCache(const CString& path);

	void TouchFile(const CString& path);
	
	static UINT WatchTestThreadEntry(LPVOID pVoid);
	UINT WatchTestThread();
public:
};
