//------------------------------------------------------------------------------
// File    : NewStatusBar.cpp
// Version : 1.03
// Date    : 21. January 2005
// Author  : Manfred Drasch
// Email   : ManfredJD@epost.de
//           (Podetti@gmx.net)
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewMenuBar
//
// For bugreport please add following informations
// - The CNewStatusBar version number (Example CNewStatusBar 1.00)
// - Operating system Win95 / WinXP and language (English / Japanese / German etc.)
// - Installed service packs
// - Version of internet explorer (important for CNewToolBar)
// - Short description how to reproduce the bug
// - Pictures/Sample are welcome too
// - You can write in English or German to the above email-address.
// - Have my compiled examples the same effect?
//------------------------------------------------------------------------------
//
// You are free to use/modify this code. This class is public domain so you are
// free to use it any of your applications (Freeware, Shareware, Commercial).
// A (very) simple StatusBar to use with Bruno Podetti's CNewMenu.
// Feel free to make it better, but please send me a copy.
//------------------------------------------------------------------------------
//
// Special
// thanks to Manfred Drasch for submitting this new statusbar
//
// 9. March 2005, 1.03 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// corrected a bug after the GetIconInfo() call in Drawpanels() where I neglected to call DeleteObject() for the bitmap mask
//
// 21. January 2005, 1.02 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// added support for icons in the panes of the status bar
//
// 27. June 2004, 1.01
// first release
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "NewMenu.h"
#include "NewStatusBar.h"

// CNewStatusBar

IMPLEMENT_DYNAMIC(CNewStatusBar, CStatusBar)
CNewStatusBar::CNewStatusBar()
{
}

CNewStatusBar::~CNewStatusBar()
{
}

BEGIN_MESSAGE_MAP(CNewStatusBar, CStatusBar)
  ON_WM_ERASEBKGND()
  ON_WM_PAINT()
END_MESSAGE_MAP()


// CNewStatusBar-Meldungshandler

void CNewStatusBar::PreSubclassWindow()
{
  ModifyStyle(0,BS_OWNERDRAW);
  CStatusBar::PreSubclassWindow();
}

BOOL CNewStatusBar::OnEraseBkgnd(CDC* pDC)
{
  pDC;
  return TRUE;
}
//////////////////////////////////////////////////////////////////////////

void CNewStatusBar::OnPaint()
{
  CPaintDC dc(this); // device context for painting

  CRect rcGrip( 0, 0, 0, 0 );
  CWnd* pWndTLP = GetTopLevelParent();
  if(pWndTLP != NULL )
  {
    WINDOWPLACEMENT wpm;
    ::memset( (void*)&wpm, 0, sizeof(WINDOWPLACEMENT) );
    wpm.length = sizeof(WINDOWPLACEMENT);
    pWndTLP->GetWindowPlacement(&wpm );
    if(wpm.showCmd != SW_SHOWMAXIMIZED)
    {
      GetClientRect(&rcGrip);
      rcGrip.left = rcGrip.right - ::GetSystemMetrics(SM_CXVSCROLL) - 2;
    }
  }
  else
  {
    CWnd::DefWindowProc( WM_PAINT, (WPARAM)dc.m_hDC, 0 );
    return;
  }

  CRect rcClient;
  CPoint brushOrg(0,0);
  GetClientRect(&rcClient);

  CBrush*pBrush = GetMenuBarBrush();
  if(!pBrush)
  {
    COLORREF crFillColor = CNewMenu::GetMenuBarColor();
    dc.FillSolidRect(rcClient, crFillColor);
    Drawpanels(&dc);

    if(!rcGrip.IsRectEmpty())
    {
      dc.FillSolidRect(rcGrip, crFillColor);

      rcGrip.left -= 2;
      rcGrip.OffsetRect( -1, -2 );
      PaintResizingGripper(dc, rcGrip);
    }
  }
  else
  {
    brushOrg = rcClient.TopLeft();

    // need for win95/98/me
    VERIFY(pBrush->UnrealizeObject());
    CPoint oldOrg = dc.SetBrushOrg(brushOrg);

    dc.FillRect(rcClient,pBrush);
    Drawpanels(&dc);

    if(!rcGrip.IsRectEmpty())
    {
      dc.FillRect(rcGrip,pBrush);

      rcGrip.left -= 2;
      rcGrip.OffsetRect( -1, -2 );
      PaintResizingGripper(dc, rcGrip);
    }

    dc.SetBrushOrg(oldOrg);
  }
}

