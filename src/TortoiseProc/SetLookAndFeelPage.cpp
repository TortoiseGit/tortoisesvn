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
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "Globals.h"
#include "ShellUpdater.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include ".\setlookandfeelpage.h"
#include "MessageBox.h"

IMPLEMENT_DYNAMIC(CSetLookAndFeelPage, CPropertyPage)
CSetLookAndFeelPage::CSetLookAndFeelPage()
	: CPropertyPage(CSetLookAndFeelPage::IDD)
	, m_bInitialized(FALSE)
	, m_OwnerDrawn(1)
	, m_bGetLockTop(FALSE)
	, m_bVista(false)
{
	m_regTopmenu = CRegDWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
	m_topmenu = m_regTopmenu;
	m_regOwnerDrawn = CRegDWORD(_T("Software\\TortoiseSVN\\OwnerdrawnMenus"), 1);
	m_OwnerDrawn = m_regOwnerDrawn;
	if (m_OwnerDrawn == 0)
		m_OwnerDrawn = 1;
	else if (m_OwnerDrawn == 1)
		m_OwnerDrawn = 0;
	m_regGetLockTop = CRegDWORD(_T("Software\\TortoiseSVN\\GetLockTop"), TRUE);
	m_bGetLockTop = m_regGetLockTop;
}

CSetLookAndFeelPage::~CSetLookAndFeelPage()
{
}

void CSetLookAndFeelPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
	DDX_Check(pDX, IDC_ENABLEACCELERATORS, m_OwnerDrawn);
	DDX_Check(pDX, IDC_GETLOCKTOP, m_bGetLockTop);
}


