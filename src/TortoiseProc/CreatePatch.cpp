// CreatePatch.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "MessageBox.h"
#include "CreatePatch.h"
#include ".\createpatch.h"


// CCreatePatch dialog

IMPLEMENT_DYNAMIC(CCreatePatch, CResizableStandAloneDialog)
CCreatePatch::CCreatePatch(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCreatePatch::IDD, pParent),
	m_bThreadRunning(false)
{
}

CCreatePatch::~CCreatePatch()
{
}

void CCreatePatch::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PATCHLIST, m_PatchList);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CCreatePatch, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


// CCreatePatch message handlers

BOOL CCreatePatch::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	//set the listcontrol to support checkboxes
	m_PatchList.Init(0, SVNSLC_POPALL ^ (SVNSLC_POPIGNORE));
	m_PatchList.SetSelectButton(&m_SelectAll);

	AddAnchor(IDC_PATCHLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CreatePatchDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	if(AfxBeginThread(PatchThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	m_bThreadRunning = TRUE;

	return TRUE;
}

UINT CCreatePatch::PatchThreadEntry(LPVOID pVoid)
{
	return ((CCreatePatch*)pVoid)->PatchThread();
}
UINT CCreatePatch::PatchThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	GetDlgItem(IDOK)->EnableWindow(false);
	GetDlgItem(IDCANCEL)->EnableWindow(false);

	m_PatchList.GetStatus(m_pathList);
	m_PatchList.Show(SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS, 
						SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS);

	GetDlgItem(IDOK)->EnableWindow(true);
	GetDlgItem(IDCANCEL)->EnableWindow(true);
	if (m_PatchList.GetItemCount()==0)
	{
		CMessageBox::Show(m_hWnd, IDS_ERR_NOTHINGTOADD, IDS_APPNAME, MB_ICONINFORMATION);
		GetDlgItem(IDCANCEL)->EnableWindow(true);
		m_bThreadRunning = FALSE;
		EndDialog(0);
		return (DWORD)-1;
	}
	m_bThreadRunning = FALSE;
	return 0;
}

BOOL CCreatePatch::PreTranslateMessage(MSG* pMsg)
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
					return TRUE;
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCreatePatch::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	theApp.DoWaitCursor(1);
	m_PatchList.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

void CCreatePatch::OnBnClickedHelp()
{
	OnHelp();
}

void CCreatePatch::OnCancel()
{
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CCreatePatch::OnOK()
{
	if (m_bThreadRunning)
		return;

	//save only the files the user has selected into the pathlist
	m_PatchList.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}
