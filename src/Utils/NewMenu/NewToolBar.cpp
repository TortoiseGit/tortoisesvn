//------------------------------------------------------------------------------
// File    : NewToolBar.cpp
// Version : 1.18
// Date    : 25. June 2005
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewToolBar
//
// For bugreport please add following informations
// - The CNewToolBar version number (Example CNewToolBar 1.11)
// - Operating system Win95 / WinXP and language (English / Japanese / German etc.)
// - Installed service packs
// - Version of internet explorer (important for CNewToolBar)
// - Short description how to reproduce the bug
// - Pictures/Sample are welcome too
// - You can write in English or German to the above email-address.
// - Have my compiled examples the same effect?
//------------------------------------------------------------------------------
//
// ToDo's
// - Supporting comboboxes
// - Seperator drawing
// - Adding shade to icons
// - Correcting checked state
// - Adding text
//
// 25. June 2005 (Version 1.18)
// added compatibility for new security enhancement in CRT (MFC 8.0)
//
// 16. Mai 2005 1.17
// Changed drawing for gripper to CMenuTheme
// added minimal support for dynamically added or replaced bitmaps
// - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
//   corrected the background color used on the toolbar.  This change affects Windows 9x/Me as far as I can tell, only.
//   added calculating layout in pain
//
// 12. January 2005 1.16 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// added glooming support to the toolbar
// changed icy to use different text color because the default was too dark instead use the system highlight text color
//
// 2. January 2005 1.15
// corrected shade-painting of selected items XP-Style
//
// 19. July 2004 1.14
// corrected menuposition with multiple monitor support
//
// 27. June 2004 1.13
// fixed highlighting from a button that has just been disabled, thnks to Hans Dietrich
// corrected menuposition, thanks to Manfred Drasch
//
// 1. May 2004 1.12
// Added closing menu when mouse over an other toolbar button
// improved drawing for menushade
//
// 25. January 2004 1.11
// icons now centered and text drawing support, thanks to juergen schmitz
// Added support for DropDown Buttons
// Added support for Windows-Controls, like comoboxes, thanks to Jean-Michel LE FOL
//
// 23. November 2003 (1.10)
// - Work with Themes (manifest)
// - Expanded office 2003 look
// - Expanded ICY-Style
// - work also with 16-bit colored toolbar
//
// 12. July 2003 Version 1.0
// - Thanks to Andreas Schärer for some hints
// - It works with IE 4.0 and above
// - only a minimal implementation not supported all styles
// - support more than 16 colors for toolbarbitmaps
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CX_GRIPPER  3
#define CY_GRIPPER  3
#define CX_BORDER_GRIPPER 2
#define CY_BORDER_GRIPPER 2

#ifndef BTNS_DROPDOWN
#define BTNS_DROPDOWN   TBSTYLE_DROPDOWN    // 0x0008
#endif


// CNewToolBar

IMPLEMENT_DYNAMIC(CNewToolBar, CToolBar)
CNewToolBar::CNewToolBar()
: m_ActMenuIndex(-1),
  m_pCustomizeMenu(NULL),
  m_DoCheck(0),
  m_transparentColor(CLR_DEFAULT)
{
}

CNewToolBar::~CNewToolBar()
{
}

BEGIN_MESSAGE_MAP(CNewToolBar, CToolBar)
  ON_WM_NCPAINT()
  ON_WM_ERASEBKGND()
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
  ON_WM_CREATE()
  ON_WM_PAINT()
//  ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
  ON_MESSAGE(TB_ADDBITMAP , OnAddBitmap)
  ON_MESSAGE(TB_REPLACEBITMAP , OnReplaceBitmap)
END_MESSAGE_MAP()



// CNewToolBar message handlers
void CNewToolBar::DrawGripper(CDC* pDC, const CRect& rect)
{
  CNewMenu::GetActualMenuTheme()->DrawGripper(pDC,rect,m_dwStyle, m_cxLeftBorder, m_cxRightBorder, m_cyTopBorder, m_cyBottomBorder);
  CRect temp(rect);
  if (m_dwStyle & CBRS_ORIENT_HORZ)
  {
    temp.InflateRect(-m_cxLeftBorder,-2,-m_cxRightBorder-1,-m_cxRightBorder);
  }
  else
  {
    temp.InflateRect(-m_cxLeftBorder+1,-3,-m_cxRightBorder,-m_cxRightBorder-1);
  } 
  CNewMenu::GetActualMenuTheme()->DrawCorner (pDC,temp,m_dwStyle);
}

void CNewToolBar::OnNcPaint()
{
  EraseNonClient();
}

void CNewToolBar::EraseNonClient()
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

void CNewToolBar::DrawBorders(CDC* pDC, CRect& rect)
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
    CToolBar::DrawBorders(pDC,rect);
    break;
  }
}

