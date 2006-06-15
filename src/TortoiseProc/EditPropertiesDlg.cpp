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
#include "SVNProperties.h"
#include "MessageBox.h"
#include "TortoiseProc.h"
#include "EditPropertiesDlg.h"
#include "EditPropertyValueDlg.h"
#include "UnicodeStrings.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "ProgressDlg.h"


// CEditPropertiesDlg dialog

IMPLEMENT_DYNAMIC(CEditPropertiesDlg, CResizableStandAloneDialog)

CEditPropertiesDlg::CEditPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CEditPropertiesDlg::IDD, pParent)
	, m_bRecursive(FALSE)
	, m_bChanged(false)
{

}

CEditPropertiesDlg::~CEditPropertiesDlg()
{
}

void CEditPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDITPROPLIST, m_propList);
}


BEGIN_MESSAGE_MAP(CEditPropertiesDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, &CEditPropertiesDlg::OnBnClickedHelp)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMCustomdrawEditproplist)
	ON_BN_CLICKED(IDC_REMOVEPROPS, &CEditPropertiesDlg::OnBnClickedRemoveProps)
	ON_BN_CLICKED(IDC_EDITPROPS, &CEditPropertiesDlg::OnBnClickedEditprops)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnLvnItemchangedEditproplist)
	ON_NOTIFY(NM_DBLCLK, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMDblclkEditproplist)
	ON_BN_CLICKED(IDC_SAVEPROP, &CEditPropertiesDlg::OnBnClickedSaveprop)
	ON_BN_CLICKED(IDC_ADDPROPS, &CEditPropertiesDlg::OnBnClickedAddprops)
END_MESSAGE_MAP()


// CEditPropertiesDlg message handlers

void CEditPropertiesDlg::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CEditPropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// fill in the path edit control
	if (m_pathlist.GetCount() == 1)
	{
		GetDlgItem(IDC_PROPPATH)->SetWindowText(m_pathlist[0].GetWinPathString());
	}
	else
	{
		CString sTemp;
		sTemp.Format(IDS_EDITPROPS_NUMPATHS, m_pathlist.GetCount());
		GetDlgItem(IDC_PROPPATH)->SetWindowText(sTemp);
	}

	// initialize the property list control
	m_propList.DeleteAllItems();
	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
	m_propList.SetExtendedStyle(exStyle);
	int c = ((CHeaderCtrl*)(m_propList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_propList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROPPROPERTY);
	m_propList.InsertColumn(0, temp);
	temp.LoadString(IDS_PROPVALUE);
	m_propList.InsertColumn(1, temp);
	m_propList.SetRedraw(false);



	AddAnchor(IDC_GROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROPPATH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EDITPROPLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SAVEPROP, BOTTOM_RIGHT);
	AddAnchor(IDC_REMOVEPROPS, BOTTOM_RIGHT);
	AddAnchor(IDC_EDITPROPS, BOTTOM_RIGHT);
	AddAnchor(IDC_ADDPROPS, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("EditPropertiesDlg"));

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(PropsThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_EDITPROPLIST)->SetFocus();

	return FALSE;
}

