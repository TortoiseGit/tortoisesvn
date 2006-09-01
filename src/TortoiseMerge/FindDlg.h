#pragma once


// CFindDlg dialog

class CFindDlg : public CDialog
{
	DECLARE_DYNAMIC(CFindDlg)

public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindDlg();
	void Create(CWnd * pParent = NULL) {CDialog::Create(IDD, pParent);ShowWindow(SW_SHOW);UpdateWindow();}
	bool IsTerminating() {return m_bTerminating;}
	bool FindNext() {return m_bFindNext;}
	bool MatchCase() {return !!m_bMatchCase;}
	bool LimitToDiffs() {return !!m_bLimitToDiffs;}
	bool WholeWord() {return !!m_bWholeWord;}
	CString GetFindString() {return m_sFindText;}
// Dialog Data
	enum { IDD = IDD_FIND };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeSearchtext();
private:
	UINT		m_FindMsg;
	bool		m_bTerminating;
	bool		m_bFindNext;
	BOOL		m_bMatchCase;
	BOOL		m_bLimitToDiffs;
	BOOL		m_bWholeWord;
	CString		m_sFindText;
};
