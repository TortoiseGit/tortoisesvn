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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "InputDlg.h"
#include ".\inputdlg.h"


// CInputDlg dialog

IMPLEMENT_DYNAMIC(CInputDlg, CResizableDialog)
CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CInputDlg::IDD, pParent)
	, m_sInputText(_T(""))
{
}

CInputDlg::~CInputDlg()
{
}

void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_INPUTTEXT, m_sInputText);
	DDX_Control(pDX, IDC_INPUTTEXT, m_Input);
}


BEGIN_MESSAGE_MAP(CInputDlg, CResizableDialog)
END_MESSAGE_MAP()


// CInputDlg message handlers

BOOL CInputDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	LOGFONT LogFont;
	LogFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
	LogFont.lfWidth          = 0;
	LogFont.lfEscapement     = 0;
	LogFont.lfOrientation    = 0;
	LogFont.lfWeight         = 400;
	LogFont.lfItalic         = 0;
	LogFont.lfUnderline      = 0;
	LogFont.lfStrikeOut      = 0;
	LogFont.lfCharSet        = DEFAULT_CHARSET;
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy(LogFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	m_logFont.CreateFontIndirect(&LogFont);
	GetDlgItem(IDC_INPUTTEXT)->SetFont(&m_logFont);

	if (!m_sHintText.IsEmpty())
	{
		GetDlgItem(IDC_HINTTEXT)->SetWindowText(m_sHintText);
	}
	if (!m_sTitle.IsEmpty())
	{
		this->SetWindowText(m_sTitle);
	}
	if (!m_sInputText.IsEmpty())
	{
		GetDlgItem(IDC_INPUTTEXT)->SetWindowText(m_sInputText);
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
		case 'A':
			{
				if ((GetAsyncKeyState(VK_CONTROL)&0x8000)&&(!(GetAsyncKeyState(VK_MENU)&0x8000)))
				{
					// Ctrl-A pressed. Select all text in the CEdit control
					m_Input.SetSel(0, -1);
					return TRUE;
				}
			}
		}
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}
