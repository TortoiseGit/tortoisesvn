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
#include "EditPropertyValueDlg.h"


// CEditPropertyValueDlg dialog

IMPLEMENT_DYNAMIC(CEditPropertyValueDlg, CResizableStandAloneDialog)

CEditPropertyValueDlg::CEditPropertyValueDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CEditPropertyValueDlg::IDD, pParent)
	, m_PropValue(_T(""))
	, m_bRecursive(FALSE)
	, m_bFolder(false)
	, m_bMultiple(false)
{

}

CEditPropertyValueDlg::~CEditPropertyValueDlg()
{
}

void CEditPropertyValueDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROPNAMECOMBO, m_PropNames);
	DDX_Text(pDX, IDC_PROPVALUE, m_PropValue);
	DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropertyValueDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, &CEditPropertyValueDlg::OnBnClickedHelp)
	ON_CBN_SELCHANGE(IDC_PROPNAMECOMBO, &CEditPropertyValueDlg::CheckRecursive)
	ON_CBN_EDITCHANGE(IDC_PROPNAMECOMBO, &CEditPropertyValueDlg::CheckRecursive)
END_MESSAGE_MAP()


// CEditPropertyValueDlg message handlers

BOOL CEditPropertyValueDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// fill the combobox control with all the
	// known properties
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:eol-style"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:executable"));
	if ((m_bFolder)||(m_bMultiple))
		m_PropNames.AddString(_T("svn:externals"));
	if ((m_bFolder)||(m_bMultiple))
		m_PropNames.AddString(_T("svn:ignore"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:keywords"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:needs-lock"));
	if ((!m_bFolder)&&(!m_bMultiple))
		m_PropNames.AddString(_T("svn:mime-type"));

	if ((m_bFolder)||(m_bMultiple))
	{
		m_PropNames.AddString(_T("bugtraq:url"));
		m_PropNames.AddString(_T("bugtraq:logregex"));
		m_PropNames.AddString(_T("bugtraq:label"));
		m_PropNames.AddString(_T("bugtraq:message"));
		m_PropNames.AddString(_T("bugtraq:number"));
		m_PropNames.AddString(_T("bugtraq:warnifnoissue"));
		m_PropNames.AddString(_T("bugtraq:append"));

		m_PropNames.AddString(_T("tsvn:logtemplate"));
		m_PropNames.AddString(_T("tsvn:logwidthmarker"));
		m_PropNames.AddString(_T("tsvn:logminsize"));
		m_PropNames.AddString(_T("tsvn:logfilelistenglish"));
		m_PropNames.AddString(_T("tsvn:projectlanguage"));
	}

	for (int i=0; i<m_PropNames.GetCount(); ++i)
	{
		CString sText;
		m_PropNames.GetLBText(i, sText);
		if (m_sPropName.Compare(sText)==0)
		{
			m_PropNames.SetCurSel(i);
			break;
		}
	}

	UpdateData(FALSE);
	CheckRecursive();

	AddAnchor(IDC_PROPNAME, TOP_LEFT, TOP_CENTER);
	AddAnchor(IDC_PROPNAMECOMBO, TOP_CENTER, TOP_RIGHT);
	AddAnchor(IDC_PROPVALUEGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROPVALUE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROPRECURSIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CEditPropertyValueDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CEditPropertyValueDlg::OnCancel()
{
	m_sPropName.Empty();
	CDialog::OnCancel();
}

void CEditPropertyValueDlg::OnOK()
{
	UpdateData();
	m_PropNames.GetWindowText(m_sPropName);
	CDialog::OnOK();
}

void CEditPropertyValueDlg::CheckRecursive()
{
	// some properties can only be applied to files
	// if the properties are edited for a folder or
	// multiple items, then such properties must be
	// applied recursively.
	// Here, we check the property the user selected
	// and check the "recursive" checkbox automatically
	// if it needs to be set.
	int idx = m_PropNames.GetCurSel();
	if (idx >= 0)
	{
		CString sName;
		m_PropNames.GetLBText(idx, sName);
		if ((m_bFolder)||(m_bMultiple))
		{
			// folder or multiple, now check for file-only props
			if (sName.Compare(_T("svn:eol-style"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:executable"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:keywords"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:needs-lock"))==0)
				m_bRecursive = TRUE;
			if (sName.Compare(_T("svn:mime-type"))==0)
				m_bRecursive = TRUE;
		}
	}
}

BOOL CEditPropertyValueDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					PostMessage(WM_COMMAND, IDOK);
				}
			}
			break;
		default:
			break;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}
