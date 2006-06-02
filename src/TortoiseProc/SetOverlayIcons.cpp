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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "DirFileEnum.h"
#include "MessageBox.h"
#include ".\setoverlayicons.h"
#include "SVNStatus.h"
#include "Utils.h"
#include "PathUtils.h"
#include "ShellUpdater.h"

// CSetOverlayIcons dialog

IMPLEMENT_DYNAMIC(CSetOverlayIcons, CPropertyPage)
CSetOverlayIcons::CSetOverlayIcons()
	: CPropertyPage(CSetOverlayIcons::IDD)
{ 
	m_regInSubversion = CRegString(_T("Software\\TortoiseSVN\\InSubversionIcon"));
	m_regModified = CRegString(_T("Software\\TortoiseSVN\\ModifiedIcon"));
	m_regConflicted = CRegString(_T("Software\\TortoiseSVN\\ConflictIcon"));
	m_regReadOnly = CRegString(_T("Software\\TortoiseSVN\\ReadOnlyIcon"));
	m_regDeleted = CRegString(_T("Software\\TortoiseSVN\\DeletedIcon"));
	m_regLocked = CRegString(_T("Software\\TortoiseSVN\\LockedIcon"));
	m_regAdded = CRegString(_T("Software\\TortoiseSVN\\AddedIcon"));
}

CSetOverlayIcons::~CSetOverlayIcons()
{
}

void CSetOverlayIcons::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ICONSETCOMBO, m_cIconSet);
	DDX_Control(pDX, IDC_ICONLIST, m_cIconList);
}


BEGIN_MESSAGE_MAP(CSetOverlayIcons, CPropertyPage)
	ON_BN_CLICKED(IDC_LISTRADIO, OnBnClickedListradio)
	ON_BN_CLICKED(IDC_SYMBOLRADIO, OnBnClickedSymbolradio)
	ON_CBN_SELCHANGE(IDC_ICONSETCOMBO, OnCbnSelchangeIconsetcombo)
END_MESSAGE_MAP()

BOOL CSetOverlayIcons::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_cIconList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES);
	//get the path to our icon sets
	m_sIconPath = CPathUtils::GetAppParentDirectory();
	m_sIconPath += _T("Icons");
	//list all the icon sets
	CDirFileEnum filefinder(m_sIconPath);
	bool isDir = false;
	CString item;
	while (filefinder.NextFile(item, &isDir))
	{
		if (!isDir)
			continue;
		m_cIconSet.AddString(CPathUtils::GetFileNameFromPath(item));
	}
	CheckRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO, IDC_LISTRADIO);
	CString sModifiedIcon = m_regModified;
	if (sModifiedIcon.IsEmpty())
	{
		//no custom icon set, use the default
		sModifiedIcon = m_sIconPath + _T("\\XPStyle\\TortoiseModified.ico");
	}
	if (sModifiedIcon.Left(m_sIconPath.GetLength()).CompareNoCase(m_sIconPath)!=0)
	{
		//an icon set outside our own installation? We don't support that,
		//so fall back to the default!
		sModifiedIcon = m_sIconPath + _T("\\XPStyle\\TortoiseModified.ico");
	}
	//the name of the icon set is the folder of the icon location
	m_sOriginalIconSet = sModifiedIcon.Mid(m_sIconPath.GetLength()+1);
	m_sOriginalIconSet = m_sOriginalIconSet.Left(m_sOriginalIconSet.ReverseFind('\\'));
	//now we have the name of the icon set. Set the combobox to show
	//that as selected
	CString ComboItem;
	for (int i=0; i<m_cIconSet.GetCount(); ++i)
	{
		m_cIconSet.GetLBText(i, ComboItem);
		if (ComboItem.CompareNoCase(m_sOriginalIconSet)==0)
			m_cIconSet.SetCurSel(i);
	}
	WORD langID = (WORD)(DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
	TCHAR statustext[MAX_STATUS_STRING_LENGTH];
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_normal, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sNormal = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_modified, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sModified = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_conflicted, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sConflicted = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_deleted, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sDeleted = statustext;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_added, statustext, sizeof(statustext)/sizeof(TCHAR), langID);
	m_sAdded = statustext;
	m_sReadOnly.LoadString(IDS_SETTINGS_READONLYNAME);
	m_sLocked.LoadString(IDS_SETTINGS_LOCKEDNAME);
	ShowIconSet(true);

	return TRUE;
}

