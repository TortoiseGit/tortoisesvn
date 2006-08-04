//------------------------------------------------------------------------------
// File    : NewMenuBar.h
// Version : 1.04
// Date    : 25. June 2005
// Author  : Bruno Podetti
// Email   : Podetti@gmx.net
// Web     : www.podetti.com/NewMenu
// Systems : VC6.0/7.0 and VC7.1 (Run under (Window 98/ME), Windows Nt 2000/XP)
//           for all systems it will be the best when you install the latest IE
//           it is recommended for CNewToolBar
//------------------------------------------------------------------------------
#ifndef __NewMenuBar_H_
#define __NewMenuBar_H_

#include <afxtempl.h>

// CNewMenuBar
#define AFX_IDW_DOCKBAR_MENU (AFX_IDW_DOCKBAR_TOP-1)


typedef struct _MBITEM
{
  CRect rcItem;
  UINT nState;

}MBITEM,*PMBITEM;

/////////////////////////////////////////////////////////////////////////////
// CNewBarItem menubar item for drawing
class GUILIBDLLEXPORT CNewBarItem: public CObject
{
public:
  CRect m_ItemRect;
  UINT m_nSyncFlag;
  UINT m_nState;

public:
  CNewBarItem();

  virtual CRect GetRect(BOOL bVertical) const;

  virtual int HitTest(CPoint point,BOOL bVertical) const;
  virtual void DrawBarItem(HWND,HDC)=0;

  virtual HMENU GetMenu(HWND hWndOwner)=0;
  virtual UINT GetMenuItemID(){return UINT(-1);}
};

/////////////////////////////////////////////////////////////////////////////
// CNewMenuBarItem for drawing
class GUILIBDLLEXPORT CNewMenuBarItem: public CNewBarItem
{
public:
  HMENU m_hMenu;
  UINT m_nIndex;

public:
  CNewMenuBarItem(HMENU hMenu, UINT nIndex);

  virtual HMENU GetMenu(HWND hWndOwner);
  virtual void DrawBarItem(HWND hWndOwner, HDC hDC);
  virtual UINT GetMenuItemID();
};

/////////////////////////////////////////////////////////////////////////////
// CNewIconBarItem menu bar icons for drawing
class GUILIBDLLEXPORT CNewIconBarItem: public CNewBarItem
{
public:
  HBITMAP m_hBitmap;
  UINT m_uCommandID;

public:
  CNewIconBarItem(HBITMAP hBitmap,UINT uCommandID);

  virtual UINT GetMenuItemID();
  virtual HMENU GetMenu(HWND hWndOwner);
  virtual void DrawBarItem(HWND hWndOwner, HDC hDC);
  virtual CRect GetRect(BOOL bVertical) const;

  void PaintHotButton(HDC hDC);
  void DrawSpecialChar(HDC hDC, LPCRECT pRect, TCHAR Sign, BOOL bBold, BOOL bDisabled);
};



/////////////////////////////////////////////////////////////////////////////
// CNewMenuBar for docking
class GUILIBDLLEXPORT CNewMenuBar : public CControlBar
{
  DECLARE_DYNAMIC(CNewMenuBar)

public:
  CNewMenuBar();
  virtual ~CNewMenuBar();

  virtual BOOL Create(CWnd* pParentWnd,
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP|CBRS_SIZE_DYNAMIC|CBRS_TOOLTIPS,
    UINT nID = AFX_IDW_DOCKBAR_MENU);
  virtual BOOL CreateEx(CWnd* pParentWnd, DWORD dwCtrlStyle = TBSTYLE_FLAT,
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP|CBRS_SIZE_DYNAMIC|CBRS_TOOLTIPS,
    CRect rcBorders = CRect(3, 0, 2, 0),
    UINT nID = AFX_IDW_DOCKBAR_MENU);

  virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
  virtual CSize CalcDynamicLayout(int nLength, DWORD nMode);
  virtual CSize CalcLayout(DWORD dwMode, int nLength=-1);

  int GetClipBoxLength(BOOL bHorz);

  virtual int WrapMenuBar(MBITEM* pData, int nCount, int nWidth);
  virtual void SizeMenuBar(MBITEM* pData, int nCount, int nLength, BOOL bVert = FALSE);
  virtual void UpdateItemLayout(MBITEM* pData, int nCount);
  virtual void CorrectItemLayout(MBITEM* pData, int nFrom, int nTo, int nWidth);

  virtual CSize CalcSize(MBITEM* pData, int nCount);

  virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

  void RefreshBar();

  typedef enum {TR_NONE, TR_POPUP, TR_MENU } ETrackingState;
  void SetSel(int nIndex, ETrackingState eState);

  // virtual from base class
  void DrawBorders(CDC* pDC, CRect& rect);
  void DrawGripper(CDC* pDC, const CRect& rect);
  // overwritten from baseclass not virtual
  void EraseNonClient();

  int GetItemCount() const { return (int)m_NewBarItemList.GetSize(); }
  int GetNextItem(int nIndex, BOOL bNext=TRUE);

  BOOL OnMenuInput(MSG msg);
  BOOL TranslateFrameMessage(MSG* pMsg);

public:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDestroy();

  void OnMDISetMenu(HMENU hNewMenu);
  int HitTest(LPPOINT ppt )const;
  BOOL ShowMenu(int nIndex);


protected:
  static CNewMenuBar* g_pMenuBar;
  static HHOOK g_hMsgHook;
  static LRESULT CALLBACK MenuInputFilter(int code, WPARAM wParam, LPARAM lParam);

  DECLARE_MESSAGE_MAP()

  // Override this to handle messages in specific handlers
  virtual LRESULT FrameWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
  // call this at the end of handler fns
  LRESULT FrameDefault();

  // Override this to handle messages in specific handlers
  virtual LRESULT ChildWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
  // call this at the end of handler fns
  LRESULT ChildDefault();

  UINT EnableTimer(BOOL bEnable=TRUE);

public:
  ETrackingState m_eTrackingState;

  BOOL m_bProcessLeftArrow;
  BOOL m_bProcessRightArrow;
  BOOL m_bLoop;
  BOOL m_bIgnoreAltKey;

  BOOL m_bInMemuLoop;
  UINT m_TimerID;

  HMENU m_hMenuBar;

  BOOL m_bMDIMaximized;
  BOOL m_bDelayedButtonLayout; // used to manage when button layout should be done

  int m_nActualSelIndex;
  int m_nCurIndex;
  CPoint m_ptMouse;
  int m_nMouseIndex;

  HMENU m_hMenuPopup;
  HWND m_hWndCommand;

  static const UINT WM_GETMENU;
  static const UINT WM_REMOVEMENU;


  // Stores list of bar items
  CTypedPtrArray<CPtrArray, CNewBarItem*> m_NewBarItemList;

  void DeleteNewBarItemList();

private:
  static CTypedPtrMap<CMapPtrToPtr,HWND,CNewMenuBar*> m_NewMenuBar;

  WNDPROC m_pOldFrameProc;
  WNDPROC m_pOldChildProc;

  static LRESULT CALLBACK FrmWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
  static LRESULT CALLBACK CilWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

public:
  afx_msg void OnTimer(UINT nIDEvent);
  afx_msg void OnNcPaint();
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
#if _MFC_VER < 0x0800  
  afx_msg UINT OnNcHitTest(CPoint point);
#else
  afx_msg LRESULT OnNcHitTest(CPoint point);
#endif
};

#endif //__NewMenuBar_H_