BOOL CNewToolBar::OnEraseBkgnd(CDC* pDC)
{
  BOOL bRet = true;//CToolBar::OnEraseBkgnd(pDC);

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
        rcToolBar.left += m_cxLeftBorder;
        rcToolBar.right -= m_cxRightBorder;

        if(!(m_dwStyle & CBRS_FLOATING))
        {
          rcToolBar.bottom = rect.bottom - 2;
          rcToolBar.top = rect.top + 2;
        }
      }
      else
      {
        rcToolBar.top += m_cxLeftBorder;
        rcToolBar.bottom -= m_cxRightBorder;
        rcToolBar.left += m_cxLeftBorder;
        rcToolBar.right -= m_cxRightBorder;
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

      //clrDarkLine = DarkenColor(80,menuColor);
      if(m_dwStyle & CBRS_ORIENT_HORZ)
      {
        CRect rcToolBar = rect;
        rcToolBar.left += m_cxLeftBorder;
        rcToolBar.right -= m_cxRightBorder;
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
            //GetClientRect(rect);
            CRect line(rect.left+m_cxLeftBorder,rect.bottom-3,rect.right-m_cxRightBorder,rect.bottom-2);
            //dark line on bottom toolbar
            pDC->FillSolidRect(line,clrDarkLine);
          }
        }
        else
        {
          CRect itemRect;
          CRect lastItemRect(0,0,0,0);
          int nCountBtn = (int)SendMessage (TB_BUTTONCOUNT, 0, 0);
          TBBUTTON tbButton;
          for(int i=0;i<nCountBtn;i++)
          {
            if (!SendMessage(TB_GETBUTTON, i, (LPARAM)&tbButton) ||
              (tbButton.fsState & TBSTATE_HIDDEN))
            { // coninue by error or hidden buttons
              continue;
            }
            GetItemRect(i,itemRect);
            if(itemRect.top==lastItemRect.top &&
               itemRect.bottom==lastItemRect.bottom)
            {
              continue;
            }
            if((tbButton.fsStyle == TBSTYLE_SEP) && (tbButton.fsState & TBSTATE_WRAP) )
            {
              CRect rcToolBar(rect.left,lastItemRect.bottom,rect.right,itemRect.bottom);
              DrawGradient(pDC,rcToolBar,clrBottomColor,clrUpperColor,false);
            }
            else
            {
              CRect rcToolBar(rect.left,itemRect.top,rect.right,(itemRect.bottom+itemRect.top)/2);
              DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,false);

              rcToolBar.top = rcToolBar.bottom;
              rcToolBar.bottom = itemRect.bottom;
              DrawGradient(pDC,rcToolBar,clrMediumColor,clrBottomColor,false);
            }
            lastItemRect = itemRect;
          }
        }
      }
      else
      {
        if(!(m_dwStyle & CBRS_FLOATING))
        {
          if(clrDarkLine!=CLR_NONE)
          {
            //GetClientRect(rect);
            CRect line(rect.right-m_cxRightBorder  ,rect.top+m_cxRightBorder,
              rect.right-m_cxRightBorder+1,rect.bottom-m_cxRightBorder);

            //dunkler strich am unteren ende der toolbar
            pDC->FillSolidRect(line,clrDarkLine);
          }
        }
        CRect rcToolBar = rect;
        rcToolBar.top += m_cxLeftBorder;
        rcToolBar.bottom -= m_cxRightBorder;
        rcToolBar.left += m_cxLeftBorder-1;
        rcToolBar.right /= 2;
        DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,true);
        rcToolBar.left = rcToolBar.right;
        rcToolBar.right = rect.right-m_cxRightBorder;
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
        rcToolBar.left += m_cxLeftBorder;
        rcToolBar.right -= m_cxRightBorder;

        if(!(m_dwStyle & CBRS_FLOATING))
        {
          rcToolBar.bottom = rect.bottom - 2;
          rcToolBar.top = rect.top + 2;
        }
      }
      else
      {
        rcToolBar.top += m_cxLeftBorder;
        rcToolBar.bottom -= m_cxRightBorder;
        rcToolBar.left += m_cxLeftBorder;
        rcToolBar.right -= m_cxRightBorder;
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
        rcToolBar.left += m_cxLeftBorder;
        rcToolBar.right -= m_cxRightBorder;
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
          CRect topLine(rect.left+m_cxLeftBorder,rect.top+m_cyTopBorder+1,
                        rect.right-m_cxRightBorder,rect.top+m_cyTopBorder+2);
          pDC->FillSolidRect(topLine,LightenColor(125,menuColor));

          CRect line(rect.left+m_cxLeftBorder,rect.bottom-3,rect.right-m_cxRightBorder,rect.bottom-2);
          //dark line on bottom toolbar
          pDC->FillSolidRect(line,clrDarkLine);
        }
        else
        {
          CRect itemRect;
          CRect lastItemRect(0,0,0,0);
          int nCountBtn = (int)SendMessage (TB_BUTTONCOUNT, 0, 0);
          TBBUTTON tbButton;
          for(int i=0;i<nCountBtn;i++)
          {
            if (!SendMessage(TB_GETBUTTON, i, (LPARAM)&tbButton) ||
              (tbButton.fsState & TBSTATE_HIDDEN))
            { // coninue by error or hidden buttons
              continue;
            }
            GetItemRect(i,itemRect);
            if(itemRect.top==lastItemRect.top &&
               itemRect.bottom==lastItemRect.bottom)
            {
              continue;
            }
            if((tbButton.fsStyle == TBSTYLE_SEP) && (tbButton.fsState & TBSTATE_WRAP) )
            {
              CRect rcToolBar(rect.left,lastItemRect.bottom,rect.right,itemRect.bottom);

              DrawGradient(pDC,rcToolBar,clrBottomColor,clrUpperColor,false);
            }
            else
            {
              CRect rcToolBar(rect.left,itemRect.top,rect.right,(itemRect.bottom+itemRect.top)/2);

              DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,false);

              rcToolBar.top = rcToolBar.bottom;
              rcToolBar.bottom = itemRect.bottom;
              DrawGradient(pDC,rcToolBar,clrMediumColor,clrBottomColor,false);
            }
            lastItemRect = itemRect;
          }
        }
      }
      else
      {
        if(!(m_dwStyle & CBRS_FLOATING))
        {
          CRect topLine(rect.left+m_cxLeftBorder-1,rect.top+m_cxRightBorder,
                        rect.left+m_cxLeftBorder  ,rect.bottom-m_cxRightBorder);
          pDC->FillSolidRect(topLine,LightenColor(125,menuColor));

          CRect line(rect.right-m_cxRightBorder  ,rect.top+m_cxRightBorder,
                     rect.right-m_cxRightBorder+1,rect.bottom-m_cxRightBorder);

          //dunkler strich am unteren ende der toolbar
          pDC->FillSolidRect(line,clrDarkLine);
        }
        CRect rcToolBar = rect;
        rcToolBar.top += m_cxLeftBorder;
        rcToolBar.bottom -= m_cxRightBorder;
        rcToolBar.left += m_cxLeftBorder;
        rcToolBar.right /= 2;
        DrawGradient(pDC,rcToolBar,clrUpperColor,clrMediumColor,true);
        rcToolBar.left = rcToolBar.right;
        rcToolBar.right = rect.right-m_cxRightBorder;
        DrawGradient(pDC,rcToolBar,clrMediumColor,clrBottomColor,true);
      }
    }
    break;

  default:
    {
// May-10-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// corrected the color use here to use the same color as the menu bar, the previous code is in the comment
      pDC->FillSolidRect(rect, CNewMenu::GetMenuBarColor()/*GetSysColor(COLOR_3DLIGHT)*/);
    }
    break;
  }
  return bRet;
}


// Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// added this function to support gloomed buttons on the toolbar, this function was taken from
// CNewMenu almost verbatim
int CNewToolBar::AddGloomIcon(HICON hIcon, int nIndex /*=-1*/)
{
  ICONINFO  iconInfo = {0};

  // get the icon information from the icon we're adding to the gloom image list
  if (! GetIconInfo(hIcon, &iconInfo))
  {
    return (-1);
  }

  // create a DC used to convert from an icon to a bitmap
  CDC myDC;
  myDC.CreateCompatibleDC(0);

  // create a bitmap from the icon
  CBitmap bmColor;
  bmColor.Attach(iconInfo.hbmColor);

  // create a bitmap mask
  CBitmap bmMask;
  bmMask.Attach(iconInfo.hbmMask);

  // select the icon into the DC as it is now
  CBitmap* pOldBitmap = myDC.SelectObject(&bmColor);

  // do the width
  for (int nX = 0; nX < m_sizeImage.cx; nX++)
  {
    // do the height
    for (int nY = 0; nY < m_sizeImage.cy; nY++)
    {
      // get the pixel as it is now
      COLORREF crPixel = myDC.GetPixel(nX, nY);

      // darken the pixel and set it
      myDC.SetPixel(nX, nY, DarkenColor(CNewMenu::GetGloomFactor(), crPixel));
    } // for
  } // for

  // reselect the previous bitmap
  myDC.SelectObject(pOldBitmap);

  // add it to the end?
  if (nIndex == -1)
  {
    return m_GloomImageList.Add(&bmColor, &bmMask);
  }

  // replace the icon, return -1 if there was an error, otherwise return the index
  return ((m_GloomImageList.Replace(nIndex, &bmColor, &bmMask)) ? nIndex: -1);
} // AddGloomIcon


// Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// this function creates the gloom image list based on the regular image list.
// each image is read from the original image list, modified by AddGloomIcon() pixel-by-pixel
// then added to the gloom image list
void CNewToolBar::BuildGloomImageList()
{
  int nCount = m_ImageList.GetImageCount();

  // if the gloom image list already exists, delete it
  if (m_GloomImageList.GetSafeHandle())
  {
    m_GloomImageList.DeleteImageList();
  }

  // create the gloom image list
  m_GloomImageList.Create(m_sizeImage.cx, m_sizeImage.cy, (ILC_COLORDDB | ILC_MASK), 0, 10);

  // do each of the images in the image list
  for (int nIndex = 0; nIndex < nCount; nIndex++)
  {
    // extract the icon (original image) from the image list
    HICON hIcon = m_ImageList.ExtractIcon(nIndex);

    // add the gloom icon to the gloom icon list
    AddGloomIcon(hIcon);

    // release the icon now that we're done with it
    DestroyIcon(hIcon);
  } // for

  // set the appropriate image list
  SendMessage(TB_SETIMAGELIST, 0, (LPARAM) ((CNewMenu::GetXpBlending()) ? m_GloomImageList.m_hImageList: m_ImageList.m_hImageList));
} // BuildGloomImageList