void CEditPropertiesDlg::Refresh()
{
	if (m_bThreadRunning)
		return;
	m_propList.DeleteAllItems();
	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(PropsThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

UINT CEditPropertiesDlg::PropsThreadEntry(LPVOID pVoid)
{
	return ((CEditPropertiesDlg*)pVoid)->PropsThread();
}

UINT CEditPropertiesDlg::PropsThread()
{
	// get all properties
	m_properties.clear();
	for (int i=0; i<m_pathlist.GetCount(); ++i)
	{
		SVNProperties props(m_pathlist[i]);
		for (int p=0; p<props.GetCount(); ++p)
		{
			wide_string prop_str = props.GetItemName(p);
			std::string prop_value = props.GetItemValue(p);
			std::map<stdstring,PropValue>::iterator it = m_properties.lower_bound(prop_str);
			if (it != m_properties.end() && it->first == prop_str)
			{
				it->second.count++;
				if (it->second.value.compare(prop_value)!=0)
					it->second.allthesamevalue = false;
			}
			else
			{
				it = m_properties.insert(it, std::make_pair(prop_str, PropValue()));
				stdstring value = MultibyteToWide((char *)prop_value.c_str());
				it->second.value = prop_value;
				CString stemp = value.c_str();
				stemp.Replace('\n', ' ');
				stemp.Replace(_T("\r"), _T(""));
				it->second.value_without_newlines = stdstring((LPCTSTR)stemp);
				it->second.count = 1;
				it->second.allthesamevalue = true;
				if (SVNProperties::IsBinary(prop_value))
					it->second.isbinary = true;
			}
		}
	}
	
	// fill the property list control with the gathered information
	int index=0;
	m_propList.SetRedraw(FALSE);
	for (std::map<stdstring,PropValue>::iterator it = m_properties.begin(); it != m_properties.end(); ++it)
	{
		m_propList.InsertItem(index, it->first.c_str());
		if (it->second.isbinary)
		{
			m_propList.SetItemText(index, 1, CString(MAKEINTRESOURCE(IDS_EDITPROPS_BINVALUE)));
			m_propList.SetItemData(index, FALSE);
		}
		else if (it->second.count != m_pathlist.GetCount())
		{
			// if the property values are the same for all paths they're set
			// but they're not set for *all* paths, then we show the entry grayed out
			m_propList.SetItemText(index, 1, it->second.value_without_newlines.c_str());
			m_propList.SetItemData(index, FALSE);
		}
		else if (it->second.allthesamevalue)
		{
			// if the property values are the same for all paths,
			// we show the value
			m_propList.SetItemText(index, 1, it->second.value_without_newlines.c_str());
			m_propList.SetItemData(index, TRUE);
		}
		else
		{
			// if the property values aren't the same for all paths
			// then we show "values are different" instead of the value
			CString sTemp(MAKEINTRESOURCE(IDS_EDITPROPS_DIFFERENTPROPVALUES));
			m_propList.SetItemText(index, 1, sTemp);
			m_propList.SetItemData(index, FALSE);
		}
		index++;
	}
	int maxcol = ((CHeaderCtrl*)(m_propList.GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		m_propList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	}

	InterlockedExchange(&m_bThreadRunning, FALSE);
	m_propList.SetRedraw(TRUE);
	return 0;
}

void CEditPropertiesDlg::OnNMCustomdrawEditproplist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (m_bThreadRunning)
		return;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		if ((int)pLVCD->nmcd.dwItemSpec > m_propList.GetItemCount())
			return;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
		if (m_propList.GetItemData(pLVCD->nmcd.dwItemSpec)==FALSE)
			crText = GetSysColor(COLOR_GRAYTEXT);
		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
	}

}

void CEditPropertiesDlg::OnLvnItemchangedEditproplist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	// disable the "remove" button if nothing
	// is selected, enable it otherwise
	int selIndex = m_propList.GetSelectionMark();
	if (selIndex < 0)
	{
		DialogEnableWindow(IDC_REMOVEPROPS, FALSE);
		DialogEnableWindow(IDC_SAVEPROP, FALSE);
		DialogEnableWindow(IDC_EDITPROPS, FALSE);
		return;
	}
	else if (m_propList.GetSelectedCount()==0)
	{
		DialogEnableWindow(IDC_REMOVEPROPS, FALSE);
		DialogEnableWindow(IDC_SAVEPROP, FALSE);
		DialogEnableWindow(IDC_EDITPROPS, FALSE);
		return;
	}
	DialogEnableWindow(IDC_REMOVEPROPS, TRUE);
	DialogEnableWindow(IDC_SAVEPROP, TRUE);
	DialogEnableWindow(IDC_EDITPROPS, TRUE);
	*pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedRemoveProps()
{
	POSITION pos = m_propList.GetFirstSelectedItemPosition();
	while ( pos )
	{
		int selIndex = m_propList.GetNextSelectedItem(pos);

		bool bRecurse = false;
		CString sName = m_propList.GetItemText(selIndex, 0);
		CString sQuestion;
		sQuestion.Format(IDS_EDITPROPS_RECURSIVEREMOVEQUESTION, sName);
		CString sRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVE));
		CString sNotRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_NOTRECURSIVE));
		CString sCancel(MAKEINTRESOURCE(IDS_EDITPROPS_CANCEL));

		if ((m_pathlist.GetCount()>1)||((m_pathlist.GetCount()==1)&&(PathIsDirectory(m_pathlist[0].GetWinPath()))))
		{
			int ret = CMessageBox::Show(m_hWnd, sQuestion, _T("TortoiseSVN"), MB_DEFBUTTON1, IDI_QUESTION, sRecursive, sNotRecursive, sCancel);
			if (ret == 1)
				bRecurse = true;
			else if (ret == 2)
				bRecurse = false;
			else
				break;
		}

		CProgressDlg prog;
		CString sTemp;
		sTemp.LoadString(IDS_SETPROPTITLE);
		prog.SetTitle(sTemp);
		sTemp.LoadString(IDS_PROPWAITCANCEL);
		prog.SetCancelMsg(sTemp);
		prog.SetTime(TRUE);
		prog.SetShowProgressBar(TRUE);
		prog.ShowModal(m_hWnd);
		for (int i=0; i<m_pathlist.GetCount(); ++i)
		{
			prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
			SVNProperties props(m_pathlist[i]);
			if (!props.Remove(sName, bRecurse))
			{
				CMessageBox::Show(m_hWnd, props.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
				m_bChanged = true;
		}
		prog.Stop();
	}
	DialogEnableWindow(IDC_REMOVEPROPS, FALSE);
	DialogEnableWindow(IDC_SAVEPROP, FALSE);
	Refresh();
}

