#pragma once
#include "afxwin.h"
#include "DiffData.h"
#include "ColourPickerXP.h"

// CSetColorPage dialog

class CSetColorPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetColorPage)

public:
	CSetColorPage();
	virtual ~CSetColorPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

// Dialog Data
	enum { IDD = IDD_SETCOLORPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	afx_msg LONG OnSelEndOK(UINT lParam, LONG wParam);
	DECLARE_MESSAGE_MAP()

protected:
	BOOL m_bInit;
	CColourPickerXP m_cBkUnknown;
	CColourPickerXP m_cFgUnknown;
	CColourPickerXP m_cBkNormal;
	CColourPickerXP m_cFgNormal;
	CColourPickerXP m_cBkRemoved;
	CColourPickerXP m_cBkWhitespaceRemoved;
	CColourPickerXP m_cBkAdded;
	CColourPickerXP m_cBkWhitespaceAdded;
	CColourPickerXP m_cFgRemoved;
	CColourPickerXP m_cFgWhitespaceRemoved;
	CColourPickerXP m_cFgAdded;
	CColourPickerXP m_cFgWhitespaceAdded;
};