void CSetOverlayIcons::ShowIconSet(bool bSmallIcons)
{
	m_cIconList.SetRedraw(FALSE);
	m_cIconList.DeleteAllItems();
	m_ImageList.DeleteImageList();
	m_ImageListBig.DeleteImageList();
	m_ImageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 20, 10);
	m_ImageListBig.Create(32, 32, ILC_COLOR32 | ILC_MASK, 20, 10);
	m_cIconList.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_cIconList.SetImageList(&m_ImageListBig, LVSIL_NORMAL);

	//find all the icons of the selected icon set
	CString sIconSet;
	int index = m_cIconSet.GetCurSel();
	if (index == CB_ERR)
	{
		//nothing selected. Shouldn't happen!
		return;
	}
	m_cIconSet.GetLBText(index, sIconSet);
	CString sIconSetPath = m_sIconPath + _T("\\") + sIconSet;

	CImageList * pImageList = bSmallIcons ? &m_ImageList : &m_ImageListBig;
	int pixelsize = (bSmallIcons ? 16 : 32);
	HICON hNormalOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseInSubversion.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hNormalOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 1));
	HICON hModifiedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseModified.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hModifiedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 2));
	HICON hConflictedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseConflict.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hConflictedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 3));
	HICON hReadOnlyOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseReadOnly.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hReadOnlyOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 4));
	HICON hDeletedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseDeleted.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hDeletedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 5));
	HICON hLockedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseLocked.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hLockedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 6));
	HICON hAddedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseAdded.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hAddedOverlay);
	VERIFY(pImageList->SetOverlayImage(index, 7));
	DestroyIcon(hNormalOverlay);
	DestroyIcon(hModifiedOverlay);
	DestroyIcon(hConflictedOverlay);
	DestroyIcon(hReadOnlyOverlay);
	DestroyIcon(hDeletedOverlay);
	DestroyIcon(hLockedOverlay);
	DestroyIcon(hAddedOverlay);

	//create an image list with different file icons
	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof sfi);

	UINT flags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;
	if (bSmallIcons)
		flags |= SHGFI_SMALLICON;
	else
		flags |= SHGFI_LARGEICON;
	SHGetFileInfo(
		_T("Doesn't matter"),
		FILE_ATTRIBUTE_DIRECTORY,
		&sfi, sizeof sfi,
		flags);

	int folderindex = pImageList->Add(sfi.hIcon);	//folder
	DestroyIcon(sfi.hIcon);
	//folders
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sNormal, folderindex);
	VERIFY(m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK));
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sModified, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sConflicted, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sReadOnly, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sDeleted, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sLocked, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(6), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sAdded, folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(7), TVIS_OVERLAYMASK);

	AddFileTypeGroup(_T(".cpp"), bSmallIcons);
	AddFileTypeGroup(_T(".h"), bSmallIcons);
	AddFileTypeGroup(_T(".txt"), bSmallIcons);
	AddFileTypeGroup(_T(".java"), bSmallIcons);
	AddFileTypeGroup(_T(".doc"), bSmallIcons);
	AddFileTypeGroup(_T(".pl"), bSmallIcons);
	AddFileTypeGroup(_T(".php"), bSmallIcons);
	AddFileTypeGroup(_T(".asp"), bSmallIcons);
	AddFileTypeGroup(_T(".cs"), bSmallIcons);
	AddFileTypeGroup(_T(".vb"), bSmallIcons);
	AddFileTypeGroup(_T(".xml"), bSmallIcons);
	AddFileTypeGroup(_T(".pas"), bSmallIcons);
	AddFileTypeGroup(_T(".dpr"), bSmallIcons);
	AddFileTypeGroup(_T(".dfm"), bSmallIcons);
	AddFileTypeGroup(_T(".res"), bSmallIcons);
	AddFileTypeGroup(_T(".asmx"), bSmallIcons);
	AddFileTypeGroup(_T(".aspx"), bSmallIcons);
	AddFileTypeGroup(_T(".resx"), bSmallIcons);
	AddFileTypeGroup(_T(".vbp"), bSmallIcons);
	AddFileTypeGroup(_T(".frm"), bSmallIcons);
	AddFileTypeGroup(_T(".frx"), bSmallIcons);
	AddFileTypeGroup(_T(".bas"), bSmallIcons);
	AddFileTypeGroup(_T(".config"), bSmallIcons);
	AddFileTypeGroup(_T(".css"), bSmallIcons);
	AddFileTypeGroup(_T(".acsx"), bSmallIcons);
	AddFileTypeGroup(_T(".jpg"), bSmallIcons);
	AddFileTypeGroup(_T(".png"), bSmallIcons);
	m_cIconList.SetRedraw(TRUE);
}
void CSetOverlayIcons::AddFileTypeGroup(CString sFileType, bool bSmallIcons)
{
	UINT flags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;
	if (bSmallIcons)
		flags |= SHGFI_SMALLICON;
	else
		flags |= SHGFI_LARGEICON;
	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(
		sFileType,
		FILE_ATTRIBUTE_NORMAL,
		&sfi, sizeof sfi,
		flags);

	int imageindex = 0;
	if (bSmallIcons)
		imageindex = m_ImageList.Add(sfi.hIcon);
	else
		imageindex = m_ImageListBig.Add(sfi.hIcon);

	DestroyIcon(sfi.hIcon);
	int index = 0;
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sNormal+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sModified+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sConflicted+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sReadOnly+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sDeleted+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sLocked+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(6), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sAdded+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(7), TVIS_OVERLAYMASK);
}

