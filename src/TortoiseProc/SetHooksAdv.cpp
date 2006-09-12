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
#include "SetHooksAdv.h"
#include "BrowseFolder.h"
#include "Balloon.h"


IMPLEMENT_DYNAMIC(CSetHooksAdv, CResizableStandAloneDialog)

CSetHooksAdv::CSetHooksAdv(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSetHooksAdv::IDD, pParent)
	, m_sPath(_T(""))
	, m_sCommandLine(_T(""))
	, m_bWait(FALSE)
	, m_bHide(FALSE)
{
}

CSetHooksAdv::~CSetHooksAdv()
{
}

void CSetHooksAdv::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_HOOKPATH, m_sPath);
	DDX_Text(pDX, IDC_HOOKCOMMANDLINE, m_sCommandLine);
	DDX_Check(pDX, IDC_WAITCHECK, m_bWait);
	DDX_Check(pDX, IDC_HIDECHECK, m_bHide);
	DDX_Control(pDX, IDC_HOOKTYPECOMBO, m_cHookTypeCombo);
}


BEGIN_MESSAGE_MAP(CSetHooksAdv, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_HOOKBROWSE, &CSetHooksAdv::OnBnClickedHookbrowse)
	ON_BN_CLICKED(IDC_HOOKCOMMANDBROWSE, &CSetHooksAdv::OnBnClickedHookcommandbrowse)
END_MESSAGE_MAP()

BOOL CSetHooksAdv::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// initialize the combo box with all the hook types we have
	int index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTCOMMIT)));
	m_cHookTypeCombo.SetItemData(index, start_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PRECOMMIT)));
	m_cHookTypeCombo.SetItemData(index, pre_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTCOMMIT)));
	m_cHookTypeCombo.SetItemData(index, post_commit_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_STARTUPDATE)));
	m_cHookTypeCombo.SetItemData(index, start_update_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_PREUPDATE)));
	m_cHookTypeCombo.SetItemData(index, pre_update_hook);
	index = m_cHookTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_HOOKTYPE_POSTUPDATE)));
	m_cHookTypeCombo.SetItemData(index, post_update_hook);
	// preselect the right hook type in the combobox
	for (int i=0; i<m_cHookTypeCombo.GetCount(); ++i)
	{
		hooktype ht = (hooktype)m_cHookTypeCombo.GetItemData(i);
		if (ht == key.htype)
		{
			CString str;
			m_cHookTypeCombo.GetLBText(i, str);
			m_cHookTypeCombo.SelectString(i, str);
			break;
		}
	}

	m_sPath = key.path.GetWinPathString();
	m_sCommandLine = cmd.commandline;
	m_bWait = cmd.bWait;
	m_bHide = !cmd.bShow;
	UpdateData(FALSE);

	AddAnchor(IDC_HOOKTYPELABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKTYPECOMBO, TOP_RIGHT);
	AddAnchor(IDC_HOOKWCPATHLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKPATH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKBROWSE, TOP_RIGHT);
	AddAnchor(IDC_HOOKCMLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKCOMMANDLINE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HOOKCOMMANDBROWSE, TOP_RIGHT);
	AddAnchor(IDC_WAITCHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HIDECHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	return TRUE;
}

void CSetHooksAdv::OnOK()
{
	UpdateData();
	int cursel = m_cHookTypeCombo.GetCurSel();
	key.htype = unknown_hook;
	if (cursel != CB_ERR)
	{
		key.htype = (hooktype)m_cHookTypeCombo.GetItemData(cursel);
		key.path = CTSVNPath(m_sPath);
		cmd.commandline = m_sCommandLine;
		cmd.bWait = !!m_bWait;
		cmd.bShow = !m_bHide;
	}
	if (key.htype == unknown_hook)
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_HOOKTYPECOMBO), IDS_ERR_NOHOOKTYPESPECIFIED, TRUE, IDI_EXCLAMATION);
		return;
	}
	if (key.path.IsEmpty())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_HOOKPATH), IDS_ERR_NOHOOKPATHSPECIFIED, TRUE, IDI_EXCLAMATION);
		return;
	}
	if (cmd.commandline.IsEmpty())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_HOOKCOMMANDLINE), IDS_ERR_NOHOOKCOMMANDPECIFIED, TRUE, IDI_EXCLAMATION);
		return;
	}
	CResizableStandAloneDialog::OnOK();
}

void CSetHooksAdv::OnBnClickedHookbrowse()
{
	CBrowseFolder browser;
	CString sPath;
	browser.SetInfo(CString(MAKEINTRESOURCE(IDS_SETTINGS_HOOKS_SELECTFOLDERPATH)));
	if (browser.Show(m_hWnd, sPath) == CBrowseFolder::OK)
	{
		m_sPath = sPath;
		UpdateData(FALSE);
	}
}

void CSetHooksAdv::OnBnClickedHookcommandbrowse()
{
	// browse for the hook script to call
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	CString temp;
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	temp.LoadString(IDS_SETTINGS_HOOKS_SELECTSCRIPTFILE);
	if (temp.IsEmpty())
		ofn.lpstrTitle = NULL;
	else
		ofn.lpstrTitle = temp;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;

	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn))
	{
		m_sCommandLine = szFile;
		UpdateData(FALSE);
	}
}
