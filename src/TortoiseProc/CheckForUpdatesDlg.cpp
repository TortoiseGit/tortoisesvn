// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "..\version.h"
#include "MessageBox.h"
#include ".\checkforupdatesdlg.h"
#include "Registry.h"
#include "Utils.h"
#include "TempFile.h"


// CCheckForUpdatesDlg dialog

IMPLEMENT_DYNAMIC(CCheckForUpdatesDlg, CStandAloneDialog)
CCheckForUpdatesDlg::CCheckForUpdatesDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CCheckForUpdatesDlg::IDD, pParent)
{
	m_bShowInfo = FALSE;
	m_bVisible = FALSE;
}

CCheckForUpdatesDlg::~CCheckForUpdatesDlg()
{
}

void CCheckForUpdatesDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCheckForUpdatesDlg, CStandAloneDialog)
	ON_STN_CLICKED(IDC_CHECKRESULT, OnStnClickedCheckresult)
	ON_WM_TIMER()
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()

BOOL CCheckForUpdatesDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	CString temp;
	temp.Format(IDS_CHECKNEWER_YOURVERSION, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
	GetDlgItem(IDC_YOURVERSION)->SetWindowText(temp);

	GetDlgItem(IDOK)->EnableWindow(FALSE);

	if (AfxBeginThread(CheckThreadEntry, this)==NULL)
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
	CStandAloneDialog::OnOK();
}

void CCheckForUpdatesDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;
	CStandAloneDialog::OnCancel();
}

UINT CCheckForUpdatesDlg::CheckThreadEntry(LPVOID pVoid)
{
	return ((CCheckForUpdatesDlg*)pVoid)->CheckThread();
}

UINT CCheckForUpdatesDlg::CheckThread()
{
	m_bThreadRunning = TRUE;

	CString temp;
	CString tempfile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
	
	CRegString checkurluser = CRegString(_T("Software\\TortoiseSVN\\UpdateCheckURL"), _T(""));
	CRegString checkurlmachine = CRegString(_T("Software\\TortoiseSVN\\UpdateCheckURL"), _T(""), FALSE, HKEY_LOCAL_MACHINE);
	CString sCheckURL = checkurluser;
	if (sCheckURL.IsEmpty())
	{
		sCheckURL = checkurlmachine;
		if (sCheckURL.IsEmpty())
			sCheckURL = _T("http://tortoisesvn.tigris.org/version.txt");
	}
	HRESULT res = URLDownloadToFile(NULL, sCheckURL, tempfile, 0, NULL);
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
				else if ((minor > TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
					bNewer = TRUE;
				else if ((micro > TSVN_VERMICRO)&&(minor == TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
					bNewer = TRUE;
				else if ((build > TSVN_VERBUILD)&&(micro == TSVN_VERMICRO)&&(minor == TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
					bNewer = TRUE;

				if (_ttoi(ver)!=0)
				{
					temp.Format(IDS_CHECKNEWER_CURRENTVERSION, (LPCTSTR)ver);
					GetDlgItem(IDC_CURRENTVERSION)->SetWindowText(temp);
					temp.Format(_T("%d.%d.%d.%d"), TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
				}
				if (_ttoi(ver)==0)
				{
					temp.LoadString(IDS_CHECKNEWER_NETERROR);
					GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
				}
				else if (bNewer)
				{
					temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
					GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
					m_bShowInfo = TRUE;
				}
				else
				{
					temp.LoadString(IDS_CHECKNEWER_YOURUPTODATE);
					GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
				}
			}
		}
		catch (CException * e)
		{
			e->Delete();
			temp.LoadString(IDS_CHECKNEWER_NETERROR);
			GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
		}
	}
	else
	{
		temp.LoadString(IDS_CHECKNEWER_NETERROR);
		GetDlgItem(IDC_CHECKRESULT)->SetWindowText(temp);
	}
	DeleteFile(tempfile);
	m_bThreadRunning = FALSE;
	GetDlgItem(IDOK)->EnableWindow(TRUE);
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

void CCheckForUpdatesDlg::OnTimer(UINT_PTR nIDEvent)
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
	CStandAloneDialog::OnTimer(nIDEvent);
}

void CCheckForUpdatesDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CStandAloneDialog::OnWindowPosChanging(lpwndpos);
	if (m_bVisible == FALSE)
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
}