BOOL CNewToolBar::LoadHiColor(LPCTSTR lpszResourceName,COLORREF transparentColor/*=CLR_DEFAULT*/)
{
  int nColors = 0;
  HBITMAP hBitmap = LoadColorBitmap(lpszResourceName,0, &nColors);
  if(hBitmap!=NULL)
  {
    //if(nColors>16)
    {
      CBitmap bitmap;
      bitmap.Attach(hBitmap);

      // Get the color of the top/lef corner
      if(transparentColor==CLR_DEFAULT)
      {
        CDC memDC;
        memDC.CreateCompatibleDC(0);
        CBitmap* pOld = memDC.SelectObject(&bitmap);
        transparentColor = memDC.GetPixel(0,0);
        memDC.SelectObject(pOld);
      }

      if(m_ImageList.GetSafeHandle())
      {
        m_ImageList.DeleteImageList();
      }
      m_ImageList.Create(m_sizeImage.cx,m_sizeImage.cy,ILC_COLORDDB|ILC_MASK,0,10);
      m_ImageList.Add(&bitmap,transparentColor);

      // Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
      // build the gloom image list and set the appropriate image list
      BuildGloomImageList();

      COLORREF transparentColor;
      switch (CNewMenu::GetMenuDrawMode())
      {
      case CNewMenu::STYLE_XP_2003:
      case CNewMenu::STYLE_XP_2003_NOBORDER:
      case CNewMenu::STYLE_COLORFUL:
      case CNewMenu::STYLE_COLORFUL_NOBORDER:
        {
          COLORREF blendcolor = LightenColor(115,CNewMenu::GetMenuBarColor2003());
          transparentColor = MakeGrayAlphablend(&bitmap,110, blendcolor);
        }
        break;

      case CNewMenu::STYLE_XP:
      case CNewMenu::STYLE_XP_NOBORDER:
      // Jan-19-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
      // added default here, because a straight black line is not the Windows default
      default:
        {
          COLORREF blendcolor = LightenColor(115,CNewMenu::GetMenuBarColorXP());
          transparentColor = MakeGrayAlphablend(&bitmap,110, blendcolor);
        }
        break;
      }

      if(m_ImageListDisabled.GetSafeHandle())
      {
        m_ImageListDisabled.DeleteImageList();
      }
      m_ImageListDisabled.Create(m_sizeImage.cx,m_sizeImage.cy,ILC_COLORDDB|ILC_MASK,0,10);
      m_ImageListDisabled.Add(&bitmap,transparentColor);

      return TRUE;
    }
  }
  return FALSE;
}

BOOL CNewToolBar::LoadToolBar(LPCTSTR lpszResourceName)
{
  BOOL bRet = CToolBar::LoadToolBar(lpszResourceName);

  LoadHiColor(lpszResourceName);

  return bRet;
}

BOOL CNewToolBar::LoadToolBar(UINT nIDResource)
{
  return LoadToolBar(MAKEINTRESOURCE(nIDResource));
}

void CNewToolBar::PaintOrangeState(CDC *pDC, CRect rc, bool bHot)
{
  CPen penBlue;
  //dunkelblauer rahmen
  penBlue.CreatePen(PS_INSIDEFRAME,1, RGB(4, 2, 132));
  CPen* pOldPen = pDC->SelectObject(&penBlue);

  pDC->Rectangle(rc);
  rc.DeflateRect(1,1);

  if(bHot)
  { //heller oranger rahmen
    DrawGradient(pDC,rc,RGB(250,239,219),RGB(255,212,151),false);
  }
  else
  { //dunkler oranger rahmen
    DrawGradient(pDC,rc,RGB(242,151,107),RGB(249,202,163),false);
  }
  pDC->SelectObject(pOldPen);
}

