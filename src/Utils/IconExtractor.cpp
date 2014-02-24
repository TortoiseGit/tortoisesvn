// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2012, 2014 - TortoiseSVN

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
#include "stdafx.h"
#include "IconExtractor.h"
#include "SmartHandle.h"

#define WIDTHBYTES(bits)      ((((bits) + 31)>>5)<<2)

CIconExtractor::CIconExtractor()
{
}

DWORD CIconExtractor::ExtractIcon(HINSTANCE hResource, LPCTSTR id, LPCTSTR TargetICON)
{
    LPICONRESOURCE      lpIR    = NULL;
    HRSRC               hRsrc   = NULL;
    HGLOBAL             hGlobal = NULL;
    LPMEMICONDIR        lpIcon  = NULL;

    // Find the group icon resource
    hRsrc = FindResource(hResource, id, RT_GROUP_ICON);

    if (hRsrc == NULL)
        return GetLastError();

    if ((hGlobal = LoadResource(hResource, hRsrc)) == NULL)
        return GetLastError();

    if ((lpIcon = (LPMEMICONDIR)LockResource(hGlobal)) == NULL)
        return GetLastError();

    if ((lpIR = (LPICONRESOURCE) malloc(sizeof(ICONRESOURCE) + ((lpIcon->idCount-1) * sizeof(ICONIMAGE)))) == NULL)
        return GetLastError();

    lpIR->nNumImages = lpIcon->idCount;

    // Go through all the icons
    for (UINT i = 0; i < lpIR->nNumImages; ++i)
    {
        // Get the individual icon
        if ((hRsrc = FindResource(hResource, MAKEINTRESOURCE(lpIcon->idEntries[i].nID), RT_ICON )) == NULL)
        {
            free(lpIR);
            return GetLastError();
        }
        if ((hGlobal = LoadResource(hResource, hRsrc )) == NULL)
        {
            free(lpIR);
            return GetLastError();
        }
        // Store a copy of the resource locally
        lpIR->IconImages[i].dwNumBytes = SizeofResource(hResource, hRsrc);
        lpIR->IconImages[i].lpBits =(LPBYTE) malloc(lpIR->IconImages[i].dwNumBytes);
        if (lpIR->IconImages[i].lpBits == NULL)
        {
            free(lpIR);
            return GetLastError();
        }

        memcpy(lpIR->IconImages[i].lpBits, LockResource(hGlobal), lpIR->IconImages[i].dwNumBytes);

        // Adjust internal pointers
        if (!AdjustIconImagePointers(&(lpIR->IconImages[i])))
        {
            free(lpIR);
            return GetLastError();
        }
    }

    DWORD ret = WriteIconToICOFile(lpIR,TargetICON);

    if (ret)
    {
        free(lpIR);
        return ret;
    }

    free(lpIR);

    return NO_ERROR;
}

DWORD CIconExtractor::WriteIconToICOFile(LPICONRESOURCE lpIR, LPCTSTR szFileName)
{
    DWORD       dwBytesWritten  = 0;

    CAutoFile hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    // open the file
    if (!hFile)
        return GetLastError();

    // Write the header
    if (WriteICOHeader(hFile, lpIR->nNumImages))
        return GetLastError();

    // Write the ICONDIRENTRY's
    for (UINT i = 0; i < lpIR->nNumImages; ++i)
    {
        ICONDIRENTRY    ide;

        // Convert internal format to ICONDIRENTRY
        ide.bWidth      = (BYTE)lpIR->IconImages[i].Width;
        ide.bHeight     = (BYTE)lpIR->IconImages[i].Height;
        ide.bReserved   = 0;
        ide.wPlanes     = lpIR->IconImages[i].lpbi->bmiHeader.biPlanes;
        ide.wBitCount   = lpIR->IconImages[i].lpbi->bmiHeader.biBitCount;

        if ((ide.wPlanes * ide.wBitCount) >= 8)
            ide.bColorCount = 0;
        else
            ide.bColorCount = 1 << (ide.wPlanes * ide.wBitCount);
        ide.dwBytesInRes = lpIR->IconImages[i].dwNumBytes;
        ide.dwImageOffset = CalculateImageOffset( lpIR, i );

        // Write the ICONDIRENTRY to disk
        if (!WriteFile(hFile, &ide, sizeof(ICONDIRENTRY), &dwBytesWritten, NULL))
            return GetLastError();

        if (dwBytesWritten != sizeof(ICONDIRENTRY))
            return GetLastError();
    }

    // Write the image bits for each image
    for (UINT i = 0; i < lpIR->nNumImages; ++i)
    {
        DWORD dwTemp = lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage;
        bool bError = false; // fix size even on error

        // Set the sizeimage member to zero
        lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage = 0;
        if (!WriteFile( hFile, lpIR->IconImages[i].lpBits, lpIR->IconImages[i].dwNumBytes, &dwBytesWritten, NULL))
            bError = true;

        if (dwBytesWritten != lpIR->IconImages[i].dwNumBytes)
            bError = true;

        // set it back
        lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage = dwTemp;
        if (bError)
            return GetLastError();
    }
    return NO_ERROR;
}

