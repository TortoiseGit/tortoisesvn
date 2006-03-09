// RevisionRangeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Balloon.h"
#include "TortoiseProc.h"
#include "RevisionRangeDlg.h"


// CRevisionRangeDlg dialog

IMPLEMENT_DYNAMIC(CRevisionRangeDlg, CStandAloneDialog)

CRevisionRangeDlg::CRevisionRangeDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CRevisionRangeDlg::IDD, pParent)
	, m_bAllowWCRevs(true)
	, m_StartRev(_T("HEAD"))
	, m_EndRev(_T("HEAD"))
{

}

CRevisionRangeDlg::~CRevisionRangeDlg()
{
}

void CRevisionRangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVNUM, m_sStartRevision);
	DDX_Text(pDX, IDC_REVNUM2, m_sEndRevision);
}


BEGIN_MESSAGE_MAP(CRevisionRangeDlg, CStandAloneDialog)
	ON_EN_CHANGE(IDC_REVNUM, OnEnChangeRevnum)
	ON_EN_CHANGE(IDC_REVNUM2, OnEnChangeRevnum2)
END_MESSAGE_MAP()


// CRevisionRangeDlg message handlers

BOOL CRevisionRangeDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	if (m_StartRev.IsHead())
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
		CString sRev;
		if (m_StartRev.IsDate())
			sRev = m_StartRev.GetDateString();
		else
			sRev.Format(_T("%ld"), (LONG)(m_StartRev));
		GetDlgItem(IDC_REVNUM)->SetWindowText(sRev);
	}
	if (m_EndRev.IsHead())
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_NEWEST2);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_REVISION_N2);
		CString sRev;
		if (m_EndRev.IsDate())
			sRev = m_EndRev.GetDateString();
		else
			sRev.Format(_T("%ld"), (LONG)(m_EndRev));
		GetDlgItem(IDC_REVNUM2)->SetWindowText(sRev);
	}

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	GetDlgItem(IDC_REVNUM)->SetFocus();
	return FALSE;
}

void CRevisionRangeDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	m_StartRev = SVNRev(m_sStartRevision);
	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_NEWEST, IDC_REVISION_N) == IDC_NEWEST)
	{
		m_StartRev = SVNRev(_T("HEAD"));
		m_sStartRevision = _T("HEAD");
	}
	if ((!m_StartRev.IsValid())||((!m_bAllowWCRevs)&&(m_StartRev.IsPrev() || m_StartRev.IsCommitted() || m_StartRev.IsBase())))
	{
		CWnd* ctrl = GetDlgItem(IDC_REVNUM);
		CRect rt;
		ctrl->GetWindowRect(rt);
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_REVNUM), m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC, TRUE, IDI_EXCLAMATION);
		return;
	}

	m_EndRev = SVNRev(m_sEndRevision);
	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_NEWEST2, IDC_REVISION_N2) == IDC_NEWEST2)
	{
		m_EndRev = SVNRev(_T("HEAD"));
		m_sEndRevision = _T("HEAD");
	}
	if ((!m_EndRev.IsValid())||((!m_bAllowWCRevs)&&(m_EndRev.IsPrev() || m_EndRev.IsCommitted() || m_EndRev.IsBase())))
	{
		CWnd* ctrl = GetDlgItem(IDC_REVNUM2);
		CRect rt;
		ctrl->GetWindowRect(rt);
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_REVNUM2), m_bAllowWCRevs ? IDS_ERR_INVALIDREV : IDS_ERR_INVALIDREVNOWC, TRUE, IDI_EXCLAMATION);
		return;
	}

	UpdateData(FALSE);

	CStandAloneDialog::OnOK();
}

void CRevisionRangeDlg::OnEnChangeRevnum()
{
	CString sText;
	GetDlgItem(IDC_REVNUM)->GetWindowText(sText);
	if (sText.IsEmpty())
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
	}
}

void CRevisionRangeDlg::OnEnChangeRevnum2()
{
	CString sText;
	GetDlgItem(IDC_REVNUM2)->GetWindowText(sText);
	if (sText.IsEmpty())
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_NEWEST2);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST2, IDC_REVISION_N2, IDC_REVISION_N2);
	}
}