void CEditPropertiesDlg::OnNMDblclkEditproplist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	EditProps();
	*pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedEditprops()
{
	EditProps();
}

void CEditPropertiesDlg::OnBnClickedAddprops()
{
	EditProps(true);
}

void CEditPropertiesDlg::EditProps(bool bAdd /* = false*/)
{
	int selIndex = m_propList.GetSelectionMark();

	CEditPropertyValueDlg dlg;
	CString sName;
	if ((bAdd==false)&&(selIndex >= 0)&&(m_propList.GetSelectedCount()))
	{
		sName = m_propList.GetItemText(selIndex, 0);
		PropValue& prop = m_properties[stdstring(sName)];
		dlg.SetPropertyName(sName);
		if (prop.allthesamevalue)
			dlg.SetPropertyValue(prop.value);
		dlg.SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_EDITTITLE)));
	}
	else
		dlg.SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_ADDTITLE)));


	if (m_pathlist.GetCount() > 1)
		dlg.SetMultiple();
	else if (m_pathlist.GetCount() == 1)
	{
		if (PathIsDirectory(m_pathlist[0].GetWinPath()))
			dlg.SetFolder();
	}

	if ( dlg.DoModal()==IDOK )
	{
		sName = dlg.GetPropertyName();
		if (!sName.IsEmpty())
		{
			CProgressDlg prog;
			CString sTemp;
			sTemp.LoadString(IDS_SETPROPTITLE);
			prog.SetTitle(sTemp);
			sTemp.LoadString(IDS_PROPWAITCANCEL);
			prog.SetCancelMsg(sTemp);
			prog.SetTime(TRUE);
			prog.SetShowProgressBar(TRUE);
			prog.ShowModal(m_hWnd);
			for (int i=0; i<m_pathlist.GetCount(); ++i)
			{
				prog.SetLine(1, m_pathlist[i].GetWinPath(), true);
				SVNProperties props(m_pathlist[i]);
				if (!props.Add(sName, dlg.IsBinary() ? dlg.GetPropertyValue() : dlg.GetPropertyValue().c_str(), dlg.GetRecursive()))
				{
					CMessageBox::Show(m_hWnd, props.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
				}
				else
					m_bChanged = true;
			}
			prog.Stop();
			Refresh();
		}
	}
}

void CEditPropertiesDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	CStandAloneDialogTmpl<CResizableDialog>::OnOK();
}

void CEditPropertiesDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;
	CStandAloneDialogTmpl<CResizableDialog>::OnCancel();
}

BOOL CEditPropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bThreadRunning)
					return __super::PreTranslateMessage(pMsg);
				Refresh();
			}
			break;
		default:
			break;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CEditPropertiesDlg::OnBnClickedSaveprop()
{
	int selIndex = m_propList.GetSelectionMark();

	CString sName;
	CString sValue;
	if ((selIndex >= 0)&&(m_propList.GetSelectedCount()))
	{
		sName = m_propList.GetItemText(selIndex, 0);
		PropValue& prop = m_properties[stdstring(sName)];
		sValue = prop.value.c_str();
		if (prop.allthesamevalue)
		{
			// now save the property value
			OPENFILENAME ofn;		// common dialog box structure
			TCHAR szFile[MAX_PATH];  // buffer for file name
			_tcscpy_s(szFile, (LPCTSTR)sName);
			CString temp;
			ZeroMemory(szFile, sizeof(szFile));
			// Initialize OPENFILENAME
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = m_hWnd;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
			temp.LoadString(IDS_REPOBROWSE_SAVEAS);
			CStringUtils::RemoveAccelerators(temp);
			if (temp.IsEmpty())
				ofn.lpstrTitle = NULL;
			else
				ofn.lpstrTitle = temp;
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;

			// Display the Open dialog box. 
			if (GetSaveFileName(&ofn)==FALSE)
			{
				return;
			}
			FILE * stream;
			_tfopen_s(&stream, ofn.lpstrFile, _T("wbS"));
			fwrite(prop.value.c_str(), sizeof(char), prop.value.size(), stream);
			fclose(stream);
		}
	}
}