DWORD CIconExtractor::CalculateImageOffset(LPICONRESOURCE lpIR, UINT nIndex) const
{
    DWORD   dwSize;

    // Calculate the ICO header size
    dwSize = 3 * sizeof(WORD);
    // Add the ICONDIRENTRY's
    dwSize += lpIR->nNumImages * sizeof(ICONDIRENTRY);
    // Add the sizes of the previous images
    for(UINT i = 0; i < nIndex; ++i)
        dwSize += lpIR->IconImages[i].dwNumBytes;

    return dwSize;
}

DWORD CIconExtractor::WriteICOHeader(HANDLE hFile, UINT nNumEntries) const
{
    WORD    Output          = 0;
    DWORD   dwBytesWritten  = 0;

    // Write 'reserved' WORD
    if (!WriteFile( hFile, &Output, sizeof(WORD), &dwBytesWritten, NULL))
        return GetLastError();
    // Did we write a WORD?
    if (dwBytesWritten != sizeof(WORD))
        return GetLastError();
    // Write 'type' WORD (1)
    Output = 1;
    if (!WriteFile( hFile, &Output, sizeof(WORD), &dwBytesWritten, NULL))
        return GetLastError();
    // Did we write a WORD?
    if (dwBytesWritten != sizeof(WORD))
        return GetLastError();
    // Write Number of Entries
    Output = (WORD)nNumEntries;
    if (!WriteFile(hFile, &Output, sizeof(WORD), &dwBytesWritten, NULL))
        return GetLastError();
    // Did we write a WORD?
    if (dwBytesWritten != sizeof(WORD))
        return GetLastError();

    return NO_ERROR;
}

BOOL CIconExtractor::AdjustIconImagePointers(LPICONIMAGE lpImage)
{
    if (lpImage == NULL)
        return FALSE;

    // BITMAPINFO is at beginning of bits
    lpImage->lpbi = (LPBITMAPINFO)lpImage->lpBits;
    // Width - simple enough
    lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
    // Icons are stored in funky format where height is doubled - account for it
    lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight)/2;
    // How many colors?
    lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;
    // XOR bits follow the header and color table
    lpImage->lpXOR = (PBYTE)FindDIBBits((LPSTR)lpImage->lpbi);
    // AND bits follow the XOR bits
    lpImage->lpAND = lpImage->lpXOR + (lpImage->Height*BytesPerLine((LPBITMAPINFOHEADER)(lpImage->lpbi)));

    return TRUE;
}

LPSTR CIconExtractor::FindDIBBits(LPSTR lpbi)
{
   return (lpbi + *(LPDWORD)lpbi + PaletteSize(lpbi));
}

WORD CIconExtractor::PaletteSize(LPSTR lpbi)
{
    return (DIBNumColors(lpbi) * sizeof(RGBQUAD));
}

DWORD CIconExtractor::BytesPerLine(LPBITMAPINFOHEADER lpBMIH) const
{
    return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}

WORD CIconExtractor::DIBNumColors(LPSTR lpbi) const
{
    WORD wBitCount;
    DWORD dwClrUsed;

    dwClrUsed = ((LPBITMAPINFOHEADER) lpbi)->biClrUsed;

    if (dwClrUsed)
        return (WORD) dwClrUsed;

    wBitCount = ((LPBITMAPINFOHEADER) lpbi)->biBitCount;

    switch (wBitCount)
    {
        case 1: return 2;
        case 4: return 16;
        case 8: return 256;
        default:return 0;
    }
    //return 0;
}

