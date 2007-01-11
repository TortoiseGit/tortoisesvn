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
#include "CheckoutDlg.h"
#include "RepositoryBrowser.h"
#include "Messagebox.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "AppUtils.h"


IMPLEMENT_DYNAMIC(CCheckoutDlg, CStandAloneDialog)
CCheckoutDlg::CCheckoutDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CCheckoutDlg::IDD, pParent)
	, Revision(_T("HEAD"))
	, m_strCheckoutDirectory(_T(""))
	, m_sCheckoutDirOrig(_T(""))
	, m_bNonRecursive(FALSE)
	, m_bNoExternals(FALSE)
	, m_pLogDlg(NULL)
{
}

CCheckoutDlg::~CCheckoutDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CCheckoutDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_REVISION_NUM, m_editRevision);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Text(pDX, IDC_REVISION_NUM, m_sRevision);
	DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strCheckoutDirectory);
	DDX_Check(pDX, IDC_NON_RECURSIVE, m_bNonRecursive);
	DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
	DDX_Control(pDX, IDC_CHECKOUTDIRECTORY, m_cCheckoutEdit);
}


BEGIN_MESSAGE_MAP(CCheckoutDlg, CStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
	ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOW_LOG, OnBnClickedShowlog)
	ON_EN_CHANGE(IDC_REVISION_NUM, &CCheckoutDlg::OnEnChangeRevisionNum)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCheckoutDlg::OnCbnEditchangeUrlcombo)
END_MESSAGE_MAP()

BOOL CCheckoutDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	m_sCheckoutDirOrig = m_strCheckoutDirectory;
	m_bAutoCreateTargetName = !PathIsDirectoryEmpty(m_sCheckoutDirOrig);

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

	SHAutoComplete(GetDlgItem(IDC_CHECKOUTDIRECTORY)->m_hWnd, SHACF_FILESYSTEM);

	if (!Revision.IsHead())
	{
		CString temp;
		temp.Format(_T("%ld"), (LONG)Revision);
		m_editRevision.SetWindowText(temp);
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	}

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;
}

void CCheckoutDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	CTSVNPath m_CheckoutDirectory;
	m_CheckoutDirectory.SetFromWin(m_strCheckoutDirectory);
	if (!m_CheckoutDirectory.IsValidOnWindows())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_CHECKOUTDIRECTORY), IDS_ERR_NOVALIDPATH, TRUE, IDI_EXCLAMATION);
		return;
	}

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

	bool bAutoCreateTargetName = m_bAutoCreateTargetName;
	m_bAutoCreateTargetName = false;
	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	if (!SVN::PathIsURL(m_URL))
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_URLCOMBO), IDS_ERR_MUSTBEURL, TRUE, IDI_ERROR);
		m_bAutoCreateTargetName = bAutoCreateTargetName;
		return;
	}

	if (m_URL.Left(7).CompareNoCase(_T("file://"))==0)
	{
		//check if the url is on a network share
		CString temp = m_URL.Mid(7);
		temp = temp.TrimLeft('/');
		temp.Replace('/', '\\');
		temp = temp.Left(3);
		if (GetDriveType(temp)==DRIVE_REMOTE)
		{
			if (SVN::IsBDBRepository(m_URL))
				// It's a network share, and the user tries to create a berkeley db on it.
				// Show a warning telling the user about the risks of doing so.
				if (CMessageBox::Show(this->m_hWnd, IDS_WARN_SHAREFILEACCESS, IDS_APPNAME, MB_ICONWARNING | MB_YESNO)==IDNO)
				{
					m_bAutoCreateTargetName = bAutoCreateTargetName;
					return;
				}
		}
	}

	if (m_strCheckoutDirectory.IsEmpty())
	{
		return;			//don't dismiss the dialog
	}
	if (!PathFileExists(m_strCheckoutDirectory))
	{
		CString temp;
		temp.Format(IDS_WARN_FOLDERNOTEXIST, (LPCTSTR)m_strCheckoutDirectory);
		if (CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CPathUtils::MakeSureDirectoryPathExists(m_strCheckoutDirectory);
		}
		else
		{
			m_bAutoCreateTargetName = bAutoCreateTargetName;
			return;		//don't dismiss the dialog
		}
	}
	if (!PathIsDirectoryEmpty(m_strCheckoutDirectory))
	{
		CString message;
		message.Format(CString(MAKEINTRESOURCE(IDS_WARN_FOLDERNOTEMPTY)),m_strCheckoutDirectory);
		if (CMessageBox::Show(this->m_hWnd, message, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		{
			m_bAutoCreateTargetName = bAutoCreateTargetName;
			return;		//don't dismiss the dialog
		}
	}
	UpdateData(FALSE);
	CStandAloneDialog::OnOK();
}

void CCheckoutDlg::OnBnClickedBrowse()
{
	SVNRev rev;
	UpdateData();
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		rev = SVNRev(_T("HEAD"));
	}
	else
		rev = SVNRev(m_sRevision);

	if (!rev.IsValid())
		rev = SVNRev::REV_HEAD;
	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
	SetRevision(rev);


	CRegString regDefCheckoutUrl(_T("Software\\TortoiseSVN\\DefaultCheckoutUrl"));
	CRegString regDefCheckoutPath(_T("Software\\TortoiseSVN\\DefaultCheckoutPath"));
	if (!CString(regDefCheckoutUrl).IsEmpty())
	{
		m_URL = m_URLCombo.GetString();
		CTSVNPath url = CTSVNPath(m_URL);
		CTSVNPath defurl = CTSVNPath(CString(regDefCheckoutUrl));
		if (defurl.IsAncestorOf(url))
		{
			if (CTSVNPath::CheckChild(CTSVNPath(CString(regDefCheckoutPath)), CTSVNPath(m_strCheckoutDirectory)))
			{
				// the default url is the parent of the specified url
				m_strCheckoutDirectory = CString(regDefCheckoutPath) + url.GetWinPathString().Mid(defurl.GetWinPathString().GetLength());
				m_strCheckoutDirectory.Replace(_T("\\\\"), _T("\\"));
				UpdateData(FALSE);
			}
		}
	}
}

void CCheckoutDlg::OnBnClickedCheckoutdirectoryBrowse()
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
		m_strCheckoutDirectory = strCheckoutDirectory;
		m_sCheckoutDirOrig = m_strCheckoutDirectory;
		m_bAutoCreateTargetName = !PathIsDirectoryEmpty(m_sCheckoutDirOrig);
		UpdateData(FALSE);
	}
}

BOOL CCheckoutDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCheckoutDlg::OnEnChangeCheckoutdirectory()
{
	UpdateData(TRUE);		
	DialogEnableWindow(IDOK, !m_strCheckoutDirectory.IsEmpty());
}

void CCheckoutDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CCheckoutDlg::OnBnClickedShowlog()
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

LPARAM CCheckoutDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	GetDlgItem(IDC_REVISION_NUM)->SetWindowText(temp);
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}

void CCheckoutDlg::OnEnChangeRevisionNum()
{
	UpdateData();
	if (m_sRevision.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CCheckoutDlg::SetRevision(const SVNRev& rev)
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

void CCheckoutDlg::OnCbnEditchangeUrlcombo()
{
	if (!m_bAutoCreateTargetName)
		return;
	if (m_sCheckoutDirOrig.IsEmpty())
		return;
	// find out what to use as the checkout directory name
	UpdateData();
	m_URLCombo.GetWindowText(m_URL);
	if (m_URL.IsEmpty())
		return;
	CString tempURL = m_URL;
	CString name;
	while (name.IsEmpty() || (name.CompareNoCase(_T("branches"))==0) ||
		(name.CompareNoCase(_T("tags"))==0) ||
		(name.CompareNoCase(_T("trunk"))==0))
	{
		name = tempURL.Mid(tempURL.ReverseFind('/')+1);
		tempURL = tempURL.Left(tempURL.ReverseFind('/'));
	}
	m_strCheckoutDirectory = m_sCheckoutDirOrig+_T('\\')+name;
	UpdateData(FALSE);
}
