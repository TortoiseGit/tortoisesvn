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
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ImportDlg.h"
#include "RepositoryBrowser.h"
#include ".\importdlg.h"
#include "DirFileEnum.h"


// CImportDlg dialog

IMPLEMENT_DYNAMIC(CImportDlg, CResizableDialog)
CImportDlg::CImportDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CImportDlg::IDD, pParent)
	, m_bSelectAll(TRUE)
{
	m_message.LoadString(IDS_IMPORT_DEFAULTMSG);
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_url = _T("");
}

CImportDlg::~CImportDlg()
{
}

void CImportDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MESSAGE, m_message);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Control(pDX, IDC_FILELIST, m_FileList);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
}


BEGIN_MESSAGE_MAP(CImportDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_FILELIST, OnLvnItemchangedFilelist)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
END_MESSAGE_MAP()

BOOL CImportDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	LOGFONT LogFont;
	LogFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
	LogFont.lfWidth          = 0;
	LogFont.lfEscapement     = 0;
	LogFont.lfOrientation    = 0;
	LogFont.lfWeight         = 400;
	LogFont.lfItalic         = 0;
	LogFont.lfUnderline      = 0;
	LogFont.lfStrikeOut      = 0;
	LogFont.lfCharSet        = DEFAULT_CHARSET;
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy(LogFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	m_logFont.CreateFontIndirect(&LogFont);
	GetDlgItem(IDC_MESSAGE)->SetFont(&m_logFont);

	if (m_url.IsEmpty())
		m_URLCombo.LoadHistory(_T("repoURLS"), _T("url"));
	else
	{
		m_URLCombo.SetWindowText(m_url);
		m_URLCombo.EnableWindow(FALSE);
	}

	//set the listcontrol to support checkboxes
	m_FileList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	m_FileList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_FileList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_FileList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOGPROMPT_FILE);
	m_FileList.InsertColumn(0, temp);

	m_FileList.SetRedraw(false);
	CDirFileEnum filefinder(m_path);
	CString filename;
	int itemCount = 0;
	while (filefinder.NextFile(filename))
	{
		if (CCheckTempFiles::IsTemp(filename))
		{
			m_FileList.InsertItem(itemCount, filename);
			m_FileList.SetCheck(itemCount, TRUE);
			itemCount++;
		}
	}

	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_FileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_FileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_FileList.SetRedraw(true);


	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_MODULENAMECHECK, IDS_IMPORT_TT_MODULENAMECHECK);
	m_tooltips.AddTool(IDC_FILELIST, IDS_IMPORT_TT_TEMPFILES);
	//m_tooltips.SetEffectBk(CBalloon::BALLOON_EFFECT_HGRADIENT);
	//m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);

	AddAnchor(IDC_STATIC1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC4, TOP_LEFT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_STATIC2, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC3, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CImportDlg::OnPaint() 
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
HCURSOR CImportDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CImportDlg::OnOK()
{
	if (m_URLCombo.IsWindowEnabled())
	{
		m_URLCombo.SaveHistory();
		m_url = m_URLCombo.GetString();
		UpdateData();
	}

	// first we check the size of all filepaths together
	DWORD len = 0;
	for (int j=0; j<m_FileList.GetItemCount(); j++)
	{
		if (m_FileList.GetCheck(j))
		{
			len += m_FileList.GetItemText(j,0).GetLength() + sizeof(TCHAR);
		}
	}
	TCHAR * filenames = new TCHAR[len+(4*sizeof(TCHAR))];
	ZeroMemory(filenames, len+(4*sizeof(TCHAR)));
	TCHAR * fileptr = filenames;
	for (int i=(m_FileList.GetItemCount()-1); i>=0; i--)
	{
		if (m_FileList.GetCheck(i))
		{
			CString temp = m_FileList.GetItemText(i,0);
			_tcscpy(fileptr, m_FileList.GetItemText(i,0));
			fileptr = _tcsninc(fileptr, _tcslen(fileptr)+1);
		} // if (m_FileList.GetCheck(i))
	} // for (int i=0; i<m_FileList.GetItemCount(); i++)
	*fileptr = '\0';
	if (_tcslen(filenames)!=0)
	{
		SHFILEOPSTRUCT fileop;
		fileop.hwnd = this->m_hWnd;
		fileop.wFunc = FO_DELETE;
		fileop.pFrom = filenames;
		fileop.pTo = _T("");
		fileop.fFlags = FOF_ALLOWUNDO | FOF_NO_CONNECTED_ELEMENTS;
		fileop.lpszProgressTitle = _T("deleting files");
		SHFileOperation(&fileop);
		if (fileop.fAnyOperationsAborted)
		{
			delete [] filenames;
			CResizableDialog::OnCancel();
			return;
		}
	} // if (_tcslen(filenames)!=0) 

	delete [] filenames;
	CResizableDialog::OnOK();
}

void CImportDlg::OnBnClickedBrowse()
{
	CString strUrl;
	m_URLCombo.GetWindowText(strUrl);
	if (strUrl.Left(7) == _T("file://"))
	{
		CString strFile(strUrl);
		SVN::UrlToPath(strFile);

		SVN svn;
		if (svn.IsRepository(strFile))
		{
			// browse repository - show repository browser
			CRepositoryBrowser browser(strUrl, this);
			if (browser.DoModal() == IDOK)
			{
				m_URLCombo.SetWindowText(browser.m_strUrl);
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (folderBrowser.Show(GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
			{
				SVN::PathToUrl(strUrl);

				m_URLCombo.SetWindowText(strUrl);
			}
		}
	}
	else if ((strUrl.Left(7) == _T("http://")
		||(strUrl.Left(8) == _T("https://"))
		||(strUrl.Left(6) == _T("svn://"))
		||(strUrl.Left(10) == _T("svn+ssl://"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, this);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetWindowText(browser.m_strUrl);
		}
	}
	else
	{
		// browse local directories
		CBrowseFolder folderBrowser;
		folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		if (folderBrowser.Show(GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
		{
			SVN::PathToUrl(strUrl);

			m_URLCombo.SetWindowText(strUrl);
		}
	}
}

BOOL CImportDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CImportDlg::OnLvnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	int index = pNMLV->iItem;
	if (m_FileList.GetCheck(index))
	{
		if (PathIsDirectory(m_FileList.GetItemText(index, 0)))
		{
			//enable all files within that folder
			CString folderpath = m_FileList.GetItemText(index, 0);
			for (int i=0; i<m_FileList.GetItemCount(); i++)
			{
				if (folderpath.CompareNoCase(m_FileList.GetItemText(i, 0).Left(folderpath.GetLength()))==0)
				{
					m_FileList.SetCheck(i, TRUE);
				}
			} // for (int i=0; i<m_FileList.GetItemCount(); i++) 
		} // if (PathIsDirectory(m_FileList.GetItemText(index, 0))) 
	} // if (!m_FileList.GetCheck(index)) 
	else
	{
		if (!PathIsDirectory(m_FileList.GetItemText(index, 0)))
		{
			//user selected a file, so we need to check if the parent folder is checked
			CString folderpath = m_FileList.GetItemText(index, 0);
			folderpath = folderpath.Left(folderpath.ReverseFind('\\'));
			for (int i=0; i<m_FileList.GetItemCount(); i++)
			{
				if (folderpath.CompareNoCase(m_FileList.GetItemText(i, 0))==0)
				{
					if (m_FileList.GetCheck(i))
						m_FileList.SetCheck(index, TRUE);
					return;
				}
			} // for (int i=0; i<m_FileList.GetItemCount(); i++) 
		}
	}
}

void CImportDlg::OnBnClickedSelectall()
{
	UpdateData();
	theApp.DoWaitCursor(1);
	for (int i=0; i<m_FileList.GetItemCount(); i++)
	{
		m_FileList.SetCheck(i, m_bSelectAll);
	}
	theApp.DoWaitCursor(-1);
}
