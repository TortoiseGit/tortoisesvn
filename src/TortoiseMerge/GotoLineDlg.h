#pragma once
#include "afxwin.h"


// CGotoLineDlg dialog

class CGotoLineDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CGotoLineDlg)

public:
    CGotoLineDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CGotoLineDlg();

    int         GetLineNumber() const {return m_nLine;}
    void        SetLabel(const CString& label) { m_sLabel = label; }
    void        SetLimits(int low, int high) { m_nLow = low; m_nHigh = high; }

    // Dialog Data
    enum { IDD = IDD_GOTO };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    DECLARE_MESSAGE_MAP()
private:
    int         m_nLine;
    int         m_nLow;
    int         m_nHigh;
    CString     m_sLabel;
    CEdit       m_cNumber;
};
