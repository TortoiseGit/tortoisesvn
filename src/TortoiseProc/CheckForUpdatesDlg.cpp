// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "..\version.h"
#include "MessageBox.h"
#include ".\checkforupdatesdlg.h"


// CCheckForUpdatesDlg dialog

IMPLEMENT_DYNAMIC(CCheckForUpdatesDlg, CDialog)
CCheckForUpdatesDlg::CCheckForUpdatesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCheckForUpdatesDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bShowInfo = FALSE;
	m_bVisible = FALSE;
}

CCheckForUpdatesDlg::~CCheckForUpdatesDlg()
{
}

void CCheckForUpdatesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCheckForUpdatesDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_STN_CLICKED(IDC_CHECKRESULT, OnStnClickedCheckresult)
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()


// CCheckForUpdatesDlg message handlers
void CCheckForUpdatesDlg::OnPaint() 
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
	} // if (IsIconic()) 
	else
	{
		CDialog::OnPaint();
	}
}
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCheckForUpdatesDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CCheckForUpdatesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString temp;
	temp.Format(IDS_CHECKNEWER_YOURVERSION, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
	GetDlgItem(IDC_YOURVERSION)->SetWindowText(temp);

	GetDlgItem(IDOK)->EnableWindow(FALSE);

	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &CheckThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	SetTimer(100, 1000, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCheckForUpdatesDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	CDialog::OnOK();
}

void CCheckForUpdatesDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;
	CDialog::OnCancel();
}

DWORD WINAPI CheckThread(LPVOID pVoid)
{
	CCheckForUpdatesDlg* pDlg;
	pDlg = (CCheckForUpdatesDlg*)pVoid;
	pDlg->m_bThreadRunning = TRUE;

	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));

	CString temp;
	CString tempfile = CUtils::GetTempFile();

	HRESULT res = URLDownloadToFile(NULL, _T("http://tortoisesvn.tigris.org/version.txt"), tempfile, 0, NULL);
	if (res == S_OK)
	{
		try
		{
			CStdioFile file(tempfile, CFile::modeRead);
			CString ver;
			if (file.ReadString(ver))
			{
				CString vertemp = ver;
				int major = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				int minor = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				int micro = _ttoi(vertemp);
				vertemp = vertemp.Mid(vertemp.Find('.')+1);
				int build = _ttoi(vertemp);
				BOOL bNewer = FALSE;
				if (major > TSVN_VERMAJOR)
					bNewer = TRUE;
				else if (minor > TSVN_VERMINOR)
					bNewer = TRUE;
				else if (micro > TSVN_VERMICRO)
					bNewer = TRUE;
				else if (build > TSVN_VERBUILD)
					bNewer = TRUE;

				if (_ttoi(ver)!=0)
				{
					temp.Format(IDS_CHECKNEWER_CURRENTVERSION, ver);
					pDlg->GetDlgItem(IDC_CURRENTVERSION)->SetWindowText(temp);
					temp.Format(_T("%d.%d.%d.%d"), TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
				}
				if (_ttoi(ver)==0)
				{
					temp.LoadString(IDS_CHECKNEWER_NETERROR);
					pDlg->GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
				}
				else if (bNewer)
				{
					temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
					pDlg->GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
					pDlg->m_bShowInfo = TRUE;
				}
				else
				{
					temp.LoadString(IDS_CHECKNEWER_YOURUPTODATE);
					pDlg->GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
				}
			}
		}
		catch (CException * e)
		{
			e->Delete();
			temp.LoadString(IDS_CHECKNEWER_NETERROR);
			pDlg->GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
		}
	}
	else
	{
		temp.LoadString(IDS_CHECKNEWER_YOURUPTODATE);
		pDlg->GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
	}
	DeleteFile(tempfile);
	pDlg->m_bThreadRunning = FALSE;
	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	return 0;
}

void CCheckForUpdatesDlg::OnStnClickedCheckresult()
{
	//user clicked on the label, start the browser with our webpage
	HINSTANCE result = ShellExecute(NULL, _T("opennew"), _T("http://tortoisesvn.tigris.org"), NULL,NULL, SW_SHOWNORMAL);
	if ((UINT)result <= HINSTANCE_ERROR)
	{
		result = ShellExecute(NULL, _T("open"), _T("http://tortoisesvn.tigris.org"), NULL,NULL, SW_SHOWNORMAL);
	}
}

void CCheckForUpdatesDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 100)
	{
		if (m_bThreadRunning == FALSE)
		{
			if (m_bShowInfo)
			{
				m_bVisible = TRUE;
				ShowWindow(SW_SHOWNORMAL);
			}
			else
			{
				EndDialog(0);
			}
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CCheckForUpdatesDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CDialog::OnWindowPosChanging(lpwndpos);
	if (m_bVisible == FALSE)
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
}
