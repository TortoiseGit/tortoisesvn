// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "ExportDlg.h"
#include "RepositoryBrowser.h"
#include "Messagebox.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "AppUtils.h"


IMPLEMENT_DYNAMIC(CExportDlg, CStandAloneDialog)
CExportDlg::CExportDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CExportDlg::IDD, pParent)
	, Revision(_T("HEAD"))
	, m_strExportDirectory(_T(""))
	, m_bNonRecursive(FALSE)
	, m_bNoExternals(FALSE)
	, m_pLogDlg(NULL)
{
}

CExportDlg::~CExportDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_REVISION_NUM, m_editRevision);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Text(pDX, IDC_REVISION_NUM, m_sRevision);
	DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strExportDirectory);
	DDX_Check(pDX, IDC_NON_RECURSIVE, m_bNonRecursive);
	DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
	DDX_Control(pDX, IDC_CHECKOUTDIRECTORY, m_cCheckoutEdit);
	DDX_Control(pDX, IDC_EOLCOMBO, m_eolCombo);
}


BEGIN_MESSAGE_MAP(CExportDlg, CStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
	ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOW_LOG, OnBnClickedShowlog)
	ON_EN_CHANGE(IDC_REVISION_NUM, &CExportDlg::OnEnChangeRevisionNum)
	ON_CBN_SELCHANGE(IDC_EOLCOMBO, &CExportDlg::OnCbnSelchangeEolcombo)
END_MESSAGE_MAP()

BOOL CExportDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));
	m_URLCombo.SetCurSel(0);

	// set radio buttons according to the revision
	SetRevision(Revision);

	m_editRevision.SetWindowText(_T(""));

	if (!m_URL.IsEmpty())
		m_URLCombo.SetWindowText(m_URL);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_CHECKOUTDIRECTORY, IDS_CHECKOUT_TT_DIR);
	m_tooltips.AddTool(IDC_EOLCOMBO, IDS_EXPORT_TT_EOL);

	SHAutoComplete(GetDlgItem(IDC_CHECKOUTDIRECTORY)->m_hWnd, SHACF_FILESYSTEM);

	// fill the combobox with the choices of eol styles
	m_eolCombo.AddString(_T("default"));
	m_eolCombo.AddString(_T("CRLF"));
	m_eolCombo.AddString(_T("LF"));
	m_eolCombo.AddString(_T("CR"));
	m_eolCombo.SelectString(0, _T("default"));

	if (!Revision.IsHead())
	{
		// if the revision is not HEAD, change the radio button and
		// fill in the revision in the edit box
		CString temp;
		temp.Format(_T("%ld"), (LONG)Revision);
		m_editRevision.SetWindowText(temp);
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	}

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;
}

void CExportDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	// check it the export path is a valid windows path
	CTSVNPath ExportDirectory;
	ExportDirectory.SetFromWin(m_strExportDirectory);
	if (!ExportDirectory.IsValidOnWindows())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_CHECKOUTDIRECTORY), IDS_ERR_NOVALIDPATH, TRUE, IDI_EXCLAMATION);
		return;
	}

	// check if the specified revision is valid
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		Revision = SVNRev(_T("HEAD"));
	}
	else
		Revision = SVNRev(m_sRevision);
	if (!Revision.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_REVISION_NUM), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	// we need an url to export from - local paths won't work
	if (!SVN::PathIsURL(m_URL))
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_URLCOMBO), IDS_ERR_MUSTBEURL, TRUE, IDI_ERROR);
		return;
	}

	if (m_strExportDirectory.IsEmpty())
	{
		return;			//don't dismiss the dialog
	}

	// if the export directory does not exist, where should we export to?
	// We ask if the directory should be created...
	if (!PathFileExists(m_strExportDirectory))
	{
		CString temp;
		temp.Format(IDS_WARN_FOLDERNOTEXIST, (LPCTSTR)m_strExportDirectory);
		if (CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CPathUtils::MakeSureDirectoryPathExists(m_strExportDirectory);
		}
		else
			return;		//don't dismiss the dialog
	}

	// if the directory we should export to is not empty, show a warning:
	// maybe the user doesn't want to overwrite the existing files.
	if (!PathIsDirectoryEmpty(m_strExportDirectory))
	{
		CString message;
		message.Format(CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY)),m_strExportDirectory);
		if (CMessageBox::Show(this->m_hWnd, message, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;		//don't dismiss the dialog
	}
	m_eolCombo.GetWindowText(m_eolStyle);
	if (m_eolStyle.Compare(_T("default"))==0)
		m_eolStyle.Empty();

	UpdateData(FALSE);
	CStandAloneDialog::OnOK();
}

void CExportDlg::OnBnClickedBrowse()
{
	SVNRev rev;
	UpdateData();
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		rev = SVNRev::REV_HEAD;
	}
	else
		rev = SVNRev(m_sRevision);
	if (!rev.IsValid())
		rev = SVNRev::REV_HEAD;
	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
	SetRevision(rev);
}

void CExportDlg::OnBnClickedCheckoutdirectoryBrowse()
{
	//
	// Create a folder browser dialog. If the user selects OK, we should update
	// the local data members with values from the controls, copy the checkout
	// directory from the browse folder, then restore the local values into the
	// dialog controls.
	//
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCheckoutDirectory;
	if (browseFolder.Show(GetSafeHwnd(), strCheckoutDirectory) == CBrowseFolder::OK) 
	{
		UpdateData(TRUE);
		m_strExportDirectory = strCheckoutDirectory;
		UpdateData(FALSE);
	}
}

BOOL CExportDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CStandAloneDialog::PreTranslateMessage(pMsg);
}

void CExportDlg::OnEnChangeCheckoutdirectory()
{
	UpdateData(TRUE);
	DialogEnableWindow(IDOK, !m_strExportDirectory.IsEmpty());
}

void CExportDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CExportDlg::OnBnClickedShowlog()
{
	UpdateData(TRUE);
	m_URL = m_URLCombo.GetString();
	if ((m_pLogDlg)&&(m_pLogDlg->IsWindowVisible()))
		return;
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for working copy
	if (!m_URL.IsEmpty())
	{
		delete [] m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		m_pLogDlg->SetParams(CTSVNPath(m_URL), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100));
		m_pLogDlg->m_wParam = 1;
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CExportDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	GetDlgItem(IDC_REVISION_NUM)->SetWindowText(temp);
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}

void CExportDlg::OnEnChangeRevisionNum()
{
	UpdateData();
	if (m_sRevision.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CExportDlg::OnCbnSelchangeEolcombo()
{
}

void CExportDlg::SetRevision(const SVNRev& rev)
{
	if (rev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		CString sRev;
		sRev.Format(_T("%ld"), (LONG)rev);
		GetDlgItem(IDC_REVISION_NUM)->SetWindowText(sRev);
	}
}