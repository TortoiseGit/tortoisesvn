//------------------------------------------------------------------------------
// File    : NewMenuBar.cpp
// Version : 1.04
// Date    : 25. June 2005
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewMenuBar
//
// For bugreport please add following informations
// - The CNewMenu version number (Example CNewMenu 1.20)
// - Operating system Win95 / WinXP and language (English / Japanese / German etc.)
// - Installed service packs
// - Version of internet explorer (important for CNewToolBar)
// - Short description how to reproduce the bug
// - Pictures/Sample are welcome too
// - You can write in English or German to the above email-address.
// - Have my compiled examples the same effect?
//------------------------------------------------------------------------------
//
// Special / ToDo
// add support for not ownerdrawn menu 
// calculating of doking size is wrong
//
// 25. June 2005 (Version 1.04)
// added compatibility for new security enhancement in CRT (MFC 8.0)
//
// 16. Mai 2005 (Version 1.03)
// Changed drawing for gripper to CMenuTheme
//
// 19. July 2004 (Version 1.02)
// corrected menuposition with multiple monitor support
//
// 27. June 2004 (Version 1.01)
// corrected menuposition, thanks to Manfred Drasch
// docking menu even when you haven't an open child window
// added new messages for get the menu handle "CNewMenuBar::WM_GETMENU"
//
// 1. May 2004 (Version 1.00)
// First draft
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "NewMenuBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

#define CX_GRIPPER  3
#define CY_GRIPPER  3
#define CX_BORDER_GRIPPER 2
#define CY_BORDER_GRIPPER 2
#define CX_GRIPPER_ALL CX_GRIPPER + CX_BORDER_GRIPPER*2
#define CY_GRIPPER_ALL CY_GRIPPER + CY_BORDER_GRIPPER*2

#ifndef HBMMENU_CALLBACK

#define HBMMENU_CALLBACK            ((HBITMAP) -1)
#define HBMMENU_SYSTEM              ((HBITMAP)  1)
#define HBMMENU_MBAR_RESTORE        ((HBITMAP)  2)
#define HBMMENU_MBAR_MINIMIZE       ((HBITMAP)  3)
#define HBMMENU_MBAR_CLOSE          ((HBITMAP)  5)
#define HBMMENU_MBAR_CLOSE_D        ((HBITMAP)  6)
#define HBMMENU_MBAR_MINIMIZE_D     ((HBITMAP)  7)
#define HBMMENU_POPUP_CLOSE         ((HBITMAP)  8)
#define HBMMENU_POPUP_RESTORE       ((HBITMAP)  9)
#define HBMMENU_POPUP_MAXIMIZE      ((HBITMAP) 10)
#define HBMMENU_POPUP_MINIMIZE      ((HBITMAP) 11)

#define WM_CHANGEUISTATE                0x0127
#define WM_UPDATEUISTATE                0x0128
#define WM_QUERYUISTATE                 0x0129

/*
* LOWORD(wParam) values in WM_*UISTATE*
*/
#define UIS_SET                         1
#define UIS_CLEAR                       2
#define UIS_INITIALIZE                  3

/*
* HIWORD(wParam) values in WM_*UISTATE*
*/
#define UISF_HIDEFOCUS                  0x1
#define UISF_HIDEACCEL                  0x2

#define SPI_GETKEYBOARDCUES                 0x100A

#define ODS_HOTLIGHT        0x0040
#define ODS_INACTIVE        0x0080
#define ODS_NOACCEL         0x0100

#endif // HBMMENU_CALLBACK

#ifndef _WIN64

#ifdef SetWindowLongPtrA
#undef SetWindowLongPtrA
#endif
inline LONG_PTR SetWindowLongPtrA( HWND hWnd, int nIndex, LONG_PTR dwNewLong )
{
  return( ::SetWindowLongA( hWnd, nIndex, LONG( dwNewLong ) ) );
}

#ifdef SetWindowLongPtrW
#undef SetWindowLongPtrW
#endif
inline LONG_PTR SetWindowLongPtrW( HWND hWnd, int nIndex, LONG_PTR dwNewLong )
{
  return( ::SetWindowLongW( hWnd, nIndex, LONG( dwNewLong ) ) );
}

#ifndef SetWindowLongPtr
#ifdef UNICODE
#define SetWindowLongPtr  SetWindowLongPtrW
#else
#define SetWindowLongPtr  SetWindowLongPtrA
#endif // !UNICODE
#endif //SetWindowLongPtr

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC        (-4)
#endif

#endif  //_WIN64 

//////////////////////////////////////////////////////////////////////
// Construction/Destruction CNewBarItem
//////////////////////////////////////////////////////////////////////

CNewBarItem::CNewBarItem()
: m_nSyncFlag(0),
m_nState(0)
{
}

CRect CNewBarItem::GetRect(BOOL bVertical) const
{
  if(bVertical)
  {
    return CRect(CPoint(m_ItemRect.top,m_ItemRect.left),CSize(m_ItemRect.Height(),m_ItemRect.Width()));
  }
  else
  {
    return m_ItemRect;
  }
}

int CNewBarItem::HitTest(CPoint point,BOOL bVertical) const
{
  return GetRect(bVertical).PtInRect(point);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction CNewMenuBarItem
//////////////////////////////////////////////////////////////////////
CNewMenuBarItem::CNewMenuBarItem(HMENU hMenu, UINT nIndex)
: m_hMenu(hMenu),
m_nIndex(nIndex)
{
}

HMENU CNewMenuBarItem::GetMenu(HWND hWndOwner)
{
  UNREFERENCED_PARAMETER(hWndOwner);
  return GetSubMenu(m_hMenu,m_nIndex);
}

void CNewMenuBarItem::DrawBarItem(HWND hWndOwner, HDC hDC)
{
  MENUITEMINFO MenuItemInfo = {0};
  MenuItemInfo.cbSize = sizeof(MenuItemInfo);
  MenuItemInfo.fMask = MIIM_DATA|MIIM_TYPE|MIIM_ID;
  if(GetMenuItemInfo(m_hMenu,m_nIndex,TRUE,&MenuItemInfo))
  {
    if(MenuItemInfo.fType&MF_OWNERDRAW)
    {
      DRAWITEMSTRUCT drawItem = {0};

      drawItem.hDC = hDC;
      drawItem.CtlType = ODT_MENU;
      drawItem.itemID = MenuItemInfo.wID;
      drawItem.itemState = m_nState;
      drawItem.hwndItem = (HWND)m_hMenu;
      drawItem.itemData = MenuItemInfo.dwItemData;
      drawItem.rcItem = GetRect(m_nState&ODS_DRAW_VERTICAL);
      drawItem.itemAction = m_nState|ODA_DRAWENTIRE;

      ::SendMessage(hWndOwner,WM_DRAWITEM,0,(LPARAM)&drawItem);
    }
    else
    {
      CRect rect = m_ItemRect;
      switch((DWORD)MenuItemInfo.dwTypeData)
      {
      case HBMMENU_SYSTEM:
      case HBMMENU_MBAR_RESTORE:
      case HBMMENU_MBAR_MINIMIZE_D:
      case HBMMENU_MBAR_MINIMIZE:
      case HBMMENU_MBAR_CLOSE_D:
      case HBMMENU_MBAR_CLOSE:

      default:
        break;
      }
    }
  }
}

UINT CNewMenuBarItem::GetMenuItemID()
{
  return ::GetMenuItemID(m_hMenu,m_nIndex);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction CNewIconBarItem
//////////////////////////////////////////////////////////////////////
CNewIconBarItem::CNewIconBarItem(HBITMAP hBitmap,UINT uCommandID)
: m_hBitmap(hBitmap),
m_uCommandID(uCommandID)
{
  BITMAP myInfo = {0};
  if(m_hBitmap>HBMMENU_POPUP_MINIMIZE && 
     m_hBitmap!=HBMMENU_CALLBACK &&
     GetObject(m_hBitmap,sizeof(myInfo),&myInfo))
  {
    m_ItemRect = CRect(0,0,myInfo.bmWidth+4,myInfo.bmHeight+4);
  }
  else
  {
  // Dimensions of menu bar buttons, such as the child window close
  // button used in the multiple document interface, in pixels.
  m_ItemRect = CRect(0,0,GetSystemMetrics(SM_CYMENUSIZE),GetSystemMetrics(SM_CXMENUSIZE));
}
}

CRect CNewIconBarItem::GetRect(BOOL bVertical) const
{
  if(bVertical)
  {
    return CRect(CPoint(m_ItemRect.top,m_ItemRect.left),CSize(m_ItemRect.Width(),m_ItemRect.Height()));
  }
  // we do not rotate images at the moment
  return m_ItemRect;
}


UINT CNewIconBarItem::GetMenuItemID()
{
  return m_uCommandID;
}

inline HWND GetMDIClient(HWND hWndOwner)
{
  CWnd *pFrame = CWnd::FromHandle(hWndOwner);
  if(pFrame)
  {
    CMDIFrameWnd *pMDIFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd,pFrame->GetTopLevelFrame());
    if(pMDIFrame)
    {
      return (HWND)::SendMessage(pMDIFrame->m_hWndMDIClient,WM_MDIGETACTIVE, 0, NULL);
    }
  }
  return NULL;
}

HMENU CNewIconBarItem::GetMenu(HWND hWndOwner)
{
  if(m_hBitmap==HBMMENU_SYSTEM)
  {
    HWND hMDIClient = GetMDIClient(hWndOwner);
    if(hMDIClient)
    {
      return GetSystemMenu(hMDIClient,FALSE);
    }
  }
  return NULL; //GetSubMenu(m_hMenu,m_nIndex);
}

void CNewIconBarItem::DrawSpecialChar(HDC hDC, LPCRECT pRect, TCHAR Sign, BOOL bBold, BOOL bDisabled)
{
  CDC* pDC = CDC::FromHandle(hDC);
  //  48 Min
  //  49 Max
  //  50 Restore
  //  98 Checkmark 
  // 105 Bullet
  // 114 Close

  CFont MyFont;
  LOGFONT logfont;

  CRect rect(pRect) ;
  rect.DeflateRect(2,2);

  logfont.lfHeight = -rect.Height();
  logfont.lfWidth = 0;
  logfont.lfEscapement = 0;
  logfont.lfOrientation = 0;
  logfont.lfWeight = (bBold) ? /*FW_BOLD:*/FW_NORMAL:FW_EXTRALIGHT;
  logfont.lfItalic = FALSE;
  logfont.lfUnderline = FALSE;
  logfont.lfStrikeOut = FALSE;
  logfont.lfCharSet = DEFAULT_CHARSET;
  logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
  logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality = DEFAULT_QUALITY;
  logfont.lfPitchAndFamily = DEFAULT_PITCH;

  _tcscpy_s(logfont.lfFaceName,ARRAY_SIZE(logfont.lfFaceName),_T("Marlett"));

  MyFont.CreateFontIndirect (&logfont);

  COLORREF oldColor = CLR_NONE;
  if(bDisabled)
  {
    oldColor = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT)); 
  }
  CFont* pOldFont = pDC->SelectObject (&MyFont); 
  int OldMode = pDC->SetBkMode(TRANSPARENT);  

  pDC->DrawText (&Sign,1,rect,DT_CENTER|DT_SINGLELINE);

  pDC->SetBkMode(OldMode);
  pDC->SelectObject(pOldFont);
  if(bDisabled)
  {
    pDC->SetTextColor(oldColor); 
  }
}

