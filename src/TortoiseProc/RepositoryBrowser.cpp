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
#include "RepositoryBrowser.h"


// CRepositoryBrowser dialog

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CDialog)
CRepositoryBrowser::CRepositoryBrowser(const CString& strUrl, CWnd* pParent /*=NULL*/)
	: CDialog(CRepositoryBrowser::IDD, pParent),
	m_treeRepository(strUrl)
	, m_strUrl(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CRepositoryBrowser::~CRepositoryBrowser()
{
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REPOS_TREE, m_treeRepository);
	DDX_Text(pDX, IDC_URL, m_strUrl);
}


BEGIN_MESSAGE_MAP(CRepositoryBrowser, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_REPOS_TREE, OnTvnSelchangedReposTree)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

BEGIN_RESIZER_MAP(CRepositoryBrowser)
    RESIZER(IDC_REPOS_TREE,RS_BORDER,RS_BORDER,RS_BORDER,IDC_URL,0)
	RESIZER(IDC_STATICURL, RS_BORDER, RS_KEEPSIZE, RS_KEEPSIZE, IDOK, 0)
	RESIZER(IDC_URL, IDC_STATICURL, RS_KEEPSIZE, RS_BORDER, IDOK, 0)
	RESIZER(IDCANCEL, RS_KEEPSIZE, RS_KEEPSIZE, RS_BORDER, RS_BORDER, 0)
	RESIZER(IDOK, RS_KEEPSIZE, RS_KEEPSIZE, IDCANCEL, RS_BORDER, 0)
END_RESIZER_MAP


// CRepositoryBrowser message handlers

void CRepositoryBrowser::OnPaint() 
{
	RESIZER_GRIP;
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
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRepositoryBrowser::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CRepositoryBrowser::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_treeRepository.Init();
	m_treeRepository.SelectItem(m_treeRepository.GetRootItem());

	INIT_RESIZER;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRepositoryBrowser::OnTvnSelchangedReposTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	m_strUrl = m_treeRepository.MakeUrl(pNMTreeView->itemNew.hItem);
	UpdateData(FALSE);
	*pResult = 0;
}

void CRepositoryBrowser::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	UPDATE_RESIZER;
}