void CSetOverlayIcons::OnBnClickedListradio()
{
	m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
	ShowIconSet(true);
}

void CSetOverlayIcons::OnBnClickedSymbolradio()
{
	m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
	ShowIconSet(false);
}

void CSetOverlayIcons::OnCbnSelchangeIconsetcombo()
{
	bool bSmallIcons = (GetCheckedRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO) == IDC_LISTRADIO);
	ShowIconSet(bSmallIcons);
	m_selIndex = m_cIconSet.GetCurSel();
	if (m_selIndex != CB_ERR)
	{
		m_cIconSet.GetLBText(m_selIndex, m_sIconSet);
	}
	if (m_sIconSet.CompareNoCase(m_sOriginalIconSet)!=0)
		SetModified();
}

int CSetOverlayIcons::SaveData()
{
	if (m_sIconSet.IsEmpty())
		return 0;
	if (m_sIconSet.CompareNoCase(m_sOriginalIconSet)!=0)
	{
		// the selected icon set has changed.
		CString msg;
		msg.Format(IDS_SETTINGS_ICONSETCHANGED, m_sOriginalIconSet, m_sIconSet);
		UINT ret = CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_ICONQUESTION | MB_OKCANCEL);
		if (ret != IDCANCEL)
		{
			m_regInSubversion = m_sIconPath + _T("\\") + m_sIconSet + _T("\\TortoiseInSubversion.ico");
			if (m_regInSubversion.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regInSubversion.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
			m_regModified = m_sIconPath + _T("\\") + m_sIconSet + _T("\\TortoiseModified.ico");
			if (m_regModified.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regModified.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
			m_regConflicted = m_sIconPath + _T("\\") + m_sIconSet + _T("\\TortoiseConflict.ico");
			if (m_regConflicted.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regConflicted.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
			m_regReadOnly = m_sIconPath + _T("\\") + m_sIconSet + _T("\\TortoiseReadOnly.ico");
			if (m_regReadOnly.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regReadOnly.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
			m_regDeleted = m_sIconPath + _T("\\") + m_sIconSet + _T("\\TortoiseDeleted.ico");
			if (m_regDeleted.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regDeleted.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
			m_regLocked = m_sIconPath + _T("\\") + m_sIconSet + _T("\\TortoiseLocked.ico");
			if (m_regLocked.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regLocked.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
			m_regAdded = m_sIconPath + _T("\\") + m_sIconSet + _T("\\TortoiseAdded.ico");
			if (m_regAdded.LastError != ERROR_SUCCESS)
				CMessageBox::Show(m_hWnd, m_regAdded.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
			return 1;
		}
		m_sOriginalIconSet = m_sIconSet;
	}
	return 0;
}

BOOL CSetOverlayIcons::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}