void CNewIconBarItem::PaintHotButton(HDC hDC)
{ 
  CRect rect = GetRect(m_nState&ODS_DRAW_VERTICAL);
  CDC* pDC = CDC::FromHandle(hDC);

  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
      if(m_nState&ODS_HOTLIGHT)
      {
        if(m_nState&ODS_SELECTED)
        {
          //dunkler oranger rahmen
          DrawGradient(pDC,rect,RGB(242,151,107),RGB(249,202,163),false);
        }
        else
        {
          //heller oranger rahmen 
          DrawGradient(pDC,rect,RGB(250,239,219),RGB(255,212,151),false);
        } 
        COLORREF colorBorder = RGB(4, 2, 132);
        pDC->Draw3dRect(rect,colorBorder,colorBorder);  
      }
      break;

  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_XP_2003:
    if(IsMenuThemeActive())
    {
      if(m_nState&ODS_HOTLIGHT)
      {
        if(m_nState&ODS_SELECTED)
        {
          //dunkler oranger rahmen
          DrawGradient(pDC,rect,RGB(242,151,107),RGB(249,202,163),false);
        }
        else
        {
          //heller oranger rahmen 
          DrawGradient(pDC,rect,RGB(250,239,219),RGB(255,212,151),false);
        } 
        COLORREF colorBorder = RGB(4, 2, 132);
        pDC->Draw3dRect(rect,colorBorder,colorBorder);  
      }
      break;
    }
    // fall throught when no theme!

  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    {
      if (m_nState&CDIS_CHECKED ||m_nState&ODS_HOTLIGHT)
      {
        COLORREF colorSel = GetXpHighlightColor();
        COLORREF colorBorder = GetSysColor(COLOR_HIGHLIGHT);
        pDC->FillSolidRect(rect,colorSel);
        pDC->Draw3dRect(rect,colorBorder,colorBorder);
      }
    }
    break;

  case CNewMenu::STYLE_ICY:
  case CNewMenu::STYLE_ICY_NOBORDER:
    {
      COLORREF colorMid = GetSysColor(COLOR_HIGHLIGHT);
      COLORREF colorSel = LightenColor(60,colorMid);
      COLORREF colorBorder = DarkenColor(60,colorMid);

      if(m_nState&ODS_HOTLIGHT)
      {
        if(m_nState&ODS_SELECTED)
        {
          DrawGradient(pDC,rect,colorMid,colorBorder,false);
        }
        else
        {
          DrawGradient(pDC,rect,colorSel,colorMid,false);
        }
        pDC->Draw3dRect(rect,colorBorder,colorBorder);
      }
    }
    break;


  default:
    {
      if(m_nState&ODS_HOTLIGHT )
      {
        if(m_nState&ODS_SELECTED)
        {
          pDC->Draw3dRect(rect,GetSysColor(COLOR_BTNSHADOW),GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
          pDC->Draw3dRect(rect,GetSysColor(COLOR_HIGHLIGHTTEXT),GetSysColor(COLOR_BTNSHADOW));
        }
      }
    }
    break;
  }
}


void CNewIconBarItem::DrawBarItem(HWND hWndOwner, HDC hDC)
{
  PaintHotButton(hDC);

  CRect rect = GetRect(m_nState&ODS_DRAW_VERTICAL);
  rect.InflateRect(-2,-2);
  //rect.OffsetRect(2,2);
  switch((DWORD)m_hBitmap)
  {
  case HBMMENU_SYSTEM:
    {
      rect.OffsetRect(2,2);
      HWND hMDIClient = GetMDIClient(hWndOwner);
      if(hMDIClient)
      {
        HICON hIcon = (HICON)GetClassLongPtr (hMDIClient, GCLP_HICONSM);
        if(hIcon)
        {
          DrawState(hDC,NULL,NULL,(LPARAM)hIcon,NULL,rect.left-2,rect.top-2,20,20,DST_ICON);
        }
      }
    }
    break;

  case HBMMENU_MBAR_MINIMIZE_D:
  case HBMMENU_MBAR_MINIMIZE:
    {
      HWND hMDIClient = GetMDIClient(hWndOwner);
      DrawSpecialChar(hDC,rect,48,true,!(GetWindowLong(hMDIClient,GWL_STYLE)&WS_MINIMIZEBOX));
    }
    break;

  case HBMMENU_MBAR_RESTORE:
    DrawSpecialChar(hDC,rect,50,true,FALSE);
    break;

  case HBMMENU_MBAR_CLOSE_D:
  case HBMMENU_MBAR_CLOSE:
    {
      BOOL bDisabled = FALSE;
      HWND hMDIClient = GetMDIClient(hWndOwner);
      if(hMDIClient)
      {
        HMENU hMenu = GetSystemMenu(hMDIClient,FALSE);
        UINT nMenuFlag = GetMenuState(hMenu,SC_CLOSE,MF_BYCOMMAND);
        if(nMenuFlag==-1 || nMenuFlag&MF_DISABLED)
        {
          bDisabled = TRUE;
        }
      }
      DrawSpecialChar(hDC,rect,114,true,bDisabled);
    }
    break;

  default:
    DrawState(hDC,NULL,NULL,(LPARAM)m_hBitmap,NULL,rect.left,rect.top,rect.Width(),rect.Height(),DST_BITMAP);
    break;
  }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction CNewMenuBar
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMenuBar, CControlBar)

// Special messages for handling menu
const UINT CNewMenuBar::WM_REMOVEMENU = ::RegisterWindowMessage(_T("CNewMenuBar::WM_REMOVEMENU"));
const UINT CNewMenuBar::WM_GETMENU = ::RegisterWindowMessage(_T("CNewMenuBar::WM_GETMENU"));

CNewMenuBar::CNewMenuBar()
: m_pOldFrameProc(NULL),
  m_hMenuBar(NULL),
  m_nActualSelIndex(-1),
  m_bProcessLeftArrow(FALSE),
  m_bProcessRightArrow(FALSE),
  m_bLoop(FALSE),
  m_bIgnoreAltKey(FALSE),
  m_nCurIndex(-1),
  m_ptMouse(0,0),
  m_bInMemuLoop(FALSE),
  m_TimerID(NULL),
  m_eTrackingState(TR_NONE),
  m_bDelayedButtonLayout(FALSE),
  m_nMouseIndex(-1),
  m_hMenuPopup(NULL),
  m_hWndCommand(NULL)
{
  m_cxLeftBorder = 3;
  m_cxRightBorder = 0;
  	
  m_cyTopBorder = 0;
  m_cyBottomBorder = 1;
}

CNewMenuBar::~CNewMenuBar()
{
}

BOOL CNewMenuBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
  return CreateEx(pParentWnd, 0, dwStyle,CRect(m_cxLeftBorder, m_cyTopBorder, m_cxRightBorder, m_cyBottomBorder), nID);
}

BOOL CNewMenuBar::CreateEx(CWnd* pParentWnd, DWORD dwCtrlStyle, DWORD dwStyle, CRect rcBorders, UINT nID)
{
  ASSERT_VALID(pParentWnd);   // must have a parent
  ASSERT (!((dwStyle & CBRS_SIZE_FIXED) && (dwStyle & CBRS_SIZE_DYNAMIC)));

  SetBorders(rcBorders);

  // save the style
  m_dwStyle = (dwStyle & CBRS_ALL);
  if (nID == AFX_IDW_DOCKBAR_MENU)
    m_dwStyle |= CBRS_HIDE_INPLACE;

  dwStyle &= ~CBRS_ALL;
  dwStyle |= dwCtrlStyle;

  // initialize common controls
  CString strClass = AfxRegisterWndClass( CS_HREDRAW | CS_VREDRAW |CS_DBLCLKS,
    AfxGetApp()->LoadStandardCursor(IDC_ARROW),
    (HBRUSH)(COLOR_BTNFACE+1));

  // create the HWND
  CRect rect; rect.SetRectEmpty();
  return CWnd::Create(strClass,_T("Menu Bar"), dwStyle, rect, pParentWnd, nID);
}

CSize CNewMenuBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
  DWORD dwMode = bStretch ? LM_STRETCH : 0;
  dwMode |= bHorz ? LM_HORZ : 0;

  return CalcLayout(dwMode);
}

