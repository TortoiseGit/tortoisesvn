// GotoLineDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseMerge.h"
#include "GotoLineDlg.h"
#include "afxdialogex.h"


// CGotoLineDlg dialog

IMPLEMENT_DYNAMIC(CGotoLineDlg, CDialogEx)

CGotoLineDlg::CGotoLineDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CGotoLineDlg::IDD, pParent)
    , m_nLine(0)
    , m_nLow(-1)
    , m_nHigh(-1)
{

}

CGotoLineDlg::~CGotoLineDlg()
{
}

void CGotoLineDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_NUMBER, m_nLine);
    DDX_Control(pDX, IDC_NUMBER, m_cNumber);
}


BEGIN_MESSAGE_MAP(CGotoLineDlg, CDialogEx)
END_MESSAGE_MAP()




BOOL CGotoLineDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    if (!m_sLabel.IsEmpty())
    {
        SetDlgItemText(IDC_LINELABEL, m_sLabel);
    }
    SetDlgItemText(IDC_NUMBER, L"");
    GetDlgItem(IDC_NUMBER)->SetFocus();

    return FALSE;
}


void CGotoLineDlg::OnOK()
{
    UpdateData();
    if ((m_nLine < m_nLow)||(m_nLine > m_nHigh))
    {
        CString sError;
        sError.Format(IDS_GOTO_OUTOFRANGE, m_nLow, m_nHigh);
        m_cNumber.ShowBalloonTip(L"", sError);
        m_cNumber.SetSel(0, -1);
        return;
    }
    CDialogEx::OnOK();
}
