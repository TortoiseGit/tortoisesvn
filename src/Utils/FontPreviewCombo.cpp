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

#include "stdafx.h"
#include "resource.h"
#include "FontPreviewCombo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SPACING      10
#define GLYPH_WIDTH  15

/////////////////////////////////////////////////////////////////////////////
// CFontPreviewCombo

CFontPreviewCombo::CFontPreviewCombo()
{
	m_iFontHeight = 16;
	m_iMaxNameWidth = 0;
    m_iMaxSampleWidth = 0;
	m_style = NAME_THEN_SAMPLE;
	m_csSample = _T("abcimABCIM");
	m_clrSample = GetSysColor(COLOR_WINDOWTEXT);
}

CFontPreviewCombo::~CFontPreviewCombo()
{
	DeleteAllFonts();
}


BEGIN_MESSAGE_MAP(CFontPreviewCombo, CComboBox)
	//{{AFX_MSG_MAP(CFontPreviewCombo)
	ON_WM_MEASUREITEM()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontPreviewCombo message handlers


int CALLBACK FPC_EnumFontProc (ENUMLOGFONTEX * lpelfe, NEWTEXTMETRICEX * /*lpntme*/, 
						        DWORD FontType, LPARAM lParam)	
{	
	CFontPreviewCombo *pThis = reinterpret_cast<CFontPreviewCombo*>(lParam);
	if (pThis->FindStringExact(-1, lpelfe->elfLogFont.lfFaceName)!=CB_ERR)
	{
		return TRUE;
	} // if (_tcscmp(buf, lpelfe->elfLogFont.lfFaceName)==0 
	int index = pThis->AddString(lpelfe->elfLogFont.lfFaceName);
	ASSERT(index!=-1);
	int ret = pThis->SetItemData (index, FontType); 

	VERIFY(ret!=-1);

	pThis->AddFont (lpelfe->elfLogFont.lfFaceName);
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

void CFontPreviewCombo::Init()
{
	LOGFONT lf;
	m_img.Detach();
	m_img.Create(IDB_TTF_BMP, GLYPH_WIDTH, 1, RGB(255,255,255));
	CClientDC dc(this);		

	ResetContent();
	DeleteAllFonts();
	ZeroMemory(&lf, sizeof(LOGFONT));
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfFaceName[0] = '\0';
	EnumFontFamiliesEx (dc, &lf, (FONTENUMPROC)FPC_EnumFontProc,(LPARAM)this, 0); //Enumerate font

    SetCurSel(0);
}

/////////////////////////////////////////////////////////////////////////////

void CFontPreviewCombo::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
{
	ASSERT(lpDIS->CtlType == ODT_COMBOBOX); 
	
	CRect rc = lpDIS->rcItem;
	
	CDC dc;
	dc.Attach(lpDIS->hDC);

	if (lpDIS->itemState & ODS_FOCUS)
		dc.DrawFocusRect(&rc);
	
	if (lpDIS->itemID == -1)
		return;

	int nIndexDC = dc.SaveDC();
	
	CBrush br;
	
	COLORREF clrSample = m_clrSample;

	if (lpDIS->itemState & ODS_SELECTED)
	{
		br.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		clrSample = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	else
	{
		br.CreateSolidBrush(dc.GetBkColor());
	}
	
	dc.SetBkMode(TRANSPARENT);
	dc.FillRect(&rc, &br);
	
	// which one are we working on?
	CString csCurFontName;
	GetLBText(lpDIS->itemID, csCurFontName);

	// draw the cute TTF glyph
	DWORD dwData = (DWORD)GetItemData(lpDIS->itemID);
	if (dwData & TRUETYPE_FONTTYPE)
	{
		m_img.Draw(&dc, 0, CPoint(rc.left+5, rc.top+4),ILD_TRANSPARENT);
	}
	rc.left += GLYPH_WIDTH;
	
	int iOffsetX = SPACING;

	// draw the text
	CSize sz;
	int iPosY = 0;
	HFONT hf = NULL;
	CFont* cf;
	m_fonts.Lookup (csCurFontName, (void*&)cf);
	switch (m_style)
	{
	case NAME_GUI_FONT:
		{
			// font name in GUI font
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY,csCurFontName);
		}
		break;
	case NAME_ONLY:
		{
			// font name in current font
			hf = (HFONT)dc.SelectObject(*cf);
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY,csCurFontName);
			dc.SelectObject(hf);
		}
		break;
	case NAME_THEN_SAMPLE:
		{
			// font name in GUI font
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY, csCurFontName);

         // condense, for edit
         int iSep = m_iMaxNameWidth;
         if ((lpDIS->itemState & ODS_COMBOBOXEDIT) == ODS_COMBOBOXEDIT)
         {
            iSep = sz.cx;
         }

         // sample in current font
			hf = (HFONT)dc.SelectObject(*cf);
			sz = dc.GetTextExtent(m_csSample);
			iPosY = (rc.Height() - sz.cy) / 2;
			COLORREF clr = dc.SetTextColor(clrSample);
			dc.TextOut(rc.left + iOffsetX + iSep + iOffsetX, rc.top + iPosY, m_csSample);
			dc.SetTextColor(clr);
			dc.SelectObject(hf);
		}
		break;
	case SAMPLE_THEN_NAME:
		{
         // sample in current font
			hf = (HFONT)dc.SelectObject(*cf);
			sz = dc.GetTextExtent(m_csSample);
			iPosY = (rc.Height() - sz.cy) / 2;
			COLORREF clr = dc.SetTextColor(clrSample);
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY, m_csSample);
			dc.SetTextColor(clr);
			dc.SelectObject(hf);

         // condense, for edit
         int iSep = m_iMaxSampleWidth;
         if ((lpDIS->itemState & ODS_COMBOBOXEDIT) == ODS_COMBOBOXEDIT)
         {
            iSep = sz.cx;
         }

			// font name in GUI font
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left + iOffsetX + iSep + iOffsetX, rc.top + iPosY, csCurFontName);
		}
		break;
	case SAMPLE_ONLY:
		{			
			// sample in current font
			hf = (HFONT)dc.SelectObject(*cf);
			sz = dc.GetTextExtent(m_csSample);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY, m_csSample);
			dc.SelectObject(hf);
		}
		break;
	}

	dc.RestoreDC(nIndexDC);

	dc.Detach();
}

