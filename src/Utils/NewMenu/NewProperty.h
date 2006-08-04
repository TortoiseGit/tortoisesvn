//------------------------------------------------------------------------------
// File    : NewProperty.h 
// Version : 1.10
// Date    : 6. February 2004
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu 
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewToolBar
//
// You are free to use/modify this code but leave this header intact.
// This class is public domain so you are free to use it any of your 
// applications (Freeware, Shareware, Commercial). 
// All I ask is that you let me know so that if you have a real winner I can
// brag to my buddies that some of my code is in your app. I also wouldn't 
// mind if you sent me a copy of your application since I like to play with
// new stuff.
//------------------------------------------------------------------------------
#pragma once
#ifndef __CNewProperty_H_
#define __CNewProperty_H_


// CNewPropertyPage dialog

class GUILIBDLLEXPORT CNewPropertyPage : public CNewFrame<CPropertyPage>
{
  DECLARE_DYNAMIC(CNewPropertyPage)
    
public:
  CNewPropertyPage();
  virtual ~CNewPropertyPage();


#if _MFC_VER < 0x0700 
  CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);
  CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0);
#else  
  CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
  CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
  
  // extended construction
  CNewPropertyPage(UINT nIDTemplate, UINT nIDCaption,UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
  CNewPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption, 	UINT nIDHeaderTitle, UINT nIDHeaderSubTitle = 0, DWORD dwSize = sizeof(PROPSHEETPAGE));
#endif //_MFC_VER < 0x0700
  
protected:
  DECLARE_MESSAGE_MAP()
};

class GUILIBDLLEXPORT CNewPropertySheet : public CNewFrame<CPropertySheet>
{
  DECLARE_DYNAMIC(CNewPropertySheet)
    
public:
  CNewPropertySheet();
  virtual ~CNewPropertySheet();
  
  CNewPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
  CNewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
  
#if _MFC_VER < 0x0700
#else
  // extended construction
  CNewPropertySheet(UINT nIDCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark = NULL, HBITMAP hbmHeader = NULL);
  CNewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd,	UINT iSelectPage, HBITMAP hbmWatermark,	HPALETTE hpalWatermark = NULL, HBITMAP hbmHeader = NULL);
#endif //_MFC_VER < 0x0700

// Implementation
public:
    // Overridables (special message map entries)
  virtual BOOL OnInitDialog();

  // Jan-18-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
  // added these calls to return the correct pointer, assuming all pages are derived from CNewPropertyPage
  CNewPropertyPage* GetActivePage() const				{ return ((CNewPropertyPage *) CPropertySheet::GetActivePage()); }
  CNewPropertyPage* GetPage(int nPage) const			{ return ((CNewPropertyPage *) CPropertySheet::GetPage(nPage)); }

protected:
  DECLARE_MESSAGE_MAP()
};

#endif //__CNewProperty_H_
