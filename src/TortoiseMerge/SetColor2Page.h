#pragma once
#include "afxwin.h"
#include "DiffData.h"
#include "ColourPickerXP.h"


// CSetColor2Page dialog

class CSetColor2Page : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetColor2Page)

public:
	CSetColor2Page();
	virtual ~CSetColor2Page();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

// Dialog Data
	enum { IDD = IDD_SETCOLOR2PAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	afx_msg LONG OnSelEndOK(UINT lParam, LONG wParam);

	DECLARE_MESSAGE_MAP()
protected:
	BOOL m_bInit;

	CColourPickerXP m_cBkEmpty;
	CColourPickerXP m_cBkConflicted;
	CColourPickerXP m_cBkConflictedAdded;
	CColourPickerXP m_cBkConflictedEmpty;
	CColourPickerXP m_cBkIdentical;
	CColourPickerXP m_cBkIdenticalAdded;
	CColourPickerXP m_cBkIdenticalRemoved;
	CColourPickerXP m_cBkTheirsAdded;
	CColourPickerXP m_cBkTheirsRemoved;
	CColourPickerXP m_cBkYoursAdded;
	CColourPickerXP m_cBkYoursRemoved;

	CColourPickerXP m_cFgEmpty;
	CColourPickerXP m_cFgConflicted;
	CColourPickerXP m_cFgConflictedAdded;
	CColourPickerXP m_cFgConflictedEmpty;
	CColourPickerXP m_cFgIdentical;
	CColourPickerXP m_cFgIdenticalAdded;
	CColourPickerXP m_cFgIdenticalRemoved;
	CColourPickerXP m_cFgTheirsAdded;
	CColourPickerXP m_cFgTheirsRemoved;
	CColourPickerXP m_cFgYoursAdded;
	CColourPickerXP m_cFgYoursRemoved;
};