void CNewStatusBar::PaintDotGripper(CDC& dc, const CRect& rcGrip, COLORREF clrDotFace, COLORREF clrDotShadow /*= CLR_NONE*/)
{
  CRect rcDotFace;
  CRect rcDotShadow;
  CRect rcDotFaceSave(CPoint(rcGrip.right - 1, rcGrip.bottom), CSize(2, 2));
  rcDotFaceSave.OffsetRect(-2, -2);
  CRect rcDotShadowSave(rcDotFaceSave);
  rcDotFaceSave.OffsetRect(-1, -1);

  int nStepH = -4;
  int nStepV = -4;

  int nX = (rcGrip.Width() / 4);  // 4 = 2 point + 2 distance (included 1 shadow)
  int nY = (rcGrip.Height() / 4);
  int nMin = min(nX, nY);
  nMin--;

  int nStep = 0;
  while(nMin > 0)
  {
    rcDotFace = rcDotFaceSave;
    rcDotShadow = rcDotShadowSave;
    rcDotFace.OffsetRect( nStepH*nStep, 0 );
    rcDotShadow.OffsetRect( nStepH*nStep, 0 );
    for(int nDot = 0; nDot < nMin; nDot++)
    {
      if(clrDotShadow != CLR_NONE)
      {
        dc.FillSolidRect( &rcDotShadow, clrDotShadow );
      }
      dc.FillSolidRect( &rcDotFace, clrDotFace );
      rcDotFace.OffsetRect( 0, nStepV );
      rcDotShadow.OffsetRect( 0, nStepV );
    }
    nMin--;
    nStep++;
  }
}

void CNewStatusBar::PaintLineGripper(CDC& dc, const CRect& rcGrip)
{
  CPen pen1(PS_SOLID,0, GetSysColor(COLOR_BTNSHADOW));
  CPen pen2(PS_SOLID,0, GetSysColor(COLOR_BTNHIGHLIGHT));
  CPen* pOld = dc.SelectObject(&pen1);

  int nX = rcGrip.Width();
  int nY = rcGrip.Height();
  int nMin = min(nX, nY);

  nMin -= 4;
  int nOffset = 2;
  while(nOffset < nMin)
  {
    dc.SelectObject(&pen1);
    dc.MoveTo(rcGrip.right - nOffset, rcGrip.bottom);
    dc.LineTo(rcGrip.right, rcGrip.bottom - nOffset);
    nOffset++;
    dc.MoveTo(rcGrip.right - nOffset, rcGrip.bottom);
    dc.LineTo(rcGrip.right, rcGrip.bottom - nOffset);
    nOffset += 1;

    dc.SelectObject(&pen2);
    dc.MoveTo(rcGrip.right - nOffset, rcGrip.bottom);
    dc.LineTo(rcGrip.right, rcGrip.bottom - nOffset);
    nOffset += 2;
  }
  dc.SelectObject(pOld);
}

void CNewStatusBar::PaintResizingGripper(CDC& dc, const CRect& rcGrip)
{
  ASSERT( dc.GetSafeHdc() != NULL );

  // draw the gripper
  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
    PaintDotGripper(dc, rcGrip, DarkenColor(100,CNewMenu::GetMenuColor()), CLR_NONE);
    break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
    PaintDotGripper(dc, rcGrip, DarkenColor(10,GetSysColor(COLOR_ACTIVECAPTION)), RGB(255,255,255));
    break;

  default:
    PaintLineGripper(dc, rcGrip);
    break;
  }
}

