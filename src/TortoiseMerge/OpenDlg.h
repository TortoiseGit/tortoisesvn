#pragma once


// COpenDlg dialog

class COpenDlg : public CDialog
{
	DECLARE_DYNAMIC(COpenDlg)

public:
	COpenDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COpenDlg();

// Dialog Data
	enum { IDD = IDD_OPENDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL BrowseForFile(CString& filepath, CString title);
	void GroupRadio(UINT nID);
	DECLARE_MESSAGE_MAP()
public:
	CString m_sBaseFile;
	CString m_sTheirFile;
	CString m_sYourFile;
	CString m_sUnifiedDiffFile;
	CString m_sPatchDirectory;

	afx_msg void OnBnClickedBasefilebrowse();
	afx_msg void OnBnClickedTheirfilebrowse();
	afx_msg void OnBnClickedYourfilebrowse();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedDifffilebrowse();
	afx_msg void OnBnClickedDirectorybrowse();
	afx_msg void OnBnClickedMergeradio();
	afx_msg void OnBnClickedApplyradio();
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
};
