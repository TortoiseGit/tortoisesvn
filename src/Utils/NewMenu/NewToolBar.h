//------------------------------------------------------------------------------
// File    : NewToolBar.h 
// Version : 1.10
// Date    : 30. November 2003
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu 
//
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
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

protected:
  DECLARE_MESSAGE_MAP()

  void DrawBorders(CDC* pDC, CRect& rect);
  void DrawGripper(CDC* pDC, const CRect& rect); 
  void EraseNonClient();
  BOOL PaintHotButton(LPNMCUSTOMDRAW lpNMCustomDraw);

  afx_msg void OnNcPaint();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult); 

public:

  // take the first pixel top/left for the tranparent-color,
  // when image has more than 16 colors
  BOOL LoadToolBar(LPCTSTR lpszResourceName);
  BOOL LoadToolBar(UINT nIDResource);

  // For replacing the toolbar with a high-color image
  BOOL LoadHiColor(LPCTSTR lpszResourceName,COLORREF transparentColor=CLR_DEFAULT);
  
  COLORREF MakeGrayAlphablend(CBitmap* pBitmap, int weighting, COLORREF blendcolor);
  void PaintOrangeState(CDC *pDC, CRect rc, bool bHot);
  void PaintTBButton(LPNMCUSTOMDRAW pInfo);
  void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle);

private:
  CImageList m_ImageList;
  CImageList m_ImageListDisabled;

  void PaintToolBarBackGnd(CDC* pDC);
  void PaintCorner(CDC *pDC, LPCRECT pRect, COLORREF color);

public:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnPaint();
};

#endif //__CNewToolBar_H_

