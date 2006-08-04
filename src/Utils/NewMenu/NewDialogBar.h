//------------------------------------------------------------------------------
// File    : NewDialogBar.h
// Version : 1.01
// Date    : 16. Mai 2005
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewToolBar
//------------------------------------------------------------------------------
#ifndef __NewDialogBar_H_
#define __NewDialogBar_H_

#pragma once


// CNewDialogBar
class GUILIBDLLEXPORT CNewDialogBar : public CDialogBar
{
	DECLARE_DYNAMIC(CNewDialogBar)

public:
	CNewDialogBar();
	virtual ~CNewDialogBar();

  public:
  // virtual from base class 
	virtual void DoPaint(CDC* pDC);
  void DrawBorders(CDC* pDC, CRect& rect);
  void DrawGripper(CDC* pDC, const CRect& rect); 
  void PaintToolBarBackGnd(CDC* pDC);

//  void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle);

  // overwritten from baseclass not virtual
  void EraseNonClient();

protected:
  afx_msg void OnNcPaint();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
};

#endif //__NewDialogBar_H_