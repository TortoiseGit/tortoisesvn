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
#include "SetMenuPage.h"
#include ".\setmenupage.h"
#include "Globals.h"

// CSetMenuPage dialog


IMPLEMENT_DYNAMIC(CSetMenuPage, CPropertyPage)
CSetMenuPage::CSetMenuPage()
	: CPropertyPage(CSetMenuPage::IDD)
	, m_bModified(FALSE)
{
	m_topmenu = CRegDWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
}

CSetMenuPage::~CSetMenuPage()
{
}

void CSetMenuPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
}


BEGIN_MESSAGE_MAP(CSetMenuPage, CPropertyPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
END_MESSAGE_MAP()

void CSetMenuPage::SaveData()
{
	if (m_bModified)
	{
		CRegDWORD regtopmenu = CRegDWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
		regtopmenu = m_topmenu;
	} // if (m_bModified) 
}

// CSetMenuPage message handlers

BOOL CSetMenuPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

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
	InsertItem(IDS_MENUREPOBROWSE, IDI_REPOBROWSE, MENUREPOBROWSE);
	InsertItem(IDS_MENUCONFLICT, IDI_CONFLICT, MENUCONFLICTEDITOR);
	InsertItem(IDS_MENURESOLVE, IDI_RESOLVE, MENURESOLVE);
	InsertItem(IDS_MENUUPDATEEXT, IDI_UPDATE, MENUUPDATEEXT);
	InsertItem(IDS_MENURENAME, IDI_RENAME, MENURENAME);
	InsertItem(IDS_MENUREMOVE, IDI_DELETE, MENUREMOVE);
	InsertItem(IDS_MENUREVERT, IDI_REVERT, MENUREVERT);
	InsertItem(IDS_MENUCLEANUP, IDI_CLEANUP, MENUCLEANUP);
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

	m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cMenuList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cMenuList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_cMenuList.SetRedraw(true);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSetMenuPage::InsertItem(UINT nTextID, UINT nIconID, DWORD dwFlags)
{
	HICON hIcon = reinterpret_cast<HICON>(::LoadImage(AfxGetResourceHandle(),
											MAKEINTRESOURCE(nIconID),
											IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT ));
	int nImage = m_imgList.Add(hIcon);
	CString temp;
	temp.LoadString(nTextID);
	int nIndex = m_cMenuList.GetItemCount();
	m_cMenuList.InsertItem(nIndex, temp, nImage);
	DWORD topmenu = CRegDWORD(_T("Software\\TortoiseSVN\\ContextMenuEntries"), MENUCHECKOUT | MENUUPDATE | MENUCOMMIT);
	m_cMenuList.SetCheck(nIndex, topmenu & dwFlags);
}

BOOL CSetMenuPage::OnApply()
{
	SaveData();
	SetModified(FALSE);
	m_bModified = FALSE;
	return CPropertyPage::OnApply();
}

void CSetMenuPage::OnLvnItemchangedMenulist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	SetModified(TRUE);
	m_bModified = TRUE;
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
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREPOBROWSE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCONFLICTEDITOR : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURESOLVE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUUPDATEEXT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENURENAME : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREMOVE : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUREVERT : 0;
		m_topmenu |= m_cMenuList.GetCheck(i++) ? MENUCLEANUP : 0;
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
	} // if (m_cMenuList.GetItemCount() > 0) 
	*pResult = 0;
}