// here we draw only the button background with border
BOOL CNewToolBar::PaintHotButton(LPNMTBCUSTOMDRAW lpNMTBCustomDraw)
{
  LPNMCUSTOMDRAW lpNMCustomDraw = &lpNMTBCustomDraw->nmcd;
  CDC* pDC = CDC::FromHandle(lpNMCustomDraw->hdc);

  int nIndex = CommandToIndex((UINT)lpNMCustomDraw->dwItemSpec);
  UINT nID = 0;
  UINT nStyle = 0;
  int iImage = 0;
  GetButtonInfo(nIndex,nID,nStyle,iImage);

   int cx=0, cy=0;
  CImageList* pImgList = (lpNMCustomDraw->uItemState&CDIS_DISABLED)?&m_ImageListDisabled:&m_ImageList;
  ::ImageList_GetIconSize(pImgList->GetSafeHandle(),&cx,&cy);

  extern BOOL bHighContrast; // defined in newmenu.cpp

  UINT nDrawMode = CNewMenu::GetMenuDrawMode();
  if( !IsMenuThemeActive() &&
     ((nDrawMode==CNewMenu::STYLE_XP_2003_NOBORDER)||
      (nDrawMode==CNewMenu::STYLE_XP_2003)) )
  {
    nDrawMode = CNewMenu::STYLE_XP;
  }
  switch (nDrawMode)
  {
  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    {
      if(lpNMCustomDraw->uItemState&CDIS_INDETERMINATE)
      {
        COLORREF colorBitmap = MixedColor(CNewMenu::GetMenuBarColor(),GetSysColor(COLOR_WINDOW));
        pDC->FillSolidRect(&(lpNMCustomDraw->rc),colorBitmap);

        //COLORREF colorBorder = GetSysColor(COLOR_HIGHLIGHT);
        COLORREF colorBorder = bHighContrast?GetSysColor(COLOR_BTNTEXT):DarkenColor(128,CNewMenu::GetMenuBarColor());
        pDC->Draw3dRect(&(lpNMCustomDraw->rc),colorBorder,colorBorder);
      }
      else
      if (lpNMCustomDraw->uItemState&CDIS_CHECKED ||
          lpNMCustomDraw->uItemState&CDIS_HOT)
      {
        COLORREF colorSel = GetXpHighlightColor();
        COLORREF colorBorder = GetSysColor(COLOR_HIGHLIGHT);
        pDC->FillSolidRect(&(lpNMCustomDraw->rc),colorSel);
        pDC->Draw3dRect(&(lpNMCustomDraw->rc),colorBorder,colorBorder);
        // we are a menu button
        if(BTNS_DROPDOWN&nStyle )
        {
          CRect rect(lpNMCustomDraw->rc);
          rect.left = lpNMCustomDraw->rc.right-13;
          pDC->Draw3dRect(rect,colorBorder,colorBorder);
        }
      }
    }
    break;

    case CNewMenu::STYLE_ICY:
    case CNewMenu::STYLE_ICY_NOBORDER:
      {
        COLORREF colorMid = GetSysColor(COLOR_HIGHLIGHT);
        COLORREF colorSel = LightenColor(60,colorMid);
        COLORREF colorBorder = DarkenColor(60,colorMid);
        CRect rect(lpNMCustomDraw->rc);
        BOOL bDrawBorder = FALSE;
        if(lpNMCustomDraw->uItemState&CDIS_INDETERMINATE)
        {
          if(NumScreenColors()<256)
          {
            pDC->FillSolidRect(rect,colorSel);
          }
          else
          {
            COLORREF menuColor = CNewMenu::GetMenuColor();
            COLORREF clrUpperColor = LightenColor(60,menuColor);
            COLORREF clrMediumColor = menuColor;
            COLORREF clrBottomColor = DarkenColor(60,menuColor);

            rect.bottom = (rect.bottom+rect.top)/2;
            DrawGradient(pDC,rect,clrUpperColor,clrMediumColor,false);
            rect.top = rect.bottom;
            rect.bottom = lpNMCustomDraw->rc.bottom;
            DrawGradient(pDC,rect,clrMediumColor,clrBottomColor,false);
          }
          pDC->Draw3dRect(&(lpNMCustomDraw->rc),colorBorder,colorBorder);
        }
        else if(lpNMCustomDraw->uItemState == CDIS_HOT)
        {
          DrawGradient(pDC,rect,colorSel,colorMid,false);
          pDC->Draw3dRect(&(lpNMCustomDraw->rc),colorBorder,colorBorder);
          bDrawBorder = TRUE;
        }
        else if (lpNMCustomDraw->uItemState == (CDIS_HOT | CDIS_SELECTED))
        {
          DrawGradient(pDC,rect,colorMid,colorBorder,false);
          pDC->Draw3dRect(&(lpNMCustomDraw->rc),colorBorder,colorBorder);
          bDrawBorder = TRUE;
        }
        else if (lpNMCustomDraw->uItemState&CDIS_CHECKED)
        {
          pDC->FillSolidRect(&(lpNMCustomDraw->rc),colorSel);
          pDC->Draw3dRect(&(lpNMCustomDraw->rc),colorBorder,colorBorder);
          bDrawBorder = TRUE;
        }
        // we are a menu button
        if(bDrawBorder && BTNS_DROPDOWN&nStyle)
        {
          CRect rect(lpNMCustomDraw->rc);
          rect.left = lpNMCustomDraw->rc.right-13;//lpNMCustomDraw->rc.left + 3 + 4 + cx;
          pDC->Draw3dRect(rect,colorBorder,colorBorder);
        }
      }
      break;

    case CNewMenu::STYLE_XP_2003_NOBORDER:
    case CNewMenu::STYLE_XP_2003:
    case CNewMenu::STYLE_COLORFUL_NOBORDER:
    case CNewMenu::STYLE_COLORFUL:
    {
      BOOL bDrawBorder = FALSE;
      if(lpNMCustomDraw->uItemState&CDIS_INDETERMINATE)
      {
        COLORREF colorMenu = CNewMenu::GetMenuBarColor2003();
        CRect rect(lpNMCustomDraw->rc);
        if(NumScreenColors() <= 256)
        {
          pDC->FillSolidRect(rect,colorMenu);
        }
        else
        {
          COLORREF colorBitmap = colorMenu;
          colorMenu = MixedColor(DarkenColor(10,GetSysColor(COLOR_WINDOW)),CNewMenu::GetMenuColor());

          DrawGradient(pDC,rect,colorMenu,colorBitmap,FALSE,TRUE);
        }

        //COLORREF colorBorder = GetSysColor(COLOR_HIGHLIGHT);
        COLORREF colorBorder = bHighContrast?GetSysColor(COLOR_BTNTEXT):DarkenColor(128,CNewMenu::GetMenuBarColor());
        pDC->Draw3dRect(rect,colorBorder,colorBorder);
      }
      else if(lpNMCustomDraw->uItemState == CDIS_HOT)
      {
        PaintOrangeState(pDC,lpNMCustomDraw->rc, true);
        bDrawBorder = TRUE;
      }
      else if (lpNMCustomDraw->uItemState == (CDIS_HOT | CDIS_SELECTED))
      {
        PaintOrangeState(pDC,lpNMCustomDraw->rc, false);
        bDrawBorder = TRUE;
      }
      else if (lpNMCustomDraw->uItemState&CDIS_CHECKED)
      {
        COLORREF colorSel = RGB(255,238,194);
        COLORREF colorBorder = RGB(4, 2, 132);//GetSysColor(COLOR_HIGHLIGHT);
        pDC->FillSolidRect(&(lpNMCustomDraw->rc),colorSel);
        pDC->Draw3dRect(&(lpNMCustomDraw->rc),colorBorder,colorBorder);
        bDrawBorder = TRUE;
      }
      // we are a menu button
      if(bDrawBorder && BTNS_DROPDOWN&nStyle)
      {
        COLORREF colorBorder = RGB(4, 2, 132);//GetSysColor(COLOR_HIGHLIGHT);
        CRect rect(lpNMCustomDraw->rc);
        rect.left = lpNMCustomDraw->rc.right-13;//lpNMCustomDraw->rc.left + 3 + 4 + cx;
        pDC->Draw3dRect(rect,colorBorder,colorBorder);
      }
    }
    break;

  default:
    {
      BOOL bDrawBorder = FALSE;
      if(lpNMCustomDraw->uItemState == CDIS_HOT )
      {
        pDC->Draw3dRect(&(lpNMCustomDraw->rc),GetSysColor(COLOR_HIGHLIGHTTEXT),GetSysColor(COLOR_BTNSHADOW));
        bDrawBorder = TRUE;
      }
      else if (lpNMCustomDraw->uItemState == (CDIS_HOT | CDIS_SELECTED)||
         lpNMCustomDraw->uItemState&CDIS_INDETERMINATE)
      {
        pDC->Draw3dRect(&(lpNMCustomDraw->rc),GetSysColor(COLOR_BTNSHADOW),GetSysColor(COLOR_HIGHLIGHTTEXT));
        bDrawBorder = TRUE;
      }
      else if (lpNMCustomDraw->uItemState&CDIS_CHECKED)
      {
        pDC->Draw3dRect(&(lpNMCustomDraw->rc),GetSysColor(COLOR_BTNSHADOW),GetSysColor(COLOR_HIGHLIGHTTEXT));
        bDrawBorder = TRUE;
      }
      // we are a menu button
      if(bDrawBorder && BTNS_DROPDOWN&nStyle && !(lpNMCustomDraw->uItemState&CDIS_INDETERMINATE) )
      {
        CRect rect(lpNMCustomDraw->rc);
        rect.left = lpNMCustomDraw->rc.right-13;//lpNMCustomDraw->rc.left + 3 + 4 + cx;
        if(lpNMCustomDraw->uItemState == CDIS_HOT)
        {
          pDC->Draw3dRect(rect,GetSysColor(COLOR_HIGHLIGHTTEXT),GetSysColor(COLOR_BTNSHADOW));
        }
        else
        {
          pDC->Draw3dRect(rect,GetSysColor(COLOR_BTNSHADOW),GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
      }
    }
    break;
  }
  if(BTNS_DROPDOWN&nStyle)
  {
    COLORREF colorDropDown;
    if(lpNMCustomDraw->uItemState&CDIS_DISABLED)
    {
      colorDropDown = GetSysColor(COLOR_GRAYTEXT);
    }
    else
    {
      colorDropDown = GetSysColor(COLOR_WINDOWTEXT);
    }
    CRect rect(lpNMCustomDraw->rc);
    rect.left = lpNMCustomDraw->rc.right-13;//lpNMCustomDraw->rc.left + 3 + 4 + cx;

    CPen pen(PS_SOLID,0,colorDropDown);
    CPen* pOldPen = pDC->SelectObject(&pen);

    CPoint cetner = rect.CenterPoint();
    pDC->MoveTo(cetner+CPoint(-2,-1));pDC->LineTo(cetner+CPoint(+3,-1));
    pDC->MoveTo(cetner+CPoint(-1,-0));pDC->LineTo(cetner+CPoint(+2,-0));
    pDC->MoveTo(cetner+CPoint(-0,+1));pDC->LineTo(cetner+CPoint(+1,+1));

    pDC->SelectObject(pOldPen);
  }

  return TRUE;
}

// here we draw the button image
void CNewToolBar::PaintTBButton(LPNMTBCUSTOMDRAW lpNMTBCustomDraw)
{
  LPNMCUSTOMDRAW pInfo = &lpNMTBCustomDraw->nmcd;
  CDC *pDC = CDC::FromHandle(pInfo->hdc);

  int nIndex = CommandToIndex((UINT)pInfo->dwItemSpec);
  UINT nID = 0;
  UINT nStyle = 0;
  int iImage = 0;
  GetButtonInfo(nIndex,nID,nStyle,iImage);

  int cx=0, cy=0;
  // Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
  // added the last ternary checking whether or not the image is hot or gloom is not being used, if hot or not glooming, then
  // the original image list is used, otherwise the gloom image list is used
  CImageList* pImgList = ((pInfo->uItemState&CDIS_DISABLED) ? &m_ImageListDisabled:
              (((pInfo->uItemState & CDIS_HOT) || (! CNewMenu::GetXpBlending())) ? &m_ImageList: &m_GloomImageList));
  ::ImageList_GetIconSize(pImgList->GetSafeHandle(),&cx,&cy);

  // always center image
  CPoint ptImage((pInfo->rc.left + pInfo->rc.right - cx) / 2 ,
                 (pInfo->rc.top + pInfo->rc.bottom - cy) / 2);

  if(BTNS_DROPDOWN&nStyle)
  {
    ptImage.x -= 7;
  }
  CString sText = GetButtonText(nIndex);
  if (!sText.IsEmpty())
  {
    // correcting of image
    DWORD dwStyle = GetStyle();
    if(dwStyle&TBSTYLE_LIST)
    {// text on the right side
      ptImage.x = pInfo->rc.left+3;
    }
    else
    {// text on the left side
      ptImage.y = pInfo->rc.top+4;
    }

    CFont* pOldFont = (CFont*)pDC->SelectStockObject(ANSI_VAR_FONT);
    int iOldMode = pDC->SetBkMode(TRANSPARENT);
    CRect rect(lpNMTBCustomDraw->rcText);
    if (pInfo->uItemState&CDIS_DISABLED)
    {
      pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
    }
    // Jan-19-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
    // added this conditional, because in icy style the menu text becomes hidden, since we're using the highlight color we really need to use the highlight text color too
    else if ((pInfo->uItemState&CDIS_HOT) && ((CNewMenu::GetMenuDrawMode() == CNewMenu::STYLE_ICY) || (CNewMenu::GetMenuDrawMode() == CNewMenu::STYLE_ICY_NOBORDER)))
    {
      pDC->SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
  else
    {
      pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
    }
    pDC->DrawText(sText, rect, DT_CENTER);
    pDC->SetBkMode(iOldMode);
    pDC->SelectObject(pOldFont);
  }

  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    if(pInfo->uItemState&CDIS_HOT && !(pInfo->uItemState&CDIS_SELECTED))
    {
      CSize imgSize(cx,cy);

      ptImage.x += 1;
      ptImage.y += 1;
      // draws the icon blended
      HICON hDrawIcon2 = pImgList->ExtractIcon(iImage);

      CBrush Brush;
      // Color of the shade
      Brush.CreateSolidBrush(pDC->GetNearestColor(DarkenColorXP(GetXpHighlightColor())));
      pDC->DrawState(ptImage, imgSize, hDrawIcon2, DSS_NORMAL | DSS_MONO, &Brush);

      DestroyIcon(hDrawIcon2);

      ptImage.x -= 2;
      ptImage.y -= 2;
    }
    break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
    break;

  default:
    // Jan-19-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
    // removed the following lines of code, because the disabled state was really ugly, far too ugly....
#if FALSE
    if((pInfo->uItemState&CDIS_SELECTED))
    {
      ptImage.x += 1;
      ptImage.y += 1;
    }
    if(m_ImageList.GetSafeHandle())
    {
      int cx=0;
      int cy=0;
      if(::ImageList_GetIconSize(m_ImageList,&cx,&cy))
      {
        HICON hDrawIcon = m_ImageList.ExtractIcon(iImage);
        ::ImageList_GetIconSize(m_ImageList,&cx,&cy);
        CSize size(cx,cy);
        if(pInfo->uItemState&CDIS_DISABLED)
        {
          pDC->DrawState(ptImage, size, hDrawIcon, DSS_DISABLED, (HBRUSH)NULL);
        }
        else
        {
          pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL, (HBRUSH)NULL);
        }
        DestroyIcon(hDrawIcon);
      }
      return;
    }
#endif
    break;
  }

  if(pImgList->GetSafeHandle())
  {
    pImgList->Draw(pDC,iImage,ptImage,ILD_TRANSPARENT);
  }
}

void CNewToolBar::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMTBCUSTOMDRAW lpNMCustomDraw = (LPNMTBCUSTOMDRAW) pNMHDR;
  *pResult = CDRF_DODEFAULT;

  switch(lpNMCustomDraw->nmcd.dwDrawStage)
  {
  case CDDS_PREPAINT:
    *pResult = CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYPOSTPAINT|CDRF_SKIPDEFAULT;
    break;

  case CDDS_POSTPAINT:
    *pResult = CDRF_NOTIFYSUBITEMDRAW|CDRF_SKIPDEFAULT;
    // Erase Seperators
    break;

  case CDDS_ITEMPOSTPAINT:
    *pResult = CDRF_NOTIFYSUBITEMDRAW|CDRF_SKIPDEFAULT;
    // Erase Seperators
    break;

  case CDDS_ITEMPREPAINT:
    if(PaintHotButton(lpNMCustomDraw))
    {
      *pResult = CDRF_DODEFAULT|TBCDRF_NOEDGES;//CDRF_SKIPDEFAULT;
    }
    PaintTBButton(lpNMCustomDraw);
    *pResult = CDRF_SKIPDEFAULT;
    break;

  default:
    break;
  }
}

int CNewToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  // For correct handling of toolbar highliting the TBSTYLE_FLAT style has to be set
  ASSERT(lpCreateStruct->style&TBSTYLE_FLAT);

  if (CToolBar::OnCreate(lpCreateStruct) == -1)
  {
    return -1;
  }
  //typedef HRESULT (WINAPI* FktSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
  //extern FktSetWindowTheme pSetWindowTheme;
  //if(pSetWindowTheme)
  //{ // try do disable thems, but it does not work!!!
  //  pSetWindowTheme(m_hWnd,L" ",L" ");
  //}

  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    {
      COLORSCHEME scheme;
      scheme.dwSize = sizeof(COLORSCHEME);
      scheme.clrBtnHighlight = CLR_DEFAULT;//DarkenColorXP(CNewMenu::GetMenuBarColor());;
      scheme.clrBtnShadow= DarkenColorXP(CNewMenu::GetMenuBarColorXP());

      SendMessage(TB_SETCOLORSCHEME,0,(LPARAM)&scheme);
    }
    break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
    {
      SetBorders(3,1,3,1);

      COLORSCHEME scheme;
      scheme.dwSize = sizeof(COLORSCHEME);
      scheme.clrBtnHighlight = CLR_DEFAULT;//DarkenColorXP(CNewMenu::GetMenuBarColor());;
      scheme.clrBtnShadow= DarkenColorXP(CNewMenu::GetMenuBarColor2003());

      SendMessage(TB_SETCOLORSCHEME,0,(LPARAM)&scheme);
    }
    break;

  default:
    break;
  }
  return 0;
}

LRESULT CNewToolBar::DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = CToolBar::DefWindowProc ( nMsg, wParam, lParam);
  if(m_DoCheck && nMsg == TB_GETBUTTON && result)
  {
    TBBUTTON* pButton = (TBBUTTON*)lParam;
    if(pButton && (pButton->fsStyle&TBSTYLE_SEP) && pButton->idCommand!=0 )
    {
      if(m_DoCheck==1)
      {
        pButton->fsState |= TBSTATE_HIDDEN;
      }
      else
      {
        pButton->fsState &= ~TBSTATE_HIDDEN;
      }
    }
  }
  return result;
}


CSize CNewToolBar::CalcDynamicLayout (int nLength, DWORD dwMode)
{
  bool bHideControls = (dwMode & LM_VERTDOCK) == LM_VERTDOCK;
  m_DoCheck = bHideControls ? 1 : 2;
  CSize size = CToolBar::CalcDynamicLayout (nLength, dwMode);
  m_DoCheck = 0;

  if ( dwMode & LM_COMMIT )
  {
    int nCountBtn = (int)SendMessage(TB_BUTTONCOUNT, 0, 0);
    TBBUTTON tbButton;
    for(int i=0;i<nCountBtn;i++)
    {
      if ( !SendMessage(TB_GETBUTTON, i, (LPARAM)&tbButton) ||
        !((tbButton.fsStyle&TBSTYLE_SEP) && tbButton.idCommand!=0) )
      { // coninue by error or not controls
        continue;
      }
      CWnd* pWnd = GetDlgItem (tbButton.idCommand);
      if ( !pWnd->GetSafeHwnd() )
      {
        continue;
      }
      if ( bHideControls )
      {
        GetToolBarCtrl().HideButton (tbButton.idCommand, true);
        pWnd->ShowWindow (SW_HIDE);
      }
      else
      {
        GetToolBarCtrl().HideButton (tbButton.idCommand, false);
        // Update control position
        CRect rectTBItem;
        if(SendMessage(TB_GETITEMRECT, i, (LPARAM)&rectTBItem))
        {
          rectTBItem.InflateRect(-1,-1);
          CRect rectControl;
          pWnd->GetWindowRect(rectControl);
          UINT nFlags = SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW;
          if ( rectControl.Width() == rectTBItem.Width() )
          {
            nFlags |= SWP_NOSIZE;
          }
          // center the control in the heigt
          int nOffsety = (rectTBItem.Height()-rectControl.Height())/2;
          rectTBItem.OffsetRect(0,nOffsety);

          pWnd->SetWindowPos (NULL, rectTBItem.left, rectTBItem.top, rectTBItem.Width(), rectTBItem.Height(),nFlags);
        }
      }
    }
  }
  return size;
}