CSize CNewMenuBar::CalcDynamicLayout(int nLength, DWORD dwMode)
{
  if ((nLength == -1) &&
    !(dwMode & LM_MRUWIDTH) &&
    !(dwMode & LM_COMMIT) &&
    ((dwMode & LM_HORZDOCK) || (dwMode & LM_VERTDOCK)))
  {
    return CalcFixedLayout(dwMode & LM_STRETCH, dwMode & LM_HORZDOCK);
  }
  return CalcLayout(dwMode, nLength);
}

CSize CNewMenuBar::CalcLayout(DWORD dwMode, int nLength)
{
  ASSERT_VALID(this);
  ASSERT(::IsWindow(m_hWnd));
  if (dwMode & LM_HORZDOCK)
  {
    ASSERT(dwMode & LM_HORZ);
  }

  // make SC_CLOSE button disable
  if (m_dwStyle & CBRS_FLOATING)
  {
    CFrameWnd* pMiniFrame = GetParentFrame();
    ASSERT_KINDOF(CMiniFrameWnd, pMiniFrame);
    CMenu* pSysMenu = pMiniFrame->GetSystemMenu(FALSE);
    ASSERT_VALID(pSysMenu);
    pSysMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
  }

  MBITEM* pData = NULL;
  CSize sizeResult(0,0);
  int nCount = GetItemCount();

  if(nCount > 0)
  {
    //BLOCK: Load Buttons
    pData = new MBITEM[nCount];
    for(int i = 0; i < nCount; i++)
    {
      pData[i].rcItem = m_NewBarItemList[i]->m_ItemRect;
      pData[i].nState = m_NewBarItemList[i]->m_nState;
    }

    if (!(m_dwStyle & CBRS_SIZE_FIXED))
    {
      BOOL bDynamic = m_dwStyle & CBRS_SIZE_DYNAMIC;
      if (bDynamic && (dwMode & LM_MRUWIDTH))
      {
        SizeMenuBar(pData, nCount, m_nMRUWidth);
        sizeResult = CalcSize(pData,nCount);
        UpdateItemLayout(pData,nCount);
      }
      else if (bDynamic && (dwMode & LM_HORZDOCK))
      {
        int nLen = GetClipBoxLength(dwMode & LM_HORZDOCK);
        SizeMenuBar(pData, nCount, nLen);
        sizeResult = CalcSize(pData,nCount);
        sizeResult.cx = nLen;
      }
      else if (bDynamic && (dwMode & LM_VERTDOCK))
      {
        int nLen = GetClipBoxLength(dwMode & LM_HORZDOCK);
        SizeMenuBar(pData, nCount, nLen);
        sizeResult = CalcSize(pData, nCount);

        long temp = sizeResult.cx;
        sizeResult.cx = sizeResult.cy;
        sizeResult.cy = temp;
      }
      else if (bDynamic && (nLength != -1))
      {
        CRect rect(0,0,0,0);
        CalcInsideRect(rect, (dwMode & LM_HORZ));
        BOOL bVert = (dwMode & LM_LENGTHY);
        int nLen = nLength + (bVert ? rect.Height() : rect.Width());

        SizeMenuBar(pData, nCount, nLen, bVert);
        sizeResult = CalcSize(pData, nCount);
      }
      else if (bDynamic && (m_dwStyle & CBRS_FLOATING))
      {
        SizeMenuBar( pData, nCount, m_nMRUWidth);
        sizeResult = CalcSize(pData, nCount);
      }
      else
      {
        if(!(m_dwStyle & CBRS_FLOATING))
        {
          int nLen = GetClipBoxLength(dwMode & LM_HORZ);
          SizeMenuBar(pData, nCount, nLen);
          sizeResult = CalcSize(pData, nCount);
          UpdateItemLayout(pData,nCount);

          if(dwMode & LM_HORZ)
          {
            sizeResult.cx = 32767;
          }
          else
          {
            sizeResult.cx = sizeResult.cy;
            sizeResult.cy = 32767;
          }
        }
        else
        {
          sizeResult = CalcSize(pData, nCount);
          //CalcItemLayout(nCount);
          UpdateItemLayout(pData,nCount);
        }
      }
    }

    if (dwMode & LM_COMMIT)
    {
      BOOL bIsDelayed = m_bDelayedButtonLayout;
      m_bDelayedButtonLayout = FALSE;

      if ((m_dwStyle & CBRS_FLOATING) && (m_dwStyle & CBRS_SIZE_DYNAMIC))
        m_nMRUWidth = sizeResult.cx;

      UpdateItemLayout(pData,nCount);
      m_bDelayedButtonLayout = bIsDelayed;
    }
    delete[] pData;
  }

  //BLOCK: Adjust Margins
  {
    CRect rect(0,0,0,0);
    CalcInsideRect(rect, (dwMode & LM_HORZ));
    sizeResult.cy -= rect.Height();
    sizeResult.cx -= rect.Width();

    CSize size = CControlBar::CalcFixedLayout((dwMode & LM_STRETCH), (dwMode & LM_HORZ));
    sizeResult.cx = max(sizeResult.cx, size.cx);
    sizeResult.cy = max(sizeResult.cy, size.cy);
  }
  // fix for NT 4.0 or win95/98/ME
  sizeResult.cx = min(sizeResult.cx,0x7f00);
  sizeResult.cy = min(sizeResult.cy,0x7f00);
  //AfxTrace(_T("CalcLayout: Mode 0x%lX, Lenght %ld, Size(%ld,%ld)\n"),dwMode, nLength,sizeResult.cx,sizeResult.cy);
  return sizeResult;
}

void CNewMenuBar::CorrectItemLayout(MBITEM* pData, int nFrom, int nTo, int nWidth)
{
  if(nWidth>0 && nFrom>=0 && nFrom<=nTo)
  {
    int nOffset = nWidth - pData[nTo].rcItem.right;
    for(;nFrom<=nTo;nFrom++)
    {
      pData[nFrom].rcItem.OffsetRect(nOffset,0);
    }
  }
}

void CNewMenuBar::UpdateItemLayout(MBITEM* pData, int nCount)
{
  int i;
  for (i = 0; i < nCount; i++)
  {
    CNewBarItem* pItem = m_NewBarItemList[i];
    pItem->m_ItemRect = pData[i].rcItem;
    pItem->m_nState = pData[i].nState;
  }
  Invalidate();
}

CSize CNewMenuBar::CalcSize(MBITEM* pData, int nCount)
{
  ASSERT(nCount > 0);
  CPoint cur(0,0);
  CSize sizeResult(0,0);
  for (int i = 0; i < nCount; ++i)
  {
    if (pData[i].nState& ODS_WRAP)
    {
      cur.x = 0;// reset x pos
      cur.y = sizeResult.cy;
    }
    sizeResult.cx = max(cur.x + pData[i].rcItem.Width(),  sizeResult.cx);
    sizeResult.cy = max(cur.y + pData[i].rcItem.Height(), sizeResult.cy);

    cur.x += pData[i].rcItem.Width();
    ASSERT(sizeResult.cx<0x7fff);
    ASSERT(sizeResult.cy<0x7fff);
  }
  return sizeResult;
}

void CNewMenuBar::SizeMenuBar(MBITEM* pData, int nCount, int nLength, BOOL bVert)
{
  // we calculate horizontal and vertical menu on the same way
  UNREFERENCED_PARAMETER(bVert);
  ASSERT(nCount > 0);

  if (IsFloating())
  {   // half size wrapping
    CSize sizeMax, sizeMin, sizeMid;

    // Wrap MenuBar vertically
    WrapMenuBar(pData,nCount, 0);
    sizeMin = CalcSize(pData,nCount);

    // Wrap MenuBar horizontally
    WrapMenuBar(pData,nCount, 32767);
    sizeMax = CalcSize(pData,nCount);

    while (sizeMin.cx < sizeMax.cx)
    {
      sizeMid.cx = (sizeMin.cx + sizeMax.cx) / 2;
      WrapMenuBar(pData,nCount, sizeMid.cx);
      sizeMid = CalcSize(pData,nCount);
      if (sizeMid.cx == sizeMax.cx)
      { 
        // if you forget, it loops forever!
        return;
      }
      if (nLength >= sizeMax.cx)
      {
        if (sizeMin == sizeMid)
        {
          WrapMenuBar(pData,nCount, sizeMax.cx);
          return;
        }
        sizeMin = sizeMid;
      }
      else if (nLength < sizeMax.cx)
      {
        sizeMax = sizeMid;
      }
      else
      {
        return;
      }
    }
  }
  else
  { // each one wrapping
    WrapMenuBar(pData,nCount, nLength);
  }
}

int CNewMenuBar::WrapMenuBar(MBITEM* pData, int nCount, int nWidth)
{
  int nResult = 0;
  CPoint pos(0,0);
  int nMax_y=0;
  int nLastIndex=-1;

  for (int i = 0; i < nCount; i++)
  {
    MBITEM* pItem = pData+i;
    pItem->nState &= ~ODS_WRAP;

    if ( (pos.x+pItem->rcItem.Width())> nWidth && pos.x)
    {// itself is over
      // Put it on a new line
      if(nLastIndex!=-1)
      {
        CorrectItemLayout(pData,nLastIndex,i-1, nWidth);
        nLastIndex = i;
      }

      pItem->nState |= ODS_WRAP;
      ++nResult;
      pos.x = 0;
      pos.y = nMax_y;
    }
    //move the item
    pItem->rcItem = CRect(pos,pItem->rcItem.Size());
    pos.x+=pItem->rcItem.Width();
    nMax_y = max(pos.y+pItem->rcItem.Height(),nMax_y);

    if(nLastIndex==-1 && pItem->nState&ODS_RIGHTJUSTIFY)
    {
      nLastIndex = i;
    }
  }
  CorrectItemLayout(pData,nLastIndex,nCount-1, nWidth);

  return nResult + 1;
}

