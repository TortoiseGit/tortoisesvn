// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__0A781DD9_5C37_49E2_A4F5_E517F5B8A621__INCLUDED_)
#define AFX_STDAFX_H__0A781DD9_5C37_49E2_A4F5_E517F5B8A621__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Set target Windows platform
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

// Use target Common Controls version for compatibility
// with CPropertyPageEx, CPropertySheetEx
#define _WIN32_IE 0x0500

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls

#ifndef WS_EX_LAYOUTRTL
#pragma message("Please update your Windows header files, get the latest SDK")
#pragma message("WinUser.h is out of date!")

#define WS_EX_LAYOUTRTL		0x00400000
#endif

#ifndef WC_BUTTON
#pragma message("Please update your Windows header files, get the latest SDK")
#pragma message("CommCtrl.h is out of date!")

#define WC_BUTTON			_T("Button")
#define WC_STATIC			_T("Static")
#define WC_EDIT				_T("Edit")
#define WC_LISTBOX			_T("ListBox")
#define WC_COMBOBOX			_T("ComboBox")
#define WC_SCROLLBAR		_T("ScrollBar")
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0A781DD9_5C37_49E2_A4F5_E517F5B8A621__INCLUDED_)
