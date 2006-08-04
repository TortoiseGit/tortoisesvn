//------------------------------------------------------------------------------
// File    : NewStatusBar.h 
// Version : 1.02
// Date    : 21. January 2005
// Author  : Manfred Drasch
// Email   : ManfredJD@epost.de 
//           (Podetti@gmx.net)
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewToolBar
//
// You are free to use/modify this code. This class is public domain so you are
// free to use it any of your applications (Freeware, Shareware, Commercial).
// A (very) simple StatusBar to use with Bruno Podetti's CNewMenu.
// Feel free to make it better, but please send me a copy.
//------------------------------------------------------------------------------


#ifndef __CNewStatusBar_H_
#define __CNewStatusBar_H_

#pragma once


// CNewStatusBar

class GUILIBDLLEXPORT CNewStatusBar : public CStatusBar
{
  DECLARE_DYNAMIC(CNewStatusBar)

public:
  CNewStatusBar();
  virtual ~CNewStatusBar();

protected:
  void PaintResizingGripper(CDC &dc, const CRect & rcGrip);
  void PaintDotGripper(CDC& dc, const CRect& rcGrip, COLORREF clrDotFace, COLORREF clrDotShadow = CLR_NONE);
  void PaintLineGripper(CDC& dc, const CRect& rcGrip);
  void Drawpanels(CDC *pDC);

  void GetItemRect(int nIndex, LPRECT lpRect) const;
  virtual void PreSubclassWindow();

protected:
  DECLARE_MESSAGE_MAP()
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnPaint();
};

#endif // __CNewStatusBar_H_
