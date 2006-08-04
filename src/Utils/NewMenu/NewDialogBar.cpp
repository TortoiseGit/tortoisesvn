//------------------------------------------------------------------------------
// File    : MyDialogBar.cpp
// Version : 1.01
// Date    : 16. Mai 2005
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewDialogBar
//
// For bugreport please add following informations
// - The CNewDialogBar version number (Example CNewDialogBar 1.11)
// - Operating system Win95 / WinXP and language (English / Japanese / German etc.)
// - Installed service packs
// - Version of internet explorer (important for CNewDialogBar)
// - Short description how to reproduce the bug
// - Pictures/Sample are welcome too
// - You can write in English or German to the above email-address.
// - Have my compiled examples the same effect?
//------------------------------------------------------------------------------
//
// 16. Mai 2005 1.01
// Changed drawing for gripper to CMenuTheme
// Corrected backgroundcolor in toolbar
//
// 29. January 2005 1.00
// First version
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
#include "NewDialogBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CX_GRIPPER  3
#define CY_GRIPPER  3
#define CX_BORDER_GRIPPER 2
#define CY_BORDER_GRIPPER 2


// CMyDialogBar

IMPLEMENT_DYNAMIC(CNewDialogBar, CDialogBar)
CNewDialogBar::CNewDialogBar()
{
}

CNewDialogBar::~CNewDialogBar()
{
}

BEGIN_MESSAGE_MAP(CNewDialogBar, CDialogBar)
  ON_WM_NCPAINT()
  ON_WM_ERASEBKGND()
  ON_WM_CREATE()
  ON_WM_PAINT()
END_MESSAGE_MAP()

// CNewDialogBar message handlers
void CNewDialogBar::DrawGripper(CDC* pDC, const CRect& rect)
{
  CNewMenu::GetActualMenuTheme()->DrawGripper(pDC,rect,m_dwStyle, 0, m_cxRightBorder, m_cyTopBorder, m_cyBottomBorder);

  CRect temp(rect);
  if (m_dwStyle & CBRS_ORIENT_HORZ)
  {
    temp.InflateRect(0,-2,-1,-3);
  }
  else
  {
    temp.InflateRect(-2,0,-3,-1);
  } 
  CNewMenu::GetActualMenuTheme()->DrawCorner (pDC,temp,m_dwStyle);
}

void CNewDialogBar::OnPaint()
{
  CDialogBar::OnPaint();
}

void CNewDialogBar::DoPaint(CDC* pDC)
{
  // we do not call basis class, because needed function are not virtual declared
	ASSERT_VALID(this);
	ASSERT_VALID(pDC);

	// paint inside client area
	CRect rect;
	GetClientRect(rect);
	DrawBorders(pDC, rect);
	DrawGripper(pDC, rect);
}


void CNewDialogBar::OnNcPaint()
{
  EraseNonClient();
}

void CNewDialogBar::EraseNonClient()
{
  // get window DC that is clipped to the non-client area
  CWindowDC dc(this);
  CRect rectClient;
  GetClientRect(rectClient);
  CRect rectWindow;
  GetWindowRect(rectWindow);
  ScreenToClient(rectWindow);

  rectClient.OffsetRect(-rectWindow.left, -rectWindow.top);
  dc.ExcludeClipRect(rectClient);

  CPoint Offset = rectWindow.TopLeft();

  // draw borders in non-client area
  rectWindow.OffsetRect(-rectWindow.left, -rectWindow.top);
  DrawBorders(&dc, rectWindow);

  // erase parts not drawn
  dc.IntersectClipRect(rectWindow);

  CPoint oldOffset;
  // correcting offset of the border
  if(!(m_dwStyle & CBRS_FLOATING))
  {
    oldOffset = dc.SetWindowOrg(Offset);
  }
  else
  {
    oldOffset = dc.GetWindowOrg();
  }
  SendMessage(WM_ERASEBKGND, (WPARAM)dc.m_hDC);

  // reset offset
  dc.SetWindowOrg(oldOffset);

  // draw gripper in non-client area
  DrawGripper(&dc, rectWindow);
}

void CNewDialogBar::DrawBorders(CDC* pDC, CRect& rect)
{
  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
    break;

  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
   break;

  default:
    CDialogBar::DrawBorders(pDC,rect);
    break;
  }
}

