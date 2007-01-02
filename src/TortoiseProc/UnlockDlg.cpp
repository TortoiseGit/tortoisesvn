// UnlockDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "UnlockDlg.h"
#include "messagebox.h"


// CUnlockDlg dialog

IMPLEMENT_DYNAMIC(CUnlockDlg, CResizableStandAloneDialog)

CUnlockDlg::CUnlockDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CUnlockDlg::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{

}

CUnlockDlg::~CUnlockDlg()
{
}

void CUnlockDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UNLOCKLIST, m_unlockListCtrl);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CUnlockDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
END_MESSAGE_MAP()


BOOL CUnlockDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// initialize the svn status list control
	m_unlockListCtrl.Init(0, _T("UnlockDlg"), SVNSLC_POPALL);
	m_unlockListCtrl.SetIgnoreRemoveOnly();	// when ignoring, don't add the parent folder since we're in the unlock dialog
	m_unlockListCtrl.SetSelectButton(&m_SelectAll);
	m_unlockListCtrl.SetConfirmButton((CButton*)GetDlgItem(IDOK));
	m_unlockListCtrl.SetEmptyString(IDS_ERR_NOTHINGTOUNLOCK);
	m_unlockListCtrl.SetCancelBool(&m_bCancelled);

	AddAnchor(IDC_UNLOCKLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("UnlockDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	if(AfxBeginThread(UnlockThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;
}

void CUnlockDlg::OnOK()
{
	if (m_bThreadRunning)
		return;

	// save only the files the user has selected into the pathlist
	m_unlockListCtrl.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

void CUnlockDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CUnlockDlg::OnBnClickedSelectall()
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
	m_unlockListCtrl.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

UINT CUnlockDlg::UnlockThreadEntry(LPVOID pVoid)
{
	return ((CUnlockDlg*)pVoid)->UnlockThread();
}

UINT CUnlockDlg::UnlockThread()
{
	// get the status of all selected file/folders recursively
	// and show the ones which the user can add (i.e. the unversioned ones)
	DialogEnableWindow(IDOK, false);
	m_bCancelled = false;
	if (!m_unlockListCtrl.GetStatus(m_pathList))
	{
		CMessageBox::Show(m_hWnd, m_unlockListCtrl.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
	}
	m_unlockListCtrl.Show(SVNSLC_SHOWLOCKS | SVNSLC_SHOWDIRECTFILES, 
		SVNSLC_SHOWLOCKS | SVNSLC_SHOWDIRECTFILES);

	DialogEnableWindow(IDOK, true);
	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

void CUnlockDlg::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CUnlockDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
					return TRUE;
				}
			}
			break;
		case VK_F5:
			{
				if (!m_bThreadRunning)
				{
					if(AfxBeginThread(UnlockThreadEntry, this) == NULL)
					{
						CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
					}
					else
						InterlockedExchange(&m_bThreadRunning, TRUE);
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CUnlockDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if(AfxBeginThread(UnlockThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}
