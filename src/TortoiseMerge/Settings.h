#pragma once

#include "SetMainPage.h"

/**
 * \ingroup TortoiseMerge
 * This is the container for all settings pages. A setting page is
 * a class derived from CPropertyPage with an additional method called
 * SaveData(). The SaveData() method is used by the dialog to save
 * the settings the user has made - if that method is not called then
 * it means that the changes are discarded! Each settings page has
 * to make sure that no changes are saved outside that method.
 *
 */
class CSettings : public CPropertySheet
{
	DECLARE_DYNAMIC(CSettings)
private:
	/**
	 * Adds all pages to this Settings-Dialog.
	 */
	void AddPropPages();
	/**
	 * Removes the pages and frees up memory.
	 */
	void RemovePropPages();

private:
	CSetMainPage *		m_pMainPage;

public:
	CSettings(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CSettings(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CSettings();

	/**
	 * Calls the SaveData()-methods of each of the settings pages.
	 */
	void SaveData();
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};


