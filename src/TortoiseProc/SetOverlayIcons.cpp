// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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


// CSetOverlayIcons dialog

IMPLEMENT_DYNAMIC(CSetOverlayIcons, CResizableDialog)
CSetOverlayIcons::CSetOverlayIcons(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSetOverlayIcons::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_regInSubversion = CRegString(_T("Software\\TortoiseSVN\\InSubversionIcon"));
	m_regModified = CRegString(_T("Software\\TortoiseSVN\\ModifiedIcon"));
	m_regConflicted = CRegString(_T("Software\\TortoiseSVN\\ConflictIcon"));
	m_regAdded = CRegString(_T("Software\\TortoiseSVN\\AddedIcon"));
	m_regDeleted = CRegString(_T("Software\\TortoiseSVN\\DeletedIcon"));
}

CSetOverlayIcons::~CSetOverlayIcons()
{
}

void CSetOverlayIcons::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ICONSETCOMBO, m_cIconSet);
	DDX_Control(pDX, IDC_ICONLIST, m_cIconList);
}


BEGIN_MESSAGE_MAP(CSetOverlayIcons, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LISTRADIO, OnBnClickedListradio)
	ON_BN_CLICKED(IDC_SYMBOLRADIO, OnBnClickedSymbolradio)
	ON_CBN_SELCHANGE(IDC_ICONSETCOMBO, OnCbnSelchangeIconsetcombo)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()

void CSetOverlayIcons::OnPaint() 
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

