// NewProperty.cpp : implementation file
//

#include "stdafx.h"

// CNewPropertyPage dialog

IMPLEMENT_DYNAMIC(CNewPropertyPage,  CPropertyPage)
CNewPropertyPage::CNewPropertyPage()
{
}

CNewPropertyPage::~CNewPropertyPage()
{
}

CNewPropertyPage::CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption, DWORD dwSize)
{
  free(m_pPSP);
  m_pPSP=NULL;

	ASSERT(nIDTemplate != 0);
	AllocPSP(dwSize);
	CommonConstruct(MAKEINTRESOURCE(nIDTemplate), nIDCaption);
}

CNewPropertyPage::CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption , DWORD dwSize)
{
  free(m_pPSP);
  m_pPSP=NULL;

	ASSERT(AfxIsValidString(lpszTemplateName));
	AllocPSP(dwSize);
	CommonConstruct(lpszTemplateName, nIDCaption);
}

// extended construction
CNewPropertyPage::CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption,UINT nIDHeaderTitle, UINT nIDHeaderSubTitle, DWORD dwSize)
{
  free(m_pPSP);
  m_pPSP=NULL;

	ASSERT(nIDTemplate != 0);
	AllocPSP(dwSize);
	CommonConstruct(MAKEINTRESOURCE(nIDTemplate), nIDCaption, nIDHeaderTitle, nIDHeaderSubTitle);
}

CNewPropertyPage::CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption, 	UINT nIDHeaderTitle, UINT nIDHeaderSubTitle, DWORD dwSize)
{
  free(m_pPSP);
  m_pPSP=NULL;

	ASSERT(AfxIsValidString(lpszTemplateName));
	AllocPSP(dwSize);
	CommonConstruct(lpszTemplateName, nIDCaption, nIDHeaderTitle, nIDHeaderSubTitle);
}


BEGIN_MESSAGE_MAP(CNewPropertyPage,  CNewFrame<CPropertyPage>)
END_MESSAGE_MAP()

// CNewPropertyPage dialog

IMPLEMENT_DYNAMIC(CNewPropertySheet,  CPropertySheet)
CNewPropertySheet::CNewPropertySheet()
{
}

CNewPropertySheet::~CNewPropertySheet()
{
}

CNewPropertySheet::CNewPropertySheet(UINT nIDCaption, CWnd* pParentWnd,	UINT iSelectPage)
{
	ASSERT(nIDCaption != 0);

	VERIFY(m_strCaption.LoadString(nIDCaption));
	CommonConstruct(pParentWnd, iSelectPage);
}

CNewPropertySheet::CNewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
{
	ASSERT(pszCaption != NULL);

	m_strCaption = pszCaption;
	CommonConstruct(pParentWnd, iSelectPage);
}

// extended construction
CNewPropertySheet::CNewPropertySheet(UINT nIDCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark, HBITMAP hbmHeader)
{
	ASSERT(nIDCaption != 0);

	VERIFY(m_strCaption.LoadString(nIDCaption));
	CommonConstruct(pParentWnd, iSelectPage);
}

CNewPropertySheet::CNewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark , HBITMAP hbmHeader)
{
	ASSERT(pszCaption != NULL);

	m_strCaption = pszCaption;
	CommonConstruct(pParentWnd, iSelectPage);
}

BEGIN_MESSAGE_MAP(CNewPropertySheet,  CNewFrame<CPropertySheet>)
END_MESSAGE_MAP()



// CNewPropertyPage message handlers
