#pragma once


// CNewPropertyPage dialog

class CNewPropertyPage : public CNewFrame<CPropertyPage>
{
	DECLARE_DYNAMIC(CNewPropertyPage)

public:
	CNewPropertyPage();
	virtual ~CNewPropertyPage();

  CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
	CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));

	// extended construction
	CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption,UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
	CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption, 	UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));

protected:
	DECLARE_MESSAGE_MAP()
};

class CNewPropertySheet : public CNewFrame<CPropertySheet>
{
	DECLARE_DYNAMIC(CNewPropertySheet)

public:
	CNewPropertySheet();
	virtual ~CNewPropertySheet();

	CNewPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL,	UINT iSelectPage = 0);
	CNewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

	// extended construction
	CNewPropertySheet(UINT nIDCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark = NULL, HBITMAP hbmHeader = NULL);
	CNewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark = NULL, HBITMAP hbmHeader = NULL);

protected:
	DECLARE_MESSAGE_MAP()
};


