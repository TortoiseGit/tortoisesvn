// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "CheckTempFiles.h"
#include "DirFileList.h"
#include "AddDlg.h"
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
	}
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
	m_addListCtrl.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

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
	m_addListCtrl.UpdateData(FALSE);

	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_LEFT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAddDlg::OnOK()
{
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
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Add!\n");
		pE->Delete();
	}

	CResizableDialog::OnOK();
}

void CAddDlg::OnBnClickedSelectall()
{
	UpdateData();
	theApp.DoWaitCursor(1);
	for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
	{
		m_addListCtrl.SetCheck(i, m_bSelectAll);
	}
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
				if (folderpath.CompareNoCase(m_arFileList.GetAt(i).Left(folderpath.GetLength()))==0)
				{
					m_addListCtrl.SetCheck(i, FALSE);
				}
			} // for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
		} // if (PathIsDirectory(m_arFileList.GetAt(index)))
	} // if (!m_addListCtrl.GetCheck(index))
	else
	{
		if (!PathIsDirectory(m_arFileList.GetAt(index)))
		{
			//user selected a file, so we need to check the parent folder too
			CString folderpath = m_arFileList.GetAt(index);
			folderpath = folderpath.Left(folderpath.ReverseFind('\\'));
			for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
			{
				if (folderpath.CompareNoCase(m_arFileList.GetAt(i))==0)
				{
					m_addListCtrl.SetCheck(i, TRUE);
					return;
				}
			} // for (int i=0; i<m_addListCtrl.GetItemCount(); i++)
		}
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

	try
	{
		CStdioFile file(pDlg->m_sPath, CFile::typeBinary | CFile::modeRead);
		CString strLine = _T("");
		const TCHAR * strbuf = NULL;;
		while (file.ReadString(strLine))
		{
			SVNStatus status;
			svn_wc_status_t *s;
			BOOL bIsDir = PathIsDirectory(strLine);
			s = status.GetFirstFileStatus(strLine, &strbuf);
			if (s!=0)
			{
				CString temp = strbuf;
				svn_wc_status_kind stat;
				stat = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
				if (!SVNStatus::IsImportant(stat))
				{
					if ((!CCheckTempFiles::IsTemp(strLine))||(!bIsDir))
					{
						//we add 'temporary' files always if they are not recursively added from a directory
						pDlg->m_arFileList.Add(strLine);
						int count = pDlg->m_addListCtrl.GetItemCount();
						pDlg->m_addListCtrl.InsertItem(count, strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1));
						pDlg->m_addListCtrl.SetCheck(count);
					} // if ((!CCheckTempFiles::IsTemp(strLine))||(!bIsDir))
					if (bIsDir)
					{
						//we have an unversioned folder -> get all files in it recursively!
						CDirFileList	filelist;
						filelist.BuildList(strLine, TRUE, TRUE);
						int count = pDlg->m_addListCtrl.GetItemCount();
						for (int i=0; i<filelist.GetSize(); i++)
						{
							CString filename = filelist.GetAt(i);
							if (!CCheckTempFiles::IsTemp(filename))
							{
								pDlg->m_arFileList.Add(filename);
								pDlg->m_addListCtrl.InsertItem(count, filename.Right(filename.GetLength() - strLine.ReverseFind('\\') - 1));
								pDlg->m_addListCtrl.SetCheck(count++);
							}
						}
					}
				}
				while ((s = status.GetNextFileStatus(&strbuf)) != NULL)
				{
					temp = strbuf;
					stat = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
					if (!SVNStatus::IsImportant(stat))
					{
						if ((!CCheckTempFiles::IsTemp(temp))||(!bIsDir))
						{
							pDlg->m_arFileList.Add(temp);
							int count = pDlg->m_addListCtrl.GetItemCount();
							pDlg->m_addListCtrl.InsertItem(count, temp.Right(temp.GetLength() - strLine.GetLength() - 1));
							pDlg->m_addListCtrl.SetCheck(count);
						}
						if (bIsDir)
						{
							//we have an unversioned folder -> get all files in it recursively!
							CDirFileList	filelist;
							filelist.BuildList(temp, TRUE, TRUE);
							int count = pDlg->m_addListCtrl.GetItemCount();
							for (int i=0; i<filelist.GetSize(); i++)
							{
								CString filename = filelist.GetAt(i);
								if (!CCheckTempFiles::IsTemp(filename))
								{
									pDlg->m_arFileList.Add(filename);
									pDlg->m_addListCtrl.InsertItem(count, filename.Right(filename.GetLength() - strLine.ReverseFind('\\') - 1));
									pDlg->m_addListCtrl.SetCheck(count++);
								}
							}
						}
					}
				} // while ((s = status.GetNextFileStatus(buf)) != NULL)
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
	return 0;
}