int CNewMenuBar::GetClipBoxLength(BOOL bHorz)
{
  if (m_dwStyle & CBRS_SIZE_DYNAMIC)
  {
    int nFrameBorder = bHorz?GetSystemMetrics(SM_CXFRAME):GetSystemMetrics(SM_CYFRAME);
    int nNonClient = nFrameBorder*2 + m_cxLeftBorder + m_cxRightBorder;

    if (m_dwStyle & CBRS_GRIPPER)
    {
      nNonClient += bHorz?CX_GRIPPER_ALL:CY_GRIPPER_ALL;
    }
    if (IsFloating())
    {
      CFrameWnd* pFrame = GetTopLevelFrame();
      ASSERT_VALID(pFrame);
      CRect rcFrame;
      pFrame->GetWindowRect(rcFrame);

      return (bHorz)?rcFrame.Width()-nNonClient : rcFrame.Height()-nNonClient;
    }
    else
    {
      CRect rcParent;
      const MSG* pMsg = GetCurrentMessage();
      if(pMsg && pMsg->message==WM_SIZEPARENT)
      {
        AFX_SIZEPARENTPARAMS* lpLayout = (AFX_SIZEPARENTPARAMS*)pMsg->lParam;
        rcParent = lpLayout->rect;
        //m_bLayoutQuery = (lpLayout->hDWP == NULL);
      }
      else
      {
        CWnd* pParent = GetParent();
        ASSERT_VALID(pParent);
        pParent->GetWindowRect(rcParent);
      }
      return (bHorz)?rcParent.Width()-nNonClient : rcParent.Height()-nNonClient;
    }
  }
  else
  {
    CRect rect;
    GetClientRect(rect);
    return bHorz ? rect.Width() : rect.Height();
  }
}

BEGIN_MESSAGE_MAP(CNewMenuBar, CControlBar)
  ON_WM_CREATE()
  ON_WM_DESTROY()
  ON_WM_TIMER()
  ON_WM_NCPAINT()
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_MOUSEMOVE()
  ON_WM_LBUTTONDOWN()
  ON_WM_LBUTTONUP()
  ON_WM_NCCALCSIZE()
  ON_WM_NCHITTEST()
END_MESSAGE_MAP()

// an map of actual opened Menu and submenu
CTypedPtrMap<CMapPtrToPtr,HWND,CNewMenuBar*> CNewMenuBar::m_NewMenuBar;

LRESULT CALLBACK CNewMenuBar::FrmWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef _USRDLL
  // If this is a DLL, need to set up MFC state
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#else
  AFX_MANAGE_STATE(AfxGetModuleState());
#endif

  _AFX_THREAD_STATE* pState = AfxGetThreadState();
  MSG oldState = pState->m_lastSentMsg;   // save for nesting
  pState->m_lastSentMsg.hwnd = hWnd;
  pState->m_lastSentMsg.message = nMsg;
  pState->m_lastSentMsg.wParam = wParam;
  pState->m_lastSentMsg.lParam = lParam;

  LRESULT lResult = NULL;

  CNewMenuBar* pMenuBar=NULL;
  if(m_NewMenuBar.Lookup(hWnd,pMenuBar))
  {
    lResult = pMenuBar->FrameWindowProc(hWnd,nMsg,wParam,lParam);
  }
  pState->m_lastSentMsg = oldState;

  return lResult;
}

LRESULT CALLBACK CNewMenuBar::CilWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef _USRDLL
  // If this is a DLL, need to set up MFC state
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#else
  AFX_MANAGE_STATE(AfxGetModuleState());
#endif

  _AFX_THREAD_STATE* pState = AfxGetThreadState();
  MSG oldState = pState->m_lastSentMsg;   // save for nesting
  pState->m_lastSentMsg.hwnd = hWnd;
  pState->m_lastSentMsg.message = nMsg;
  pState->m_lastSentMsg.wParam = wParam;
  pState->m_lastSentMsg.lParam = lParam;

  LRESULT lResult = NULL;

  CNewMenuBar* pMenuBar=NULL;
  if(m_NewMenuBar.Lookup(hWnd,pMenuBar))
  {
    lResult = pMenuBar->ChildWindowProc(hWnd,nMsg,wParam,lParam);
  }
  pState->m_lastSentMsg = oldState;

  return lResult;
}

LRESULT CNewMenuBar::FrameWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
  switch(nMsg)
  {
  case WM_DESTROY:
    {
      VERIFY(SetWindowLongPtr(hWnd, GWLP_WNDPROC, (INT_PTR)m_pOldFrameProc));
      m_NewMenuBar.RemoveKey(hWnd);
      WNDPROC FrameProc = m_pOldFrameProc;
      m_pOldFrameProc = NULL;
      return ::CallWindowProc(FrameProc, hWnd,nMsg,wParam,lParam);
    }
    break;

  //case WM_SYSCOMMAND:
  //  {
  //    LRESULT result = FrameDefault();
  //    if(wParam==SC_RESTORE || wParam==SC_MAXIMIZE)
  //    {
  //      RefreshBar();
  //    }
  //    return result; 
  //  }
  //  break;

  case WM_SYSCOLORCHANGE:
  case WM_SETTINGCHANGE:
    CNewMenu::OnSysColorChange();
    UpdateMenuBarColor(m_hMenuBar);
    RefreshBar();
    break;

  case WM_MEASUREITEM:
    if(!CNewMenu::OnMeasureItem(GetCurrentMessage()))
    {
      LPMEASUREITEMSTRUCT pMeasureItem = (LPMEASUREITEMSTRUCT)lParam;
      if(pMeasureItem)
      {
        CMenu* pMenu = CNewMenu::FindPopupMenuFromID(m_hMenuBar,pMeasureItem->itemID);
        if(pMenu)
        {
          pMenu->MeasureItem(pMeasureItem);
          return NULL;
        }
      }
    }
    break;

    //case WM_NEXTMENU:
    //  {
    //    MDINEXTMENU *pNext = (MDINEXTMENU*)lParam;
    //    FrameDefault();
    //    if(wParam==VK_LEFT)
    //    {
    //      if(m_NewBarItemList.GetCount())
    //      {
    //        pNext->hmenuNext = m_hMenuPopup;//m_NewBarItemList[1]->GetMenu(m_hWndOwner);
    //        pNext->hwndNext = m_hWnd;
    //      }
    //    }
    //    return 0;
    //  }
    //  break;

  case WM_INITMENUPOPUP:
    // Very important: must let frame window handle it first!
    // Because if someone calls CCmdUI::SetText, MFC will change item to
    // MFT_STRING, so I must change back to MFT_OWNERDRAW.
    //
    {
      FrameDefault();
      // We must set the right handle for updating the mainframe window 
      // it is already handled in the frame window
      //CNewMenu::OnInitMenuPopup(hWnd,CMenu::FromHandle((HMENU)wParam),(UINT)LOWORD(lParam), (BOOL)HIWORD(lParam));
      m_bProcessLeftArrow = TRUE;
    }
    return 0;

  case WM_MENUSELECT:
    {
      HMENU hMenu = (HMENU)lParam;
      UINT nIndex = (UINT)LOWORD(wParam);
      m_bProcessRightArrow = (::GetSubMenu(hMenu, nIndex) == NULL);
      m_bProcessLeftArrow = (hMenu == m_hMenuPopup);
    }
    break;

  case WM_MENUCHAR:
    {
      UINT nChar = (TCHAR)LOWORD(wParam);
      UINT nFlags = (UINT)HIWORD(wParam);
      CNewMenu* pMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandle((HMENU)lParam));
      if( pMenu )
      {
        LRESULT lr = CNewMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
        if (lr!=0)
        {
          return lr;
        }
      }
    }
    break;

  default:
    if(nMsg==WM_GETMENU)
    {
      return (LRESULT)m_hMenuBar;
    }
    else if(nMsg==WM_REMOVEMENU)
    {
      return (LRESULT)::SetMenu(hWnd,NULL);
    }
    break;
  }
  return FrameDefault();
}

LRESULT CNewMenuBar::FrameDefault()
{
  // MFC stores current MSG in thread state
  MSG& curMsg = AfxGetThreadState()->m_lastSentMsg;
  ASSERT(m_pOldFrameProc);
  return ::CallWindowProc(m_pOldFrameProc, curMsg.hwnd,curMsg.message, curMsg.wParam, curMsg.lParam);
}

// Override this to handle messages in specific handlers
LRESULT CNewMenuBar::ChildWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
  switch(nMsg)
  {
  case WM_DESTROY:
    {
      VERIFY(SetWindowLongPtr(hWnd, GWLP_WNDPROC, (INT_PTR)m_pOldChildProc));
      m_NewMenuBar.RemoveKey(hWnd);
      WNDPROC ChildProc = m_pOldChildProc;
      m_pOldChildProc = NULL;
      return ::CallWindowProc(ChildProc, hWnd,nMsg,wParam,lParam);
    }
    break;

  case WM_MDISETMENU:
    // only sent to MDI client window
    // Setting new frame/window menu: bypass MDI. wParam is new menu.
    if (::GetMenu(m_hWndOwner) != NULL)
    {
      ::SetMenu(m_hWndOwner,NULL);
    }
    // but let the window to handle the window menu
    ::CallWindowProc(m_pOldChildProc, hWnd,nMsg,NULL,lParam);
    OnMDISetMenu((HMENU)wParam);
    return 0;

  case WM_MDIREFRESHMENU:
    // only sent to MDI client window
    // Normally, would call DrawMenuBar, but I have the menu, so eat it.
    {
      if (::GetMenu(m_hWndOwner) != NULL)
      {
        ::SetMenu(m_hWndOwner,NULL);
      }
    }
    OnMDISetMenu((HMENU)ChildDefault());
    return 0;
  }
  return ChildDefault();
}

