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
#include "messagebox.h"
#include "Revertdlg.h"
#include "SVN.h"


// CRevertDlg dialog

IMPLEMENT_DYNAMIC(CRevertDlg, CResizableDialog)
CRevertDlg::CRevertDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CRevertDlg::IDD, pParent)
	, m_bSelectAll(TRUE)
	, m_bThreadRunning(FALSE)
	
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CRevertDlg::~CRevertDlg()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
}

void CRevertDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REVERTLIST, m_RevertList);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CRevertDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CRevertDlg message handlers
void CRevertDlg::OnPaint() 
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
		CResizableDialog::OnPaint();
	}
}
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRevertDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CRevertDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_RevertList.Init(SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS);
	m_RevertList.SetSelectButton(&m_SelectAll);
	
	AddAnchor(IDC_REVERTLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RevertDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((CreateThread(NULL, 0, &RevertThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bThreadRunning = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

DWORD WINAPI RevertThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CRevertDlg*	pDlg;
	pDlg = (CRevertDlg*)pVoid;
	pDlg->GetDlgItem(IDOK)->EnableWindow(false);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(false);

	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));

	pDlg->m_RevertList.GetStatus(pDlg->m_sPath);
	pDlg->m_RevertList.Show(SVNSLC_SHOWVERSIONEDBUTNORMAL | SVNSLC_SHOWDIRECTS, SVNSLC_SHOWVERSIONEDBUTNORMAL | SVNSLC_SHOWDIRECTS);

	pDlg->GetDlgItem(IDOK)->EnableWindow(true);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);

	if (pDlg->m_RevertList.GetItemCount()==0)
	{
		CMessageBox::Show(pDlg->m_hWnd, IDS_LOGPROMPT_NOTHINGTOCOMMIT, IDS_APPNAME, MB_ICONINFORMATION);
		pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
		pDlg->m_bThreadRunning = FALSE;
		pDlg->EndDialog(0);
		return (DWORD)-1;
	}

	pDlg->m_bThreadRunning = FALSE;
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	return 0;
}

void CRevertDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	//save only the files the user has selected into the temporary file
	m_bRecursive = TRUE;
	for (int i=0; i<m_RevertList.GetItemCount(); ++i)
	{
		if (!m_RevertList.GetCheck(i))
		{
			m_bRecursive = FALSE;
			break;
		}
	}
	if (!m_bRecursive)
	{
		try
		{
			CStdioFile file(m_sPath, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
			for (int i=0; i<m_RevertList.GetItemCount(); i++)
			{
				if (m_RevertList.GetCheck(i))
				{
					file.WriteString(m_RevertList.GetListEntry(i)->path+_T("\n"));
				}
			} 
			file.Close();
		}
		catch (CFileException* pE)
		{
			TRACE("CFileException in Add!\n");
			pE->Delete();
		}
	}

	CResizableDialog::OnOK();
}

void CRevertDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;

	CResizableDialog::OnCancel();
}

void CRevertDlg::OnBnClickedHelp()
{
	OnHelp();
}


BOOL CRevertDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bThreadRunning)
	{
		if ((pWnd)&&(pWnd == GetDlgItem(IDC_REVERTLIST)))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CRevertDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == 2)
		return;
	theApp.DoWaitCursor(1);
	m_RevertList.SelectAll(state == 1);
	theApp.DoWaitCursor(-1);
}
