// ChangedDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ChangedDlg.h"
#include "messagebox.h"
#include "cursor.h"


// CChangedDlg dialog

IMPLEMENT_DYNAMIC(CChangedDlg, CResizableDialog)
CChangedDlg::CChangedDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CChangedDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CChangedDlg::~CChangedDlg()
{
}

void CChangedDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHANGEDLIST, m_FileListCtrl);
}


BEGIN_MESSAGE_MAP(CChangedDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


void CChangedDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CResizableDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CChangedDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CChangedDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_FileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT );

	m_FileListCtrl.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_FileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_FileListCtrl.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_CHSTAT_FILECOL);
	m_FileListCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_CHSTAT_WCCOL);
	m_FileListCtrl.InsertColumn(1, temp);
	temp.LoadString(IDS_CHSTAT_REPOCOL);
	m_FileListCtrl.InsertColumn(2, temp);

	m_FileListCtrl.SetRedraw(FALSE);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_FileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_FileListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_FileListCtrl.SetRedraw(TRUE);

	//first start a thread to obtain the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &ChangedStatusThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK);
	}
	GetDlgItem(IDC_CHANGEDLIST)->UpdateData(FALSE);


	AddAnchor(IDC_CHANGEDLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
void CChangedDlg::AddEntry(CString file, svn_wc_status_t * status)
{
	int index = m_FileListCtrl.GetItemCount();

	svn_wc_status_kind text = svn_wc_status_none;
	if (status->text_status != svn_wc_status_ignored)
		text = status->text_status;
	if (status->prop_status != svn_wc_status_ignored)
		text = text > status->prop_status ? text : status->prop_status;

	svn_wc_status_kind repo = svn_wc_status_none;
	if (status->repos_text_status != svn_wc_status_ignored)
		repo = status->repos_text_status;
	if (status->repos_prop_status != svn_wc_status_ignored)
		repo = repo > status->repos_prop_status ? repo : status->repos_prop_status;

	if ((text > svn_wc_status_normal)||(repo > svn_wc_status_normal))
	{
		TCHAR buf[MAX_PATH];
		m_FileListCtrl.InsertItem(index, file);
		GetStatusString(theApp.m_hInstance, text, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
		m_FileListCtrl.SetItemText(index, 1, buf);
		GetStatusString(theApp.m_hInstance, repo, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID()));
		m_FileListCtrl.SetItemText(index, 2, buf);

		m_FileListCtrl.SetRedraw(FALSE);
		int mincol = 0;
		int maxcol = ((CHeaderCtrl*)(m_FileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
		int col;
		for (col = mincol; col <= maxcol; col++)
		{
			m_FileListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
		}
		m_FileListCtrl.SetRedraw(TRUE);

		m_arWCStatus.Add(text);
		m_arRepoStatus.Add(repo);
		m_arPaths.Add(file);
	}
}

DWORD WINAPI ChangedStatusThread(LPVOID pVoid)
{
	svn_wc_status_t * status;
	const TCHAR * file = NULL;
	CChangedDlg * pDlg;
	pDlg = (CChangedDlg *)pVoid;

	pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	
	status = pDlg->GetFirstFileStatus(pDlg->m_path, &file, true);
	if (status)
	{
		pDlg->AddEntry(file, status);
		while (status = pDlg->GetNextFileStatus(&file))
		{
			pDlg->AddEntry(CString(file), status);
		}
	} // if (status)
	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	return 0;
}