BEGIN_MESSAGE_MAP(CSetLookAndFeelPage, CPropertyPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
	ON_BN_CLICKED(IDC_GETLOCKTOP, OnChange)
END_MESSAGE_MAP()


int CSetLookAndFeelPage::SaveData()
{
	if (m_bInitialized)
	{
		m_regTopmenu = m_topmenu;
		if (m_regTopmenu.LastError != ERROR_SUCCESS)
			CMessageBox::Show(m_hWnd, m_regTopmenu.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
		m_regGetLockTop = m_bGetLockTop;
		if (m_regGetLockTop.LastError != ERROR_SUCCESS)
			CMessageBox::Show(m_hWnd, m_regGetLockTop.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
		if ((m_OwnerDrawn == 1)||(m_bVista))
		{
			m_regOwnerDrawn = 0;
			if (m_regOwnerDrawn.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regOwnerDrawn.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else if (m_OwnerDrawn == 0)
		{
			m_regOwnerDrawn = 1;
			if (m_regOwnerDrawn.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regOwnerDrawn.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
			m_regOwnerDrawn = 2;
			if (m_regOwnerDrawn.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regOwnerDrawn.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
		}
	}
	return 0;
}

BOOL CSetLookAndFeelPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	OSVERSIONINFOEX inf;
	ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	if (fullver >= 0x0600)
		m_bVista = true;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_MENULAYOUT_TT);
	if (!m_bVista)
		m_tooltips.AddTool(IDC_ENABLEACCELERATORS, IDS_SETTINGS_OWNERDRAWN_TT);
	else
		GetDlgItem(IDC_ENABLEACCELERATORS)->ShowWindow(SW_HIDE);
	m_tooltips.AddTool(IDC_GETLOCKTOP, IDS_SETTINGS_GETLOCKTOP_TT);

	m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_cMenuList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cMenuList.DeleteColumn(c--);
	m_cMenuList.InsertColumn(0, _T(""));

	m_cMenuList.SetRedraw(false);

	m_imgList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 4, 1);

	InsertItem(IDS_MENUCHECKOUT, IDI_CHECKOUT, MENUCHECKOUT);
	InsertItem(IDS_MENUUPDATE, IDI_UPDATE, MENUUPDATE);
	InsertItem(IDS_MENUCOMMIT, IDI_COMMIT, MENUCOMMIT);
	InsertItem(IDS_MENUDIFF, IDI_DIFF, MENUDIFF);
	InsertItem(IDS_MENULOG, IDI_LOG, MENULOG);
	InsertItem(IDS_MENUSHOWCHANGED, IDI_SHOWCHANGED, MENUSHOWCHANGED);
	InsertItem(IDS_MENUREVISIONGRAPH, IDI_REVISIONGRAPH, MENUREVISIONGRAPH);
	InsertItem(IDS_MENUREPOBROWSE, IDI_REPOBROWSE, MENUREPOBROWSE);
	InsertItem(IDS_MENUCONFLICT, IDI_CONFLICT, MENUCONFLICTEDITOR);
	InsertItem(IDS_MENURESOLVE, IDI_RESOLVE, MENURESOLVE);
	InsertItem(IDS_MENUUPDATEEXT, IDI_UPDATE, MENUUPDATEEXT);
	InsertItem(IDS_MENURENAME, IDI_RENAME, MENURENAME);
	InsertItem(IDS_MENUREMOVE, IDI_DELETE, MENUREMOVE);
	InsertItem(IDS_MENUREVERT, IDI_REVERT, MENUREVERT);
	InsertItem(IDS_MENUDELUNVERSIONED, IDI_DELUNVERSIONED, MENUDELUNVERSIONED);
	InsertItem(IDS_MENUCLEANUP, IDI_CLEANUP, MENUCLEANUP);
	InsertItem(IDS_MENU_LOCK, IDI_LOCK, MENULOCK);
	InsertItem(IDS_MENU_UNLOCK, IDI_UNLOCK, MENUUNLOCK);
	InsertItem(IDS_MENUBRANCH, IDI_COPY, MENUCOPY);
	InsertItem(IDS_MENUSWITCH, IDI_SWITCH, MENUSWITCH);
	InsertItem(IDS_MENUMERGE, IDI_MERGE, MENUMERGE);
	InsertItem(IDS_MENUEXPORT, IDI_EXPORT, MENUEXPORT);
	InsertItem(IDS_MENURELOCATE, IDI_RELOCATE, MENURELOCATE);
	InsertItem(IDS_MENUCREATEREPOS, IDI_CREATEREPOS, MENUCREATEREPOS);
	InsertItem(IDS_MENUADD, IDI_ADD, MENUADD);
	InsertItem(IDS_MENUIMPORT, IDI_IMPORT, MENUIMPORT);
	InsertItem(IDS_MENUBLAME, IDI_BLAME, MENUBLAME);
	InsertItem(IDS_MENUIGNORE, IDI_IGNORE, MENUIGNORE);
	InsertItem(IDS_MENUCREATEPATCH, IDI_CREATEPATCH, MENUCREATEPATCH);
	InsertItem(IDS_MENUAPPLYPATCH, IDI_PATCH, MENUAPPLYPATCH);
	InsertItem(IDS_MENUPROPERTIES, IDI_PROPERTIES, MENUPROPERTIES);

	m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cMenuList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_cMenuList.SetRedraw(true);

	m_bInitialized = TRUE;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CSetLookAndFeelPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CSetLookAndFeelPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSetLookAndFeelPage::InsertItem(UINT nTextID, UINT nIconID, DWORD dwFlags)
{
	HICON hIcon = reinterpret_cast<HICON>(::LoadImage(AfxGetResourceHandle(),
		MAKEINTRESOURCE(nIconID),
		IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT ));
	int nImage = m_imgList.Add(hIcon);
	CString temp;
	temp.LoadString(nTextID);
	CStringUtils::RemoveAccelerators(temp);
	int nIndex = m_cMenuList.GetItemCount();
	m_cMenuList.InsertItem(nIndex, temp, nImage);
	DWORD topmenu = CRegDWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
	m_cMenuList.SetCheck(nIndex, (topmenu & dwFlags));
}

void CSetLookAndFeelPage::OnLvnItemchangedMenulist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	SetModified(TRUE);
	if (m_cMenuList.GetItemCount() > 0)
	{
		int i=0;
		m_topmenu = 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCHECKOUT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUUPDATE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCOMMIT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUDIFF : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENULOG : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUSHOWCHANGED : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREVISIONGRAPH : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREPOBROWSE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCONFLICTEDITOR : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURESOLVE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUUPDATEEXT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURENAME : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREMOVE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREVERT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUDELUNVERSIONED : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCLEANUP : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENULOCK : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUUNLOCK : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCOPY : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUSWITCH : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUMERGE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUEXPORT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURELOCATE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCREATEREPOS : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUADD : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUIMPORT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUBLAME : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUIGNORE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCREATEPATCH : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUAPPLYPATCH : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUPROPERTIES : 0;
	}
	*pResult = 0;
}

void CSetLookAndFeelPage::OnChange()
{
	SetModified();
}