LRESULT CNewMenuBar::ChildDefault()
{
  // MFC stores current MSG in thread state
  MSG& curMsg = AfxGetThreadState()->m_lastSentMsg;
  ASSERT(m_pOldChildProc);
  return ::CallWindowProc(m_pOldChildProc, curMsg.hwnd,curMsg.message, curMsg.wParam, curMsg.lParam);
}


// CNewMenuBar message handlers
void CNewMenuBar::OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL /*bDisableIfNoHndler*/)
{
  CWnd *pOwnerWnd = CWnd::FromHandlePermanent(m_hWndOwner);
  CMDIFrameWnd* pParent = DYNAMIC_DOWNCAST(CMDIFrameWnd,pOwnerWnd);
  if(pParent)
  {
    HMENU hMenu = ::GetMenu(m_hWndOwner);
    if(m_hWndOwner && ::IsMenu(hMenu))
    { 
      //::SetMenu(m_hWndOwner,NULL);
      OnMDISetMenu(hMenu);
      // here we need to post the message otherwice MFC can't handle default menu right
      ::PostMessage(m_hWndOwner,WM_REMOVEMENU,0,0);
    }
    BOOL bMDIMaximized = FALSE;
    if(pParent->MDIGetActive(&bMDIMaximized) && bMDIMaximized!=m_bMDIMaximized) 
    {
      OnMDISetMenu(NULL);
      Invalidate();
    }
  }
  else if(pOwnerWnd)
  {
    CFrameWnd* pSDIParent = DYNAMIC_DOWNCAST(CFrameWnd,pOwnerWnd);
    if(pSDIParent)
    {
      HMENU hMenu = ::GetMenu(pSDIParent->GetSafeHwnd());
      if(::IsMenu(hMenu))
      {
        ::SetMenu(pSDIParent->GetSafeHwnd(),NULL);
        OnMDISetMenu(hMenu);
      }
    }
  }
}

int CNewMenuBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CControlBar::OnCreate(lpCreateStruct) == -1)
    return -1;

  if(g_Shell>=Win2000)
  {
    ::SendMessage(m_hWnd,WM_CHANGEUISTATE,MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS),0);
  }

  ASSERT(!m_pOldFrameProc);
  CWnd *pParent = GetParent();
  HWND hParent = pParent->GetSafeHwnd();
  ASSERT(hParent);
  m_NewMenuBar.SetAt (hParent,this);
  m_pOldFrameProc = (WNDPROC)SetWindowLongPtr(hParent, GWLP_WNDPROC,(INT_PTR)FrmWindowProc);

  CMDIFrameWnd* pParentFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd,pParent);
  if(pParentFrame && pParentFrame->m_hWndMDIClient)
  {
    m_NewMenuBar.SetAt (pParentFrame->m_hWndMDIClient,this);
    m_pOldChildProc = (WNDPROC)SetWindowLongPtr(pParentFrame->m_hWndMDIClient, GWLP_WNDPROC,(INT_PTR)CilWindowProc);
  }

  return 0;
}

void CNewMenuBar::OnDestroy()
{
  CControlBar::OnDestroy();
  DeleteNewBarItemList();
}

void CNewMenuBar::DeleteNewBarItemList()
{
  for(int i=0;i<=(int)m_NewBarItemList.GetUpperBound();++i)
  {
    if(!m_NewBarItemList[i]->m_nSyncFlag)
    {
      delete m_NewBarItemList[i];
    }
  }
}

int CNewMenuBar::HitTest(LPPOINT ppt )const
{
  int nIndex = GetItemCount();
  while(nIndex--)
  {
    if(m_NewBarItemList[nIndex]->HitTest(*ppt,m_dwStyle & CBRS_ORIENT_VERT))
    {
      return nIndex;
    }
  }
  return -1;
}

//BOOL MDIAddMenuBarButton(HMENU hMenuBar, HWND hWndChild)
//{
//  if(!IsMenu(hMenuBar) || !IsWindow(hWndChild))
//  {
//    return FALSE;
//  }
//
//  HMENU hSysMenu = ::GetSystemMenu(hWndChild,FALSE);
//  IsMenu(hSysMenu);
//  MENUITEMINFO MenuItemInfo = {0};
//  MenuItemInfo.cbSize = sizeof(MenuItemInfo);
//  MenuItemInfo.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_BITMAP;
//  if(GetMenuItemInfo(hMenuBar,0,TRUE,&MenuItemInfo))
//  {
//    if(MenuItemInfo.hbmpItem == HBMMENU_SYSTEM)
//    {
//      if(MenuItemInfo.hSubMenu == hSysMenu &&
//        MenuItemInfo.dwItemData == (ULONG_PTR)hWndChild)
//      {
//        // no changes
//        return TRUE;
//      }
//      RemoveMenu(hMenuBar,0,MF_BYPOSITION);
//
//      int nItemCount = GetMenuItemCount(hMenuBar);
//      // remove SC_CLOSE
//      DeleteMenu(hMenuBar,nItemCount-1,MF_BYPOSITION);
//      // remove SC_RESTORE
//      DeleteMenu(hMenuBar,nItemCount-2,MF_BYPOSITION);
//      // remove SC_MINIMIZE
//      DeleteMenu(hMenuBar,nItemCount-3,MF_BYPOSITION);
//    }
//  }
//
//  MenuItemInfo.hSubMenu = hSysMenu;
//  MenuItemInfo.dwItemData = (ULONG_PTR)hWndChild;
//  MenuItemInfo.hbmpItem = HBMMENU_SYSTEM;
//
//  if(!InsertMenuItem(hMenuBar,0,TRUE,&MenuItemInfo))
//  {
//    return FALSE;
//  }
//
//  // Add minimize button and check for disabled
//  MenuItemInfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_BITMAP;
//  MenuItemInfo.fType = MFT_RIGHTJUSTIFY;
//  if(GetWindowLong(hWndChild,GWL_STYLE)&WS_MINIMIZEBOX)
//  {
//    MenuItemInfo.hbmpItem = HBMMENU_MBAR_MINIMIZE;
//  }
//  else
//  {
//    MenuItemInfo.hbmpItem = HBMMENU_MBAR_MINIMIZE_D;
//  }
//  MenuItemInfo.wID = SC_MINIMIZE;
//  if (!InsertMenuItem(hMenuBar, UINT(-1), TRUE, &MenuItemInfo))
//  {
//    RemoveMenu(hMenuBar, 0, MF_BYPOSITION);
//    return FALSE;
//  }
//
//  // Add restore button
//  MenuItemInfo.fType &= ~MFT_RIGHTJUSTIFY;
//  MenuItemInfo.hbmpItem = HBMMENU_MBAR_RESTORE;
//  MenuItemInfo.wID = SC_RESTORE;
//  if (!InsertMenuItem(hMenuBar, UINT(-1), TRUE, &MenuItemInfo))
//  {
//    // remove SC_MINIMIZE
//    DeleteMenu(hMenuBar, GetMenuItemCount(hMenuBar)-1, MF_BYPOSITION);
//    // remove systemmenu
//    RemoveMenu(hMenuBar, 0, MF_BYPOSITION);
//    return FALSE;
//  }
//
//  // Add close button and check for disabled
//  UINT nMenuFlag = GetMenuState(hMenuBar,SC_CLOSE,MF_BYCOMMAND);
//  if(nMenuFlag==-1 || nMenuFlag&MF_DISABLED)
//  {
//    MenuItemInfo.hbmpItem = HBMMENU_MBAR_CLOSE_D;
//  }
//  else
//  {
//    MenuItemInfo.hbmpItem = HBMMENU_MBAR_CLOSE;
//  }
//  MenuItemInfo.wID = SC_CLOSE;
//  if (!InsertMenuItem(hMenuBar, UINT(-1), TRUE, &MenuItemInfo))
//  {
//    // remove SC_RESTORE
//    DeleteMenu(hMenuBar, GetMenuItemCount(hMenuBar)-1, MF_BYPOSITION);
//    // remove SC_MINIMIZE
//    DeleteMenu(hMenuBar, GetMenuItemCount(hMenuBar)-1, MF_BYPOSITION);
//    // remove systemmenu
//    RemoveMenu(hMenuBar, 0, MF_BYPOSITION);
//    return FALSE;
//  }
//  return TRUE;
//}
//
//BOOL MDIRemoveMenuBarButton(HMENU hMenuBar, HWND hWndChild)
//{
//  if(!IsMenu(hMenuBar) || !IsWindow(hWndChild))
//  {
//    return FALSE;
//  }
//
//  int nItemCount = GetMenuItemCount(hMenuBar)-1;
//  if(GetMenuItemID(hMenuBar,nItemCount)!=SC_CLOSE)
//  {
//    return FALSE;
//  }
//
//  MENUITEMINFO MenuItemInfo = {0};
//  MenuItemInfo.cbSize = sizeof(MenuItemInfo);
//  MenuItemInfo.fMask = MIIM_DATA|MIIM_TYPE|MIIM_ID|MIIM_SUBMENU;
//  if(GetMenuItemInfo(hMenuBar,0,TRUE,&MenuItemInfo))
//  {
//    if(!IsMenu(MenuItemInfo.hSubMenu) || hWndChild!=(HWND)MenuItemInfo.dwItemData)
//    {
//      return FALSE;
//    }
//    //MenuItemInfo.hSubMenu = NULL;
//    //MenuItemInfo.fMask |= MIIM_SUBMENU;
//    //SetMenuItemInfo(hMenuBar,0,TRUE,&MenuItemInfo);
//    //LONG oldStyle = GetWindowLong((HWND)MenuItemInfo.dwItemData,GWL_STYLE);
//    //SetWindowLong((HWND)MenuItemInfo.dwItemData,GWL_STYLE,oldStyle|WS_SYSMENU);
//  }
//
//  //SetWindowLong(hWndChild,GWL_STYLE,GetWindowLong(hWndChild,GWL_STYLE)|WS_SYSMENU);
//  HMENU hSysMenu = GetSubMenu(hMenuBar,0);
//  IsMenu(hSysMenu);
//  // remove systemmenu
//  RemoveMenu(hMenuBar,0,MF_BYPOSITION);
//  // remove SC_CLOSE
//  DeleteMenu(hMenuBar,nItemCount-1,MF_BYPOSITION);
//  // remove SC_RESTORE
//  DeleteMenu(hMenuBar,nItemCount-2,MF_BYPOSITION);
//  // remove SC_MINIMIZE
//  DeleteMenu(hMenuBar,nItemCount-3,MF_BYPOSITION);
//
//  HMENU hSysMenu2 = ::GetSystemMenu(hWndChild,FALSE);
//  IsMenu(hSysMenu2);
//  return TRUE;
//}


