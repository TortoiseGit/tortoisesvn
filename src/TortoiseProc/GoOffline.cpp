// GoOffline.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "GoOffline.h"


// CGoOffline dialog

IMPLEMENT_DYNAMIC(CGoOffline, CDialog)

CGoOffline::CGoOffline(CWnd* pParent /*=NULL*/)
    : CDialog(CGoOffline::IDD, pParent)
    , asDefault(false)
    , selection(LogCache::online)
    , doRetry(false)
{

}

CGoOffline::~CGoOffline()
{
}

void CGoOffline::SetErrorMessage(const CString& errMsg)
{
    m_errMsg = errMsg;
}

void CGoOffline::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_ASDEFAULTOFFLINE, asDefault);
}


BEGIN_MESSAGE_MAP(CGoOffline, CDialog)
    ON_BN_CLICKED(IDOK, &CGoOffline::OnBnClickedOk)
    ON_BN_CLICKED(IDC_PERMANENTLYOFFLINE, &CGoOffline::OnBnClickedPermanentlyOffline)
    ON_BN_CLICKED(IDCANCEL, &CGoOffline::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_RETRY, &CGoOffline::OnBnClickedRetry)
END_MESSAGE_MAP()


BOOL CGoOffline::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetDlgItemText(IDC_ERRORMSG, m_errMsg);

    return TRUE;
}

// CGoOffline message handlers

void CGoOffline::OnBnClickedOk()
{
    selection = LogCache::tempOffline;

    OnOK();
}

void CGoOffline::OnBnClickedPermanentlyOffline()
{
    selection = LogCache::offline;

    OnOK();
}

void CGoOffline::OnBnClickedCancel()
{
    selection = LogCache::online;

    OnCancel();
}


void CGoOffline::OnBnClickedRetry()
{
    doRetry = true;
    selection = LogCache::online;

    OnCancel();
}
