#pragma once

//////////////////////////////////////////////////
// CMemDC - memory DC
//
// Author: Keith Rule
// Email:  keithr@europa.com
// Copyright 1996-1997, Keith Rule
//
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// History - 10/3/97 Fixed scrolling bug.
//                   Added print support.
//           25 feb 98 - fixed minor assertion bug
//
// This class implements a memory Device Context

#ifdef _MFC_VER
class CMemDC : public CDC
{
public:
	// constructor sets up the memory DC
	CMemDC(CDC* pDC, int nOffset = 0) : CDC()
    {
		ASSERT(pDC != NULL);
		
		m_pDC = pDC;
		m_pOldBitmap = NULL;
        m_bMemDC = ((!pDC->IsPrinting()) && (!GetSystemMetrics(SM_REMOTESESSION)));
		
        if (m_bMemDC)	// Create a Memory DC
		{
            pDC->GetClipBox(&m_rect);
            CreateCompatibleDC(pDC);
            m_bitmap.CreateCompatibleBitmap(pDC, m_rect.Width() - nOffset, m_rect.Height());
			m_pOldBitmap = SelectObject(&m_bitmap);
            SetWindowOrg(m_rect.left, m_rect.top);
        }
		else		// Make a copy of the relevent parts of the current DC for printing
		{
            m_bPrinting = pDC->m_bPrinting;
            m_hDC		= pDC->m_hDC;
            m_hAttribDC = pDC->m_hAttribDC;
        }

		FillSolidRect(m_rect, pDC->GetBkColor());
	}
	
	CMemDC(CDC* pDC, const CRect* pRect) : CDC()
	{
		ASSERT(pDC != NULL); 

		// Some initialization
		m_pDC = pDC;
		m_pOldBitmap = NULL;
		m_bMemDC = !pDC->IsPrinting();

		// Get the rectangle to draw
		if (pRect == NULL) {
			pDC->GetClipBox(&m_rect);
		} else {
			m_rect = *pRect;
		}

		if (m_bMemDC) {
			// Create a Memory DC
			CreateCompatibleDC(pDC);
			pDC->LPtoDP(&m_rect);

			m_bitmap.CreateCompatibleBitmap(pDC, m_rect.Width(), m_rect.Height());
			m_pOldBitmap = SelectObject(&m_bitmap);

			SetMapMode(pDC->GetMapMode());

			SetWindowExt(pDC->GetWindowExt());
			SetViewportExt(pDC->GetViewportExt());

			pDC->DPtoLP(&m_rect);
			SetWindowOrg(m_rect.left, m_rect.top);
		} else {
			// Make a copy of the relevent parts of the current DC for printing
			m_bPrinting = pDC->m_bPrinting;
			m_hDC       = pDC->m_hDC;
			m_hAttribDC = pDC->m_hAttribDC;
		}

		// Fill background 
		FillSolidRect(m_rect, pDC->GetBkColor());
	}
	
	// Destructor copies the contents of the mem DC to the original DC
	~CMemDC()
    {
		if (m_bMemDC) {	
			// Copy the offscreen bitmap onto the screen.
			m_pDC->BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(),
				this, m_rect.left, m_rect.top, SRCCOPY);
			
            //Swap back the original bitmap.
            SelectObject(m_pOldBitmap);
		} else {
			// All we need to do is replace the DC with an illegal value,
			// this keeps us from accidently deleting the handles associated with
			// the CDC that was passed to the constructor.
            m_hDC = m_hAttribDC = NULL;
		}
	}
	
	// Allow usage as a pointer
    CMemDC* operator->() {return this;}
	
    // Allow usage as a pointer
    operator CMemDC*() {return this;}

private:
	CBitmap  m_bitmap;		// Offscreen bitmap
    CBitmap* m_pOldBitmap;	// bitmap originally found in CMemDC
    CDC*     m_pDC;			// Saves CDC passed in constructor
    CRect    m_rect;		// Rectangle of drawing area.
    BOOL     m_bMemDC;		// TRUE if CDC really is a Memory DC.
};
#else
class CMemDC
{
public:

	// constructor sets up the memory DC
	CMemDC(HDC hDC, int nOffset = 0) 
	{	
		m_hDC = hDC;
		m_hOldBitmap = NULL;

		::GetClipBox(m_hDC, &m_rect);
		m_hMemDC = ::CreateCompatibleDC(m_hDC);
		m_hBitmap = CreateCompatibleBitmap(m_hDC, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top);
		m_hOldBitmap = (HBITMAP)SelectObject(m_hMemDC, m_hBitmap);
		SetWindowOrgEx(m_hMemDC, m_rect.left, m_rect.top, NULL);
	}

	// Destructor copies the contents of the mem DC to the original DC
	~CMemDC()
	{
		if (m_hMemDC) {	
			// Copy the offscreen bitmap onto the screen.
			BitBlt(m_hDC, m_rect.left, m_rect.top, m_rect.right-m_rect.left, m_rect.bottom-m_rect.top, m_hMemDC, m_rect.left, m_rect.top, SRCCOPY);

			//Swap back the original bitmap.
			SelectObject(m_hMemDC, m_hOldBitmap);
			DeleteObject(m_hBitmap);
			DeleteDC(m_hMemDC);
		} else {
			// All we need to do is replace the DC with an illegal value,
			// this keeps us from accidently deleting the handles associated with
			// the CDC that was passed to the constructor.
			DeleteObject(m_hBitmap);
			DeleteDC(m_hMemDC);
			m_hMemDC = NULL;
		}
	}

	// Allow usage as a pointer
	operator HDC() {return m_hMemDC;}

private:
	HBITMAP  m_hBitmap;		// Offscreen bitmap
	HBITMAP	 m_hOldBitmap;	// bitmap originally found in CMemDC
	HDC      m_hDC;			// Saves CDC passed in constructor
	HDC		 m_hMemDC;		// our own memory DC
	RECT	 m_rect;		// Rectangle of drawing area.
};

#endif

