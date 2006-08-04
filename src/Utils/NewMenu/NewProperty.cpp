//------------------------------------------------------------------------------
// File    : NewProperty.cpp 
// Version : 1.10
// Date    : 6. February 2004
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu 
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewToolBar
//
// For bugreport please add following informations
// - The CNewPropertyPage version number (Example CNewPropertyPage 1.10)
// - Operating system Win95 / WinXP and language (English / Japanese / German etc.)
// - Intalled service packs
// - Version of internet explorer (important for CNewToolBar)
// - Short description how to reproduce the bug
// - Pictures/Sample are welcome too
// - You can write in English or German to the above email-address.
// - Have my compiled examples the same effect?
//------------------------------------------------------------------------------
//
// You are free to use/modify this code but leave this header intact.
// This class is public domain so you are free to use it any of your 
// applications (Freeware, Shareware, Commercial). 
// All I ask is that you let me know so that if you have a real winner I can
// brag to my buddies that some of my code is in your app. I also wouldn't 
// mind if you sent me a copy of your application since I like to play with
// new stuff.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "NewMenu.h"
#include "NewToolBar.h"

// CNewPropertyPage dialog

IMPLEMENT_DYNAMIC(CNewPropertyPage,  CPropertyPage)
CNewPropertyPage::CNewPropertyPage()
{
}

CNewPropertyPage::~CNewPropertyPage()
{
}

#if _MFC_VER < 0x0700 
CNewPropertyPage::CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption)
{
  ASSERT(nIDTemplate != 0);
  CommonConstruct(MAKEINTRESOURCE(nIDTemplate), nIDCaption);
}

CNewPropertyPage::CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption)
{
  ASSERT(AfxIsValidString(lpszTemplateName));
  CommonConstruct(lpszTemplateName, nIDCaption);
}

#else

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
#endif

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

#if _MFC_VER < 0x0700
#else
// extended construction
CNewPropertySheet::CNewPropertySheet(UINT nIDCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark, HBITMAP hbmHeader)
{
  ASSERT(nIDCaption != 0);
  
  VERIFY(m_strCaption.LoadString(nIDCaption));
	CommonConstruct(pParentWnd, iSelectPage, hbmWatermark, hpalWatermark, hbmHeader);
}

CNewPropertySheet::CNewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark , HBITMAP hbmHeader)
{
  ASSERT(pszCaption != NULL);
  
  m_strCaption = pszCaption;
	CommonConstruct(pParentWnd, iSelectPage, hbmWatermark, hpalWatermark, hbmHeader);
}
#endif

BEGIN_MESSAGE_MAP(CNewPropertySheet,  CNewFrame<CPropertySheet>)
END_MESSAGE_MAP()

BOOL CNewPropertySheet::OnInitDialog()
{
  BOOL bRetval = CPropertySheet::OnInitDialog();

  HMENU hMenu = m_SystemNewMenu.Detach();
  HMENU hSysMenu = ::GetSystemMenu(m_hWnd,FALSE);
  if(hMenu!=hSysMenu)
  {
    if(IsMenu(hMenu))
    {
      ::DestroyMenu(hMenu);
    }
  }
  m_SystemNewMenu.Attach(hSysMenu);
  m_DefaultNewMenu.LoadMenu(::GetMenu(m_hWnd));

  if(IsMenu(m_DefaultNewMenu.m_hMenu))
  {
    UpdateMenuBarColor(m_DefaultNewMenu);
  }

  return bRetval;
}
// CNewPropertyPage message handlers