/////////////////////////////////////////////////////////////////////////////

void CFontPreviewCombo::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	// ok, how big is this ?

	CString csFontName;
	GetLBText(lpMeasureItemStruct->itemID, csFontName);

	CFont cf;
	if (!cf.CreateFont(m_iFontHeight,0,0,0,FW_NORMAL,FALSE, FALSE, FALSE,DEFAULT_CHARSET ,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH, csFontName))
	{
		ASSERT(0);
		return;
	}

	LOGFONT lf;
	cf.GetLogFont(&lf);

	if ((m_style == NAME_ONLY) || (m_style == SAMPLE_ONLY) || (m_style == NAME_GUI_FONT))
	{
		m_iMaxNameWidth = 0;
		m_iMaxSampleWidth = 0;
	}
	else
	{
		CClientDC dc(this);

		// measure font name in GUI font
		HFONT hFont = ((HFONT)GetStockObject( DEFAULT_GUI_FONT ));
		HFONT hf = (HFONT)dc.SelectObject(hFont);
		CSize sz = dc.GetTextExtent(csFontName);
		m_iMaxNameWidth = max(m_iMaxNameWidth, sz.cx);
		dc.SelectObject(hf);

		// measure sample in cur font
		hf = (HFONT)dc.SelectObject(cf);
      if (hf)
      {
		   sz = dc.GetTextExtent(m_csSample);
		   m_iMaxSampleWidth = max(m_iMaxSampleWidth, sz.cx);
		   dc.SelectObject(hf);
      }
	}

	lpMeasureItemStruct->itemHeight = lf.lfHeight + 4;
}

