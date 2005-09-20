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
#include "RepositoryBar.h"
#include "RepositoryTree.h"
#include "RevisionDlg.h"
#include ".\repositorybar.h"

#define IDC_URL_COMBO     10000
#define IDC_REVISION_BTN  10001


// CRepositoryBar

IMPLEMENT_DYNAMIC(CRepositoryBar, CReBarCtrl)

#pragma warning(push)
#pragma warning(disable: 4355)	// 'this' used in base member initializer list

CRepositoryBar::CRepositoryBar() :
	m_pRepositoryTree(0),
	m_cbxUrl(this)
{
}
#pragma warning(pop)

CRepositoryBar::~CRepositoryBar()
{
}

BEGIN_MESSAGE_MAP(CRepositoryBar, CReBarCtrl)
	ON_CBN_SELENDOK(IDC_URL_COMBO, OnCbnSelEndOK)
	ON_BN_CLICKED(IDC_REVISION_BTN, OnBnClicked)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CRepositoryBar public interface

bool CRepositoryBar::Create(CWnd* parent, UINT id, bool in_dialog)
{
	CRect rect;
	ASSERT(parent != 0);
	parent->GetWindowRect(&rect);

	DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
				| CCS_TOP | RBS_AUTOSIZE | RBS_VARHEIGHT;

	DWORD style_ex = WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;

	if (in_dialog)
	{
		style |= CCS_NODIVIDER;
		style_ex |= 0; //WS_EX_CONTROLPARENT;
	}
	else
	{
		style |= RBS_BANDBORDERS; 
	}

	if (CReBarCtrl::CreateEx(style_ex, style, CRect(0,0,200,100), parent, id))
	{
		CFont *font = parent->GetFont();
		CString temp;

		REBARINFO rbi;
		ZeroMemory(&rbi, sizeof rbi);
		rbi.cbSize = sizeof rbi;
		rbi.fMask  = 0;
		rbi.himl   = (HIMAGELIST)0;

		if (!this->SetBarInfo(&rbi))
			return false;

		REBARBANDINFO rbbi;
		ZeroMemory(&rbbi, sizeof rbbi);
		rbbi.cbSize = sizeof rbbi;
		rbbi.fMask  = RBBIM_TEXT | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
		rbbi.fStyle = RBBS_NOGRIPPER | RBBS_FIXEDBMP;

		if (in_dialog)
			rbbi.fMask |= RBBIM_COLORS;
		else
			rbbi.fMask |= RBBS_CHILDEDGE;

		// Create the "URL" combo box control to be added
		rect = CRect(0, 0, 100, 200);
		m_cbxUrl.Create(WS_CHILD | WS_TABSTOP | CBS_DROPDOWN, rect, this, IDC_URL_COMBO);
		m_cbxUrl.SetURLHistory(true);
		m_cbxUrl.SetFont(font);
		m_cbxUrl.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));
		temp.LoadString(IDS_REPO_BROWSEURL);
		rbbi.lpText     = const_cast<LPTSTR>((LPCTSTR)temp);
		rbbi.hwndChild  = m_cbxUrl.m_hWnd;
		rbbi.clrFore	= ::GetSysColor(COLOR_WINDOWTEXT);
		rbbi.clrBack	= ::GetSysColor(COLOR_BTNFACE);
		rbbi.cx         = rect.right - rect.left;
		rbbi.cxMinChild = rect.right - rect.left;
		rbbi.cyMinChild = m_cbxUrl.GetItemHeight(-1) + 10;
		if (!InsertBand(0, &rbbi))
			return false;

		// Reposition the combobox for correct redrawing
		m_cbxUrl.GetWindowRect(rect);
		m_cbxUrl.MoveWindow(rect.left, rect.top, rect.Width(), 300);

		// Create the "Revision" button control to be added
		rect = CRect(0, 0, 60, m_cbxUrl.GetItemHeight(-1) + 8);
		m_btnRevision.Create(_T("HEAD"), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, rect, this, IDC_REVISION_BTN);
		m_btnRevision.SetFont(font);
		temp.LoadString(IDS_REPO_BROWSEREV);
		rbbi.lpText     = const_cast<LPTSTR>((LPCTSTR)temp);
		rbbi.hwndChild  = m_btnRevision.m_hWnd;
		rbbi.clrFore	= ::GetSysColor(COLOR_WINDOWTEXT);
		rbbi.clrBack	= ::GetSysColor(COLOR_BTNFACE);
		rbbi.cx         = rect.right - rect.left;
		rbbi.cxMinChild = rect.right - rect.left;
		rbbi.cyMinChild = rect.bottom - rect.top;
		if (!InsertBand(1, &rbbi))
			return false;

		MaximizeBand(0);

		return true;
	}

	return false;
}

