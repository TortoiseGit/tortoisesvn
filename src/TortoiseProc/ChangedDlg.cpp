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
#include "ChangedDlg.h"
#include "messagebox.h"
#include "cursor.h"
#include ".\changeddlg.h"


IMPLEMENT_DYNAMIC(CChangedDlg, CResizableStandAloneDialog)
CChangedDlg::CChangedDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CChangedDlg::IDD, pParent)
	, m_bShowUnversioned(FALSE)
	, m_iShowUnmodified(0)
	, m_bBlock(FALSE)
	, m_bCanceled(false)
{
	m_bRemote = FALSE;
}

CChangedDlg::~CChangedDlg()
{
}

void CChangedDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHANGEDLIST, m_FileListCtrl);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Check(pDX, IDC_SHOWUNMODIFIED, m_iShowUnmodified);
}


BEGIN_MESSAGE_MAP(CChangedDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_CHECKREPO, OnBnClickedCheckrepo)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_BN_CLICKED(IDC_SHOWUNMODIFIED, OnBnClickedShowUnmodified)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
END_MESSAGE_MAP()

BOOL CChangedDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	GetWindowText(m_sTitle);

	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;
	UpdateData(FALSE);

	m_FileListCtrl.Init(SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS | 
						SVNSLC_COLREMOTETEXT | SVNSLC_COLREMOTEPROP | 
						SVNSLC_COLLOCK | SVNSLC_COLLOCKCOMMENT |
						SVNSLC_COLAUTHOR | SVNSLC_COLAUTHOR |
						SVNSLC_COLREVISION | SVNSLC_COLDATE, SVNSLC_POPALL, false);
	m_FileListCtrl.SetCancelBool(&m_bCanceled);
	
	AddAnchor(IDC_CHANGEDLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SUMMARYTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNMODIFIED, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CHECKREPO, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	SetPromptParentWindow(m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("ChangedDlg"));

	m_bRemote = !!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\CheckRepo"), FALSE);
	
	//first start a thread to obtain the status without
	//blocking the dialog
	if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

UINT CChangedDlg::ChangedStatusThreadEntry(LPVOID pVoid)
{
	return ((CChangedDlg*)pVoid)->ChangedStatusThread();
}

UINT CChangedDlg::ChangedStatusThread()
{
	m_bBlock = TRUE;
	m_bCanceled = false;
	GetDlgItem(IDOK)->SetWindowText(CString(MAKEINTRESOURCE(IDS_MSGBOX_CANCEL)));
	GetDlgItem(IDC_CHECKREPO)->EnableWindow(FALSE);
	GetDlgItem(IDC_SHOWUNVERSIONED)->EnableWindow(FALSE);
	GetDlgItem(IDC_SHOWUNMODIFIED)->EnableWindow(FALSE);
	CString temp;
	if (!m_FileListCtrl.GetStatus(m_pathList, m_bRemote))
	{
		CMessageBox::Show(m_hWnd, m_FileListCtrl.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
	}
	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL | SVNSLC_SHOWLOCKS;
	dwShow |= m_bShowUnversioned ? SVNSLC_SHOWUNVERSIONED : 0;
	dwShow |= m_iShowUnmodified ? SVNSLC_SHOWNORMAL : 0;
	m_FileListCtrl.Show(dwShow);
	LONG lMin, lMax;
	m_FileListCtrl.GetMinMaxRevisions(lMin, lMax);
	if (LONG(m_FileListCtrl.m_HeadRev) >= 0)
	{
		temp.Format(IDS_REPOSTATUS_HEADREV, lMin, lMax, LONG(m_FileListCtrl.m_HeadRev));
		GetDlgItem(IDC_SUMMARYTEXT)->SetWindowText(temp);
	}
	else
	{
		temp.Format(IDS_REPOSTATUS_WCINFO, lMin, lMax);
		GetDlgItem(IDC_SUMMARYTEXT)->SetWindowText(temp);
	}
	CTSVNPath commonDir = m_FileListCtrl.GetCommonDirectory(false);
	SetWindowText(m_sTitle + _T(" - ") + commonDir.GetWinPathString());
	GetDlgItem(IDOK)->SetWindowText(CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)));
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	GetDlgItem(IDC_CHECKREPO)->EnableWindow(TRUE);
	GetDlgItem(IDC_SHOWUNVERSIONED)->EnableWindow(TRUE);
	GetDlgItem(IDC_SHOWUNMODIFIED)->EnableWindow(TRUE);
	m_bBlock = FALSE;
	return 0;
}

void CChangedDlg::OnOK()
{
	if (m_bBlock)
	{
		m_bCanceled = true;
		return;
	}
	__super::OnOK();
}

void CChangedDlg::OnCancel()
{
	if (m_bBlock)
	{
		m_bCanceled = true;
		return;
	}
	__super::OnCancel();
}

void CChangedDlg::OnBnClickedCheckrepo()
{
	m_bRemote = TRUE;
	if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CChangedDlg::OnBnClickedShowunversioned()
{
	UpdateData();
	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL | SVNSLC_SHOWLOCKS;
	dwShow |= m_bShowUnversioned ? SVNSLC_SHOWUNVERSIONED : 0;
	dwShow |= m_iShowUnmodified ? SVNSLC_SHOWNORMAL : 0;
	m_FileListCtrl.Show(dwShow);
	m_regAddBeforeCommit = m_bShowUnversioned;
}

void CChangedDlg::OnBnClickedShowUnmodified()
{
	UpdateData();
	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL | SVNSLC_SHOWLOCKS;
	dwShow |= m_bShowUnversioned ? SVNSLC_SHOWUNVERSIONED : 0;
	dwShow |= m_iShowUnmodified ? SVNSLC_SHOWNORMAL : 0;
	m_FileListCtrl.Show(dwShow);
	m_regAddBeforeCommit = m_bShowUnversioned;
}

LRESULT CChangedDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}

BOOL CChangedDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bBlock)
					return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
				if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
				{
					CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