void CNewToolBar::OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle)
{
  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:

  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
    if(dwNewStyle& CBRS_FLOATING)
    {
      SetBorders(0,0,0,0);
    }
    else
    {
      SetBorders(3,1,3,1);
    }
    break;

  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
  default:
    break;
  }
  CToolBar::OnBarStyleChange(dwOldStyle, dwNewStyle);
}

void CNewToolBar::OnPaint()
{
  if (m_bDelayedButtonLayout)
  {
		Layout();
  }

  CPaintDC dc(this);
  CWnd* pParent = GetParent();

  CWnd* pFrameWnd = GetParentFrame();
  if (pFrameWnd == NULL)
  {
    pFrameWnd = m_pDockSite;
  }
  if(pFrameWnd == NULL)
  {
    pFrameWnd = AfxGetMainWnd();
  }
  while(pFrameWnd && pFrameWnd->GetParentFrame())
  {
    pFrameWnd = pFrameWnd->GetParentFrame();
  }
  BOOL bFocus = pFrameWnd?pFrameWnd->IsChild(GetFocus()):false;

  //CFrameWnd* pFrame = GetDockingFrame();
  //while(pFrame && pFrame->GetParentFrame())
  //{
  //  pFrame = pFrame->GetParentFrame();
  //}
  //BOOL bFocus = AfxGetMainWnd()->IsChild(GetFocus());

  NMTBCUSTOMDRAW myDraw = {0};
  myDraw.nmcd.hdr.code = NM_CUSTOMDRAW;
  myDraw.nmcd.hdr.hwndFrom = m_hWnd;
  myDraw.nmcd.hdr.idFrom = GetDlgCtrlID();
  CRect client;
  GetClientRect(client);
  myDraw.nmcd.rc = client;
  myDraw.nmcd.hdc = dc.m_hDC;

  COLORSCHEME colorscheme = {0};
  colorscheme.dwSize = sizeof(colorscheme);
  if(SendMessage(TB_GETCOLORSCHEME,0,(LPARAM)&colorscheme))
  {
    myDraw.clrBtnHighlight = colorscheme.clrBtnHighlight;
    myDraw.clrBtnFace = colorscheme.clrBtnShadow;
  }

  myDraw.nmcd.dwDrawStage = CDDS_PREPAINT;
  LRESULT result = pParent->SendMessage(WM_NOTIFY,myDraw.nmcd.hdr.idFrom,(LPARAM)&myDraw);

  myDraw.nmcd.dwDrawStage = CDDS_PREERASE;
  result = pParent->SendMessage(WM_NOTIFY,myDraw.nmcd.hdr.idFrom,(LPARAM)&myDraw);

  myDraw.nmcd.dwDrawStage = CDDS_POSTERASE;
  result = pParent->SendMessage(WM_NOTIFY,myDraw.nmcd.hdr.idFrom,(LPARAM)&myDraw);

  int nCountBtn = (int)SendMessage(TB_BUTTONCOUNT, 0, 0);
  //int nHotItem = (int)SendMessage(TB_GETHOTITEM,0,0);

  COLORREF clrUpperColor = GetSysColor(COLOR_3DLIGHT);
  COLORREF clrBottomColor = GetSysColor(COLOR_3DDKSHADOW);
  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    {
      COLORREF menuColor = CNewMenu::GetMenuBarColorXP();
      clrUpperColor = clrBottomColor = DarkenColor(40,menuColor);
    }
    break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
  default :
    {
      COLORREF menuColor = CNewMenu::GetMenuBarColor2003();
      clrUpperColor = LightenColor(140,menuColor);
      clrBottomColor = DarkenColor(40,menuColor);
    }
    break;

  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
    clrBottomColor = clrUpperColor = LightenColor(150,clrBottomColor);
    break;
  }

  TBBUTTON tbButton;
  CPoint pt;
  GetCursorPos(&pt);
  if(m_hWnd==::WindowFromPoint(pt))
  {
    ScreenToClient(&pt);
  }
  else
  {
    pt = CPoint(-1,-1);
  }
  DWORD dwStyle = GetStyle();//TBSTYLE_LIST

  for(int i=0;i<nCountBtn;i++)
  {
    if (!SendMessage(TB_GETBUTTON, i, (LPARAM)&tbButton) ||
        (tbButton.fsState & TBSTATE_HIDDEN))
    { // coninue by error or hidden buttons
      continue;
    }

    GetItemRect(i,&myDraw.nmcd.rc);
    if (tbButton.fsStyle == TBSTYLE_SEP)
    {
      CRect rect(myDraw.nmcd.rc);
      if (!(tbButton.fsState & TBSTATE_WRAP) || ! IsFloating())
      {
        if(tbButton.idCommand && GetDlgItem(tbButton.idCommand))
        {
          // do not draw seperators under controls
          continue;
        }
        if (m_dwStyle & CBRS_ORIENT_HORZ)
        {
          rect.left = ((rect.left + rect.right)/2)-1;
          if(clrBottomColor==clrUpperColor)
          {
            rect.right = rect.left + 1;
          }
          else
          {
            rect.right = rect.left + 2;
          }
          rect.top += 3;
          rect.bottom -= 3;
          dc.Draw3dRect(rect,clrBottomColor,clrUpperColor);
        }
        else
        {
          CRect lastRect(0,0,0,0);
          if(i>0)
          {
            GetItemRect(i-1,lastRect);
          }

          rect.top = (rect.bottom + lastRect.bottom)/2;
          if(clrBottomColor==clrUpperColor)
          {
            rect.bottom = rect.top + 1;
          }
          else
          {
            rect.bottom = rect.top + 2;
          }
          rect.right = lastRect.right - 3;
          rect.left += 3;

          dc.Draw3dRect(rect,clrBottomColor,clrUpperColor);
        }
      }
      else if(tbButton.fsState & TBSTATE_WRAP)
      {
        CRect lastRect(0,0,0,0);
        if(i>0)
        {
          GetItemRect(i-1,lastRect);
        }

        rect.top = (rect.bottom + lastRect.bottom)/2-1;
        if(clrBottomColor==clrUpperColor)
        {
          rect.bottom = rect.top + 1;
        }
        else
        {
          rect.bottom = rect.top + 2;
        }
        rect.right = client.right-2;
        rect.left += 2+2;

        dc.Draw3dRect(rect,clrBottomColor,clrUpperColor);
      }
    }
    else
    {
      if(tbButton.idCommand)
      {
        myDraw.nmcd.uItemState = 0;
        myDraw.nmcd.uItemState |= (tbButton.fsState&TBSTATE_CHECKED)?CDIS_CHECKED:0;
        myDraw.nmcd.uItemState |= (tbButton.fsState&TBSTATE_ENABLED)?0:CDIS_DISABLED;
        myDraw.nmcd.uItemState |= (tbButton.fsState&TBSTATE_INDETERMINATE)?CDIS_INDETERMINATE:0;

        if(bFocus && PtInRect (&myDraw.nmcd.rc,pt))
        {
          if(tbButton.fsState&TBSTATE_PRESSED)
          {
            myDraw.nmcd.uItemState |= CDIS_HOT|CDIS_SELECTED;
          }
          else if (tbButton.fsState&TBSTATE_ENABLED)
          {
            myDraw.nmcd.uItemState |= CDIS_HOT;
          }
        }
        else if(tbButton.fsState&TBSTATE_PRESSED)
        {
          myDraw.nmcd.uItemState |= CDIS_HOT;
        }
        myDraw.rcText = CRect(0,0,0,0);
        if(tbButton.iString!=(-1))
        {
          int nSize = (int)SendMessage(TB_GETBUTTONTEXT,tbButton.idCommand,0);
          if(nSize!=(-1))
          {
            PTCHAR nBuffer = (PTCHAR)_alloca((nSize+2)*sizeof(TCHAR));
            *nBuffer = 0;
            SendMessage(TB_GETBUTTONTEXT,tbButton.idCommand,(LPARAM)nBuffer);
            SIZE size = {0,0};
            VERIFY(::GetTextExtentPoint32(dc.m_hDC,nBuffer,nSize,&size));
            if(dwStyle&TBSTYLE_LIST)
            {// text will be shown right
              int nBorderY = (myDraw.nmcd.rc.bottom-myDraw.nmcd.rc.top-size.cy)/2;
              myDraw.rcText.top = myDraw.nmcd.rc.top+nBorderY+2;
              myDraw.rcText.bottom = myDraw.rcText.top+size.cy;

              if(tbButton.fsStyle&BTNS_DROPDOWN)
              {
                myDraw.rcText.right = myDraw.nmcd.rc.right-13 - 4;
                myDraw.rcText.left  = myDraw.nmcd.rc.left + m_sizeImage.cx + 4;
              }
              else
              {
                myDraw.rcText.right = myDraw.nmcd.rc.right - 2;
                myDraw.rcText.left  = myDraw.nmcd.rc.left  + m_sizeImage.cx + 4;
              }
            }
            else
            {// text will be shown below
              int nBorderX = (myDraw.nmcd.rc.right-myDraw.nmcd.rc.left-size.cx)/2;
              if(tbButton.fsStyle&BTNS_DROPDOWN)
              {
                nBorderX -= 7;
              }
              myDraw.rcText.left = myDraw.nmcd.rc.left+nBorderX;
              myDraw.rcText.right = myDraw.rcText.left+size.cx;

              myDraw.rcText.bottom = myDraw.nmcd.rc.bottom-1;
              myDraw.rcText.top = myDraw.nmcd.rc.bottom-size.cy;
            }
          }
        }
        myDraw.nmcd.dwItemSpec  = tbButton.idCommand;
        myDraw.nmcd.lItemlParam = tbButton.dwData;

        myDraw.nmcd.dwDrawStage = CDDS_PREPAINT|CDDS_ITEM;
        result = pParent->SendMessage(WM_NOTIFY,myDraw.nmcd.hdr.idFrom,(LPARAM)&myDraw);

        myDraw.nmcd.dwDrawStage = CDDS_POSTPAINT|CDDS_ITEM;
        result = pParent->SendMessage(WM_NOTIFY,myDraw.nmcd.hdr.idFrom,(LPARAM)&myDraw);
      }
    }
  }

  myDraw.nmcd.dwDrawStage = CDDS_POSTPAINT;
  result = pParent->SendMessage(WM_NOTIFY,myDraw.nmcd.hdr.idFrom,(LPARAM)&myDraw);
}