void CRepositoryBar::AssocTree(CRepositoryTree *repo_tree)
{
	if (m_pRepositoryTree != 0)
		m_pRepositoryTree->m_pRepositoryBar = 0;

	m_pRepositoryTree = repo_tree;

	if (m_pRepositoryTree != 0)
		m_pRepositoryTree->m_pRepositoryBar = this;
}

void CRepositoryBar::ShowUrl(const SVNUrl& svn_url)
{
	m_SvnUrl = svn_url;
	m_cbxUrl.SetWindowText(svn_url.GetPath());
	m_btnRevision.SetWindowText(svn_url.GetRevisionText());
}

void CRepositoryBar::GotoUrl(const SVNUrl& svn_url)
{
	SVNUrl new_url = svn_url;

	if (new_url.IsEmpty())
		new_url = GetCurrentUrl();

	if (m_pRepositoryTree != 0)
		m_pRepositoryTree->ChangeToUrl(new_url);

	ShowUrl(new_url);
}

void CRepositoryBar::SetRevision(SVNRev rev)
{
	m_btnRevision.SetWindowText(SVNUrl::GetTextFromRev(rev));
	m_SvnUrl = SVNUrl(m_SvnUrl.GetPath(), rev);
}

SVNUrl CRepositoryBar::GetCurrentUrl() const
{
	if (m_cbxUrl.m_hWnd != 0)
	{
		CString path, revision;
		m_cbxUrl.GetWindowText(path);
		m_btnRevision.GetWindowText(revision);
		return SVNUrl(path, revision);
	}
	else
	{
		return m_SvnUrl;
	}
}

void CRepositoryBar::SaveHistory()
{
	m_cbxUrl.SaveHistory();
}

bool CRepositoryBar::CRepositoryCombo::OnReturnKeyPressed()
{
	if (m_bar != 0)
		m_bar->GotoUrl();
	if (GetDroppedState())
		ShowDropDown(FALSE);
	return true;
}


// CRepositoryBar message handlers

void CRepositoryBar::OnCbnSelEndOK()
{
	if (m_cbxUrl.GetDroppedState())
	{
		int idx = m_cbxUrl.GetCurSel();
		if (idx >= 0)
		{
			CString path, revision;
			m_cbxUrl.GetLBText(idx, path);
			m_btnRevision.GetWindowText(revision);
			m_SvnUrl = SVNUrl(path, revision);

			if (m_pRepositoryTree != 0)
				m_pRepositoryTree->ChangeToUrl(m_SvnUrl);
		}
	}
}

void CRepositoryBar::OnBnClicked()
{
	CString revision;
	m_btnRevision.GetWindowText(revision);

	CRevisionDlg dlg(this);
	*((SVNRev*)&dlg) = SVNRev(revision);

	if (dlg.DoModal() == IDOK)
	{
		revision = dlg.GetEnteredRevisionString();
		m_btnRevision.SetWindowText(SVNUrl::GetTextFromRev(SVNRev(revision)));
		GotoUrl();
	}
}



// CRepositoryBarCnr control

CRepositoryBarCnr::CRepositoryBarCnr(CRepositoryBar *repository_bar) :
	m_pbarRepository(repository_bar)
{
}

CRepositoryBarCnr::~CRepositoryBarCnr()
{
}

BEGIN_MESSAGE_MAP(CRepositoryBarCnr, CStatic)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CRepositoryBarCnr, CStatic)

BOOL CRepositoryBarCnr::OnEraseBkgnd(CDC* /* pDC */)
{
	return TRUE;
}

void CRepositoryBarCnr::OnSize(UINT /* nType */, int cx, int cy)
{
	m_pbarRepository->MoveWindow(0, 0, cx, cy);
}

void CRepositoryBarCnr::DrawItem(LPDRAWITEMSTRUCT)
{
}

UINT CRepositoryBarCnr::OnGetDlgCode()
{
	return CStatic::OnGetDlgCode() | DLGC_WANTTAB;
}

void CRepositoryBarCnr::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_TAB)
	{
		CWnd *child = m_pbarRepository->GetWindow(GW_CHILD);
		if (child != 0)
		{
			child = child->GetWindow(GW_HWNDLAST);
			if (child != 0)
				child->SetFocus();
		}
	}

	CStatic::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CRepositoryBar::OnDestroy()
{
	int idx = m_cbxUrl.GetCurSel();
	if (idx >= 0)
	{
		CString path, revision;
		m_cbxUrl.GetLBText(idx, path);
		m_btnRevision.GetWindowText(revision);
		m_SvnUrl = SVNUrl(path, revision);
	}
	CReBarCtrl::OnDestroy();
}

