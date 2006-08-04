//------------------------------------------------------------------------------
// File    : NewMenu.cpp
// Version : 1.25
// Date    : 25. June 2005
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewToolBar
//
// For bugreport please add following informations
// - The CNewMenu version number (Example CNewMenu 1.16)
// - Operating system Win95 / WinXP and language (English / Japanese / German etc.)
// - Installed service packs
// - Version of internet explorer (important for CNewToolBar)
// - Short description how to reproduce the bug
// - Pictures/Sample are welcome too
// - You can write in English or German to the above email-address.
// - Have my compiled examples the same effect?
//------------------------------------------------------------------------------
//
// Special
// Compiled version with visual studio V7.0/7.1 doesn't run correctly on
// WinNt 4.0 nor Windows 95. For those platforms, you have to compile it with
// visual studio V6.0.
// (It may run with 7.0/7.1, but you must be sure to have all required and newest
//  dll from the MFC installed => otherwise menu painting problems)
// Keep in mind you should have installed latest service-packs to the operating-
// system too! (And a new IE because some fixes of common components were made)
//------------------------------------------------------------------------------
//
// ToDo's
// checking GDI-Leaks, are there some more?
// Shade-Drawing, when underground is changing
// Border repainting after menu closing!!!
// Shade painting problems? On Chinese or Japanese systems?
// supporting additional bitmap in menu with ownerdrawn style
//
// 25. June 2005 (Version 1.25)
// added compatibility for new security enhancement in CRT (MFC 8.0)
//
// 16. May 2005 (Version 1.24) 
// added tranparency to menu
// added some more information for visualtudio work better with messagemaps 
// safer handling of limitted HDC's
// added new function CreateBitmapMask
// - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
//   corrected background and highlight colors for XP styles.  Read comments dated May-05-2005 below for more information
//
// 23. January 2005 (Version 1.23) - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// added another LoadToolbar() function to map a 16 color toolbar to a 256 color bitmap
// created CNewMenuThreadHook class to be instantiated and destroyed in a thread to setup and release the menu hook
// Bruno: Now we subclass only NewMenu (Drawing border etc.)
//
// 11. September 2004 (Version 1.22)
// added helperfunction for forcing compiler to export templatefunction
// better support for Hebrew or Arabic (MFT_RIGHTORDER)
// added new support for multiple monitors
// added some more details to errorhandling of CNewMenu ShowLastError only debug
// Changed AddTheme for compatibility to VisualStudio 2005
//
// 27. June 2004 (Version 1.21)
// colorsheme for office 2003 style changed and more like real office 2003
// enabled keyboardcues in style office 2003
// Old colorscheme from office 2003 is now called STYLE_COLORFUL
//
// 1. May 2004 (Version 1.20)
// additionally drawing for menubar
// added some compatibility issue with wine
// shading window (only test)
// changed the colorscheme with more theme support
// corected the menubar detection
//
// 6. February 2004 (Version 1.19)
// Icy and standard menu: systemmenu now with symbols
// underline enabling and disabling added, thanks to Robin Leatherbarrow
// make it compatible to 64-bit windows, thanks to David Pritchard
// added accelerator support to menu, thanks to Iain Clarke
// Corrected functions names SetXpBlendig/GetXpBlendig to SetXpBlending/GetXpBlending
// removed 3d-look from office 2003 style menu (now like real office 2003 menu)
// Handling of a normal menu-item in menubar changed
//
// 30. November 2003 (Version 1.18)
// minor changes for CNewToolBar
//
// 11. September 2003 Bug Fixes or nice hints from 1.16
// Better handling of menubar brushes (never destroy) Style_XP and STYLE_XP_2003
// Drawing of menuicons on WinNt 4.0 fixed
// spelling & initializing & unused brushes fixed, thanks to John Zegers
//
// 12. July 2003 Bug Fixes or nice hints from 1.15
// Added gradient-Dockbar (CNewDockBar) for office 2003 style
// (you need to define USE_NEW_DOCK_BAR)
// fixed menu checkmarks with bitmaps (Selection-with is not correct on all styles)
// Drawing to a memory-DC minimizing flickering
// with of menu-bar-item without & now better
//
// 22. June 2003 Bug Fixes or nice hints from 1.14
// Shortkey-handling for ModifyODMenu and InsertODMenu
// Change os identifer for OS>5.0 to WinXP
// Color scheme for Office menu changed and updating menubarcolor too
// Gradientcolor for Office 2003 bar
// XP-changing settings Border Style => fixed border painting without restarting
//
// 23. April 2003 Bug Fixes or nice hints from 1.13
// Fixed the window menu with extreme width, thanks to Voy
// Fix bug about shortkey for added menu item with AppendMenu, thanks to Mehdy Bohlool(Zy)
// Measureitem for default item corrected, thanks to LordLiverpool
// New menu style office 2003, but color schema is not tested, changing of menubar color
// on win95 and WinNt is not possible
// Menubardrawing on deactivated app corrected for win95 & WinNt4.0
//
// 23. January 2003 Bug Fixes or nice hints from 1.12
// Fixed SetMenuText, thanks to Eugene Plokhov
// Easier for having an other menu font, but on your own risk ;)
// Added new function SetNewMenuBorderAllMenu for borderhandling off non CNewMenu
//
// 22. December 2002 Bug Fixes or nice hints from 1.11
// Added some new tracing stuffs
// fixed SetMenuText
// XP-Style menu more office style and better handling with less 256 color, thanks to John Zegers
// Close window button disabling under XP with theme fixed
// Menubar color with theme fixed, thanks to Luther Weeks
//
// (9. November) Bug Fixes or nice hints from 1.11b
// Some new function for bleaching bitmaps, thanks to John Zegers
// GetSubMenu(LPCTSTR) , thanks to George Menhorn
// Menu animation under Win 98 fixed
// Borderdrawing a little better also under win 98
// Added new menustyle ICY
//
// (21. August 2002) Bug Fixes or nice hints from 1.10
// Bugfix for Windows NT 4.0 System Bitmaps, thanks to Christoph Weber
//
// (30. July 2002) Bug Fixes or nice hints from 1.03
// Color of checked icons / selection marks more XP-look
// Updating images 16 color by changing systems colors
// Icon drawing with more highlighting and half transparency
// Scroller for large menu correcting (but the overlapping border is not corrected)
// Resource-problem on windows 95/98/ME?, when using too much bitmaps in menus
//  changing the kind of storing bitmaps (images list)
// Calculating the size for a menuitem (specially submenuitem) corrected.
// Recentfilelist corrected (Seperator gots a wrong ID number)
//
// (25. June 2002) Bug Fixes or nice hints from 1.02
// Adding removing menu corrected for drawing the shade in menubar
// Draw the menuborder corrected under win 98 at the bottom side
// Highlighting under 98 for menu corrected (Assert), Neville Franks
//  Works also on Windows 95
// Changing styles on XP menucolor of the bar now corrected.
//
// (18. June 2002) Bug Fixes or nice hints from 1.01
// Popup menu which has more items than will fit, better
// LoadMenu changed for better languagessuport, SnowCat
// Bordercolor & Drawing of XP-Style-Menu changed to more XP-Look
// Added some functions for setting/reading MenuItemData and MenuData
// Menubar highlighting now supported under 98/ME (NT 4.0 and 95?)
// Font for menutitle can be changed
// Bugs for popupmenu by adding submenu corrected, no autodestroy
//
// (6. June 2002) Bug Fixes or nice hints from 1.00
// Loading Icons from a resource dll expanded (LoadToolBar) Jonathan de Halleux, Belgium.
// Minimized resource leak of User and Gdi objects
// Problem of disabled windows button on 98/Me solved
// Gradient-drawing without Msimg32.dll now supported especially for win 95
// Using solid colors for drawing menu items
// GetSubMenu changed to const
// Added helper function for setting popup-flag for popup menu (centered text)
// Redraw menu bar corrected after canceling popup menu in old style menu
//
// (23. Mai 2002) Bug Fixes and portions of code from previous version supplied by:
// Brent Corkum, Ben Ashley, Girish Bharadwaj, Jean-Edouard Lachand-Robert,
// Robert Edward Caldecott, Kenny Goers, Leonardo Zide, Stefan Kuhr,
// Reiner Jung, Martin Vladic, Kim Yoo Chul, Oz Solomonovich, Tongzhe Cui,
// Stephane Clog, Warren Stevens, Damir Valiulin
//
// You are free to use/modify this code but leave this header intact.
// This class is public domain so you are free to use it any of your
// applications (Freeware, Shareware, Commercial).
// All I ask is that you let me know so that if you have a real winner I can
// brag to my buddies that some of my code is in your app. I also wouldn't
// mind if you sent me a copy of your application since I like to play with
// new stuff.
//------------------------------------------------------------------------------

#include "stdafx.h"        // Standard windows header file
#include "NewMenu.h"       // CNewMenu class declaration

// Those two following constants should be only defined for tracing.
// Normally they are commented out.
//#define _TRACE_MENU_
//#define _TRACE_MENU_LOGFILE

//#define _NEW_MENU_USER_FONT
// For using an userfont you can set the define. It does not work right in the
// systemmenu nor in the menubar. But when you like to use the new menu only in
// popups then it will work, but it is newer a good idea to set or change the
// menufont.
// I do not support this mode, you use it on your own risk. :)
#ifdef _NEW_MENU_USER_FONT
static  LOGFONT MENU_USER_FONT = {20, 0, 0, 0, FW_NORMAL,
                                  0, 0, 0, DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH,_T("Comic Sans MS")};
#endif

#ifdef _TRACE_MENU_
#include "MyTrace.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

#ifndef IS_INTRESOURCE
#define IS_INTRESOURCE(hh) (HIWORD(hh)==NULL)
#endif

#define GAP 2

#ifndef COLOR_MENUBAR
#define COLOR_MENUBAR       30
#endif

#ifndef SPI_GETDROPSHADOW
#define SPI_GETDROPSHADOW   0x1024
#endif

#ifndef SPI_GETFLATMENU
#define SPI_GETFLATMENU     0x1022
#endif

#ifndef ODS_NOACCEL
#define ODS_NOACCEL         0x0100
#endif

#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX  0x00100000
#endif

#ifndef DT_PREFIXONLY
#define DT_PREFIXONLY  0x00200000
#endif

#ifndef SPI_GETKEYBOARDCUES
#define SPI_GETKEYBOARDCUES                 0x100A
#endif

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

#ifdef GetWindowLongPtrA
#undef GetWindowLongPtrA
#endif
inline LONG_PTR GetWindowLongPtrA( HWND hWnd, int nIndex )
{
  return( ::GetWindowLongA( hWnd, nIndex ) );
}

#ifdef GetWindowLongPtrW
#undef GetWindowLongPtrW
#endif
inline LONG_PTR GetWindowLongPtrW( HWND hWnd, int nIndex )
{
  return( ::GetWindowLongW( hWnd, nIndex ) );
}

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC        (-4)
#endif

#ifndef SetWindowLongPtr
#ifdef UNICODE
#define SetWindowLongPtr  SetWindowLongPtrW
#else
#define SetWindowLongPtr  SetWindowLongPtrA
#endif // !UNICODE
#endif //SetWindowLongPtr

#ifndef GetWindowLongPtr
#ifdef UNICODE
#define GetWindowLongPtr  GetWindowLongPtrW
#else
#define GetWindowLongPtr  GetWindowLongPtrA
#endif // !UNICODE
#endif // GetWindowLongPtr

#endif //_WIN64


// Count of menu icons normal gloomed and grayed
#define MENU_ICONS    3

BOOL bHighContrast = FALSE;
BOOL bWine = FALSE;

typedef BOOL (WINAPI* FktGradientFill)( IN HDC, IN PTRIVERTEX, IN ULONG, IN PVOID, IN ULONG, IN ULONG);
typedef BOOL (WINAPI* FktIsThemeActive)();
typedef HRESULT (WINAPI* FktSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

FktGradientFill pGradientFill = NULL;
FktIsThemeActive pIsThemeActive = NULL;
FktSetWindowTheme pSetWindowTheme = NULL;

BOOL GUILIBDLLEXPORT IsMenuThemeActive()
{
  return pIsThemeActive?pIsThemeActive():FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Helper datatypes
class CToolBarData
{
public:
  WORD wVersion;
  WORD wWidth;
  WORD wHeight;
  WORD wItemCount;
  //WORD aItems[wItemCount]
  WORD* items()
  { return (WORD*)(this+1); }
};

class CNewMenuIconInfo
{
public:
  WORD wBitmapID;
  WORD wWidth;
  WORD wHeight;

  WORD* ids(){ return (WORD*)(this+1); }
};

// Helpers for casting
__inline HMENU UIntToHMenu(const unsigned int ui )
{
  return( (HMENU)(UINT_PTR)ui );
}

__inline HMENU HWndToHMenu(const HWND hWnd )
{
  return( (HMENU)hWnd );
}

__inline HWND HMenuToHWnd(const HMENU hMenu)
{
  return( (HWND)hMenu );
}

__inline UINT HWndToUInt(const HWND hWnd )
{
  return( (UINT)(UINT_PTR) hWnd);
}

__inline HWND UIntToHWnd(const UINT hWnd )
{
  return( (HWND)(UINT_PTR) hWnd);
}

__inline UINT HMenuToUInt(const HMENU hMenu )
{
  return( (UINT)(UINT_PTR) hMenu);
}

__inline LONG_PTR LongToPTR(const LONG value )
{
  return( LONG_PTR)value;
}

#ifdef _DEBUG
static void ShowLastError(LPCTSTR pErrorTitle=NULL)
{
  if(pErrorTitle==NULL)
{
    pErrorTitle=_T("Error from Menu");
  }
  DWORD error = GetLastError();
  if(error)
  {
    LPVOID lpMsgBuf=NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0,
      NULL );
    if(lpMsgBuf)
    {
      // Display the string.
      MessageBox( NULL, (LPCTSTR)lpMsgBuf, pErrorTitle, MB_OK | MB_ICONINFORMATION );
      // Free the buffer.
      LocalFree( lpMsgBuf );
    }
    else
    {
      CString temp;
      temp.Format(_T("Error message 0x%lx not found"),error);
      // Display the string.
      MessageBox( NULL,temp, pErrorTitle, MB_OK | MB_ICONINFORMATION );
    }
  }
}
#else
#define ShowLastError(sz)
#endif


// this is a bad code! The timer 31 will be restartet, when it was used!!!!
UINT GetSafeTimerID(HWND hWnd, const UINT uiElapse)
{
  UINT nNewTimerId = NULL;
  if(IsWindow(hWnd))
  { // Try to start with TimerID 31, normaly is used 1, 10, or 100
    for(UINT nTimerId=31; nTimerId<100 && nNewTimerId==NULL; nTimerId++)
    { // Try to set a timer each uiElapse ms
      nNewTimerId = (UINT)::SetTimer(hWnd,nTimerId,uiElapse,NULL);
    }
    // Wow you have more than 69 timers!!!
    // Try to increase nTimerId<100 to nTimerId<1000;
    ASSERT(nNewTimerId);
  }
  return nNewTimerId;
}

WORD NumBitmapColors(LPBITMAPINFOHEADER lpBitmap)
{
  if ( lpBitmap->biClrUsed != 0)
    return (WORD)lpBitmap->biClrUsed;

  switch (lpBitmap->biBitCount)
  {
  case 1:
    return 2;
  case 4:
    return 16;
  case 8:
    return 256;
  }
  return 0;
}

int NumScreenColors()
{
  static int nColors = 0;
  if (!nColors)
  {
    // DC of the desktop
    CClientDC myDC(NULL);
    nColors = myDC.GetDeviceCaps(NUMCOLORS);
    if (nColors == -1)
    {
      nColors = 64000;
    }
  }
  return nColors;
}

HBITMAP LoadColorBitmap(LPCTSTR lpszResourceName, HMODULE hInst, int* pNumcol)
{
  if(hInst==0)
  {
    hInst = AfxFindResourceHandle(lpszResourceName, RT_BITMAP);
  }
  HRSRC hRsrc = ::FindResource(hInst,MAKEINTRESOURCE(lpszResourceName),RT_BITMAP);
  if (hRsrc == NULL)
    return NULL;

  // determine how many colors in the bitmap
  HGLOBAL hglb;
  if ((hglb = LoadResource(hInst, hRsrc)) == NULL)
    return NULL;

  LPBITMAPINFOHEADER lpBitmap = (LPBITMAPINFOHEADER)LockResource(hglb);
  if (lpBitmap == NULL)
    return NULL;
  WORD numcol = NumBitmapColors(lpBitmap);
  if(pNumcol)
  {
    *pNumcol = numcol;
  }

  UnlockResource(hglb);
  FreeResource(hglb);

  return LoadBitmap(hInst,lpszResourceName);
  //TODO: check it bpo
  //if(numcol!=16)
  //{
  //  return LoadBitmap(hInst,lpszResourceName);
  //  //  (HBITMAP)LoadImage(hInst,lpszResourceName,IMAGE_BITMAP,0,0,LR_DEFAULTCOLOR|LR_CREATEDIBSECTION|LR_SHARED);
  //}
  //else
  //{
  //  // mapping from color in DIB to system color
  //  // { RGB_TO_RGBQUAD(0x00, 0x00, 0x00),  COLOR_BTNTEXT },       // black
  //  // { RGB_TO_RGBQUAD(0x80, 0x80, 0x80),  COLOR_BTNSHADOW },     // dark gray
  //  // { RGB_TO_RGBQUAD(0xC0, 0xC0, 0xC0),  COLOR_BTNFACE },       // bright gray
  //  // { RGB_TO_RGBQUAD(0xFF, 0xFF, 0xFF),  COLOR_BTNHIGHLIGHT }   // white
  //  return (HBITMAP)AfxLoadSysColorBitmap(hInst,hRsrc,FALSE);
  //}
}

HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent)
{
  HBITMAP hbmMask = NULL;
  BITMAP bm = {0};
  if(GetObject(hbmColour, sizeof(BITMAP), &bm))
  {
    hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
    if(hbmMask)
    {
      // Get some HDCs that are compatible with the display driver
      HDC hdcMem = CreateCompatibleDC(0);
      HDC hdcMem2 = CreateCompatibleDC(0);

      SelectObject(hdcMem, hbmColour);
      SelectObject(hdcMem2, hbmMask);

      // Set the background colour of the colour image to the colour
      // you want to be transparent.
      SetBkColor(hdcMem, crTransparent);

      // Copy the bits from the colour image to the B+W mask... everything
      // with the background colour ends up white while everythig else ends up
      // black...Just what we wanted.
      BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

      // Take our new mask and use it to turn the transparent colour in our
      // original colour image to black so the transparency effect will
      // work right.
      BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem2, 0, 0, SRCINVERT);

      // Clean up.
      DeleteDC(hdcMem);
      DeleteDC(hdcMem2);
    }
  }
  return hbmMask;
}


COLORREF MakeGrayAlphablend(CBitmap* pBitmap, int weighting, COLORREF blendcolor)
{
  CDC myDC;
  // Create a compatible bitmap to the screen
  myDC.CreateCompatibleDC(0);
  // Select the bitmap into the DC
  CBitmap* pOldBitmap = myDC.SelectObject(pBitmap);

  BITMAP myInfo = {0};
  GetObject((HGDIOBJ)pBitmap->m_hObject,sizeof(myInfo),&myInfo);

  for (int nHIndex = 0; nHIndex < myInfo.bmHeight; nHIndex++)
  {
    for (int nWIndex = 0; nWIndex < myInfo.bmWidth; nWIndex++)
    {
      COLORREF ref = myDC.GetPixel(nWIndex,nHIndex);

      // make it gray
      DWORD nAvg =  (GetRValue(ref) + GetGValue(ref) + GetBValue(ref))/3;

      // Algorithme for alphablending
      //dest' = ((weighting * source) + ((255-weighting) * dest)) / 256
      DWORD refR = ((weighting * nAvg) + ((255-weighting) * GetRValue(blendcolor))) / 256;
      DWORD refG = ((weighting * nAvg) + ((255-weighting) * GetGValue(blendcolor))) / 256;;
      DWORD refB = ((weighting * nAvg) + ((255-weighting) * GetBValue(blendcolor))) / 256;;

      myDC.SetPixel(nWIndex,nHIndex,RGB(refR,refG,refB));
    }
  }
  COLORREF topLeftColor = myDC.GetPixel(0,0);
  myDC.SelectObject(pOldBitmap);
  return topLeftColor;
}

class CNewLoadLib
{
public:

  HMODULE m_hModule;
  void* m_pProg;

  CNewLoadLib(LPCTSTR pName,LPCSTR pProgName)
  {
    m_hModule = LoadLibrary(pName);
    m_pProg = m_hModule ? (void*)GetProcAddress (m_hModule,pProgName) : NULL;
  }

  ~CNewLoadLib()
  {
    if(m_hModule)
    {
      FreeLibrary(m_hModule);
      m_hModule = NULL;
    }
  }
};



#if(WINVER < 0x0500)

DECLARE_HANDLE(HMONITOR);

typedef struct tagMONITORINFO
{
  DWORD  cbSize;
  RECT   rcMonitor;
  RECT   rcWork;
  DWORD  dwFlags;
} MONITORINFO, *LPMONITORINFO;

BOOL GetMonitorInfo( HMONITOR hMonitor, LPMONITORINFO lpmi)
{
#ifdef UNICODE
  static CNewLoadLib menuInfo(_T("user32.dll"),"GetMonitorInfoW");
#else
  static CNewLoadLib menuInfo(_T("user32.dll"),"GetMonitorInfoA");
#endif
  if(menuInfo.m_pProg)
  {
    typedef BOOL (WINAPI* FktGetMonitorInfo)( HMONITOR,  LPMONITORINFO);
    return ((FktGetMonitorInfo)menuInfo.m_pProg)(hMonitor,lpmi);
  }
  return FALSE;
}

#define MONITOR_DEFAULTTONULL       0x00000000
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#define MONITOR_DEFAULTTONEAREST    0x00000002

#define MONITORINFOF_PRIMARY        0x00000001

HMONITOR MonitorFromWindow(
  HWND hwnd,       // handle to a window
  DWORD dwFlags    // determine return value
)
{
  static CNewLoadLib menuInfo(_T("user32.dll"),"MonitorFromWindow");
  if(menuInfo.m_pProg)
  {
    typedef HMONITOR (WINAPI* FktMonitorFromWindow)( HWND,  DWORD);
    return ((FktMonitorFromWindow)menuInfo.m_pProg)(hwnd,dwFlags);
  }
  return NULL;
}

HMONITOR MonitorFromRect(
  LPCRECT lprc,    // rectangle
  DWORD dwFlags    // determine return value
)
{
  static CNewLoadLib menuInfo(_T("user32.dll"),"MonitorFromRect");
  if(menuInfo.m_pProg)
  {
    typedef HMONITOR (WINAPI* FktMonitorFromRect)( LPCRECT,  DWORD);
    return ((FktMonitorFromRect)menuInfo.m_pProg)(lprc,dwFlags);
  }
  return NULL;
}


BOOL GetMenuInfo( HMENU hMenu, LPMENUINFO pInfo)
{
  static CNewLoadLib menuInfo(_T("user32.dll"),"GetMenuInfo");
  if(menuInfo.m_pProg)
  {
    typedef BOOL (WINAPI* FktGetMenuInfo)(HMENU, LPMENUINFO);
    return ((FktGetMenuInfo)menuInfo.m_pProg)(hMenu,pInfo);
  }
  return FALSE;
}

BOOL SetMenuInfo( HMENU hMenu, LPCMENUINFO pInfo)
{
  static CNewLoadLib menuInfo(_T("user32.dll"),"SetMenuInfo");
  if(menuInfo.m_pProg)
  {
    typedef BOOL (WINAPI* FktSetMenuInfo)(HMENU, LPCMENUINFO);
    return ((FktSetMenuInfo)menuInfo.m_pProg)(hMenu,pInfo);
  }
  return FALSE;
}

DWORD GetLayout(HDC hDC)
{
  static CNewLoadLib getLayout(_T("gdi32.dll"),"GetLayout");
  if(getLayout.m_pProg)
  {
    typedef DWORD (WINAPI* FktGetLayout)(HDC);
    return ((FktGetLayout)getLayout.m_pProg)(hDC);
  }
  return NULL;
}

DWORD SetLayout(HDC hDC, DWORD dwLayout)
{
  static CNewLoadLib setLayout(_T("gdi32.dll"),"SetLayout");
  if(setLayout.m_pProg)
  {
    typedef DWORD (WINAPI* FktSetLayout)(HDC,DWORD);
    return ((FktSetLayout)setLayout.m_pProg)(hDC,dwLayout);
  }
  return NULL;
}

#define WS_EX_LAYERED 0x00080000
#define LWA_ALPHA    2 // Use bAlpha to determine the opacity of the layer

BOOL SetLayeredWindowAttributes(HWND hWnd, COLORREF cr, BYTE bAlpha, DWORD dwFlags)
{
  static CNewLoadLib setLayout(_T("User32.dll"),"SetLayeredWindowAttributes");
  if(setLayout.m_pProg)
  {
    typedef  BOOL (WINAPI *lpfnSetLayeredWindowAttributes) (HWND hWnd, COLORREF cr, BYTE bAlpha, DWORD dwFlags);
    return ((lpfnSetLayeredWindowAttributes)setLayout.m_pProg)(hWnd, cr, bAlpha, dwFlags);
  }
  return false;
}



#endif

void MenuDrawText(HDC hDC ,LPCTSTR lpString,int nCount,LPRECT lpRect,UINT uFormat)
{
  if(nCount==-1)
  {
    nCount = lpString?(int)_tcslen(lpString):0;
  }
  LOGFONT logfont = {0};
  if(!GetObject(GetCurrentObject(hDC, OBJ_FONT),sizeof(logfont),&logfont))
  {
    logfont.lfOrientation = 0;
  }
  size_t bufSizeTChar = nCount + 1;
  TCHAR* pBuffer = (TCHAR*)_alloca(bufSizeTChar*sizeof(TCHAR));
  _tcsncpy_s(pBuffer,bufSizeTChar,lpString,nCount);
  pBuffer[nCount] = 0;

  UINT oldAlign =  GetTextAlign(hDC);

  const int nBorder=4;
  HGDIOBJ hOldFont = NULL;
  CFont TempFont;

  TEXTMETRIC textMetric;
  if (!GetTextMetrics(hDC, &textMetric))
  {
    textMetric.tmOverhang = 0;
    textMetric.tmAscent = 0;
  }
  else if ((textMetric.tmPitchAndFamily&(TMPF_VECTOR|TMPF_TRUETYPE))==0 )
  {
    // we have a bitmapfont it is not possible to rotate
    if(logfont.lfOrientation || logfont.lfEscapement)
    {
      hOldFont = GetCurrentObject(hDC,OBJ_FONT);
      _tcscpy_s(logfont.lfFaceName,ARRAY_SIZE(logfont.lfFaceName),_T("Arial"));
      // we need a truetype font for rotation
      logfont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
      // the font will be destroyed at the end by destructor
      TempFont.CreateFontIndirect(&logfont);
      // we select at the end the old font
      SelectObject(hDC,TempFont);
      GetTextMetrics(hDC, &textMetric);
    }
  }

  DWORD dwLayout = GetLayout(hDC);
  if(dwLayout&LAYOUT_RTL)
  {
    SetTextAlign (hDC,TA_RIGHT|TA_TOP|TA_UPDATECP);
  }
  else
  {
    SetTextAlign (hDC,TA_LEFT|TA_TOP|TA_UPDATECP);
  }
  CPoint pos(lpRect->left,lpRect->top);

  if( (uFormat&DT_VCENTER) &&lpRect)
  {
    switch(logfont.lfOrientation%3600)
    {
    default:
    case 0:
      logfont.lfOrientation = 0;
      pos.y = (lpRect->top+lpRect->bottom-textMetric.tmHeight)/2;
      if(dwLayout&LAYOUT_RTL)
      {
        pos.x = lpRect->right - nBorder;
      }
      else
      {
        pos.x = lpRect->left + nBorder;
      }
      break;

    case 1800:
    case -1800:
      logfont.lfOrientation = 1800;
      pos.y = (lpRect->top+textMetric.tmHeight +lpRect->bottom)/2;
      if(dwLayout&LAYOUT_RTL)
      {
        pos.x = lpRect->left + nBorder;
      }
      else
      {
        pos.x = lpRect->right - nBorder;
      }
      break;

    case 900:
    case -2700:
      logfont.lfOrientation = 900;
      pos.x = (lpRect->left+lpRect->right-textMetric.tmHeight)/2;
      pos.y = lpRect->bottom - nBorder;
      if(dwLayout&LAYOUT_RTL)
      {
        pos.y = lpRect->top + nBorder;
      }
      else
      {
        pos.y = lpRect->bottom - nBorder;
      }
      break;

    case -900:
    case 2700:
      logfont.lfOrientation = 2700;
      pos.x = (lpRect->left+lpRect->right+textMetric.tmHeight)/2;
      if(dwLayout&LAYOUT_RTL)
      {
        pos.y = lpRect->bottom - nBorder;
      }
      else
      {
        pos.y = lpRect->top + nBorder;
      }
      break;
    }
  }

  CPoint oldPos;
  MoveToEx(hDC,pos.x,pos.y,&oldPos);

  while(nCount)
  {
    TCHAR *pTemp =_tcsstr(pBuffer,_T("&"));
    if(pTemp)
    {
      // we found &
      if(*(pTemp+1)==_T('&'))
      {
        // the different is in character unicode and byte works
        int nTempCount = DWORD(pTemp-pBuffer)+1;
        ExtTextOut(hDC,pos.x,pos.y,ETO_CLIPPED,lpRect,pBuffer,nTempCount,NULL);
        nCount -= nTempCount+1;
        pBuffer = pTemp+2;
      }
      else
      {
        // draw underline the different is in character unicode and byte works
        int nTempCount = DWORD(pTemp-pBuffer);
        ExtTextOut(hDC,pos.x,pos.y,ETO_CLIPPED,lpRect,pBuffer,nTempCount,NULL);
        nCount -= nTempCount+1;
        pBuffer = pTemp+1;
        if(!(uFormat&DT_HIDEPREFIX) )
        {
          CSize size;
          GetTextExtentPoint(hDC, pTemp+1, 1, &size);
          GetCurrentPositionEx(hDC,&pos);

          COLORREF oldColor = SetBkColor(hDC, GetTextColor(hDC));
          LONG cx = size.cx - textMetric.tmOverhang / 2;
          LONG nTop;
          CRect rc;
          switch(logfont.lfOrientation)
          {
          case 0:
            // Get height of text so that underline is at bottom.
            nTop = pos.y + textMetric.tmAscent + 1;
            // Draw the underline using the foreground color.
            if(dwLayout&LAYOUT_RTL)
            {
              rc.SetRect(pos.x-cx+2, nTop, pos.x+1, nTop+1);
            }
            else
            {
              rc.SetRect(pos.x, nTop, pos.x+cx, nTop+1);
            }
            ExtTextOut(hDC, pos.x, nTop, ETO_OPAQUE, &rc, _T(""), 0, NULL);
            break;

          case 1800:
            // Get height of text so that underline is at bottom.
            nTop = pos.y -(textMetric.tmAscent + 1);
            // Draw the underline using the foreground color.
            if(dwLayout&LAYOUT_RTL)
            {
              rc.SetRect(pos.x-1, nTop-1, pos.x+cx-1, nTop);
            }
            else
            {
              rc.SetRect(pos.x-cx, nTop-1, pos.x, nTop);
            }
            ExtTextOut(hDC, pos.x, nTop, ETO_OPAQUE, &rc, _T(""), 0, NULL);
            break;

          case 900:
            // draw up
            // Get height of text so that underline is at bottom.
            nTop = pos.x + (textMetric.tmAscent + 1);
            // Draw the underline using the foreground color.
            if(dwLayout&LAYOUT_RTL)
    {
              rc.SetRect(nTop-1, pos.y, nTop, pos.y+cx-2);
    }
    else
    {
              rc.SetRect(nTop, pos.y-cx, nTop+1, pos.y);
            }
            ExtTextOut(hDC, nTop, pos.y, ETO_OPAQUE, &rc, _T(""), 0, NULL);
            break;

          case 2700:
            // draw down
            // Get height of text so that underline is at bottom.
            nTop = pos.x -(textMetric.tmAscent + 1);
            // Draw the underline using the foreground color.
            if(dwLayout&LAYOUT_RTL)
            {
              rc.SetRect(nTop, pos.y-cx+1 , nTop+1, pos.y);
    }
            else
            {
              rc.SetRect(nTop-1, pos.y, nTop, pos.y+cx);
  }
            ExtTextOut(hDC, nTop, pos.y, ETO_OPAQUE, &rc, _T(""), 0, NULL);
            break;
}
          SetBkColor(hDC, oldColor);
          // corect the actual drawingpoint
          MoveToEx(hDC,pos.x,pos.y,NULL);
    }
  }
}
    else
{
      // draw the rest of the string
      ExtTextOut(hDC,pos.x,pos.y,ETO_CLIPPED,lpRect,pBuffer,nCount,NULL);
      break;
    }
  }
  // restore old point
  MoveToEx(hDC,oldPos.x,oldPos.y,NULL);
  // restore old align
  SetTextAlign(hDC,oldAlign);

  if(hOldFont!=NULL)
  {
    SelectObject(hDC,hOldFont);
  }
}




CRect GUILIBDLLEXPORT GetScreenRectFromWnd(HWND hWnd)
{
  CRect rect;
  HMONITOR hMonitor = MonitorFromWindow(hWnd,MONITOR_DEFAULTTONEAREST);
  MONITORINFO monitorinfo={0};
  monitorinfo.cbSize = sizeof(monitorinfo);
  if(hMonitor!=NULL && GetMonitorInfo(hMonitor,&monitorinfo))
  {
    rect = monitorinfo.rcWork;
  }
  else
    {
    rect.SetRect(0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
    }
  return rect;
}

CRect GUILIBDLLEXPORT GetScreenRectFromRect(LPCRECT pRect)
{
  CRect rect;
  HMONITOR hMonitor = MonitorFromRect(pRect,MONITOR_DEFAULTTONEAREST);
  MONITORINFO monitorinfo={0};
  monitorinfo.cbSize = sizeof(monitorinfo);
  if(hMonitor!=NULL && GetMonitorInfo(hMonitor,&monitorinfo))
  {
    rect = monitorinfo.rcWork;
  }
  else
  {
    rect.SetRect(0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
  }
  return rect;
}

 BOOL GUILIBDLLEXPORT TrackPopupMenuSpecial(CMenu *pMenu, UINT uFlags, CRect rcTBItem, CWnd *pWndCommand, BOOL bOnSide)
{
   CRect rect = GetScreenRectFromRect(rcTBItem);
   CPoint CenterPoint = rect.CenterPoint();
   CNewMenu* pNewMenu = DYNAMIC_DOWNCAST(CNewMenu,pMenu);

   if(bOnSide)
   {
     if(CenterPoint.x<rcTBItem.right)
     {
       // menu on the right side of the screen
       if(rcTBItem.left<=rect.left){rcTBItem.left=rect.left+1;}
       if(rcTBItem.top<rect.top){rcTBItem.top=rect.top;}
       if(IsMirroredWnd(pWndCommand->GetSafeHwnd()))
  {
        return pMenu->TrackPopupMenu(TPM_LEFTBUTTON|uFlags, rcTBItem.left-1, rcTBItem.top+1,pWndCommand,NULL);
       }
       else
    {
        return pMenu->TrackPopupMenu(TPM_LEFTBUTTON|TPM_RIGHTALIGN|uFlags, rcTBItem.left-1, rcTBItem.top+1,pWndCommand,NULL);
       }
    }
     else
     {
       if(rcTBItem.right<=rect.left){rcTBItem.right=rect.left+1;}
       if(rcTBItem.top<rect.top){rcTBItem.top=rect.top;}
       return pMenu->TrackPopupMenu(TPM_LEFTBUTTON|uFlags, rcTBItem.right-1, rcTBItem.top+1,pWndCommand,NULL);
  }
}
   else
{
     int nMenuHeight = 0;
     if(pNewMenu)
  {
       CSize size = pNewMenu->GetIconSize();
       // calculate more or less the height, seperators and titles may be wrong
       nMenuHeight = pNewMenu->GetMenuItemCount()*size.cy;
  }

     bool bMenuAbove = CenterPoint.y<rcTBItem.top;
     if(bMenuAbove && nMenuHeight)
  {
       if((rcTBItem.bottom+nMenuHeight)<rect.bottom)
    {
         bMenuAbove = false;
    }
  }

     if(bMenuAbove)
     {
       // bottom of the screen
       if(rcTBItem.left<rect.left){rcTBItem.left=rect.left;}
       if(rcTBItem.top>rect.bottom){rcTBItem.top=rect.bottom;}
       if(IsMirroredWnd(pWndCommand->GetSafeHwnd()))
{
        return pMenu->TrackPopupMenu(TPM_LEFTBUTTON|TPM_BOTTOMALIGN|TPM_RIGHTALIGN|uFlags, rcTBItem.left, rcTBItem.top,pWndCommand,rcTBItem);
       }
       else
  {
        return pMenu->TrackPopupMenu(TPM_LEFTBUTTON|TPM_BOTTOMALIGN|uFlags, rcTBItem.left, rcTBItem.top,pWndCommand,rcTBItem);
  }
}
     else
     {
       // top of the screen
       if(rcTBItem.left<rect.left){rcTBItem.left=rect.left;}
       if(rcTBItem.bottom<rect.top){rcTBItem.bottom=rect.top;}
       if(IsMirroredWnd(pWndCommand->GetSafeHwnd()))
{
        return pMenu->TrackPopupMenu(TPM_LEFTBUTTON|TPM_RIGHTALIGN|uFlags, rcTBItem.left, rcTBItem.bottom,pWndCommand,rcTBItem);
       }
       else
  {
        return pMenu->TrackPopupMenu(TPM_LEFTBUTTON|uFlags, rcTBItem.left, rcTBItem.bottom,pWndCommand,rcTBItem);
       }
     }
  }
}

class CNewBrushList : public CObList
{
public:
  CNewBrushList(){}
  ~CNewBrushList()
  {
    while(!IsEmpty())
    {
      delete RemoveTail();
    }
  }
};

class CNewBrush : public CBrush
{
public:
  UINT m_nMenuDrawMode;
  COLORREF m_BarColor;
  COLORREF m_BarColor2;

  CNewBrush(UINT menuDrawMode, COLORREF barColor, COLORREF barColor2):m_nMenuDrawMode(menuDrawMode),m_BarColor(barColor),m_BarColor2(barColor2)
  {
    if (g_Shell!=WinNT4 &&
        g_Shell!=Win95 &&
        (menuDrawMode==CNewMenu::STYLE_XP_2003 || menuDrawMode==CNewMenu::STYLE_COLORFUL))
    {
      // Get the desktop hDC...
      HDC hDcDsk = GetWindowDC(0);
      CDC* pDcDsk = CDC::FromHandle(hDcDsk);

      CDC clientDC;
      clientDC.CreateCompatibleDC(pDcDsk);

      CRect rect(0,0,(GetSystemMetrics(SM_CXFULLSCREEN)+16)&~7,20);

      CBitmap bitmap;
      bitmap.CreateCompatibleBitmap(pDcDsk,rect.Width(),rect.Height());
      CBitmap* pOldBitmap = clientDC.SelectObject(&bitmap);

      int nRight = rect.right;
      if(rect.right>700)
      {
        rect.right  = 700;
        DrawGradient(&clientDC,rect,barColor,barColor2,TRUE,TRUE);
        rect.left = rect.right;
        rect.right = nRight;
        clientDC.FillSolidRect(rect, barColor2);
      }
      else
      {
        DrawGradient(&clientDC,rect,barColor,barColor2,TRUE,TRUE);
      }
      clientDC.SelectObject(pOldBitmap);

      // Release the desktopdc
      ReleaseDC(0,hDcDsk);

      CreatePatternBrush(&bitmap);
    }
    else
    {
      CreateSolidBrush(barColor);
    }
  }
};

CBrush* GetMenuBarBrush()
{
  // The brushes will be destroyed at program-end => Not a memory-leak
  static CNewBrushList brushList;
  static CNewBrush* lastBrush = NULL;

  COLORREF menuBarColor;
  COLORREF menuBarColor2;  //JUS

  UINT nMenuDrawMode = CNewMenu::GetMenuDrawMode();

  switch(nMenuDrawMode)
  {
  case CNewMenu::STYLE_XP_2003 :
  case CNewMenu::STYLE_XP_2003_NOBORDER :
    {
      CNewMenu::GetMenuBarColor2003(menuBarColor, menuBarColor2);
      if (NumScreenColors()>=256 && !bHighContrast)
      {
        nMenuDrawMode = CNewMenu::STYLE_XP_2003;
      }
      else
      {
       menuBarColor = menuBarColor2 = GetSysColor(COLOR_3DFACE);
       nMenuDrawMode = CNewMenu::STYLE_XP;
      }
    }
    break;

  case CNewMenu::STYLE_COLORFUL :
  case CNewMenu::STYLE_COLORFUL_NOBORDER :
    {
      CNewMenu::GetMenuBarColor2003(menuBarColor, menuBarColor2);
      if (NumScreenColors()>=256 && !bHighContrast)
      {
        nMenuDrawMode = CNewMenu::STYLE_COLORFUL;
      }
      else
      {
        nMenuDrawMode = CNewMenu::STYLE_XP;
      }
    }
    break;

  case CNewMenu::STYLE_XP :
  case CNewMenu::STYLE_XP_NOBORDER :
    menuBarColor = menuBarColor2 = GetSysColor(COLOR_3DFACE);
    nMenuDrawMode = CNewMenu::STYLE_XP;
    break;

  default:
    return NULL;
  }

  // check if the last brush the one which we want
  if(lastBrush!=NULL &&
     lastBrush->m_nMenuDrawMode==nMenuDrawMode &&
     lastBrush->m_BarColor==menuBarColor &&
     lastBrush->m_BarColor2==menuBarColor2)
  {
    return lastBrush;
  }

  // Check if the brush is allready created
  POSITION pos = brushList.GetHeadPosition();
  while (pos)
  {
    lastBrush = (CNewBrush*)brushList.GetNext(pos);
    if(lastBrush!=NULL &&
      lastBrush->m_nMenuDrawMode==nMenuDrawMode &&
      lastBrush->m_BarColor==menuBarColor &&
      lastBrush->m_BarColor2==menuBarColor2)
    {
      return lastBrush;
    }
  }
  // create a new one and insert into the list
  brushList.AddHead(lastBrush = new CNewBrush(nMenuDrawMode,menuBarColor,menuBarColor2));
  return lastBrush;
}

void UpdateMenuBarColor(HMENU hMenu)
{
  CBrush* pBrush = GetMenuBarBrush();

  // for WindowsBlind activating it's better for not to change menubar background
  if(!pBrush /* || (pIsThemeActive && pIsThemeActive()) */ )
  {
    // menubackground hasn't been set
    return;
  }

  MENUINFO menuInfo = {0};
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.hbrBack = *pBrush;
  menuInfo.fMask = MIM_BACKGROUND;

  // Change color only for CNewMenu and derived classes
  if(IsMenu(hMenu) && DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(hMenu))!=NULL)
  {
    SetMenuInfo(hMenu,&menuInfo);
  }

  CWinApp* pWinApp = AfxGetApp();
  // Update menu from template
  if(pWinApp)
  {
    CDocManager* pManager = pWinApp->m_pDocManager;
    if(pManager)
    {
      POSITION pos = pManager->GetFirstDocTemplatePosition();
      while(pos)
      {
        CDocTemplate* pTemplate = pManager->GetNextDocTemplate(pos);
        CMultiDocTemplate* pMultiTemplate = DYNAMIC_DOWNCAST(CMultiDocTemplate,pTemplate);
        if(pMultiTemplate)
        {
          // Change color only for CNewMenu and derived classes
          if(DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pMultiTemplate->m_hMenuShared))!=NULL)
          {
            // need for correct menubar color
            SetMenuInfo(pMultiTemplate->m_hMenuShared,&menuInfo);
          }
        }

        /*      // does not work with embeded menues
        if(pTemplate)
        {
        // Change color only for CNewMenu and derived classes
        // need for correct menubar color
        if(DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pTemplate->m_hMenuInPlace))!=NULL)
        {
        // menu & accelerator resources for in-place container
        SetMenuInfo(pTemplate->m_hMenuInPlace,&menuInfo);
        }
        if(DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pTemplate->m_hMenuEmbedding))!=NULL)
        {
        // menu & accelerator resource for server editing embedding
        SetMenuInfo(pTemplate->m_hMenuEmbedding,&menuInfo);
        }
        if(DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pTemplate->m_hMenuInPlaceServer))!=NULL)
        {
        // menu & accelerator resource for server editing in-place
        SetMenuInfo(pTemplate->m_hMenuInPlaceServer,&menuInfo);
        }
        }
        */
      }
    }
  }
}

BOOL DrawMenubarItem(CWnd* pWnd,CMenu* pMenu, UINT nItemIndex,UINT nState)
{
  CRect itemRect;
  if (nItemIndex!=UINT(-1) && GetMenuItemRect(pWnd->m_hWnd,pMenu->m_hMenu, nItemIndex, &itemRect))
  {
    MENUITEMINFO menuInfo = {0};
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIIM_DATA|MIIM_TYPE|MIIM_ID;
    if(pMenu->GetMenuItemInfo(nItemIndex,&menuInfo,TRUE))
    {
      // Only ownerdrawn were allowed
      if(menuInfo.fType&MF_OWNERDRAW)
      {
        CWindowDC dc(pWnd);

        CFont fontMenu;
        LOGFONT logFontMenu;

#ifdef _NEW_MENU_USER_FONT
        logFontMenu =  MENU_USER_FONT;
#else
        NONCLIENTMETRICS nm = {0};
        nm.cbSize = sizeof (NONCLIENTMETRICS);
        VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
        logFontMenu =  nm.lfMenuFont;
#endif

        fontMenu.CreateFontIndirect (&logFontMenu);
        CFont* pOldFont = dc.SelectObject(&fontMenu);

        CRect wndRect;
        GetWindowRect(*pWnd,wndRect);
        itemRect.OffsetRect(-wndRect.TopLeft());

        // Bugfix for wine
        if(bWine)
        {
          if(!(GetWindowLongPtr(*pWnd, GWL_STYLE)&WS_THICKFRAME) )
          {
            itemRect.OffsetRect(-GetSystemMetrics(SM_CXFIXEDFRAME),
                                -(GetSystemMetrics(SM_CYMENU)+GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFIXEDFRAME)+1));
          }
          else
          {
            itemRect.OffsetRect(-GetSystemMetrics(SM_CXFRAME),
                                -(GetSystemMetrics(SM_CYMENU)+GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFRAME)+1));
          }
        }

        DRAWITEMSTRUCT drwItem = {0};
        drwItem.CtlType = ODT_MENU;
        drwItem.hwndItem = HMenuToHWnd(pMenu->m_hMenu);
        drwItem.itemID = menuInfo.wID;
        drwItem.itemData = menuInfo.dwItemData;
        drwItem.rcItem = itemRect;
        drwItem.hDC = dc.m_hDC;
        drwItem.itemState = nState;
        drwItem.itemAction = ODA_DRAWENTIRE;

        ASSERT(menuInfo.dwItemData);

        CNewMenu::m_dwLastActiveItem = (DWORD)menuInfo.dwItemData;

        CPoint windowOrg;
        SetWindowOrgEx(dc.m_hDC,0,0,&windowOrg);
        SendMessage(pWnd->GetSafeHwnd(),WM_DRAWITEM,NULL,(LPARAM)&drwItem);
        SetWindowOrgEx(dc.m_hDC,windowOrg.x,windowOrg.y,NULL);

        dc.SelectObject(pOldFont);

        return TRUE;
      }
    }
  }
  return FALSE;
}

Win32Type IsShellType()
{
  OSVERSIONINFO osvi = {0};
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  DWORD winVer=GetVersion();
  if(winVer<0x80000000)
  {/*NT */
    if(!GetVersionEx(&osvi))
    {
      ShowLastError(_T("Error from Menu: GetVersionEx NT"));
    }
    if(osvi.dwMajorVersion==4L)
    {
      return WinNT4;
    }
    else if(osvi.dwMajorVersion==5L && osvi.dwMinorVersion==0L)
    {
      return Win2000;
    }
    // thanks to John Zegers
    else if(osvi.dwMajorVersion>=5L)// && osvi.dwMinorVersion==1L)
    {
      return WinXP;
    }
    return WinNT3;
  }
  else if (LOBYTE(LOWORD(winVer))<4)
  {
    return Win32s;
  }

  if(!GetVersionEx(&osvi))
  {
    ShowLastError(_T("Error from Menu: GetVersionEx"));
  }
  if(osvi.dwMajorVersion==4L && osvi.dwMinorVersion==10L)
  {
    return Win98;
  }
  else if(osvi.dwMajorVersion==4L && osvi.dwMinorVersion==90L)
  {
    return WinME;
  }
  return Win95;
}

BOOL IsShadowEnabled()
{
  BOOL bEnabled = FALSE;
  if(SystemParametersInfo(SPI_GETDROPSHADOW,0,&bEnabled,0))
  {
    return bEnabled;
  }
  return FALSE;
}

COLORREF DarkenColorXP(COLORREF color)
{
  return RGB( MulDiv(GetRValue(color),7,10),
    MulDiv(GetGValue(color),7,10),
    MulDiv(GetBValue(color)+55,7,10));
}

// Function splits a color into its RGB components and
// transforms the color using a scale 0..255
COLORREF DarkenColor( long lScale, COLORREF lColor)
{
  long red   = MulDiv(GetRValue(lColor),(255-lScale),255);
  long green = MulDiv(GetGValue(lColor),(255-lScale),255);
  long blue  = MulDiv(GetBValue(lColor),(255-lScale),255);

  return RGB(red, green, blue);
}

COLORREF MixedColor(COLORREF colorA,COLORREF colorB)
{
  // ( 86a + 14b ) / 100
  int red   = MulDiv(86,GetRValue(colorA),100) + MulDiv(14,GetRValue(colorB),100);
  int green = MulDiv(86,GetGValue(colorA),100) + MulDiv(14,GetGValue(colorB),100);
  int blue  = MulDiv(86,GetBValue(colorA),100) + MulDiv(14,GetBValue(colorB),100);

  return RGB( min(red,0xff),min(green,0xff), min(blue,0xff));
}

COLORREF MidColor(COLORREF colorA,COLORREF colorB)
{
  // (7a + 3b)/10
  int red   = MulDiv(7,GetRValue(colorA),10) + MulDiv(3,GetRValue(colorB),10);
  int green = MulDiv(7,GetGValue(colorA),10) + MulDiv(3,GetGValue(colorB),10);
  int blue  = MulDiv(7,GetBValue(colorA),10) + MulDiv(3,GetBValue(colorB),10);

  return RGB( min(red,0xff),min(green,0xff), min(blue,0xff));
}

COLORREF GrayColor(COLORREF crColor)
{
  int Gray  = (((int)GetRValue(crColor)) + GetGValue(crColor) + GetBValue(crColor))/3;

  return RGB( Gray,Gray,Gray);
}

BOOL IsLightColor(COLORREF crColor)
{
  return (((int)GetRValue(crColor)) + GetGValue(crColor) + GetBValue(crColor))>(3*128);
}

COLORREF GetXpHighlightColor()
{
  if(bHighContrast)
  {
    return GetSysColor(COLOR_HIGHLIGHT);
  }

  if (NumScreenColors() > 256)
  {
    // May-05-2005 - Mark P. Peterson (mpp@rhinosoft.com) - Changed to use "MixedColor1()" instead when not using XP Theme mode.
    // It appears that using MidColor() in many non XP Themes cases returns the wrong color, this needs to be much more like the
    // highlight color as the user has selected in Windows.  If the highlight color is YELLOW, for example, and the COLOR_WINDOW
    // value is WHITE, using MidColor() the function returns a dark blue color making the menus too hard to read.
    if ((! IsMenuThemeActive()) && (IsLightColor(GetSysColor(COLOR_HIGHLIGHT))))
      return MixedColor(GetSysColor(COLOR_WINDOW),GetSysColor(COLOR_HIGHLIGHT));
    else
      return MidColor(GetSysColor(COLOR_WINDOW),GetSysColor(COLOR_HIGHLIGHT));
	// as released by Bruno
//    return MidColor(GetSysColor(COLOR_WINDOW),GetSysColor(COLOR_HIGHLIGHT));
  }
  return GetSysColor(COLOR_WINDOW);
}

// Function splits a color into its RGB components and
// transforms the color using a scale 0..255
COLORREF LightenColor( long lScale, COLORREF lColor)
{
  long R = MulDiv(255-GetRValue(lColor),lScale,255)+GetRValue(lColor);
  long G = MulDiv(255-GetGValue(lColor),lScale,255)+GetGValue(lColor);
  long B = MulDiv(255-GetBValue(lColor),lScale,255)+GetBValue(lColor);

  return RGB(R, G, B);
}

COLORREF BleachColor(int Add, COLORREF color)
{
  return RGB( min (GetRValue(color)+Add, 255),
              min (GetGValue(color)+Add, 255),
              min (GetBValue(color)+Add, 255));
}

COLORREF GetAlphaBlendColor(COLORREF blendColor, COLORREF pixelColor,int weighting)
{
  if(pixelColor==CLR_NONE)
  {
    return CLR_NONE;
  }
  // Algorithme for alphablending
  //dest' = ((weighting * source) + ((255-weighting) * dest)) / 256
  DWORD refR = ((weighting * GetRValue(pixelColor)) + ((255-weighting) * GetRValue(blendColor))) / 256;
  DWORD refG = ((weighting * GetGValue(pixelColor)) + ((255-weighting) * GetGValue(blendColor))) / 256;;
  DWORD refB = ((weighting * GetBValue(pixelColor)) + ((255-weighting) * GetBValue(blendColor))) / 256;;

  return RGB(refR,refG,refB);
}


void DrawGradient(CDC* pDC,CRect& Rect,
                  COLORREF StartColor,COLORREF EndColor,
                  BOOL bHorizontal,BOOL bUseSolid)
{
  int Count = pDC->GetDeviceCaps(NUMCOLORS);
  if(Count==-1)
    bUseSolid = FALSE;

  // for running under win95 and WinNt 4.0 without loading Msimg32.dll
  if(!bUseSolid && pGradientFill )
  {
    TRIVERTEX vert[2];
    GRADIENT_RECT gRect;

    vert [0].y = Rect.top;
    vert [0].x = Rect.left;

    vert [0].Red    = COLOR16(COLOR16(GetRValue(StartColor))<<8);
    vert [0].Green  = COLOR16(COLOR16(GetGValue(StartColor))<<8);
    vert [0].Blue   = COLOR16(COLOR16(GetBValue(StartColor))<<8);
    vert [0].Alpha  = 0x0000;

    vert [1].y = Rect.bottom;
    vert [1].x = Rect.right;

    vert [1].Red    = COLOR16(COLOR16(GetRValue(EndColor))<<8);
    vert [1].Green  = COLOR16(COLOR16(GetGValue(EndColor))<<8);
    vert [1].Blue   = COLOR16(COLOR16(GetBValue(EndColor))<<8);
    vert [1].Alpha  = 0x0000;

    gRect.UpperLeft  = 0;
    gRect.LowerRight = 1;

    if(bHorizontal)
    {
      pGradientFill(pDC->m_hDC,vert,2,&gRect,1,GRADIENT_FILL_RECT_H);
    }
    else
    {
      pGradientFill(pDC->m_hDC,vert,2,&gRect,1,GRADIENT_FILL_RECT_V);
    }
  }
  else
  {
    BYTE StartRed   = GetRValue(StartColor);
    BYTE StartGreen = GetGValue(StartColor);
    BYTE StartBlue  = GetBValue(StartColor);

    BYTE EndRed    = GetRValue(EndColor);
    BYTE EndGreen  = GetGValue(EndColor);
    BYTE EndBlue   = GetBValue(EndColor);

    int n = (bHorizontal)?Rect.Width():Rect.Height();

    // only need for the rest, can be optimized
    {
      if(bUseSolid)
      {
        // We need a solid brush (can not be doted)
        pDC->FillSolidRect(Rect,pDC->GetNearestColor(EndColor));
      }
      else
      {
        // We need a brush (can be doted)
        CBrush TempBrush(EndColor);
        pDC->FillRect(Rect,&TempBrush);
      }
    }
    int dy = 2;
    n-=dy;
    for(int dn=0;dn<=n;dn+=dy)
    {
      BYTE ActRed   = (BYTE)(MulDiv(int(EndRed)  -StartRed,  dn,n)+StartRed);
      BYTE ActGreen = (BYTE)(MulDiv(int(EndGreen)-StartGreen,dn,n)+StartGreen);
      BYTE ActBlue  = (BYTE)(MulDiv(int(EndBlue) -StartBlue,  dn,n)+StartBlue);

      CRect TempRect;
      if(bHorizontal)
      {
        TempRect = CRect(CPoint(Rect.left+dn,Rect.top),CSize(dy,Rect.Height()));
      }
      else
      {
        TempRect = CRect(CPoint(Rect.left,Rect.top+dn),CSize(Rect.Width(),dy));
      }
      if(bUseSolid)
      {
        pDC->FillSolidRect(TempRect,pDC->GetNearestColor(RGB(ActRed,ActGreen,ActBlue)));
      }
      else
      {
        CBrush TempBrush(RGB(ActRed,ActGreen,ActBlue));
        pDC->FillRect(TempRect,&TempBrush);
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// CNewMenuHook important class for subclassing menus!

class CNewMenuHook
{
  friend class CNewMenuThreadHook;
public:
  class CMenuHookData
  {
  public:
    CMenuHookData(HWND hWnd,BOOL bSpecialWnd)
      : m_dwData(bSpecialWnd),m_bDrawBorder(FALSE),m_Point(0,0),
      m_hRgn((HRGN)1),m_bDoSubclass(TRUE)
      //, m_hRightShade(NULL),m_hBottomShade(NULL),m_TimerID(0)
    {
      // Safe actual menu
      SetMenu(CNewMenuHook::m_hLastMenu);
      // Reset for the next menu
      CNewMenuHook::m_hLastMenu = NULL;

      // Save actual border setting etc.
      m_dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
      m_dwExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

      //if(pSetWindowTheme)pSetWindowTheme(hWnd,L" ",L" ");
    }

    ~CMenuHookData()
    {
      if(m_hRgn!=(HRGN)1)
      {
        DeleteObject(m_hRgn);
        m_hRgn = (HRGN)1;
      }
    }

    BOOL SetMenu(HMENU hMenu)
    {
      m_hMenu = hMenu;
      if(!CNewMenu::GetNewMenuBorderAllMenu() &&
        !DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(hMenu)))
      {
        m_bDoSubclass = FALSE;
      }
      else
      {
        m_bDoSubclass = TRUE;
      }
      // When I change the backgroundcolor the borderdrawing well be wrong (scrollmenu)
      //if(IsMenu(hMenu) &&
      //    CNewMenu::GetMenuDrawMode()==CNewMenu::STYLE_XP ||
      //    CNewMenu::GetMenuDrawMode()==CNewMenu::STYLE_XP_NOBORDER )
      //{
      //  MENUINFO menuInfo;
      //  ZeroMemory(&menuInfo,sizeof(menuInfo));
      //  menuInfo.cbSize = sizeof(menuInfo);
      //  menuInfo.hbrBack = g_MenuBrush;
      //  menuInfo.fMask = MIM_BACKGROUND;
      //  ::SetMenuInfo(hMenu,&menuInfo);
      //}
      return m_bDoSubclass;
    }
    LONG_PTR m_dwStyle;
    LONG_PTR m_dwExStyle;

    CPoint m_Point;
    DWORD m_dwData; //  1=Sepcial WND, 2=Styles Changed,4=VK_ESCAPE, 8=in Print

    BOOL m_bDrawBorder;
    HMENU m_hMenu;

    CBitmap m_Screen;
    HRGN m_hRgn;

    BOOL m_bDoSubclass;

    //HWND m_hRightShade;
    //HWND m_hBottomShade;
    //UINT m_TimerID;
  };

public:
  CNewMenuHook();
  ~CNewMenuHook();

public:
  static CMenuHookData* GetMenuHookData(HWND hWnd);

  static BOOL AddTheme(CMenuTheme*);
  static CMenuTheme* RemoveTheme(DWORD dwThemeId);
  static CMenuTheme* FindTheme(DWORD dwThemeId);

private:
  static LRESULT CALLBACK NewMenuHook(int code, WPARAM wParam, LPARAM lParam);
  static BOOL CheckSubclassing(HWND hWnd,BOOL bSpecialWnd);
  static LRESULT CALLBACK SubClassMenu(HWND hWnd,  UINT uMsg, WPARAM wParam,  LPARAM lParam );
  static void UnsubClassMenu(HWND hWnd);

  static BOOL SubClassMenu2(HWND hWnd,  UINT uMsg, WPARAM wParam,  LPARAM lParam, DWORD* pResult);

public:
  static HMENU m_hLastMenu;
  static DWORD m_dwMsgPos;
  static DWORD m_bSubclassFlag;

private:
  static HMODULE m_hLibrary;
  static HMODULE m_hThemeLibrary;
  static HHOOK HookOldMenuCbtFilter;

#ifdef _TRACE_MENU_LOGFILE
  static HANDLE m_hLogFile;
#endif //_TRACE_MENU_LOGFILE

  // an map of actual opened Menu and submenu
  static CTypedPtrMap<CMapPtrToPtr,HWND,CMenuHookData*> m_MenuHookData;
  // Stores list of all defined Themes
  static CTypedPtrList<CPtrList, CMenuTheme*>* m_pRegisteredThemesList;
};

/////////////////////////////////////////////////////////////////////////////
// CNewMenuIconLock Helperclass for reference-counting !

class CNewMenuIconLock
{
  CNewMenuIcons* m_pIcons;

public:
  CNewMenuIconLock(CNewMenuIcons* pIcons):m_pIcons(pIcons)
  {
    m_pIcons->AddRef();
  }

  ~CNewMenuIconLock()
  {
    m_pIcons->Release();
  }
  operator CNewMenuIcons*(){return m_pIcons;}
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMenuIcons,CObject);

CNewMenuIcons::CNewMenuIcons()
: m_lpszResourceName(NULL),
m_hInst(NULL),
m_hBitmap(NULL),
m_nColors(0),
m_crTransparent(CLR_NONE),
m_dwRefCount(0)
{
}

CNewMenuIcons::~CNewMenuIcons()
{
  if(m_lpszResourceName && !IS_INTRESOURCE(m_lpszResourceName))
  {
    delete (LPTSTR)m_lpszResourceName;
  }
}

int CNewMenuIcons::AddRef()
{
  if(this==NULL)
    return NULL;

  return ++m_dwRefCount;
}

int CNewMenuIcons::Release()
{
  if(this==NULL)
    return NULL;

  DWORD dwCount = --m_dwRefCount;
  if(m_dwRefCount==0)
  {
    if(CNewMenu::m_pSharedMenuIcons)
    {
      POSITION pos = CNewMenu::m_pSharedMenuIcons->Find(this);
      if(pos)
      {
        CNewMenu::m_pSharedMenuIcons->RemoveAt(pos);
      }
    }
    delete this;
  }
  return dwCount;
}

#if defined(_DEBUG) || defined(_AFXDLL)
// Diagnostic Support
void CNewMenuIcons::AssertValid() const
{
  CObject::AssertValid();
}

void CNewMenuIcons::Dump(CDumpContext& dc) const
{
  CObject::Dump(dc);
  dc << _T("NewMenuIcons: ") << _T("\n");
}
#endif

BOOL CNewMenuIcons::DoMatch(LPCTSTR lpszResourceName, HMODULE hInst)
{
  if(hInst==m_hInst && lpszResourceName)
  {
    if(IS_INTRESOURCE(m_lpszResourceName))
    {
      return (lpszResourceName==m_lpszResourceName);
    }

    return (_tcscmp(lpszResourceName,m_lpszResourceName)==0);
  }
  return FALSE;
}

BOOL CNewMenuIcons::DoMatch(HBITMAP hBitmap, CSize size, UINT* pID)
{
  if(pID && m_hBitmap==hBitmap)
  {
    CSize iconSize = GetIconSize();
    if(iconSize==size)
    {
      int nCount = (int)m_IDs.GetSize();
      for(int nIndex=0 ; nIndex<nCount ; nIndex++,pID++)
      {
        if( (*pID)==0 || m_IDs.GetAt(nIndex)!=(*pID) )
        {
          return FALSE;
        }
      }
      return TRUE;
    }
  }
  return FALSE;
}

BOOL CNewMenuIcons::DoMatch(WORD* pIconInfo, COLORREF crTransparent)
{
  if(m_crTransparent==crTransparent && pIconInfo!=NULL)
  {
    CNewMenuIconInfo* pInfo = (CNewMenuIconInfo*)pIconInfo;

    // Check for the same resource ID
    if( pInfo->wBitmapID && IS_INTRESOURCE(m_lpszResourceName) &&
      ((UINT)(UINT_PTR)m_lpszResourceName)==pInfo->wBitmapID)
    {
      int nCount = (int)m_IDs.GetSize();
      WORD* pID = pInfo->ids();
      for(int nIndex=0 ; nIndex<nCount ; nIndex++,pID++)
      {
        if( (*pID)==0 || m_IDs.GetAt(nIndex)!=(*pID) )
        {
          return FALSE;
        }
      }
      return TRUE;
    }
  }
  return FALSE;
}

int CNewMenuIcons::FindIndex(UINT nID)
{
  int nIndex = (int)m_IDs.GetSize();
  while(nIndex--)
  {
    if(m_IDs.GetAt(nIndex)==nID)
    {
      return nIndex*MENU_ICONS;
    }
  }
  return -1;
}

BOOL CNewMenuIcons::GetIconSize(int* cx, int* cy)
{
  return ::ImageList_GetIconSize(m_IconsList,cx,cy);
}

CSize CNewMenuIcons::GetIconSize()
{
  int cx=0;
  int cy=0;
  if(::ImageList_GetIconSize(m_IconsList,&cx,&cy))
  {
    return CSize(cx,cy);
  }
  return CSize(0,0);
}

void CNewMenuIcons::OnSysColorChange()
{
  if(m_lpszResourceName!=NULL)
  {
    int cx=16,cy=16;
    if(GetIconSize(&cx, &cy) && LoadBitmap(cx,cy,m_lpszResourceName,m_hInst))
    {
      MakeImages();
    }
  }
}

BOOL CNewMenuIcons::LoadBitmap(int nWidth, int nHeight, LPCTSTR lpszResourceName, HMODULE hInst)
{
  m_nColors = 0;
  HBITMAP hBitmap = LoadColorBitmap(lpszResourceName,hInst,&m_nColors);
  if(hBitmap!=NULL)
  {
    CBitmap bitmap;
    bitmap.Attach(hBitmap);
    if(m_IconsList.GetSafeHandle())
    {
      m_IconsList.DeleteImageList();
    }
    m_IconsList.Create(nWidth,nHeight,ILC_COLORDDB|ILC_MASK,0,10);
    m_IconsList.Add(&bitmap,m_crTransparent);

    return TRUE;
  }
  return FALSE;
}

BOOL CNewMenuIcons::LoadToolBar(HBITMAP hBitmap, CSize size, UINT* pID, COLORREF crTransparent)
{
  BOOL bResult = FALSE;
  m_nColors = 0;
  if(hBitmap!=NULL)
  {
    BITMAP myInfo = {0};
    if(GetObject(hBitmap,sizeof(myInfo),&myInfo))
    {
      m_crTransparent = crTransparent;
      if(m_IconsList.GetSafeHandle())
      {
        m_IconsList.DeleteImageList();
      }
      m_IconsList.Create(size.cx,size.cy,ILC_COLORDDB|ILC_MASK,0,10);
      // Changed by Mehdy Bohlool(zy) ( December_28_2003 )
      //
      // CImageList::Add function change the background color ( color
      // specified as transparent ) to black, and this bitmap may use
      // after call to this function, It seem that Load functions do
      // not change their source data provider ( currently hBitmap ).
      // Old Code:
      // CBitmap* pBitmap = CBitmap::FromHandle(hBitmap);
      // m_IconsList.Add(pBitmap,m_crTransparent);
      // New Code:
      {
        HBITMAP hBitmapCopy;

        hBitmapCopy = (HBITMAP) CopyImage( hBitmap, IMAGE_BITMAP, 0,0,0);

        CBitmap* pBitmap = CBitmap::FromHandle(hBitmapCopy);
        m_IconsList.Add(pBitmap,m_crTransparent);

        DeleteObject( hBitmapCopy );
      }

      while(*pID)
      {
        UINT nID = *(pID++);
        m_IDs.Add(nID);
        bResult = TRUE;
      }
      MakeImages();
    }
  }
  return bResult;
}


BOOL CNewMenuIcons::LoadToolBar(WORD* pIconInfo, COLORREF crTransparent)
{
  BOOL bResult = FALSE;
  m_crTransparent = crTransparent;
  CNewMenuIconInfo* pInfo = (CNewMenuIconInfo*)pIconInfo;

  if (LoadBitmap(pInfo->wWidth,pInfo->wHeight,MAKEINTRESOURCE(pInfo->wBitmapID)))
  {
    SetResourceName(MAKEINTRESOURCE(pInfo->wBitmapID));

    WORD* pID = pInfo->ids();
    while(*pID)
    {
      UINT nID = *(pID++);
      m_IDs.Add(nID);
      bResult = TRUE;
    }
    MakeImages();
  }
  return bResult;
}

void CNewMenuIcons::SetResourceName(LPCTSTR lpszResourceName)
{
  ASSERT_VALID(this);
  ASSERT(lpszResourceName != NULL);

  if(m_lpszResourceName && !IS_INTRESOURCE(m_lpszResourceName))
  {
    delete [](LPTSTR)m_lpszResourceName;
  }
  if( lpszResourceName && !IS_INTRESOURCE(lpszResourceName))
  {
    size_t bufSizeTchar = _tcslen(lpszResourceName)+1;
    m_lpszResourceName = new TCHAR[bufSizeTchar];
    _tcscpy_s((LPTSTR)m_lpszResourceName,bufSizeTchar,lpszResourceName);
  }
  else
  {
    m_lpszResourceName = lpszResourceName;
  }
}

BOOL CNewMenuIcons::LoadToolBar(LPCTSTR lpszResourceName, HMODULE hInst)
{
  ASSERT_VALID(this);

  SetResourceName(lpszResourceName);

  m_hInst = hInst;

  // determine location of the bitmap in resource
  if(hInst==0)
  {
    hInst = AfxFindResourceHandle(lpszResourceName, RT_TOOLBAR);
  }
  HRSRC hRsrc = ::FindResource(hInst, lpszResourceName, RT_TOOLBAR);

  if (hRsrc == NULL)
  { // Special purpose when you try to load it from a dll 30.05.2002
    if(AfxGetResourceHandle()!=hInst)
    {
      hInst = AfxGetResourceHandle();
      hRsrc = ::FindResource(hInst, lpszResourceName, RT_TOOLBAR);
    }
    if (hRsrc == NULL)
    {
      return FALSE;
    }
  }

  HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
  if (hGlobal == NULL)
  {
    return FALSE;
  }

  CToolBarData* pData = (CToolBarData*)LockResource(hGlobal);
  if (pData == NULL)
  {
    return FALSE;
  }

  BOOL bResult = FALSE;
  ASSERT(pData->wVersion == 1);

  if(LoadBitmap(pData->wWidth,pData->wHeight,lpszResourceName,hInst))
  {
    // Remove all previous ID's
    m_IDs.RemoveAll();
    for (int i = 0; i < pData->wItemCount; i++)
    {
      UINT nID = pData->items()[i];
      if (nID)
      {
        m_IDs.Add(nID);
        bResult = TRUE;
      }
    }
  }

  UnlockResource(hGlobal);
  FreeResource(hGlobal);

  MakeImages();

  return bResult;
}

int CNewMenuIcons::AddGloomIcon(HICON hIcon, int nIndex)
{
  ICONINFO iconInfo = {0};
  if(!GetIconInfo(hIcon,&iconInfo))
  {
    return -1;
  }

  CSize size = GetIconSize();
  CDC myDC;
  myDC.CreateCompatibleDC(0);

  CBitmap bmColor;
  bmColor.Attach(iconInfo.hbmColor);
  CBitmap bmMask;
  bmMask.Attach(iconInfo.hbmMask);

  CBitmap* pOldBitmap = myDC.SelectObject(&bmColor);
  COLORREF crPixel;
  for(int i=0;i<size.cx;++i)
  {
    for(int j=0;j<size.cy;++j)
    {
      crPixel = myDC.GetPixel(i,j);
      // Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
      // added so the gloom value can be adjusted, this was 50
      myDC.SetPixel(i,j,DarkenColor(CNewMenu::GetGloomFactor(), crPixel));
    }
  }
  myDC.SelectObject(pOldBitmap);
  if(nIndex==-1)
  {
    return m_IconsList.Add(&bmColor,&bmMask);
  }

  return (m_IconsList.Replace(nIndex,&bmColor,&bmMask)) ? nIndex: -1;
}

int CNewMenuIcons::AddGrayIcon(HICON hIcon, int nIndex)
{
  ICONINFO iconInfo = {0};
  if(!GetIconInfo(hIcon,&iconInfo))
  {
    return -1;
  }

  CBitmap bmColor;
  bmColor.Attach(iconInfo.hbmColor);
  CBitmap bmMask;
  bmMask.Attach(iconInfo.hbmMask);

  COLORREF blendcolor;
  switch (CNewMenu::GetMenuDrawMode())
  {
  case CNewMenu::STYLE_XP_2003:
  case CNewMenu::STYLE_XP_2003_NOBORDER:
  case CNewMenu::STYLE_COLORFUL_NOBORDER:
  case CNewMenu::STYLE_COLORFUL:
    blendcolor = LightenColor(115,CNewMenu::GetMenuBarColor2003());
    break;

  case CNewMenu::STYLE_XP:
  case CNewMenu::STYLE_XP_NOBORDER:
    blendcolor = LightenColor(115,CNewMenu::GetMenuBarColorXP());
    break;

  default:
    blendcolor = LightenColor(115,CNewMenu::GetMenuBarColor());
    break;
  }
  MakeGrayAlphablend(&bmColor,110, blendcolor);

  if(nIndex==-1)
  {
    return m_IconsList.Add(&bmColor,&bmMask);
  }

  return (m_IconsList.Replace(nIndex,&bmColor,&bmMask)) ? nIndex: -1;
}

BOOL CNewMenuIcons::MakeImages()
{
  int nCount = m_IconsList.GetImageCount();
  if(!nCount)
  {
    return FALSE;
  }

  CSize size = GetIconSize();
  CImageList ilTemp;
  ilTemp.Attach(m_IconsList.Detach());
  m_IconsList.Create(size.cx,size.cy,ILC_COLORDDB|ILC_MASK,0,10);

  for(int nIndex=0;nIndex<nCount;nIndex++)
  {
    HICON hIcon = ilTemp.ExtractIcon(nIndex);
    m_IconsList.Add(hIcon);
    AddGloomIcon(hIcon);
    AddGrayIcon(hIcon);

    DestroyIcon(hIcon);
  }
  return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMenuBitmaps,CNewMenuIcons);

CNewMenuBitmaps::CNewMenuBitmaps()
{
}

CNewMenuBitmaps::~CNewMenuBitmaps()
{
}

int CNewMenuBitmaps::Add(UINT nID, COLORREF crTransparent)
{
  int nIndex = (int)m_IDs.GetSize();
  while(nIndex--)
  {
    if(m_IDs.GetAt(nIndex)==nID)
    {
      return nIndex*MENU_ICONS;
    }
  }
  // Try to load the bitmap for getting dimension
  HBITMAP hBitmap = LoadColorBitmap(MAKEINTRESOURCE(nID),0);
  if(hBitmap!=NULL)
  {
    CBitmap temp;
    temp.Attach(hBitmap);

    BITMAP bitmap = {0};
    if(!temp.GetBitmap(&bitmap))
    {
      return -1;
    }

    if(m_IconsList.GetSafeHandle()==NULL)
    {
      m_IconsList.Create(bitmap.bmWidth,bitmap.bmHeight,ILC_COLORDDB|ILC_MASK,0,10);
    }
    else
    {
      CSize size = GetIconSize();
      // Wrong size?
      if(size.cx!=bitmap.bmWidth || size.cy!=bitmap.bmHeight)
      {
        return -1;
      }
    }
    m_TranspColors.Add(crTransparent);
    m_IDs.Add(nID);

    nIndex = m_IconsList.Add(&temp,crTransparent);
    HICON hIcon = m_IconsList.ExtractIcon(nIndex);
    AddGloomIcon(hIcon);
    AddGrayIcon(hIcon);
    DestroyIcon(hIcon);

    //SetBlendImage();
    return nIndex;
  }
  return -1;
}

void CNewMenuBitmaps::OnSysColorChange()
{
  int nCount = (int)m_IDs.GetSize();
  for(int nIndex=0;nIndex<nCount;nIndex+=MENU_ICONS)
  {
    //Todo reload icons
    HICON hIcon = m_IconsList.ExtractIcon(nIndex);
    AddGloomIcon(hIcon,nIndex+1);
    AddGrayIcon(hIcon,nIndex+2);

    DestroyIcon(hIcon);
  }
}

int CNewMenuBitmaps::Add(HICON hIcon, UINT nID)
{
  ICONINFO iconInfo = {0};
  if(!GetIconInfo(hIcon,&iconInfo))
  {
    return -1;
  }

  CBitmap temp;
  temp.Attach(iconInfo.hbmColor);
  ::DeleteObject(iconInfo.hbmMask);

  BITMAP bitmap = {0};
  if(!temp.GetBitmap(&bitmap))
  {
    return -1;
  }

  if(m_IconsList.GetSafeHandle()==NULL)
  {
    m_IconsList.Create(bitmap.bmWidth,bitmap.bmHeight,ILC_COLORDDB|ILC_MASK,0,10);
  }
  else
  {
    CSize size = GetIconSize();
    // Wrong size?
    if(size.cx!=bitmap.bmWidth || size.cy!=bitmap.bmHeight)
    {
      return -1;
    }
  }
  if(nID)
  {
    int nIndex = (int)m_IDs.GetSize();
    while(nIndex--)
    {
      if(m_IDs.GetAt(nIndex)==nID)
      {
        // We found the index also replace the icon
        nIndex = nIndex*MENU_ICONS;
        m_IconsList.Replace(nIndex,hIcon);
        AddGloomIcon(hIcon,nIndex+1);
        AddGrayIcon(hIcon,nIndex+2);
        return nIndex;
      }
    }
  }
  COLORREF clr = CLR_NONE;
  m_TranspColors.Add(clr);
  m_IDs.Add(nID);
  int nIndex = m_IconsList.Add(hIcon);
  AddGloomIcon(hIcon);
  AddGrayIcon(hIcon);

  return nIndex;
}

int CNewMenuBitmaps::Add(CBitmap* pBitmap, COLORREF crTransparent)
{
  ASSERT(pBitmap);

  BITMAP bitmap = {0};
  if(!pBitmap->GetBitmap(&bitmap))
  {
    return -1;
  }

  if(m_IconsList.GetSafeHandle()==NULL)
  {
    m_IconsList.Create(bitmap.bmWidth,bitmap.bmHeight,ILC_COLORDDB|ILC_MASK,0,10);
  }
  else
  {
    CSize size = GetIconSize();
    // Wrong size?
    if(size.cx!=bitmap.bmWidth || size.cy!=bitmap.bmHeight)
    {
      return -1;
    }
  }
  UINT nID = 0;
  m_TranspColors.Add(crTransparent);
  m_IDs.Add(nID);
  int nIndex = m_IconsList.Add(pBitmap,crTransparent);
  HICON hIcon = m_IconsList.ExtractIcon(nIndex);
  AddGloomIcon(hIcon);
  AddGrayIcon(hIcon);
  DestroyIcon(hIcon);
  //SetBlendImage();
  return nIndex;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMenuItemData,CObject);

CNewMenuItemData::CNewMenuItemData()
: m_nTitleFlags(0),
m_nFlags(0),
m_nID(0),
m_nSyncFlag(0),
m_pData(NULL),
m_pMenuIcon(NULL),
m_nMenuIconOffset(-1)
{
}

CNewMenuItemData::~CNewMenuItemData()
{
  // it's a safe release. Do not care for NULL pointers.
  m_pMenuIcon->Release();
}

// Get the string, along with (manufactured) accelerator text.
CString CNewMenuItemData::GetString (HACCEL hAccel)
{
  if (m_nFlags & MF_POPUP )
  { // No accelerators if we're the title of a popup menu. Otherwise we get spurious accels
    hAccel = NULL;
  }

  CString s = m_szMenuText;
  int iTabIndex = s.Find ('\t');
  if (!hAccel || iTabIndex>= 0) // Got one hard coded in, or we're a popup menu
  {
    if (!hAccel && iTabIndex>= 0)
    {
      s = s.Left (iTabIndex);
    }
    return s;
  }

  // OK, we've got to go hunting through the default accelerator.
  if (hAccel == INVALID_HANDLE_VALUE)
  {
    hAccel = NULL;
    CFrameWnd *pFrame = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd ());
    // No frame. Maybe we're part of a dialog app. etc.
    if (pFrame)
    {
      hAccel = pFrame->GetDefaultAccelerator ();
    }
  }
  // No default found, or we've turned accelerators off.
  if (hAccel == NULL)
  {
    return s;
  }
  // Get the number of entries
  int nEntries = ::CopyAcceleratorTable (hAccel, NULL, 0);
  if (nEntries)
  {
    ACCEL *pAccel = (ACCEL *)_alloca(nEntries*sizeof(ACCEL));
    if (::CopyAcceleratorTable (hAccel, pAccel, nEntries))
    {
      CString sAccel;
      for (int n = 0; n < nEntries; n++)
      {
        if (pAccel [n].cmd != (WORD) m_nID)
        {
          continue;
        }
        if (!sAccel.IsEmpty ())
        {
          sAccel += _T(", ");
        }
        // Translate the accelerator into more useful code.
        if (pAccel [n].fVirt & FALT)         sAccel += _T("Alt+");
        if (pAccel [n].fVirt & FCONTROL)     sAccel += _T("Ctrl+");
        if (pAccel [n].fVirt & FSHIFT)       sAccel += _T("Shift+");
        if (pAccel [n].fVirt & FVIRTKEY)
        {
          TCHAR keyname[64];
          UINT vkey = MapVirtualKey(pAccel [n].key, 0)<<16;
          GetKeyNameText(vkey, keyname, sizeof(keyname));
          sAccel += keyname;
        }
        else
        {
          sAccel += (TCHAR)pAccel [n].key;
        }
      }
      if (!sAccel.IsEmpty ()) // We found one!
      {
        s += '\t';
        s += sAccel;
      }
    }
  }
  return s;
}

void CNewMenuItemData::SetString(LPCTSTR szMenuText)
{
  m_szMenuText = szMenuText;
}

#if defined(_DEBUG) || defined(_AFXDLL)
// Diagnostic Support
void CNewMenuItemData::AssertValid() const
{
  CObject::AssertValid();
}

void CNewMenuItemData::Dump(CDumpContext& dc) const
{
  CObject::Dump(dc);
  dc << _T("MenuItem: ") << m_szMenuText << _T("\n");
}

#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMenuItemDataTitle,CNewMenuItemData);

CNewMenuItemDataTitle::CNewMenuItemDataTitle()
: m_clrTitle(CLR_DEFAULT),
m_clrLeft(CLR_DEFAULT),
m_clrRight(CLR_DEFAULT)
{
}

CNewMenuItemDataTitle::~CNewMenuItemDataTitle()
{
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMenu,CMenu);

// actual selectet menu-draw mode
CMenuTheme* CNewMenu::m_pActMenuDrawing = NULL;
CTypedPtrList<CPtrList, CNewMenuIcons*>* CNewMenu::m_pSharedMenuIcons = NULL;

// Gloabal logfont for all menutitles
LOGFONT CNewMenu::m_MenuTitleFont = {16, 0, 0, 0, FW_BOLD,
0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
DEFAULT_PITCH,_T("Arial")};


void CNewMenu::SetMenuTitleFont(CFont* pFont)
{
  ASSERT(pFont);
  pFont->GetLogFont(&m_MenuTitleFont);
}

void CNewMenu::SetMenuTitleFont(LOGFONT* pLogFont)
{
  ASSERT(pLogFont);
  m_MenuTitleFont = *pLogFont;
}

LOGFONT CNewMenu::GetMenuTitleFont()
{
  return m_MenuTitleFont;
}

DWORD CNewMenu::m_dwLastActiveItem = NULL;

// how the menu's are drawn in winXP
BOOL CNewMenu::m_bEnableXpBlending = TRUE;
BOOL CNewMenu::m_bNewMenuBorderAllMenu = TRUE;
BOOL CNewMenu::m_bSelectDisable = TRUE;
// Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// added so the gloom value can be adjusted
// 50 is the default, may be too high, 20 is subtle
int CNewMenu::m_nGloomFactor = 50;

const Win32Type g_Shell = IsShellType();
BOOL bRemoteSession = FALSE;

// one instance of the hook for menu-subclassing
static CNewMenuHook MyNewMenuHookInstance;

CNewMenu::CNewMenu(HMENU hParent)
: m_hTempOwner(NULL),
m_hParentMenu(hParent),
m_bIsPopupMenu(TRUE),
m_dwOpenMenu(NULL),
m_LastActiveMenuRect(0,0,0,0),
m_pData(NULL),
m_hAccelToDraw((HACCEL)INVALID_HANDLE_VALUE)
{
  // O.S. - no dynamic icons by default
  m_bDynIcons = FALSE;
  // Icon sizes default to 16 x 16
  m_iconX = 16;
  m_iconY = 15;
  m_selectcheck = -1;
  m_unselectcheck = -1;
  m_checkmaps=NULL;
  m_checkmapsshare=FALSE;

  // set the color used for the transparent background in all bitmaps
  m_bitmapBackground = CLR_DEFAULT;
}

CNewMenu::~CNewMenu()
{
  DestroyMenu();
}

COLORREF CNewMenu::GetMenuColor(HMENU hMenu)
{
  if(hMenu!=NULL)
  {
    MENUINFO menuInfo={0};
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIM_BACKGROUND;

    if(::GetMenuInfo(hMenu,&menuInfo) && menuInfo.hbrBack)
    {
      LOGBRUSH logBrush;
      if(GetObject(menuInfo.hbrBack,sizeof(LOGBRUSH),&logBrush))
      {
        return logBrush.lbColor;
      }
    }
  }

  if(IsShellType()==WinXP)
  {
    BOOL bFlatMenu = FALSE;
    // theme ist not checket, that must be so
    if( (SystemParametersInfo(SPI_GETFLATMENU,0,&bFlatMenu,0) && bFlatMenu==TRUE) )
    {
      return GetSysColor(COLOR_MENUBAR);
    }
  }
  return GetSysColor(COLOR_MENU);
}

COLORREF CNewMenu::GetMenuBarColorXP()
{
  // Win95 or WinNT do not support to change the menubarcolor
  if(IsShellType()==Win95 || IsShellType()==WinNT4)
  {
    return GetSysColor(COLOR_MENU);
  }
  return GetSysColor(COLOR_3DFACE);
}


HRESULT localGetCurrentThemeName( LPWSTR pszThemeFileName, int dwMaxNameChars,
                                  LPWSTR pszColorBuff,     int cchMaxColorChars,
                                  LPWSTR pszSizeBuff,      int cchMaxSizeChars)
{
  static CNewLoadLib menuInfo(_T("UxTheme.dll"),"GetCurrentThemeName");
  if(menuInfo.m_pProg)
  {
   typedef HRESULT (WINAPI* FktGetCurrentThemeName)(LPWSTR pszThemeFileName,int dwMaxNameChars, LPWSTR pszColorBuff, int cchMaxColorChars, LPWSTR pszSizeBuff, int cchMaxSizeChars);
    return ((FktGetCurrentThemeName)menuInfo.m_pProg)(pszThemeFileName, dwMaxNameChars, pszColorBuff, cchMaxColorChars, pszSizeBuff, cchMaxSizeChars);
  }
  return NULL;
}

HANDLE localGetWindowTheme(HWND hWnd)
{
  static CNewLoadLib menuInfo(_T("UxTheme.dll"),"GetWindowTheme");
  if(menuInfo.m_pProg)
  {
    typedef HANDLE (WINAPI* FktGetWindowTheme)(HWND hWnd);
    return ((FktGetWindowTheme)menuInfo.m_pProg)(hWnd);
  }
  return NULL;
}

HANDLE localOpenThemeData( HWND hWnd, LPCWSTR pszClassList)
{
  static CNewLoadLib menuInfo(_T("UxTheme.dll"),"OpenThemeData");
  if(menuInfo.m_pProg)
  {
    typedef HANDLE (WINAPI* FktOpenThemeData)(HWND hWnd, LPCWSTR pszClassList);
    return ((FktOpenThemeData)menuInfo.m_pProg)(hWnd,pszClassList);
  }
  return NULL;
}

HRESULT localCloseThemeData(HANDLE hTheme)
{
  static CNewLoadLib menuInfo(_T("UxTheme.dll"),"CloseThemeData");
  if(menuInfo.m_pProg)
  {
    typedef HRESULT (WINAPI* FktCloseThemeData)(HANDLE hTheme);
    return ((FktCloseThemeData)menuInfo.m_pProg)(hTheme);
  }
  return NULL;
}

HRESULT localGetThemeColor(HANDLE hTheme,
    int iPartId,
    int iStateId,
    int iColorId,
    COLORREF *pColor)
{
  static CNewLoadLib menuInfo(_T("UxTheme.dll"),"GetThemeColor");
  if(menuInfo.m_pProg)
  {
    typedef HRESULT (WINAPI* FktGetThemeColor)(HANDLE hTheme,int iPartId,int iStateId,int iColorId, COLORREF *pColor);
    return ((FktGetThemeColor)menuInfo.m_pProg)(hTheme, iPartId, iStateId, iColorId, pColor);
  }
  return S_FALSE;
}



COLORREF CNewMenu::GetMenuBarColor2003()
{
  COLORREF colorLeft,colorRight;
  GetMenuBarColor2003(colorLeft,colorRight);
  return colorLeft;
}

void CNewMenu::GetMenuBarColor2003(COLORREF& color1, COLORREF& color2, BOOL bBackgroundColor /* = TRUE */)
{
  // Win95 or WinNT do not support to change the menubarcolor
  if(IsShellType()==Win95 || IsShellType()==WinNT4)
  {
    color1 = color2 = GetSysColor(COLOR_MENU);
    return;
  }

  if(GetMenuDrawMode()==STYLE_COLORFUL_NOBORDER ||
     GetMenuDrawMode()==STYLE_COLORFUL)
  {
    COLORREF colorWindow = DarkenColor(10,GetSysColor(COLOR_WINDOW));
    COLORREF colorCaption = GetSysColor(COLOR_ACTIVECAPTION);

    CClientDC myDC(NULL);
    COLORREF nearColor = myDC.GetNearestColor(MidColor(colorWindow,colorCaption));

    // some colorscheme corrections (Andreas Schärer)
    if (nearColor == 15779244) //standartblau
    { //entspricht (haar-)genau office 2003
      nearColor = RGB(163,194,245);
    }
    else if (nearColor == 15132390) //standartsilber
    {
      nearColor = RGB(215,215,229);
    }
    else if (nearColor == 13425878) //olivgrün
    {
      nearColor = RGB(218,218,170);
    }

    color1 = nearColor;
    color2 = LightenColor(110,color1);
    return;
  }
  else
  {
    if (IsMenuThemeActive())
    {
      COLORREF colorWindow = DarkenColor(10,GetSysColor(COLOR_WINDOW));
      COLORREF colorCaption = GetSysColor(COLOR_ACTIVECAPTION);

      //CWnd* pWnd = AfxGetMainWnd();
      //if (pWnd)
      {
        //HANDLE hTheme = localOpenThemeData(pWnd->GetSafeHwnd(),L"Window");
        // get the them from desktop window
        HANDLE hTheme = localOpenThemeData(NULL,L"Window");
        if(hTheme)
        {
          COLORREF color = CLR_NONE;
          // defined in Tmschema.h
          //    TM_PART(1, WP, CAPTION)
          //    TM_STATE(1, CS, ACTIVE)
          //    TM_PROP(204, TMT, COLOR,     COLOR)
          //for (int iColorID=0;iColorID<6000;iColorID++)
          //{
          //  if(localGetThemeColor(hTheme,1,1,iColorID,&color)==S_OK)
          //  {
          //    colorCaption = color;
          //  }
          //}

          // TM_PROP(3804, TMT, EDGELIGHTCOLOR,     COLOR)     // edge color
          // TM_PROP(3821, TMT, FILLCOLORHINT, COLOR)          // hint about fill color used (for custom controls)
          if(localGetThemeColor(hTheme,1,1,3821,&color)==S_OK && (color!=1))
          {
            colorCaption = color;
          }
          //TM_PROP(3805, TMT, EDGEHIGHLIGHTCOLOR, COLOR)     // edge color
          if(localGetThemeColor(hTheme,1,1,3805,&color)==S_OK && (color!=1))
          {
            colorWindow = color;
          }
          localCloseThemeData(hTheme);

          // left side Menubar
          switch(colorCaption)
          {
          case 0x00e55400: // blue
            color1 = RGB(163,194,245);
            break;

          case 0x00bea3a4:  // silver
            color1 = RGB(215,215,229);
            break;

          case 0x0086b8aa:  //olive green
            color1 = RGB(218,218,170);
            break;

          default:
            {
              CClientDC myDC(NULL);
              color1 = myDC.GetNearestColor(MidColor(colorWindow,colorCaption));
            }
            break;
          }

          if (bBackgroundColor)
          {
            color2 = LightenColor(110,color1);
          }
          else
          {
            color2 = LightenColor(200,color1);
            color1 = DarkenColor(20,color1);
          }
          return;
        }
      }

      CClientDC myDC(NULL);
      COLORREF nearColor = myDC.GetNearestColor(MidColor(colorWindow,colorCaption));

      // some colorscheme corrections (Andreas Schärer)
      if (nearColor == 15779244) //standartblau
      { //entspricht (haar-)genau office 2003
        color1 = RGB(163,194,245);
      }
      else if (nearColor == 15132390) //standartsilber
      {
        color1 = RGB(215,215,229);
      }
      else if (nearColor == 13425878) //olivgrün
      {
        color1 = RGB(218,218,170);
      }
      else
      {
        color1 = nearColor;
      }

      if (bBackgroundColor)
      {
        color2 = LightenColor(100,color1);
      }
      else
      {
        color2 = LightenColor(200,color1);
        color1 = DarkenColor(20,color1);
      }
    }
    else
    {
      //color1 = ::GetSysColor(COLOR_MENU);
      color1 = ::GetSysColor(COLOR_BTNFACE); // same as COLOR_3DFACE
      color2 = ::GetSysColor(COLOR_WINDOW);
      color2 =  GetAlphaBlendColor(color1,color2,220);
    }
  }
}

COLORREF CNewMenu::GetMenuBarColor(HMENU hMenu)
{
  if(hMenu!=NULL)
  {
    MENUINFO menuInfo = {0};
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIM_BACKGROUND;

    if(::GetMenuInfo(hMenu,&menuInfo) && menuInfo.hbrBack)
    {
      LOGBRUSH logBrush;
      if(GetObject(menuInfo.hbrBack,sizeof(LOGBRUSH),&logBrush))
      {
        return logBrush.lbColor;
      }
    }
  }
  if(IsShellType()==WinXP)
  {
    BOOL bFlatMenu = FALSE;
    if((SystemParametersInfo(SPI_GETFLATMENU,0,&bFlatMenu,0) && bFlatMenu==TRUE) ||
      (IsMenuThemeActive()))
    {
      return GetSysColor(COLOR_MENUBAR);
    }
  }
  // May-05-2005 - Mark P. Peterson (mpp@rhinosoft.com) - Changed to use the menubar color, I could not find why COLOR_MENU should
  // be used when not flat and no XP Theme is active.  The problem is that using this shows the wrong color in any other theme, when
  // the background color of menus is something other than the same color of the menu bar.
  return (GetMenuBarColorXP());
//  return GetSysColor(COLOR_MENU);
}

void CNewMenu::SetLastMenuRect(HDC hDC, LPRECT pRect)
{
  if(!m_bIsPopupMenu)
  {
    HWND hWnd = WindowFromDC(hDC);
    if(hWnd && pRect)
    {
      CRect Temp;
      GetWindowRect(hWnd,Temp);
      m_LastActiveMenuRect = *pRect;
      m_LastActiveMenuRect.OffsetRect(Temp.TopLeft());
#ifdef _TRACE_MENU_
      AfxTrace(_T("ActiveRect: (%ld,%ld,%ld,%ld)\n"),m_LastActiveMenuRect.left,m_LastActiveMenuRect.top,m_LastActiveMenuRect.right,m_LastActiveMenuRect.bottom);
#endif
    }
  }
}

void CNewMenu::SetLastMenuRect(LPRECT pRect)
{
  ASSERT(pRect);
  m_LastActiveMenuRect = *pRect;
}

BOOL CNewMenu::IsNewShell()
{
  return (g_Shell>=Win95);
}

BOOL CNewMenu::OnMeasureItem(const MSG* pMsg)
{
  if(pMsg->message==WM_MEASUREITEM)
  {
    LPMEASUREITEMSTRUCT lpMIS = (LPMEASUREITEMSTRUCT)pMsg->lParam;
    if(lpMIS->CtlType==ODT_MENU)
    {
      CMenu* pMenu=NULL;
      if(::IsMenu(UIntToHMenu(lpMIS->itemID)) )
      {
        pMenu = CMenu::FromHandlePermanent(UIntToHMenu(lpMIS->itemID) );
      }
      else
      {
        _AFX_THREAD_STATE* pThreadState = AfxGetThreadState ();
        if (pThreadState->m_hTrackingWindow == pMsg->hwnd)
        {
          // start from popup
          pMenu = FindPopupMenuFromIDData(pThreadState->m_hTrackingMenu,lpMIS->itemID,lpMIS->itemData);
        }
        if(pMenu==NULL)
        {
          // start from menubar
          pMenu = FindPopupMenuFromIDData(::GetMenu(pMsg->hwnd),lpMIS->itemID,lpMIS->itemData);
          if(pMenu==NULL)
          {
            // finaly start from system menu.
            pMenu = FindPopupMenuFromIDData(::GetSystemMenu(pMsg->hwnd,FALSE),lpMIS->itemID,lpMIS->itemData);

#ifdef USE_NEW_MENU_BAR
            if(!pMenu)
            { // Support for new menubar
              static UINT WM_GETMENU = ::RegisterWindowMessage(_T("CNewMenuBar::WM_GETMENU"));
              pMenu = FindPopupMenuFromIDData((HMENU)::SendMessage(pMsg->hwnd,WM_GETMENU,0,0),lpMIS->itemID,lpMIS->itemData);
            }
#endif
          }
        }
      }
      if(pMenu!=NULL)
      {
#ifdef _TRACE_MENU_
        UINT oldWidth = lpMIS->itemWidth;
#endif //_TRACE_MENU_

        pMenu->MeasureItem(lpMIS);

#ifdef _TRACE_MENU_
        AfxTrace(_T("NewMenu MeasureItem: ID:0x08%X, oW:0x%X, W:0x%X, H:0x%X\n"),lpMIS->itemID,oldWidth,lpMIS->itemWidth,lpMIS->itemHeight);
#endif //_TRACE_MENU_
        return TRUE;
      }
    }
  }
  return FALSE;
}

CMenu* CNewMenu::FindPopupMenuFromID(HMENU hMenu, UINT nID)
{
  // check for a valid menu-handle
  if ( ::IsMenu(hMenu))
  {
    CMenu *pMenu = CMenu::FromHandlePermanent(hMenu);
    if(pMenu)
    {
      return FindPopupMenuFromID(pMenu,nID);
    }
  }
  return NULL;
}

CMenu* CNewMenu::FindPopupMenuFromIDData(HMENU hMenu, UINT nID, ULONG_PTR pData)
{
  // check for a valid menu-handle
  if ( ::IsMenu(hMenu))
  {
    CMenu *pMenu = CMenu::FromHandlePermanent(hMenu);
    if(pMenu)
    {
      return FindPopupMenuFromIDData(pMenu,nID,pData);
    }
  }
  return NULL;
}

CMenu* CNewMenu::FindPopupMenuFromIDData(CMenu* pMenu, UINT nID, ULONG_PTR pData)
{
  if(!pMenu || !IsMenu(pMenu->m_hMenu))
  {
    return NULL;
  }
  ASSERT_VALID(pMenu);
  // walk through all items, looking for ID match
  UINT nItems = pMenu->GetMenuItemCount();
  for (int iItem = 0; iItem < (int)nItems; iItem++)
  {
    CMenu* pPopup = pMenu->GetSubMenu(iItem);
    if (pPopup!=NULL)
    {
      // recurse to child popup
      pPopup = FindPopupMenuFromIDData(pPopup, nID, pData);
      // check popups on this popup
      if (pPopup != NULL)
      {
        return pPopup;
      }
    }
    else if (pMenu->GetMenuItemID(iItem) == nID)
    {
      MENUITEMINFO MenuItemInfo = {0};
      MenuItemInfo.cbSize = sizeof(MenuItemInfo);
      MenuItemInfo.fMask = MIIM_DATA;

      if(pMenu->GetMenuItemInfo(iItem,&MenuItemInfo,TRUE))
      {
        if(MenuItemInfo.dwItemData==pData)
        {
          // it is a normal item inside our popup
          return pMenu;
        }
      }
    }
  }
  // not found
  return NULL;
}

CMenu* CNewMenu::FindPopupMenuFromID(CMenu* pMenu, UINT nID)
{
  if(!pMenu || !IsMenu(pMenu->m_hMenu))
  {
    return NULL;
  }
  ASSERT_VALID(pMenu);
  // walk through all items, looking for ID match
  UINT nItems = pMenu->GetMenuItemCount();
  for (int iItem = 0; iItem < (int)nItems; iItem++)
  {
    CMenu* pPopup = pMenu->GetSubMenu(iItem);
    if (pPopup != NULL)
    {
      // recurse to child popup
      pPopup = FindPopupMenuFromID(pPopup, nID);
      // check popups on this popup
      if (pPopup != NULL)
      {
        return pPopup;
      }
    }
    else if (pMenu->GetMenuItemID(iItem) == nID)
    {
      // it is a normal item inside our popup
      return pMenu;
    }
  }
  // not found
  return NULL;
}

BOOL CNewMenu::DestroyMenu()
{
  // Destroy Sub menus:
  int nIndex = (int)m_SubMenus.GetSize();
  while(nIndex--)
  {
    // Destroy only if we createt it!!!!!
    CNewMenu* pMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(m_SubMenus[nIndex]));
    if(pMenu)
    {
      delete pMenu;
    }
  }
  m_SubMenus.RemoveAll();

  // Destroy menu data
  nIndex = (int)m_MenuItemList.GetSize();
  while(nIndex--)
  {
    delete(m_MenuItemList[nIndex]);
  }
  m_MenuItemList.RemoveAll();

  if(m_checkmaps&&!m_checkmapsshare)
  {
    delete m_checkmaps;
    m_checkmaps=NULL;
  }
  // Call base-class implementation last:
  return(CMenu::DestroyMenu());
};

UINT CNewMenu::GetMenuDrawMode()
{
  ASSERT(m_pActMenuDrawing);
  return m_pActMenuDrawing->m_dwThemeId;
}

UINT CNewMenu::SetMenuDrawMode(UINT mode)
{
#ifdef _TRACE_MENU_
  if(mode&1)
  {
    AfxTrace(_T("\nDraw menu no border\n"));
  }
  else
  {
    AfxTrace(_T("\nDraw menu with border\n"));
  }
#endif //  _TRACE_MENU_

  // under wine we disable flat menu and shade drawing
  if(bWine)
  {
    switch(mode)
    {
    case STYLE_ORIGINAL:
      mode = STYLE_ORIGINAL_NOBORDER;
      break;

    case STYLE_XP:
      mode = STYLE_XP_NOBORDER;
      break;

    case STYLE_SPECIAL:
      mode = STYLE_SPECIAL_NOBORDER;
      break;

    case STYLE_ICY:
      mode = STYLE_ICY_NOBORDER;
      break;

    case STYLE_XP_2003:
      mode = STYLE_XP_2003_NOBORDER;
      break;

    case STYLE_COLORFUL:
      mode = STYLE_COLORFUL_NOBORDER;
      break;
    }
  }

  UINT nOldMode = (UINT)STYLE_UNDEFINED;
  CMenuTheme* pTheme = CNewMenuHook::FindTheme(mode);
  if(pTheme)
  {
    if(m_pActMenuDrawing)
    {
      nOldMode = m_pActMenuDrawing->m_dwThemeId;
    }

    m_pActMenuDrawing = pTheme;
  }
  return nOldMode;
}

HMENU CNewMenu::GetParent()
{
  return m_hParentMenu;
}

BOOL CNewMenu::IsPopup()
{
  return m_bIsPopupMenu;
}

BOOL CNewMenu::SetPopup(BOOL bIsPopup)
{
  BOOL bOldFlag = m_bIsPopupMenu;
  m_bIsPopupMenu = bIsPopup;
  return bOldFlag;
}

BOOL CNewMenu::SetSelectDisableMode(BOOL mode)
{
  BOOL bOldMode = m_bSelectDisable;
  m_bSelectDisable=mode;
  return bOldMode;
}

BOOL CNewMenu::GetSelectDisableMode()
{
  return m_bSelectDisable;
}

BOOL CNewMenu::SetXpBlending(BOOL bEnable)
{
  BOOL bOldMode = m_bEnableXpBlending;
  m_bEnableXpBlending = bEnable;
  return bOldMode;
}

BOOL CNewMenu::GetXpBlending()
{
  return m_bEnableXpBlending;
}


// Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// added SetGloomFactor() and GetGloomFactor() so that the glooming can be done in a more or less subtle way
int CNewMenu::SetGloomFactor(int nGloomFactor)
{
  int nOldGloomFactor = m_nGloomFactor;

  // set the new gloom factor
  m_nGloomFactor = nGloomFactor;

  // return the previous gloom factor
  return (nOldGloomFactor);
} // SetGloomFactor


int CNewMenu::GetGloomFactor()
{
  // return the current gloom factor
  return (m_nGloomFactor);
} // GetGloomFactor


// Function to set how default menu border were drawn
//(enable=TRUE means that all menu in the application has the same border)
BOOL CNewMenu::SetNewMenuBorderAllMenu(BOOL bEnable /* =TRUE*/)
{
  BOOL bOldMode = m_bNewMenuBorderAllMenu;
  m_bNewMenuBorderAllMenu = bEnable;
  return bOldMode;
}

BOOL CNewMenu::GetNewMenuBorderAllMenu()
{
  return m_bNewMenuBorderAllMenu;
}

void CNewMenu::OnSysColorChange()
{
  static DWORD dwLastTicks = 0;
  DWORD dwAktTicks = GetTickCount();

  // Last Update 2 sec
  if((dwAktTicks-dwLastTicks)>2000)
  {
    dwLastTicks = dwAktTicks;
    if(m_pSharedMenuIcons)
    {
      POSITION pos = m_pSharedMenuIcons->GetHeadPosition();
      while(pos)
      {
        CNewMenuIcons* pMenuIcons = m_pSharedMenuIcons->GetNext(pos);
        pMenuIcons->OnSysColorChange();
      }
    }
  }
}

void CNewMenu::MeasureItem( LPMEASUREITEMSTRUCT lpMIS )
{
  ASSERT(m_pActMenuDrawing);

  BOOL bIsMenuBar = IsMenuBar(UIntToHMenu(lpMIS->itemID));
  if(!bIsMenuBar && m_hParentMenu && !FindMenuItem(lpMIS->itemID)) //::IsMenu(UIntToHMenu(lpMIS->itemID)) )
  {
    CNewMenu* pMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(m_hParentMenu));
    if(pMenu)
    {
      ((*pMenu).*m_pActMenuDrawing->m_pMeasureItem)(lpMIS,bIsMenuBar);
      return;
    }
  }
  ((*this).*m_pActMenuDrawing->m_pMeasureItem)(lpMIS,bIsMenuBar);
}

void CNewMenu::DrawItem (LPDRAWITEMSTRUCT lpDIS)
{
  ASSERT(m_pActMenuDrawing);

  BOOL bIsMenuBar = m_hParentMenu ? FALSE: ((m_bIsPopupMenu)?FALSE:TRUE);

  if(bIsMenuBar && m_dwLastActiveItem==lpDIS->itemData)
  {
    if(! (lpDIS->itemState&ODS_HOTLIGHT) )
    {
      // Mark for redraw helper for win 98
      m_dwLastActiveItem = NULL;
    }
  }

  (this->*m_pActMenuDrawing->m_pDrawItem)(lpDIS,bIsMenuBar);
}

// Erase the Background of the menu
BOOL CNewMenu::EraseBkgnd(HWND hWnd, HDC hDC)
{
  CDC* pDC = CDC::FromHandle (hDC);
  CRect Rect;
  //  Get the size of the menu...
  GetClientRect(hWnd, Rect );

  pDC->FillSolidRect (Rect,GetMenuColor());

  return TRUE;
}

void CNewMenu::DrawTitle(LPDRAWITEMSTRUCT lpDIS,BOOL bIsMenuBar)
{
  ASSERT(m_pActMenuDrawing);
  (this->*m_pActMenuDrawing->m_pDrawTitle)(lpDIS,bIsMenuBar);
}

void CNewMenu::DrawMenuTitle(LPDRAWITEMSTRUCT lpDIS, BOOL bIsMenuBar)
{
  UNREFERENCED_PARAMETER(bIsMenuBar);

  CDC* pDC = CDC::FromHandle(lpDIS->hDC);

  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpDIS->itemData);
  ASSERT(pMenuData);

  COLORREF colorWindow = GetSysColor(COLOR_WINDOW);
  COLORREF colorMenuBar = GetMenuColor();

  COLORREF colorLeft = MixedColor(colorWindow,colorMenuBar);
  COLORREF colorRight = ::GetSysColor(COLOR_ACTIVECAPTION);
  COLORREF colorText = ::GetSysColor(COLOR_CAPTIONTEXT);

  CSize iconSize(0,0);
  if(pMenuData->m_nMenuIconOffset!=(-1) && pMenuData->m_pMenuIcon)
  {
    iconSize = pMenuData->m_pMenuIcon->GetIconSize();
    if(iconSize!=CSize(0,0))
    {
      iconSize += CSize(4,4);
    }
  }

  CNewMenuItemDataTitle* pItem = DYNAMIC_DOWNCAST(CNewMenuItemDataTitle,pMenuData);
  if(pItem)
  {
    if(pItem->m_clrRight!=CLR_DEFAULT)
    {
      colorRight = pItem->m_clrRight;
    }
    if(pItem->m_clrLeft!=CLR_DEFAULT)
    {
      colorLeft = pItem->m_clrLeft;
    }
    if(pItem->m_clrTitle!=CLR_DEFAULT)
    {
      colorText = pItem->m_clrTitle;
    }
  }

  CRect rcClipBox;

  HWND hWnd = ::WindowFromDC(lpDIS->hDC);
  // try to get the real size of the client window
  if(hWnd==NULL || !GetClientRect(hWnd,rcClipBox) )
  {
    // when we have menu animation the DC is a memory DC
    pDC->GetClipBox(rcClipBox);
  }

  // draw the title bar
  CRect rect = lpDIS->rcItem;
  CPoint TextPoint;

  CFont Font;
  LOGFONT MyFont = m_MenuTitleFont;
  if(pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
  {
    rect.top = rcClipBox.top;
    rect.bottom = rcClipBox.bottom;
    rect.right += GetSystemMetrics(SM_CXMENUCHECK);
    MyFont.lfOrientation = 900;
    MyFont.lfEscapement = 900;
    TextPoint = CPoint(rect.left+2, rect.bottom-4-iconSize.cy);
  }
  else
  {
    MyFont.lfOrientation = 0;
    MyFont.lfEscapement = 0;

    TextPoint = CPoint(rect.left+2+iconSize.cx, rect.top);
  }
  Font.CreateFontIndirect(&MyFont);
  CFont *pOldFont = pDC->SelectObject(&Font);
  SIZE size = {0,0};
  VERIFY(::GetTextExtentPoint32(pDC->m_hDC,pMenuData->m_szMenuText,pMenuData->m_szMenuText.GetLength(),&size));
  COLORREF oldColor = pDC->SetTextColor(colorText);
  int OldMode = pDC->SetBkMode(TRANSPARENT);

  if(pMenuData->m_nTitleFlags&MFT_GRADIENT)
  {
    if(pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
    {
      DrawGradient(pDC,rect,colorLeft,colorRight,false);
    }
    else
    {
      DrawGradient(pDC,rect,colorRight,colorLeft,true);
    }
  }
  else
  {
    if(pMenuData->m_nTitleFlags&MFT_ROUND)
    {
      if(pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
      {
        TextPoint.y-=2;
        rect.right = rect.left+size.cy+4;
      }
      else
      {
        int maxSpace = ((rect.Width()-size.cx)/2);
        TextPoint.x+=min(maxSpace,10);
      }

      CBrush brush(colorRight);
      CPen* pOldPen = (CPen*)pDC->SelectStockObject(WHITE_PEN);
      CBrush* pOldBrush = pDC->SelectObject(&brush);

      pDC->RoundRect(rect,CPoint(10,10));
      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
    }
    else
    {
      pDC->FillSolidRect(rect,colorRight);
    }
  }
  if (pMenuData->m_nTitleFlags&MFT_SUNKEN)
  {
    pDC->Draw3dRect(rect,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));
  }

  if (pMenuData->m_nTitleFlags&MFT_CENTER)
  {
    if (pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
    {
      TextPoint.y = rect.bottom - ((rect.Height()-size.cx-iconSize.cy)>>1)-iconSize.cy;
    }
    else
    {
      TextPoint.x = rect.left + ((rect.Width()-size.cx-iconSize.cx)>>1)+iconSize.cx;
    }
  }

  pDC->TextOut(TextPoint.x,TextPoint.y, pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL));

  if(pMenuData->m_nMenuIconOffset!=(-1) && pMenuData->m_pMenuIcon)
  {
    CPoint ptImage = TextPoint;
    if (pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
    {
      ptImage.y += 2;
    }
    else
    {
      ptImage.x -= iconSize.cx;
      ptImage.y += 2;
    }
    // draws the icon
    HICON hDrawIcon2 = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset);
    pDC->DrawState(ptImage, iconSize-CSize(4,4), hDrawIcon2, DSS_NORMAL,(HBRUSH)NULL);
    DestroyIcon(hDrawIcon2);
  }

  if(pMenuData->m_nTitleFlags&MFT_LINE)
  {
    if(pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
    {
      CRect rect2(rect.left+20,rect.top+5,rect.left+22,rect.bottom-5);
      pDC->Draw3dRect(rect2,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));
      rect2.OffsetRect(3,0);
      rect2.InflateRect(0,-10);
      pDC->Draw3dRect(rect2,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));
    }
    else
    {
      CRect rect2(rect.left+2,rect.bottom-7,rect.right-2,rect.bottom-5);
      pDC->Draw3dRect(rect2,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DSHADOW));
      rect2.OffsetRect(0,3);
      rect2.InflateRect(-10,0);
      pDC->Draw3dRect(rect2,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));
    }
  }
  pDC->SetBkMode(OldMode);
  pDC->SetTextColor(oldColor);
  pDC->SelectObject(pOldFont);
}

void CNewMenu::DrawItem_WinXP(LPDRAWITEMSTRUCT lpDIS, BOOL bIsMenuBar)
{
  ASSERT(lpDIS != NULL);

  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpDIS->itemData);
  ASSERT(pMenuData);

  UINT nFlags = pMenuData->m_nFlags;

  CNewMemDC memDC(&lpDIS->rcItem,lpDIS->hDC);
  CDC* pDC;
  if( bIsMenuBar || (nFlags&MF_SEPARATOR) )
  { // For title and menubardrawing disable memory painting
    memDC.DoCancel();
    pDC = CDC::FromHandle(lpDIS->hDC);
  }
  else
  {
    pDC = &memDC;
  }

  COLORREF colorWindow = GetSysColor(COLOR_WINDOW);
  //  COLORREF colorMenuBar = bIsMenuBar?GetMenuBarColor(m_hMenu):GetMenuColor();
  COLORREF colorMenuBar = GetMenuBarColor(m_hMenu);
  COLORREF colorMenu = MixedColor(colorWindow,colorMenuBar);
  COLORREF colorBitmap = MixedColor(GetMenuBarColor(m_hMenu),colorWindow);
  COLORREF colorSel = GetXpHighlightColor();
  COLORREF colorBorder = GetSysColor(COLOR_HIGHLIGHT);//DarkenColor(128,colorMenuBar);

  if(bHighContrast)
  {
    colorBorder = GetSysColor(COLOR_BTNTEXT);
  }

  if (NumScreenColors() <= 256)
  {
    colorBitmap = GetSysColor(COLOR_BTNFACE);
  }

  // Better contrast when you have less than 256 colors
  if(pDC->GetNearestColor(colorMenu)==pDC->GetNearestColor(colorBitmap))
  {
    colorMenu = colorWindow;
    colorBitmap = colorMenuBar;
  }

  //CPen Pen(PS_SOLID,0,GetSysColor(COLOR_HIGHLIGHT));
  CPen Pen(PS_SOLID,0,colorBorder);

  if(bIsMenuBar)
  {
#ifdef _TRACE_MENU_
    //   AfxTrace(_T("BarState: 0x%lX Menus %ld\n"),lpDIS->itemState,m_dwOpenMenu);
#endif
    if(!IsMenu(UIntToHMenu(lpDIS->itemID)) && (lpDIS->itemState&ODS_SELECTED_OPEN))
    {
      lpDIS->itemState = (lpDIS->itemState&~(ODS_SELECTED|ODS_SELECTED_OPEN))|ODS_HOTLIGHT;
    }
    else if(!(lpDIS->itemState&ODS_SELECTED_OPEN) && !m_dwOpenMenu && lpDIS->itemState&ODS_SELECTED)
    {
      lpDIS->itemState = (lpDIS->itemState&~ODS_SELECTED)|ODS_HOTLIGHT;
    }
    if(!(lpDIS->itemState&ODS_HOTLIGHT))
    {
      colorSel = colorBitmap;
    }
    colorMenu = colorMenuBar;
  }

  CBrush m_brSel(colorSel);
  CBrush m_brBitmap(colorBitmap);

  CRect RectIcon(lpDIS->rcItem);
  CRect RectText(lpDIS->rcItem);
  CRect RectSel(lpDIS->rcItem);

  if(bIsMenuBar)
  {
    RectText.InflateRect (0,0,0,0);
    if(lpDIS->itemState&ODS_DRAW_VERTICAL)
      RectSel.InflateRect (0,0,0, -4);
    else
      RectSel.InflateRect (0,0,-2 -2,0);
  }
  else
  {
    if(nFlags&MFT_RIGHTORDER)
    {
      RectIcon.left = RectIcon.right - (m_iconX + 8);
      RectText.right  = RectIcon.left;
    }
    else
    {
      RectIcon.right = RectIcon.left + m_iconX + 8;
      RectText.left  = RectIcon.right;
    }
    // Draw for Bitmapbackground
    pDC->FillSolidRect (RectIcon,colorBitmap);
  }
  // Draw for Textbackground
  pDC->FillSolidRect (RectText,colorMenu);

  // Spacing for submenu only in popups
  if(!bIsMenuBar)
  {
    if(nFlags&MFT_RIGHTORDER)
    {
      RectText.left += 15;
      RectText.right -= 4;
    }
    else
    {
      RectText.left += 4;
      RectText.right -= 15;
    }
  }

  //  Flag for highlighted item
  if(lpDIS->itemState & (ODS_HOTLIGHT|ODS_INACTIVE) )
  {
    lpDIS->itemState |= ODS_SELECTED;
  }

  if(bIsMenuBar && (lpDIS->itemState&ODS_SELECTED) )
  {
    if(!(lpDIS->itemState&ODS_INACTIVE) )
    {
      SetLastMenuRect(lpDIS->hDC,RectSel);
      if(!(lpDIS->itemState&ODS_HOTLIGHT) )
      {
        // Create a new pen for the special color
        Pen.DeleteObject();
        colorBorder = bHighContrast?GetSysColor(COLOR_BTNTEXT):DarkenColor(128,GetMenuBarColor());
        Pen.CreatePen(PS_SOLID,0,colorBorder);

        int X,Y;
        CRect rect = RectText;
        int winH = rect.Height();

        // Simulate a shadow on right edge...
        if (NumScreenColors() <= 256)
        {
          DWORD clr = GetSysColor(COLOR_BTNSHADOW);
          if(lpDIS->itemState&ODS_DRAW_VERTICAL)
          {
            int winW = rect.Width();
            for (Y=0; Y<=1 ;Y++)
            {
              for (X=4; X<=(winW-1) ;X++)
              {
                pDC->SetPixel(rect.left+X,rect.bottom-Y,clr);
              }
            }
          }
          else
          {
            for (X=3; X<=4 ;X++)
            {
              for (Y=4; Y<=(winH-1) ;Y++)
              {
                pDC->SetPixel(rect.right - X, Y+rect.top, clr );
              }
            }
          }
        }
        else
        {
          if(lpDIS->itemState&ODS_DRAW_VERTICAL)
          {
            int winW = rect.Width();
            COLORREF barColor = pDC->GetPixel(rect.left+4,rect.bottom-4);
            for (Y=1; Y<=4 ;Y++)
            {
              for (X=0; X<4 ;X++)
              {
                if(barColor==CLR_INVALID)
                {
                  barColor = pDC->GetPixel(rect.left+X,rect.bottom-Y);
                }
                pDC->SetPixel(rect.left+X,rect.bottom-Y,colorMenuBar);
              }
              for (X=4; X<8 ;X++)
              {
                pDC->SetPixel(rect.left+X,rect.bottom-Y,DarkenColor(2* 3 * Y * (X - 3), colorMenuBar));
              }
              for (X=8; X<=(winW-1) ;X++)
              {
                pDC->SetPixel(rect.left+X,rect.bottom-Y, DarkenColor(2*15 * Y, colorMenuBar) );
              }
            }
          }
          else
          {
            for (X=1; X<=4 ;X++)
            {
              for (Y=0; Y<4 ;Y++)
              {
                pDC->SetPixel(rect.right-X,Y+rect.top, colorMenuBar );
              }
              for (Y=4; Y<8 ;Y++)
              {
                pDC->SetPixel(rect.right-X,Y+rect.top,DarkenColor(2* 3 * X * (Y - 3), colorMenuBar));
              }
              for (Y=8; Y<=(winH-1) ;Y++)
              {
                pDC->SetPixel(rect.right - X, Y+rect.top, DarkenColor(2*15 * X, colorMenuBar) );
              }
            }
          }
        }
      }
    }
  }
  // For keyboard navigation only
  BOOL bDrawSmallSelection = FALSE;
  // remove the selected bit if it's grayed out
  if( (lpDIS->itemState&ODS_GRAYED) && !m_bSelectDisable)
  {
    if( lpDIS->itemState & ODS_SELECTED )
    {
      lpDIS->itemState = lpDIS->itemState & (~ODS_SELECTED);
      DWORD MsgPos = ::GetMessagePos();
      if( MsgPos==CNewMenuHook::m_dwMsgPos )
      {
        bDrawSmallSelection = TRUE;
      }
      else
      {
        CNewMenuHook::m_dwMsgPos = MsgPos;
      }
    }
  }

  // Draw the seperator
  if( nFlags & MF_SEPARATOR )
  {
    if( pMenuData->m_nTitleFlags&MFT_TITLE )
    {
      DrawTitle(lpDIS,bIsMenuBar);
    }
    else
    {
      // Draw only the seperator
      CRect rect;
      rect.top = RectText.CenterPoint().y;
      rect.bottom = rect.top+1;
      if(nFlags&MFT_RIGHTORDER)
      {
        rect.right = RectText.right;
        rect.left = lpDIS->rcItem.left;
      }
      else
      {
      rect.right = lpDIS->rcItem.right;
        rect.left = RectText.left;
      }
      pDC->FillSolidRect(rect,GetSysColor(COLOR_GRAYTEXT));
    }
  }
  else
  {
    if( (lpDIS->itemState & ODS_SELECTED) && !(lpDIS->itemState&ODS_INACTIVE) )
    {
      pDC->FillSolidRect(RectSel,colorSel);
      // Draw the selection
      CPen* pOldPen = pDC->SelectObject(&Pen);
      CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);
      pDC->Rectangle(RectSel);
      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
    }
    else if (bDrawSmallSelection)
    {
      pDC->FillSolidRect(RectSel,colorMenu);
      // Draw the selection for keyboardnavigation
      CPen* pOldPen = pDC->SelectObject(&Pen);
      CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);
      pDC->Rectangle(RectSel);
      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
    }

    UINT state = lpDIS->itemState;

    BOOL standardflag=FALSE;
    BOOL selectedflag=FALSE;
    BOOL disableflag=FALSE;
    BOOL checkflag=FALSE;

    CString strText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);

    if( (state&ODS_CHECKED) && (pMenuData->m_nMenuIconOffset<0) )
    {
      if(state&ODS_SELECTED && m_selectcheck>0)
      {
        checkflag=TRUE;
      }
      else if(m_unselectcheck>0)
      {
        checkflag=TRUE;
      }
    }
    else if(pMenuData->m_nMenuIconOffset != -1)
    {
      standardflag=TRUE;
      if(state&ODS_SELECTED)
      {
        selectedflag=TRUE;
      }
      else if(state&ODS_GRAYED)
      {
        disableflag=TRUE;
      }
    }

    // draw the menutext
    if(!strText.IsEmpty())
    {
      LOGFONT logFontMenu;
      CFont fontMenu;

#ifdef _NEW_MENU_USER_FONT
      logFontMenu = MENU_USER_FONT;
#else
      NONCLIENTMETRICS nm = {0};
      nm.cbSize = sizeof (nm);
      VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
      logFontMenu =  nm.lfMenuFont;
#endif

      // Default selection?
      if(state&ODS_DEFAULT)
      {
        // Make the font bold
        logFontMenu.lfWeight = FW_BOLD;
      }
      if(state&ODS_DRAW_VERTICAL)
      {
        // rotate font 90°
        logFontMenu.lfOrientation = -900;
        logFontMenu.lfEscapement = -900;
      }

      fontMenu.CreateFontIndirect(&logFontMenu);

      CString leftStr;
      CString rightStr;
      leftStr.Empty();
      rightStr.Empty();

      int tablocr=strText.ReverseFind(_T('\t'));
      if(tablocr!=-1)
      {
        rightStr=strText.Mid(tablocr+1);
        leftStr=strText.Left(strText.Find(_T('\t')));
      }
      else
      {
        leftStr = strText;
      }

      // Draw the text in the correct color:
      UINT nFormat  = DT_LEFT| DT_SINGLELINE|DT_VCENTER;
      UINT nFormatr = DT_RIGHT|DT_SINGLELINE|DT_VCENTER;
      if(nFlags&MFT_RIGHTORDER)
      {
        nFormat  = DT_RIGHT| DT_SINGLELINE|DT_VCENTER|DT_RTLREADING;
        nFormatr = DT_LEFT|DT_SINGLELINE|DT_VCENTER;
      }

      int iOldMode = pDC->SetBkMode( TRANSPARENT);
      CFont* pOldFont = pDC->SelectObject (&fontMenu);

      COLORREF OldTextColor;
      if( (lpDIS->itemState&ODS_GRAYED) ||
        (bIsMenuBar && lpDIS->itemState&ODS_INACTIVE) )
      {
        // Draw the text disabled?
        OldTextColor = pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
      }
      else
      {
        // Draw the text normal
        if( bHighContrast && !bIsMenuBar && !(state&ODS_SELECTED) )
        {
          OldTextColor = pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
        }
        else
        {
          OldTextColor = pDC->SetTextColor(GetSysColor(COLOR_MENUTEXT));
        }
      }
      UINT dt_Hide = (lpDIS->itemState & ODS_NOACCEL)?DT_HIDEPREFIX:0;
      if(dt_Hide && g_Shell>=Win2000)
      {
        BOOL bMenuUnderlines = TRUE;
        if(SystemParametersInfo( SPI_GETKEYBOARDCUES,0,&bMenuUnderlines,0)==TRUE && bMenuUnderlines==TRUE)
        {
          // do not hide
          dt_Hide = 0;
        }
      }

      if(bIsMenuBar)
      {
        MenuDrawText(pDC->m_hDC,leftStr,-1,RectSel, DT_SINGLELINE|DT_VCENTER|DT_CENTER|dt_Hide);
      }
      else
      {
        pDC->DrawText(leftStr,RectText, nFormat|dt_Hide);
        if(tablocr!=-1)
        {
          pDC->DrawText (rightStr,RectText,nFormatr|dt_Hide);
        }
      }
      pDC->SetTextColor(OldTextColor);
      pDC->SelectObject(pOldFont);
      pDC->SetBkMode(iOldMode);
    }

    // Draw the bitmap or checkmarks
    if(!bIsMenuBar)
    {
      CRect rect2 = RectText;

      if(checkflag||standardflag||selectedflag||disableflag)
      {
        if(checkflag && m_checkmaps)
        {
          CPoint ptImage(RectIcon.left+3,RectIcon.top+4);

          if(state&ODS_SELECTED)
          {
            m_checkmaps->Draw(pDC,1,ptImage,ILD_TRANSPARENT);
          }
          else
          {
            m_checkmaps->Draw(pDC,0,ptImage,ILD_TRANSPARENT);
          }
        }
        else
        {
          CSize size = pMenuData->m_pMenuIcon->GetIconSize();
          HICON hDrawIcon = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset);
          //CPoint ptImage(RectIcon.left+3,RectIcon.top+ 4);
          CPoint ptImage( RectIcon.left+3, RectIcon.top + ((RectIcon.Height()-size.cy)>>1) );

          // Need to draw the checked state
          if (state&ODS_CHECKED)
          {
            CRect rect = RectIcon;
            if(nFlags&MFT_RIGHTORDER)
            {
              rect.InflateRect (-2,-1,-1,-1);
            }
            else
            {
            rect.InflateRect (-1,-1,-2,-1);
            }
            if(selectedflag)
            {
              if (NumScreenColors() > 256)
              {
                pDC->FillSolidRect(rect,MixedColor(colorSel,GetSysColor(COLOR_HIGHLIGHT)));
              }
              else
              {
                pDC->FillSolidRect(rect,colorSel); //GetSysColor(COLOR_HIGHLIGHT)
              }
            }
            else
            {
              pDC->FillSolidRect(rect,MixedColor(colorBitmap,GetSysColor(COLOR_HIGHLIGHT)));
            }

            CPen* pOldPen = pDC->SelectObject(&Pen);
            CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);

            pDC->Rectangle(rect);

            pDC->SelectObject(pOldBrush);
            pDC->SelectObject(pOldPen);
          }

          // Correcting of a smaler icon
          if(size.cx<m_iconX)
          {
            ptImage.x += (m_iconX-size.cx)>>1;
          }

          if(state & ODS_DISABLED)
          {
            if(m_bEnableXpBlending)
            {
              // draws the icon blended
              HICON hDrawIcon2 = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset+2);
              pDC->DrawState(ptImage, size, hDrawIcon2, DSS_NORMAL,(HBRUSH)NULL);
              DestroyIcon(hDrawIcon2);
            }
            else
            {
              CBrush Brush;
              Brush.CreateSolidBrush(pDC->GetNearestColor(DarkenColor(70,colorBitmap)));
              pDC->DrawState(ptImage, size, hDrawIcon, DSS_MONO, &Brush);
            }
          }
          else
          {
            if(selectedflag)
            {
              CBrush Brush;
              // Color of the shade
              Brush.CreateSolidBrush(pDC->GetNearestColor(DarkenColorXP(colorSel)));
              if(!(state & ODS_CHECKED))
              {
                ptImage.x++; ptImage.y++;
                pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL | DSS_MONO, &Brush);
                ptImage.x-=2; ptImage.y-=2;
              }
              pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL,(HBRUSH)NULL);
            }
            else
            {
              if(m_bEnableXpBlending)
              {
                // draws the icon blended
                HICON hDrawIcon2 = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset+1);
                pDC->DrawState(ptImage, size, hDrawIcon2, DSS_NORMAL,(HBRUSH)NULL);
                DestroyIcon(hDrawIcon2);
              }
              else
              {
                // draws the icon with normal color
                pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL,(HBRUSH)NULL);
              }
            }
          }
          DestroyIcon(hDrawIcon);
        }
      }

      if(pMenuData->m_nMenuIconOffset<0 /*&& state&ODS_CHECKED */ && !checkflag)
      {
        MENUITEMINFO info = {0};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_CHECKMARKS;

        ::GetMenuItemInfo(HWndToHMenu(lpDIS->hwndItem),lpDIS->itemID,MF_BYCOMMAND, &info);

        if(state&ODS_CHECKED || info.hbmpUnchecked)
        {
          CRect rect = RectIcon;
          if(nFlags&MFT_RIGHTORDER)
          {
            rect.InflateRect (-2,-1,-1,-1);
          }
          else
          {
          rect.InflateRect (-1,-1,-2,-1);
          }
          // draw the color behind checkmarks
          if(state&ODS_SELECTED)
          {
            if (NumScreenColors() > 256)
            {
              pDC->FillSolidRect(rect,MixedColor(colorSel,GetSysColor(COLOR_HIGHLIGHT)));
            }
            else
            {
              pDC->FillSolidRect(rect,colorSel);
            }
          }
          else
          {
            pDC->FillSolidRect(rect,MixedColor(colorBitmap,GetSysColor(COLOR_HIGHLIGHT)));
          }
          CPen* pOldPen = pDC->SelectObject(&Pen);
          CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);

          pDC->Rectangle(rect);

          pDC->SelectObject(pOldBrush);
          pDC->SelectObject(pOldPen);
          if (state&ODS_CHECKED)
          {
            CRect rect(RectIcon);
            rect.InflateRect(2,((m_iconY-RectIcon.Height())>>1)+2);

            if (!info.hbmpChecked)
            { // Checkmark
              DrawSpecialCharStyle(pDC,rect,98,state);
            }
            else if(!info.hbmpUnchecked)
            { // Bullet
              DrawSpecialCharStyle(pDC,rect,105,state);
            }
            else
            { // Draw Bitmap
              BITMAP myInfo = {0};
              GetObject((HGDIOBJ)info.hbmpChecked,sizeof(myInfo),&myInfo);
              CPoint Offset = RectIcon.TopLeft() + CPoint((RectIcon.Width()-myInfo.bmWidth)/2,(RectIcon.Height()-myInfo.bmHeight)/2);
              pDC->DrawState(Offset,CSize(0,0),info.hbmpChecked,DST_BITMAP|DSS_MONO);
            }
          }
          else if(info.hbmpUnchecked)
          {
            // Draw Bitmap
            BITMAP myInfo = {0};
            GetObject((HGDIOBJ)info.hbmpUnchecked,sizeof(myInfo),&myInfo);
            CPoint Offset = RectIcon.TopLeft() + CPoint((RectIcon.Width()-myInfo.bmWidth)/2,(RectIcon.Height()-myInfo.bmHeight)/2);
            if(state & ODS_DISABLED)
            {
              pDC->DrawState(Offset,CSize(0,0),info.hbmpUnchecked,DST_BITMAP|DSS_MONO|DSS_DISABLED);
            }
            else
            {
              pDC->DrawState(Offset,CSize(0,0),info.hbmpUnchecked,DST_BITMAP|DSS_MONO);
            }
          }
        }
        else if ((lpDIS->itemID&0xffff)>=SC_SIZE && (lpDIS->itemID&0xffff)<=SC_HOTKEY )
        {
          DrawSpecial_WinXP(pDC,RectIcon,lpDIS->itemID,state);
        }
      }
    }
  }
}

void CNewMenu::DrawItem_XP_2003(LPDRAWITEMSTRUCT lpDIS, BOOL bIsMenuBar)
{
#ifdef _TRACE_MENU_
  AfxTrace(_T("BarState: 0x%lX MenuID 0x%lX\n"),lpDIS->itemState,lpDIS->itemID);
#endif
  ASSERT(lpDIS != NULL);

  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpDIS->itemData);
  ASSERT(pMenuData);

  UINT nFlags = pMenuData->m_nFlags;

  CNewMemDC memDC(&lpDIS->rcItem,lpDIS->hDC);
  CDC* pDC;
  if( bIsMenuBar || (nFlags&MF_SEPARATOR) )
  { // For title and menubardrawing disable memory painting
    memDC.DoCancel();
    pDC = CDC::FromHandle(lpDIS->hDC);
  }
  else
  {
    pDC = &memDC;
  }

  COLORREF colorWindow = DarkenColor(10,GetSysColor(COLOR_WINDOW));
  COLORREF colorMenuBar = bIsMenuBar?GetMenuBarColor():GetMenuColor();
  COLORREF colorMenu = MixedColor(colorWindow,colorMenuBar);
  COLORREF colorBitmap = MixedColor(GetMenuBarColor(),colorWindow);
  COLORREF colorSel = GetXpHighlightColor();
  COLORREF colorBorder = GetSysColor(COLOR_HIGHLIGHT);//DarkenColor(128,colorMenuBar);

  //COLORREF colorCaption = GetSysColor(COLOR_ACTIVECAPTION); //RGB(0,84,227);
  //COLORREF cc1 = MixedColor(colorWindow,colorCaption);
  //COLORREF cc2 = MidColor(colorWindow,colorCaption);

  COLORREF colorCheck    = RGB(255,192,111);
  COLORREF colorCheckSel = RGB(254,128,62);
  colorSel = RGB(255,238,194);
  if(!IsMenuThemeActive() &&
    GetMenuDrawMode()!=STYLE_COLORFUL_NOBORDER &&
      GetMenuDrawMode()!=STYLE_COLORFUL)
  {
    colorSel = GetXpHighlightColor();
    //colorCheck = pDC->GetNearestColor(DarkenColorXP(colorSel));
    colorCheckSel = MixedColor(colorSel,GetSysColor(COLOR_HIGHLIGHT));
    colorCheck = colorCheckSel;
  }

  colorBitmap = colorMenuBar = GetMenuBarColor2003();
  if(bHighContrast)
  {
    colorBorder = GetSysColor(COLOR_BTNTEXT);
  }

  // Better contrast when you have less than 256 colors
  if(pDC->GetNearestColor(colorMenu)==pDC->GetNearestColor(colorBitmap))
  {
    colorMenu = colorWindow;
    colorBitmap = colorMenuBar;
  }

  CPen Pen(PS_SOLID,0,colorBorder);

  if(bIsMenuBar)
  {
    if(!IsMenu(UIntToHMenu(lpDIS->itemID)) && (lpDIS->itemState&ODS_SELECTED_OPEN))
    {
      lpDIS->itemState = (lpDIS->itemState&~(ODS_SELECTED|ODS_SELECTED_OPEN))|ODS_HOTLIGHT;
    }
    else if( !(lpDIS->itemState&ODS_SELECTED_OPEN) && !m_dwOpenMenu && lpDIS->itemState&ODS_SELECTED)
    {
      lpDIS->itemState = (lpDIS->itemState&~ODS_SELECTED)|ODS_HOTLIGHT;
    }
    if(!(lpDIS->itemState&ODS_HOTLIGHT))
    {
      colorSel = colorBitmap;
    }
    colorMenu = colorMenuBar;
  }

  CBrush m_brSel(colorSel);
  CBrush m_brBitmap(colorBitmap);

  CRect RectIcon(lpDIS->rcItem);
  CRect RectText(lpDIS->rcItem);
  CRect RectSel(lpDIS->rcItem);

  if(bIsMenuBar)
  {
    RectText.InflateRect (0,0,0,0);
    if(lpDIS->itemState&ODS_DRAW_VERTICAL)
    {
      RectSel.InflateRect (0,0,0, -4);
    }
    else
    {
      RectSel.InflateRect (0,0,-4,0);
    }

    if(lpDIS->itemState&ODS_SELECTED)
    {
      if(lpDIS->itemState&ODS_DRAW_VERTICAL)
        RectText.bottom -=4;
      else
        RectText.right -=4;
      if(NumScreenColors() <= 256)
      {
        pDC->FillSolidRect(RectText,colorMenu);
      }
      else
      {
        DrawGradient(pDC,RectText,colorMenu,colorBitmap,FALSE,TRUE);
      }
      if(lpDIS->itemState&ODS_DRAW_VERTICAL)
      {
        RectText.bottom +=4;
      }
      else
      {
        RectText.right +=4;
      }
    }
    else
    {
      MENUINFO menuInfo = {0};
      menuInfo.cbSize = sizeof(menuInfo);
      menuInfo.fMask = MIM_BACKGROUND;

      if(::GetMenuInfo(m_hMenu,&menuInfo) && menuInfo.hbrBack)
      {
        CBrush *pBrush = CBrush::FromHandle(menuInfo.hbrBack);
        CPoint brushOrg(0,0);

        // new support for menubar
        CControlBar* pControlBar = DYNAMIC_DOWNCAST(CControlBar,pDC->GetWindow());
        if(pControlBar)
        {
          CFrameWnd* pFrame = pControlBar->GetParentFrame();
          if(pFrame)
          {
            CRect windowRect;
            pFrame->GetWindowRect(windowRect);
            pControlBar->ScreenToClient(windowRect);
            brushOrg = windowRect.TopLeft();
          }
        }

        VERIFY(pBrush->UnrealizeObject());
        CPoint oldOrg = pDC->SetBrushOrg(brushOrg);
        pDC->FillRect(RectText,pBrush);
        pDC->SetBrushOrg(oldOrg);
      }
      else
      {
        pDC->FillSolidRect(RectText,colorMenu);
      }
      }
    }
  else
  {
    if(nFlags&MFT_RIGHTORDER)
    {
      RectIcon.left = RectIcon.right - (m_iconX + 8);
      RectText.right= RectIcon.left;
      // Draw for Bitmapbackground
      DrawGradient(pDC,RectIcon,colorBitmap,colorMenu,TRUE);
  }
  else
  {
      RectIcon.right = RectIcon.left + m_iconX + 8;
      RectText.left  = RectIcon.right;
    // Draw for Bitmapbackground
      DrawGradient(pDC,RectIcon,colorMenu,colorBitmap,TRUE);
    }
    // Draw for Textbackground
    pDC->FillSolidRect(RectText,colorMenu);
  }

  // Spacing for submenu only in popups
  if(!bIsMenuBar)
  {
    if(nFlags&MFT_RIGHTORDER)
    {
      RectText.left += 15;
      RectText.right -= 4;
    }
    else
    {
      RectText.left += 4;
      RectText.right -= 15;
    }
  }

  //  Flag for highlighted item
  if(lpDIS->itemState & (ODS_HOTLIGHT|ODS_INACTIVE) )
  {
    lpDIS->itemState |= ODS_SELECTED;
  }

  if(bIsMenuBar && (lpDIS->itemState&ODS_SELECTED) )
  {
    if(!(lpDIS->itemState&ODS_INACTIVE) )
    {
      SetLastMenuRect(lpDIS->hDC,RectSel);
      if(!(lpDIS->itemState&ODS_HOTLIGHT) )
      {
        // Create a new pen for the special color
        Pen.DeleteObject();
        colorBorder = bHighContrast?GetSysColor(COLOR_BTNTEXT):DarkenColor(128,GetMenuBarColor());
        Pen.CreatePen(PS_SOLID,0,colorBorder);

        int X,Y;
        CRect rect = RectText;
        int winH = rect.Height();

        // Simulate a shadow on right edge...
        if (NumScreenColors() <= 256)
        {
          DWORD clr = GetSysColor(COLOR_BTNSHADOW);
          if(lpDIS->itemState&ODS_DRAW_VERTICAL)
          {
            int winW = rect.Width();
            for (Y=0; Y<=1 ;Y++)
            {
              for (X=4; X<=(winW-1) ;X++)
              {
                pDC->SetPixel(rect.left+X,rect.bottom-Y,clr);
              }
            }
          }
          else
          {
            for (X=3; X<=4 ;X++)
            {
              for (Y=4; Y<=(winH-1) ;Y++)
              {
                pDC->SetPixel(rect.right - X, Y + rect.top, clr );
              }
            }
          }
        }
        else
        {
          if(lpDIS->itemState&ODS_DRAW_VERTICAL)
          {
            int winW = rect.Width();
            COLORREF barColor = pDC->GetPixel(rect.left+4,rect.bottom-4);
            for (Y=1; Y<=4 ;Y++)
            {
              for (X=0; X<4 ;X++)
              {
                if(barColor==CLR_INVALID)
                {
                  barColor = pDC->GetPixel(rect.left+X,rect.bottom-Y);
                }
                pDC->SetPixel(rect.left+X,rect.bottom-Y,barColor);
              }
              for (X=4; X<8 ;X++)
              {
                pDC->SetPixel(rect.left+X,rect.bottom-Y,DarkenColor(2* 3 * Y * (X - 3), barColor));
              }
              for (X=8; X<=(winW-1) ;X++)
              {
                pDC->SetPixel(rect.left+X,rect.bottom-Y, DarkenColor(2*15 * Y, barColor) );
              }
            }
          }
          else
          {
            COLORREF barColor = pDC->GetPixel(rect.right-1,rect.top);
            for (X=1; X<=4 ;X++)
            {
              for (Y=0; Y<4 ;Y++)
              {
                if(barColor==CLR_INVALID)
                {
                  barColor = pDC->GetPixel(rect.right-X,Y+rect.top);
                }
                pDC->SetPixel(rect.right-X,Y+rect.top, barColor );
              }
              for (Y=4; Y<8 ;Y++)
              {
                pDC->SetPixel(rect.right-X,Y+rect.top,DarkenColor(2* 3 * X * (Y - 3), barColor));
              }
              for (Y=8; Y<=(winH-1) ;Y++)
              {
                pDC->SetPixel(rect.right - X, Y+rect.top, DarkenColor(2*15 * X, barColor) );
              }
            }
          }
        }
      }
    }
  }
  // For keyboard navigation only
  BOOL bDrawSmallSelection = FALSE;
  // remove the selected bit if it's grayed out
  if( (lpDIS->itemState&ODS_GRAYED) && !m_bSelectDisable)
  {
    if( lpDIS->itemState & ODS_SELECTED )
    {
      lpDIS->itemState = lpDIS->itemState & (~ODS_SELECTED);
      DWORD MsgPos = ::GetMessagePos();
      if( MsgPos==CNewMenuHook::m_dwMsgPos )
      {
        bDrawSmallSelection = TRUE;
      }
      else
      {
        CNewMenuHook::m_dwMsgPos = MsgPos;
      }
    }
  }

  // Draw the seperator
  if( nFlags & MFT_SEPARATOR )
  {
    if( pMenuData->m_nTitleFlags&MFT_TITLE )
    {
      DrawTitle(lpDIS,bIsMenuBar);
    }
    else
    {
      // Draw only the seperator
      CRect rect;
      rect.top = RectText.CenterPoint().y;
      rect.bottom = rect.top+1;

      if(nFlags&MFT_RIGHTORDER)
      {
        rect.right = RectText.right;
        rect.left = lpDIS->rcItem.left;
      }
      else
      {
      rect.right = lpDIS->rcItem.right;
        rect.left = RectText.left;
      }
      pDC->FillSolidRect(rect,GetSysColor(COLOR_GRAYTEXT));
    }
  }
  else
  {
    if( (lpDIS->itemState & ODS_SELECTED) && !(lpDIS->itemState&ODS_INACTIVE) )
    {
      if(bIsMenuBar)
      {
        if(NumScreenColors() <= 256)
        {
          pDC->FillSolidRect(RectSel,colorWindow);
        }
        else
        {
          DrawGradient(pDC,RectSel,colorWindow,colorSel,FALSE,TRUE);
        }
      }
      else
      {
        pDC->FillSolidRect(RectSel,colorSel);
      }
      // Draw the selection
      CPen* pOldPen = pDC->SelectObject(&Pen);
      CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);
      pDC->Rectangle(RectSel);
      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
    }
    else if (bDrawSmallSelection)
    {
      pDC->FillSolidRect(RectSel,colorMenu);
      // Draw the selection for keyboardnavigation
      CPen* pOldPen = pDC->SelectObject(&Pen);
      CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);
      pDC->Rectangle(RectSel);
      pDC->SelectObject(pOldBrush);
      pDC->SelectObject(pOldPen);
    }

    UINT state = lpDIS->itemState;

    BOOL standardflag=FALSE;
    BOOL selectedflag=FALSE;
    BOOL disableflag=FALSE;
    BOOL checkflag=FALSE;

    CString strText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);

    if( (state&ODS_CHECKED) && (pMenuData->m_nMenuIconOffset<0) )
    {
      if(state&ODS_SELECTED && m_selectcheck>0)
      {
        checkflag=TRUE;
      }
      else if(m_unselectcheck>0)
      {
        checkflag=TRUE;
      }
    }
    else if(pMenuData->m_nMenuIconOffset != -1)
    {
      standardflag = TRUE;
      if(state&ODS_SELECTED)
      {
        selectedflag=TRUE;
      }
      else if(state&ODS_GRAYED)
      {
        disableflag=TRUE;
      }
    }

    if(!strText.IsEmpty())
    {  // draw the menutext
      LOGFONT logFontMenu;
      CFont fontMenu;

#ifdef _NEW_MENU_USER_FONT
      logFontMenu = MENU_USER_FONT;
#else
      NONCLIENTMETRICS nm = {0};
      nm.cbSize = sizeof (nm);
      VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
      logFontMenu = nm.lfMenuFont;
#endif

      // Default selection?
      if(state&ODS_DEFAULT)
      {
        // Make the font bold
        logFontMenu.lfWeight = FW_BOLD;
      }
      if(state&ODS_DRAW_VERTICAL)
      {
        // rotate font 90°
        logFontMenu.lfOrientation = -900;
        logFontMenu.lfEscapement = -900;
      }

      fontMenu.CreateFontIndirect(&logFontMenu);

      CString leftStr;
      CString rightStr;
      leftStr.Empty();
      rightStr.Empty();

      int tablocr = strText.ReverseFind(_T('\t'));
      if(tablocr!=-1)
      {
        rightStr = strText.Mid(tablocr+1);
        leftStr = strText.Left(strText.Find(_T('\t')));
      }
      else
      {
        leftStr=strText;
      }

      // Draw the text in the correct color:
      UINT nFormat  = DT_LEFT| DT_SINGLELINE|DT_VCENTER;
      UINT nFormatr = DT_RIGHT|DT_SINGLELINE|DT_VCENTER;
      if(nFlags&MFT_RIGHTORDER)
      {
        nFormat  = DT_RIGHT| DT_SINGLELINE|DT_VCENTER|DT_RTLREADING;
        nFormatr = DT_LEFT|DT_SINGLELINE|DT_VCENTER;
      }

      int iOldMode = pDC->SetBkMode( TRANSPARENT);
      CFont* pOldFont = pDC->SelectObject (&fontMenu);

      COLORREF OldTextColor;
      if( (lpDIS->itemState&ODS_GRAYED) ||
        (bIsMenuBar && lpDIS->itemState&ODS_INACTIVE) )
      {
        // Draw the text disabled?
        if(bIsMenuBar && (NumScreenColors() <= 256) )
        {
          OldTextColor = pDC->SetTextColor(colorWindow);
        }
        else
        {
          OldTextColor = pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
        }
      }
      else
      {
        // Draw the text normal
        if( bHighContrast && !bIsMenuBar && !(state&ODS_SELECTED) )
        {
          OldTextColor = pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
        }
        else
        {
          OldTextColor = pDC->SetTextColor(GetSysColor(COLOR_MENUTEXT));
        }
      }
      // Office 2003 ignors this settings
      UINT dt_Hide = 0;//(lpDIS->itemState & ODS_NOACCEL)?DT_HIDEPREFIX:0;
      if(bIsMenuBar)
      {
        MenuDrawText(pDC->m_hDC,leftStr,-1,RectSel, DT_SINGLELINE|DT_VCENTER|DT_CENTER|dt_Hide);
      }
      else
      {
        pDC->DrawText(leftStr,RectText, nFormat|dt_Hide);
        if(tablocr!=-1)
        {
          pDC->DrawText (rightStr,RectText,nFormatr|dt_Hide);
        }
      }
      pDC->SetTextColor(OldTextColor);
      pDC->SelectObject(pOldFont);
      pDC->SetBkMode(iOldMode);
    }

    // Draw the bitmap or checkmarks
    if(!bIsMenuBar)
    {
      CRect rect2 = RectText;

      if(checkflag||standardflag||selectedflag||disableflag)
      {
        if(checkflag && m_checkmaps)
        {
          CPoint ptImage(RectIcon.left+3,RectIcon.top+4);

          if(state&ODS_SELECTED)
          {
            m_checkmaps->Draw(pDC,1,ptImage,ILD_TRANSPARENT);
          }
          else
          {
            m_checkmaps->Draw(pDC,0,ptImage,ILD_TRANSPARENT);
          }
        }
        else
        {
          CSize size = pMenuData->m_pMenuIcon->GetIconSize();
          HICON hDrawIcon = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset);
          //CPoint ptImage(RectIcon.left+3,RectIcon.top+ 4);
          CPoint ptImage( RectIcon.left+3, RectIcon.top + ((RectIcon.Height()-size.cy)>>1) );

          // Need to draw the checked state
          if (state&ODS_CHECKED)
          {
            CRect rect = RectIcon;
            if(nFlags&MFT_RIGHTORDER)
            {
              rect.InflateRect (-2,-1,-1,-1);
            }
            else
            {
            rect.InflateRect (-1,-1,-2,-1);
            }
            if(selectedflag)
            {
              pDC->FillSolidRect(rect,colorCheckSel);
            }
            else
            {
              pDC->FillSolidRect(rect,colorCheck);
            }

            CPen* pOldPen = pDC->SelectObject(&Pen);
            CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);

            pDC->Rectangle(rect);

            pDC->SelectObject(pOldBrush);
            pDC->SelectObject(pOldPen);
          }

          // Correcting of a smaler icon
          if(size.cx<m_iconX)
          {
            ptImage.x += (m_iconX-size.cx)>>1;
          }

          if(state & ODS_DISABLED)
          {
            //if(m_bEnableXpBlending)
            {
              // draws the icon blended
              HICON hDrawIcon2 = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset+2);
              pDC->DrawState(ptImage, size, hDrawIcon2, DSS_NORMAL,(HBRUSH)NULL);
              DestroyIcon(hDrawIcon2);
            }
            //else
            //{
            //  CBrush Brush;
            //  Brush.CreateSolidBrush(pDC->GetNearestColor(DarkenColor(70,colorBitmap)));
            //  pDC->DrawState(ptImage, size, hDrawIcon, DSS_MONO, &Brush);
            //}
          }
          else
          {
            if(selectedflag)
            {
              //if(!(state & ODS_CHECKED))
              //{
              //  CBrush Brush;
              //  // Color of the shade
              //  Brush.CreateSolidBrush(pDC->GetNearestColor(DarkenColorXP(colorSel)));
              //  ptImage.x++; ptImage.y++;
              //  pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL | DSS_MONO, &Brush);
              //  ptImage.x-=2; ptImage.y-=2;
              //}
              pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL,(HBRUSH)NULL);
            }
            else
            {
              if(m_bEnableXpBlending)
              {
                // draws the icon blended
                HICON hDrawIcon2 = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset+1);
                pDC->DrawState(ptImage, size, hDrawIcon2, DSS_NORMAL,(HBRUSH)NULL);
                DestroyIcon(hDrawIcon2);
              }
              else
              {
                // draws the icon with normal color
                pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL,(HBRUSH)NULL);
                //ImageList_DrawEx(pMenuData->m_pMenuIcon->m_IconsList,pMenuData->m_nMenuIconOffset,pDC->GetSafeHdc(),ptImage.x,ptImage.y,0,0,CLR_DEFAULT,CLR_DEFAULT,ILD_NORMAL);
              }
            }
          }
          DestroyIcon(hDrawIcon);
        }
      }

      if(pMenuData->m_nMenuIconOffset<0 /*&& state&ODS_CHECKED */ && !checkflag)
      {
        MENUITEMINFO info = {0};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_CHECKMARKS;

        ::GetMenuItemInfo(HWndToHMenu(lpDIS->hwndItem),lpDIS->itemID,MF_BYCOMMAND, &info);

        if(state&ODS_CHECKED || info.hbmpUnchecked)
        {
          CRect rect = RectIcon;
          if(nFlags&MFT_RIGHTORDER)
          {
            rect.InflateRect (-2,-1,-1,-1);
          }
          else
          {
          rect.InflateRect (-1,-1,-2,-1);
          }
          // draw the color behind checkmarks
          if(state&ODS_SELECTED)
          {
            pDC->FillSolidRect(rect,colorCheckSel);
          }
          else
          {
            pDC->FillSolidRect(rect,colorCheck);
          }
          CPen* pOldPen = pDC->SelectObject(&Pen);
          CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(HOLLOW_BRUSH);

          pDC->Rectangle(rect);

          pDC->SelectObject(pOldBrush);
          pDC->SelectObject(pOldPen);
          if (state&ODS_CHECKED)
          {
            CRect rect(RectIcon);
            rect.InflateRect(2,((m_iconY-RectIcon.Height())>>1)+2);

            if (!info.hbmpChecked)
            { // Checkmark
              DrawSpecialCharStyle(pDC,rect,98,state);
            }
            else if(!info.hbmpUnchecked)
            { // Bullet
              DrawSpecialCharStyle(pDC,rect,105,state);
            }
            else
            { // Draw Bitmap
              BITMAP myInfo = {0};
              GetObject((HGDIOBJ)info.hbmpChecked,sizeof(myInfo),&myInfo);
              CPoint Offset = RectIcon.TopLeft() + CPoint((RectIcon.Width()-myInfo.bmWidth)/2,(RectIcon.Height()-myInfo.bmHeight)/2);
              pDC->DrawState(Offset,CSize(0,0),info.hbmpChecked,DST_BITMAP|DSS_MONO);
            }
          }
          else
          {
            // Draw Bitmap
            BITMAP myInfo = {0};
            GetObject((HGDIOBJ)info.hbmpUnchecked,sizeof(myInfo),&myInfo);
            CPoint Offset = RectIcon.TopLeft() + CPoint((RectIcon.Width()-myInfo.bmWidth)/2,(RectIcon.Height()-myInfo.bmHeight)/2);
            if(state & ODS_DISABLED)
            {
              pDC->DrawState(Offset,CSize(0,0),info.hbmpUnchecked,DST_BITMAP|DSS_MONO|DSS_DISABLED);
            }
            else
            {
              pDC->DrawState(Offset,CSize(0,0),info.hbmpUnchecked,DST_BITMAP|DSS_MONO);
            }
          }
        }
        else if ((lpDIS->itemID&0xffff)>=SC_SIZE && (lpDIS->itemID&0xffff)<=SC_HOTKEY )
        {
          DrawSpecial_WinXP(pDC,RectIcon,lpDIS->itemID,state);
        }
      }
    }
  }
}

void CNewMenu::DrawItem_SpecialStyle (LPDRAWITEMSTRUCT lpDIS, BOOL bIsMenuBar)
{
  if(!bIsMenuBar)
  {
    DrawItem_OldStyle(lpDIS,bIsMenuBar);
    return;
  }

  ASSERT(lpDIS != NULL);
  //CNewMemDC memDC(&lpDIS->rcItem,lpDIS->hDC);
  //CDC* pDC = &memDC;
  CDC* pDC = CDC::FromHandle(lpDIS->hDC);

  ASSERT(lpDIS->itemData);
  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpDIS->itemData);

  CRect rect(lpDIS->rcItem);
  //rect.InflateRect(0,-1);

  COLORREF colorBack;
  if(lpDIS->itemState&(ODS_SELECTED|ODS_HOTLIGHT))
  {
    colorBack = GetSysColor(COLOR_HIGHLIGHT);
    SetLastMenuRect(lpDIS->hDC,rect);
  }
  else
  {
    colorBack = GetMenuBarColor();
  }
  pDC->FillSolidRect(rect,colorBack);

  int iOldMode = pDC->SetBkMode( TRANSPARENT);
  CString strText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
  COLORREF crTextColor;
  if(!(lpDIS->itemState & ODS_GRAYED))
  {
    if(lpDIS->itemState&(ODS_SELECTED|ODS_HOTLIGHT))
    {
      crTextColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
    }
    else
    {
      crTextColor = GetSysColor(COLOR_MENUTEXT);
    }
  }
  else
  {
    crTextColor = GetSysColor(COLOR_GRAYTEXT);
  }
  COLORREF oldColor = pDC->SetTextColor(crTextColor);

  CFont fontMenu;
  LOGFONT logFontMenu;

#ifdef _NEW_MENU_USER_FONT
  logFontMenu =  MENU_USER_FONT;
#else
  NONCLIENTMETRICS nm = {0};
  nm.cbSize = sizeof (nm);
  VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
  logFontMenu =  nm.lfMenuFont;
#endif
  if(lpDIS->itemState&ODS_DRAW_VERTICAL)
  {
    // rotate font 90°
    logFontMenu.lfOrientation = -900;
    logFontMenu.lfEscapement = -900;
  }
  fontMenu.CreateFontIndirect (&logFontMenu);

  CFont* pOldFont = pDC->SelectObject(&fontMenu);
  UINT dt_Hide = (lpDIS->itemState & ODS_NOACCEL)?DT_HIDEPREFIX:0;
  if(dt_Hide && g_Shell>=Win2000)
  {
    BOOL bMenuUnderlines = TRUE;
    if(SystemParametersInfo( SPI_GETKEYBOARDCUES,0,&bMenuUnderlines,0)==TRUE && bMenuUnderlines==TRUE)
    {
      // do not hide
      dt_Hide = 0;
    }
  }
  MenuDrawText(pDC->m_hDC,strText,-1,rect,DT_CENTER|DT_SINGLELINE|DT_VCENTER|dt_Hide);
  pDC->SelectObject(pOldFont);

  pDC->SetTextColor(oldColor);
  pDC->SetBkMode( iOldMode);
}

void CNewMenu::DrawItem_Icy(LPDRAWITEMSTRUCT lpDIS, BOOL bIsMenuBar)
{
  ASSERT(lpDIS != NULL);
  CRect rect;

  ASSERT(lpDIS->itemData);
  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpDIS->itemData);

  UINT nFlags = pMenuData->m_nFlags;
  CNewMemDC memDC(&lpDIS->rcItem,lpDIS->hDC);
  CDC* pDC;
  if( bIsMenuBar || (nFlags&MF_SEPARATOR) )
  { // For title and menubardrawing disable memory painting
    memDC.DoCancel();
    pDC = CDC::FromHandle(lpDIS->hDC);
  }
  else
  {
    pDC = &memDC;
  }

  COLORREF colorMenu = GetMenuColor();
  COLORREF colorBack = bIsMenuBar?GetMenuBarColor():colorMenu;
  COLORREF color3DShadow = DarkenColor(60,colorMenu);
  COLORREF color3DHilight = LightenColor(60,colorMenu);
  COLORREF colorGrayText = DarkenColor(100,colorMenu);//GetSysColor(COLOR_GRAYTEXT);
  COLORREF colorHilight = GetSysColor(COLOR_HIGHLIGHT);
  COLORREF colorGrayed = bHighContrast?GetSysColor(COLOR_BTNSHADOW):pDC->GetNearestColor(DarkenColor(70,color3DShadow));

  if(bHighContrast)
  {
    color3DShadow = GetSysColor(COLOR_BTNTEXT);
    color3DHilight = GetSysColor(COLOR_BTNTEXT);
  }

  CRect RectIcon(lpDIS->rcItem);
  CRect RectText(lpDIS->rcItem);
  CRect RectSel(lpDIS->rcItem);

  RectIcon.InflateRect (-1,0,0,0);
  RectText.InflateRect (-1,0,0,0);
  RectSel.InflateRect (0,0,0,0);

  if(!bIsMenuBar)
  {
    if(nFlags&MFT_RIGHTORDER)
    {
      RectIcon.left = RectIcon.right - (m_iconX + 6 + GAP);
      RectText.right  = RectIcon.left;
    }
    else
    {
      RectIcon.right = RectIcon.left + m_iconX + 6 + GAP;
      RectText.left  = RectIcon.right;
    }
  }
  else
  {
    RectText.right += 6;
    RectSel.InflateRect(0,0,-1,-1);
    RectText.InflateRect (1,-2,0,0);

    if(!IsMenu(UIntToHMenu(lpDIS->itemID)) && (lpDIS->itemState&ODS_SELECTED_OPEN))
    {
      lpDIS->itemState = (lpDIS->itemState&~(ODS_SELECTED|ODS_SELECTED_OPEN))|ODS_HOTLIGHT;
    }
    else if(!(lpDIS->itemState&ODS_SELECTED_OPEN) && !m_dwOpenMenu && lpDIS->itemState&ODS_SELECTED)
    {
      lpDIS->itemState = (lpDIS->itemState&~ODS_SELECTED)|ODS_HOTLIGHT;
    }
    if(lpDIS->itemState&(ODS_SELECTED|ODS_HOTLIGHT))
    {
      SetLastMenuRect(lpDIS->hDC,RectSel);
    }
  }

  // For keyboard navigation only
  BOOL bDrawSmallSelection = FALSE;

  // remove the selected bit if it's grayed out
  if( (lpDIS->itemState&ODS_GRAYED) && !m_bSelectDisable )
  {
    if(lpDIS->itemState & ODS_SELECTED)
    {
      lpDIS->itemState &= ~ODS_SELECTED;
      DWORD MsgPos = ::GetMessagePos();
      if(MsgPos==CNewMenuHook::m_dwMsgPos)
      {
        bDrawSmallSelection = TRUE;
      }
      else
      {
        CNewMenuHook::m_dwMsgPos = MsgPos;
      }
    }
  }

  if(nFlags & MF_SEPARATOR)
  {
    if(pMenuData->m_nTitleFlags&MFT_TITLE)
    {
      DrawTitle(lpDIS,bIsMenuBar);
    }
    else
    {
      rect = lpDIS->rcItem;
      //pDC->FillSolidRect(rect,colorBack);
      pDC->FillSolidRect(rect,color3DHilight);
      rect.left += 1;
      rect.right -= 1;
      pDC->DrawEdge(&rect,EDGE_ETCHED,BF_TOP);
    }
  }
  else
  {
    CRect rect2;
    BOOL standardflag=FALSE,selectedflag=FALSE,disableflag=FALSE;
    BOOL checkflag=FALSE;

    CBrush m_brSelect;
    int nIconNormal=-1;

    // set some colors and the font
    m_brSelect.CreateSolidBrush(colorHilight);
    rect2=rect=RectSel;

    // draw the up/down/focused/disabled state
    UINT state = lpDIS->itemState;

    nIconNormal = pMenuData->m_nMenuIconOffset;

    if( (state&ODS_CHECKED) && nIconNormal<0)
    {
      if(state&ODS_SELECTED && m_selectcheck>0)
      {
        checkflag = TRUE;
      }
      else if(m_unselectcheck>0)
      {
        checkflag = TRUE;
      }
    }
    else if(nIconNormal != -1)
    {
      standardflag = TRUE;
      if(state&ODS_SELECTED && !(state&ODS_GRAYED))
      {
        selectedflag=TRUE;
      }
      else if(state&ODS_GRAYED)
      {
        disableflag=TRUE;
      }
    }

    if(bIsMenuBar)
    {
      CRect tempRect = rect;
      tempRect.InflateRect (0,0,0,1);
      rect.OffsetRect(0,1);
      rect2=rect;

      if( bHighContrast || (state&ODS_INACTIVE) || (!(state&ODS_HOTLIGHT) && !(state&ODS_SELECTED)) )
      {
        // pDC->FillSolidRect(tempRect,colorBack);
        MENUINFO menuInfo = {0};
        menuInfo.cbSize = sizeof(menuInfo);
        menuInfo.fMask = MIM_BACKGROUND;

        if(!bHighContrast && ::GetMenuInfo(m_hMenu,&menuInfo) && menuInfo.hbrBack)
        {
          CBrush *pBrush = CBrush::FromHandle(menuInfo.hbrBack);
          VERIFY(pBrush->UnrealizeObject());
          CPoint oldOrg = pDC->SetBrushOrg(0,0);
          pDC->FillRect(tempRect,pBrush);
          pDC->SetBrushOrg(oldOrg);
        }
        else
        {
          pDC->FillSolidRect(tempRect,colorBack);
        }
      }
      else
      {
        colorBack = GetMenuColor();
        DrawGradient(pDC,tempRect,LightenColor(30,colorBack),DarkenColor(30,colorBack),false,true);
      }
    }
    else
    {
      if(bHighContrast)
      {
        pDC->FillSolidRect (rect,colorBack);
      }
      else
      {
        DrawGradient(pDC,rect,LightenColor(30,colorBack),DarkenColor(30,colorBack),false,true);
      }
    }
    // Draw the selection
    if(state&ODS_SELECTED)
    {
      // You need only Text highlight and that's what you get
      if(!bIsMenuBar)
      {
        if(checkflag||standardflag||selectedflag||disableflag||state&ODS_CHECKED)
        {
          rect2 = RectText;
        }

        if(bHighContrast)
        {
          pDC->FillSolidRect (rect2,colorHilight);
        }
        else
        {
          DrawGradient(pDC,rect2,LightenColor(30,colorHilight),DarkenColor(30,colorHilight),false,true);
        }
        pDC->Draw3dRect(rect2 ,color3DShadow,color3DHilight);
      }
      else
      {
        pDC->Draw3dRect(rect2 ,color3DShadow,color3DHilight);
      }
    }
    else if(bIsMenuBar && (state&ODS_HOTLIGHT) && !(state&ODS_INACTIVE))
    {
      pDC->Draw3dRect(rect,color3DHilight,color3DShadow);
    }
    else if (bDrawSmallSelection)
    {
      pDC->DrawFocusRect(rect);
    }

    // Draw the Bitmap or checkmarks
    if(!bIsMenuBar)
    {
      CRect IconRect = RectIcon;
      // center the image
      IconRect.InflateRect(-(RectIcon.Width()-m_iconX)>>1,-(RectIcon.Height()-m_iconY)>>1);
      CPoint ptImage = IconRect.TopLeft();
      IconRect.InflateRect(2,2);

      if(checkflag||standardflag||selectedflag||disableflag)
      {
        if(checkflag && m_checkmaps)
        {
          if(state&ODS_SELECTED)
          {
            m_checkmaps->Draw(pDC,1,ptImage,ILD_TRANSPARENT);
          }
          else
          {
            m_checkmaps->Draw(pDC,0,ptImage,ILD_TRANSPARENT);
          }
        }
        else
        {
          // Need to draw the checked state
          if (IsNewShell())
          {
            if(state&ODS_CHECKED)
            {
              pDC->Draw3dRect(IconRect,color3DShadow,color3DHilight);
            }
            else if (selectedflag)
            {
              pDC->Draw3dRect(IconRect,color3DHilight,color3DShadow);
            }
          }

          CSize size = pMenuData->m_pMenuIcon->GetIconSize();
          // Correcting of a smaler icon
          if(size.cx<m_iconX)
          {
            ptImage.x += (m_iconX-size.cx)>>1;
          }
          if(size.cy<m_iconY)
          {
            ptImage.y += (m_iconY-size.cy)>>1;
          }

          HICON hDrawIcon = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset);
          if(state & ODS_DISABLED)
          {
            CBrush Brush;
            Brush.CreateSolidBrush(colorGrayed);
            pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL | DSS_MONO, (HBRUSH)Brush);
          }
          else
          {
            pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL, (HBRUSH)NULL);
          }
          DestroyIcon(hDrawIcon);
        }
      }
      if ((lpDIS->itemID&0xffff)>=SC_SIZE && (lpDIS->itemID&0xffff)<=SC_HOTKEY )
      {
        DrawSpecial_OldStyle(pDC,IconRect,lpDIS->itemID,state);
      }
      else if(nIconNormal<0 /*&& state&ODS_CHECKED */&& !checkflag)
      {
        MENUITEMINFO info = {0};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_CHECKMARKS;

        ::GetMenuItemInfo(HWndToHMenu(lpDIS->hwndItem),lpDIS->itemID,MF_BYCOMMAND, &info);

        Draw3DCheckmark(pDC, IconRect,info.hbmpChecked,info.hbmpUnchecked,state);
      }
    }


    CString strText;
    strText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
    if(!strText.IsEmpty())
    {
      COLORREF crText = GetSysColor(COLOR_MENUTEXT);

      if(bIsMenuBar)
      {
        rect.left += 6;
        if(lpDIS->itemState&ODS_INACTIVE)
        {
          crText = colorGrayText;
        }
      }
      else
      {
        if(lpDIS->itemState&ODS_SELECTED)
        {
          crText = GetSysColor(COLOR_HIGHLIGHTTEXT);
        }
        rect.left += m_iconX + 12;
      }
      CRect rectt(rect.left,rect.top-1,rect.right,rect.bottom-1);

      // Find tabs
      CString leftStr,rightStr;
      leftStr.Empty();rightStr.Empty();

      int tablocr = strText.ReverseFind(_T('\t'));
      if(tablocr!=-1)
      {
        rightStr = strText.Mid(tablocr+1);
        leftStr = strText.Left(strText.Find(_T('\t')));
        rectt.right -= m_iconX;
      }
      else
      {
        leftStr = strText;
      }

      int iOldMode = pDC->SetBkMode( TRANSPARENT);
      // Draw the text in the correct colour:
      UINT dt_Hide = (lpDIS->itemState & ODS_NOACCEL)?DT_HIDEPREFIX:0;
      if(dt_Hide && g_Shell>=Win2000)
      {
        BOOL bMenuUnderlines = TRUE;
        if(SystemParametersInfo( SPI_GETKEYBOARDCUES,0,&bMenuUnderlines,0)==TRUE && bMenuUnderlines==TRUE)
        {
          // do not hide
          dt_Hide = 0;
        }
      }

      UINT nFormat  = DT_LEFT|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      UINT nFormatr = DT_RIGHT|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      if(nFlags&MFT_RIGHTORDER)
      {
        nFormat  = DT_RIGHT| DT_SINGLELINE|DT_VCENTER|DT_RTLREADING|dt_Hide;
        nFormatr = DT_LEFT|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      }

      if(bIsMenuBar)
      {
        rectt = RectSel;
        rectt.OffsetRect(-1,0);
        if(state & ODS_SELECTED)
        {
          rectt.OffsetRect(1,1);
        }
        nFormat = DT_CENTER|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      }
      else
      {
        if(nFlags&MFT_RIGHTORDER)
        {
          RectText.left += 15;
          RectText.right -= 4;
        }
        else
        {
          RectText.left += 4;
          RectText.right -= 15;
        }
      }

      CFont fontMenu;
      LOGFONT logFontMenu;

#ifdef _NEW_MENU_USER_FONT
      logFontMenu =  MENU_USER_FONT;
#else
      NONCLIENTMETRICS nm = {0};
      nm.cbSize = sizeof (nm);
      VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
      logFontMenu =  nm.lfMenuFont;
#endif

      // Default selection?
      if(state&ODS_DEFAULT)
      {
        // Make the font bold
        logFontMenu.lfWeight = FW_BOLD;
      }
      if(state&ODS_DRAW_VERTICAL)
      {
        // rotate font 90°
        logFontMenu.lfOrientation = -900;
        logFontMenu.lfEscapement = -900;
      }

      fontMenu.CreateFontIndirect (&logFontMenu);
      CFont* pOldFont = pDC->SelectObject(&fontMenu);

      if(!(lpDIS->itemState & ODS_GRAYED))
      {
        pDC->SetTextColor(crText);
        if(bIsMenuBar)
        {
          MenuDrawText(pDC->m_hDC,leftStr,-1,RectText,nFormat);
          if(tablocr!=-1)
          {
            MenuDrawText(pDC->m_hDC,rightStr,-1,RectText,nFormatr);
          }
        }
        else
        {
          pDC->DrawText (leftStr,RectText,nFormat);
          if(tablocr!=-1)
          {
            pDC->DrawText (rightStr,RectText,nFormatr);
          }
        }
      }
      else
      {
        // Draw the disabled text
        if(!(state & ODS_SELECTED))
        {
          CRect offset = RectText;
          offset.OffsetRect (1,1);

          pDC->SetTextColor(colorGrayed);
          pDC->DrawText(leftStr,RectText, nFormat);
          if(tablocr!=-1)
          {
            pDC->DrawText(rightStr,RectText,nFormatr);
          }
        }
        else
        {
          // And the standard Grey text:
          pDC->SetTextColor(colorBack);
          pDC->DrawText(leftStr,RectText, nFormat);
          if(tablocr!=-1)
          {
            pDC->DrawText (rightStr,RectText,nFormatr);
          }
        }
      }
      pDC->SelectObject(pOldFont);
      pDC->SetBkMode( iOldMode );
    }
    m_brSelect.DeleteObject();
  }
}

void CNewMenu::DrawItem_OldStyle (LPDRAWITEMSTRUCT lpDIS, BOOL bIsMenuBar)
{
  ASSERT(lpDIS != NULL);
  CRect rect;

  ASSERT(lpDIS->itemData);
  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpDIS->itemData);

  UINT nFlags = pMenuData->m_nFlags;

  CNewMemDC memDC(&lpDIS->rcItem,lpDIS->hDC);
  CDC* pDC;
  if( bIsMenuBar || (nFlags&MF_SEPARATOR) )
  { // For title and menubardrawing disable memory painting
    memDC.DoCancel();
    pDC = CDC::FromHandle(lpDIS->hDC);
  }
  else
  {
    pDC = &memDC;
  }

  COLORREF colorBack = bIsMenuBar?GetMenuBarColor():GetSysColor(COLOR_MENU);

  CRect RectIcon(lpDIS->rcItem);
  CRect RectText(lpDIS->rcItem);
  CRect RectSel(lpDIS->rcItem);

  RectIcon.InflateRect (-1,0,0,0);
  RectText.InflateRect (-1,0,0,0);
  RectSel.InflateRect (0,0,0,0);

  if(!bIsMenuBar)
  {
    if(nFlags&MFT_RIGHTORDER)
    {
      RectIcon.left = RectIcon.right - (m_iconX + 6 + GAP);
      RectText.right  = RectIcon.left;
    }
    else
    {
      RectIcon.right = RectIcon.left + m_iconX + 6 + GAP;
      RectText.left  = RectIcon.right;
    }
  }
  else
  {
    RectText.right += 6;
    RectText.InflateRect (1,0,0,0);
    RectSel.InflateRect (0,0,-2,0);
    RectSel.OffsetRect(1,-1);

#ifdef _TRACE_MENU_
    //   AfxTrace(_T("BarState: 0x%lX Menus %ld\n"),lpDIS->itemState,m_dwOpenMenu);
#endif
    if(!IsMenu(UIntToHMenu(lpDIS->itemID)) && (lpDIS->itemState&ODS_SELECTED_OPEN))
    {
      lpDIS->itemState = (lpDIS->itemState&~(ODS_SELECTED|ODS_SELECTED_OPEN))|ODS_HOTLIGHT;
    }
    else if( !(lpDIS->itemState&ODS_SELECTED_OPEN) && !m_dwOpenMenu && lpDIS->itemState&ODS_SELECTED)
    {
      lpDIS->itemState = (lpDIS->itemState&~ODS_SELECTED)|ODS_HOTLIGHT;
    }

    if(lpDIS->itemState&(ODS_SELECTED|ODS_HOTLIGHT))
    {
      SetLastMenuRect(lpDIS->hDC,RectSel);
    }
  }

  // For keyboard navigation only
  BOOL bDrawSmallSelection = FALSE;

  // remove the selected bit if it's grayed out
  if( (lpDIS->itemState&ODS_GRAYED) && !m_bSelectDisable )
  {
    if(lpDIS->itemState & ODS_SELECTED)
    {
      lpDIS->itemState &= ~ODS_SELECTED;
      DWORD MsgPos = ::GetMessagePos();
      if(MsgPos==CNewMenuHook::m_dwMsgPos)
      {
        bDrawSmallSelection = TRUE;
      }
      else
      {
        CNewMenuHook::m_dwMsgPos = MsgPos;
      }
    }
  }

  if(nFlags & MF_SEPARATOR)
  {
    if(pMenuData->m_nTitleFlags&MFT_TITLE)
    {
      DrawTitle(lpDIS,bIsMenuBar);
    }
    else
    {
      rect = lpDIS->rcItem;
      pDC->FillSolidRect(rect,colorBack);
      rect.left += 1;
      rect.top += 1;
      pDC->DrawEdge(&rect,EDGE_ETCHED,BF_TOP);
    }
  }
  else
  {
    CRect rect2;
    BOOL standardflag=FALSE,selectedflag=FALSE,disableflag=FALSE;
    BOOL checkflag=FALSE;

    CBrush m_brSelect;
    int nIconNormal=-1;
    //CImageList *bitmap=NULL;

    // set some colors and the font
    m_brSelect.CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));

    // draw the colored rectangle portion
    rect2=rect=RectSel;

    // draw the up/down/focused/disabled state
    UINT state = lpDIS->itemState;

    nIconNormal = pMenuData->m_nMenuIconOffset;

    if( (state&ODS_CHECKED) && nIconNormal<0)
    {
      if(state&ODS_SELECTED && m_selectcheck>0)
      {
        checkflag = TRUE;
      }
      else if(m_unselectcheck>0)
      {
        checkflag = TRUE;
      }
    }
    else if(nIconNormal != -1)
    {
      standardflag=TRUE;
      if(state&ODS_SELECTED && !(state&ODS_GRAYED))
      {
        selectedflag = TRUE;
      }
      else if(state&ODS_GRAYED)
      {
        disableflag = TRUE;
      }
    }

    if(bIsMenuBar)
    {
      //rect.InflateRect (1,0,0,0);
      rect.OffsetRect(-1,1);
      rect2=rect;
      rect.right +=2;
      pDC->FillSolidRect (rect,colorBack);
      rect.right -=2;
    }
    else
    {
      // Draw the background
      pDC->FillSolidRect (rect,colorBack);
    }
    // Draw the selection
    if(state&ODS_SELECTED)
    {
      // You need only Text highlight and that's what you get
      if(!bIsMenuBar)
      {
        if(checkflag||standardflag||selectedflag||disableflag||state&ODS_CHECKED)
        {
          rect2 = RectText;
        }
        pDC->FillRect(rect2,&m_brSelect);
      }
      else
      {
        pDC->Draw3dRect(rect ,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));
      }
    }
    else if(bIsMenuBar && (state&ODS_HOTLIGHT) && !(state&ODS_INACTIVE))
    {
      pDC->Draw3dRect(rect,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DSHADOW));
    }
    else if (bDrawSmallSelection)
    {
      pDC->DrawFocusRect(rect);
    }

    // Draw the Bitmap or checkmarks
    if(!bIsMenuBar)
    {
      CRect IconRect = RectIcon;
      // center the image
      IconRect.InflateRect(-(RectIcon.Width()-m_iconX)>>1,-(RectIcon.Height()-m_iconY)>>1);
      CPoint ptImage = IconRect.TopLeft();
      IconRect.InflateRect(2,2);

      if(checkflag||standardflag||selectedflag||disableflag)
      {
        if(checkflag && m_checkmaps)
        {
          if(state&ODS_SELECTED)
          {
            m_checkmaps->Draw(pDC,1,ptImage,ILD_TRANSPARENT);
          }
          else
          {
            m_checkmaps->Draw(pDC,0,ptImage,ILD_TRANSPARENT);
          }
        }
        else
        {
          // Need to draw the checked state
          if (IsNewShell())
          {
            if(state&ODS_CHECKED)
            {
              pDC->Draw3dRect(IconRect,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));
            }
            else if (selectedflag)
            {
              pDC->Draw3dRect(IconRect,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DSHADOW));
            }
          }

          CSize size = pMenuData->m_pMenuIcon->GetIconSize();
          // Correcting of a smaler icon
          if(size.cx<m_iconX)
          {
            ptImage.x += (m_iconX-size.cx)>>1;
          }
          if(size.cy<m_iconY)
          {
            ptImage.y += (m_iconY-size.cy)>>1;
          }

          HICON hDrawIcon = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset);
          if(state & ODS_DISABLED)
          {
            // Jan-19-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
            // opted to use blended disabled icons instead, the default is just too bad to think about using
            HICON hDrawIcon2 = pMenuData->m_pMenuIcon->m_IconsList.ExtractIcon(pMenuData->m_nMenuIconOffset+2);
            pDC->DrawState(ptImage, size, hDrawIcon2, DSS_NORMAL,(HBRUSH)NULL);
            DestroyIcon(hDrawIcon2);
//          pDC->DrawState(ptImage, size, hDrawIcon, DSS_DISABLED, (HBRUSH)NULL);
          }
          else
          {
            pDC->DrawState(ptImage, size, hDrawIcon, DSS_NORMAL, (HBRUSH)NULL);
          }
          DestroyIcon(hDrawIcon);
        }
      }
      if ((lpDIS->itemID&0xffff)>=SC_SIZE && (lpDIS->itemID&0xffff)<=SC_HOTKEY )
      {
        DrawSpecial_OldStyle(pDC,IconRect,lpDIS->itemID,state);
      }
      else if(nIconNormal<0 /*&& state&ODS_CHECKED */ && !checkflag)
      {
        MENUITEMINFO info = {0};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_CHECKMARKS;

        ::GetMenuItemInfo(HWndToHMenu(lpDIS->hwndItem),lpDIS->itemID,MF_BYCOMMAND, &info);
        Draw3DCheckmark(pDC, IconRect,info.hbmpChecked,info.hbmpUnchecked,state);
      }
    }

    CString strText;
    strText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
    if(!strText.IsEmpty())
    {
      COLORREF crText = GetSysColor(COLOR_MENUTEXT);

      if(bIsMenuBar)
      {
        if(lpDIS->itemState&ODS_INACTIVE)
          crText = GetSysColor(COLOR_GRAYTEXT);
      }
      else
      {
        if(lpDIS->itemState&ODS_SELECTED)
        {
          crText = GetSysColor(COLOR_HIGHLIGHTTEXT);
        }
      }
      // Find tabs
      CString leftStr,rightStr;
      leftStr.Empty();rightStr.Empty();

      int tablocr = strText.ReverseFind(_T('\t'));
      if(tablocr!=-1)
      {
        rightStr = strText.Mid(tablocr+1);
        leftStr = strText.Left(strText.Find(_T('\t')));
      }
      else
      {
        leftStr = strText;
      }

      int iOldMode = pDC->SetBkMode( TRANSPARENT);
      // Draw the text in the correct colour:
      UINT dt_Hide = (lpDIS->itemState & ODS_NOACCEL)?DT_HIDEPREFIX:0;
      if(dt_Hide && g_Shell>=Win2000)
      {
        BOOL bMenuUnderlines = TRUE;
        if(SystemParametersInfo( SPI_GETKEYBOARDCUES,0,&bMenuUnderlines,0)==TRUE && bMenuUnderlines==TRUE)
        {
          // do not hide
          dt_Hide = 0;
        }
      }
      UINT nFormat  = DT_LEFT|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      UINT nFormatr = DT_RIGHT|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      if(nFlags&MFT_RIGHTORDER)
      {
        nFormat  = DT_RIGHT| DT_SINGLELINE|DT_VCENTER|DT_RTLREADING|dt_Hide;
        nFormatr = DT_LEFT|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      }

      if(bIsMenuBar)
      {
        if(state & ODS_SELECTED)
        {
          RectText.OffsetRect(1,1);
        }
        nFormat = DT_CENTER|DT_SINGLELINE|DT_VCENTER|dt_Hide;
      }
      else
      {
        if(nFlags&MFT_RIGHTORDER)
        {
          RectText.left += 15;
          RectText.right -= 4;
        }
        else
        {
          RectText.left += 4;
          RectText.right -= 15;
        }
      }
      CFont fontMenu;
      LOGFONT logFontMenu;

#ifdef _NEW_MENU_USER_FONT
      logFontMenu =  MENU_USER_FONT;
#else
      NONCLIENTMETRICS nm = {0};
      nm.cbSize = sizeof (nm);
      VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
      logFontMenu =  nm.lfMenuFont;
#endif

      // Default selection?
      if(state&ODS_DEFAULT)
      {
        // Make the font bold
        logFontMenu.lfWeight = FW_BOLD;
      }
      if(state&ODS_DRAW_VERTICAL)
      {
        // rotate font 90°
        logFontMenu.lfOrientation = -900;
        logFontMenu.lfEscapement = -900;
      }

      fontMenu.CreateFontIndirect (&logFontMenu);
      CFont* pOldFont = pDC->SelectObject(&fontMenu);

      if(!(lpDIS->itemState & ODS_GRAYED))
      {
        pDC->SetTextColor(crText);
        if(bIsMenuBar)
        {
          MenuDrawText(pDC->m_hDC,leftStr,-1,RectText,nFormat);
          if(tablocr!=-1)
          {
            MenuDrawText(pDC->m_hDC,rightStr,-1,RectText,nFormatr);
          }
        }
        else
        {
          pDC->DrawText (leftStr,RectText,nFormat);
          if(tablocr!=-1)
          {
            pDC->DrawText (rightStr,RectText,nFormatr);
          }
        }
      }
      else
      {
        // Draw the disabled text
        if(!(state & ODS_SELECTED))
        {
          CRect offset = RectText;
          offset.OffsetRect (1,1);

          pDC->SetTextColor(GetSysColor(COLOR_BTNHILIGHT));
          pDC->DrawText(leftStr,&offset, nFormat);
          if(tablocr!=-1)
          {
            pDC->DrawText (rightStr,&offset,nFormatr);
          }

          pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
          pDC->DrawText(leftStr,RectText, nFormat);
          if(tablocr!=-1)
          {
            pDC->DrawText(rightStr,RectText,nFormatr);
          }
        }
        else
        {
          // And the standard Grey text:
          pDC->SetTextColor(colorBack);
          pDC->DrawText(leftStr,RectText, nFormat);
          if(tablocr!=-1)
          {
            pDC->DrawText (rightStr,RectText,nFormatr);
          }
        }
      }
      pDC->SelectObject(pOldFont);
      pDC->SetBkMode( iOldMode );
    }
    m_brSelect.DeleteObject();
  }
}

BOOL CNewMenu::IsMenuBar(HMENU hMenu)
{
  BOOL bIsMenuBar = FALSE;
  if(!FindMenuItem(HMenuToUInt(hMenu)))
  {
    if(m_hParentMenu==NULL)
    {
      return m_bIsPopupMenu?FALSE:TRUE;
    }
    CNewMenu* pMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(m_hParentMenu));
    if (pMenu!=NULL)
    {
      return pMenu->m_bIsPopupMenu?FALSE:TRUE;
    }
  }
  else
  {
    // Test for only one item not bar
    bIsMenuBar = m_hParentMenu ? FALSE: ((m_bIsPopupMenu)?FALSE:TRUE);
  }
  return bIsMenuBar;
}

/*
==========================================================================
void CNewMenu::MeasureItem(LPMEASUREITEMSTRUCT)
---------------------------------------------

Called by the framework when it wants to know what the width and height
of our item will be.  To accomplish this we provide the width of the
icon plus the width of the menu text, and then the height of the icon.

==========================================================================
*/
void CNewMenu::MeasureItem_OldStyle( LPMEASUREITEMSTRUCT lpMIS, BOOL bIsMenuBar )
{
  ASSERT(lpMIS->itemData);
  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpMIS->itemData);

  UINT state = pMenuData->m_nFlags;
  if(state & MF_SEPARATOR)
  {
    if(pMenuData->m_nTitleFlags&MFT_TITLE)
    {
      SIZE size = {0,0};
      {// We need this sub-block for releasing the dc
        // DC of the desktop
        CClientDC myDC(NULL);

        CFont font;
        LOGFONT MyFont = m_MenuTitleFont;
        MyFont.lfOrientation = 0;
        MyFont.lfEscapement = 0;
        font.CreateFontIndirect(&MyFont);

        CFont* pOldFont = myDC.SelectObject (&font);
        CString lpstrText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
        VERIFY(::GetTextExtentPoint32(myDC.m_hDC,lpstrText,(int)_tcslen(lpstrText),&size));
        // Select old font in
        myDC.SelectObject(pOldFont);
      } // We need this sub-block for releasing the dc

      CSize iconSize(0,0);
      if(pMenuData->m_nMenuIconOffset!=(-1) && pMenuData->m_pMenuIcon)
      {
        iconSize = pMenuData->m_pMenuIcon->GetIconSize();
        if(iconSize!=CSize(0,0))
        {
          iconSize += CSize(2,2);
        }
      }

      if(pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
      {
        if(bWine)
        {
          lpMIS->itemWidth = max(size.cy,iconSize.cx);
        }
        else
        {
          lpMIS->itemWidth = max(size.cy,iconSize.cx) - GetSystemMetrics(SM_CXMENUCHECK);
        }
        // Don't make the menu higher than menuitems in it
        lpMIS->itemHeight = 0;
        if(pMenuData->m_nTitleFlags&MFT_LINE)
        {
          lpMIS->itemWidth += 8;
        }
        else if(pMenuData->m_nTitleFlags&MFT_ROUND)
        {
          lpMIS->itemWidth += 4;
        }
      }
      else
      {
        lpMIS->itemWidth = size.cx + iconSize.cx;
        lpMIS->itemHeight = max(size.cy,iconSize.cy);
        if(pMenuData->m_nTitleFlags&MFT_LINE)
        {
          lpMIS->itemHeight += 8;
        }
      }
    }
    else
    {
      lpMIS->itemHeight = 3;
      lpMIS->itemWidth = 3;
    }
  }
  else
  {
    //Get pointer to text SK
    CString itemText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
    SIZE size = {0,0};

    {// We need this sub-block for releasing the dc
      CFont fontMenu;
      LOGFONT logFontMenu;

  #ifdef _NEW_MENU_USER_FONT
      logFontMenu =  MENU_USER_FONT;
  #else
      NONCLIENTMETRICS nm = {0};
      nm.cbSize = sizeof (nm);
      VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
      logFontMenu =  nm.lfMenuFont;
  #endif

      // Default selection?
      if(GetDefaultItem(0, FALSE) == pMenuData->m_nID)
      {
        // Make the font bold
        logFontMenu.lfWeight = FW_BOLD;
      }
      fontMenu.CreateFontIndirect (&logFontMenu);

      // DC of the desktop
      CClientDC myDC(NULL);
      // Select menu font in...
      CFont* pOldFont = myDC.SelectObject (&fontMenu);
      VERIFY(::GetTextExtentPoint32(myDC.m_hDC,itemText,itemText.GetLength(),&size));
      // Select old font in
      myDC.SelectObject(pOldFont);
    } // We need this sub-block for releasing the dc


    // Set width and height:
    if(bIsMenuBar)
    {
      if(itemText.Find(_T("&"))>=0)
      {
        lpMIS->itemWidth = size.cx - GetSystemMetrics(SM_CXMENUCHECK)/2;
      }
      else
      {
        lpMIS->itemWidth = size.cx;
      }
    }
    else
    {
      lpMIS->itemWidth = m_iconX + size.cx + m_iconX + GAP;
    }

    int temp = GetSystemMetrics(SM_CYMENU);
    lpMIS->itemHeight = (temp>(m_iconY+4)) ? temp : (m_iconY+4);
    if(lpMIS->itemHeight<((UINT)size.cy) )
    {
      lpMIS->itemHeight=((UINT)size.cy);
    }

    // set status bar as appropriate
    UINT nItemID = (lpMIS->itemID & 0xFFF0);
    // Special case for system menu
    if (nItemID>=SC_SIZE && nItemID<=SC_HOTKEY)
    {
      BOOL bGetNext = FALSE;
      // search the actual menu item
      for (int j=0;j<m_MenuItemList.GetUpperBound();++j)
      {
        CNewMenuItemData* pTemp = m_MenuItemList[j];
        if(pTemp==pMenuData || bGetNext==TRUE)
        {
          bGetNext = TRUE;
          pTemp = m_MenuItemList[j+1];
          if(pTemp->m_nID)
          {
            lpMIS->itemData = (ULONG_PTR)pTemp;
            lpMIS->itemID = pTemp->m_nID;
            UINT nOrgWidth = lpMIS->itemWidth;
            MeasureItem_OldStyle(lpMIS,bIsMenuBar);
            // Restore old values
            lpMIS->itemData = (ULONG_PTR)pMenuData;
            lpMIS->itemID = pMenuData->m_nID;
            lpMIS->itemWidth = max(lpMIS->itemWidth,nOrgWidth);
            break;
          }
        }
      }
      lpMIS->itemHeight = temp;
    }
  }
}

void CNewMenu::MeasureItem_Icy( LPMEASUREITEMSTRUCT lpMIS, BOOL bIsMenuBar )
{
  ASSERT(lpMIS->itemData);
  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpMIS->itemData);

  UINT state = pMenuData->m_nFlags;
  if(state & MF_SEPARATOR)
  {
    if(pMenuData->m_nTitleFlags&MFT_TITLE)
    {
      SIZE size = {0,0};
      {// We need this sub-block for releasing the dc
        // DC of the desktop
        CClientDC myDC(NULL);

        CFont font;
        LOGFONT MyFont = m_MenuTitleFont;
        MyFont.lfOrientation = 0;
        MyFont.lfEscapement = 0;
        font.CreateFontIndirect(&MyFont);

        CFont* pOldFont = myDC.SelectObject (&font);
        CString lpstrText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
        VERIFY(::GetTextExtentPoint32(myDC.m_hDC,lpstrText,(int)_tcslen(lpstrText),&size));
        // Select old font in
        myDC.SelectObject(pOldFont);
      } // We need this sub-block for releasing the dc

      CSize iconSize(0,0);
      if(pMenuData->m_nMenuIconOffset!=(-1) && pMenuData->m_pMenuIcon)
      {
        iconSize = pMenuData->m_pMenuIcon->GetIconSize();
        if(iconSize!=CSize(0,0))
        {
          iconSize += CSize(2,2);
        }
      }

      if(pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
      {
        if(bWine)
        {
          lpMIS->itemWidth = max(size.cy,iconSize.cx);
        }
        else
        {
          lpMIS->itemWidth = max(size.cy,iconSize.cx) - GetSystemMetrics(SM_CXMENUCHECK);
        }
        // Don't make the menu higher than menuitems in it
        lpMIS->itemHeight = 0;
        if(pMenuData->m_nTitleFlags&MFT_LINE)
        {
          lpMIS->itemWidth += 8;
        }
        else if(pMenuData->m_nTitleFlags&MFT_ROUND)
        {
          lpMIS->itemWidth += 4;
        }
      }
      else
      {
        lpMIS->itemWidth = size.cx + iconSize.cx;
        lpMIS->itemHeight = max(size.cy,iconSize.cy);
        if(pMenuData->m_nTitleFlags&MFT_LINE)
        {
          lpMIS->itemHeight += 8;
        }
      }
    }
    else
    {
      lpMIS->itemHeight = 3;
      lpMIS->itemWidth = 3;
    }
  }
  else
  {
    //Get pointer to text SK
    CString itemText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
    SIZE size = {0,0};
    {// We need this sub-block for releasing the dc
      LOGFONT logFontMenu;
      CFont fontMenu;

  #ifdef _NEW_MENU_USER_FONT
      logFontMenu =  MENU_USER_FONT;
  #else
      NONCLIENTMETRICS nm = {0};
      nm.cbSize = sizeof (nm);
      VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
      logFontMenu =  nm.lfMenuFont;
  #endif

      // Default selection?
      if(GetDefaultItem(0, FALSE) == pMenuData->m_nID)
      {
        // Make the font bold
        logFontMenu.lfWeight = FW_BOLD;
      }

      fontMenu.CreateFontIndirect (&logFontMenu);

      // DC of the desktop
      CClientDC myDC(NULL);
      // Select menu font in...
      CFont* pOldFont = myDC.SelectObject (&fontMenu);
      // Check the Key-Shortcut replacing for japanise/chinese calculating space
      itemText.Replace(_T("\t"),_T("nnn"));
      VERIFY(::GetTextExtentPoint32(myDC.m_hDC,itemText,itemText.GetLength(),&size));
      // Select old font in
      myDC.SelectObject(pOldFont);
    }// We need this sub-block for releasing the dc

    // Set width and height:
    if(bIsMenuBar)
    {
      if(itemText.Find(_T("&"))>=0)
      {
        lpMIS->itemWidth = size.cx - GetSystemMetrics(SM_CXMENUCHECK)/2;
      }
      else
      {
        lpMIS->itemWidth = size.cx;
      }
    }
    else
    {
      lpMIS->itemWidth = m_iconX + size.cx + m_iconX + GAP;
    }

    int temp = GetSystemMetrics(SM_CYMENU);
    lpMIS->itemHeight = (temp>(m_iconY + 6)) ? temp : (m_iconY+6);
    if(lpMIS->itemHeight<((UINT)size.cy) )
    {
      lpMIS->itemHeight=((UINT)size.cy);
    }

    // set status bar as appropriate
    UINT nItemID = (lpMIS->itemID & 0xFFF0);
    // Special case for system menu
    if (nItemID>=SC_SIZE && nItemID<=SC_HOTKEY)
    {
      BOOL bGetNext = FALSE;
      // search the actual menu item
      for (int j=0;j<m_MenuItemList.GetUpperBound();++j)
      {
        CNewMenuItemData* pTemp = m_MenuItemList[j];
        if(pTemp==pMenuData || bGetNext==TRUE)
        {
          bGetNext = TRUE;
          pTemp = m_MenuItemList[j+1];
          if(pTemp->m_nID)
          {
            lpMIS->itemData = (ULONG_PTR)pTemp;
            lpMIS->itemID = pTemp->m_nID;
            UINT nOrgWidth = lpMIS->itemWidth;
            MeasureItem_Icy(lpMIS,bIsMenuBar);
            // Restore old values
            lpMIS->itemData = (ULONG_PTR)pMenuData;
            lpMIS->itemID = pMenuData->m_nID;
            lpMIS->itemWidth = max(lpMIS->itemWidth,nOrgWidth);
            break;
          }
        }
      }
      lpMIS->itemHeight = temp;
    }
  }
}

void CNewMenu::MeasureItem_WinXP( LPMEASUREITEMSTRUCT lpMIS, BOOL bIsMenuBar )
{
  ASSERT(lpMIS->itemData);
  CNewMenuItemData* pMenuData = (CNewMenuItemData*)(lpMIS->itemData);

  UINT nFlag = pMenuData->m_nFlags;
  if(nFlag & MF_SEPARATOR)
  {
    if(pMenuData->m_nTitleFlags&MFT_TITLE)
    {
      SIZE size = {0,0};
      {// We need this sub-block for releasing the dc
        // DC of the desktop
        CClientDC myDC(NULL);

        CFont font;
        LOGFONT MyFont = m_MenuTitleFont;
        MyFont.lfOrientation = 0;
        MyFont.lfEscapement = 0;
        font.CreateFontIndirect(&MyFont);

        CFont* pOldFont = myDC.SelectObject (&font);
        CString lpstrText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
        VERIFY(::GetTextExtentPoint32(myDC.m_hDC,lpstrText,(int)_tcslen(lpstrText),&size));
        // Select old font in
        myDC.SelectObject(pOldFont);
      }

      CSize iconSize(0,0);
      if(pMenuData->m_nMenuIconOffset!=(-1) && pMenuData->m_pMenuIcon)
      {
        iconSize = pMenuData->m_pMenuIcon->GetIconSize();
        if(iconSize!=CSize(0,0))
        {
          iconSize += CSize(2,2);
        }
      }

      if(pMenuData->m_nTitleFlags&MFT_SIDE_TITLE)
      {
        if(bWine)
        {
          lpMIS->itemWidth = max(size.cy,iconSize.cx);
        }
        else
        {
          lpMIS->itemWidth = max(size.cy,iconSize.cx) - GetSystemMetrics(SM_CXMENUCHECK);
        }
        // Don't make the menu higher than menuitems in it
        lpMIS->itemHeight = 0;
        if(pMenuData->m_nTitleFlags&MFT_LINE)
        {
          lpMIS->itemWidth += 8;
        }
        else if(pMenuData->m_nTitleFlags&MFT_ROUND)
        {
          lpMIS->itemWidth += 4;
        }
      }
      else
      {
        lpMIS->itemWidth = size.cx + iconSize.cx;
        lpMIS->itemHeight = max(size.cy,iconSize.cy);
        if(pMenuData->m_nTitleFlags&MFT_LINE)
        {
          lpMIS->itemHeight += 8;
        }
      }
    }
    else
    {
      lpMIS->itemHeight = 3;
      lpMIS->itemWidth = 3;
    }
  }
  else
  {
    SIZE size = {0,0};
    //Get pointer to text SK
    CString itemText = pMenuData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
    { // We need this sub-block for releasing the dc
      CFont fontMenu;
      LOGFONT logFontMenu;

  #ifdef _NEW_MENU_USER_FONT
      logFontMenu =  MENU_USER_FONT;
  #else
      NONCLIENTMETRICS nm = {0};
      nm.cbSize = sizeof (NONCLIENTMETRICS);
      VERIFY (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,nm.cbSize,&nm,0));
      logFontMenu =  nm.lfMenuFont;
  #endif

      // Default selection?
      if (GetDefaultItem(0, FALSE) == pMenuData->m_nID)
      {
        // Make the font bold
        logFontMenu.lfWeight = FW_BOLD;
      }

      fontMenu.CreateFontIndirect (&logFontMenu);

       // DC of the desktop
      CClientDC myDC(NULL);
      // Select menu font in...
      CFont* pOldFont = myDC.SelectObject (&fontMenu);
      // Check the Key-Shortcut replacing for japanise/chinese calculating space
      itemText.Replace(_T("\t"),_T("nnn"));
      VERIFY(::GetTextExtentPoint32(myDC.m_hDC,itemText,itemText.GetLength(),&size));
      // Select old font in
      myDC.SelectObject(pOldFont);
    }// We need this sub-block for releasing the dc

    int temp = GetSystemMetrics(SM_CYMENU);
    // Set width and height:
    if(bIsMenuBar)
    {
      if(itemText.Find(_T("&"))>=0)
      {
        lpMIS->itemWidth = size.cx-6; // - GetSystemMetrics(SM_CXMENUCHECK)/2;
      }
      else
      {
        lpMIS->itemWidth = size.cx;
      }
      lpMIS->itemHeight = temp >(m_iconY+2) ? temp : m_iconY+2;
    }
    else
    {
      if(nFlag&MF_POPUP)
      {
        lpMIS->itemWidth = 2 + m_iconX + 4 + size.cx + GetSystemMetrics(SM_CYMENU);
      }
      else
      {
      lpMIS->itemWidth = 2 + m_iconX +4+ size.cx + GetSystemMetrics(SM_CYMENU) / 2;
      }
      lpMIS->itemHeight = temp>m_iconY+8 ? temp : m_iconY+7;
    }

    if(lpMIS->itemHeight<((UINT)size.cy) )
    {
      lpMIS->itemHeight=((UINT)size.cy);
    }

    // set status bar as appropriate
    UINT nItemID = (lpMIS->itemID & 0xFFF0);
    // Special case for system menu
    if (nItemID>=SC_SIZE && nItemID<=SC_HOTKEY)
    {
      BOOL bGetNext = FALSE;
      // search the actual menu item
      for (int j=0;j<m_MenuItemList.GetUpperBound();++j)
      {
        CNewMenuItemData* pTemp = m_MenuItemList[j];
        if(pTemp==pMenuData || bGetNext==TRUE)
        {
          bGetNext = TRUE;
          pTemp = m_MenuItemList[j+1];
          if(pTemp->m_nID)
          {
            lpMIS->itemData = (ULONG_PTR)pTemp;
            lpMIS->itemID = pTemp->m_nID;
            UINT nOrgWidth = lpMIS->itemWidth;
            MeasureItem_WinXP(lpMIS,bIsMenuBar);
            // Restore old values
            lpMIS->itemData = (ULONG_PTR)pMenuData;
            lpMIS->itemID = pMenuData->m_nID;
            lpMIS->itemWidth = max(lpMIS->itemWidth,nOrgWidth);
            break;
          }
        }
      }
      lpMIS->itemHeight = temp;
    }
  }
  //AfxTrace("IsMenuBar %ld, Height %2ld, width %3ld, ID 0x%08lX \r\n",int(bIsMenuBar),lpMIS->itemHeight,lpMIS->itemWidth,lpMIS->itemID);
}

void CNewMenu::SetIconSize (int width, int height)
{
  m_iconX = width;
  m_iconY = height;
}

CSize CNewMenu::GetIconSize()
{
  return CSize(m_iconX,m_iconY);
}

BOOL CNewMenu::AppendODMenu(LPCTSTR lpstrText, UINT nFlags, UINT nID, int nIconNormal)
{
  return CNewMenu::AppendODMenu(lpstrText,nFlags,nID,(CImageList*)NULL,nIconNormal);
}

BOOL CNewMenu::AppendODMenu(LPCTSTR lpstrText, UINT nFlags, UINT nID,
                            CBitmap* pBmp)
{
  int nIndex = -1;
  CNewMenuIconLock iconLock(GetMenuIcon(nIndex,pBmp));
  return AppendODMenu(lpstrText,nFlags,nID,iconLock,nIndex);
}

BOOL CNewMenu::AppendODMenu(LPCTSTR lpstrText, UINT nFlags, UINT nID,
                            CImageList* pil, int xoffset)
{
  int nIndex = 0;
  // Helper for addref and release
  CNewMenuIconLock iconLock(GetMenuIcon(nIndex,nID,pil,xoffset));
  return AppendODMenu(lpstrText,nFlags,nID,iconLock,nIndex);
}

BOOL CNewMenu::AppendODMenu(LPCTSTR lpstrText, UINT nFlags, UINT nID,
                            CNewMenuIcons* pIcons, int nIndex)
{
  // Add the MF_OWNERDRAW flag if not specified:
  if(!nID)
  {
    if(nFlags&MF_BYPOSITION)
      nFlags=MF_SEPARATOR|MF_OWNERDRAW|MF_BYPOSITION;
    else
      nFlags=MF_SEPARATOR|MF_OWNERDRAW;
  }
  else if(!(nFlags & MF_OWNERDRAW))
  {
    nFlags |= MF_OWNERDRAW;
  }

  if(nFlags & MF_POPUP)
  {
    CNewMenu* pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(UIntToHMenu(nID)));
    if(pSubMenu)
    {
      pSubMenu->m_hParentMenu = m_hMenu;
    }
  }

  CNewMenuItemData* pItemData = new CNewMenuItemData;
  m_MenuItemList.Add(pItemData);
  pItemData->SetString(lpstrText);

  pIcons->AddRef();
  pItemData->m_pMenuIcon->Release();
  pItemData->m_pMenuIcon = pIcons;
  pItemData->m_nFlags = nFlags;
  pItemData->m_nID = nID;

  if(pIcons && nIndex>=0)
  {
    pItemData->m_nMenuIconOffset = nIndex;
    CSize size = pIcons->GetIconSize();
    m_iconX = max(m_iconX,size.cx);
    m_iconY = max(m_iconY,size.cy);
  }
  else
  {
    pItemData->m_nMenuIconOffset = -1;
  }

  // for having automated shortcut handling, thank to Mehdy Bohlool
  if (CMenu::AppendMenu(nFlags&~MF_OWNERDRAW, nID, pItemData->m_szMenuText))
  {
    return CMenu::ModifyMenu( CMenu::GetMenuItemCount()-1, MF_BYPOSITION| nFlags, nID, (LPCTSTR)pItemData );
  }
  return FALSE;
}

BOOL CNewMenu::InsertODMenu(UINT nPosition, LPCTSTR lpstrText, UINT nFlags, UINT nID,
                            int nIconNormal)
{
  int nIndex = -1;
  CNewMenuIcons* pIcons=NULL;

  if(nIconNormal>=0)
  {
    if(LoadFromToolBar(nID,nIconNormal,nIndex))
    {
      // the nIconNormal is a toolbar
      pIcons = GetToolbarIcons(nIconNormal);
      if(pIcons)
      {
        nIndex = pIcons->FindIndex(nID);
      }
    }
    else
    {
      // the nIconNormal is a bitmap
      pIcons = GetMenuIcon(nIndex,nIconNormal);
    }
  }

  CNewMenuIconLock iconLock(pIcons);
  return InsertODMenu(nPosition,lpstrText,nFlags,nID,iconLock,nIndex);
}

BOOL CNewMenu::InsertODMenu(UINT nPosition, LPCTSTR lpstrText, UINT nFlags, UINT nID,
                            CBitmap* pBmp)
{
  int nIndex = -1;
  CNewMenuIconLock iconLock(GetMenuIcon(nIndex,pBmp));
  return InsertODMenu(nPosition,lpstrText,nFlags,nID,iconLock,nIndex);
}

BOOL CNewMenu::InsertODMenu(UINT nPosition, LPCTSTR lpstrText, UINT nFlags, UINT nID,
                            CImageList *pil, int xoffset)
{
  int nIndex = -1;
  CNewMenuIconLock iconLock(GetMenuIcon(nIndex,nID,pil,xoffset));
  return InsertODMenu(nPosition,lpstrText,nFlags,nID,iconLock,nIndex);
}

BOOL CNewMenu::InsertODMenu(UINT nPosition, LPCTSTR lpstrText, UINT nFlags, UINT nID,
                            CNewMenuIcons* pIcons, int nIndex)
{
  if(!(nFlags & MF_BYPOSITION))
  {
    int iPosition =0;
    CNewMenu* pMenu = FindMenuOption(nPosition,iPosition);
    if(pMenu)
    {
      return(pMenu->InsertODMenu(iPosition,lpstrText,nFlags|MF_BYPOSITION,nID,pIcons,nIndex));
    }
    else
    {
      return(FALSE);
    }
  }

  if(!nID)
  {
    nFlags=MF_SEPARATOR|MF_OWNERDRAW|MF_BYPOSITION;
  }
  else if(!(nFlags & MF_OWNERDRAW))
  {
    nFlags |= MF_OWNERDRAW;
  }

  if(nFlags & MF_POPUP)
  {
    CNewMenu* pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(UIntToHMenu(nID)));
    if(pSubMenu)
    {
      pSubMenu->m_hParentMenu = m_hMenu;
    }
  }

  //Stephane Clog suggested adding this, believe it or not it's in the help
  if(nPosition==(UINT)-1)
  {
    nPosition = GetMenuItemCount();
  }

  CNewMenuItemData *pItemData = new CNewMenuItemData;
  m_MenuItemList.InsertAt(nPosition,pItemData);
  pItemData->SetString(lpstrText);

  pIcons->AddRef();
  pItemData->m_pMenuIcon->Release();
  pItemData->m_pMenuIcon = pIcons;
  pItemData->m_nFlags = nFlags;
  pItemData->m_nID = nID;

  if(pIcons && nIndex>=0)
  {
    pItemData->m_nMenuIconOffset = nIndex;
    CSize size = pIcons->GetIconSize();
    m_iconX = max(m_iconX,size.cx);
    m_iconY = max(m_iconY,size.cy);
  }
  else
  {
    pItemData->m_nMenuIconOffset = -1;
  }
  // for having automated shortcut handling, thank to Mehdy Bohlool
  if (CMenu::InsertMenu(nPosition,nFlags&~MF_OWNERDRAW,nID,pItemData->m_szMenuText))
  {
    return CMenu::ModifyMenu(nPosition, MF_BYPOSITION| nFlags, nID, (LPCTSTR)pItemData );
  }
  return FALSE;
}

// Same as ModifyMenu but replacement for CNewMenu
BOOL CNewMenu::ModifyODMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem,LPCTSTR lpszNewItem)
{
  if(!(nFlags & MF_BYPOSITION))
  {
    int iPosition =0;
    CNewMenu* pMenu = FindMenuOption(nPosition,iPosition);
    if(pMenu)
    {
      return(pMenu->ModifyODMenu(iPosition,nFlags|MF_BYPOSITION,nIDNewItem,lpszNewItem));
    }
    else
    {
      return(FALSE);
    }
  }
  UINT nMenuID = GetMenuItemID(nPosition);
  if(nMenuID==(UINT)-1)
  {
    nMenuID = HMenuToUInt(::GetSubMenu(m_hMenu,nPosition));
  }
  CNewMenuItemData* pItemData = FindMenuItem(nMenuID);
  BOOL bRet = CMenu::ModifyMenu(nPosition, nFlags, nIDNewItem,lpszNewItem);
  if(pItemData)
  {
    pItemData->m_nID = nIDNewItem;
    GetMenuString(nPosition,pItemData->m_szMenuText,MF_BYPOSITION);
    CMenu::ModifyMenu(nPosition, nFlags|MF_OWNERDRAW, nIDNewItem,(LPCTSTR)pItemData);
  }
  if(bRet && (nFlags & MF_POPUP) )
  {
    CNewMenu* pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(UIntToHMenu(nIDNewItem)));
    if(pSubMenu)
    {
      pSubMenu->m_hParentMenu = m_hMenu;
    }
  }
  return bRet;
}

BOOL CNewMenu::ModifyODMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, const CBitmap* pBmp)
{
  if(!(nFlags & MF_BYPOSITION))
  {
    int iPosition =0;
    CNewMenu* pMenu = FindMenuOption(nPosition,iPosition);
    if(pMenu)
    {
      return(pMenu->ModifyODMenu(iPosition,nFlags|MF_BYPOSITION,nIDNewItem,pBmp));
    }
    else
    {
      return(FALSE);
    }
  }
  UINT nMenuID = GetMenuItemID(nPosition);
  if(nMenuID==(UINT)-1)
  {
    nMenuID = HMenuToUInt(::GetSubMenu(m_hMenu,nPosition));
  }
  CNewMenuItemData* pItemData = FindMenuItem(nMenuID);
  BOOL bRet = CMenu::ModifyMenu(nPosition, nFlags, nIDNewItem,pBmp);
  if(pItemData)
  {
    pItemData->m_nID = nIDNewItem;
    pItemData->m_szMenuText.Empty();
    CMenu::ModifyMenu(nPosition, nFlags|MF_OWNERDRAW, nIDNewItem,(LPCTSTR)pItemData);
  }
  if(bRet && (nFlags & MF_POPUP) )
  {
    CNewMenu* pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(UIntToHMenu(nIDNewItem)));
    if(pSubMenu)
    {
      pSubMenu->m_hParentMenu = m_hMenu;
    }
  }
  return bRet;
}

BOOL CNewMenu::ModifyODMenu(LPCTSTR lpstrText, UINT nID, int nIconNormal)
{
  int nLoc;
  CNewMenuItemData* pItemData;
  CArray<CNewMenu*,CNewMenu*>newSubs;
  CArray<int,int&>newLocs;

  BOOL bModifyOK = TRUE;

  // Find the old CNewMenuItemData structure:
  CNewMenu* pSubMenu = FindMenuOption(nID,nLoc);
  do {
    if(pSubMenu && nLoc>=0)
    {
      pItemData = pSubMenu->m_MenuItemList[nLoc];
    }
    else
    {
      // Create a new CNewMenuItemData structure:
      pItemData = new CNewMenuItemData;
      m_MenuItemList.Add(pItemData);
    }

    BOOL bTextChanged = FALSE;
    ASSERT(pItemData);
    if(lpstrText && pItemData->m_szMenuText.Compare(lpstrText)!=NULL)
    {
      bTextChanged = TRUE;
      pItemData->SetString(lpstrText);
    }

    pItemData->m_nMenuIconOffset=-1;
    if(nIconNormal>=0)
    {
      int nxOffset = -1;
      CNewMenuIcons* pIcons=NULL;
      if(LoadFromToolBar(nID,nIconNormal,nxOffset))
      {
        // the nIconNormal is a toolbar
        pIcons = GetToolbarIcons(nIconNormal);
        if(pIcons)
        {
          pItemData->m_nMenuIconOffset = pIcons->FindIndex(nID);
        }
      }
      else
      {
        // the nIconNormal is a bitmap
        pIcons = GetMenuIcon(pItemData->m_nMenuIconOffset,nIconNormal);
      }
      pIcons->AddRef();
      pItemData->m_pMenuIcon->Release();
      pItemData->m_pMenuIcon = pIcons;
      if(pIcons)
      {
        CSize size = pIcons->GetIconSize();
        pSubMenu->m_iconX = max(pSubMenu->m_iconX,size.cx);
        pSubMenu->m_iconY = max(pSubMenu->m_iconY,size.cy);
      }
    }
    pItemData->m_nFlags &= ~(MF_BYPOSITION);
    pItemData->m_nFlags |= MF_OWNERDRAW;
    pItemData->m_nID = nID;

    // for having automated shortcut handling
    if(pSubMenu && bTextChanged)
    {
      if(pSubMenu->ModifyMenu(nLoc, MF_BYPOSITION|(pItemData->m_nFlags&~MF_OWNERDRAW), nID,pItemData->m_szMenuText) )
      {
        if(!pSubMenu->ModifyMenu(nLoc, MF_BYPOSITION|pItemData->m_nFlags, nID,(LPCTSTR)pItemData))
        {
          bModifyOK = FALSE;
        }
      }
      else
      {
        bModifyOK = FALSE;
      }
    }

    newSubs.Add(pSubMenu);
    newLocs.Add(nLoc);

    if(pSubMenu && nLoc>=0)
    {
      pSubMenu = FindAnotherMenuOption(nID,nLoc,newSubs,newLocs);
    }
    else
    {
      pSubMenu = NULL;
    }
  }while(pSubMenu);

  return (CMenu::ModifyMenu(nID,pItemData->m_nFlags,nID,(LPCTSTR)pItemData)) && bModifyOK;
}

BOOL CNewMenu::ModifyODMenu(LPCTSTR lpstrText, UINT nID, CImageList *pil, int xoffset)
{
  int nIndex = 0;
  CNewMenuIcons* pIcons = GetMenuIcon(nIndex,nID,pil,xoffset);
  pIcons->AddRef();

  BOOL bResult = ModifyODMenu(lpstrText,nID,pIcons,nIndex);

  pIcons->Release();

  return bResult;
}

BOOL CNewMenu::ModifyODMenu(LPCTSTR lpstrText, UINT nID, CNewMenuIcons* pIcons, int xoffset)
{
  ASSERT(pIcons);
  int nLoc;
  CNewMenuItemData *pItemData;
  CArray<CNewMenu*,CNewMenu*>newSubs;
  CArray<int,int&>newLocs;
  BOOL bModifyOK = TRUE;

  // Find the old CNewMenuItemData structure:
  CNewMenu *pSubMenu = FindMenuOption(nID,nLoc);
  do {
    if(pSubMenu && nLoc>=0)
    {
      pItemData = pSubMenu->m_MenuItemList[nLoc];
    }
    else
    {
      // Create a new CNewMenuItemData structure:
      pItemData = new CNewMenuItemData;
      m_MenuItemList.Add(pItemData);
    }

    BOOL bTextChanged = FALSE;
    ASSERT(pItemData);
    if(lpstrText && pItemData->m_szMenuText.Compare(lpstrText)!=NULL)
    {
      bTextChanged = TRUE;
      pItemData->SetString(lpstrText);
    }

    if(pIcons)
    {
      pIcons->AddRef();
      pItemData->m_pMenuIcon->Release();
      pItemData->m_pMenuIcon = pIcons;

      pItemData->m_nMenuIconOffset = xoffset;

      int x=0;
      int y=0;
      if(pSubMenu && pIcons->GetIconSize(&x,&y))
      {
        // Correct the size of the menuitem
        pSubMenu->m_iconX = max(pSubMenu->m_iconX,x);
        pSubMenu->m_iconY = max(pSubMenu->m_iconY,y);
      }
    }
    else
    {
      pItemData->m_nMenuIconOffset = -1;
    }
    pItemData->m_nFlags &= ~(MF_BYPOSITION);
    pItemData->m_nFlags |= MF_OWNERDRAW;
    pItemData->m_nID = nID;

    // for having automated shortcut handling
    if(pSubMenu && bTextChanged)
    {
      if(pSubMenu->ModifyMenu(nLoc, MF_BYPOSITION|(pItemData->m_nFlags&~MF_OWNERDRAW), nID,pItemData->m_szMenuText) )
      {
        if(!pSubMenu->ModifyMenu(nLoc, MF_BYPOSITION|pItemData->m_nFlags, nID,(LPCTSTR)pItemData))
        {
          bModifyOK = FALSE;
        }
      }
      else
      {
        bModifyOK = FALSE;
      }
    }

    newSubs.Add(pSubMenu);
    newLocs.Add(nLoc);
    if(pSubMenu && nLoc>=0)
    {
      pSubMenu = FindAnotherMenuOption(nID,nLoc,newSubs,newLocs);
    }
    else
    {
      pSubMenu = NULL;
    }
  } while(pSubMenu);

  return (CMenu::ModifyMenu(nID,pItemData->m_nFlags,nID,(LPCTSTR)pItemData)) && bModifyOK;
}

BOOL CNewMenu::ModifyODMenu(LPCTSTR lpstrText, UINT nID, CBitmap* bmp)
{
  if (bmp)
  {
    CImageList temp;
    temp.Create(m_iconX,m_iconY,ILC_COLORDDB|ILC_MASK,1,1);

    temp.Add(bmp,GetBitmapBackground());

    return ModifyODMenu(lpstrText,nID,&temp,0);
  }
  return ModifyODMenu(lpstrText,nID,(CImageList*)NULL,0);
}

BOOL CNewMenu::ModifyODMenu(LPCTSTR lpstrText, UINT nID, COLORREF fill, COLORREF border, int hatchstyle)
{
  // Get device context
  CSize bitmap_size(m_iconX,m_iconY);
  CBitmap bmp;
  {
    CClientDC DC(0);
    ColorBitmap(&DC,bmp,bitmap_size,fill,border,hatchstyle);
  }
  return ModifyODMenu(lpstrText,nID,&bmp);
}

BOOL CNewMenu::ModifyODMenu(LPCTSTR lpstrText, LPCTSTR OptionText, int nIconNormal)
{
  int nIndex = 0;
  CNewMenu* pOptionMenu = FindMenuOption(OptionText,nIndex);

  if(pOptionMenu!=NULL && nIndex>=0)
  {
    CNewMenuItemData* pItemData = pOptionMenu->m_MenuItemList[nIndex];

    BOOL bTextChanged = FALSE;
    ASSERT(pItemData);
    if(lpstrText && pItemData->m_szMenuText.Compare(lpstrText)!=NULL)
    {
      bTextChanged = TRUE;
      pItemData->SetString(lpstrText);
    }

    pItemData->m_nMenuIconOffset = nIconNormal;
    if(nIconNormal>=0)
    {
      CNewMenuIcons* pIcons = GetMenuIcon(pItemData->m_nMenuIconOffset,nIconNormal);
      pIcons->AddRef();
      pItemData->m_pMenuIcon->Release();
      pItemData->m_pMenuIcon = pIcons;

      CNewMenuBitmaps* pMenuIcon = DYNAMIC_DOWNCAST(CNewMenuBitmaps,pItemData->m_pMenuIcon);
      if(pMenuIcon)
      {
        CSize size = pMenuIcon->GetIconSize();
        pOptionMenu->m_iconX = max(pOptionMenu->m_iconX,size.cx);
        pOptionMenu->m_iconY = max(pOptionMenu->m_iconY,size.cy);
      }
    }

    // for having automated shortcut handling
    if(pOptionMenu && bTextChanged)
    {
      if(!pOptionMenu->ModifyMenu(nIndex, MF_BYPOSITION|(pItemData->m_nFlags&~MF_OWNERDRAW), pItemData->m_nID,pItemData->m_szMenuText) ||
        !pOptionMenu->ModifyMenu(nIndex, MF_BYPOSITION|pItemData->m_nFlags, pItemData->m_nID,(LPCTSTR)pItemData))
      {
        return FALSE;
      }
    }
    return TRUE;
  }
  return FALSE;
}

CNewMenuItemData* CNewMenu::NewODMenu(UINT pos, UINT nFlags, UINT nID, LPCTSTR string)
{
  CNewMenuItemData* pItemData;

  pItemData = new CNewMenuItemData;
  pItemData->m_nFlags = nFlags;
  pItemData->m_nID = nID;
  if(!(nFlags&MF_BITMAP))
  {
  pItemData->SetString (string);
  }

  if(nFlags & MF_POPUP)
  {
    CNewMenu* pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(UIntToHMenu(nID)));
    if(pSubMenu)
    {
      pSubMenu->m_hParentMenu = m_hMenu;
    }
  }

  BOOL bModified = FALSE;
  if (nFlags&MF_OWNERDRAW)
  {
    bModified = ModifyMenu(pos,nFlags,nID,(LPCTSTR)pItemData);
  }
  else if (nFlags&MF_BITMAP)
  {
    bModified = ModifyMenu(pos,nFlags,nID,(CBitmap*)string);
  }
  else if (nFlags&MF_SEPARATOR)
  {
    ASSERT(nFlags&MF_SEPARATOR);
    bModified = ModifyMenu(pos,nFlags,nID);
  }
  else // (nFlags&MF_STRING)
  {
    ASSERT(!(nFlags&MF_OWNERDRAW));
    bModified = ModifyMenu(pos,nFlags,nID,pItemData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL));
  }
  if(!bModified)
  {
    ShowLastError(_T("Error from Menu: ModifyMenu"));
  }
  return(pItemData);
};

BOOL CNewMenu::LoadToolBars(const UINT* arID, int n, HMODULE hInst)
{
  ASSERT(arID);
  BOOL returnflag = TRUE;
  for(int i=0;i<n;++i)
  {
    if(!LoadToolBar(arID[i],hInst))
    {
      returnflag = FALSE;
    }
  }
  return(returnflag);
}

DWORD CNewMenu::SetMenuIcons(CNewMenuIcons* pMenuIcons)
{
  ASSERT(pMenuIcons);
  int nCount = (int)pMenuIcons->m_IDs.GetSize();
  while(nCount--)
  {
    ModifyODMenu(NULL,pMenuIcons->m_IDs[nCount],pMenuIcons,nCount*MENU_ICONS);
  }
  return pMenuIcons->m_dwRefCount;
}

CNewMenuIcons* CNewMenu::GetMenuIcon(int &nIndex, UINT nID, CImageList *pil, int xoffset)
{
  if(pil==NULL || xoffset<0)
  {
    nIndex=-1;
    return NULL;
  }

  HICON hIcon = pil->ExtractIcon(xoffset);

  if(m_pSharedMenuIcons!=NULL)
  {
    POSITION pos = m_pSharedMenuIcons->GetHeadPosition();
    while(pos)
    {
      CNewMenuBitmaps* pMenuIcon = DYNAMIC_DOWNCAST(CNewMenuBitmaps,m_pSharedMenuIcons->GetNext(pos));
      if(pMenuIcon)
      {
        nIndex = pMenuIcon->Add(hIcon,nID);
        if(nIndex!=-1)
        {
          DestroyIcon(hIcon);
          return pMenuIcon;
        }
      }
    }
  }
  else
  {
    m_pSharedMenuIcons = new CTypedPtrList<CPtrList, CNewMenuIcons*>;
  }
  CNewMenuBitmaps* pMenuIcon = new CNewMenuBitmaps();
  pMenuIcon->m_crTransparent = m_bitmapBackground;
  nIndex = pMenuIcon->Add(hIcon,nID);
  DestroyIcon(hIcon);
  if(nIndex!=-1)
  {
    m_pSharedMenuIcons->AddTail(pMenuIcon);
    return pMenuIcon;
  }
  delete pMenuIcon;
  return NULL;
}

CNewMenuIcons* CNewMenu::GetMenuIcon(int &nIndex, int nID)
{
  if(m_pSharedMenuIcons!=NULL)
  {
    POSITION pos = m_pSharedMenuIcons->GetHeadPosition();
    while(pos)
    {
      CNewMenuBitmaps* pMenuIcon = DYNAMIC_DOWNCAST(CNewMenuBitmaps,m_pSharedMenuIcons->GetNext(pos));
      if(pMenuIcon)
      {
        if(m_bDynIcons)
        {
          nIndex = pMenuIcon->Add((HICON)(INT_PTR)nID);
        }
        else
        {
          nIndex = pMenuIcon->Add(nID,m_bitmapBackground);
        }
        if(nIndex!=-1)
        {
          return pMenuIcon;
        }
      }
    }
  }
  else
  {
    m_pSharedMenuIcons = new CTypedPtrList<CPtrList, CNewMenuIcons*>;
  }
  CNewMenuBitmaps* pMenuIcon = new CNewMenuBitmaps();
  pMenuIcon->m_crTransparent = m_bitmapBackground;
  nIndex = pMenuIcon->Add(nID,m_bitmapBackground);
  if(nIndex!=-1)
  {
    m_pSharedMenuIcons->AddTail(pMenuIcon);
    return pMenuIcon;
  }
  delete pMenuIcon;
  return NULL;
}

CNewMenuIcons* CNewMenu::GetMenuIcon(int &nIndex, CBitmap* pBmp)
{
  if(pBmp==NULL)
  {
    nIndex=-1;
    return NULL;
  }

  if(m_pSharedMenuIcons!=NULL)
  {
    POSITION pos = m_pSharedMenuIcons->GetHeadPosition();
    while(pos)
    {
      CNewMenuBitmaps* pMenuIcon = DYNAMIC_DOWNCAST(CNewMenuBitmaps,m_pSharedMenuIcons->GetNext(pos));
      if(pMenuIcon)
      {
        nIndex = pMenuIcon->Add(pBmp,m_bitmapBackground);
        if(nIndex!=-1)
        {
          return pMenuIcon;
        }
      }
    }
  }
  else
  {
    m_pSharedMenuIcons = new CTypedPtrList<CPtrList, CNewMenuIcons*>;
  }
  CNewMenuBitmaps* pMenuIcon = new CNewMenuBitmaps();
  pMenuIcon->m_crTransparent = m_bitmapBackground;
  nIndex = pMenuIcon->Add(pBmp,m_bitmapBackground);
  if(nIndex!=-1)
  {
    m_pSharedMenuIcons->AddTail(pMenuIcon);
    return pMenuIcon;
  }
  delete pMenuIcon;
  return NULL;
}


CNewMenuIcons* CNewMenu::GetToolbarIcons(UINT nToolBar, HMODULE hInst)
{
  ASSERT_VALID(this);
  ASSERT(nToolBar != NULL);

  if(m_pSharedMenuIcons!=NULL)
  {
    POSITION pos = m_pSharedMenuIcons->GetHeadPosition();
    while(pos)
    {
      CNewMenuIcons* pMenuIcon = m_pSharedMenuIcons->GetNext(pos);
      if(pMenuIcon->DoMatch(MAKEINTRESOURCE(nToolBar),hInst))
      {
        return pMenuIcon;
      }
    }
  }
  else
  {
    m_pSharedMenuIcons = new CTypedPtrList<CPtrList, CNewMenuIcons*>;
  }
  CNewMenuIcons* pMenuIcon = new CNewMenuIcons();
  pMenuIcon->m_crTransparent = m_bitmapBackground;
  if(pMenuIcon->LoadToolBar(MAKEINTRESOURCE(nToolBar),hInst))
  {
    m_pSharedMenuIcons->AddTail(pMenuIcon);
    return pMenuIcon;
  }
  delete pMenuIcon;
  return NULL;
}


BOOL CNewMenu::LoadToolBar(LPCTSTR lpszResourceName, HMODULE hInst)
{
  CNewMenuIcons* pMenuIcon = GetToolbarIcons((UINT)(UINT_PTR)lpszResourceName,hInst);
  if(pMenuIcon)
  {
    SetMenuIcons(pMenuIcon);
    return TRUE;
  }
  return FALSE;
}

BOOL CNewMenu::LoadToolBar(HBITMAP hBitmap, CSize size, UINT* pID, COLORREF crTransparent)
{
  ASSERT_VALID(this);
  ASSERT(pID);

  if(crTransparent==CLR_NONE)
  {
    crTransparent = m_bitmapBackground;
  }
  if(m_pSharedMenuIcons!=NULL)
  {
    POSITION pos = m_pSharedMenuIcons->GetHeadPosition();
    while(pos)
    {
      CNewMenuIcons* pMenuIcon = m_pSharedMenuIcons->GetNext(pos);
      if(pMenuIcon->DoMatch(hBitmap,size,pID))
      {
        SetMenuIcons(pMenuIcon);
        return TRUE;
      }
    }
  }
  else
  {
    m_pSharedMenuIcons = new CTypedPtrList<CPtrList, CNewMenuIcons*>;
  }
  CNewMenuIcons* pMenuIcon = new CNewMenuIcons();
  if(pMenuIcon->LoadToolBar(hBitmap,size,pID,crTransparent))
  {
    m_pSharedMenuIcons->AddTail(pMenuIcon);
    SetMenuIcons(pMenuIcon);
    return TRUE;
  }
  delete pMenuIcon;
  return FALSE;
}

BOOL CNewMenu::LoadToolBar(WORD* pToolInfo, COLORREF crTransparent)
{
  ASSERT_VALID(this);
  ASSERT(pToolInfo);

  if(crTransparent==CLR_NONE)
  {
    crTransparent = m_bitmapBackground;
  }

  if(m_pSharedMenuIcons!=NULL)
  {
    POSITION pos = m_pSharedMenuIcons->GetHeadPosition();
    while(pos)
    {
      CNewMenuIcons* pMenuIcon = m_pSharedMenuIcons->GetNext(pos);
      if(pMenuIcon->DoMatch(pToolInfo,crTransparent))
      {
        SetMenuIcons(pMenuIcon);
        return TRUE;
      }
    }
  }
  else
  {
    m_pSharedMenuIcons = new CTypedPtrList<CPtrList, CNewMenuIcons*>;
  }
  CNewMenuIcons* pMenuIcon = new CNewMenuIcons();
  if(pMenuIcon->LoadToolBar(pToolInfo,crTransparent))
  {
    m_pSharedMenuIcons->AddTail(pMenuIcon);
    SetMenuIcons(pMenuIcon);
    return TRUE;
  }
  delete pMenuIcon;
  return FALSE;
}


// Jan-12-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/
// added this function as a helper to LoadToolBar(WORD* pToolInfo, COLORREF crTransparent), the purpose of this function
// is to easily load a high color toolbar to the menu, instead of creating and maintaining a command map.  this function
// simply reads an existing 16 bit toolbar, and builds the word map based on that toolbar, using the 256 color bitmap.
BOOL CNewMenu::LoadToolBar(UINT n16ToolBarID, UINT n256BitmapID, COLORREF transparentColor, HMODULE hInst)
{
  BOOL bRet = FALSE;
  // determine location of the bitmap in resource
  if(hInst==0)
  {
    hInst = AfxFindResourceHandle(MAKEINTRESOURCE(n16ToolBarID), RT_TOOLBAR);
  }
  HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(n16ToolBarID), RT_TOOLBAR);

  if (hRsrc == NULL)
  { // Special purpose when you try to load it from a dll 30.05.2002
    if(AfxGetResourceHandle()!=hInst)
    {
      hInst = AfxGetResourceHandle();
      hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(n16ToolBarID), RT_TOOLBAR);
    }
    if (hRsrc == NULL)
    {
      return FALSE;
    }
  }

  HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
  if (hGlobal)
  {
    CToolBarData* pData = (CToolBarData*)LockResource(hGlobal);
    if (pData)
    {
      int nSize = sizeof(WORD)*(pData->wItemCount+4);
      // no need for delete
      WORD* pToolId = (WORD*)_alloca(nSize);
      // sets also the last entry to zero
      ZeroMemory(pToolId,nSize);

      pToolId[0] = (WORD)n256BitmapID;
      pToolId[1] = pData->wWidth;
      pToolId[2] = pData->wHeight;

      WORD* pTemp = pToolId+3;
      for (int nIndex = 0; nIndex < pData->wItemCount; nIndex++)
      { // not a seperator?
        if( (pData->items()[nIndex]) )
        {
          *pTemp++ = pData->items()[nIndex];
        }
      }
      // load the toolbar images into the menu
      bRet = LoadToolBar(pToolId, transparentColor);
      UnlockResource(hGlobal);
    }
    FreeResource(hGlobal);
  }
  // return TRUE for success, FALSE for failure
  return (bRet);
} // LoadHiColor


BOOL CNewMenu::LoadToolBar(UINT nToolBar, HMODULE hInst)
{
  return LoadToolBar((LPCTSTR)(UINT_PTR)nToolBar,hInst);
}

BOOL CNewMenu::LoadFromToolBar(UINT nID, UINT nToolBar, int& xoffset)
{
  int xset,offset;
  UINT nStyle;
  BOOL returnflag=FALSE;
  CToolBar bar;

  CWnd* pWnd = AfxGetMainWnd();
  if (pWnd == NULL)
  {
    pWnd = CWnd::GetDesktopWindow();
  }
  bar.Create(pWnd);

  if(bar.LoadToolBar(nToolBar))
  {
    offset=bar.CommandToIndex(nID);
    if(offset>=0)
    {
      bar.GetButtonInfo(offset,nID,nStyle,xset);
      if(xset>0)
      {
        xoffset = xset;
      }
      returnflag=TRUE;
    }
  }
  return returnflag;
}

// O.S.
CNewMenuItemData* CNewMenu::FindMenuItem(UINT nID)
{
  CNewMenuItemData *pData = NULL;
  int i;

  for(i = 0; i < m_MenuItemList.GetSize(); i++)
  {
    if (m_MenuItemList[i]->m_nID == nID)
    {
      pData = m_MenuItemList[i];
      break;
    }
  }
  if (!pData)
  {
    int loc;
    CNewMenu *pMenu = FindMenuOption(nID, loc);
    ASSERT(pMenu != this);
    if (loc >= 0)
    {
      return pMenu->FindMenuItem(nID);
    }
  }
  return pData;
}


CNewMenu* CNewMenu::FindAnotherMenuOption(int nId, int& nLoc,
                                          CArray<CNewMenu*,CNewMenu*>&newSubs,
                                          CArray<int,int&>&newLocs)
{
  int i,numsubs,j;
  CNewMenu *pSubMenu,*pgoodmenu;
  BOOL foundflag;

  for(i=0;i<(int)(GetMenuItemCount());++i)
  {
    pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,GetSubMenu(i));
    if(pSubMenu)
    {
      pgoodmenu = pSubMenu->FindAnotherMenuOption(nId,nLoc,newSubs,newLocs);
      if(pgoodmenu)
      {
        return pgoodmenu;
      }
    }
    else if(nId==(int)GetMenuItemID(i))
    {
      numsubs = (int)newSubs.GetSize();
      foundflag = TRUE;
      for(j=0;j<numsubs;++j)
      {
        if(newSubs[j]==this && newLocs[j]==i)
        {
          foundflag = FALSE;
          break;
        }
      }
      if(foundflag)
      {
        nLoc = i;
        return this;
      }
    }
  }
  nLoc = -1;

  return NULL;
}

CNewMenu* CNewMenu::FindMenuOption(int nId, int& nLoc)
{
  int i;
  CNewMenu *pSubMenu,*pgoodmenu;

  for(i=0;i<(int)(GetMenuItemCount());++i)
  {
    pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,GetSubMenu(i));
    if(pSubMenu)
    {
      pgoodmenu = pSubMenu->FindMenuOption(nId,nLoc);
      if(pgoodmenu)
      {
        return pgoodmenu;
      }
    }
    else if(nId==(int)GetMenuItemID(i))
    {
      nLoc = i;
      return(this);
    }
  }
  nLoc = -1;
  return NULL;
}

CNewMenu* CNewMenu::FindMenuOption(LPCTSTR lpstrText, int& nLoc)
{
  int i;
  // First look for all item text.
  for(i=0;i<(int)m_MenuItemList.GetSize();++i)
  {
    if(m_MenuItemList[i]->m_szMenuText.Compare(lpstrText)==NULL)
    {
      nLoc = i;
      return this;
    }
  }
  CNewMenu* pSubMenu;
  // next, look in all submenus
  for(i=0; i<(int)(GetMenuItemCount());++i)
  {
    pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,GetSubMenu(i));
    if(pSubMenu)
    {
      pSubMenu = pSubMenu->FindMenuOption(lpstrText,nLoc);
      if(pSubMenu)
      {
        return pSubMenu;
      }
    }
  }
  nLoc = -1;
  return NULL;
}

BOOL CNewMenu::LoadMenu(HMENU hMenu)
{
  if(!::IsMenu(hMenu) || !Attach(hMenu))
  {
    return FALSE;
  }
  m_bIsPopupMenu = FALSE;
  for(int i=0;i<(int)(GetMenuItemCount());++i)
  {
    HMENU hSubMenu = ::GetSubMenu(m_hMenu,i);
    if(hSubMenu)
    {
      CNewMenu* pMenu = new CNewMenu(m_hMenu);
      m_SubMenus.Add(hSubMenu);
      pMenu->LoadMenu(hSubMenu);
      pMenu->m_bIsPopupMenu = TRUE;
    }
  }
  SynchronizeMenu();
  return TRUE;
}

BOOL CNewMenu::LoadMenu(int nResource)
{
  return(CNewMenu::LoadMenu(MAKEINTRESOURCE(nResource)));
}

BOOL CNewMenu::LoadMenu(LPCTSTR lpszResourceName)
{
  ASSERT(this);
  HMENU hMenu = ::LoadMenu(AfxFindResourceHandle(lpszResourceName,RT_MENU), lpszResourceName);
  #ifdef _DEBUG
  ShowLastError(_T("Error from Menu: LoadMenu"));
  #endif
  return LoadMenu(hMenu);
}

BOOL CNewMenu::SetItemData(UINT uiId, DWORD_PTR dwItemData, BOOL fByPos)
{
  MENUITEMINFO MenuItemInfo = {0};
  MenuItemInfo.cbSize = sizeof(MenuItemInfo);
  MenuItemInfo.fMask = MIIM_DATA;
  if(::GetMenuItemInfo(m_hMenu,uiId,fByPos,&MenuItemInfo))
  {
    CNewMenuItemData* pItem = CheckMenuItemData(MenuItemInfo.dwItemData);
    if(pItem)
    {
      pItem->m_pData = (void*)dwItemData;
      return TRUE;
    }
  }
  return FALSE;
}

BOOL CNewMenu::SetItemDataPtr(UINT uiId, void* pItemData, BOOL fByPos )
{
  return SetItemData(uiId, (DWORD_PTR) pItemData, fByPos);
}

DWORD_PTR CNewMenu::GetItemData(UINT uiId, BOOL fByPos) const
{
  MENUITEMINFO MenuItemInfo = {0};
  MenuItemInfo.cbSize = sizeof(MenuItemInfo);
  MenuItemInfo.fMask = MIIM_DATA;
  if(::GetMenuItemInfo(m_hMenu,uiId,fByPos,&MenuItemInfo))
  {
    CNewMenuItemData* pItem = CheckMenuItemData(MenuItemInfo.dwItemData);
    if(pItem)
    {
      return (DWORD_PTR)pItem->m_pData;
    }
  }
  return DWORD_PTR(-1);
}

void* CNewMenu::GetItemDataPtr(UINT uiId, BOOL fByPos) const
{
  return (void*)GetItemData(uiId,fByPos);
}

BOOL CNewMenu::SetMenuData(DWORD_PTR dwMenuData)
{
  m_pData = (void*)dwMenuData;
  return TRUE;
}

BOOL CNewMenu::SetMenuDataPtr(void* pMenuData)
{
  m_pData = (void*)pMenuData;
  return TRUE;
}

DWORD_PTR CNewMenu::GetMenuData() const
{
  return (DWORD_PTR)m_pData;
}

void* CNewMenu::GetMenuDataPtr() const
{
  return m_pData;
}

BOOL  CNewMenu::m_bDrawAccelerators = TRUE;

BOOL  CNewMenu::SetAcceleratorsDraw (BOOL bDraw)
{
  BOOL bOld = m_bDrawAccelerators;
  m_bDrawAccelerators = bDraw;
  return bOld;
}
BOOL  CNewMenu::GetAcceleratorsDraw ()
{
  return m_bDrawAccelerators;
}

BYTE CNewMenu::m_bAlpha = 255;

BYTE CNewMenu::SetAlpha(BYTE bAlpha)
{
  BYTE oldAlpha = m_bAlpha;
  m_bAlpha = bAlpha;
  return oldAlpha;
}

BYTE CNewMenu::GetAlpha()
{
  return m_bAlpha;
}

// INVALID_HANDLE_VALUE = Draw default frame's accel. NULL = Off
HACCEL  CNewMenu::SetAccelerator (HACCEL hAccel)
{
  HACCEL hOld = m_hAccelToDraw;
  m_hAccelToDraw = hAccel;
  return hOld;
}
HACCEL  CNewMenu::GetAccelerator ()
{
  return m_hAccelToDraw;
}

void CNewMenu::LoadCheckmarkBitmap(int unselect, int select)
{
  if(unselect>0 && select>0)
  {
    m_selectcheck = select;
    m_unselectcheck = unselect;
    if(m_checkmaps)
    {
      m_checkmaps->DeleteImageList();
    }
    else
    {
      m_checkmaps = new CImageList();
    }

    m_checkmaps->Create(m_iconX,m_iconY,ILC_MASK,2,1);
    BOOL flag1 = AddBitmapToImageList(m_checkmaps,unselect);
    BOOL flag2 = AddBitmapToImageList(m_checkmaps,select);

    if(!flag1||!flag2)
    {
      m_checkmaps->DeleteImageList();
      delete m_checkmaps;
      m_checkmaps = NULL;
    }
  }
}

BOOL CNewMenu::GetMenuText(UINT id, CString& string, UINT nFlags/*= MF_BYPOSITION*/)
{
  if(MF_BYPOSITION&nFlags)
  {
    UINT numMenuItems = (int)m_MenuItemList.GetSize();
    if(id<numMenuItems)
    {
      string = m_MenuItemList[id]->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
      return TRUE;
    }
  }
  else
  {
    int uiLoc;
    CNewMenu* pMenu = FindMenuOption(id,uiLoc);
    if(NULL!=pMenu)
    {
      return pMenu->GetMenuText(uiLoc,string);
    }
  }
  return FALSE;
}

CNewMenuItemData* CNewMenu::CheckMenuItemData(ULONG_PTR nItemData) const
{
  for(int i=0;i<m_MenuItemList.GetSize();++i)
  {
    CNewMenuItemData* pItem = m_MenuItemList[i];
    if ( ((ULONG_PTR)pItem)==nItemData )
    {
      return pItem;
    }
  }
  return NULL;
}


CNewMenuItemData* CNewMenu::FindMenuList(UINT nID)
{
  for(int i=0;i<m_MenuItemList.GetSize();++i)
  {
    CNewMenuItemData* pMenuItem = m_MenuItemList[i];
    if(pMenuItem->m_nID==nID && !pMenuItem->m_nSyncFlag)
    {
      pMenuItem->m_nSyncFlag = 1;
      return pMenuItem;
    }
  }
  return NULL;
}

void CNewMenu::InitializeMenuList(int value)
{
  for(int i=0;i<m_MenuItemList.GetSize();++i)
  {
    m_MenuItemList[i]->m_nSyncFlag = value;
  }
}

void CNewMenu::DeleteMenuList()
{
  for(int i=0;i<m_MenuItemList.GetSize();++i)
  {
    if(!m_MenuItemList[i]->m_nSyncFlag)
    {
      delete m_MenuItemList[i];
    }
  }
}

void CNewMenu::SynchronizeMenu()
{
  CTypedPtrArray<CPtrArray, CNewMenuItemData*> temp;
  CNewMenuItemData *pItemData;
  CString string;
  UINT submenu,state,j;

  InitializeMenuList(0);
  for(j=0;j<GetMenuItemCount();++j)
  {
    MENUITEMINFO menuInfo = {0};
    menuInfo.cbSize = sizeof(menuInfo);
    // we get a wrong state for seperators
    menuInfo.fMask = MIIM_TYPE|MIIM_ID|MIIM_STATE;
    // item doesn't exist
    if(!GetMenuItemInfo(j,&menuInfo,TRUE))
    {
      break;
    }
    state = GetMenuState(j,MF_BYPOSITION);
    if(state!=menuInfo.fState)
    {
      state|=0;
    }
    pItemData=NULL;
    if(state==UINT_PTR(-1))
    {
      break;
    }
    if(!(state&MF_SYSMENU))
    {
      if(menuInfo.fType&MFT_RIGHTJUSTIFY)
      {
        state |= MF_RIGHTJUSTIFY;
      }
      // MFT_RIGHTORDER is the same value as MFT_SYSMENU.
      // We distinguish between the two by also looking for MFT_BITMAP.
      if(!(state&MF_BITMAP) && (menuInfo.fType&MFT_RIGHTORDER) )
      {
        state |= MFT_RIGHTORDER;
      }
    }

    // Special purpose for Windows NT 4.0
    if(state&MF_BITMAP)
    {
      // Bitmap-button like system menu, maximize, minimize and close
      // just ignore.
      UINT nID = GetMenuItemID(j);
      pItemData = FindMenuList(nID);
      if(!pItemData)
      {
        //if(nID>=SC_SIZE && nID<=SC_HOTKEY)
        // we do not support bitmap in menu because the OS handle it not right with
        // ownerdrawn style so let show the default one
        {
        pItemData = new CNewMenuItemData;
        pItemData->m_nFlags = state;
        pItemData->m_nID = nID;
      }
        //else
        //{
        //  // sets the handle of the bitmap
        //  pItemData = NewODMenu(j,state|MF_BYPOSITION|MF_OWNERDRAW,nID,menuInfo.dwTypeData);
        //}
      }
    }
    else if(state&MF_POPUP)
    {
      HMENU hSubMenu = GetSubMenu(j)->m_hMenu;
      submenu = HMenuToUInt(hSubMenu);
      pItemData = FindMenuList(submenu);
      GetMenuString(j,string,MF_BYPOSITION);

      if(!pItemData)
      {
        state &= ~(MF_USECHECKBITMAPS|MF_SEPARATOR);
        pItemData = NewODMenu(j,state|MF_BYPOSITION|MF_POPUP|MF_OWNERDRAW,submenu,string);
      }
      else if(!string.IsEmpty ())
      {
        pItemData->SetString(string);
      }

      CNewMenu* pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(hSubMenu));
      if(pSubMenu && pSubMenu->m_hParentMenu!=m_hMenu)
      { // Sets again the parent to this one
        pSubMenu->m_hParentMenu = m_hMenu;
      }
    }
    else if(state&MF_SEPARATOR)
    {
      pItemData = FindMenuList(0);
      if(!pItemData)
      {
        pItemData = NewODMenu(j,state|MF_BYPOSITION|MF_OWNERDRAW,0,_T(""));
      }
      else
      {
        pItemData->m_nFlags = state|MF_BYPOSITION|MF_OWNERDRAW;
        ModifyMenu(j,pItemData->m_nFlags,0,(LPCTSTR)pItemData);
      }
    }
    else
    {
      UINT nID = GetMenuItemID(j);
      pItemData = FindMenuList(nID);
      GetMenuString(j,string,MF_BYPOSITION);

      if(!pItemData)
      {
        pItemData = NewODMenu(j,state|MF_BYPOSITION|MF_OWNERDRAW,nID,string);
      }
      else
      {
        pItemData->m_nFlags = state|MF_BYPOSITION|MF_OWNERDRAW;
        if(string.GetLength()>0)
        {
          pItemData->SetString(string);
        }
        ModifyMenu(j,pItemData->m_nFlags,nID,(LPCTSTR)pItemData);
      }
    }
    if(pItemData)
    {
      temp.Add(pItemData);
    }
  }
  DeleteMenuList();
  m_MenuItemList.RemoveAll();
  m_MenuItemList.Append(temp);
  temp.RemoveAll();
}

void CNewMenu::OnInitMenuPopup(HWND hWnd, CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
  UNREFERENCED_PARAMETER(nIndex);
  UNREFERENCED_PARAMETER(bSysMenu);

#ifdef _TRACE_MENU_
  AfxTrace(_T("InitMenuPopup: 0x%lx from Wnd 0x%lx\n"),HMenuToUInt(pPopupMenu->m_hMenu),HWndToUInt(hWnd));
#endif
  CNewMenuHook::m_hLastMenu = pPopupMenu->m_hMenu;

  if(IsMenu(pPopupMenu->m_hMenu))
  {
    CNewMenu* pSubMenu = DYNAMIC_DOWNCAST(CNewMenu,pPopupMenu);
    if(pSubMenu)
    {
      pSubMenu->m_hTempOwner = hWnd;
      pSubMenu->OnInitMenuPopup();
      HMENU hMenu = pSubMenu->GetParent();
      CNewMenu* pParent = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(hMenu));
      if(pParent)
      {
        pParent->m_dwOpenMenu += 1;
        if(pParent->m_dwOpenMenu==1 && !pParent->m_bIsPopupMenu)
        {
          // Redraw the menubar for the shade
          CRect rect = pParent->GetLastActiveMenuRect();
          if(!rect.IsRectEmpty())
          {
            rect.InflateRect(0,0,10,10);
            CPoint Point(0,0);
            ClientToScreen(hWnd,&Point);
            rect.OffsetRect(-Point);
            RedrawWindow(hWnd,rect,0,RDW_FRAME|RDW_INVALIDATE);
          }
        }
      }
    }
  }
}

BOOL CNewMenu::Replace(UINT nID, UINT nNewID)
{
  int nLoc=0;
  CNewMenu* pTempMenu = FindMenuOption(nID,nLoc);
  if(pTempMenu && nLoc >= 0)
  {
#ifdef _TRACE_MENU_
    AfxTrace(_T("Replace MenuID 0x%X => 0x%X\n"),nID,nNewID);
#endif
    CNewMenuItemData* pData = pTempMenu->m_MenuItemList[nLoc];
    UINT nFlags = pData->m_nFlags|MF_OWNERDRAW|MF_BYPOSITION;
    pData->m_nID = nNewID;
    return pTempMenu->ModifyMenu(nLoc,nFlags,nNewID,(LPCTSTR)pData);
  }
  return FALSE;
}

void CNewMenu::OnInitMenuPopup()
{
  m_bIsPopupMenu = true;
  SynchronizeMenu();
  // Special purpose for windows XP with themes!!!
  if(g_Shell==WinXP)
  {
    Replace(SC_RESTORE,SC_RESTORE+1);
    Replace(SC_CLOSE,SC_CLOSE+1);
    Replace(SC_MINIMIZE,SC_MINIMIZE+1);
  }
}

BOOL CNewMenu::OnUnInitPopupMenu()
{
#ifdef _TRACE_MENU_
  AfxTrace(_T("UnInitMenuPopup: 0x%lx\n"),HMenuToUInt(m_hMenu));
#endif
  if(g_Shell==WinXP)
  {
    // Special purpose for windows XP with themes!!!
    // Restore old values otherwise you have disabled windowbuttons
    Replace(SC_MINIMIZE+1,SC_MINIMIZE);
    Replace(SC_RESTORE+1,SC_RESTORE);
    if(Replace(SC_CLOSE+1,SC_CLOSE))
    {
      //EnableMenuItem(SC_CLOSE, MF_BYCOMMAND|MF_ENABLED);
      SetWindowPos(m_hTempOwner,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
    }
    //Replace(SC_RESTORE+1,SC_RESTORE);
    //Replace(SC_MINIMIZE+1,SC_MINIMIZE);
  }

  HMENU hMenu = GetParent();
  CNewMenu* pParent = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(hMenu));
  if(pParent)
  {
    pParent->m_dwOpenMenu -= 1;
    if(pParent->m_dwOpenMenu>=NULL && !pParent->m_bIsPopupMenu)
    {
      pParent->m_dwOpenMenu = 0;
      // Redraw the menubar for the shade
      CRect rect = pParent->GetLastActiveMenuRect();
      if(!rect.IsRectEmpty())
      {
        rect.InflateRect(0,0,10,10);
        CPoint Point(0,0);
        ClientToScreen(m_hTempOwner,&Point);
        rect.OffsetRect(-Point);
        RedrawWindow(m_hTempOwner,rect,0,RDW_FRAME|RDW_INVALIDATE);
      }
      return TRUE;
    }
  }
  return FALSE;
}

LRESULT CNewMenu::FindKeyboardShortcut(UINT nChar, UINT nFlags, CMenu* pMenu)
{
  UNREFERENCED_PARAMETER(nFlags);

  CNewMenu* pNewMenu = DYNAMIC_DOWNCAST(CNewMenu,pMenu);
  if(pNewMenu)
  {
    //SK: modified for Unicode correctness
    CString key(_T('&'),2);
    key.SetAt(1,(TCHAR)nChar);
    key.MakeLower();
    CString menutext;
    int menusize = (int)pNewMenu->GetMenuItemCount();
    if(menusize!=(pNewMenu->m_MenuItemList.GetSize()))
    {
      pNewMenu->SynchronizeMenu();
    }
    for(int i=0;i<menusize;++i)
    {
      if(pNewMenu->GetMenuText(i,menutext))
      {
        menutext.MakeLower();
        if(menutext.Find(key)>=0)
        {
          return(MAKELRESULT(i,2));
        }
      }
    }
  }
  return NULL;
}

BOOL CNewMenu::AddBitmapToImageList(CImageList* bmplist,UINT nResourceID)
{
  // O.S.
  if (m_bDynIcons)
  {
    bmplist->Add((HICON)(UINT_PTR)nResourceID);
    return TRUE;
  }

  CBitmap mybmp;
  HBITMAP hbmp = LoadSysColorBitmap(nResourceID);
  if(hbmp)
  {
    // Object will be destroyd by destructor of CBitmap
    mybmp.Attach(hbmp);
  }
  else
  {
    mybmp.LoadBitmap(nResourceID);
  }

  if (mybmp.m_hObject && bmplist->Add(&mybmp,GetBitmapBackground())>=0 )
  {
    return TRUE;
  }

  return FALSE;
}

COLORREF CNewMenu::SetBitmapBackground(COLORREF newColor)
{
  COLORREF oldColor = m_bitmapBackground;
  m_bitmapBackground = newColor;
  return oldColor;
}

COLORREF CNewMenu::GetBitmapBackground()
{
  if(m_bitmapBackground==CLR_DEFAULT)
    return GetSysColor(COLOR_3DFACE);

  return m_bitmapBackground;
}

BOOL CNewMenu::Draw3DCheckmark(CDC *pDC, CRect rect, HBITMAP hbmpChecked, HBITMAP hbmpUnchecked, DWORD dwState)
{
  if(dwState&ODS_CHECKED || hbmpUnchecked)
  {
    rect.InflateRect(-1,-1);

    if (IsNewShell()) //SK: looks better on the old shell
    {
      pDC->DrawEdge(rect, BDR_SUNKENOUTER, BF_RECT);
    }

    rect.InflateRect (2,2);
    if(dwState&ODS_CHECKED)
    {
      if (!hbmpChecked)
      { // Checkmark
        rect.OffsetRect(1,2);
        DrawSpecialCharStyle(pDC,rect,98,dwState);
      }
      else if(!hbmpUnchecked)
      { // Bullet
        DrawSpecialCharStyle(pDC,rect,105,dwState);
      }
      else
      {
        // Draw Bitmap
        BITMAP myInfo = {0};
        GetObject((HGDIOBJ)hbmpChecked,sizeof(myInfo),&myInfo);
        CPoint Offset = rect.TopLeft() + CPoint((rect.Width()-myInfo.bmWidth)/2,(rect.Height()-myInfo.bmHeight)/2);
        pDC->DrawState(Offset,CSize(0,0),hbmpChecked,DST_BITMAP|DSS_MONO);
      }
    }
    else
    {
      // Draw Bitmap
      BITMAP myInfo = {0};
      GetObject((HGDIOBJ)hbmpUnchecked,sizeof(myInfo),&myInfo);
      CPoint Offset = rect.TopLeft() + CPoint((rect.Width()-myInfo.bmWidth)/2,(rect.Height()-myInfo.bmHeight)/2);
      if(dwState & ODS_DISABLED)
      {
        pDC->DrawState(Offset,CSize(0,0),hbmpUnchecked,DST_BITMAP|DSS_MONO|DSS_DISABLED);
      }
      else
      {
        pDC->DrawState(Offset,CSize(0,0),hbmpUnchecked,DST_BITMAP|DSS_MONO);
      }
    }
    return TRUE;
  }
  return FALSE;
}

HBITMAP CNewMenu::LoadSysColorBitmap(int nResourceId)
{
  HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(nResourceId),RT_BITMAP);
  HRSRC hRsrc = ::FindResource(hInst,MAKEINTRESOURCE(nResourceId),RT_BITMAP);
  if (hRsrc == NULL)
  {
    return NULL;
  }

  // determine how many colors in the bitmap
  HGLOBAL hglb;
  if ((hglb = LoadResource(hInst, hRsrc)) == NULL)
  {
    return NULL;
  }
  LPBITMAPINFOHEADER lpBitmap = (LPBITMAPINFOHEADER)LockResource(hglb);
  if (lpBitmap == NULL)
  {
    return NULL;
  }
  WORD numcol = NumBitmapColors(lpBitmap);
  ::FreeResource(hglb);

  if(numcol!=16)
  {
    return(NULL);
  }
  return AfxLoadSysColorBitmap(hInst, hRsrc, FALSE);
}

// sPos means Seperator's position, since we have no way to find the
// seperator's position in the menu we have to specify them when we call the
// RemoveMenu to make sure the unused seperators are removed;
// sPos  = None no seperator removal;
//       = Head  seperator in front of this menu item;
//       = Tail  seperator right after this menu item;
//       = Both  seperators at both ends;
// remove the menu item based on their text, return -1 if not found, otherwise
// return the menu position;
int CNewMenu::RemoveMenu(LPCTSTR pText, ESeperator sPos)
{
  int nPos = GetMenuPosition(pText);
  if(nPos != -1)
  {
    switch (sPos)
    {
    case CNewMenu::NONE:
      RemoveMenu(nPos, MF_BYPOSITION);
      break;

    case CNewMenu::HEAD:
      ASSERT(nPos - 1 >= 0);
      RemoveMenu(nPos-1, MF_BYPOSITION);
      break;

    case CNewMenu::TAIL:
      RemoveMenu(nPos+1, MF_BYPOSITION);
      break;

    case CNewMenu::BOTH:
      // remove the end first;
      RemoveMenu(nPos+1, MF_BYPOSITION);
      // remove the head;
      ASSERT(nPos - 1 >= 0);
      RemoveMenu(nPos-1, MF_BYPOSITION);
      break;
    }
  }
  return nPos;
}

BOOL CNewMenu::RemoveMenu(UINT uiId, UINT nFlags)
{
  if(MF_BYPOSITION&nFlags)
  {
    UINT nItemState = GetMenuState(uiId,MF_BYPOSITION);
    if((nItemState&MF_SEPARATOR) && !(nItemState&MF_POPUP))
    {
      CNewMenuItemData* pData =  m_MenuItemList.GetAt(uiId);
      m_MenuItemList.RemoveAt(uiId);
      delete pData;
    }
    else
    {
      CMenu* pSubMenu = GetSubMenu(uiId);
      if(NULL==pSubMenu)
      {
        UINT uiCommandId = GetMenuItemID(uiId);
        for(int i=0;i<m_MenuItemList.GetSize(); i++)
        {
          if(m_MenuItemList[i]->m_nID==uiCommandId)
          {
            CNewMenuItemData* pData = m_MenuItemList.GetAt(i);
            m_MenuItemList.RemoveAt(i);
            delete pData;
            break;
          }
        }
      }
      else
      {
        // Only remove the menu.
        int numSubMenus = (int)m_SubMenus.GetSize();
        while(numSubMenus--)
        {
          if(m_SubMenus[numSubMenus]==pSubMenu->m_hMenu)
          {
            m_SubMenus.RemoveAt(numSubMenus);
          }
        }
        numSubMenus = (int)m_MenuItemList.GetSize();
        while(numSubMenus--)
        {
          if(m_MenuItemList[numSubMenus]->m_nID==HMenuToUInt(pSubMenu->m_hMenu) )
          {
            CNewMenuItemData* pData = m_MenuItemList.GetAt(numSubMenus);
            m_MenuItemList.RemoveAt(numSubMenus);
            delete pData;
            break;
          }
        }
        // Don't delete it's only remove
        //delete pSubMenu;
      }
    }
  }
  else
  {
    int iPosition =0;
    CNewMenu* pMenu = FindMenuOption(uiId,iPosition);
    if(pMenu)
    {
      return pMenu->RemoveMenu(iPosition,MF_BYPOSITION);
    }
  }
  return CMenu::RemoveMenu(uiId,nFlags);
}

BOOL CNewMenu::DeleteMenu(UINT uiId, UINT nFlags)
{
  if(MF_BYPOSITION&nFlags)
  {
    UINT nItemState = GetMenuState(uiId,MF_BYPOSITION);
    if( (nItemState&MF_SEPARATOR) && !(nItemState&MF_POPUP))
    {
      CNewMenuItemData* pData = m_MenuItemList.GetAt(uiId);
      m_MenuItemList.RemoveAt(uiId);
      delete pData;
    }
    else
    {
      CMenu* pSubMenu = GetSubMenu(uiId);
      if(NULL==pSubMenu)
      {
        UINT uiCommandId = GetMenuItemID(uiId);
        for(int i=0;i<m_MenuItemList.GetSize(); i++)
        {
          if(m_MenuItemList[i]->m_nID==uiCommandId)
          {
            CNewMenuItemData* pData = m_MenuItemList.GetAt(i);
            m_MenuItemList.RemoveAt(i);
            delete pData;
          }
        }
      }
      else
      {
        BOOL bCanDelete = FALSE;
        int numSubMenus = (int)m_SubMenus.GetSize();
        while(numSubMenus--)
        {
          if(m_SubMenus[numSubMenus]==pSubMenu->m_hMenu)
          {
            m_SubMenus.RemoveAt(numSubMenus);
            bCanDelete = TRUE;
          }
        }

        numSubMenus = (int)m_MenuItemList.GetSize();
        while(numSubMenus--)
        {
          if(m_MenuItemList[numSubMenus]->m_nID==HMenuToUInt(pSubMenu->m_hMenu) )
          {
            CNewMenuItemData* pData = m_MenuItemList.GetAt(numSubMenus);
            m_MenuItemList.RemoveAt(numSubMenus);
            delete pData;
            break;
          }
        }
        // Did we created the menu
        if(bCanDelete)
        { // Oh yes so we can destroy it
          delete pSubMenu;
        }
      }
    }
  }
  else
  {
    int iPosition =0;
    CNewMenu* pMenu = FindMenuOption(uiId,iPosition);
    if(pMenu)
    {
      return pMenu->DeleteMenu(iPosition,MF_BYPOSITION);
    }
  }
  return CMenu::DeleteMenu(uiId,nFlags);
}

BOOL CNewMenu::AppendMenu(UINT nFlags, UINT nIDNewItem, LPCTSTR lpszNewItem, int nIconNormal)
{
  return AppendODMenu(lpszNewItem,nFlags,nIDNewItem,nIconNormal);
}

BOOL CNewMenu::AppendMenu(UINT nFlags, UINT nIDNewItem, LPCTSTR lpszNewItem, CImageList* il, int xoffset)
{
  return AppendODMenu(lpszNewItem,nFlags,nIDNewItem,il,xoffset);
}

BOOL CNewMenu::AppendMenu(UINT nFlags, UINT nIDNewItem, LPCTSTR lpszNewItem, CBitmap* bmp)
{
  return AppendODMenu(lpszNewItem,nFlags,nIDNewItem,bmp);
}

BOOL CNewMenu::InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, LPCTSTR lpszNewItem, int nIconNormal)
{
  return InsertODMenu(nPosition,lpszNewItem,nFlags,nIDNewItem,nIconNormal);
}

BOOL CNewMenu::InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, LPCTSTR lpszNewItem, CImageList* il, int xoffset)
{
  return InsertODMenu(nPosition,lpszNewItem,nFlags,nIDNewItem,il,xoffset);
}

BOOL CNewMenu::InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, LPCTSTR lpszNewItem, CBitmap* bmp)
{
  return InsertODMenu(nPosition,lpszNewItem,nFlags,nIDNewItem,bmp);
}

CNewMenu* CNewMenu::AppendODPopupMenu(LPCTSTR lpstrText)
{
  CNewMenu* pSubMenu = new CNewMenu(m_hMenu);
  pSubMenu->m_unselectcheck=m_unselectcheck;
  pSubMenu->m_selectcheck=m_selectcheck;
  pSubMenu->m_checkmaps=m_checkmaps;
  pSubMenu->m_checkmapsshare=TRUE;
  pSubMenu->CreatePopupMenu();
  if(AppendODMenu(lpstrText,MF_POPUP,HMenuToUInt(pSubMenu->m_hMenu), -1))
  {
    m_SubMenus.Add(pSubMenu->m_hMenu);
    return pSubMenu;
  }
  delete pSubMenu;
  return NULL;
}

CMenu* CNewMenu::GetSubMenu(int nPos) const
{
  return CMenu::GetSubMenu (nPos);
}

CMenu* CNewMenu::GetSubMenu(LPCTSTR lpszSubMenuName) const
{
  int num = GetMenuItemCount ();
  CString name;
  MENUITEMINFO info = {0};

  for (int i=0; i<num; i++)
  {
    GetMenuString (i, name, MF_BYPOSITION);
    // fix from George Menhorn
    if(name.IsEmpty())
    {
      info.cbSize = sizeof (MENUITEMINFO);
      info.fMask = MIIM_DATA;
      ::GetMenuItemInfo(m_hMenu, i, TRUE, &info);

      CNewMenuItemData* pItemData = CheckMenuItemData(info.dwItemData);
      if (pItemData)
      {
        name = pItemData->GetString(m_bDrawAccelerators ? m_hAccelToDraw : NULL);
      }
    }

    if (name.Compare (lpszSubMenuName) == 0)
    {
      return CMenu::GetSubMenu (i);
    }
  }
  return NULL;
}

// Tongzhe Cui, Functions to remove a popup menu based on its name. Seperators
// before and after the popup menu can also be removed if they exist.
int CNewMenu::GetMenuPosition(LPCTSTR pText)
{
  for(int i=0;i<(int)(GetMenuItemCount());++i)
  {
    if(!GetSubMenu(i))
    {
      for(int j=0;j<m_MenuItemList.GetSize();++j)
      {
        if(m_MenuItemList[j]->m_szMenuText.Compare(pText)==NULL)
        {
          return j;
        }
      }
    }
  }
  // means no found;
  return -1;
}

BOOL CNewMenu::RemoveMenuTitle()
{
  int numMenuItems = (int)m_MenuItemList.GetSize();

  // We need a seperator at the beginning of the menu
  if(!numMenuItems || !((m_MenuItemList[0]->m_nFlags)&MF_SEPARATOR) )
  {
    return FALSE;
  }
  CNewMenuItemData* pMenuData = m_MenuItemList[0];
  // Check for title
  if(pMenuData->m_nTitleFlags&MFT_TITLE)
  {
    if(numMenuItems>0)
    {
      CNewMenuItemData* pMenuNextData = m_MenuItemList[1];
      if((pMenuNextData->m_nFlags&MF_MENUBREAK))
      {
        pMenuNextData->m_nFlags &= ~MF_MENUBREAK;
        CMenu::ModifyMenu(1,MF_BYPOSITION|pMenuNextData->m_nFlags,pMenuNextData->m_nID,(LPCTSTR)pMenuNextData);
      }
    }
    // Now remove the title
    RemoveMenu(0,MF_BYPOSITION);
    return TRUE;
  }
  return FALSE;
}

BOOL CNewMenu::SetMenuTitle(LPCTSTR pTitle, UINT nTitleFlags, int nIconNormal)
{
  int nIndex = -1;
  CNewMenuIconLock iconLock(GetMenuIcon(nIndex,nIconNormal));
  return SetMenuTitle(pTitle,nTitleFlags,iconLock,nIndex);
}

BOOL CNewMenu::SetMenuTitle(LPCTSTR pTitle, UINT nTitleFlags, CBitmap* pBmp)
{
  int nIndex = -1;
  CNewMenuIconLock iconLock(GetMenuIcon(nIndex,pBmp));
  return SetMenuTitle(pTitle,nTitleFlags,iconLock,nIndex);
}

BOOL CNewMenu::SetMenuTitle(LPCTSTR pTitle, UINT nTitleFlags, CImageList *pil, int xoffset)
{
  int nIndex = -1;
  CNewMenuIconLock iconLock(GetMenuIcon(nIndex,0,pil,xoffset));
  return SetMenuTitle(pTitle,nTitleFlags,iconLock,nIndex);
}

BOOL CNewMenu::SetMenuTitle(LPCTSTR pTitle, UINT nTitleFlags, CNewMenuIcons* pIcons, int nIndex)
{
  // Check if menu is valid
  if(!::IsMenu(m_hMenu))
  {
    return FALSE;
  }
  // Check the menu integrity
  if((int)GetMenuItemCount()!=(int)m_MenuItemList.GetSize())
  {
    SynchronizeMenu();
  }
  int numMenuItems = (int)m_MenuItemList.GetSize();
  // We need a seperator at the beginning of the menu
  if(!numMenuItems || !((m_MenuItemList[0]->m_nFlags)&MF_SEPARATOR) )
  {
    // We add the special menu item for title
    CNewMenuItemData *pItemData = new CNewMenuItemDataTitle;
    m_MenuItemList.InsertAt(0,pItemData);
    pItemData->SetString(pTitle);
    pItemData->m_nFlags = MF_BYPOSITION|MF_SEPARATOR|MF_OWNERDRAW;

    VERIFY(CMenu::InsertMenu(0,MF_SEPARATOR|MF_BYPOSITION,0,pItemData->m_szMenuText));
    VERIFY(CMenu::ModifyMenu(0, MF_BYPOSITION|MF_SEPARATOR|MF_OWNERDRAW, 0, (LPCTSTR)pItemData ));

    //InsertMenu(0,MF_SEPARATOR|MF_BYPOSITION);
  }

  numMenuItems = (int)m_MenuItemList.GetSize();
  if(numMenuItems)
  {
    CNewMenuItemData* pMenuData = m_MenuItemList[0];
    if(pMenuData->m_nFlags&MF_SEPARATOR)
    {
      pIcons->AddRef();
      pMenuData->m_pMenuIcon->Release();
      pMenuData->m_pMenuIcon = pIcons;
      if(pIcons && nIndex>=0)
      {
        pMenuData->m_nMenuIconOffset = nIndex;
        //CSize size = pIcons->GetIconSize();
        //m_iconX = max(m_iconX,size.cx);
        //m_iconY = max(m_iconY,size.cy);
      }
      else
      {
        pMenuData->m_nMenuIconOffset = -1;
      }
      pMenuData->SetString(pTitle);
      pMenuData->m_nTitleFlags = nTitleFlags|MFT_TITLE;

      if(numMenuItems>1)
      {
        CNewMenuItemData* pMenuData = m_MenuItemList[1];

        if(nTitleFlags&MFT_SIDE_TITLE)
        {
          if(!(pMenuData->m_nFlags&MF_MENUBREAK))
          {
            pMenuData->m_nFlags |= MF_MENUBREAK;
            CMenu::ModifyMenu(1,MF_BYPOSITION|pMenuData->m_nFlags,pMenuData->m_nID,(LPCTSTR)pMenuData);
          }
        }
        else
        {
          if((pMenuData->m_nFlags&MF_MENUBREAK))
          {
            pMenuData->m_nFlags &= ~MF_MENUBREAK;
            CMenu::ModifyMenu(1,MF_BYPOSITION|pMenuData->m_nFlags,pMenuData->m_nID,(LPCTSTR)pMenuData);
          }
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

CNewMenuItemDataTitle* CNewMenu::GetMemuItemDataTitle()
{
  // Check if menu is valid
  if(!this || !::IsMenu(m_hMenu))
  {
    return NULL;
  }
  // Check the menu integrity
  if((int)GetMenuItemCount()!=(int)m_MenuItemList.GetSize())
  {
    SynchronizeMenu();
  }

  if(m_MenuItemList.GetSize()>0)
  {
    return DYNAMIC_DOWNCAST(CNewMenuItemDataTitle,m_MenuItemList[0]);
  }
  return NULL;
}


BOOL CNewMenu::SetMenuTitleColor(COLORREF clrTitle, COLORREF clrLeft, COLORREF clrRight)
{
  CNewMenuItemDataTitle* pItem = GetMemuItemDataTitle();
  if(pItem)
  {
    pItem->m_clrTitle = clrTitle;
    pItem->m_clrLeft = clrLeft;
    pItem->m_clrRight = clrRight;
    return TRUE;
  }
  return FALSE;
}

BOOL CNewMenu::SetMenuText(UINT id, CString string, UINT nFlags/*= MF_BYPOSITION*/ )
{
  if(MF_BYPOSITION&nFlags)
  {
    int numMenuItems = (int)m_MenuItemList.GetSize();
    if(id<UINT(numMenuItems))
    {
      // get current menu state so it doesn't change
      UINT nState = GetMenuState(id, MF_BYPOSITION);
      nState &= ~(MF_BITMAP|MF_OWNERDRAW|MF_SEPARATOR);
      // change the menutext
      CNewMenuItemData* pItemData = m_MenuItemList[id];
      pItemData->SetString(string);

      if(CMenu::ModifyMenu(id,MF_BYPOSITION|MF_STRING | nState, pItemData->m_nID, string))
      {
        return ModifyMenu(id,MF_BYPOSITION | MF_OWNERDRAW,pItemData->m_nID,(LPCTSTR)pItemData);
      }
    }
  }
  else
  {
    int uiLoc;
    CNewMenu* pMenu = FindMenuOption(id,uiLoc);
    if(NULL!=pMenu)
    {
      return pMenu->SetMenuText(uiLoc,string);
    }
  }
  return FALSE;
}

// courtesy of Warren Stevens
void CNewMenu::ColorBitmap(CDC* pDC, CBitmap& bmp, CSize size, COLORREF fill, COLORREF border, int hatchstyle)
{
  // Create a memory DC
  CDC MemDC;
  MemDC.CreateCompatibleDC(pDC);
  bmp.CreateCompatibleBitmap(pDC, size.cx, size.cy);
  CPen border_pen(PS_SOLID, 1, border);

  CBrush fill_brush;
  if(hatchstyle!=-1)
  {
    fill_brush.CreateHatchBrush(hatchstyle, fill);
  }
  else
  {
    fill_brush.CreateSolidBrush(fill);
  }

  CBitmap* pOldBitmap = MemDC.SelectObject(&bmp);
  CPen*    pOldPen    = MemDC.SelectObject(&border_pen);
  CBrush*  pOldBrush  = MemDC.SelectObject(&fill_brush);

  MemDC.Rectangle(0,0, size.cx, size.cy);

  if(NULL!=pOldBrush)  { MemDC.SelectObject(pOldBrush);  }
  if(NULL!=pOldPen)    { MemDC.SelectObject(pOldPen);    }
  if(NULL!=pOldBitmap) { MemDC.SelectObject(pOldBitmap); }
}

void CNewMenu::DrawSpecial_OldStyle(CDC* pDC, LPCRECT pRect, UINT nID, DWORD dwStyle)
{
  COLORREF oldColor;
  if( (dwStyle&ODS_GRAYED) || (dwStyle&ODS_INACTIVE))
  {
    oldColor = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  }
  else if (dwStyle&ODS_SELECTED)
  {
    oldColor = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
  }
  else
  {
    oldColor = pDC->SetTextColor(::GetSysColor(COLOR_MENUTEXT));
  }
  BOOL bBold = (dwStyle&ODS_DEFAULT) ? TRUE : FALSE;

  switch(nID&0xfff0)
  {
  case SC_MINIMIZE:
    DrawSpecialChar(pDC,pRect,48,bBold); // Min
    break;
  case SC_MAXIMIZE:
    DrawSpecialChar(pDC,pRect,49,bBold); // Max
    break;
  case SC_CLOSE:
    DrawSpecialChar(pDC,pRect,114,bBold); // Close
    break;
  case SC_RESTORE:
    DrawSpecialChar(pDC,pRect,50,bBold); // restore
    break;
  }
  pDC->SetTextColor(oldColor);
}

void CNewMenu::DrawSpecial_WinXP(CDC* pDC, LPCRECT pRect, UINT nID, DWORD dwStyle)
{
  TCHAR cSign = 0;
  switch(nID&0xfff0)
  {
  case SC_MINIMIZE:
    cSign = 48; // Min
    break;
  case SC_MAXIMIZE:
    cSign = 49;// Max
    break;
  case SC_CLOSE:
    cSign = 114;// Close
    break;
  case SC_RESTORE:
    cSign = 50;// Restore
    break;
  }
  if(cSign)
  {
    COLORREF oldColor;
    BOOL bBold = (dwStyle&ODS_DEFAULT) ? TRUE : FALSE;
    CRect rect(pRect);
    rect.InflateRect(0,(m_iconY-rect.Height())>>1);


    if( (dwStyle&ODS_GRAYED) || (dwStyle&ODS_INACTIVE))
    {
      oldColor = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    }
    else if(dwStyle&ODS_SELECTED)
    {
      oldColor = pDC->SetTextColor(DarkenColorXP(GetXpHighlightColor()));
      rect.OffsetRect(1,1);
      DrawSpecialChar(pDC,rect,cSign,bBold);
      pDC->SetTextColor(::GetSysColor(COLOR_MENUTEXT));
      rect.OffsetRect(-2,-2);
    }
    else
    {
      oldColor = pDC->SetTextColor(::GetSysColor(COLOR_MENUTEXT));
    }
    DrawSpecialChar(pDC,rect,cSign,bBold);

    pDC->SetTextColor(oldColor);
  }
}

CRect CNewMenu::GetLastActiveMenuRect()
{
  return m_LastActiveMenuRect;
}

void CNewMenu::DrawSpecialCharStyle(CDC* pDC, LPCRECT pRect, TCHAR Sign, DWORD dwStyle)
{
  COLORREF oldColor;
  if( (dwStyle&ODS_GRAYED) || (dwStyle&ODS_INACTIVE))
  {
    oldColor = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  }
  else
  {
    oldColor = pDC->SetTextColor(::GetSysColor(COLOR_MENUTEXT));
  }

  DrawSpecialChar(pDC,pRect,Sign,(dwStyle&ODS_DEFAULT) ? TRUE : FALSE);

  pDC->SetTextColor(oldColor);
}

void CNewMenu::DrawSpecialChar(CDC* pDC, LPCRECT pRect, TCHAR Sign, BOOL bBold)
{
  //  48 Min
  //  49 Max
  //  50 Restore
  //  98 Checkmark
  // 105 Bullet
  // 114 Close

  CFont MyFont;
  LOGFONT logfont;

  CRect rect(pRect);
  rect.DeflateRect(2,2);

  logfont.lfHeight = -rect.Height();
  logfont.lfWidth = 0;
  logfont.lfEscapement = 0;
  logfont.lfOrientation = 0;
  logfont.lfWeight = (bBold) ? FW_BOLD:FW_NORMAL;
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

  CFont* pOldFont = pDC->SelectObject (&MyFont);
  int OldMode = pDC->SetBkMode(TRANSPARENT);

  pDC->DrawText (&Sign,1,rect,DT_CENTER|DT_SINGLELINE);

  pDC->SetBkMode(OldMode);
  pDC->SelectObject(pOldFont);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMenuTheme::CMenuTheme()
: m_dwThemeId(0),
m_dwFlags(0),
m_pMeasureItem(NULL),
m_pDrawItem(NULL),
m_pDrawTitle(NULL),
m_BorderTopLeft(0,0),
m_BorderBottomRight(0,0)
{
}

CMenuTheme::CMenuTheme( DWORD dwThemeId,
                       pItemMeasureFkt pMeasureItem,
                       pItemDrawFkt pDrawItem,
                       pItemDrawFkt pDrawTitle,
                       DWORD dwFlags)
                       :m_dwThemeId(dwThemeId),
                       m_dwFlags(dwFlags),
                       m_pMeasureItem(pMeasureItem),
                       m_pDrawItem(pDrawItem),
                       m_pDrawTitle(pDrawTitle)
{
  UpdateSysColors();
  UpdateSysMetrics();
}

CMenuTheme::~CMenuTheme()
{
}

void CMenuTheme::DrawFrame(CDC* pDC, CRect rectOuter, CRect rectInner, COLORREF crBorder)
{
  CRect Temp;
  rectInner.right -= 1;
  // Border top
  Temp.SetRect(rectOuter.TopLeft(),CPoint(rectOuter.right,rectInner.top));
  pDC->FillSolidRect(Temp,crBorder);
  // Border bottom
  Temp.SetRect(CPoint(rectOuter.left,rectInner.bottom),rectOuter.BottomRight());
  pDC->FillSolidRect(Temp,crBorder);

  // Border left
  Temp.SetRect(rectOuter.TopLeft(),CPoint(rectInner.left,rectOuter.bottom));
  pDC->FillSolidRect(Temp,crBorder);
  // Border right
  Temp.SetRect(CPoint(rectInner.right,rectOuter.top),rectOuter.BottomRight());
  pDC->FillSolidRect(Temp,crBorder);
}

BOOL CMenuTheme::DoDrawBorder()
{
  return (m_dwFlags&1)?TRUE:FALSE;
}

void CMenuTheme::DrawSmalBorder( HWND hWnd, HDC hDC)
{
  CNewMenuHook::CMenuHookData* pData = CNewMenuHook::GetMenuHookData(hWnd);
  if(pData!=NULL)
  {
    if(pData->m_hMenu)
    {
      CNewMenu* pMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pData->m_hMenu));
      if(pMenu && pMenu->GetParent())
      {
        CNewMenu* pParentMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(pMenu->GetParent()));
        if(pParentMenu && !pParentMenu->IsPopup())
        {
          CRect Rect;
          // Get the size of the menu...
          GetWindowRect(hWnd, Rect );
          Rect.OffsetRect(pData->m_Point - Rect.TopLeft());
          Rect &= pParentMenu->GetLastActiveMenuRect();
          if(!Rect.IsRectEmpty())
          {
            if(Rect.Width()>Rect.Height())
            {
              Rect.InflateRect(-1,0);
            }
            else
            {
              Rect.InflateRect(0,-1);
            }
            Rect.OffsetRect(-pData->m_Point);
            CDC* pDC = CDC::FromHandle(hDC);

            COLORREF colorSmalBorder;
            if (NumScreenColors() > 256)
            {
              colorSmalBorder = MixedColor(CNewMenu::GetMenuBarColor(),GetSysColor(COLOR_WINDOW));
            }
            else
            {
              colorSmalBorder = GetSysColor(COLOR_BTNFACE);
            }
            pDC->FillSolidRect(Rect,colorSmalBorder);
          }
        }
      }
    }
  }
}

void CMenuTheme::DrawShade( HWND hWnd, HDC hDC, CPoint screen)
{
  if(IsShadowEnabled())
    return;

  // Get the size of the menu...
  CRect Rect;
  GetWindowRect(hWnd, Rect );

  long winW = Rect.Width();
  long winH = Rect.Height();
  long xOrg = screen.x;
  long yOrg = screen.y;

  CNewMenuHook::CMenuHookData* pData = CNewMenuHook::GetMenuHookData(hWnd);

  CDC* pDC = CDC::FromHandle(hDC);
  CDC memDC;
  memDC.CreateCompatibleDC(pDC);
  CBitmap* pOldBitmap = memDC.SelectObject(&pData->m_Screen);

  HDC hDcDsk = memDC.m_hDC;
  xOrg = 0;
  yOrg = 0;

  //// Get the desktop hDC...
  //HDC hDcDsk = GetWindowDC(0);

  int X,Y;
  // Simulate a shadow on right edge...
  if (NumScreenColors() <= 256)
  {
    DWORD rgb = ::GetSysColor(COLOR_BTNSHADOW);
    BitBlt(hDC,winW-2,0,    2,winH,hDcDsk,xOrg+winW-2,0,SRCCOPY);
    BitBlt(hDC,0,winH-2,winW,2,hDcDsk,0,yOrg+winH-2,SRCCOPY);
    for (X=3; X<=4 ;X++)
    {
      for (Y=0; Y<4 ;Y++)
      {
        SetPixel(hDC,winW-X,Y,GetPixel(hDcDsk,xOrg+winW-X,yOrg+Y));
      }
      for (Y=4; Y<8 ;Y++)
      {
        SetPixel(hDC,winW-X,Y,rgb);
      }
      for (Y=8; Y<=(winH-5) ;Y++)
      {
        SetPixel( hDC, winW - X, Y, rgb);
      }
      for (Y=(winH-4); Y<=(winH-3) ;Y++)
      {
        SetPixel( hDC, winW - X, Y, rgb);
      }
    }
    // Simulate a shadow on the bottom edge...
    for(Y=3; Y<=4 ;Y++)
    {
      for(X=0; X<=3 ;X++)
      {
        SetPixel(hDC,X,winH-Y, GetPixel(hDcDsk,xOrg+X,yOrg+winH-Y));
      }
      for(X=4; X<=7 ;X++)
      {
        SetPixel( hDC, X, winH - Y, rgb);
      }
      for(X=8; X<=(winW-5) ;X++)
      {
        SetPixel( hDC, X, winH - Y, rgb);
      }
    }
  }
  else
  {
    for (X=1; X<=4 ;X++)
    {
      for (Y=0; Y<4 ;Y++)
      {
        SetPixel(hDC,winW-X,Y, GetPixel(hDcDsk,xOrg+winW-X,yOrg+Y) );
      }
      for (Y=4; Y<8 ;Y++)
      {
        COLORREF c = GetPixel(hDcDsk, xOrg + winW - X, yOrg + Y);
        SetPixel(hDC,winW-X,Y,DarkenColor(2* 3 * X * (Y - 3), c));
      }
      for (Y=8; Y<=(winH-5) ;Y++)
      {
        COLORREF c = GetPixel(hDcDsk, xOrg + winW - X, yOrg + Y);
        SetPixel( hDC, winW - X, Y, DarkenColor(2* 15 * X, c) );
      }
      for (Y=(winH-4); Y<=(winH-1) ;Y++)
      {
        COLORREF c = GetPixel(hDcDsk, xOrg + winW - X, yOrg + Y);
        SetPixel( hDC, winW - X, Y, DarkenColor(2* 3 * X * -(Y - winH), c));
      }
    }

    // Simulate a shadow on the bottom edge...
    for(Y=1; Y<=4 ;Y++)
    {
      for(X=0; X<=3 ;X++)
      {
        SetPixel(hDC,X,winH-Y, GetPixel(hDcDsk,xOrg+X,yOrg+winH-Y));
      }
      for(X=4; X<=7 ;X++)
      {
        COLORREF c = GetPixel(hDcDsk, xOrg + X, yOrg + winH - Y);
        SetPixel( hDC, X, winH - Y, DarkenColor(2*3 * (X - 3) * Y, c));
      }
      for(X=8; X<=(winW-5) ;X++)
      {
        COLORREF  c = GetPixel(hDcDsk, xOrg + X, yOrg + winH - Y);
        SetPixel( hDC, X, winH - Y, DarkenColor(2* 15 * Y, c));
      }
    }
  }

  memDC.SelectObject(pOldBitmap);

  //// Release the desktop hDC...
  //ReleaseDC(0,hDcDsk);
}

#define CX_GRIPPER  3
#define CY_GRIPPER  3
#define CX_BORDER_GRIPPER 2
#define CY_BORDER_GRIPPER 2

void CMenuTheme::UpdateSysColors()
{
	clrBtnFace = ::GetSysColor(COLOR_BTNFACE);
	clrBtnShadow = ::GetSysColor(COLOR_BTNSHADOW);
	clrBtnHilite = ::GetSysColor(COLOR_BTNHIGHLIGHT);
	clrBtnText = ::GetSysColor(COLOR_BTNTEXT);
	clrWindowFrame = ::GetSysColor(COLOR_WINDOWFRAME);
}

void CMenuTheme::UpdateSysMetrics()
{
  m_BorderTopLeft = CSize(2,2);
  if(!IsShadowEnabled())
  {
    m_BorderBottomRight = CSize(5,6);
  }
  else
  {
    m_BorderBottomRight = CSize(2,2);
  }
}

void CMenuTheme::GetBarColor(COLORREF &clrUpperColor, COLORREF &clrMediumColor, COLORREF &clrBottomColor, COLORREF &clrDarkLine)
{
  if(IsMenuThemeActive())
  {
    COLORREF menuColor = CNewMenu::GetMenuBarColor2003();
    // corrections from Andreas Schärer
    switch(menuColor)
    {
    case RGB(163,194,245)://blau
      {
        clrUpperColor = RGB(221,236,254);
        clrMediumColor = RGB(196, 219,249);
        clrBottomColor = RGB(129,169,226);
        clrDarkLine = RGB(59,97,156);
        break;
      }
    case RGB(215,215,229)://silber
      {
        clrUpperColor = RGB(243,244,250);
        clrMediumColor = RGB(225,226,236);
        clrBottomColor = RGB(153,151,181);
        clrDarkLine = RGB(124,124,148);
        break;
      }
    case RGB(218,218,170)://olivgrün
      {
        clrUpperColor = RGB(244,247,222);
        clrMediumColor = RGB(209,222,172);
        clrBottomColor = RGB(183,198,145);
        clrDarkLine = RGB(96,128,88);
        break;
      }
    default:
      {
        clrUpperColor = LightenColor(140,menuColor);
        clrMediumColor = LightenColor(115,menuColor);
        clrBottomColor = DarkenColor(40,menuColor);
        clrDarkLine = DarkenColor(110,menuColor);
        break;
      }
    }
  }
  else
  {
    COLORREF menuColor, menuColor2;
    CNewMenu::GetMenuBarColor2003(menuColor, menuColor2, FALSE);
    clrUpperColor = menuColor2;
    clrBottomColor = menuColor;
    clrMediumColor = GetAlphaBlendColor(clrBottomColor, clrUpperColor,100);
    clrDarkLine = DarkenColor(50,clrBottomColor);
  }
}

void CMenuTheme::PaintCorner(CDC *pDC, LPCRECT pRect, COLORREF color)
{
  pDC->SetPixel(pRect->left+1,pRect->top  ,color);
  pDC->SetPixel(pRect->left+0,pRect->top  ,color);
  pDC->SetPixel(pRect->left+0,pRect->top+1,color);

  pDC->SetPixel(pRect->left+0,pRect->bottom  ,color);
  pDC->SetPixel(pRect->left+0,pRect->bottom-1,color);
  pDC->SetPixel(pRect->left+1,pRect->bottom  ,color);

  pDC->SetPixel(pRect->right-1,pRect->top  ,color);
  pDC->SetPixel(pRect->right  ,pRect->top  ,color);
  pDC->SetPixel(pRect->right  ,pRect->top+1,color);

  pDC->SetPixel(pRect->right-1,pRect->bottom  ,color);
  pDC->SetPixel(pRect->right  ,pRect->bottom  ,color);
  pDC->SetPixel(pRect->right  ,pRect->bottom-1,color);
}


void CMenuTheme::DrawGripper(CDC* pDC, const CRect& rect, DWORD dwStyle, 	
                             int m_cxLeftBorder, int m_cxRightBorder,
                             int m_cyTopBorder, int m_cyBottomBorder)
{
  // only draw the gripper if not floating and gripper is specified
  if ((dwStyle & (CBRS_GRIPPER|CBRS_FLOATING)) == CBRS_GRIPPER)
  {
    // draw the gripper in the border
    switch (CNewMenu::GetMenuDrawMode())
    {
    case CNewMenu::STYLE_XP:
    case CNewMenu::STYLE_XP_NOBORDER:
      {
        COLORREF col = DarkenColorXP(CNewMenu::GetMenuBarColorXP());
        CPen pen(PS_SOLID,0,col);
        CPen* pOld = pDC->SelectObject(&pen);
        if (dwStyle & CBRS_ORIENT_HORZ)
        {
          for (int n=rect.top+m_cyTopBorder+2*CY_BORDER_GRIPPER;n<rect.Height()-m_cyTopBorder-m_cyBottomBorder-4*CY_BORDER_GRIPPER;n+=2)
          {
            pDC->MoveTo(rect.left+CX_BORDER_GRIPPER+2,n);
            pDC->LineTo(rect.left+CX_BORDER_GRIPPER+CX_GRIPPER+2,n);
          }
        }
        else
        {
          for (int n=rect.top+m_cxLeftBorder+2*CX_BORDER_GRIPPER;n<rect.Width()-m_cxLeftBorder-m_cxRightBorder-2*CX_BORDER_GRIPPER;n+=2)
          {
            pDC->MoveTo(n,rect.top+CY_BORDER_GRIPPER+2);
            pDC->LineTo(n,rect.top+CY_BORDER_GRIPPER+CY_GRIPPER+2);
          }
        }
        pDC->SelectObject(pOld);
      }
      break;

    case CNewMenu::STYLE_ICY:
    case CNewMenu::STYLE_ICY_NOBORDER:
      {
        COLORREF color = DarkenColor(100,CNewMenu::GetMenuColor());
        if (dwStyle & CBRS_ORIENT_HORZ)
        {
          for (int n=rect.top+m_cyTopBorder+2*CY_BORDER_GRIPPER;n<rect.Height()-m_cyTopBorder-m_cyBottomBorder-4*CY_BORDER_GRIPPER;n+=4)
          {
            pDC->FillSolidRect(rect.left+CX_BORDER_GRIPPER+2+m_cxLeftBorder,n  +1,2,2,color);
          }
        }
        else
        {
          for (int n=rect.top+m_cxLeftBorder+2*CX_BORDER_GRIPPER;n<rect.Width()-m_cxRightBorder-2*CX_BORDER_GRIPPER;n+=4)
          {
            pDC->FillSolidRect(n,  rect.top+CY_BORDER_GRIPPER+2+m_cxLeftBorder,2,2,color);
          }
        }
      }
      break;

    case CNewMenu::STYLE_XP_2003_NOBORDER:
    case CNewMenu::STYLE_XP_2003:
    case CNewMenu::STYLE_COLORFUL_NOBORDER:
    case CNewMenu::STYLE_COLORFUL:
      {
        COLORREF clrUpperColor, clrMediumColor, clrBottomColor, clrDarkLine;
        CNewMenu::GetActualMenuTheme()->GetBarColor(clrUpperColor,clrMediumColor,clrBottomColor,clrDarkLine);

        COLORREF color1 = GetSysColor(COLOR_WINDOW);
        //COLORREF color2 = DarkenColor(40,MidColor(GetSysColor(COLOR_BTNFACE),color1));

        if (dwStyle & CBRS_ORIENT_HORZ)
        {
          for (int n=rect.top+m_cyTopBorder+2*CY_BORDER_GRIPPER;n<rect.Height()-m_cyTopBorder-m_cyBottomBorder-4*CY_BORDER_GRIPPER;n+=4)
          {
            pDC->FillSolidRect(rect.left+CX_BORDER_GRIPPER+3+m_cxLeftBorder,n+1+1,2,2,color1);
            pDC->FillSolidRect(rect.left+CX_BORDER_GRIPPER+2+m_cxLeftBorder,n  +1,2,2,clrDarkLine);
          }
        }
        else
        {
          for (int n=rect.top+m_cxLeftBorder+2*CX_BORDER_GRIPPER;n<rect.Width()-m_cxRightBorder-2*CX_BORDER_GRIPPER;n+=4)
          {
            pDC->FillSolidRect(n+1,rect.top+CY_BORDER_GRIPPER+3+m_cxLeftBorder,2,2,color1);
            pDC->FillSolidRect(n,  rect.top+CY_BORDER_GRIPPER+2+m_cxLeftBorder,2,2,clrDarkLine);
          }
        }
      }
      break;

    default:
      {
        //BPO late this can be eliminated (cached?)
	      clrBtnShadow = ::GetSysColor(COLOR_BTNSHADOW);
	      clrBtnHilite = ::GetSysColor(COLOR_BTNHIGHLIGHT);

        CRect temp = rect;
        //temp.left+=2;
        if (dwStyle & CBRS_ORIENT_HORZ)
        {
          pDC->Draw3dRect(rect.left+CX_BORDER_GRIPPER+m_cxLeftBorder,
                          rect.top+m_cyTopBorder,
                          CX_GRIPPER, rect.Height()-m_cyTopBorder-m_cyBottomBorder,
                          clrBtnHilite, clrBtnShadow);
        }
        else
        {
          temp.top+=2;
          pDC->Draw3dRect(rect.left+m_cyTopBorder,
                          rect.top+CY_BORDER_GRIPPER,
                          rect.Width()-m_cyTopBorder-m_cyBottomBorder, CY_GRIPPER,
                          clrBtnHilite, clrBtnShadow);
        }
      }
      break;
    }
  }
}

void CMenuTheme::DrawCorner(CDC* pDC, LPCRECT pRect, DWORD dwStyle)
{
  if ((dwStyle & (CBRS_GRIPPER|CBRS_FLOATING)) == CBRS_GRIPPER)
  {
    // draw the gripper in the border
    switch (CNewMenu::GetMenuDrawMode())
    {
    case CNewMenu::STYLE_ICY:
    case CNewMenu::STYLE_ICY_NOBORDER:
      {
        // make round corners
        COLORREF color = GetSysColor(COLOR_3DLIGHT);
        PaintCorner(pDC,pRect,color);
      }
      break;

    case CNewMenu::STYLE_XP_2003_NOBORDER:
    case CNewMenu::STYLE_XP_2003:
    case CNewMenu::STYLE_COLORFUL_NOBORDER:
    case CNewMenu::STYLE_COLORFUL:
      {
        // make round corners
        COLORREF color = CNewMenu::GetMenuBarColor2003();
        PaintCorner(pDC,pRect,color);
      }
      break;
    }
  }
}

void SetWindowShade(HWND hWndMenu, CNewMenuHook::CMenuHookData* pData );

#define CMS_MENUFADE            175
#define MENU_TIMER_ID 0x1234

BOOL CMenuTheme::OnInitWnd(HWND hWnd)
{
  CNewMenuHook::CMenuHookData* pData = CNewMenuHook::GetMenuHookData(hWnd);
  ASSERT(pData);
  if( pData->m_bDoSubclass)
  {
    if(!DoDrawBorder())
    {
      if(g_Shell>=Win2000 && CNewMenu::GetAlpha()!=255)
      {
        // Flag for changing styles
        pData->m_dwData |= 2;
        SetWindowLongPtr (hWnd, GWL_EXSTYLE,(pData->m_dwExStyle| WS_EX_LAYERED));
        SetLayeredWindowAttributes(hWnd, 0, CNewMenu::GetAlpha(), LWA_ALPHA);
        pData->m_dwData |= 4;
        SetTimer(hWnd,MENU_TIMER_ID,50,NULL);
      }
    }
    else
    {
      // Flag for changing styles
      pData->m_dwData |= 2;

      SetWindowLongPtr (hWnd, GWL_STYLE, pData->m_dwStyle & (~WS_BORDER) );

      if(g_Shell>=Win2000 && CNewMenu::GetAlpha()!=255)
      {
        SetWindowLongPtr (hWnd, GWL_EXSTYLE,(pData->m_dwExStyle| WS_EX_LAYERED) & ~(WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME));
        SetLayeredWindowAttributes(hWnd, 0, CNewMenu::GetAlpha(), LWA_ALPHA);
        pData->m_dwData |= 4;
        SetTimer(hWnd,MENU_TIMER_ID,50,NULL);
        //RedrawWindow(hWnd,NULL,NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);  
      }
      else
      {
        SetWindowLongPtr (hWnd, GWL_EXSTYLE,pData->m_dwExStyle & ~(WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME));
      }

      //if(!IsShadowEnabled())
      //{
      //  BOOL bAnimate = FALSE;
      //  if(SystemParametersInfo(SPI_GETMENUANIMATION,0,&bAnimate,0) && bAnimate==TRUE)
      //  {
      //    // All menuanimation should be terminated after 200 ms
      //    pData->m_TimerID = GetSafeTimerID(hWnd,200);
      //  }
      //  else
      //  {
      //    pData->m_TimerID = GetSafeTimerID(hWnd,500);
      //    //SetWindowShade(hWnd, pData);
      //  }
      //}

      return TRUE;
    }
  }
  return FALSE;
}

BOOL CMenuTheme::OnUnInitWnd(HWND hWnd)
{
  CNewMenuHook::CMenuHookData* pData = CNewMenuHook::GetMenuHookData(hWnd);
  if(pData)
  {
    if(pData->m_dwData&4)
    {
      KillTimer(hWnd,MENU_TIMER_ID);
      pData->m_dwData &= ~4;
    }
    HMENU hMenu = pData->m_hMenu;
    if(IsMenu(hMenu))
    {
      CNewMenu* pNewMenu = DYNAMIC_DOWNCAST(CNewMenu,CMenu::FromHandlePermanent(hMenu));

      if(pNewMenu)
      {
        // Redraw menubar on place.
        pNewMenu->OnUnInitPopupMenu();
      }
    }

    //// Destroy object and window for shading support
    //if(pData->m_TimerID)
    //{
    //  KillTimer(hWnd,pData->m_TimerID);
    //  pData->m_TimerID = NULL;
    //}
    //if(pData->m_hRightShade)
    //{
    //  DestroyWindow(pData->m_hRightShade);
    //  pData->m_hRightShade = NULL;
    //}
    //if(pData->m_hBottomShade)
    //{
    //  DestroyWindow(pData->m_hBottomShade);
    //  pData->m_hBottomShade = NULL;
    //}

    // were windows-style changed?
    if(pData->m_dwData&2)
    {
      SetLastError(0);
      if(!(pData->m_dwData&1))
      {
        SetWindowLongPtr (hWnd, GWL_STYLE,pData->m_dwStyle);
        ShowLastError(_T("Error from Menu: SetWindowLongPtr I"));
      }
      else
      {
        // Restore old Styles for special menu!!
        // (Menu 0x10012!!!) special VISIBLE flag must be set
        SetWindowLongPtr (hWnd, GWL_STYLE,pData->m_dwStyle|WS_VISIBLE);
        ShowLastError(_T("Error from Menu: SetWindowLongPtr I, Special"));
      }

      SetWindowLongPtr (hWnd, GWL_EXSTYLE, pData->m_dwExStyle);
      ShowLastError(_T("Error from Menu: SetWindowLongPtr II"));
      // Normaly when you change the style you shold call next function
      // but in this case you would lose the focus for the menu!!
      //SetWindowPos(hWnd,0,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED|SWP_HIDEWINDOW);
    }
  }
  return TRUE;
}

BOOL CMenuTheme::OnEraseBkgnd(HWND hWnd, HDC hDC)
{
  //  CNewMenuHook::CMenuHookData* pData = CNewMenuHook::GetMenuHookData(hWnd);

  //  Get the size of the menu...
  CDC* pDC = CDC::FromHandle (hDC);
  CRect Rect;
  GetClientRect(hWnd, Rect );
  if(DoDrawBorder())
  {
    Rect.InflateRect(+2,0,-1,0);
  }
  else
  {
    Rect.InflateRect(+2,0,0,0);
  }

  pDC->FillSolidRect (Rect,CNewMenu::GetMenuColor(0));//GetSysColor(COLOR_MENU));

  return TRUE;
}

BOOL CMenuTheme::OnWindowPosChanging(HWND hWnd, LPWINDOWPOS pPos)
{
  UNREFERENCED_PARAMETER(hWnd);

  if(DoDrawBorder())
  {
#ifdef _TRACE_MENU_
    AfxTrace(_T("WindowPosChanging hwnd=0x%lX, (%ld,%ld)(%ld,%ld)\n"),hWnd,pPos->x,pPos->y,pPos->cx,pPos->cy);
#endif

    if(!IsShadowEnabled())
    {
      pPos->cx +=2;
      pPos->cy +=2;
    }
    else
    {
      pPos->cx -=2;
      pPos->cy -=2;
    }
    pPos->y -=1;

#ifdef _TRACE_MENU_
    AfxTrace(_T("WindowPosChanged  hwnd=0x%lX, (%ld,%ld)(%ld,%ld)\n"),hWnd,pPos->x,pPos->y,pPos->cx,pPos->cy);
#endif

    return TRUE;
  }
  return FALSE;
}

BOOL CMenuTheme::OnNcCalcSize(HWND hWnd, NCCALCSIZE_PARAMS* pCalc)
{
  UNREFERENCED_PARAMETER(hWnd);

  if(DoDrawBorder())
  {
#ifdef _TRACE_MENU_
    AfxTrace(_T("OnNcCalcSize 0 hwnd=0x%lX, (top=%ld,bottom=%ld,left=%ld,right=%ld)\n"),hWnd,pCalc->rgrc->top,pCalc->rgrc->bottom,pCalc->rgrc->left,pCalc->rgrc->right);
#endif

    int cx=0,cy=0;
    if(!IsShadowEnabled())
    {
      cx = 5;
      cy = 6;
    }
    else
    {
      cx = 1;
      cy = 2;
    }

    pCalc->rgrc->top  += m_BorderTopLeft.cy;
    pCalc->rgrc->left += m_BorderTopLeft.cx;
    pCalc->rgrc->bottom -= cy;
    pCalc->rgrc->right  -= cx;

#ifdef _TRACE_MENU_
    AfxTrace(_T("OnNcCalcSize 0 hwnd=0x%lX, (top=%ld,bottom=%ld,left=%ld,right=%ld)\n"),hWnd,pCalc->rgrc->top,pCalc->rgrc->bottom,pCalc->rgrc->left,pCalc->rgrc->right);
    AfxTrace(_T("OnNcCalcSize 2 hwnd=0x%lX, (top=%ld,bottom=%ld,left=%ld,right=%ld)\n"),hWnd,pCalc->rgrc[2].top,pCalc->rgrc[2].bottom,pCalc->rgrc[2].left,pCalc->rgrc[2].right);
#endif

    //WNDPROC oldWndProc = (WNDPROC)::GetProp(hWnd, _OldMenuProc);
    //LRESULT result = NULL;
    //BOOL bCallDefault = TRUE;
    //result = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

    //return true;
  }
  return FALSE;
}

BOOL CMenuTheme::OnCalcFrameRect(HWND hWnd,LPRECT pRect)
{
  if(GetWindowRect(hWnd,pRect))
  {
    if(DoDrawBorder())
    {
      pRect->top += 2;
      pRect->left += 2;
      if(!IsShadowEnabled())
      {
        pRect->bottom -= 7;
        pRect->right -= 7;
      }
      else
      {
        pRect->bottom -= 3;
        pRect->right -= 3;
      }
    }
    return TRUE;
  }
  return FALSE;
}

void* CMenuTheme::SetScreenBitmap(HWND hWnd, HDC hDC)
{
  UNREFERENCED_PARAMETER(hDC);

  CNewMenuHook::CMenuHookData* pData = CNewMenuHook::GetMenuHookData(hWnd);
  if(pData->m_Screen.m_hObject==NULL)
  {
    // Get the desktop hDC...
    HDC hDcDsk = GetWindowDC(0);
    CDC* pDcDsk = CDC::FromHandle(hDcDsk);

    CDC dc;
    dc.CreateCompatibleDC(pDcDsk);

    CRect rect;
    GetWindowRect(hWnd,rect);
    pData->m_Screen.CreateCompatibleBitmap(pDcDsk,rect.Width()+10,rect.Height()+10);
    CBitmap* pOldBitmap = dc.SelectObject(&pData->m_Screen);
    dc.BitBlt(0,0,rect.Width()+10,rect.Height()+10,pDcDsk,pData->m_Point.x,pData->m_Point.y,SRCCOPY);

    dc.SelectObject(pOldBitmap);
    // Release the desktop hDC...
    ReleaseDC(0,hDcDsk);
  }
  return pData;
}

BOOL CMenuTheme::OnDrawBorder(HWND hWnd, HDC hDC, BOOL bOnlyBorder)
{
  CNewMenuHook::CMenuHookData* pData = (CNewMenuHook::CMenuHookData*)SetScreenBitmap(hWnd,hDC);
  if(DoDrawBorder() && pData)
  {
    CRect rect;
    CRect client;
    CDC* pDC = CDC::FromHandle (hDC);

    // Get the size of the menu...
    GetWindowRect(hWnd, rect );
    GetClientRect(hWnd, client);
    CPoint offset(0,0);
    ClientToScreen(hWnd,&offset);
    client.OffsetRect(offset-rect.TopLeft());

    long winW = rect.Width();
    long winH = rect.Height();

    if(!IsShadowEnabled())
    {
      if(!bOnlyBorder)
      {
        DrawFrame(pDC,CRect(CPoint(1,1),CSize(winW-6,winH-6)),client,CNewMenu::GetMenuColor());
      }
      if(bHighContrast)
      {
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-4,winH-4)),GetSysColor(COLOR_BTNTEXT ),GetSysColor(COLOR_BTNTEXT ));
      }
      else
      {
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-4,winH-4)),GetSysColor(COLOR_HIGHLIGHT),GetSysColor(COLOR_HIGHLIGHT));
        DrawShade(hWnd,hDC,pData->m_Point);
      }
    }
    else
    {
      if(!bOnlyBorder)
      {
        DrawFrame(pDC,CRect(CPoint(1,1),CSize(winW-2,winH-2)),client,CNewMenu::GetMenuColor());
        //pDC->FillSolidRect(winW-2,2,2,winH-2,RGB(0,0,255));
      }

      if(bHighContrast)
      {
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-0,winH-0)),GetSysColor(COLOR_BTNTEXT ),GetSysColor(COLOR_BTNTEXT ));
      }
      else
      {
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-0,winH-0)),GetSysColor(COLOR_HIGHLIGHT),GetSysColor(COLOR_HIGHLIGHT));
      }
    }
    //DrawSmalBorder(hWnd,hDC);
    return TRUE;
  }
  return FALSE;
}

CMenuThemeXP::CMenuThemeXP(DWORD dwThemeId,
                           pItemMeasureFkt pMeasureItem,
                           pItemDrawFkt pDrawItem,
                           pItemDrawFkt pDrawTitle,
                           DWORD dwFlags)
                           :CMenuTheme(dwThemeId,pMeasureItem,pDrawItem,pDrawTitle,dwFlags)
{
}

BOOL CMenuThemeXP::OnDrawBorder(HWND hWnd, HDC hDC, BOOL bOnlyBorder)
{
  CNewMenuHook::CMenuHookData* pData = (CNewMenuHook::CMenuHookData*)SetScreenBitmap(hWnd,hDC);
  if(DoDrawBorder() && pData)
  {
    UINT nMenuDrawMode = CNewMenu::GetMenuDrawMode();
  
    CRect rect;
    CRect client;
    CDC* pDC = CDC::FromHandle (hDC);

    // Get the size of the menu...
    GetWindowRect(hWnd, rect );
    GetClientRect(hWnd,client);
    CPoint offset(0,0);
    ClientToScreen(hWnd,&offset);
    client.OffsetRect(offset-rect.TopLeft());

    long winW = rect.Width();
    long winH = rect.Height();

    // Same Color as in DrawItem_WinXP
    COLORREF crMenuBar = CNewMenu::GetMenuColor(pData->m_hMenu);
    COLORREF crWindow = GetSysColor(COLOR_WINDOW);
    COLORREF crThinBorder = MixedColor(crWindow,crMenuBar);
    COLORREF clrBorder = DarkenColor(128,crMenuBar);
    COLORREF colorBitmap;

    if (NumScreenColors() > 256)
    {
      colorBitmap = MixedColor(crMenuBar,crWindow);
    }
    else
    {
      colorBitmap = GetSysColor(COLOR_BTNFACE);
    }

    // Better contrast when you have less than 256 colors
    if(pDC->GetNearestColor(crThinBorder)==pDC->GetNearestColor(colorBitmap))
    {
      crThinBorder = crWindow;
      colorBitmap = crMenuBar;
    }

    if(!IsShadowEnabled())
    {
      if(!bOnlyBorder)
      {
        DrawFrame(pDC,CRect(CPoint(1,1),CSize(winW-6,winH-6)),client,crThinBorder);
      }

      if(bHighContrast)
      {
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-4,winH-4)),GetSysColor(COLOR_BTNTEXT),GetSysColor(COLOR_BTNTEXT));
      }
      else
      {
        pDC->Draw3dRect(CRect(CPoint(1,1),CSize(winW-6,winH-6)),crThinBorder,crThinBorder);
        pDC->FillSolidRect(1,2,1,winH-8,colorBitmap);
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-4,winH-4)),clrBorder,clrBorder);

        DrawShade(hWnd,pDC->m_hDC,pData->m_Point);
      }
    }
    else
    {
      if(!bOnlyBorder)
      {
        DrawFrame(pDC,CRect(CPoint(1,1),CSize(winW-2,winH-2)),client,crThinBorder);
      }

      if(bHighContrast)
      {
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-0,winH-0)),GetSysColor(COLOR_BTNTEXT ),GetSysColor(COLOR_BTNTEXT ));
      }
      else
      {
        pDC->Draw3dRect(CRect(CPoint(1,1),CSize(winW-2,winH-2)),crThinBorder,crThinBorder);
        if(nMenuDrawMode==CNewMenu::STYLE_XP_2003 || nMenuDrawMode==CNewMenu::STYLE_XP_2003_NOBORDER)
        {
          pDC->FillSolidRect(1,2,1,winH-4,crWindow);
        }
        else
        {
          pDC->FillSolidRect(1,2,1,winH-4,colorBitmap);
        }
        pDC->Draw3dRect(CRect(CPoint(0,0),CSize(winW-0,winH-0)),clrBorder,clrBorder);
      }
    }
    DrawSmalBorder(hWnd,pDC->m_hDC);

    return TRUE;
  }
  return FALSE;
}

BOOL CMenuThemeXP::OnEraseBkgnd(HWND hWnd, HDC hDC)
{
  CNewMenuHook::CMenuHookData* pData = CNewMenuHook::GetMenuHookData(hWnd);
  if(pData->m_hMenu==NULL)
  {
    return CMenuTheme::OnEraseBkgnd(hWnd,hDC);
  }
  //  Get the size of the menu...
  CDC* pDC = CDC::FromHandle (hDC);
  CRect Rect;
  GetClientRect(hWnd, Rect );
  if(DoDrawBorder())
  {
    Rect.InflateRect(+2,0,-1,0);
  }
  else
  {
    Rect.InflateRect(+2,0,0,0);
  }

  //BPO!!!!!!!!!!  fehler Zeichnet auch in den rand hinein 2
  //pDC->FillSolidRect (Rect,CNewMenu::GetMenuColor());
  pDC->FillSolidRect (Rect,CNewMenu::GetMenuBarColor(pData->m_hMenu));

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMenuTheme2003 for drawing border and the rest

CMenuTheme2003::CMenuTheme2003( DWORD dwThemeId,
                                pItemMeasureFkt pMeasureItem,
                                pItemDrawFkt pDrawItem,
                                pItemDrawFkt pDrawTitle,
                                DWORD dwFlags)
 : CMenuThemeXP(dwThemeId,pMeasureItem,pDrawItem,pDrawTitle,dwFlags)
{
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const TCHAR _OldMenuProc[] = _T("OldMenuProc");

HMODULE CNewMenuHook::m_hLibrary = NULL;
HMODULE CNewMenuHook::m_hThemeLibrary = NULL;
HHOOK CNewMenuHook::HookOldMenuCbtFilter = NULL;
HMENU CNewMenuHook::m_hLastMenu = NULL;
DWORD CNewMenuHook::m_dwMsgPos = 0;
DWORD CNewMenuHook::m_bSubclassFlag = 0;

#ifdef _TRACE_MENU_LOGFILE
HANDLE CNewMenuHook::m_hLogFile = INVALID_HANDLE_VALUE;
#endif //_TRACE_MENU_LOGFILE


CTypedPtrList<CPtrList, CMenuTheme*>* CNewMenuHook::m_pRegisteredThemesList = NULL;

CTypedPtrMap<CMapPtrToPtr,HWND,CNewMenuHook::CMenuHookData*> CNewMenuHook::m_MenuHookData;

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION     0x1000
#endif

CNewMenuHook::CNewMenuHook()
{
  if(g_Shell>=Win2000 && GetSystemMetrics(SM_REMOTESESSION))
  {
    bRemoteSession=TRUE;
  }

  { // Detecting wine because we have some issue
    HMODULE hModule = GetModuleHandle(_T("kernel32.dll"));
    bWine = hModule && GetProcAddress(hModule,"wine_get_unix_file_name");
//    if(Wine)
//    {
//      MessageBox(NULL,_T("We are running under wine!!"),_T("wine detected"),MB_OK);
//    }
  }


  HIGHCONTRAST highcontrast = {0};
  highcontrast.cbSize = sizeof(highcontrast);
  if(SystemParametersInfo(SPI_GETHIGHCONTRAST,sizeof(highcontrast),&highcontrast,0) )
  {
    bHighContrast = ((highcontrast.dwFlags&HCF_HIGHCONTRASTON)!=0);
  }

#ifdef _TRACE_MENU_LOGFILE
  m_hLogFile = CreateFile(_T("c:\\NewMenuLog.txt"), GENERIC_WRITE, FILE_SHARE_WRITE,
    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(m_hLogFile!=INVALID_HANDLE_VALUE)
  {
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, m_hLogFile);

    _RPT0(_CRT_WARN,_T("file message\n"));
  }
#endif // _TRACE_MENU_LOGFILE

  // Try to load theme library from XP
  m_hThemeLibrary = LoadLibrary(_T("uxtheme.dll"));
  if(m_hThemeLibrary)
  {
    pIsThemeActive = (FktIsThemeActive)GetProcAddress(m_hThemeLibrary,"IsThemeActive");
    pSetWindowTheme = (FktSetWindowTheme)GetProcAddress(m_hThemeLibrary,"SetWindowTheme");
  }
  else
  {
    pIsThemeActive = NULL;
  }

  // Try to load the library for gradient drawing
  m_hLibrary = LoadLibrary(_T("Msimg32.dll"));

  // Don't use the gradientfill under Win98 because it is buggy!!!
  if(g_Shell!=Win98 && m_hLibrary)
  {
    pGradientFill = (FktGradientFill)GetProcAddress(m_hLibrary,"GradientFill");
  }
  else
  {
    pGradientFill = NULL;
  }

  AddTheme(new CMenuTheme(CNewMenu::STYLE_ICY,
    &CNewMenu::MeasureItem_Icy,
    &CNewMenu::DrawItem_Icy,
    &CNewMenu::DrawMenuTitle,TRUE));

  AddTheme(new CMenuTheme(CNewMenu::STYLE_ICY_NOBORDER,
    &CNewMenu::MeasureItem_Icy,
    &CNewMenu::DrawItem_Icy,
    &CNewMenu::DrawMenuTitle));

  AddTheme(new CMenuTheme(CNewMenu::STYLE_ORIGINAL,
    &CNewMenu::MeasureItem_OldStyle,
    &CNewMenu::DrawItem_OldStyle,
    &CNewMenu::DrawMenuTitle,TRUE));

  AddTheme(new CMenuTheme(CNewMenu::STYLE_ORIGINAL_NOBORDER,
    &CNewMenu::MeasureItem_OldStyle,
    &CNewMenu::DrawItem_OldStyle,
    &CNewMenu::DrawMenuTitle));

  AddTheme(new CMenuThemeXP(CNewMenu::STYLE_XP,
    &CNewMenu::MeasureItem_WinXP,
    &CNewMenu::DrawItem_WinXP,
    &CNewMenu::DrawMenuTitle,TRUE));

  AddTheme(new CMenuThemeXP(CNewMenu::STYLE_XP_NOBORDER,
    &CNewMenu::MeasureItem_WinXP,
    &CNewMenu::DrawItem_WinXP,
    &CNewMenu::DrawMenuTitle));

  AddTheme(new CMenuTheme2003(CNewMenu::STYLE_XP_2003,
    &CNewMenu::MeasureItem_WinXP,
    &CNewMenu::DrawItem_XP_2003,
    &CNewMenu::DrawMenuTitle,TRUE));

  AddTheme(new CMenuTheme2003(CNewMenu::STYLE_XP_2003_NOBORDER,
    &CNewMenu::MeasureItem_WinXP,
    &CNewMenu::DrawItem_XP_2003,
    &CNewMenu::DrawMenuTitle));

  AddTheme(new CMenuThemeXP(CNewMenu::STYLE_COLORFUL,
    &CNewMenu::MeasureItem_WinXP,
    &CNewMenu::DrawItem_XP_2003,
    &CNewMenu::DrawMenuTitle,TRUE));

  AddTheme(new CMenuThemeXP(CNewMenu::STYLE_COLORFUL_NOBORDER,
    &CNewMenu::MeasureItem_WinXP,
    &CNewMenu::DrawItem_XP_2003,
    &CNewMenu::DrawMenuTitle));

  AddTheme(new CMenuTheme(CNewMenu::STYLE_SPECIAL,
    &CNewMenu::MeasureItem_OldStyle,
    &CNewMenu::DrawItem_SpecialStyle,
    &CNewMenu::DrawMenuTitle,TRUE));

  AddTheme(new CMenuTheme(CNewMenu::STYLE_SPECIAL_NOBORDER,
    &CNewMenu::MeasureItem_OldStyle,
    &CNewMenu::DrawItem_SpecialStyle,
    &CNewMenu::DrawMenuTitle));

  //  CNewMenu::m_pActMenuDrawing = FindTheme(CNewMenu::STYLE_ORIGINAL);
  //  CNewMenu::m_pActMenuDrawing = FindTheme(CNewMenu::STYLE_ORIGINAL_NOBORDER);
  CNewMenu::m_pActMenuDrawing = FindTheme(CNewMenu::STYLE_XP);
  //  CNewMenu::m_pActMenuDrawing = FindTheme(CNewMenu::STYLE_XP_NOBORDER);

  if (HookOldMenuCbtFilter == NULL)
  {
    HookOldMenuCbtFilter = ::SetWindowsHookEx(WH_CALLWNDPROC, NewMenuHook, NULL, ::GetCurrentThreadId());
    if (HookOldMenuCbtFilter == NULL)
    {
      ShowLastError(_T("Error from Menu: SetWindowsHookEx"));
      AfxThrowMemoryException();
    }
  }
}

CNewMenuHook::~CNewMenuHook()
{
  if (HookOldMenuCbtFilter != NULL)
  {
    if(!::UnhookWindowsHookEx(HookOldMenuCbtFilter))
    {
      ShowLastError(_T("Error from Menu: UnhookWindowsHookEx"));
    }
    HookOldMenuCbtFilter = NULL;
  }
  // Destroy all registered themes.
  if( m_pRegisteredThemesList!= NULL)
  {
    while(m_pRegisteredThemesList->GetCount())
    {
      CMenuTheme* pTheme = m_pRegisteredThemesList->RemoveTail();
      delete pTheme;
    }
    delete m_pRegisteredThemesList;
    m_pRegisteredThemesList = NULL;
  }

  pIsThemeActive = NULL;
  FreeLibrary(m_hThemeLibrary);

  pGradientFill = NULL;
  FreeLibrary(m_hLibrary);

  // Cleanup for shared menu icons
  if(CNewMenu::m_pSharedMenuIcons)
  {
    delete CNewMenu::m_pSharedMenuIcons;
    CNewMenu::m_pSharedMenuIcons = NULL;
  }

#ifdef _TRACE_MENU_LOGFILE
  if(m_hLogFile!=INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hLogFile);
    m_hLogFile = INVALID_HANDLE_VALUE;
  }
#endif
}

BOOL CNewMenuHook::AddTheme(CMenuTheme* pTheme)
{
  if( m_pRegisteredThemesList== NULL)
  {
    m_pRegisteredThemesList = new CTypedPtrList<CPtrList, CMenuTheme*>;
  }
  if(m_pRegisteredThemesList->Find(pTheme))
  {
    return FALSE;
  }
  m_pRegisteredThemesList->AddTail(pTheme);
  return TRUE;
}

CMenuTheme* CNewMenuHook::RemoveTheme(DWORD dwThemeId)
{
  CMenuTheme* pTheme = FindTheme(dwThemeId);
  if(pTheme==NULL)
  {
    return NULL;
  }
  POSITION pos = m_pRegisteredThemesList->Find(pTheme);
  ASSERT(pos);
  if(pos)
  {
    m_pRegisteredThemesList->RemoveAt(pos);
    if(m_pRegisteredThemesList->GetCount()==NULL)
    {
      // Destroy the empty list.
      delete m_pRegisteredThemesList;
      m_pRegisteredThemesList = NULL;
    }
  }
  return pTheme;
}

CMenuTheme* CNewMenuHook::FindTheme(DWORD dwThemeId)
{
  if(m_pRegisteredThemesList==NULL)
  {
    return NULL;
  }

  POSITION pos = m_pRegisteredThemesList->GetHeadPosition();
  while(pos)
  {
    CMenuTheme* pTheme = m_pRegisteredThemesList->GetNext(pos);
    if(pTheme->m_dwThemeId==dwThemeId)
    {
      return pTheme;
    }
  }
  return NULL;
}

CNewMenuHook::CMenuHookData* CNewMenuHook::GetMenuHookData(HWND hWnd)
{
  CMenuHookData* pData=NULL;
  if(m_MenuHookData.Lookup(hWnd,pData))
  {
    return pData;
  }
  return NULL;
}

void CNewMenuHook::UnsubClassMenu(HWND hWnd)
{
  AFX_MANAGE_STATE(AfxGetModuleState());

  WNDPROC oldWndProc = (WNDPROC)::GetProp(hWnd, _OldMenuProc);
  ASSERT(oldWndProc != NULL);

  SetLastError(0);
  if(!SetWindowLongPtr(hWnd, GWLP_WNDPROC, (INT_PTR)oldWndProc))
  {
    ShowLastError(_T("Error from Menu: SetWindowLongPtr III"));
  }
  RemoveProp(hWnd, _OldMenuProc);
  GlobalDeleteAtom(GlobalFindAtom(_OldMenuProc));

  // now Clean up
  HMENU hMenu = NULL;
  // Restore old Styles for special menu!! (Menu 0x10012!!!)
  CMenuHookData* pData = GetMenuHookData(hWnd);
  if(pData)
  {
    hMenu = pData->m_hMenu;
    CNewMenu::m_pActMenuDrawing->OnUnInitWnd(hWnd);

    m_MenuHookData.RemoveKey(hWnd);

    delete pData;
  }

#ifdef _TRACE_MENU_
  AfxTrace(_T("Unsubclass Menu=0x%lX, hwnd=0x%lX\n"),hMenu,hWnd);
#endif
}

LRESULT CALLBACK CNewMenuHook::SubClassMenu(HWND hWnd,      // handle to window
                                            UINT uMsg,      // message identifier
                                            WPARAM wParam,  // first message parameter
                                            LPARAM lParam)  // second message parameter
{
  AFX_MANAGE_STATE(AfxGetModuleState());

  WNDPROC oldWndProc = (WNDPROC)::GetProp(hWnd, _OldMenuProc);
  LRESULT result = NULL;
  BOOL bCallDefault = TRUE;


#ifdef _TRACE_MENU_
  static long NestedLevel = 0;
  NestedLevel++;
  MSG msg = {hWnd,uMsg,wParam,lParam,0,0,0};
  TCHAR Buffer[30];
  wsprintf(Buffer,_T("Level %02ld"),NestedLevel);
  MyTraceMsg(Buffer,&msg);
#endif

  CMenuHookData* pData = GetMenuHookData(hWnd);
  if(NULL == pData)
  {
    return ::CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
  }
  ASSERT(pData);

  switch(uMsg)
  {
  case WM_NCPAINT:
    ASSERT(pData);

    if(pData->m_bDoSubclass)
    {
#ifdef _TRACE_MENU_
      AfxTrace(_T("WM_NCPAINT (0x%x) 0x%X, 0x%X\n"),hWnd,wParam,lParam);
#endif
      if(!pData->m_bDrawBorder)
      {
        if(pData->m_hRgn!=(HRGN)wParam)
        {
          if(pData->m_hRgn!=(HRGN)1)
          {
            DeleteObject(pData->m_hRgn);
            pData->m_hRgn=(HRGN)1;
          }
          if(wParam!=1)
          {
            CRgn dest;
            dest.CreateRectRgn( 0, 0, 1, 1);
            dest.CopyRgn(CRgn::FromHandle((HRGN)wParam));
            pData->m_hRgn = (HRGN)dest.Detach();
          }
        }
      }

      if(pData->m_dwData&8)
      {
        // do not call default!!!
        bCallDefault=FALSE;
      }

      if(pData->m_bDrawBorder &&  CNewMenu::m_pActMenuDrawing->DoDrawBorder())
      {
        //HDC hDC = GetWindowDC(hWnd);
        HDC hDC;
        //if(wParam!=1)
        //  hDC = GetDCEx(hWnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
        //else
        hDC = GetWindowDC (hWnd);

#ifdef _TRACE_MENU_
        if(wParam!=1)
        {
          DWORD dwCount = GetRegionData((HRGN)wParam,0,0);
          RGNDATA* pRegionData = (RGNDATA*)_alloca(dwCount);
          GetRegionData((HRGN)wParam,dwCount,pRegionData);

          TRACE(_T("WM_NCPAINT (0x%x) region %ld "),hWnd,pRegionData->rdh.nCount);
          CRect* pRect = (CRect*)pRegionData->Buffer;
          for(DWORD n=0; n<pRegionData->rdh.nCount; n++)
          {
            TRACE( _T("(%ld,%ld,%ld,%ld)"),pRect[n].left,pRect[n].top,pRect[n].right,pRect[n].bottom);
          }
          TRACE(_T("\r\n"));
        }
#endif //  _TRACE_MENU_

        if(hDC)
        {
          if(CNewMenu::m_pActMenuDrawing->OnDrawBorder(hWnd,hDC))
          {
            CRect rect;
            if(CNewMenu::m_pActMenuDrawing->OnCalcFrameRect(hWnd,rect))
            {
              CRgn rgn;
              rect.InflateRect(-1,-1);
              rgn.CreateRectRgnIndirect(rect);
              // do we need a combination of the regions?
              //if(wParam!=1)
              //{
              //  rgn.CombineRgn(&rgn,CRgn::FromHandle((HRGN)wParam),RGN_AND);
              //}
              ASSERT(oldWndProc);

              bCallDefault=FALSE;
              result = CallWindowProc(oldWndProc, hWnd, uMsg, (WPARAM)rgn.m_hObject, lParam);
              //result = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
              // Redraw the border and shade
              CNewMenu::m_pActMenuDrawing->OnDrawBorder(hWnd,hDC,true);
            }
          }
          ReleaseDC(hWnd,hDC);
        }
      }
      if(CNewMenu::m_pActMenuDrawing->DoDrawBorder() && bCallDefault)
      {
        // Save the background
        HDC hDC = GetWindowDC (hWnd);
        CNewMenu::m_pActMenuDrawing->SetScreenBitmap(hWnd,hDC);
        ReleaseDC(hWnd,hDC);
      }
    }

    break;

  case WM_PRINT:
    if(pData && pData->m_bDoSubclass)
    {
      if(CNewMenu::m_pActMenuDrawing->DoDrawBorder())
      {
        // Mark for WM_PRINT
        pData->m_dwData |= 8;

        //      pData->m_bDrawBorder = FALSE;
        // We need to create a bitmap for drawing
        // We can't clipp or make a offset to the DC because NT2000 (blue-screen!!)
        CRect rect;
        GetWindowRect(hWnd, rect);
        CDC dc;
        CBitmap bitmap;

        CDC* pDC = CDC::FromHandle((HDC)wParam);
        dc.CreateCompatibleDC(pDC);

        bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
        CBitmap* pOldBitmap = dc.SelectObject(&bitmap);

        // new
        //       dc.FillSolidRect(0,0,rect.Width(), rect.Height(),CNewMenu::GetMenuBarColor(pData->m_hMenu));

        CNewMenu::m_pActMenuDrawing->OnDrawBorder(hWnd,dc.m_hDC);

        CRect rectClient;
        if(CNewMenu::m_pActMenuDrawing->OnCalcFrameRect(hWnd,rectClient))
        {
          // might as well clip to the same rectangle
          CRect clipRect = rectClient;
          clipRect.OffsetRect(rectClient.TopLeft()-rect.TopLeft());
          clipRect.InflateRect(-1,-1);
          dc.IntersectClipRect(clipRect);
          result = CallWindowProc(oldWndProc, hWnd, uMsg, (WPARAM)dc.m_hDC, lParam&~PRF_CLIENT);

          pDC->BitBlt(0,0, rect.Width(), rect.Height(), &dc,0,0, SRCCOPY);

          GetClientRect(hWnd,clipRect);
          SelectClipRgn(dc.m_hDC,NULL);
          dc.IntersectClipRect(clipRect);

          SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)dc.m_hDC,0);
          SendMessage(hWnd,WM_PRINTCLIENT,(WPARAM)dc.m_hDC,lParam);

          CPoint wndOffset(0,0);
          ClientToScreen(hWnd,&wndOffset);
          wndOffset -= rect.TopLeft();
          pDC->BitBlt(wndOffset.x,wndOffset.y, clipRect.Width()-1, clipRect.Height(), &dc, 0, 0, SRCCOPY);
          //pDC->BitBlt(wndOffset.x,wndOffset.y, clipRect.Width(), clipRect.Height(), &dc, 0, 0, SRCCOPY);
        }
        //
        dc.SelectObject(pOldBitmap);
        bCallDefault=FALSE;

        // Clear for WM_PRINT
        pData->m_dwData &= ~8;
      }
    }
    break;

  case WM_ERASEBKGND:
    ASSERT(pData);
    if(pData->m_bDoSubclass)
    {
      if(CNewMenu::m_pActMenuDrawing->DoDrawBorder())
      {
        if(!(pData->m_dwData&8) && !pData->m_bDrawBorder )
        {
          pData->m_bDrawBorder = true;
          //SendMessage(hWnd,WM_NCPAINT,(WPARAM)pData->m_hRgn,0);
          SendMessage(hWnd,WM_NCPAINT,(WPARAM)1,0);
        }
      }

      if(CNewMenu::m_pActMenuDrawing->OnEraseBkgnd(hWnd,(HDC)wParam))
      {
        bCallDefault=FALSE;
        result = TRUE;
      }
    }
    break;

  case WM_WINDOWPOSCHANGED:
  case WM_WINDOWPOSCHANGING:
    {
      ASSERT(pData);

      LPWINDOWPOS pPos = (LPWINDOWPOS)lParam;
      if(uMsg==WM_WINDOWPOSCHANGING)
      {
        CNewMenu::m_pActMenuDrawing->OnWindowPosChanging(hWnd,pPos);
      }
      if(!(pPos->flags&SWP_NOMOVE) )
      {
        if(pData->m_Point==CPoint(0,0))
        {
          pData->m_Point = CPoint(pPos->x,pPos->y);
        }
        else if(pData->m_Point!=CPoint(pPos->x,pPos->y))
        {
          /*          CRect rect(0,0,0,0);
          if(!GetWindowRect(hWnd,rect))
          {
          AfxTrace(_T("Error get rect\n"));
          }
          #ifdef _TRACE_MENU_

          DWORD dwPos =GetMessagePos();
          AfxTrace(_T("Rect pos (%ld/%ld), dimensions [%ld,%ld], Delta(%ld/%ld),MPos %lx\n"),
          pData->m_Point.x,pData->m_Point.y,rect.Width(),rect.Height(),
          pData->m_Point.x-pPos->x,pData->m_Point.y-pPos->y,dwPos);
          #endif
          */
          if(!(pPos->flags&SWP_NOSIZE))
          {
            UnsubClassMenu(hWnd);
          }
          else
          {
            pData->m_Point=CPoint(pPos->x,pPos->y);
            pData->m_Screen.DeleteObject();
          }
        }
      }
    }
    break;

  case WM_KEYDOWN:
    if(wParam==VK_ESCAPE)
    {
      if(pData)
      {
        pData->m_dwData |= 4;
      }
    }
    m_dwMsgPos = GetMessagePos();
    break;

  case WM_NCCALCSIZE:
    if(pData->m_bDoSubclass)
    {
      NCCALCSIZE_PARAMS* pCalc = (NCCALCSIZE_PARAMS*)lParam;
      if(CNewMenu::m_pActMenuDrawing->OnNcCalcSize(hWnd,pCalc))
      {
        bCallDefault=FALSE;
      }
    }
    break;

  case WM_SHOWWINDOW:
    // Hide the window ? Test for 98 and 2000
    if(wParam==NULL)
    {
      // Special handling for NT 2000 and WND 0x10012.
      UnsubClassMenu(hWnd);
    }
    break;

  case WM_NCDESTROY:
    UnsubClassMenu (hWnd);
    break;

  case WM_TIMER:
    if(wParam==MENU_TIMER_ID)
    {
      if(g_Shell>=Win2000)
      {
        LONG_PTR dwExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        if( !(dwExStyle&WS_EX_LAYERED) )
        {
          SetWindowLongPtr (hWnd, GWL_EXSTYLE,dwExStyle| WS_EX_LAYERED);
          SetLayeredWindowAttributes(hWnd, 0, CNewMenu::GetAlpha(), LWA_ALPHA);
          pData->m_dwData &= ~4;
          KillTimer(hWnd,MENU_TIMER_ID);
        }
      }
    }
    break;

    //  if(pData->m_TimerID && pData->m_TimerID==wParam)
    //  {
    //    bCallDefault = FALSE;
    //    KillTimer(hWnd,pData->m_TimerID);
    //    pData->m_TimerID = NULL;
    //    result = 0;
    //    // Create shade windows
    //    SetWindowShade(hWnd, pData);
    //  }
    //  break;

    //default:
    //  {
    //    static const UINT WBHook = RegisterWindowMessage(_T("WBHook"));
    //    if (uMsg==WBHook)
    //    {
    //      bCallDefault = false;
    //    }
    //  }
    //  break;
  }

  if( bCallDefault )
  {
    ASSERT(oldWndProc != NULL);
    // call original wndproc for default handling
    result = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
  }

#ifdef _TRACE_MENU_
  NestedLevel--;
#endif

  return result;
}

BOOL CNewMenuHook::CheckSubclassing(HWND hWnd, BOOL bSpecialWnd)
{
  TCHAR Name[20];
  int Count = GetClassName (hWnd,Name,ARRAY_SIZE(Name));
  // Check for the menu-class
  if(Count!=6 || _tcscmp(Name,_T("#32768"))!=0)
  {
    // does not match to menuclass
    return false;
  }
  BOOL bDoNewSubclass = FALSE;
  CMenuHookData* pData=GetMenuHookData(hWnd);
  // check if we have allready some data
  if(pData==NULL)
  {

    // a way for get the menu-handle not documented
    if(!m_hLastMenu)
    {
      // WinNt 4.0 will crash in submenu
      m_hLastMenu = (HMENU)SendMessage(hWnd,0x01E1,0,0);
    }
    CMenu *pMenu = CMenu::FromHandlePermanent(m_hLastMenu);
    // now we support only new menu
    if( !DYNAMIC_DOWNCAST(CNewMenu,pMenu) )
    {
      return false;
    }

    WNDPROC oldWndProc;
    // subclass the window with the proc which does gray backgrounds
    oldWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    if (oldWndProc != NULL && GetProp(hWnd, _OldMenuProc) == NULL)
    {
      ASSERT(oldWndProc!=SubClassMenu);

      if(!SetProp(hWnd, _OldMenuProc, oldWndProc))
      {
        ShowLastError(_T("Error from Menu: SetProp"));
      }
      if ((WNDPROC)GetProp(hWnd, _OldMenuProc) == oldWndProc)
      {
        GlobalAddAtom(_OldMenuProc);

        CMenuHookData* pData=GetMenuHookData(hWnd);
        ASSERT(pData==NULL);
        if(pData==NULL)
        {
          pData = new CMenuHookData(hWnd,bSpecialWnd);
          m_MenuHookData.SetAt (hWnd,pData);

          SetLastError(0);
          if(!SetWindowLongPtr(hWnd, GWLP_WNDPROC,(INT_PTR)SubClassMenu))
          {
            ShowLastError(_T("Error from Menu: SetWindowLongPtr IV"));
          }
          bDoNewSubclass = TRUE;
#ifdef _TRACE_MENU_
          //SendMessage(hWnd,0x01E1,0,0) a way for get the menu-handle
          AfxTrace(_T("Subclass Menu=0x%lX, hwnd=0x%lX\n"),pData->m_hMenu,hWnd);
#endif
          CNewMenu::m_pActMenuDrawing->OnInitWnd(hWnd);
        }
      }
      else
      {
        ASSERT(0);
      }
    }
  }

  // Menu was set also assign it to this menu.
  if(m_hLastMenu)
  {
    CMenuHookData* pData = GetMenuHookData(hWnd);
    if(pData)
    {
      // Safe actual menu
      pData->SetMenu(m_hLastMenu);
      // Reset for the next menu
      m_hLastMenu = NULL;
      //
      CNewMenu::m_pActMenuDrawing->OnInitWnd(hWnd);
    }
  }
  return bDoNewSubclass;
}

/////////////////////////////////////////////////////////////////////////////
// CNewMenuThreadHook class used to hook for threads
// Jan-17-2005 - Mark P. Peterson - mpp@rhinosoft.com - http://www.RhinoSoft.com/

CNewMenuThreadHook::CNewMenuThreadHook()
{
  // set the hook handler for this thread
  m_hHookOldMenuCbtFilter = ::SetWindowsHookEx(WH_CALLWNDPROC, CNewMenuHook::NewMenuHook, NULL, ::GetCurrentThreadId());
} // CNewMenuThreadHook


CNewMenuThreadHook::~CNewMenuThreadHook()
{
  // reset the hook handler
  if (m_hHookOldMenuCbtFilter)
  {
    ::UnhookWindowsHookEx(m_hHookOldMenuCbtFilter);
  }
  // clear the handle
  m_hHookOldMenuCbtFilter = NULL;
} // ~CNewMenuThreadHook


/////////////////////////////////////////////////////////////////////////////
// CNewMenuTempHandler class used to add NewMenu to system dialog
class CNewMenuTempHandler : public CObject
{
  CNewMenu m_SystemNewMenu;
  WNDPROC m_oldWndProc;
  HWND m_hWnd;

public:
  CNewMenuTempHandler(HWND hWnd)
  {
    m_hWnd = hWnd;
    VERIFY(SetProp(hWnd,_T("CNewMenuTempHandler"),this));
    // Subclass the dialog control.
    m_oldWndProc = (WNDPROC)(LONG_PTR)SetWindowLongPtr(hWnd, GWLP_WNDPROC,(INT_PTR)TempSubclass);
  }

  ~CNewMenuTempHandler()
  {
    SetWindowLongPtr(m_hWnd, GWLP_WNDPROC,(INT_PTR)m_oldWndProc);
    VERIFY(RemoveProp(m_hWnd,_T("CNewMenuTempHandler")));
  }

  LRESULT Default(UINT uMsg, WPARAM wParam, LPARAM lParam )
  {
    // call original wndproc for default handling
    return CallWindowProc(m_oldWndProc, m_hWnd, uMsg, wParam, lParam);
  }

  LRESULT OnCmd(UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    MSG msg = {m_hWnd,uMsg,wParam,lParam,0,0,0};
    switch(uMsg)
    {
    case WM_DRAWITEM:
      {
        if(m_SystemNewMenu.m_hMenu)
        {
          DRAWITEMSTRUCT* lpDrawItemStruct = (DRAWITEMSTRUCT*)lParam;
          if (lpDrawItemStruct->CtlType == ODT_MENU)
          {
            CMenu* pMenu = CMenu::FromHandlePermanent((HMENU)lpDrawItemStruct->hwndItem);
            if (DYNAMIC_DOWNCAST(CNewMenu,pMenu))
            {
              pMenu->DrawItem(lpDrawItemStruct);
              return true; // eat it
            }
          }
        }
        return Default(uMsg, wParam, lParam);
      }

    case WM_MEASUREITEM:
      if(CNewMenu::OnMeasureItem(&msg))
      {
        return TRUE;
      }
      break;

    case WM_MENUCHAR:
      {
        CMenu* pMenu = CMenu::FromHandle(UIntToHMenu((UINT)lParam));;
        LRESULT lresult;
        if( DYNAMIC_DOWNCAST(CNewMenu,pMenu) )
          lresult=CNewMenu::FindKeyboardShortcut(LOWORD(wParam), HIWORD(wParam),pMenu );
        else
          lresult=Default(uMsg, wParam, lParam);
        return lresult;
      }
      break;

    case WM_INITMENUPOPUP:
      {
        CMenu* pMenu = CMenu::FromHandle(UIntToHMenu((UINT)wParam));
        LRESULT result = Default(uMsg, wParam, lParam);
        CNewMenu::OnInitMenuPopup(m_hWnd, pMenu, LOWORD(lParam), HIWORD(lParam));
        return result;
      }

    case WM_INITDIALOG:
      if(CNewMenuHook::m_bSubclassFlag&NEW_MENU_DIALOG_SYSTEM_MENU)
      {
        LRESULT bRetval = Default(uMsg, wParam, lParam);
        VERIFY(m_SystemNewMenu.m_hMenu==0);

        HMENU hMenu = ::GetSystemMenu(m_hWnd,FALSE);
        if(IsMenu(hMenu))
        {
          CMenu* pMenu = CMenu::FromHandlePermanent(hMenu);
          // Only attach to CNewMenu once
          if (DYNAMIC_DOWNCAST(CNewMenu,pMenu)==NULL )
          {
            m_SystemNewMenu.Attach(hMenu);
          }
        }
        return bRetval;
      }
      break;

    case WM_DESTROY:
      LRESULT result = Default(uMsg, wParam, lParam);
      delete this;
      return result;
    }

    return Default(uMsg, wParam, lParam);
  }

  // Subclass procedure
  static LRESULT APIENTRY TempSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    AFX_MANAGE_STATE(AfxGetModuleState());

    CNewMenuTempHandler* pTemp = (CNewMenuTempHandler*)GetProp(hwnd,_T("CNewMenuTempHandler"));
    ASSERT(pTemp);
    return pTemp->OnCmd(uMsg, wParam, lParam);
  }

};

LRESULT CALLBACK CNewMenuHook::NewMenuHook(int code, WPARAM wParam, LPARAM lParam)
{
  AFX_MANAGE_STATE(AfxGetModuleState());

  CWPSTRUCT* pTemp = (CWPSTRUCT*)lParam;
  if(code == HC_ACTION )
  {
    HWND hWnd = pTemp->hwnd;

#ifdef _TRACE_MENU_
    static HWND acthMenu = NULL;
    static HWND acthWnd = NULL;

    // Check if we need to find out the window
    if (hWnd && hWnd!=acthWnd && hWnd!=acthMenu)
    {
      TCHAR Name[20];
      int Count = GetClassName (hWnd,Name,ARRAY_SIZE(Name));
      // Check for the menu-class
      if(Count!=6 || _tcscmp(Name,_T("#32768"))!=0)
      {
        // does not match to menuclass
        acthWnd = hWnd;
      }
      else
      {
        acthMenu = hWnd;
        // AfxTrace(_T("Found new menu HWND: 0x%08X\n"),acthMenu);
      }
    }
    // Trace all menu msg
    if(acthMenu==hWnd && acthMenu)
    {
      MSG msg = {hWnd,pTemp->message,pTemp->wParam,pTemp->lParam,0,0,0};
      MyTraceMsg(_T(" Menu   "),&msg);
    }
#endif

    // Normal and special handling for menu 0x10012
    if(pTemp->message==WM_CREATE || pTemp->message==0x01E2)
    {
      if(!CheckSubclassing(hWnd,pTemp->message==0x01E2))
      {
        if( (m_bSubclassFlag&NEW_MENU_DIALOG_SUBCLASS) && pTemp->message==WM_CREATE)
        {
          TCHAR Name[20];
          int Count = GetClassName (hWnd,Name,ARRAY_SIZE(Name));
          // Check for the Dialog-class
          if(Count==6 && _tcscmp(Name,_T("#32770"))==0 )
          {
            // Only first dialog
            // m_bSubclassFlag &= ~NEW_MENU_DIALOG_SUBCLASS;
            // Freed by WM_DESTROY
            new CNewMenuTempHandler(hWnd);
          }
        }
      }
    }
  }
  return CallNextHookEx(HookOldMenuCbtFilter, code,wParam, lParam);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewDialog,CDialog)

BEGIN_MESSAGE_MAP(CNewDialog, CNewFrame<CDialog>)
  //{{AFX_MSG_MAP(CNewDialog)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  ON_WM_INITMENUPOPUP()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CNewDialog::CNewDialog()
{
}

CNewDialog::CNewDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
{
  ASSERT(IS_INTRESOURCE(lpszTemplateName) || AfxIsValidString(lpszTemplateName));

  m_pParentWnd = pParentWnd;
  m_lpszTemplateName = lpszTemplateName;
  if (IS_INTRESOURCE(m_lpszTemplateName))
    m_nIDHelp = LOWORD((DWORD_PTR)m_lpszTemplateName);
}

CNewDialog::CNewDialog(UINT nIDTemplate, CWnd* pParentWnd)
{
  m_pParentWnd = pParentWnd;
  m_lpszTemplateName = MAKEINTRESOURCE(nIDTemplate);
  m_nIDHelp = nIDTemplate;
}

BOOL CNewDialog::OnInitDialog()
{
  BOOL bRetval = CDialog::OnInitDialog();

  HMENU hMenu = m_SystemNewMenu.Detach();
  HMENU hSysMenu = ::GetSystemMenu(m_hWnd,FALSE);
  if(hMenu!=hSysMenu)
  {
    if(IsMenu(hMenu))
    {
      if(!::DestroyMenu(hMenu))
      {
        ShowLastError(_T("Error from Menu: DestroyMenu"));
      }
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

extern void AFXAPI AfxCancelModes(HWND hWndRcvr);

// Most of the code is copied from the MFC code from CFrameWnd::OnInitMenuPopup
void CNewDialog::OnInitMenuPopup(CMenu* pMenu, UINT nIndex, BOOL bSysMenu)
{
  AfxCancelModes(m_hWnd);

  // don't support system menu
  if (!bSysMenu)
  {
    ASSERT(pMenu != NULL);
    // check the enabled state of various menu items

    CCmdUI state;
    state.m_pMenu = pMenu;
    ASSERT(state.m_pOther == NULL);
    ASSERT(state.m_pParentMenu == NULL);

    // determine if menu is popup in top-level menu and set m_pOther to
    //  it if so (m_pParentMenu == NULL indicates that it is secondary popup)
    HMENU hParentMenu;
    if (AfxGetThreadState()->m_hTrackingMenu == pMenu->m_hMenu)
    {
      state.m_pParentMenu = pMenu;    // parent == child for tracking popup
    }
    else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
    {
      CWnd* pParent = GetTopLevelParent();
      // child windows don't have menus -- need to go to the top!
      if (pParent != NULL &&
        (hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
      {
        int nIndexMax = ::GetMenuItemCount(hParentMenu);
        for (int nIndex = 0; nIndex < nIndexMax; nIndex++)
        {
          if (::GetSubMenu(hParentMenu, nIndex) == pMenu->m_hMenu)
          {
            // when popup is found, m_pParentMenu is containing menu
            state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
            break;
          }
        }
      }
    }

    state.m_nIndexMax = pMenu->GetMenuItemCount();
    for (state.m_nIndex = 0; state.m_nIndex<state.m_nIndexMax; state.m_nIndex++)
    {
      state.m_nID = pMenu->GetMenuItemID(state.m_nIndex);
      if (state.m_nID == 0)
        continue; // menu separator or invalid cmd - ignore it

      ASSERT(state.m_pOther == NULL);
      ASSERT(state.m_pMenu != NULL);
      if (state.m_nID == (UINT)-1)
      {
        // possibly a popup menu, route to first item of that popup
        state.m_pSubMenu = pMenu->GetSubMenu(state.m_nIndex);
        if (state.m_pSubMenu == NULL ||
          (state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
          state.m_nID == (UINT)-1)
        {
          continue;       // first item of popup can't be routed to
        }
        state.DoUpdate(this, FALSE);    // popups are never auto disabled
      }
      else
      {
        // normal menu item
        // Auto enable/disable if frame window has 'm_bAutoMenuEnable'
        //    set and command is _not_ a system command.
        state.m_pSubMenu = NULL;
        state.DoUpdate(this, state.m_nID < 0xF000);
      }

      // adjust for menu deletions and additions
      UINT nCount = pMenu->GetMenuItemCount();
      if (nCount < state.m_nIndexMax)
      {
        state.m_nIndex -= (state.m_nIndexMax - nCount);
        while (state.m_nIndex < nCount &&
          pMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
        {
          state.m_nIndex++;
        }
      }
      state.m_nIndexMax = nCount;
    }
  }
  // Do the work for update
  CNewFrame<CDialog>::OnInitMenuPopup(pMenu,nIndex,bSysMenu);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNewMiniFrameWnd,CMiniFrameWnd)
BEGIN_MESSAGE_MAP(CNewMiniFrameWnd, CNewFrame<CMiniFrameWnd>)
  //{{AFX_MSG_MAP(CNewMiniFrameWnd)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CNewMDIChildWnd,CMDIChildWnd)
BEGIN_MESSAGE_MAP(CNewMDIChildWnd, CNewFrame<CMDIChildWnd>)
  //{{AFX_MSG_MAP(CNewMDIChildWnd)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CNewMiniDockFrameWnd,CMiniDockFrameWnd);
BEGIN_MESSAGE_MAP(CNewMiniDockFrameWnd, CNewFrame<CMiniDockFrameWnd>)
  //{{AFX_MSG_MAP(CNewMiniDockFrameWnd)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNewFrameWnd,CFrameWnd)

BEGIN_MESSAGE_MAP(CNewFrameWnd, CNewFrame<CFrameWnd>)
  //{{AFX_MSG_MAP(CNewFrameWnd)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

#if _MFC_VER < 0x0700

#include <../src/afximpl.h>

BOOL CNewFrameWnd::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle,
                             CWnd* pParentWnd, CCreateContext* pContext)
{
  // only do this once
  ASSERT_VALID_IDR(nIDResource);
  ASSERT(m_nIDHelp == 0 || m_nIDHelp == nIDResource);

  m_nIDHelp = nIDResource;    // ID for help context (+HID_BASE_RESOURCE)

  CString strFullString;
  if (strFullString.LoadString(nIDResource))
    AfxExtractSubString(m_strTitle, strFullString, 0);    // first sub-string

  VERIFY(AfxDeferRegisterClass(AFX_WNDFRAMEORVIEW_REG));

  // attempt to create the window
  LPCTSTR lpszClass = GetIconWndClass(dwDefaultStyle, nIDResource);
  LPCTSTR lpszTitle = m_strTitle;
  if (!Create(lpszClass, lpszTitle, dwDefaultStyle, rectDefault,
    pParentWnd, MAKEINTRESOURCE(nIDResource), 0L, pContext))
  {
    return FALSE;   // will self destruct on failure normally
  }

  // save the default menu handle
  ASSERT(m_hWnd != NULL);
  m_hMenuDefault = ::GetMenu(m_hWnd);

  // load accelerator resource
  LoadAccelTable(MAKEINTRESOURCE(nIDResource));

  if (pContext == NULL)   // send initial update
    SendMessageToDescendants(WM_INITIALUPDATE, 0, 0, TRUE, TRUE);

  return TRUE;
}
#endif

BOOL CNewFrameWnd::Create(LPCTSTR lpszClassName,
                          LPCTSTR lpszWindowName,
                          DWORD dwStyle,
                          const RECT& rect,
                          CWnd* pParentWnd,
                          LPCTSTR lpszMenuName,
                          DWORD dwExStyle,
                          CCreateContext* pContext)
{
  if (lpszMenuName != NULL)
  {
    m_DefaultNewMenu.LoadMenu(lpszMenuName);
    // load in a menu that will get destroyed when window gets destroyed
    if (m_DefaultNewMenu.m_hMenu == NULL)
    {
#if _MFC_VER < 0x0700
      TRACE0("Warning: failed to load menu for CNewFrameWnd.\n");
#else
      TRACE(traceAppMsg, 0, "Warning: failed to load menu for CNewFrameWnd.\n");
#endif
      PostNcDestroy();            // perhaps delete the C++ object
      return FALSE;
    }
  }
  m_strTitle = lpszWindowName;    // save title for later

  if (!CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle,
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
    pParentWnd->GetSafeHwnd(), m_DefaultNewMenu.m_hMenu, (LPVOID)pContext))
  {
#if _MFC_VER < 0x0700
    TRACE0("Warning: failed to create CNewFrameWnd.\n");
#else
    TRACE(traceAppMsg, 0, "Warning: failed to create CNewFrameWnd.\n");
#endif

    return FALSE;
  }

  UpdateMenuBarColor(m_DefaultNewMenu);

  return TRUE;
}

#ifdef USE_NEW_DOCK_BAR

//Bug in compiler??

#ifdef _AFXDLL
// control bar docking
// dwDockBarMap
const DWORD CFrameWnd::dwDockBarMap[4][2] =
{
  { AFX_IDW_DOCKBAR_TOP,      CBRS_TOP    },
  { AFX_IDW_DOCKBAR_BOTTOM,   CBRS_BOTTOM },
  { AFX_IDW_DOCKBAR_LEFT,     CBRS_LEFT   },
  { AFX_IDW_DOCKBAR_RIGHT,    CBRS_RIGHT  },
};
#endif

// dock bars will be created in the order specified by dwDockBarMap
// this also controls which gets priority during layout
// this order can be changed by calling EnableDocking repetitively
// with the exact order of priority
void CNewFrameWnd::EnableDocking(DWORD dwDockStyle)
{
  // must be CBRS_ALIGN_XXX or CBRS_FLOAT_MULTI only
  ASSERT((dwDockStyle & ~(CBRS_ALIGN_ANY|CBRS_FLOAT_MULTI)) == 0);

  m_pFloatingFrameClass = RUNTIME_CLASS(CNewMiniDockFrameWnd);
  for (int i = 0; i < 4; i++)
  {
    if (dwDockBarMap[i][1] & dwDockStyle & CBRS_ALIGN_ANY)
    {
      CDockBar* pDock = (CDockBar*)GetControlBar(dwDockBarMap[i][0]);
      if (pDock == NULL)
      {
        pDock = new CNewDockBar;
        if (!pDock->Create(this,
          WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE |
          dwDockBarMap[i][1], dwDockBarMap[i][0]))
        {
          AfxThrowResourceException();
        }
      }
    }
  }
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMDIFrameWnd,CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CNewMDIFrameWnd, CNewFrame<CMDIFrameWnd>)
  //{{AFX_MSG_MAP(CNewMDIFrameWnd)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CNewMDIFrameWnd::ShowMenu(BOOL bShow)
{
  // Gets the actual menu
  HMENU hMenu = ::GetMenu(m_hWnd);
  if(bShow)
  {
    if(m_hShowMenu)
    {
      ::SetMenu(m_hWnd, m_hShowMenu);
      DrawMenuBar();
      m_hShowMenu = NULL;
      return FALSE;
    }
    return TRUE;
  }
  else
  {
    m_hShowMenu = hMenu;
    if(m_hShowMenu)
    {
      ::SetMenu(m_hWnd, NULL);
      DrawMenuBar();
      return TRUE;
    }
    return FALSE;
  }
}



#if _MFC_VER < 0x0700

BOOL CNewMDIFrameWnd::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle,
                                CWnd* pParentWnd, CCreateContext* pContext)
{
  // only do this once
  ASSERT_VALID_IDR(nIDResource);
  ASSERT(m_nIDHelp == 0 || m_nIDHelp == nIDResource);

  m_nIDHelp = nIDResource;    // ID for help context (+HID_BASE_RESOURCE)

  CString strFullString;
  if (strFullString.LoadString(nIDResource))
    AfxExtractSubString(m_strTitle, strFullString, 0);    // first sub-string

  VERIFY(AfxDeferRegisterClass(AFX_WNDFRAMEORVIEW_REG));

  // attempt to create the window
  LPCTSTR lpszClass = GetIconWndClass(dwDefaultStyle, nIDResource);
  LPCTSTR lpszTitle = m_strTitle;
  if (!Create(lpszClass, lpszTitle, dwDefaultStyle, rectDefault,
    pParentWnd, MAKEINTRESOURCE(nIDResource), 0L, pContext))
  {
    return FALSE;   // will self destruct on failure normally
  }

  // save the default menu handle
  ASSERT(m_hWnd != NULL);
  m_hMenuDefault = ::GetMenu(m_hWnd);

  // load accelerator resource
  LoadAccelTable(MAKEINTRESOURCE(nIDResource));

  if (pContext == NULL)   // send initial update
    SendMessageToDescendants(WM_INITIALUPDATE, 0, 0, TRUE, TRUE);

  // save menu to use when no active MDI child window is present
  ASSERT(m_hWnd != NULL);
  m_hMenuDefault = ::GetMenu(m_hWnd);
  if (m_hMenuDefault == NULL)
    TRACE0("Warning: CMDIFrameWnd without a default menu.\n");
  return TRUE;
}
#endif

BOOL CNewMDIFrameWnd::Create( LPCTSTR lpszClassName,
                             LPCTSTR lpszWindowName,
                             DWORD dwStyle,
                             const RECT& rect,
                             CWnd* pParentWnd,
                             LPCTSTR lpszMenuName,
                             DWORD dwExStyle,
                             CCreateContext* pContext)
{
  if (lpszMenuName != NULL)
  {
    m_DefaultNewMenu.LoadMenu(lpszMenuName);
    // load in a menu that will get destroyed when window gets destroyed
    if (m_DefaultNewMenu.m_hMenu == NULL)
    {
#if _MFC_VER < 0x0700
      TRACE0("Warning: failed to load menu for CNewMDIFrameWnd.\n");
#else
      TRACE(traceAppMsg, 0, "Warning: failed to load menu for CNewMDIFrameWnd.\n");
#endif
      PostNcDestroy();            // perhaps delete the C++ object
      return FALSE;
    }
  }

  m_strTitle = lpszWindowName;    // save title for later

  if (!CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle,
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
    pParentWnd->GetSafeHwnd(), m_DefaultNewMenu.m_hMenu, (LPVOID)pContext))
  {
#if _MFC_VER < 0x0700
    TRACE0("Warning: failed to create CNewMDIFrameWnd.\n");
#else
    TRACE(traceAppMsg, 0, "Warning: failed to create CNewMDIFrameWnd.\n");
#endif

    return FALSE;
  }

  UpdateMenuBarColor(m_DefaultNewMenu);

  return TRUE;
}

#ifdef USE_NEW_DOCK_BAR

// dock bars will be created in the order specified by dwDockBarMap
// this also controls which gets priority during layout
// this order can be changed by calling EnableDocking repetitively
// with the exact order of priority
void CNewMDIFrameWnd::EnableDocking(DWORD dwDockStyle)
{
  // must be CBRS_ALIGN_XXX or CBRS_FLOAT_MULTI only
  ASSERT((dwDockStyle & ~(CBRS_ALIGN_ANY|CBRS_FLOAT_MULTI)) == 0);

  m_pFloatingFrameClass = RUNTIME_CLASS(CNewMiniDockFrameWnd);
  for (int i = 0; i < 4; i++)
  {
    if (dwDockBarMap[i][1] & dwDockStyle & CBRS_ALIGN_ANY)
    {
      CDockBar* pDock = (CDockBar*)GetControlBar(dwDockBarMap[i][0]);
      if (pDock == NULL)
      {
        pDock = new CNewDockBar;
        if (!pDock->Create(this,
          WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE |
          dwDockBarMap[i][1], dwDockBarMap[i][0]))
        {
          AfxThrowResourceException();
        }
      }
    }
  }
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNewMultiDocTemplate,CMultiDocTemplate)

CNewMultiDocTemplate::CNewMultiDocTemplate(UINT nIDResource,
                                           CRuntimeClass* pDocClass,
                                           CRuntimeClass* pFrameClass,
                                           CRuntimeClass* pViewClass)
                                           : CMultiDocTemplate(nIDResource,pDocClass,pFrameClass,pViewClass)
{
  if (m_nIDResource != 0 && m_hMenuShared)
  {
    // Attach the menu.
    m_NewMenuShared.LoadMenu(m_hMenuShared);
    // Try to load icons to the toolbar.
    m_NewMenuShared.LoadToolBar(m_nIDResource);
  }
}

CNewMultiDocTemplate::~CNewMultiDocTemplate()
{
  // Let only the CNewMenu to destroy the menu.
  if(m_hMenuShared==m_NewMenuShared.m_hMenu)
  {
    m_hMenuShared = NULL;
  }
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNewMenuHelper::CNewMenuHelper(DWORD dwFlags)
{
  m_dwOldFlags = CNewMenuHook::m_bSubclassFlag;
  CNewMenuHook::m_bSubclassFlag = dwFlags;

  m_OldMenuDrawStyle = CNewMenu::GetMenuDrawMode();
  if(CNewMenuHook::m_bSubclassFlag&NEW_MENU_DEFAULT_BORDER)
  {
    CNewMenu::SetMenuDrawMode(m_OldMenuDrawStyle|1);
  }
}

CNewMenuHelper::CNewMenuHelper(CNewMenu::EDrawStyle setTempStyle)
{
  m_dwOldFlags = CNewMenuHook::m_bSubclassFlag;
  m_OldMenuDrawStyle = CNewMenu::SetMenuDrawMode(setTempStyle);
}

CNewMenuHelper::~CNewMenuHelper()
{
  // Restore the old border style
  CNewMenu::SetMenuDrawMode(m_OldMenuDrawStyle);
  CNewMenuHook::m_bSubclassFlag = m_dwOldFlags;
}


// CNewDockBar

IMPLEMENT_DYNCREATE(CNewDockBar, CDockBar)

CNewDockBar::CNewDockBar(BOOL bFloating):CDockBar(bFloating)
{
}

CNewDockBar::~CNewDockBar()
{
}

BEGIN_MESSAGE_MAP(CNewDockBar, CDockBar)
  //{{AFX_MSG_MAP(CNewDockBar)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
  ON_WM_ERASEBKGND()
  ON_WM_NCPAINT()
END_MESSAGE_MAP()


// CNewDockBar diagnostics

#if defined(_DEBUG) || defined(_AFXDLL)
void CNewDockBar::AssertValid() const
{
  CDockBar::AssertValid();
}

void CNewDockBar::Dump(CDumpContext& dc) const
{
  CDockBar::Dump(dc);
}
#endif //_DEBUG


// CNewDockBar message handlers
void CNewDockBar::EraseNonClient()
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

  // draw borders in non-client area
  rectWindow.OffsetRect(-rectWindow.left, -rectWindow.top);

  //DrawBorders(&dc, rectWindow);

  // erase parts not drawn
  dc.IntersectClipRect(rectWindow);
  SendMessage(WM_ERASEBKGND, (WPARAM)dc.m_hDC);

  // draw gripper in non-client area
  DrawGripper(&dc, rectWindow);
}

BOOL CNewDockBar::OnEraseBkgnd(CDC* pDC)
{
  MENUINFO menuInfo = {0};
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.fMask = MIM_BACKGROUND;

  //if(::GetMenuInfo(::GetMenu(::GetParent(m_hWnd)),&menuInfo) && menuInfo.hbrBack)
  CBrush *pBrush = GetMenuBarBrush();
  if(pBrush)
  {
    CRect rectA;
    CRect rectB;
    GetWindowRect(rectA);
    ::GetWindowRect(::GetParent(m_hWnd),rectB);

    // need for win95/98/me
    VERIFY(pBrush->UnrealizeObject());
    CPoint oldOrg = pDC->SetBrushOrg(-(rectA.left-rectB.left),0);
    CRect rect;
    pDC->GetClipBox(rect);
    pDC->FillRect(rect,pBrush);
    pDC->SetBrushOrg(oldOrg);
    return true;
  }
  else
  {
    // do default drawing
    return CDockBar::OnEraseBkgnd(pDC);
  }
}

void CNewDockBar::OnNcPaint()
{
  // Do not call CDockBar::OnNcPaint() for painting messages
  EraseNonClient();
}

DWORD CNewDockBar::RecalcDelayShow(AFX_SIZEPARENTPARAMS* lpLayout)
{
  DWORD result = CDockBar::RecalcDelayShow(lpLayout);

#ifdef USE_NEW_MENU_BAR

  // we want to have the menubar on its own row or column and align to border
  int nCount = (int)m_arrBars.GetSize();
  for(int nPos=0;nPos<nCount;nPos++)
  {
    CControlBar *pBar = GetDockedControlBar(nPos);
    if(DYNAMIC_DOWNCAST(CNewMenuBar,pBar))
    {
      if(nPos && GetDockedControlBar(nPos-1))
      {
        // insert a row break befor the menubar
        m_arrBars.InsertAt(nPos,(void*)0);
        nPos++;
        nCount++;
      }
      CRect rect;
      pBar->GetWindowRect(rect);
      ScreenToClient(rect);
      if(pBar->m_dwStyle&CBRS_ORIENT_HORZ)
      {
        if(rect.left>0)
        {
          pBar->SetWindowPos(NULL,-5,rect.top,0,0,SWP_NOZORDER|SWP_NOSIZE|SWP_FRAMECHANGED);
        }
      }
      else
      {
        if(rect.top>0)
        {
          pBar->SetWindowPos(NULL,rect.left,-5,0,0,SWP_NOZORDER|SWP_NOSIZE|SWP_FRAMECHANGED);
        }
      }

      if((nPos+1)<nCount && GetDockedControlBar(nPos+1))
      {
        // insert a row break after the menubar
        m_arrBars.InsertAt(nPos+1,(void*)0);
        nCount++;
      }
    }
  }
#endif //USE_NEW_MENU_BAR

  return result;
}

TCHAR szShadeWindowClass[] = _T("ShadeWindow");

typedef struct
{
  CRect parentRect;
  HWND hParent;
  BOOL bSideShade;
}SShadeInfo;


void DrawWindowShade(HDC hDC, CRect rect, COLORREF blendColor)
{
  if (NumScreenColors() <= 256)
  {
    DWORD rgb = ::GetSysColor(COLOR_BTNSHADOW);
    COLORREF oldColor = ::SetBkColor(hDC, rgb);
    ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
    ::SetBkColor(hDC, oldColor);
  }
  else
  {
    COLORREF ref;
    int width = rect.Width();
    int height = rect.Height();
    int X,Y;

    if(width<height)
    {
      for (X=0; X<=4 ;X++)
      {
        for (Y=0; Y<=4 ;Y++)
        {
          // Simulate a shadow on the left edge...
          ref = GetAlphaBlendColor(blendColor,GetPixel(hDC,X,Y),255-(4-X)*Y*10);
          SetPixel(hDC,X,Y,ref);
        }
        for (;Y<height;Y++)
        {
          // Simulate a shadow on the left...
          ref = GetAlphaBlendColor(blendColor,GetPixel(hDC,X,Y),240-(4-X)*40);
          SetPixel(hDC,X,Y,ref);
        }
      }
    }
    else
    {
      for(Y=0; Y<=4 ;Y++)
      {
        // Simulate a shadow on the bottom edge...
        for(X=0; X<=4 ;X++)
        {
          ref = GetAlphaBlendColor(blendColor,GetPixel(hDC,X,Y),255-(4-Y)*X*10);
          SetPixel(hDC,X,Y,ref);
        }
        // Simulate a shadow on the bottom...
        for(;X<=(width-5);X++)
        {
          ref = GetAlphaBlendColor(blendColor,GetPixel(hDC,X,Y),240-(4-Y)*40);
          SetPixel(hDC,X,Y,ref);
        }
        // Simulate a shadow on the bottom edge...
        for(;X<width;X++)
        {
          ref = GetAlphaBlendColor(blendColor,GetPixel(hDC,X,Y),255-((width-1-X)*(4-Y))*10);
          SetPixel(hDC,X,Y,ref);
        }
      }
    }
  }

}
//
//  FUNCTION: ShadeWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the shade window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT  - Paint the main window
//  WM_DESTROY  - post a quit message and return


LRESULT CALLBACK ShadeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  //  static long NestedLevel = 0;
  //  NestedLevel++;
  //  MSG msg = {hWnd,message,wParam,lParam,0,0,0};
  //  TCHAR Buffer[30];
  //  wsprintf(Buffer,_T("Level %02ld"),NestedLevel);
  //  MyTraceMsg(Buffer,&msg);

  LRESULT result = 0;
  switch (message)
  {
  case WM_MOVE:
    result = DefWindowProc(hWnd, message, wParam, lParam);
    InvalidateRect(hWnd,NULL,TRUE);
    break;

  case WM_ERASEBKGND:
    {
      CRect rect;
      GetClientRect(hWnd,&rect);
      COLORREF blendColor = ::GetSysColor(COLOR_ACTIVECAPTION);//RGB(255,0,0);

      DrawWindowShade((HDC) wParam, rect, blendColor);
      ValidateRect(hWnd,&rect);
    }
    return FALSE;

  case WM_NCHITTEST:
    return HTTRANSPARENT;

  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);
      CRect rect;
      COLORREF blendColor = ::GetSysColor(COLOR_ACTIVECAPTION);//RGB(255,0,0);
      GetClientRect(hWnd,&rect);

      DrawWindowShade(hDC, rect, blendColor);

      ValidateRect(hWnd,&rect);

      EndPaint(hWnd, &ps);
    }
    break;

  case WM_TIMER:
    {
      KillTimer(hWnd,wParam);
      CRect rect;
      HWND hParent = GetParent(hWnd);
      GetWindowRect(hParent,rect);
      SetWindowPos(hWnd,0,rect.right,rect.top,6,rect.Height(),SWP_NOZORDER|SWP_SHOWWINDOW);
    }

    break;

  case WM_WINDOWPOSCHANGED :
    //if (true)
    {
      result = DefWindowProc(hWnd, message, wParam, lParam);
      WINDOWPOS *pWP = (WINDOWPOS*)lParam;
      if (pWP->hwnd != hWnd)
        SetWindowPos (hWnd, HWND_TOP, 0, 0, 0, 0,SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }
    break;

  case WM_SHOWWINDOW:
    {
      result = DefWindowProc(hWnd, message, wParam, lParam);
      //if(wParam)
      //{
      //  // window is show
      //  //InvalidateRect(hWnd,NULL,FALSE);
      //  SetWindowPos(hWnd,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
      //}
      //else
      //{
      //  // window disappear
      //  InvalidateRect(hWnd,NULL,FALSE);
      //}
    }
    break;

  case WM_CREATE:
    {
      LPCREATESTRUCT pCreate = (LPCREATESTRUCT)lParam;
      ASSERT(pCreate);
      //CRect rect;
      //GetWindowRect(pCreate->hwndParent,rect);
      result = DefWindowProc(hWnd, message, wParam, lParam);
      SShadeInfo *pShadeInfo = (SShadeInfo*)pCreate->lpCreateParams;
      ASSERT(pShadeInfo);
      if(pShadeInfo->bSideShade)
      {
        SetWindowPos(hWnd,HWND_TOPMOST,
          pShadeInfo->parentRect.left+5,pShadeInfo->parentRect.bottom,
          pShadeInfo->parentRect.Width(),5,
          SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOZORDER);
      }
      else
      {
        SetWindowPos(hWnd,HWND_TOPMOST,
          pShadeInfo->parentRect.right,pShadeInfo->parentRect.top+5,
          5,pShadeInfo->parentRect.Height()-5,
          SWP_SHOWWINDOW|SWP_NOACTIVATE);
      }
      //GetSafeTimerID(hWnd,200);
    }
    break;

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return result;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
BOOL ShadeRegisterClass()
{
  WNDCLASS wndcls;

  wndcls.style         = CS_HREDRAW | CS_VREDRAW;
  wndcls.lpfnWndProc   = (WNDPROC)ShadeWndProc;
  wndcls.cbClsExtra    = 0;
  wndcls.cbWndExtra    = 0;
  wndcls.hInstance     = AfxGetInstanceHandle();
  wndcls.hIcon         = (HICON)NULL;
  wndcls.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wndcls.hbrBackground = (HBRUSH)NULL;
  wndcls.lpszMenuName  = (LPCTSTR)NULL;
  wndcls.lpszClassName = szShadeWindowClass;

  return AfxRegisterClass(&wndcls);
}

void SetWindowShade(HWND hWndParent,LPRECT pRect)
{
  ShadeRegisterClass();

  SShadeInfo shadeInfo;

  shadeInfo.hParent = hWndParent;
  if(pRect!=NULL)
  {
    shadeInfo.parentRect = *pRect;
  }
  else
  {
    GetWindowRect(hWndParent,&shadeInfo.parentRect);
  }

  shadeInfo.bSideShade = true;
  HWND hWnd = CreateWindowEx(WS_EX_TRANSPARENT|WS_EX_TOPMOST ,
    szShadeWindowClass,
    NULL,
    WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
    CW_USEDEFAULT, 0,
    CW_USEDEFAULT, 0,
    hWndParent, (HMENU)0,
    AfxGetInstanceHandle(),
    &shadeInfo);

  shadeInfo.bSideShade = false;
  hWnd = CreateWindowEx( WS_EX_TRANSPARENT|WS_EX_TOPMOST ,
    szShadeWindowClass,
    NULL,
    WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
    CW_USEDEFAULT, 0,
    CW_USEDEFAULT, 0,
    hWndParent, (HMENU)0,
    AfxGetInstanceHandle(),
    &shadeInfo);
}

//void SetWindowShade(HWND hWndMenu, CNewMenuHook::CMenuHookData* pData )
//{
//  if(pData==NULL || pData->m_hRightShade!=NULL || pData->m_hBottomShade)
//  {
//    return;
//  }
//
//  ShadeRegisterClass();
//
//  HWND hWndParent = AfxGetMainWnd()->GetSafeHwnd();
//  SShadeInfo shadeInfo;
//  shadeInfo.hParent = hWndMenu;
//  GetWindowRect(hWndMenu,&shadeInfo.parentRect);
//
//  shadeInfo.bSideShade = true;
//
//  pData->m_hRightShade = CreateWindowEx(WS_EX_TRANSPARENT|WS_EX_TOPMOST ,
//                                        szShadeWindowClass,
//                                        NULL,
//                                        WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
//                                        CW_USEDEFAULT, 0,
//                                        CW_USEDEFAULT, 0,
//                                        hWndParent, (HMENU)0,
//                                        AfxGetInstanceHandle(),
//                                        &shadeInfo);
//
//  shadeInfo.bSideShade = false;
//  pData->m_hBottomShade = CreateWindowEx( WS_EX_TRANSPARENT|WS_EX_TOPMOST ,
//                                          szShadeWindowClass,
//                                          NULL,
//                                          WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
//                                          CW_USEDEFAULT, 0,
//                                          CW_USEDEFAULT, 0,
//                                          hWndParent, (HMENU)0,
//                                          AfxGetInstanceHandle(),
//                                          &shadeInfo);
//}


#ifdef _AFXDLL

static void ForceVectorDelete()
{
  // this function is only need for exporting all template
  // functions

  // never called
  ASSERT(FALSE);

  new CNewFrameWnd[2];

  new CNewMDIChildWnd[2];
  new CNewMDIFrameWnd[2];

  new CNewDialog[2];

  new CNewMiniDockFrameWnd[2];
  new CNewMiniFrameWnd[2];
}

void (*_ForceVectorDelete)() = &ForceVectorDelete;

#endif
