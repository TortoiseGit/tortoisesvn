// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>


class CD2D
{
private:
    CD2D(void);
    CD2D(const CD2D&) {}
    ~CD2D(void);

public:
    static CD2D& Instance();
public:
    bool IsD2D() const { return (m_pDirect2dFactory != nullptr); }

    HRESULT CreateHwndRenderTarget(HWND hWnd, ID2D1HwndRenderTarget** target);
    HRESULT CreateTextFormat(const WCHAR * fontFamilyName,
                             DWRITE_FONT_WEIGHT  fontWeight,
                             DWRITE_FONT_STYLE  fontStyle,
                             DWRITE_FONT_STRETCH  fontStretch,
                             int fontSize,
                             IDWriteTextFormat ** textFormat);

    HRESULT CreateTextLayout(const WCHAR * string,
                             UINT32  stringLength,
                             IDWriteTextFormat * textFormat,
                             FLOAT  maxWidth,
                             FLOAT  maxHeight,
                             IDWriteTextLayout ** textLayout);

    static HRESULT LoadResourceBitmap(ID2D1RenderTarget *pRT,
                                      IWICImagingFactory *pIWICFactory,
                                      PCWSTR resourceName,
                                      PCWSTR resourceType,
                                      __deref_out ID2D1Bitmap **ppBitmap);

    static inline COLORREF GetBgra(COLORREF rgb)
    {
        // Swaps color order (bgra <-> rgba) from COLORREF to D2D/GDI+'s.
        // Sets alpha to full opacity.
        return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)) | 0xFF000000;
    }

    static inline COLORREF GetColorRef(UINT32 bgra)
    {
        // Swaps color order (bgra <-> rgba) from D2D/GDI+'s to a COLORREF.
        // This also leaves the top byte 0, since alpha is ignored anyway.
        return RGB(GetBValue(bgra), GetGValue(bgra), GetRValue(bgra));
    }
private:
    static CD2D *           m_pInstance;
    static ID2D1Factory *   m_pDirect2dFactory;
    static IDWriteFactory * m_pWriteFactory;
    static HMODULE          m_hD2D1Lib;
    static HMODULE          m_hWriteLib;
};