HCURSOR CSetOverlayIcons::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CSetOverlayIcons::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	m_ImageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 20, 10);
	m_ImageListBig.Create(32, 32, ILC_COLOR32 | ILC_MASK, 20, 10);
	m_cIconList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES);
	m_cIconList.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_cIconList.SetImageList(&m_ImageListBig, LVSIL_NORMAL);
	//get the path to our icon sets
	TCHAR procpath[MAX_PATH] = {0};
	GetModuleFileName(NULL, procpath, MAX_PATH);
	*(_tcsrchr(procpath, '\\'))='\0';
	*(_tcsrchr(procpath, '\\'))='\0';
	_tcscat(procpath, _T("\\Icons"));
	m_sIconPath = procpath;
	//list all the icon sets
	CDirFileEnum filefinder(m_sIconPath);
	bool isDir = false;
	CString item;
	while (filefinder.NextFile(item, &isDir))
	{
		if (!isDir)
			continue;
		m_cIconSet.AddString(CUtils::GetFileNameFromPath(item));
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
	TCHAR statustext[40];
	SVNStatus::GetStatusString(svn_wc_status_normal, statustext);
	m_sNormal = statustext;
	SVNStatus::GetStatusString(svn_wc_status_modified, statustext);
	m_sModified = statustext;
	SVNStatus::GetStatusString(svn_wc_status_conflicted, statustext);
	m_sConflicted = statustext;
	SVNStatus::GetStatusString(svn_wc_status_added, statustext);
	m_sAdded = statustext;
	SVNStatus::GetStatusString(svn_wc_status_deleted, statustext);
	m_sDeleted = statustext;

	ShowIconSet();

	AddAnchor(IDC_ICONSETLABEL, TOP_LEFT);
	AddAnchor(IDC_ICONSETCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_ICONLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LISTRADIO, BOTTOM_LEFT);
	AddAnchor(IDC_SYMBOLRADIO, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	return TRUE;
}

void CSetOverlayIcons::ShowIconSet()
{
	m_cIconList.DeleteAllItems();
	for (int i=0; i<m_ImageList.GetImageCount(); ++i)
	{
		m_ImageList.Remove(i);
	}
	for (int i=0; i<m_ImageListBig.GetImageCount(); ++i)
	{
		m_ImageListBig.Remove(i);
	}
	//find all the icons of the selected icon set
	CString sIconSet;
	int index = m_cIconSet.GetCurSel();
	if (index == CB_ERR)
	{
		//nothing selected. Shouldn't happen!
		index = 0;
	}
	m_cIconSet.GetLBText(index, sIconSet);
	CString sIconSetPath = m_sIconPath + _T("\\") + sIconSet;

	BOOL bSmallIcons = (GetCheckedRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO) == IDC_LISTRADIO);
	CImageList * pImageList = bSmallIcons ? &m_ImageList : &m_ImageListBig;
	int pixelsize = (bSmallIcons ? 16 : 32);
	HICON hNormalOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseInSubversion.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hNormalOverlay);
	pImageList->SetOverlayImage(index, 1);
	DestroyIcon(hNormalOverlay);
	HICON hModifiedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseModified.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hModifiedOverlay);
	pImageList->SetOverlayImage(index, 2);
	DestroyIcon(hModifiedOverlay);
	HICON hConflictedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseConflict.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hConflictedOverlay);
	pImageList->SetOverlayImage(index, 3);
	DestroyIcon(hConflictedOverlay);
	HICON hAddedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseAdded.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hAddedOverlay);
	pImageList->SetOverlayImage(index, 4);
	DestroyIcon(hAddedOverlay);
	HICON hDeletedOverlay = (HICON)LoadImage(NULL, sIconSetPath+_T("\\TortoiseDeleted.ico"), IMAGE_ICON, pixelsize, pixelsize, LR_LOADFROMFILE);
	index = pImageList->Add(hDeletedOverlay);
	pImageList->SetOverlayImage(index, 5);
	DestroyIcon(hDeletedOverlay);

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
	index = m_cIconList.InsertItem(0, _T("normal"), folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(1, _T("modified"), folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(2, _T("conflicted"), folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(3, _T("added"), folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(4, _T("deleted"), folderindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);

	AddFileTypeGroup(_T(".cpp"));
	AddFileTypeGroup(_T(".h"));
	AddFileTypeGroup(_T(".txt"));
	AddFileTypeGroup(_T(".java"));
	AddFileTypeGroup(_T(".doc"));
	AddFileTypeGroup(_T(".pl"));
	AddFileTypeGroup(_T(".php"));
	AddFileTypeGroup(_T(".asp"));
	AddFileTypeGroup(_T(".cs"));
	AddFileTypeGroup(_T(".vb"));
	AddFileTypeGroup(_T(".xml"));
	AddFileTypeGroup(_T(".pas"));
	AddFileTypeGroup(_T(".dpr"));
	AddFileTypeGroup(_T(".dfm"));
	AddFileTypeGroup(_T(".res"));
	AddFileTypeGroup(_T(".asmx"));
	AddFileTypeGroup(_T(".aspx"));
	AddFileTypeGroup(_T(".resx"));
	AddFileTypeGroup(_T(".vbp"));
	AddFileTypeGroup(_T(".frm"));
	AddFileTypeGroup(_T(".frx"));
	AddFileTypeGroup(_T(".bas"));
	AddFileTypeGroup(_T(".config"));
	m_cIconList.RedrawItems(0, m_cIconList.GetItemCount());
}
void CSetOverlayIcons::AddFileTypeGroup(CString sFileType)
{
	BOOL bSmallIcons = (GetCheckedRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO) == IDC_LISTRADIO);
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
	index = m_cIconList.InsertItem(0, m_sNormal+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(1, m_sModified+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(2, m_sConflicted+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(3, m_sAdded+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
	index = m_cIconList.InsertItem(4, m_sDeleted+sFileType, imageindex);
	m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);

}

void CSetOverlayIcons::OnBnClickedListradio()
{
	m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
	ShowIconSet();
}

void CSetOverlayIcons::OnBnClickedSymbolradio()
{
	m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
	ShowIconSet();
}

void CSetOverlayIcons::OnCbnSelchangeIconsetcombo()
{
	ShowIconSet();
}

void CSetOverlayIcons::OnOK()
{
	CString sIconSet;
	int index = m_cIconSet.GetCurSel();
	if (index != CB_ERR)
	{
		m_cIconSet.GetLBText(index, sIconSet);
		if (sIconSet.CompareNoCase(m_sOriginalIconSet)!=0)
		{
			// the selected icon set has changed.
			CString msg;
			msg.Format(IDS_SETTINGS_ICONSETCHANGED, m_sOriginalIconSet, sIconSet);
			UINT ret = CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_ICONQUESTION | MB_OKCANCEL);
			if (ret != IDCANCEL)
			{
				m_regInSubversion = m_sIconPath + _T("\\") + sIconSet + _T("\\TortoiseInSubversion.ico");
				m_regModified = m_sIconPath + _T("\\") + sIconSet + _T("\\TortoiseModified.ico");
				m_regConflicted = m_sIconPath + _T("\\") + sIconSet + _T("\\TortoiseConflict.ico");
				m_regAdded = m_sIconPath + _T("\\") + sIconSet + _T("\\TortoiseAdded.ico");
				m_regDeleted = m_sIconPath + _T("\\") + sIconSet + _T("\\TortoiseDeleted.ico");
			}
		}
	}
	CResizableDialog::OnOK();
}

void CSetOverlayIcons::OnBnClickedHelp()
{
	OnHelp();
}
