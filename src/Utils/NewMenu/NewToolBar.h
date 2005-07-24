//------------------------------------------------------------------------------
// File    : NewToolBar.h 
// Version : 1.11
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
#ifndef __CNewToolBar_H_
#define __CNewToolBar_H_

#pragma once 

class CNewToolBar : public CToolBar
{
  DECLARE_DYNAMIC(CNewToolBar)

public:
  CNewToolBar();
  virtual ~CNewToolBar();

public:
  // take the first pixel top/left for the tranparent-color,
  // when image has more than 16 colors
  BOOL LoadToolBar(LPCTSTR lpszResourceName);
  BOOL LoadToolBar(UINT nIDResource);

  // For replacing the toolbar with a high-color image
  BOOL LoadHiColor(LPCTSTR lpszResourceName,COLORREF transparentColor=CLR_DEFAULT);

  bool InsertControl (int nIndex, CWnd* pControl, DWORD_PTR dwData=NULL);
  
  bool SetMenuButton (int nIndex);
  bool SetMenuButtonID(UINT nCommandID);

  void TrackPopupMenu (UINT nID, CMenu* pMenu);

public:
  // virtual
  CSize CalcDynamicLayout (int nLength, DWORD dwMode);
  LRESULT DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

public:
  // virtual from base class 
  void DrawBorders(CDC* pDC, CRect& rect);
  void DrawGripper(CDC* pDC, const CRect& rect); 

  void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle);

  // overwritten from baseclass not virtual
  void EraseNonClient();

protected:

  afx_msg void OnNcPaint();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult); 
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnPaint();

  DECLARE_MESSAGE_MAP()

protected:
  void PaintToolBarBackGnd(CDC* pDC);
  void PaintCorner(CDC *pDC, LPCRECT pRect, COLORREF color);
  void PaintOrangeState(CDC *pDC, CRect rc, bool bHot);

  void PaintTBButton(LPNMTBCUSTOMDRAW pInfo);
  BOOL PaintHotButton(LPNMTBCUSTOMDRAW lpNMCustomDraw); 

private:
  CImageList m_ImageList;
  CImageList m_ImageListDisabled;
  CMenu* m_pCustomizeMenu;
  DWORD m_DoCheck;
};

#endif //__CNewToolBar_H_

