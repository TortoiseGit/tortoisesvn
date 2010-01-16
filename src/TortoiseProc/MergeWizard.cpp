// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MergeWizard.h"
#include "SVN.h"
#include "Registry.h"

#define BOTTOMMARG 48

IMPLEMENT_DYNAMIC(CMergeWizard, CResizableSheetEx)

CMergeWizard::CMergeWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CResizableSheetEx(nIDCaption, pParentWnd, iSelectPage)
	, bReverseMerge(FALSE)
	, nRevRangeMerge(MERGEWIZARD_REVRANGE)
	, m_bIgnoreAncestry(FALSE)
	, m_bIgnoreEOL(FALSE)
	, m_depth(svn_depth_unknown)
	, m_bRecordOnly(FALSE)
	, m_FirstPageActivation(true)
{
	SetWizardMode();
	AddPage(&page1);
	AddPage(&tree);
	AddPage(&revrange);
	AddPage(&options);
	AddPage(&reintegrate);

	m_psh.dwFlags |= PSH_WIZARD97|PSP_HASHELP|PSH_HEADER;
	m_psh.pszbmHeader = MAKEINTRESOURCE(IDB_MERGEWIZARDTITLE);

	m_psh.hInstance = AfxGetResourceHandle();
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


CMergeWizard::~CMergeWizard()
{
}


BEGIN_MESSAGE_MAP(CMergeWizard, CResizableSheetEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CMergeWizard message handlers

BOOL CMergeWizard::OnInitDialog()
{
	BOOL bResult = CResizableSheetEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SVN svn;
	url = svn.GetURLFromPath(wcPath);
	sUUID = svn.GetUUIDFromPath(wcPath);

	MARGINS margs;
	margs.cxLeftWidth = 0;
	margs.cyTopHeight = 0;
	margs.cxRightWidth = 0;
	margs.cyBottomHeight = BOTTOMMARG;

	if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\EnableDWMFrame"), TRUE))
	{
		m_Dwm.Initialize();
		if (m_Dwm.IsDwmCompositionEnabled())
		{
			m_Dwm.DwmExtendFrameIntoClientArea(m_hWnd, &margs);
			ShowGrip(false);
		}
		m_aeroControls.SubclassControl(GetDlgItem(IDCANCEL)->GetSafeHwnd());
		m_aeroControls.SubclassControl(GetDlgItem(IDOK)->GetSafeHwnd());
		m_aeroControls.SubclassControl(GetDlgItem(IDHELP)->GetSafeHwnd());
		m_aeroControls.SubclassControl(GetDlgItem(ID_WIZFINISH)->GetSafeHwnd());
		m_aeroControls.SubclassControl(GetDlgItem(ID_WIZBACK)->GetSafeHwnd());
		m_aeroControls.SubclassControl(GetDlgItem(ID_WIZNEXT)->GetSafeHwnd());
	}

	return bResult;
}


void CMergeWizard::SaveMode()
{
	CRegDWORD regMergeWizardMode(_T("Software\\TortoiseSVN\\MergeWizardMode"), 0);
	if (DWORD(regMergeWizardMode))
	{
		switch (nRevRangeMerge)
		{
		case IDD_MERGEWIZARD_REVRANGE:
			regMergeWizardMode = 2;
			break;
		case IDD_MERGEWIZARD_REINTEGRATE:
			regMergeWizardMode = 3;
			break;
		case IDD_MERGEWIZARD_TREE:
			regMergeWizardMode = 1;
			break;
		default:
			regMergeWizardMode = 0;
			break;
		}
	}
}

LRESULT CMergeWizard::GetSecondPage()
{
	switch (nRevRangeMerge)
	{
	case MERGEWIZARD_REVRANGE:
		return IDD_MERGEWIZARD_REVRANGE;
	case MERGEWIZARD_TREE:
		return IDD_MERGEWIZARD_TREE;
	case MERGEWIZARD_REINTEGRATE:
		return IDD_MERGEWIZARD_REINTEGRATE;
	}
	return IDD_MERGEWIZARD_REVRANGE;
}

void CMergeWizard::OnPaint()
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
		CResizableSheetEx::OnPaint();
	}
}

HCURSOR CMergeWizard::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CMergeWizard::OnEraseBkgnd(CDC* pDC)
{
	CResizableSheetEx::OnEraseBkgnd(pDC);

	if (m_Dwm.IsDwmCompositionEnabled())
	{
		// draw the frame margins in black
		RECT rc;
		GetClientRect(&rc);
		pDC->FillSolidRect(rc.left, rc.bottom-BOTTOMMARG, rc.right-rc.left, BOTTOMMARG, RGB(0,0,0));
	}
	return TRUE;
}
