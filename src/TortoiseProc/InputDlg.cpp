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
#include "InputDlg.h"
#include "Registry.h"


// CInputDlg dialog

IMPLEMENT_DYNAMIC(CInputDlg, CResizableStandAloneDialog)
CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CInputDlg::IDD, pParent)
	, m_sInputText(_T(""))
	, m_pProjectProperties(NULL)
{
}

CInputDlg::~CInputDlg()
{
}

void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INPUTTEXT, m_cInput);
}


BEGIN_MESSAGE_MAP(CInputDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()


// CInputDlg message handlers

BOOL CInputDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	if (m_pProjectProperties)
		m_cInput.Init(m_pProjectProperties->lProjectLanguage);
	else
		m_cInput.Init();
	m_cInput.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));

	if (m_pProjectProperties)
	{
		if (m_pProjectProperties->nLogWidthMarker)
		{
			m_cInput.Call(SCI_SETWRAPMODE, SC_WRAP_NONE);
			m_cInput.Call(SCI_SETEDGEMODE, EDGE_LINE);
			m_cInput.Call(SCI_SETEDGECOLUMN, m_pProjectProperties->nLogWidthMarker);
		}
		else
		{
			m_cInput.Call(SCI_SETEDGEMODE, EDGE_NONE);
			m_cInput.Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
		}
		m_cInput.SetText(m_pProjectProperties->sLogTemplate);
	}

	if (!m_sHintText.IsEmpty())
	{
		GetDlgItem(IDC_HINTTEXT)->SetWindowText(m_sHintText);
	}
	if (!m_sTitle.IsEmpty())
	{
		this->SetWindowText(m_sTitle);
	}

	AddAnchor(IDC_HINTTEXT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_INPUTTEXT, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(_T("InputDlg"));
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInputDlg::OnOK()
{
	m_sInputText = m_cInput.GetText();
	CResizableDialog::OnOK();
}

BOOL CInputDlg::PreTranslateMessage(MSG* pMsg)
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
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