BOOL CNewDialogBar::OnEraseBkgnd(CDC* pDC)
{
  BOOL bRet = true;

  CRect rect;
  if(m_dwStyle & CBRS_FLOATING)
  {
    GetClientRect(rect);
  }
  else
  {
    GetWindowRect(rect);
    ScreenToClient(rect);
  }

  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    {
      COLORREF menuColor = CNewMenu::GetMenuBarColorXP();
      pDC->FillSolidRect(rect,MixedColor(menuColor,GetSysColor(COLOR_WINDOW)));
    }
    break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
    if(NumScreenColors()<256)
    {
      CRect rcToolBar = rect;
      if(m_dwStyle & CBRS_ORIENT_HORZ)
      {
        if(!(m_dwStyle & CBRS_FLOATING))
        {
          rcToolBar.bottom = rect.bottom - 2;
          rcToolBar.top = rect.top + 2;
        }
      }
      else
      {
        rcToolBar.left += 2;
        rcToolBar.right -= 3;
      }

      PaintToolBarBackGnd(pDC);
      COLORREF menuColor = GetXpHighlightColor();
      pDC->FillSolidRect(rcToolBar,DarkenColor(30,menuColor));
    }
    else
    {
      COLORREF clrUpperColor, clrMediumColor, clrBottomColor, clrDarkLine;
      CNewMenu::GetActualMenuTheme()->GetBarColor(clrUpperColor,clrMediumColor,clrBottomColor,clrDarkLine);
      PaintToolBarBackGnd(pDC);
      if(m_dwStyle & CBRS_ORIENT_HORZ)
      {
        CRect rcToolBar = rect;
        rcToolBar.bottom /= 2;
        rcToolBar.top = rect.top + 2;
        DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,false);

        rcToolBar.top = rcToolBar.bottom;
        if(!(m_dwStyle & CBRS_FLOATING))
        {
          rcToolBar.bottom = rect.bottom - 2;
        }
        else
        {
          rcToolBar.bottom = rect.bottom;
        }
        DrawGradient(pDC,rcToolBar,clrMediumColor,clrBottomColor,false);

        if(!(m_dwStyle & CBRS_FLOATING))
        {
          if(clrDarkLine!=CLR_NONE)
          {
            CRect line(rect.left+2,rect.bottom-3,rect.right-2,rect.bottom-2);
            //dark line on bottom toolbar
            pDC->FillSolidRect(line,clrDarkLine);
          }
        }
      }
      else
      {
        if(!(m_dwStyle & CBRS_FLOATING))
        {
          if(clrDarkLine!=CLR_NONE)
          {
            CRect line(rect.right-3,rect.top,rect.right-2,rect.bottom);
            //dunkler strich am unteren ende der toolbar
            pDC->FillSolidRect(line,clrDarkLine);
          }
        }
        CRect rcToolBar = rect;
        rcToolBar.left += 2;
        rcToolBar.right /= 2;
        DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,true);
        rcToolBar.left = rcToolBar.right;
        rcToolBar.right = rect.right-3;
        DrawGradient(pDC,rcToolBar,clrMediumColor,clrBottomColor,true);
      }
    }
    break;

  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
    if(NumScreenColors()<256)
    {
      CRect rcToolBar = rect;
      if(m_dwStyle & CBRS_ORIENT_HORZ)
      {
        if(!(m_dwStyle & CBRS_FLOATING))
        {
          rcToolBar.bottom = rect.bottom - 2;
          rcToolBar.top = rect.top + 2;
        }
      }
      else
      {
        rcToolBar.left += 2;
        rcToolBar.right -= -3;
      }
      PaintToolBarBackGnd(pDC);
      COLORREF menuColor = CNewMenu::GetMenuBarColor();
      pDC->FillSolidRect(rcToolBar,DarkenColor(30,menuColor));
    }
    else
    {
      COLORREF menuColor = CNewMenu::GetMenuColor();
      COLORREF clrUpperColor = LightenColor(60,menuColor);
      COLORREF clrMediumColor = menuColor;
      COLORREF clrBottomColor = DarkenColor(60,menuColor);
      COLORREF clrDarkLine = DarkenColor(150,menuColor);

      PaintToolBarBackGnd(pDC);
      if(m_dwStyle & CBRS_ORIENT_HORZ)
      {
        CRect rcToolBar = rect;
        rcToolBar.bottom /= 2;
        rcToolBar.top = rect.top + 2;
        DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,false);

        rcToolBar.top = rcToolBar.bottom;
         if(!(m_dwStyle & CBRS_FLOATING))
        {
          rcToolBar.bottom = rect.bottom - 2;
        }
        else
        {
          rcToolBar.bottom = rect.bottom;
        }
        DrawGradient(pDC,rcToolBar,clrMediumColor,clrBottomColor,false);

        if(!(m_dwStyle & CBRS_FLOATING))
        {
          CRect line(rect.left+2,rect.bottom-3,rect.right-2,rect.bottom-2);
          //dark line on bottom toolbar
          pDC->FillSolidRect(line,clrDarkLine);
        }
      }
      else
      {
        if(!(m_dwStyle & CBRS_FLOATING))
        {
           if(clrDarkLine!=CLR_NONE)
          {
            CRect line(rect.right-3,rect.top,rect.right-2,rect.bottom);
            //dunkler strich am unteren ende der toolbar
            pDC->FillSolidRect(line,clrDarkLine);
          }
        }
        CRect rcToolBar = rect;
        rcToolBar.left += 2;
        rcToolBar.right /= 2;
        DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,true);
        rcToolBar.left = rcToolBar.right;
        rcToolBar.right = rect.right-3;
        DrawGradient(pDC,rcToolBar,clrMediumColor,clrBottomColor,true);
      }
    }
    break;

  default:
    {
      pDC->FillSolidRect(rect, CNewMenu::GetMenuBarColor());
    }
    break;
  }
  return bRet;
}

void CNewDialogBar::PaintToolBarBackGnd(CDC* pDC)
{
  if((m_dwStyle & CBRS_FLOATING))
  {
    return;
  }

  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:

  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
    break;

  default:
    return;
  }

  HWND hParent = ::GetParent(m_hWnd);
  // if there is a parent to the parent, then get it
  if (::GetParent(hParent))
  {
    hParent = ::GetParent(hParent);
  }

  { // Block for local variable cleanup
    CRect clientRect;
    GetWindowRect(clientRect);
    CRect windowRect;
    ::GetWindowRect(hParent,windowRect);
    ScreenToClient(windowRect);
    ScreenToClient(clientRect);

    CBrush*pBrush = GetMenuBarBrush();
    if(pBrush)
    {
      // need for win95/98/me
      VERIFY(pBrush->UnrealizeObject());
      CPoint oldOrg = pDC->SetBrushOrg(windowRect.left,0);

      pDC->FillRect(clientRect,pBrush);
      pDC->SetBrushOrg(oldOrg);
    }
    else
    {
      pDC->FillSolidRect(clientRect,CNewMenu::GetMenuBarColor());
    }
  }
}