/////////////////////////////////////////////////////////////////////////////

void CFontPreviewCombo::OnDropdown() 
{
   int nScrollWidth = ::GetSystemMetrics(SM_CXVSCROLL);
   int nWidth = nScrollWidth;
   nWidth += GLYPH_WIDTH;

	switch (m_style)
	{
	case NAME_GUI_FONT:
      nWidth += m_iMaxNameWidth;
		break;
	case NAME_ONLY:
      nWidth += m_iMaxNameWidth;
		break;
	case NAME_THEN_SAMPLE:
      nWidth += m_iMaxNameWidth;
      nWidth += m_iMaxSampleWidth;
      nWidth += SPACING * 2;
		break;
	case SAMPLE_THEN_NAME:
      nWidth += m_iMaxNameWidth;
      nWidth += m_iMaxSampleWidth;
      nWidth += SPACING * 2;
		break;
	case SAMPLE_ONLY:
      nWidth += m_iMaxSampleWidth;
		break;
	}

   SetDroppedWidth(nWidth);
}

void
CFontPreviewCombo::AddFont (const CString& faceName)
{
	if (m_style != NAME_GUI_FONT)
	{
		CFont* cf = new CFont;
		BOOL createFontResult = 
			cf->CreateFont(m_iFontHeight,0,0,0,FW_NORMAL,FALSE, FALSE, 
			FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH, faceName);
		VERIFY (createFontResult);
		CFont* old_cf = NULL;
		if (m_fonts.Lookup(faceName, (void*&)old_cf))
			delete old_cf;

		m_fonts.SetAt (faceName, cf);
	}
}

void CFontPreviewCombo::SetFontHeight(int newHeight, bool reinitialize)
{
	if (newHeight == m_iFontHeight) return;
	m_iFontHeight = newHeight;
	if (reinitialize)
		Init();
}

int CFontPreviewCombo::GetFontHeight()
{
	return m_iFontHeight;
}

void CFontPreviewCombo::SetPreviewStyle(PreviewStyle style, bool reinitialize)
{
	if (style == m_style) return;
	m_style = style;
	if (reinitialize)
		Init();
}

int CFontPreviewCombo::GetPreviewStyle()
{
	return m_style;
}

void CFontPreviewCombo::DeleteAllFonts()
{
	POSITION i = m_fonts.GetStartPosition();
	CFont* font;
	CString dummy;
	while (i != NULL)
	{
		m_fonts.GetNextAssoc (i, dummy, (void*&)font);
		delete font;
	}
	m_fonts.RemoveAll ();
}

void WINAPI 
DDX_FontPreviewCombo (CDataExchange* pDX, int nIDC, CString& faceName)
{
    HWND hWndCtrl = pDX->PrepareCtrl(nIDC);
    _ASSERTE (hWndCtrl != NULL);                
    
    CFontPreviewCombo* ctrl = 
		(CFontPreviewCombo*) CWnd::FromHandle(hWndCtrl);
 
	if (pDX->m_bSaveAndValidate) //data FROM control
	{
		//no validation needed when coming FROM control
		int pos = ctrl->GetCurSel();
		if (pos != CB_ERR)
		{
			ctrl->GetLBText(pos, faceName);
		}
		else
			faceName = "";
	}
	else //data TO control
	{
		//need to make sure this fontname is one we have
		//but if it's not we can't use the Fail() DDX mechanism since we're
		//not in save-and-validate mode.  So instead we just set the 
		//selection to the first item, which is the default anyway, if the
		//facename isn't in the box.

		int pos = ctrl->FindString (-1, faceName);
		if (pos != CB_ERR)
		{
			ctrl->SetCurSel (pos);
		}
		else
			ctrl->SetCurSel (0);
	}
}