void CNewMenuBar::OnMDISetMenu(HMENU hNewMenu)
{
  m_bMDIMaximized = FALSE;
  HWND hwndChild = NULL;

  CWnd *pFrame = CWnd::FromHandle(m_hWndOwner);
  if(pFrame)
  {
    CMDIFrameWnd *pMDIFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd,pFrame->GetTopLevelFrame());
    if(pMDIFrame)
    {
      hwndChild = (HWND)::SendMessage(pMDIFrame->m_hWndMDIClient,WM_MDIGETACTIVE, 0,(LPARAM)&m_bMDIMaximized);
    }
  }

  if(hNewMenu)
  {
    //if( m_hMenuBar!=hNewMenu)
    //{
    //  MDIRemoveMenuBarButton(m_hMenuBar,hwndChild);
    //}
    m_hMenuBar = hNewMenu;
  }

  DeleteNewBarItemList();
  m_NewBarItemList.RemoveAll();

  UINT nItems = GetMenuItemCount(m_hMenuBar);

  //if(m_bMDIMaximized)
  //{
  //  MDIAddMenuBarButton(m_hMenuBar,hwndChild);
  //}
  //else
  //{
  //  MDIRemoveMenuBarButton(m_hMenuBar,hwndChild);
  //}
  //nItems = GetMenuItemCount(m_hMenuBar);
  for (int iItem = 0; iItem < (int)nItems; iItem++)
  {
    MENUITEMINFO MenuItemInfo = {0};
    MenuItemInfo.cbSize = sizeof(MenuItemInfo);
    MenuItemInfo.fMask = MIIM_DATA|MIIM_TYPE|MIIM_ID;
    if(GetMenuItemInfo(m_hMenuBar,iItem,TRUE,&MenuItemInfo))
    {
      if(MenuItemInfo.fType&MF_BITMAP)
      {
        m_NewBarItemList.Add(new CNewIconBarItem((HBITMAP)MenuItemInfo.dwTypeData,MenuItemInfo.wID));
      }
      else
      {
      CNewMenuBarItem* pItem = new CNewMenuBarItem(m_hMenuBar,iItem);
      m_NewBarItemList.Add(pItem);
        if(MenuItemInfo.fType&MFT_RIGHTJUSTIFY)
        {
          pItem->m_nState |= ODS_RIGHTJUSTIFY;
        }
      if(MenuItemInfo.fType&MF_OWNERDRAW)
      {
        MEASUREITEMSTRUCT measureItem = {0};
        measureItem.CtlType = ODT_MENU;
        measureItem.itemID = MenuItemInfo.wID;
        measureItem.itemData = MenuItemInfo.dwItemData;
        ::SendMessage(m_hWndOwner,WM_MEASUREITEM,0,(LPARAM)&measureItem);
        pItem->m_ItemRect = CRect(0,0,measureItem.itemWidth+GetSystemMetrics(SM_CXMENUCHECK),GetSystemMetrics(SM_CYMENU));
      }
      else
      {
        pItem->m_ItemRect = CRect(0,0,GetSystemMetrics(SM_CXMENUSIZE)+2,GetSystemMetrics(SM_CYMENUSIZE)+2);
      }
    }
  }
  }
  if(m_bMDIMaximized)
  {
    m_NewBarItemList.InsertAt(0,new CNewIconBarItem(HBMMENU_SYSTEM,UINT(-1)));

    m_NewBarItemList.Add(new CNewIconBarItem(HBMMENU_MBAR_MINIMIZE,SC_MINIMIZE));
    m_NewBarItemList.Add(new CNewIconBarItem(HBMMENU_MBAR_RESTORE,SC_RESTORE));
    m_NewBarItemList.Add(new CNewIconBarItem(HBMMENU_MBAR_CLOSE,SC_CLOSE));

    CNewBarItem* pItem = m_NewBarItemList[m_NewBarItemList.GetSize()-3];
    pItem->m_nState |= ODS_RIGHTJUSTIFY;
  }
  RefreshBar();
}

void CNewMenuBar::RefreshBar()
{
  Invalidate();

  CFrameWnd* pFrame = GetTopLevelFrame();
  pFrame->RecalcLayout();

  // floating frame
  CMiniFrameWnd* pMiniFrame = DYNAMIC_DOWNCAST(CMiniFrameWnd,GetParentFrame());
  if (pMiniFrame)
  {
    pMiniFrame->RecalcLayout();
  }
}


void CNewMenuBar::OnPaint()
{
  CPaintDC dc(this); // device context for painting
  // TODO: Add your message handler code here

  OnEraseBkgnd(&dc);

  CRect client;
  GetClientRect(client);


  //::SendMessage(m_hWnd,WM_CHANGEUISTATE,MAKEWPARAM(UIS_INITIALIZE,0),0);
  DWORD dwUIState = 0;
  if(g_Shell>=Win2000)
  {
    BOOL bMenuUnderlines = TRUE;
    if(SystemParametersInfo( SPI_GETKEYBOARDCUES,0,&bMenuUnderlines,0)==TRUE && bMenuUnderlines==FALSE)
    {
      CFrameWnd* pParent = GetTopLevelFrame();
      dwUIState = (DWORD)::SendMessage(pParent->GetSafeHwnd(),WM_QUERYUISTATE,0,0);
    }
  }

  CFont fontMenu;
  LOGFONT logFontMenu;
  NONCLIENTMETRICS nm = {0};
  nm.cbSize = sizeof (NONCLIENTMETRICS);
  VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
  logFontMenu =  nm.lfMenuFont;

  fontMenu.CreateFontIndirect (&logFontMenu);
  CFont* pOldFont = dc.SelectObject(&fontMenu);

  int nCount = (int)m_NewBarItemList.GetUpperBound();
  for(int n=0;n<=nCount;n++)
  {
    CNewBarItem *pBarItem = m_NewBarItemList[n];
    if(dwUIState&UISF_HIDEACCEL)
    {
      pBarItem->m_nState |= ODS_NOACCEL;
    }
    else
    {
      pBarItem->m_nState &= ~ODS_NOACCEL;
    }
    if(!IsFloating() && (m_dwStyle & CBRS_ORIENT_VERT))
    {
      pBarItem->m_nState |= ODS_DRAW_VERTICAL;
    }
    else
    {
      pBarItem->m_nState &= ~ODS_DRAW_VERTICAL;
    }

    pBarItem->DrawBarItem(m_hWndOwner,dc);
  }
  dc.SelectObject(pOldFont);
}

void CNewMenuBar::DrawGripper(CDC* pDC, const CRect& rect)
{
  CNewMenu::GetActualMenuTheme()->DrawGripper(pDC,rect,m_dwStyle, m_cxLeftBorder, m_cxRightBorder, m_cyTopBorder, m_cyBottomBorder);
}

#if _MFC_VER < 0x0800  
  UINT CNewMenuBar::OnNcHitTest(CPoint)
  {
    return HTCLIENT;
  }
#else
  LRESULT CNewMenuBar::OnNcHitTest(CPoint)
  {
    return HTCLIENT;
  }
#endif

void CNewMenuBar::OnNcPaint()
{
  EraseNonClient();
}

void CNewMenuBar::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp)
{
  UNREFERENCED_PARAMETER(bCalcValidRects);
  // calculate border space (will add to top/bottom, subtract from right/bottom)
  CRect rect(0,0,0,0);
  BOOL bHorz = (m_dwStyle & CBRS_ORIENT_HORZ) != 0;
  CalcInsideRect(rect, bHorz);

  if ((m_dwStyle & CBRS_FLOATING))
  {
    lpncsp->rgrc[0].left += rect.left;
    lpncsp->rgrc[0].top  += rect.top;
  }
  else
  {
    // adjust non-client area for border space
    lpncsp->rgrc[0].left += rect.left;
    lpncsp->rgrc[0].top  += rect.top;
    lpncsp->rgrc[0].right += rect.right;
    lpncsp->rgrc[0].bottom += rect.bottom;

    ASSERT(lpncsp->rgrc[0].right<0x7fff);
    ASSERT(lpncsp->rgrc[0].bottom<0x7fff);
  }
}

void CNewMenuBar::EraseNonClient()
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

  //SetLayout(dc,dwOldLayout);
}

void CNewMenuBar::DrawBorders(CDC*, CRect&)
{
  return;
}