void CNewStatusBar::Drawpanels(CDC* pDC)
{
  COLORREF color1, color2;

  ASSERT(pDC->GetSafeHdc() != NULL );

  // draw the panels
  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
    color1 = DarkenColor(100,CNewMenu::GetMenuColor());
    color2 = color1;
    break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
    color1 = DarkenColor(10,GetSysColor(COLOR_ACTIVECAPTION));
    color2 = RGB(255,255,255);
    break;

  default:
    color1 = GetSysColor(COLOR_BTNSHADOW);
    color2 = GetSysColor(COLOR_BTNHIGHLIGHT);
    break;
  }

  CRect rcItem;
  CString csPaneText;
  int OldMode=pDC->SetBkMode(TRANSPARENT);

  CFont fontBar;
  NONCLIENTMETRICS nm = {0};
  nm.cbSize = sizeof (NONCLIENTMETRICS);
  VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
  fontBar.CreateFontIndirect (&nm.lfMenuFont);
  CFont* pOldFont = pDC->SelectObject(&fontBar);

  for (int nIndex=0; nIndex < m_nCount; nIndex++)
  {
    GetItemRect(nIndex,&rcItem);
    csPaneText = CStatusBar::GetStatusBarCtrl().GetText(nIndex);

    // Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
    // added support for icons in the status bar
    // for some reason the top pixel is too low, move it up
    --rcItem.top;

    // create another rectangle based on the area of the pane
    CRect rcTextRect = rcItem;

    // check if the status bar has an icon first
    HICON hIcon = (HICON) ::SendMessage(CStatusBar::GetStatusBarCtrl().m_hWnd, SB_GETICON, nIndex, (LPARAM) 0);

    // if the status bar has an icon, draw it and adjust the text rectangle
    if (hIcon)
    {
      ICONINFO iconInfo = {0};

      // get the icon information
      if (GetIconInfo(hIcon, &iconInfo))
      {
        CBitmap Bitmap;
        BITMAP  bmBitmap;

        // create a bitmap
        Bitmap.Attach(iconInfo.hbmColor);

		// release the mask
		::DeleteObject(iconInfo.hbmMask);

        // get the bitmap information
        Bitmap.GetBitmap(&bmBitmap);

        // determine the icon size
        CSize imgSize(bmBitmap.bmWidth, bmBitmap.bmHeight);

        // determine where the icon should be drawn
        CPoint ptImage((rcItem.left + 2), (rcItem.top + 1));

        // draw the icon
        pDC->DrawState(ptImage, imgSize, hIcon, DSS_NORMAL, (HBRUSH) NULL);

        // move the text to the right the width of the icon
        rcTextRect.left += (bmBitmap.bmWidth + 1);
      } // if
    } // if

    // draw the text of the pane
    pDC->ExtTextOut((rcTextRect.left + 2), (rcTextRect.top + 2), ETO_CLIPPED, rcTextRect, csPaneText, NULL);
    if(0 != GetItemID(nIndex))
    {
      pDC->Draw3dRect(rcItem,color1,color2);
    }
  }

  pDC->SelectObject(pOldFont);
  pDC->SetBkMode(OldMode);
}

// Sometimes the Original CStatusBar::GetItemRect() returns invalid size of the last pane
// we try to make it better
void CNewStatusBar::GetItemRect(int nIndex, LPRECT lpRect) const
{
  ASSERT_VALID(this);
  ASSERT(::IsWindow(m_hWnd));

  CNewStatusBar* pBar = (CNewStatusBar*)this;
  if (!pBar->DefWindowProc(SB_GETRECT, nIndex, (LPARAM)lpRect))
  {
    ::SetRectEmpty(lpRect);
  }

  if(nIndex == (m_nCount-1))
  {
    if((GetPaneStyle(nIndex) & SBPS_STRETCH) == 0)
    {
      UINT nID, nStyle;
      int  cxWidth;
      GetPaneInfo(nIndex, nID, nStyle, cxWidth);
      lpRect->right = lpRect->left + cxWidth + (3 * ::GetSystemMetrics(SM_CXEDGE));
    }
    else
    {
      CRect rcClient;
      GetClientRect(&rcClient);
      lpRect->right = rcClient.right;
      if((GetStyle() & SBARS_SIZEGRIP) == SBARS_SIZEGRIP)
      {
        lpRect->right -= ::GetSystemMetrics(SM_CXSMICON) + ::GetSystemMetrics(SM_CXEDGE);
      }
    }
  }
}
