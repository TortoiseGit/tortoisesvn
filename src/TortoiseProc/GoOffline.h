#pragma once

#include "RepositoryInfo.h"

// CGoOffline dialog

class CGoOffline : public CDialog
{
    DECLARE_DYNAMIC(CGoOffline)

public:
    CGoOffline(CWnd* pParent = NULL);   // standard constructor
    virtual ~CGoOffline();

    void SetErrorMessage(const CString& errMsg);

    LogCache::ConnectionState selection;
    BOOL asDefault;
    BOOL doRetry;

// Dialog Data
    enum { IDD = IDD_GOOFFLINE };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedPermanentlyOffline();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedRetry();

    DECLARE_MESSAGE_MAP()
private:
    CString m_errMsg;
};