void CNewToolBar::PaintToolBarBackGnd(CDC* pDC)
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

  //// To eliminate small border between menu and client rect
  //MENUINFO menuInfo = {0};
  //menuInfo.cbSize = sizeof(menuInfo);
  //menuInfo.fMask = MIM_BACKGROUND;
  HWND hParent = ::GetParent(m_hWnd);

  // if there is a parent to the parent, then get it
  // Jan-20-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
  // added the check to see if the parent has a parent before resetting the parent handle.
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

    //CBrush *pBrush = CBrush::FromHandle(menuInfo.hbrBack);
    CBrush*pBrush = GetMenuBarBrush();
    if(pBrush)
    {
      // need for win95/98/me
      VERIFY(pBrush->UnrealizeObject());
      CPoint oldOrg = pDC->SetBrushOrg(windowRect.left,0);

      pDC->FillRect(clientRect,pBrush);
      //pDC->FillSolidRect(clientRect,RGB(255,0,0));
      pDC->SetBrushOrg(oldOrg);
    }
    else
    {
      pDC->FillSolidRect(clientRect,CNewMenu::GetMenuBarColor());
    }
  }
}

bool CNewToolBar::SetMenuButtonID(UINT nCommandID)
{
  int nTBIndex = CommandToIndex (nCommandID);
  if(nTBIndex==-1)
  {
    return false;
  }
  return SetMenuButton(nTBIndex);
}

bool CNewToolBar::SetMenuButton (int nIndex)
{
  SendMessage(TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);

  TBBUTTON button;
  if(SendMessage(TB_GETBUTTON,nIndex,(LPARAM)&button))
  {
    SetButtonStyle(nIndex,button.fsStyle | TBBS_DROPDOWN);
    return true;
  }
  return false;
}

bool CNewToolBar::InsertControl (int nIndex, CWnd* pControl, DWORD_PTR dwData)
{
  HWND hWnd = pControl->GetSafeHwnd();
  if(!hWnd)
  {
    return false;
  }
  // Set the same font as in toolbar
  pControl->SetFont(GetFont());

  int nCount = (int)SendMessage(TB_BUTTONCOUNT, 0, 0);
  CRect rect;
  pControl->GetWindowRect(&rect);

  TBBUTTON tbbutton = {0};
  tbbutton.iBitmap = rect.Width();
  tbbutton.idCommand = pControl->GetDlgCtrlID();
  tbbutton.fsStyle = TBSTYLE_SEP;
  tbbutton.iString = -1;
  tbbutton.dwData = dwData;

  if ( nIndex < 0 || nIndex > nCount )
  {
    nIndex = nCount;
  }
  if (SendMessage(TB_INSERTBUTTON, nIndex, (LPARAM)&tbbutton) )
  {
    m_bDelayedButtonLayout = true;
    GetItemRect(nIndex,rect);
    //pControl->SetWindowPos(NULL,rect.top,rect.left,0,0,SWP_NOZORDER|SWP_NOSIZE|SWP_FRAMECHANGED);
    return true;
  }
  return false;
}

CNewToolBar* CNewToolBar::g_pNewToolBar = NULL;
HHOOK CNewToolBar::g_hMsgHook = NULL;

LRESULT CALLBACK CNewToolBar::MenuInputFilter(int code, WPARAM wParam, LPARAM lParam)
{
  return (code==MSGF_MENU && g_pNewToolBar && g_pNewToolBar->OnMenuInput( *((MSG*)lParam) )) ? TRUE : CallNextHookEx(g_hMsgHook, code, wParam, lParam);
}

BOOL CNewToolBar::OnMenuInput(MSG msg)
{
  ASSERT_VALID(this);
  if(msg.message==WM_MOUSEMOVE)
  {
    CPoint pt = msg.lParam;
    if(m_hWnd==::WindowFromPoint(pt))
    {
      ScreenToClient(&pt);
      TOOLINFO toolInfo = {0};
      int nID = (int)OnToolHitTest(pt,&toolInfo);
      if(nID)
      {
        int nTBIndex = (int)CommandToIndex (nID);
        if(nTBIndex!=-1 && m_ActMenuIndex!=nTBIndex)
        {
          GetParentFrame()->PostMessage(WM_CANCELMODE,0,0);
        }
      }
    }
  }
  return FALSE; // pass along...
}

