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
#include "DirFileEnum.h"
#include "AddDlg.h"
#include "SVNConfig.h"
#include ".\adddlg.h"


// CAddDlg dialog

IMPLEMENT_DYNAMIC(CAddDlg, CResizableDialog)
CAddDlg::CAddDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CAddDlg::IDD, pParent)
	, m_bSelectAll(TRUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CAddDlg::~CAddDlg()
{
}

void CAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ADDLIST, m_addListCtrl);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
}


BEGIN_MESSAGE_MAP(CAddDlg, CResizableDialog)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ADDLIST, OnLvnItemchangedAddlist)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


// CAddDlg message handlers
void CAddDlg::OnPaint() 
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
HCURSOR CAddDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CAddDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//set the listcontrol to support checkboxes
	m_addListCtrl.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_addListCtrl.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_addListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_addListCtrl.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOGPROMPT_FILE);
	m_addListCtrl.InsertColumn(0, temp);

	m_addListCtrl.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_addListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_addListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &AddThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bThreadRunning = TRUE;
	m_addListCtrl.UpdateData(FALSE);

	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(_T("AddDlg"));
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAddDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	//save only the files the user has selected into the temporary file
	try
	{
		CStdioFile file(m_sPath, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
		for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
		{
			if (m_addListCtrl.GetCheck(i))
			{
				file.WriteString(m_arFileList.GetAt(i)+_T("\n"));
			}
		} // for (int i=0; i<m_addListCtrl.GetItemCount(); i++) 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Add!\n");
		pE->Delete();
	}

	CResizableDialog::OnOK();
}

void CAddDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;

	CResizableDialog::OnCancel();
}

void CAddDlg::OnBnClickedSelectall()
{
	UpdateData();
	theApp.DoWaitCursor(1);
	m_addListCtrl.SetRedraw(false);
	int itemCount = m_addListCtrl.GetItemCount();
	for (int i=0; i<itemCount; i++)
	{
		m_addListCtrl.SetCheck(i, m_bSelectAll);
	}
	m_addListCtrl.SetRedraw(true);
	theApp.DoWaitCursor(-1);
}

void CAddDlg::OnLvnItemchangedAddlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	int index = pNMLV->iItem;
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
		return;			//thread is still running
	if (!m_addListCtrl.GetCheck(index))
	{
		if (PathIsDirectory(m_arFileList.GetAt(index)))
		{
			//disable all files within that folder
			CString folderpath = m_arFileList.GetAt(index);
			for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
			{
				if (CUtils::PathIsParent(folderpath, m_arFileList.GetAt(i)))
				{
					m_addListCtrl.SetCheck(i, FALSE);
				}
			} // for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
		} // if (PathIsDirectory(m_arFileList.GetAt(index)))
	} // if (!m_addListCtrl.GetCheck(index))
	else
	{
		//we need to check the parent folder too
		CString folderpath = m_arFileList.GetAt(index);
		for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
		{
			if (CUtils::PathIsParent(m_arFileList.GetAt(i), folderpath))
			{
				m_addListCtrl.SetCheck(i, TRUE);
			} // if (folderpath.CompareNoCase(m_arFileList.GetAt(i))==0) 
		} // for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
	} 
}

DWORD WINAPI AddThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CAddDlg*	pDlg;
	pDlg = (CAddDlg*)pVoid;
	pDlg->GetDlgItem(IDOK)->EnableWindow(false);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(false);

	pDlg->m_addListCtrl.SetRedraw(false);
	SVNConfig config;
	try
	{
		CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
		CString strLine = _T("");
		const TCHAR * strbuf = NULL;
		while (file.ReadString(strLine))
		{
			SVNStatus status;
			svn_wc_status_t *s;

			// Determine whether the item the user selected (strLine) is a directory.

			BOOL bIsDir = PathIsDirectory(strLine);

			// The directory that contains this item.
			// Even if strLine is a directory, we will hold its root. 
			// Thus, we can distinguish sub-items with the same relative 
			// path in different selected directories.

			CString root = strLine.Left(strLine.ReverseFind('\\') + 1);

			// Introduce this flag to allow ignored items to be added deliberately.
			// It will be reset once the first entry found for strLine is added.

			bool firstRun = true;

			// Ask SVN for the first item found for strLine.

			s = status.GetFirstFileStatus(strLine, &strbuf);
			while (s!=0)
			{
				// the current item (path)

				CString item = strbuf;

				// Get the "combined" status of item and its properties

				svn_wc_status_kind stat = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);

				// Is the status not "below normal", i.e. non-versioned?

				bool nonVersioned = SVNStatus::GetMoreImportant(svn_wc_status_normal, stat)!=stat;

				// Shall the item be ignored?

				bool ignore = (stat==svn_wc_status_ignored) && !firstRun;

				// This one fixes a problem with externals: 
				// If a strLine is a file, svn:externals at its parent directory
				// will also be returned by GetXXXFileStatus. Hence, make sure the 
				// item's parent is the strLine's parent.

				bool actuallySelected = bIsDir || (item.ReverseFind('/')+1 == root.GetLength());

				// Add to the selection list, if all 3 conditions are met:

				firstRun = false;

				if (nonVersioned && !ignore && actuallySelected)
				{

					pDlg->m_arFileList.Add(item);
					int count = pDlg->m_addListCtrl.GetItemCount();
					pDlg->m_addListCtrl.InsertItem(count, item.Mid(root.GetLength()));
					pDlg->m_addListCtrl.SetCheck(count);
					if (PathIsDirectory(item))
					{
						//we have an unversioned folder -> get all files in it recursively!
						int count = pDlg->m_addListCtrl.GetItemCount();
						CDirFileEnum filefinder(item);
						CString filename;
						while (filefinder.NextFile(filename))
						{
							if (!config.MatchIgnorePattern(filename))
							{
								filename.Replace('\\', '/');
								pDlg->m_arFileList.Add(filename);
								pDlg->m_addListCtrl.InsertItem(count, filename.Mid(root.GetLength()));
								pDlg->m_addListCtrl.SetCheck(count++);
							}
						} // while (filefinder.NextFile(filename))
					} // if (bIsDir) 
				} // if (nonVersioned && !ignore && actuallySelected) 
			
				// Ask SVN for the next item for strLine. 
				// This will usually be the case for directories and svn:externals.

				s = status.GetNextFileStatus(&strbuf);
			} // if (s!=0) 
		} // while (file.ReadString(strLine)) 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Commit!\n");
		pE->Delete();
	}


	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(pDlg->m_addListCtrl.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		pDlg->m_addListCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	pDlg->m_addListCtrl.SetRedraw(true);

	pDlg->GetDlgItem(IDOK)->EnableWindow(true);
	pDlg->GetDlgItem(IDCANCEL)->EnableWindow(true);
	pDlg->m_bThreadRunning = FALSE;
	return 0;
}

void CAddDlg::OnBnClickedHelp()
{
	OnHelp();
}






