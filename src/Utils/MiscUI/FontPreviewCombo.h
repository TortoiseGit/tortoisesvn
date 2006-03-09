/*
   - Replaced the deprecated EnumFonts function with the recommended
     EnumFontFamiliesEx() API call.
   - Filter the various fonts, so that fonts with the same name only
     appear once in the combobox.
   - init() param to filter out variable width fonts
   
   01-03-04 | Stefan Kueng
*/
/*********************************************************************

   Copyright (C) 2002 Smaller Animals Software, Inc.

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

   3. This notice may not be removed or altered from any source distribution.

   http://www.smalleranimals.com
   smallest@smalleranimals.com

   --------

   This code is based, in part, on:
   "A WTL-based Font preview combo box", Ramon Smits
   http://www.codeproject.com/wtl/rsprevfontcmb.asp

**********************************************************************/

#if !defined(AFX_FONTPREVIEWCOMBO_H__3787F1C9_E55D_4F86_A3F2_2405B523A6DB__INCLUDED_)
#define AFX_FONTPREVIEWCOMBO_H__3787F1C9_E55D_4F86_A3F2_2405B523A6DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

int CALLBACK FPC_EnumFontProc (ENUMLOGFONTEX * lpelfe, NEWTEXTMETRICEX *lpntme, 
						        DWORD FontType, LPARAM lParam);	

void WINAPI DDX_FontPreviewCombo (CDataExchange* pDX, int nIDC, CString& faceName);
// FontPreviewCombo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFontPreviewCombo window

class CFontPreviewCombo : public CComboBox
{
// Construction
public:
	CFontPreviewCombo();

// Attributes
public:

   /*
      All of the following options must be set before you call Init() !!
   */

	// set the "sample" text string, defaults to "abcdeABCDE"
	CString m_csSample;

	// choose the sample color (only applies with NAME_THEN_SAMPLE and SAMPLE_THEN_NAME)
	COLORREF	m_clrSample;

	// choose how the name and sample are displayed
	typedef enum
	{
		NAME_ONLY = 0,		// font name, drawn in font
		NAME_GUI_FONT,		// font name, drawn in GUI font
		NAME_THEN_SAMPLE,	// font name in GUI font, then sample text in font
		SAMPLE_THEN_NAME,	// sample text in font, then font name in GUI font
		SAMPLE_ONLY			// sample text in font
	} PreviewStyle;

// Operations
public:
	
	// call this to load the font strings
	void	Init(bool bFixedWidthOnly = false);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFontPreviewCombo)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	int GetPreviewStyle (void);
	void SetPreviewStyle (PreviewStyle style, bool reinitialize = true);
	int GetFontHeight (void);
	void SetFontHeight (int newHeight, bool reinitialize = true);
	void AdjustHeight(CWnd * heightRefControl)
	{
		if (heightRefControl)
		{
			LRESULT lbItemHeight = heightRefControl->SendMessage(CB_GETITEMHEIGHT, (WPARAM)-1);
			SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, lbItemHeight);
		}
	}
	virtual ~CFontPreviewCombo();
	
protected:
	CImageList m_img;	
	CMapStringToPtr m_fonts;

	PreviewStyle	m_style;
	int m_iFontHeight;
	int m_iMaxNameWidth;
	int m_iMaxSampleWidth;
	bool m_bFixedWidthOnly;

	void AddFont (const CString& faceName);
	friend int CALLBACK FPC_EnumFontProc(ENUMLOGFONTEX * lpelfe, NEWTEXTMETRICEX *lpntme, 
						        DWORD FontType, LPARAM lParam);
	void DeleteAllFonts (void);	

	// Generated message map functions
	//{{AFX_MSG(CFontPreviewCombo)
	afx_msg void OnDropdown();
	//}}AFX_MSG

	

	DECLARE_MESSAGE_MAP()


};

//void WINAPI DDX_FontPreviewCombo (CDataExchange* pDX, int nIDC, CString& faceName);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FONTPREVIEWCOMBO_H__3787F1C9_E55D_4F86_A3F2_2405B523A6DB__INCLUDED_)
