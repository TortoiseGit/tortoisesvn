#pragma once


// CRenameDlg dialog

class CRenameDlg : public CDialog
{
	DECLARE_DYNAMIC(CRenameDlg)

public:
	CRenameDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRenameDlg();

// Dialog Data
	enum { IDD = IDD_RENAME };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	CString m_name;
	CString m_windowtitle;
	CString m_label;
};