BOOL CNewMenuBar::OnEraseBkgnd(CDC* pDC)
{
  CBrush*pBrush = GetMenuBarBrush();
  if(pBrush)
  {
    CPoint brushOrg(0,0);
    CFrameWnd* pFrame = GetParentFrame();
    if(pFrame)
    {
      CRect windowRect;
      pFrame->GetWindowRect(windowRect);
      ScreenToClient(windowRect);
      brushOrg = windowRect.TopLeft();
    }
    // need for win95/98/me
    VERIFY(pBrush->UnrealizeObject());
    CPoint oldOrg = pDC->SetBrushOrg(brushOrg);

    CRect clientRect;
    GetWindowRect(clientRect);

    CRect clientRect2 = clientRect;
    ScreenToClient(clientRect);
    pDC->FillRect(clientRect,pBrush);
    pDC->SetBrushOrg(oldOrg);
  }
  else
  {
    CRect clientRect;
    GetWindowRect(clientRect);
    ScreenToClient(clientRect);
    pDC->FillSolidRect(clientRect,CNewMenu::GetMenuBarColor());//GetSysColor(COLOR_3DLIGHT));
  }
  return true;
}
int CNewMenuBar::GetNextItem(int nIndex, BOOL bNext)
{
  int nCount = GetItemCount();
  int nTest = nCount;
  while(nIndex<0)
  {
    nIndex +=nCount;
  }
  while(nTest--)
  {
    nIndex += bNext?1:nCount-1;
    nIndex %= nCount;
    UINT uID = m_NewBarItemList[nIndex]->GetMenuItemID();
    if(SC_SIZE<=uID && uID<=SC_HOTKEY)
    {
      // Ignore menu button
      continue;
    }
    if( !(m_NewBarItemList[nIndex]->m_nState&ODS_HIDDEN))
    {
      return nIndex;
    }
  }
  return -1;
}

CNewMenuBar* CNewMenuBar::g_pMenuBar = NULL;
HHOOK CNewMenuBar::g_hMsgHook = NULL;

LRESULT CALLBACK CNewMenuBar::MenuInputFilter(int code, WPARAM wParam, LPARAM lParam)
{
  return (code==MSGF_MENU && g_pMenuBar && g_pMenuBar->OnMenuInput( *((MSG*)lParam) )) ? TRUE : CallNextHookEx(g_hMsgHook, code, wParam, lParam);
}

BOOL CNewMenuBar::OnMenuInput(MSG msg)
{
  ASSERT_VALID(this);
  CPoint pt = msg.lParam;
  int nIndex = -1;
  if(m_hWnd==::WindowFromPoint(pt))
  {
    ScreenToClient(&pt);
    nIndex = HitTest(&pt);
  }

  switch (msg.message)
  {
  case WM_MOUSEMOVE:
    if (pt != m_ptMouse)
    {
      if (nIndex>=0 && (nIndex!=m_nCurIndex) )
      { // destroy popupped menu
        ::PostMessage(m_hWndCommand,WM_CANCELMODE,0,0);
        m_nCurIndex = nIndex;
        m_bLoop = TRUE;             // continue loop
      }
      m_ptMouse = pt;
    }
    break;

  case WM_LBUTTONDOWN:
    if(nIndex>=0)
    {
      if(nIndex!=m_nCurIndex)
      {
        // close popup menu
        ::PostMessage(m_hWndCommand,WM_CANCELMODE,0,0);
        m_nCurIndex = nIndex;
        m_bLoop = TRUE;
      }
      m_nMouseIndex = m_nCurIndex;
      return TRUE;
    }
    break;

  case WM_LBUTTONUP:
    if(nIndex>=0)
    {
      if (m_nMouseIndex!=-1 && nIndex==m_nMouseIndex)
      {
        // close popup menu
        ::PostMessage(m_hWndCommand,WM_CANCELMODE,0,0);
        m_bLoop = FALSE;
        m_eTrackingState = TR_MENU;
        return TRUE;
      }
    }
    break;

  case WM_KEYDOWN:
    {
      TCHAR vKey = (TCHAR)msg.wParam;
      //if (m_dwStyle & CBRS_ORIENT_VERT)
      //{ // if vertical
      //  break; // do nothing
      //}

      if ((vKey == VK_LEFT  && m_bProcessLeftArrow) ||
        (vKey == VK_RIGHT && m_bProcessRightArrow))
      {
        // no submenu
        int nNewIndex = GetNextItem(m_nCurIndex, vKey==VK_RIGHT);
        ::PostMessage(m_hWndCommand,WM_CANCELMODE,0,0);
        //UpdateBar();
        m_nCurIndex = nNewIndex;
        m_bLoop = TRUE;             // continue loop
        return TRUE;              // eat it!
      }
    }
    break;

  case WM_SYSKEYDOWN:
    m_bIgnoreAltKey = TRUE;         // next SysKeyUp will be ignored
    break;
  }
  return FALSE; // pass along...
}


void CNewMenuBar::SetSel(int nIndex, ETrackingState eState)
{
  if(m_eTrackingState!=eState)
  {
    if(g_Shell>=Win2000)
    {
      CFrameWnd* pParent = GetTopLevelFrame();
      DWORD dwUIState = (DWORD)::SendMessage(pParent->GetSafeHwnd(),WM_QUERYUISTATE,0,0);
      if(eState==TR_NONE)
      {
        if(dwUIState!=(UISF_HIDEACCEL | UISF_HIDEFOCUS))
        {
          ::SendMessage(pParent->GetSafeHwnd(),WM_CHANGEUISTATE,MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS),0);
          Invalidate(false);
        }
      }
      else if(m_eTrackingState==TR_NONE)
      {
        ::SendMessage(pParent->GetSafeHwnd(),WM_CHANGEUISTATE,MAKEWPARAM(UIS_INITIALIZE,0),0);
        if((DWORD)::SendMessage(pParent->GetSafeHwnd(),WM_QUERYUISTATE,0,0)!=dwUIState)
        {
          Invalidate(false);
        }
      }
    }
    m_eTrackingState = eState;
  }

  UINT nState;
  switch(eState)
  {
  case TR_MENU:
    nState = ODS_HOTLIGHT;
    break;

  case TR_POPUP:
    nState = ODS_SELECTED|ODS_SELECTED_OPEN;
    break;

  default:
    nState = 0;
    break;
  }
  BOOL bVert = m_dwStyle & CBRS_ORIENT_VERT;
  if(bVert)
  {
    nState |= ODS_DRAW_VERTICAL;
  }
  UINT nRemove = (UINT) ~(ODS_SELECTED|ODS_SELECTED_OPEN|ODS_HOTLIGHT|ODS_DRAW_VERTICAL);

  if(nIndex!=m_nActualSelIndex)
  {
    if(m_nActualSelIndex!=-1 && m_nActualSelIndex<m_NewBarItemList.GetSize())
    {
      CNewBarItem *pItem = m_NewBarItemList[m_nActualSelIndex];

      pItem->m_nState &= nRemove;
      if(bVert)
      {
        pItem->m_nState |= ODS_DRAW_VERTICAL;
      }
      InvalidateRect(pItem->GetRect(bVert),FALSE);
    }
    m_nActualSelIndex = nIndex;
    if(m_nActualSelIndex!=-1 && m_nActualSelIndex<m_NewBarItemList.GetSize())
    {
      CNewBarItem *pItem = m_NewBarItemList[m_nActualSelIndex];
      UINT nNewState = nState | (pItem->m_nState&nRemove);
      if(pItem->m_nState!=nNewState)
      {
        pItem->m_nState = nNewState;
        InvalidateRect(pItem->GetRect(bVert),FALSE);
      }
    }
  }
  else if(m_nActualSelIndex!=-1 && m_nActualSelIndex<m_NewBarItemList.GetSize())
  {
    CNewBarItem *pItem = m_NewBarItemList[m_nActualSelIndex];
    UINT nNewState = nState | (pItem->m_nState&nRemove);

    if((pItem->m_nState&~ODS_NOACCEL)!=(nNewState&~ODS_NOACCEL))
    {
      pItem->m_nState = nNewState;
      InvalidateRect(pItem->GetRect(bVert),FALSE);
    }
  }

  m_nCurIndex = nIndex;
}

UINT CNewMenuBar::EnableTimer(BOOL bEnable)
{
  if(bEnable)
  {
    if(!m_TimerID)
    {
      m_TimerID = GetSafeTimerID(m_hWnd,100);
    }
  }
  else if(m_TimerID)
  {
    KillTimer(m_TimerID);
    m_TimerID = NULL;
  }
  return m_TimerID;
}

