#pragma once


// CRelocateDlg dialog

class CRelocateDlg : public CDialog
{
	DECLARE_DYNAMIC(CRelocateDlg)

public:
	CRelocateDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRelocateDlg();

// Dialog Data
	enum { IDD = IDD_RELOCATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_sToUrl;
	CString m_sFromUrl;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBrowse();
};
