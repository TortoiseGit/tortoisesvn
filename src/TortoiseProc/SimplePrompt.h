#pragma once


// CSimplePrompt dialog

class CSimplePrompt : public CDialog
{
	DECLARE_DYNAMIC(CSimplePrompt)

public:
	CSimplePrompt(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSimplePrompt();

// Dialog Data
	enum { IDD = IDD_SIMPLEPROMPT };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	CString m_sUsername;
	CString m_sPassword;
	BOOL m_bSaveAuthentication;
};
