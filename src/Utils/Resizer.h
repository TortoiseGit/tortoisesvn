// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
 * \defgroup sizer RESIZER
 * Macros for easy resizing of dialogs and controls.
 * How to make your dialog resizable:\n
 * -# Design your dialog in the resource editor
 * -# Set the dialog as resizable
 * -# include the header file 'resizer.h'
 * -# Add the macro DECLARE_RESIZER anywhere to your class declaration:\n
 *    \code class CMyDialog : public CDialog
 *			{
 *				DECLARE_RESIZER
 *				...
 *			}
 *    \endcode
 * -# Create an OnInitDialog-handler and add the INIT_RESIZER to the END of it:\n
 *    \code CMyDialog::OnInitDialog()
 *			{
 *				CDialog::OnInitDialog();
 *				....
 *				INIT_RESIZER;
 *			}
 *    \endcode
 * -# Recommended (but optional): Create an OnSizing-handler and add the RESIZER_MINSIZE(mx, my, s, r) to it:\n
 *    \code CMyDialog::OnSizing(UINT fwSide, LPRECT pRect)
 *			{
 *				CDialog::OnSizing(fwSide, pRect);
 *				RESIZER_MINSIZE(300, 400, fwSide, pRect);
 *				//where 300 is the minimum width and 400 the minimum height we want
 *				//the dialog will never get smaller than that
 *			}
 *    \endcode
 * -# Create an OnSize-handler and add the UPDATE_RESIZER macro to it:\n
 *    \code CMyDialog::OnSize(UINT nType, int cx, int cy)
 *			{
 *				CDialog::OnSize(nType, cx, cy);
 *				UPDATE_RESIZER;
 *			}
 *    \endcode
 * -# Now add the map anywhere inside your class implementation (your *.cpp file). The map specifies
 *    how the individual controls should be resized.\n
 *    \code 
 *    BEGIN_RESIZER_MAP(CMyDialog)
 *    ...
 *    RESIZER(control_id, left, top, right, bottom, options)
 *    ...
 *    END_RESIZER_MAP
 *    \endcode
 * -# Inside the OnPaint() method place RESIZER_GRIP; to draw the resizing grip on the dialog. If you don't
 *    then no sizegrip will be painted.
 * @{
 */

/**
 * \def DECLARE_RESIZER
 * \hideinitializer
 * Add this macro somewhere inside your class declaration in the header file
 */

/**
 * \def INIT_RESIZER
 * \hideinitializer
 * Add this macro to the end of your OnInitDialog() implementation.
 */
 
/**
 * \def UPDATE_RESIZER
 * \hideinitializer
 * Add this macro to the OnSize() method of your dialog class. It will
 * resize the controls according to the UPDATE_RESIZER_MAP.
 */
  
/**
 * \def RESIZER_MINSIZE(mx, my, s, r)
 * \hideinitializer
 * Put this macro inside the OnSizing() method of your dialog class. It
 * will prevent the user to decrease the size below the values you specify.
 * \param mx the minimum x-size of our dialog
 * \param my the minimum y-size of our dialog
 * \param s the UINT nSize from OnSizing()
 * \param r the LPRECT lpRect from OnSizing()
 * \remark the code used to do this is the following
 * \code
 * if(r->right-r->left < mx) 
 * {
 * 	if((s == WMSZ_BOTTOMLEFT)||(s == WMSZ_LEFT)||(s == WMSZ_TOPLEFT))
 * 		r->left = r->right-mx; 
 * 	else
 * 		r->right = r->left+mx; 
 * }
 * if(r->bottom-r->top < my) 
 * { 
 * 	if((s == WMSZ_TOP)||(s == WMSZ_TOPLEFT)||(s == WMSZ_TOPRIGHT))
 * 		r->top = r->bottom-my; 
 * 	else 
 * 		r->bottom = r->top+my;
 * }
 * \endcode
 */

/**
 * \def BEGIN_RESIZER_MAP(classname)
 * Marks the start of a resizer map. See RESIZER for details.
 * \hideinitializer
 * \param classname the name of your dialog class.
 * \remark the code used to do this is the following
 * \code
 * void class::__RS__CalcBottomRight(CWnd *pThis, BOOL bBottom, int &bottomright, int &topleft, UINT id, UINT br, int RS_br, CRect &rect, int clientbottomright) 
 * {
 * 	if(br==RS_BORDER) 
 * 		bottomright = clientbottomright-RS_br;
 * 	else if (br==RS_KEEPSIZE) 
 * 		bottomright = topleft+RS_br;
 * 	else 
 * 	{ 
 * 		CRect rect2;
 * 		pThis->GetDlgItem(br)->GetWindowRect(rect2); 
 * 		pThis->ScreenToClient(rect2);
 * 		bottomright = (bBottom?rect2.top:rect2.left) - RS_br;
 * 	}
 * }
 * void class::__RS__RepositionControls(BOOL bInit)
 * { 
 * 	CRect rect,rect2,client; 
 * 	GetClientRect(client);
 * 	//here come the entries from RESIZER(id, left, right, top, bottom, redraw);
 *  //now the END_RESIZER_MAP
 * 	Invalidate();
 * 	UpdateWindow();
 * }
 * \endcode
 */


/**
 * \def RESIZER(control_id,left,top,right,bottom,options)
 * \hideinitializer
 * Used to specify how a control should be resized/repositioned.
 * \param control_id the ID of the control to resize
 * \param left,top,right,bottom either the ID of another control or a predefined value. See remarks for details.
 * \param options either RS_HCENTER, RS_VCENTER or both or 0. RS_HCENTER will center the control horizontally while RS_VCENTER does it vertically.
 * If options are 0 then the control is not centered in any way.
 * \remark the predefined values for left,top,right and bottom can be\n
 * - RS_BORDER to keep the distance between the control and the border constant
 * - RS_KEEPSIZE to keep the size of the control itself constant.
 * Please note that RS_KEEPSIZE must not be applied to both top AND bottom or left AND right.\n
 * For your convenience the code behind this macro:
 * \code
 * static int	id##_RS__l, id##_RS__t, id##_RS__r, id##_RS__b;
 * if (bInit)
 * {
 * 	GetDlgItem(id)->GetWindowRect(rect);
 * 	ScreenToClient(rect);
 * 	if (o & RS_HCENTER) 
 * 		id##_RS__l = rect.Width()/2; 
 * 	else 
 * 	{
 * 		if(l==RS_BORDER)
 * 			id##_RS__l = rect.left; 
 * 		else if(l==RS_KEEPSIZE) 
 * 			id##_RS__l = rect.Width(); 
 * 		else 
 * 		{
 * 			GetDlgItem(l)->GetWindowRect(rect2); 
 * 			ScreenToClient(rect2);
 * 			id##_RS__l = rect.left-rect2.right;
 * 		}
 * 	}
 * 	if(o & RS_VCENTER) 
 * 		id##_RS__t = rect.Height()/2; 
 * 	else 
 * 	{
 * 		if(t==RS_BORDER) 
 * 			id##_RS__t = rect.top; 
 * 		else if(t==RS_KEEPSIZE) 
 * 			id##_RS__t = rect.Height(); 
 * 		else 
 * 		{
 * 			GetDlgItem(t)->GetWindowRect(rect2); 
 * 			ScreenToClient(rect2);
 * 			id##_RS__t = rect.top-rect2.bottom;
 * 		}
 * 	}
 * 	if(o & RS_HCENTER) 
 * 		id##_RS__r = rect.Width(); 
 * 	else 
 * 	{ 
 * 		if(r==RS_BORDER) 
 * 			id##_RS__r = client.right-rect.right; 
 * 		else if(r==RS_KEEPSIZE) 
 * 			id##_RS__r = rect.Width(); 
 * 		else
 * 		{
 * 			GetDlgItem(r)->GetWindowRect(rect2); 
 * 			ScreenToClient(rect2);
 * 			id##_RS__r = rect2.left-rect.right;
 * 		}
 * 	}
 * 	if(o & RS_VCENTER)
 * 		id##_RS__b = rect.Height();
 * 	else  
 * 	{ 
 * 		if(b==RS_BORDER)
 * 			id##_RS__b = client.bottom-rect.bottom;
 * 		else if(b==RS_KEEPSIZE)
 * 			id##_RS__b = rect.Height();
 * 		else
 * 		{
 * 			GetDlgItem(b)->GetWindowRect(rect2);
 * 			ScreenToClient(rect2);
 * 			id##_RS__b = rect2.top-rect.bottom;
 * 		}
 * 	}
 * }
 * else 
 * {
 * 	int left,top,right,bottom; 
 * 	BOOL bR = FALSE,bB = FALSE;
 * 	if(o & RS_HCENTER) 
 * 	{
 * 		int _a,_b;
 * 		if(l==RS_BORDER) 
 * 			_a = client.left;
 * 		else
 * 		{
 * 			GetDlgItem(l)->GetWindowRect(rect2);
 * 			ScreenToClient(rect2);
 * 			_a = rect2.right;
 * 		}
 * 		if(r==RS_BORDER)
 * 			_b = client.right;
 * 		else
 * 		{
 * 			GetDlgItem(r)->GetWindowRect(rect2);
 * 			ScreenToClient(rect2);
 * 			_b = rect2.left;
 * 		}
 * 		left = _a+((_b-_a)/2-id##_RS__l);
 * 		right = left + id##_RS__r;
 * 	} 
 * 	else 
 * 	{
 * 		if(l==RS_BORDER) 
 * 			left = id##_RS__l;
 * 		else if(l==RS_KEEPSIZE) 
 * 		{ 
 * 			__RS__CalcBottomRight(this,FALSE,right,left,id,r,id##_RS__r,rect,client.right); 
 * 			left = right-id##_RS__l;
 * 		} 
 * 		else 
 * 		{ 
 * 			GetDlgItem(l)->GetWindowRect(rect2);
 * 			ScreenToClient(rect2);
 * 			left = rect2.right + id##_RS__l;
 * 		}
 * 		if(l != RS_KEEPSIZE)
 * 			__RS__CalcBottomRight(this,FALSE,right,left,id,r,id##_RS__r,rect,client.right);
 *	}
 *	if(o & RS_VCENTER)
 * 	{ 
 * 		int _a,_b;
 * 		if(t==RS_BORDER)
 * 			_a = client.top;
 * 		else
 * 		{
 * 			GetDlgItem(t)->GetWindowRect(rect2);
 * 			ScreenToClient(rect2);
 * 			_a = rect2.bottom;
 * 		}
 * 		if(b==RS_BORDER)
 * 			_b = client.bottom;
 * 		else
 * 		{
 * 			GetDlgItem(b)->GetWindowRect(rect2);
 * 			ScreenToClient(rect2);
 * 			_b = rect2.top;
 * 		}
 * 		top = _a+((_b-_a)/2-id##_RS__t);
 * 		bottom = top + id##_RS__b;
 * 	}
 * 	else
 * 	{
 * 		if(t==RS_BORDER)
 *			top = id##_RS__t;
 * 		else if(t==RS_KEEPSIZE) 
 * 		{ 
 * 			__RS__CalcBottomRight(this,TRUE,bottom,top,id,b,id##_RS__b,rect,client.bottom); 
 * 			top = bottom-id##_RS__t;
 * 		} 
 * 		else 
 * 		{ 
 * 			GetDlgItem(t)->GetWindowRect(rect2); 
 * 			ScreenToClient(rect2); 
 * 			top = rect2.bottom + id##_RS__t; 
 * 		}
 * 		if(t != RS_KEEPSIZE) 
 * 			__RS__CalcBottomRight(this,TRUE,bottom,top,id,b,id##_RS__b,rect,client.bottom);
 * 	}
 * 	GetDlgItem(id)->MoveWindow(left,top,right-left,bottom-top,FALSE);
 * }
 * \endcode
 */

/**
 * \def END_RESIZER_MAP
 * Marks the end of a resizer map. See RESIZER for details.
 */

/**
 * \def RESIZER_GRIP
 * \hideinitializer
 * Add this macro somewhere inside the OnPaint() method of your dialog class
 */

/**
 * @}
 */

#pragma once

#define RS_BORDER 0xffffffff
#define RS_KEEPSIZE 0xfffffffe
#define RS_HCENTER 0x00000001
#define RS_VCENTER 0x00000002
// declaration of method to reposition and method to calculate the bottom right corner
#define DECLARE_RESIZER \
void __RS__RepositionControls(BOOL bInit);\
void __RS__CalcBottomRight(CWnd *pThis, BOOL bBottom, int &bottomright, int &topleft, UINT id, UINT br, int RS_br, CRect &rect, int clientbottomright);
#define INIT_RESIZER __RS__RepositionControls(TRUE); __RS__RepositionControls(FALSE)

//reposition the controls for the window
#define UPDATE_RESIZER if(GetWindow(GW_CHILD)!=NULL) __RS__RepositionControls(FALSE)
//don't let the window get smaller
#define RESIZER_MINSIZE(mx,my,s,r) if(r->right-r->left < mx) { if((s == WMSZ_BOTTOMLEFT)||(s == WMSZ_LEFT)||(s == WMSZ_TOPLEFT)) r->left = r->right-mx; else r->right = r->left+mx; } if(r->bottom-r->top < my) { if((s == WMSZ_TOP)||(s == WMSZ_TOPLEFT)||(s == WMSZ_TOPRIGHT)) r->top = r->bottom-my; else r->bottom = r->top+my; }
//implementation of the methods
#define BEGIN_RESIZER_MAP(class) \
void class::__RS__CalcBottomRight(CWnd *pThis, BOOL bBottom, int &bottomright, int &topleft, UINT id, UINT br, int RS_br, CRect &rect, int clientbottomright) {\
if(br==RS_BORDER) bottomright = clientbottomright-RS_br;\
else if(br==RS_KEEPSIZE) bottomright = topleft+RS_br;\
else { CRect rect2;\
pThis->GetDlgItem(br)->GetWindowRect(rect2); pThis->ScreenToClient(rect2);\
bottomright = (bBottom?rect2.top:rect2.left) - RS_br;}}\
void class::__RS__RepositionControls(BOOL bInit) { CRect rect,rect2,client; GetClientRect(client);
#define END_RESIZER_MAP Invalidate(); UpdateWindow(); }
//one RESIZER entry:
#define RESIZER(id,l,t,r,b,o) \
static int id##_RS__l, id##_RS__t, id##_RS__r, id##_RS__b;\
if(bInit) {\
GetDlgItem(id)->GetWindowRect(rect); ScreenToClient(rect);\
if(o & RS_HCENTER) id##_RS__l = rect.Width()/2; else {\
if(l==RS_BORDER) id##_RS__l = rect.left; else if(l==RS_KEEPSIZE) id##_RS__l = rect.Width(); else {\
	GetDlgItem(l)->GetWindowRect(rect2); ScreenToClient(rect2);\
	id##_RS__l = rect.left-rect2.right;}}\
if(o & RS_VCENTER) id##_RS__t = rect.Height()/2; else {\
if(t==RS_BORDER) id##_RS__t = rect.top; else if(t==RS_KEEPSIZE) id##_RS__t = rect.Height(); else {\
	GetDlgItem(t)->GetWindowRect(rect2); ScreenToClient(rect2);\
	id##_RS__t = rect.top-rect2.bottom;}}\
if(o & RS_HCENTER) id##_RS__r = rect.Width(); else { if(r==RS_BORDER) id##_RS__r = client.right-rect.right; else if(r==RS_KEEPSIZE) id##_RS__r = rect.Width(); else {\
	GetDlgItem(r)->GetWindowRect(rect2); ScreenToClient(rect2);\
	id##_RS__r = rect2.left-rect.right;}}\
if(o & RS_VCENTER) id##_RS__b = rect.Height(); else  { if(b==RS_BORDER) id##_RS__b = client.bottom-rect.bottom; else if(b==RS_KEEPSIZE) id##_RS__b = rect.Height(); else {\
	GetDlgItem(b)->GetWindowRect(rect2); ScreenToClient(rect2);\
	id##_RS__b = rect2.top-rect.bottom;}}\
} else {\
int left,top,right,bottom; BOOL bR = FALSE,bB = FALSE;\
if(o & RS_HCENTER) { int _a,_b;\
if(l==RS_BORDER) _a = client.left; else { GetDlgItem(l)->GetWindowRect(rect2); ScreenToClient(rect2); _a = rect2.right; }\
if(r==RS_BORDER) _b = client.right; else { GetDlgItem(r)->GetWindowRect(rect2); ScreenToClient(rect2); _b = rect2.left; }\
left = _a+((_b-_a)/2-id##_RS__l); right = left + id##_RS__r;} else {\
if(l==RS_BORDER) left = id##_RS__l;\
else if(l==RS_KEEPSIZE) { __RS__CalcBottomRight(this,FALSE,right,left,id,r,id##_RS__r,rect,client.right); left = right-id##_RS__l;\
} else { GetDlgItem(l)->GetWindowRect(rect2); ScreenToClient(rect2); left = rect2.right + id##_RS__l; }\
if(l != RS_KEEPSIZE) __RS__CalcBottomRight(this,FALSE,right,left,id,r,id##_RS__r,rect,client.right);}\
if(o & RS_VCENTER) { int _a,_b;\
if(t==RS_BORDER) _a = client.top; else { GetDlgItem(t)->GetWindowRect(rect2); ScreenToClient(rect2); _a = rect2.bottom; }\
if(b==RS_BORDER) _b = client.bottom; else { GetDlgItem(b)->GetWindowRect(rect2); ScreenToClient(rect2); _b = rect2.top; }\
top = _a+((_b-_a)/2-id##_RS__t); bottom = top + id##_RS__b;} else {\
if(t==RS_BORDER) top = id##_RS__t;\
else if(t==RS_KEEPSIZE) { __RS__CalcBottomRight(this,TRUE,bottom,top,id,b,id##_RS__b,rect,client.bottom); top = bottom-id##_RS__t;\
} else { GetDlgItem(t)->GetWindowRect(rect2); ScreenToClient(rect2); top = rect2.bottom + id##_RS__t; }\
if(t != RS_KEEPSIZE) __RS__CalcBottomRight(this,TRUE,bottom,top,id,b,id##_RS__b,rect,client.bottom);}\
GetDlgItem(id)->MoveWindow(left,top,right-left,bottom-top,FALSE);\
}

#define RESIZER_GRIP \
	CPaintDC _dc(this);RECT _r = _dc.m_ps.rcPaint;_r.top = _r.bottom - GetSystemMetrics( SM_CYHSCROLL );_r.left = _r.right - GetSystemMetrics( SM_CXVSCROLL ); _dc.DrawFrameControl( &_r, DFC_SCROLL, DFCS_SCROLLSIZEGRIP )
