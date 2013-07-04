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
#include "scintilla.h"


WPARAM GetScintillaFontQuality()
{
    BOOL bSmooth = FALSE;
    SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &bSmooth, 0);
    UINT uSmoothType = 0;
    if (bSmooth)
        SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &uSmoothType, 0);
    WPARAM scintillaFontQuality = SC_EFF_QUALITY_LCD_OPTIMIZED;
    switch (uSmoothType)
    {
    case FE_FONTSMOOTHINGSTANDARD:
        scintillaFontQuality = SC_EFF_QUALITY_ANTIALIASED;
        break;
    case FE_FONTSMOOTHINGCLEARTYPE:
        scintillaFontQuality = SC_EFF_QUALITY_LCD_OPTIMIZED;
        break;
    default:
        scintillaFontQuality = SC_EFF_QUALITY_NON_ANTIALIASED;
        break;

    }
    return scintillaFontQuality;
}