// OpenDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseMerge.h"
#include "BrowseFolder.h"
#include "OpenDlg.h"
#include ".\opendlg.h"


// COpenDlg dialog

IMPLEMENT_DYNAMIC(COpenDlg, CDialog)
COpenDlg::COpenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpenDlg::IDD, pParent)
	, m_sBaseFile(_T(""))
	, m_sTheirFile(_T(""))
	, m_sYourFile(_T(""))
	, m_sUnifiedDiffFile(_T(""))
	, m_sPatchDirectory(_T(""))
{
}

COpenDlg::~COpenDlg()
{
}

void COpenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BASEFILEEDIT, m_sBaseFile);
	DDX_Text(pDX, IDC_THEIRFILEEDIT, m_sTheirFile);
	DDX_Text(pDX, IDC_YOURFILEEDIT, m_sYourFile);
	DDX_Text(pDX, IDC_DIFFFILEEDIT, m_sUnifiedDiffFile);
	DDX_Text(pDX, IDC_DIRECTORYEDIT, m_sPatchDirectory);
}

BEGIN_MESSAGE_MAP(COpenDlg, CDialog)
	ON_BN_CLICKED(IDC_BASEFILEBROWSE, OnBnClickedBasefilebrowse)
	ON_BN_CLICKED(IDC_THEIRFILEBROWSE, OnBnClickedTheirfilebrowse)
	ON_BN_CLICKED(IDC_YOURFILEBROWSE, OnBnClickedYourfilebrowse)
	ON_BN_CLICKED(IDC_HELPBUTTON, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_DIFFFILEBROWSE, OnBnClickedDifffilebrowse)
	ON_BN_CLICKED(IDC_DIRECTORYBROWSE, OnBnClickedDirectorybrowse)
	ON_BN_CLICKED(IDC_MERGERADIO, OnBnClickedMergeradio)
	ON_BN_CLICKED(IDC_APPLYRADIO, OnBnClickedApplyradio)
END_MESSAGE_MAP()

BOOL COpenDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GroupRadio(IDC_MERGERADIO);

	CheckRadioButton(IDC_MERGERADIO, IDC_APPLYRADIO, IDC_MERGERADIO);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

// COpenDlg message handlers

void COpenDlg::OnBnClickedBasefilebrowse()
{
	CString temp;
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sBaseFile, temp);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedTheirfilebrowse()
{
	CString temp;
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sTheirFile, temp);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedYourfilebrowse()
{
	CString temp;
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sYourFile, temp);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedHelp()
{
	this->OnHelp();
}

BOOL COpenDlg::BrowseForFile(CString& filepath, CString title)
{
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	//ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	ofn.lpstrFilter = _T("All\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		filepath = CString(ofn.lpstrFile);
		return TRUE;
	} // if (GetOpenFileName(&ofn)==TRUE)

	return FALSE;			//user cancelled the dialog
}

void COpenDlg::OnBnClickedDifffilebrowse()
{
	CString temp;
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sUnifiedDiffFile, temp);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedDirectorybrowse()
{
	CBrowseFolder folderBrowser;
	folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	folderBrowser.Show(GetSafeHwnd(), m_sPatchDirectory);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedMergeradio()
{
	GroupRadio(IDC_MERGERADIO);
}

void COpenDlg::OnBnClickedApplyradio()
{
	GroupRadio(IDC_APPLYRADIO);
}

void COpenDlg::GroupRadio(UINT nID)
{
	BOOL bMerge = FALSE;
	BOOL bUnified = FALSE;
	if (nID == IDC_MERGERADIO)
		bMerge = TRUE;
	if (nID == IDC_APPLYRADIO)
		bUnified = TRUE;

	GetDlgItem(IDC_BASEFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_BASEFILEBROWSE)->EnableWindow(bMerge);
	GetDlgItem(IDC_THEIRFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_THEIRFILEBROWSE)->EnableWindow(bMerge);
	GetDlgItem(IDC_YOURFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_YOURFILEBROWSE)->EnableWindow(bMerge);

	GetDlgItem(IDC_DIFFFILEEDIT)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIFFFILEBROWSE)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIRECTORYEDIT)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIRECTORYBROWSE)->EnableWindow(bUnified);
}

void COpenDlg::OnOK()
{
	UpdateData(TRUE);
	if (GetDlgItem(IDC_BASEFILEEDIT)->IsWindowEnabled())
	{
		m_sUnifiedDiffFile.Empty();
		m_sPatchDirectory.Empty();
	} // if (GetDlgItem(IDC_BASEFILEEDIT)->IsWindowEnabled())
	else
	{
		m_sBaseFile.Empty();
		m_sYourFile.Empty();
		m_sTheirFile.Empty();
	}
	UpdateData(FALSE);
	CDialog::OnOK();
}
