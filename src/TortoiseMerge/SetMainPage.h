#pragma once
#include "resource.h"
#include "registry.h"
#include "afxwin.h"

// CSetMainPage dialog

class CSetMainPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetMainPage)

public:
	CSetMainPage();
	virtual ~CSetMainPage();

	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	enum { IDD = IDD_SETMAINPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	DWORD			m_dwLanguage;
	CRegDWORD		m_regLanguage;
	BOOL			m_bBackup;
	CRegDWORD		m_regBackup;
	BOOL			m_bFirstDiffOnLoad;
	CRegDWORD		m_regFirstDiffOnLoad;
	int				m_nTabSize;
	CRegDWORD		m_regTabSize;
	BOOL			m_bIgnoreEOL;
	CRegDWORD		m_regIgnoreEOL;
	BOOL			m_bOnePane;
	CRegDWORD		m_regOnePane;
	DWORD			m_nIgnoreWS;
	CRegDWORD		m_regIgnoreWS;
	
	CComboBox		m_LanguageCombo;
public:
	afx_msg void OnBnClickedBackup();
	afx_msg void OnBnClickedIgnorelf();
	afx_msg void OnBnClickedOnepane();
	afx_msg void OnBnClickedFirstdiffonload();
	afx_msg void OnBnClickedWscompare();
	afx_msg void OnBnClickedWsignoreleading();
	afx_msg void OnBnClickedWsignoreall();
	afx_msg void OnEnChangeTabsize();
};
