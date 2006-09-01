// FindDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseMerge.h"
#include "FindDlg.h"


// CFindDlg dialog

IMPLEMENT_DYNAMIC(CFindDlg, CDialog)

CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFindDlg::IDD, pParent)
	, m_bTerminating(false)
	, m_bFindNext(false)
	, m_bMatchCase(FALSE)
	, m_bLimitToDiffs(FALSE)
	, m_bWholeWord(FALSE)
	, m_sFindText(_T(""))
{
}

CFindDlg::~CFindDlg()
{
}

void CFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MATCHCASE, m_bMatchCase);
	DDX_Check(pDX, IDC_LIMITTODIFFS, m_bLimitToDiffs);
	DDX_Check(pDX, IDC_WHOLEWORD, m_bWholeWord);
	DDX_Text(pDX, IDC_SEARCHTEXT, m_sFindText);
}


BEGIN_MESSAGE_MAP(CFindDlg, CDialog)
	ON_EN_CHANGE(IDC_SEARCHTEXT, &CFindDlg::OnEnChangeSearchtext)
END_MESSAGE_MAP()


// CFindDlg message handlers

void CFindDlg::OnCancel()
{
	m_bTerminating = true;
	if (GetParent())
		GetParent()->SendMessage(m_FindMsg);
	DestroyWindow();
}

void CFindDlg::PostNcDestroy()
{
	delete this;
}

void CFindDlg::OnOK()
{
	UpdateData();
	if (m_sFindText.IsEmpty())
		return;
	m_bFindNext = true;
	if (GetParent())
		GetParent()->SendMessage(m_FindMsg);
	m_bFindNext = false;
}

BOOL CFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_FindMsg = RegisterWindowMessage(FINDMSGSTRING);

	GetDlgItem(IDC_SEARCHTEXT)->SetFocus();
	return FALSE;
}

void CFindDlg::OnEnChangeSearchtext()
{
	UpdateData();
	GetDlgItem(IDOK)->EnableWindow(!m_sFindText.IsEmpty());
}