BOOL CNewMenuBar::TranslateFrameMessage(MSG* pMsg)
{
  ASSERT_VALID(this);
  ASSERT(pMsg);

  UINT nMsg = pMsg->message;
  if (nMsg == WM_SYSKEYDOWN || nMsg == WM_SYSKEYUP || nMsg == WM_KEYDOWN)
  { // Alt key presed?
    BOOL bAlt = HIWORD(pMsg->lParam) & KF_ALTDOWN;
    // + X key
    TCHAR vkey = (TCHAR)pMsg->wParam;
    if( vkey == VK_MENU ||
      (vkey == VK_F10 && !((GetKeyState(VK_SHIFT) & 0x80000000) ||
      (GetKeyState(VK_CONTROL) & 0x80000000) || bAlt)))
    {
      // only alt key pressed
      if (nMsg == WM_SYSKEYUP)
      {
        switch (m_eTrackingState)
        {
        case TR_POPUP:
        case TR_NONE:
          if (m_bIgnoreAltKey == TRUE)
          {
            m_bIgnoreAltKey = FALSE;
            break;
          }
          SetSel(0,TR_MENU);
          break;

        case TR_MENU:
          SetSel(-1,TR_NONE);
          break;
        }
      }
      return TRUE;
    }
    else if ((nMsg == WM_SYSKEYDOWN || nMsg == WM_KEYDOWN))
    {
      if (m_eTrackingState == TR_MENU || m_eTrackingState == TR_POPUP)
      {
        if (m_dwStyle & CBRS_ORIENT_HORZ)
        { // if horizontal
          switch (vkey)
          {
          case VK_LEFT:
          case VK_RIGHT:
            m_nCurIndex=GetNextItem(m_nCurIndex, vkey == VK_RIGHT);
            if(m_eTrackingState == TR_POPUP)
            {
              ShowMenu(m_nCurIndex);
            }
            else
            {
              SetSel(m_nCurIndex,m_eTrackingState);
            }
            return TRUE;

          case VK_RETURN:
            {
              if(m_nCurIndex>=0 && m_nCurIndex<m_NewBarItemList.GetSize())
              {
                CNewBarItem *pItem = m_NewBarItemList[m_nCurIndex];
                if(!::IsMenu(pItem->GetMenu(m_hWndOwner)))
                {
                  SetSel(m_nCurIndex,TR_NONE);
                  ::SendMessage(m_hWndOwner,WM_COMMAND, (WPARAM)pItem->GetMenuItemID(), (LPARAM)m_hWnd);
                  return TRUE;
                }
              }
            }
            break;

          case VK_SPACE:
            {
              if(m_nCurIndex>=0 && m_nCurIndex<m_NewBarItemList.GetSize())
              {
                CNewBarItem *pItem = m_NewBarItemList[m_nCurIndex];
                if(!::IsMenu(pItem->GetMenu(m_hWndOwner)))
                {
                  SetSel(m_nCurIndex,TR_NONE);
                  ::SendMessage(m_hWndOwner,WM_COMMAND, (WPARAM)pItem->GetMenuItemID(), (LPARAM)m_hWnd);
                  return TRUE;
                }
              }
            }

          case VK_UP:
          case VK_DOWN:
            ShowMenu(m_nCurIndex);
            return TRUE;

          case VK_ESCAPE:
            bAlt = false;
            SetSel(-1,TR_NONE);
            return TRUE;
          }
        }
        else
        { // if vertical
          switch (vkey)
          {
          case VK_UP:
          case VK_DOWN:
            m_nCurIndex=GetNextItem(m_nCurIndex, vkey==VK_DOWN);
            if(m_eTrackingState == TR_POPUP)
            {
              ShowMenu(m_nCurIndex);
            }
            else
            {
              SetSel(m_nCurIndex,m_eTrackingState);
            }
            return TRUE;

          case VK_RETURN:
            {
              if(m_nCurIndex>=0 && m_nCurIndex<m_NewBarItemList.GetSize())
              {
                CNewBarItem *pItem = m_NewBarItemList[m_nCurIndex];
                if(!::IsMenu(pItem->GetMenu(m_hWndOwner)))
                {
                  SetSel(m_nCurIndex,TR_NONE);
                  ::SendMessage(m_hWndOwner,WM_COMMAND, (WPARAM)pItem->GetMenuItemID(), (LPARAM)m_hWnd);
                  return TRUE;
                }
              }
            }
            break;

          case VK_SPACE:
            {
              if(m_nCurIndex>=0 && m_nCurIndex<m_NewBarItemList.GetSize())
              {
                CNewBarItem *pItem = m_NewBarItemList[m_nCurIndex];
                if(!::IsMenu(pItem->GetMenu(m_hWndOwner)))
                {
                  SetSel(m_nCurIndex,TR_NONE);
                  ::SendMessage(m_hWndOwner,WM_COMMAND, (WPARAM)pItem->GetMenuItemID(), (LPARAM)m_hWnd);
                  return TRUE;
                }
              }
            }

          case VK_RIGHT:
          case VK_LEFT:
            ShowMenu(m_nCurIndex);
            return TRUE;

          case VK_ESCAPE:
            bAlt=false;
            SetSel(-1,TR_NONE);
            return TRUE;
          }
        }
      }

      // Alt + X pressed
      if ((bAlt || m_eTrackingState == TR_MENU) && _istalnum(vkey))
      {
        LRESULT lr = CNewMenu::FindKeyboardShortcut(vkey,0, CMenu::FromHandle(m_hMenuBar));
        if (lr)
        {
          if(m_bMDIMaximized)
          {
            // Add offset to systemmenu
            ShowMenu(m_nCurIndex=(LOWORD(lr)+1));
          }
          else
          {
            ShowMenu(m_nCurIndex=LOWORD(lr));
          }
          return TRUE;    // eat it!
        }
        else if (m_eTrackingState==TR_MENU && !bAlt)
        {
          //MessageBeep(0);   // if you want
          return TRUE;
        }
      }
    }
  }
  return FALSE; // pass along...
}

void CNewMenuBar::OnTimer(UINT_PTR nIDEvent)
{
  if(nIDEvent!=m_TimerID)
  {
    CControlBar::OnTimer(nIDEvent);
    return;
  }

  CFrameWnd* pFrame = GetDockingFrame();
  while(pFrame && pFrame->GetParentFrame())
  { 
    pFrame = pFrame->GetParentFrame();
  }
  BOOL bFocus = pFrame?pFrame->IsChild(GetFocus()):false;
  if(!bFocus)
  {
    SetSel(-1,TR_NONE);
    EnableTimer(FALSE);
  }
  else if (m_eTrackingState == TR_MENU)
  {
    CPoint point;
    GetCursorPos(&point);
    if(m_hWnd!=::WindowFromPoint(point))
    {
      SetSel(-1,TR_NONE);
      EnableTimer(FALSE);
    }
  }
  else if (m_eTrackingState == TR_NONE)
  {
    EnableTimer(FALSE);
  }
}


void CNewMenuBar::OnMouseMove(UINT nFlags, CPoint point)
{
  if(m_ptMouse!=point)
  {
    CFrameWnd* pFrame = GetDockingFrame();
    while(pFrame && pFrame->GetParentFrame())
    { 
      pFrame = pFrame->GetParentFrame();
    }
    BOOL bFocus = pFrame?pFrame->IsChild(GetFocus()):false;
    if(!bFocus )
    { 
      SetSel(-1,TR_NONE);
      EnableTimer(FALSE);
    }
    else
    {
      m_bIgnoreAltKey = FALSE;
      m_ptMouse = point;
      int nIndex = HitTest(&point);
      if(nIndex!=-1)
      {
        if(m_eTrackingState==TR_POPUP)
        {
          OnLButtonDown(nFlags,point);
          return;
        }
        else
        {
          SetSel(nIndex,TR_MENU);
          EnableTimer();
        }
        return;
      }
      else if(m_eTrackingState == TR_MENU)
      {
        SetSel(-1,TR_NONE);
        EnableTimer(FALSE);
      }
    }
  }
  CControlBar::OnMouseMove(nFlags, point);
}

void CNewMenuBar::OnLButtonDown(UINT nFlags, CPoint point)
{
  m_nMouseIndex = -1;
  m_nCurIndex = HitTest(&point);
  if(m_nCurIndex!=-1)
  {
    ShowMenu(m_nCurIndex);
    return;
  }
  else
  {
    SetSel(-1,TR_NONE);
  }
  CControlBar::OnLButtonDown(nFlags, point);
}

BOOL CNewMenuBar::ShowMenu(int nIndex)
{
  if(m_bLoop)
  {
    return FALSE;
  }
  if(nIndex>=0 && nIndex<m_NewBarItemList.GetSize())
  {
    CNewBarItem *pItem = m_NewBarItemList[m_nCurIndex=nIndex];
    do
    {
      CMenu *pMenu = CMenu::FromHandle(pItem->GetMenu(m_hWndOwner));
      if(pMenu)
      {
        SetSel(m_nCurIndex,TR_POPUP);
        UpdateWindow();

        m_hWndCommand = m_hWndOwner;
        m_hMenuPopup = pMenu->GetSafeHmenu();

        CRect rcTBItem = pItem->GetRect(m_dwStyle & CBRS_ORIENT_VERT);
        ClientToScreen(rcTBItem);
        // Need to set the rectangle of the button!
        CNewMenu* pNewMenu = DYNAMIC_DOWNCAST(CNewMenu,pMenu);
        if(pNewMenu)
        {
          CNewMenu* pParentMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pNewMenu->GetParent()));
          if(pParentMenu)
          {
            CRect rect = pParentMenu->GetLastActiveMenuRect();
            CRect winRect;
            GetWindowRect(winRect);
            ScreenToClient(winRect);
            rect.OffsetRect(-winRect.TopLeft());
            pParentMenu->SetLastMenuRect(rect);
          }
        }

        // handle pending WM_PAINT messages
        MSG msg;
        while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
        {
          if (!GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
            return false;
          DispatchMessage(&msg);
        }

        // install hook
        ASSERT(g_pMenuBar == NULL);
        ASSERT(g_hMsgHook == NULL);


        g_pMenuBar = this;
        m_bLoop = FALSE;
        g_hMsgHook = ::SetWindowsHookEx(WH_MSGFILTER, MenuInputFilter, NULL, AfxGetApp()->m_nThreadID);

        TrackPopupMenuSpecial(pMenu,0,rcTBItem,CWnd::FromHandle(m_hWndCommand),m_dwStyle & CBRS_ORIENT_VERT);

        ::UnhookWindowsHookEx(g_hMsgHook);
        g_hMsgHook = NULL;
        g_pMenuBar = NULL;

        UpdateWindow();
        if(!m_bLoop || m_nCurIndex<0 || m_eTrackingState!=TR_POPUP)
        {
          m_bLoop = FALSE;
          SetSel(m_nCurIndex,(m_eTrackingState==TR_POPUP)?TR_NONE:m_eTrackingState);
          return TRUE;
        }

        pItem = m_NewBarItemList[m_nCurIndex];
      }
      else
      {
        m_bLoop = FALSE;
        SetSel(m_nCurIndex,m_eTrackingState==TR_POPUP?TR_POPUP:TR_MENU);
        EnableTimer();
        return FALSE;
      }
    }while(m_bLoop);
  }
  return TRUE;
}

void CNewMenuBar::OnLButtonUp(UINT nFlags, CPoint point)
{
  int nIndex = HitTest(&point);
  if(nIndex!=-1)
  {
    CNewBarItem *pItem = m_NewBarItemList[nIndex];
    if(!::IsMenu(pItem->GetMenu(m_hWndOwner)))
    {
      SetSel(m_nCurIndex,TR_NONE);
      ::SendMessage(m_hWndOwner,WM_COMMAND, (WPARAM)pItem->GetMenuItemID(), (LPARAM)m_hWnd);
    }
    return;
  }
  CControlBar::OnLButtonUp(nFlags, point);
}