void CNewToolBar::TrackPopupMenu (UINT nID, CMenu* pMenu)
{
  int nTBIndex = CommandToIndex (nID);
  if(pMenu && nTBIndex!=-1)
  {
    CRect rcTBItem;
    GetItemRect (nTBIndex, rcTBItem);
    ClientToScreen (rcTBItem);

    SetButtonStyle (nTBIndex, GetButtonStyle (nTBIndex)|TBBS_INDETERMINATE);
    UpdateWindow();

    // Need to set the rectangle of the button!
    CNewMenu* pNewMenu = DYNAMIC_DOWNCAST(CNewMenu,pMenu);
    if(pNewMenu)
    {
      CNewMenu* pParentMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pNewMenu->GetParent()));
      if(pParentMenu)
      {
        pParentMenu->SetLastMenuRect(rcTBItem);
      }
    }

    // handle pending WM_PAINT messages
    MSG msg;
    while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
    {
      if (!GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
        return;
      DispatchMessage(&msg);
    }

    ASSERT(g_pNewToolBar==NULL);
    ASSERT(g_hMsgHook==NULL);

    m_ActMenuIndex = nTBIndex;
    g_pNewToolBar = this;
    g_hMsgHook = ::SetWindowsHookEx(WH_MSGFILTER, MenuInputFilter, NULL, AfxGetApp()->m_nThreadID);

    TrackPopupMenuSpecial(pMenu,0,rcTBItem,GetParentFrame(),m_dwStyle & CBRS_ORIENT_VERT);

    // uninstall hook
    ::UnhookWindowsHookEx(g_hMsgHook);
    g_pNewToolBar = NULL;
    g_hMsgHook = NULL;
    m_ActMenuIndex = -1;

    SetButtonStyle (nTBIndex, GetButtonStyle (nTBIndex)&~TBBS_INDETERMINATE);
  }
}

COLORREF CNewToolBar::SetTansparentColor(COLORREF transparentColor)
{
  COLORREF oldColor = m_transparentColor;
  m_transparentColor = transparentColor;
  return oldColor;
}

COLORREF CNewToolBar::GetTansparentColor()
{
  return m_transparentColor;
}

LRESULT CNewToolBar::OnReplaceBitmap(WPARAM wParam, LPARAM lParam)
{
  LRESULT result = DefWindowProc ( TB_REPLACEBITMAP, wParam, lParam);
  if(result)
  {
    TBREPLACEBITMAP *pReplaceBitmap = (TBREPLACEBITMAP*)lParam;
    if(pReplaceBitmap)
    {
      TBADDBITMAP addBitmap = {0};
      addBitmap.hInst = pReplaceBitmap->hInstNew;
      addBitmap.nID = pReplaceBitmap->nIDNew;
      int nIndex = (int)DefWindowProc ( TB_ADDBITMAP, pReplaceBitmap->nButtons, (LPARAM)&addBitmap);
      if(nIndex!=-1)
      {
        HBITMAP hBitmap = NULL;
        if(addBitmap.hInst)
        {
          hBitmap = ::LoadBitmap(addBitmap.hInst,MAKEINTRESOURCE(addBitmap.nID));
        }
        else
        {
          hBitmap = (HBITMAP) CopyImage((HANDLE) addBitmap.nID, IMAGE_BITMAP, 0,0,0);
        }
        if(hBitmap && m_ImageList.GetSafeHandle())
        {
          CBitmap bitmap;
          bitmap.Attach(hBitmap);

          // Get the color of the top/lef corner
          COLORREF transparentColor = m_transparentColor;
          if(m_transparentColor==CLR_DEFAULT)
          {
            CDC memDC;
            memDC.CreateCompatibleDC(0);
            CBitmap* pOld = memDC.SelectObject(&bitmap);
            transparentColor = memDC.GetPixel(0,0);
            memDC.SelectObject(pOld);
          }

          CBitmap mask;
          mask.Attach(CreateBitmapMask(bitmap,transparentColor));

          if( m_ImageList.Replace(nIndex,&bitmap,&mask))
          {
            BuildGloomImageList();
            m_ImageListDisabled.Replace(nIndex,&bitmap,&mask);}
        }
      }
    }
  }
  return result;
}


LRESULT CNewToolBar::OnAddBitmap(WPARAM nButtons, LPARAM lParam)
{
  LRESULT result = DefWindowProc ( TB_ADDBITMAP, nButtons, lParam);
  if(result!=-1)
  {
    if( !m_ImageList.GetSafeHandle() || m_ImageList.GetImageCount()<=result)
     {
      TBADDBITMAP *pAddImage = (TBADDBITMAP*)lParam;
      if(pAddImage)
      {
        HBITMAP hBitmap = NULL;
        if(pAddImage->hInst)
        {
          hBitmap = ::LoadBitmap(pAddImage->hInst,MAKEINTRESOURCE(pAddImage->nID));
        }
        else
        {
          hBitmap = (HBITMAP) CopyImage((HANDLE) pAddImage->nID, IMAGE_BITMAP, 0,0,0);
        }
        if(hBitmap)
        {
          CBitmap copyBitmap;
          copyBitmap.Attach(CopyImage( hBitmap, IMAGE_BITMAP, 0,0,0));
          // Get the color of the top/lef corner
          COLORREF transparentColor = m_transparentColor;
          if(m_transparentColor==CLR_DEFAULT)
          {
            CDC memDC;
            memDC.CreateCompatibleDC(0);
            CBitmap* pOld = memDC.SelectObject(&copyBitmap);
            transparentColor = memDC.GetPixel(0,0);
            memDC.SelectObject(pOld);
          }
          CBitmap mask;
          mask.Attach(CreateBitmapMask(hBitmap,transparentColor));

          if(!m_ImageList.GetSafeHandle())
          {
            // we must first create an imagelist
            BITMAP myInfo = {0};
            VERIFY(GetObject(hBitmap,sizeof(myInfo),&myInfo));
            if(!nButtons)
            {
              nButtons = 1;
            }
            m_ImageList.Create((int)(myInfo.bmWidth/nButtons),myInfo.bmHeight,ILC_COLORDDB|ILC_MASK,0,10);
          }
          m_ImageList.Add(&copyBitmap,&mask);

          CBitmap bitmap;
          bitmap.Attach(hBitmap);
          BuildGloomImageList();
          if(!m_ImageListDisabled.GetSafeHandle())
          {
            BITMAP myInfo = {0};
            VERIFY(GetObject(hBitmap,sizeof(myInfo),&myInfo));
            if(!nButtons)
            {
              nButtons = 1;
            }
            m_ImageListDisabled.Create((int)(myInfo.bmWidth/nButtons),myInfo.bmHeight,ILC_COLORDDB|ILC_MASK,0,10);
          }
          m_ImageListDisabled.Add(&bitmap,&mask);
        }
      }
    }
  }
  return result;
}

//LRESULT CNewToolBar::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
//{
//  // handle delay hide/show
//  BOOL bVis = GetStyle() & WS_VISIBLE;
//  UINT swpFlags = 0;
//  if ((m_nStateFlags & delayHide) && bVis)
//    swpFlags = SWP_HIDEWINDOW;
//  else if ((m_nStateFlags & delayShow) && !bVis)
//    swpFlags = SWP_SHOWWINDOW;
//  m_nStateFlags &= ~(delayShow|delayHide);
//  if (swpFlags != 0)
//  {
//    SetWindowPos(NULL, 0, 0, 0, 0, swpFlags|
//      SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
//  }
//
//  // the style must be visible and if it is docked
//  // the dockbar style must also be visible
//  if ((GetStyle() & WS_VISIBLE) &&
//    (m_pDockBar == NULL || (m_pDockBar->GetStyle() & WS_VISIBLE)))
//  {
//    CFrameWnd* pTarget = (CFrameWnd*)GetOwner();
//
//    CNewDialog* pDlg = DYNAMIC_DOWNCAST(CNewDialog,pTarget);
//    if(pDlg)
//    {
//      OnUpdateCmdUI(pTarget, (BOOL)wParam);
//    }
//    else
//    {
//      if (pTarget == NULL || !pTarget->IsFrameWnd())
//        pTarget = GetParentFrame();
//      if (pTarget != NULL)
//        OnUpdateCmdUI(pTarget, (BOOL)wParam);
//    }
//  }
//  return 0L;
//}
