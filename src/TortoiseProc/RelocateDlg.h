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
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBrowse();

	DECLARE_MESSAGE_MAP()
public:
	CHistoryCombo m_URLCombo;
	CString m_sToUrl;
	CString m_sFromUrl;
};
