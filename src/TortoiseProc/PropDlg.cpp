// PropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "MessageBox.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include ".\propdlg.h"


// CPropDlg dialog

IMPLEMENT_DYNAMIC(CPropDlg, CResizableDialog)
CPropDlg::CPropDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CPropDlg::IDD, pParent)
	, m_rev(SVNRev::REV_WC)
{
}

CPropDlg::~CPropDlg()
{
}

void CPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROPERTYLIST, m_proplist);
}


BEGIN_MESSAGE_MAP(CPropDlg, CResizableDialog)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CPropDlg message handlers

BOOL CPropDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	m_proplist.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );

	m_proplist.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_proplist.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_proplist.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROPPROPERTY);
	m_proplist.InsertColumn(0, temp);
	temp.LoadString(IDS_PROPVALUE);
	m_proplist.InsertColumn(1, temp);
	m_proplist.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_proplist.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_proplist.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_proplist.SetRedraw(false);

	GetDlgItem(IDOK)->EnableWindow(FALSE);
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &PropThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	AddAnchor(IDC_PROPERTYLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_CENTER);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPropDlg::OnCancel()
{
	if (GetDlgItem(IDOK)->IsWindowEnabled())
		CResizableDialog::OnCancel();
}

void CPropDlg::OnOK()
{
	if (GetDlgItem(IDOK)->IsWindowEnabled())
		CResizableDialog::OnOK();
}

DWORD WINAPI PropThread(LPVOID pVoid)
{
	CPropDlg*	pDlg;
	pDlg = (CPropDlg*)pVoid;
	SVNProperties props(pDlg->m_sPath, pDlg->m_rev);
	pDlg->m_proplist.SetRedraw(false);
	for (int i=0; i<props.GetCount(); ++i)
	{
		CString name = props.GetItemName(i).c_str();
		CString val = CUnicodeUtils::GetUnicode((char *)props.GetItemValue(i).c_str());
		val.Replace('\n', ' ');
		val.Replace('\r', ' ');
		pDlg->m_proplist.InsertItem(i, name);
		pDlg->m_proplist.SetItemText(i, 1, val);
	}
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(pDlg->m_proplist.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		pDlg->m_proplist.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	pDlg->m_proplist.SetRedraw(true);
	pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	return 0;
}

BOOL CPropDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ((GetDlgItem(IDOK)->IsWindowEnabled())||(pWnd != GetDlgItem(IDC_PROPERTYLIST)))
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
		SetCursor(hCur);
		return CResizableDialog::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	SetCursor(hCur);
	return TRUE;
}
