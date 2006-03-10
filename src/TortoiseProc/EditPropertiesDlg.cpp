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
#include "Utils.h"
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
	ON_NOTIFY(NM_CLICK, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMClickEditproplist)
	ON_BN_CLICKED(IDC_REMOVEPROPS, &CEditPropertiesDlg::OnBnClickedRemoveProps)
	ON_BN_CLICKED(IDC_EDITPROPS, &CEditPropertiesDlg::OnBnClickedEditprops)
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
	AddAnchor(IDC_REMOVEPROPS, BOTTOM_RIGHT);
	AddAnchor(IDC_EDITPROPS, BOTTOM_RIGHT);
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

	return TRUE;
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
			std::map<stdstring,PropValue>::iterator it = m_properties.find(props.GetItemName(p));
			if (it != m_properties.end())
			{
				it->second.count++;
				stdstring value =  MultibyteToWide((char *)props.GetItemValue(p).c_str());
				if (it->second.value.compare(value)!=0)
					it->second.allthesamevalue = false;
			}
			else
			{
				PropValue plist;
				stdstring value =  MultibyteToWide((char *)props.GetItemValue(p).c_str());
				plist.value = value;
				CString stemp = value.c_str();
				stemp.Replace('\n', ' ');
				stemp.Replace(_T("\r"), _T(""));
				plist.value_without_newlines = stdstring((LPCTSTR)stemp);
				plist.count = 1;
				plist.allthesamevalue = true;
				m_properties[props.GetItemName(p)] = plist;
			}
		}
	}
	
	// fill the property list control with the gathered information
	int index=0;
	m_propList.SetRedraw(FALSE);
	for (std::map<stdstring,PropValue>::iterator it = m_properties.begin(); it != m_properties.end(); ++it)
	{
		m_propList.InsertItem(index, it->first.c_str());
		if (it->second.count != m_pathlist.GetCount())
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
	CUtils::ResizeAllListCtrlCols(&m_propList);

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

void CEditPropertiesDlg::OnNMClickEditproplist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	// disable the "remove" button if nothing
	// is selected, enable it otherwise
	int selIndex = m_propList.GetSelectionMark();
	if (selIndex < 0)
	{
		GetDlgItem(IDC_REMOVEPROPS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITPROPS)->SetWindowText(CString(MAKEINTRESOURCE(IDS_EDITPROPS_ADDBUTTON)));
		return;
	}
	else if (m_propList.GetSelectedCount()==0)
	{
		GetDlgItem(IDC_REMOVEPROPS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITPROPS)->SetWindowText(CString(MAKEINTRESOURCE(IDS_EDITPROPS_ADDBUTTON)));
		return;
	}
	GetDlgItem(IDC_REMOVEPROPS)->EnableWindow(TRUE);
	GetDlgItem(IDC_EDITPROPS)->SetWindowText(CString(MAKEINTRESOURCE(IDS_EDITPROPS_EDITBUTTON)));
	GetDlgItem(IDC_EDITPROPS)->EnableWindow(TRUE);
}

void CEditPropertiesDlg::OnBnClickedRemoveProps()
{
	int selIndex = m_propList.GetSelectionMark();
	if (selIndex < 0)
		return;
	bool bRecurse = false;
	CString sName = m_propList.GetItemText(selIndex, 0);
	CString sQuestion;
	sQuestion.Format(IDS_EDITPROPS_RECURSIVEREMOVEQUESTION, sName);
	CString sRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVE));
	CString sNotRecursive(MAKEINTRESOURCE(IDS_EDITPROPS_NOTRECURSIVE));
	CString sCancel(MAKEINTRESOURCE(IDS_EDITPROPS_CANCEL));

	int ret = CMessageBox::Show(m_hWnd, sQuestion, _T("TortoiseSVN"), MB_DEFBUTTON1, IDI_QUESTION, sRecursive, sNotRecursive, sCancel);
	if (ret == 1)
		bRecurse = true;
	else if (ret == 2)
		bRecurse = false;
	else
		return;

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
	Refresh();
}

void CEditPropertiesDlg::OnBnClickedEditprops()
{
	int selIndex = m_propList.GetSelectionMark();

	CEditPropertyValueDlg dlg;
	CString sName;
	CString sValue;
	if (selIndex >= 0)
	{
		sName = m_propList.GetItemText(selIndex, 0);
		PropValue& prop = m_properties[stdstring(sName)];
		sValue = prop.value.c_str();
		dlg.SetPropertyName(sName);
		if (prop.allthesamevalue)
			dlg.SetPropertyValue(sValue);
	}

	if (m_pathlist.GetCount() > 1)
		dlg.SetMultiple();
	else if (m_pathlist.GetCount() == 1)
	{
		if (PathIsDirectory(m_pathlist[0].GetWinPath()))
			dlg.SetFolder();
	}

	dlg.DoModal();

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
			if (!props.Add(sName, CStringA(dlg.GetPropertyValue()), dlg.GetRecursive()))
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
