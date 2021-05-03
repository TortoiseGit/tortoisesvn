// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2012, 2014-2015, 2021 - TortoiseSVN
// Copyright (C) 2018 - Sven Strickroth <email@cs-ware.de>

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

#define WIDTHBYTES(bits) ((((bits) + 31) >> 5) << 2)

CIconExtractor::CIconExtractor()
{
}

DWORD CIconExtractor::ExtractIcon(HINSTANCE hResource, LPCWSTR id, LPCWSTR targetIcon)
{
    LPICONRESOURCE lpIr    = nullptr;
    HRSRC          hRsrc   = nullptr;
    HGLOBAL        hGlobal = nullptr;
    LPMEMICONDIR   lpIcon  = nullptr;

    // Find the group icon resource
    hRsrc = FindResource(hResource, id, RT_GROUP_ICON);

    if (hRsrc == nullptr)
        return GetLastError();

    if ((hGlobal = LoadResource(hResource, hRsrc)) == nullptr)
        return GetLastError();

    if ((lpIcon = static_cast<LPMEMICONDIR>(LockResource(hGlobal))) == nullptr)
        return GetLastError();

    if ((lpIr = static_cast<LPICONRESOURCE>(malloc(sizeof(ICONRESOURCE) + ((lpIcon->idCount - 1) * sizeof(ICONIMAGE))))) == nullptr)
        return GetLastError();
    SecureZeroMemory(lpIr, sizeof(ICONRESOURCE) + ((lpIcon->idCount - 1) * sizeof(ICONIMAGE)));
    lpIr->nNumImages = lpIcon->idCount;
    OnOutOfScope(
        for (UINT i = 0; i < lpIr->nNumImages; ++i)
            free(lpIr->IconImages[i].lpBits);
        free(lpIr););

    // Go through all the icons
    for (UINT i = 0; i < lpIr->nNumImages; ++i)
    {
        // Get the individual icon
        if ((hRsrc = FindResource(hResource, MAKEINTRESOURCE(lpIcon->idEntries[i].nID), RT_ICON)) == nullptr)
            return GetLastError();

        if ((hGlobal = LoadResource(hResource, hRsrc)) == nullptr)
            return GetLastError();

        // Store a copy of the resource locally
        lpIr->IconImages[i].dwNumBytes = SizeofResource(hResource, hRsrc);
        lpIr->IconImages[i].lpBits     = static_cast<LPBYTE>(malloc(lpIr->IconImages[i].dwNumBytes));
        if (lpIr->IconImages[i].lpBits == nullptr)
            return GetLastError();

        memcpy(lpIr->IconImages[i].lpBits, LockResource(hGlobal), lpIr->IconImages[i].dwNumBytes);

        // Adjust internal pointers
        if (!AdjustIconImagePointers(&(lpIr->IconImages[i])))
            return GetLastError();
    }

    return WriteIconToICOFile(lpIr, targetIcon);
}

DWORD CIconExtractor::WriteIconToICOFile(LPICONRESOURCE lpIr, LPCWSTR szFileName)
{
    DWORD dwBytesWritten = 0;

    CAutoFile hFile = CreateFile(szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    // open the file
    if (!hFile)
        return GetLastError();

    // Write the header
    if (WriteICOHeader(hFile, lpIr->nNumImages))
        return GetLastError();

    // Write the ICONDIRENTRY's
    for (UINT i = 0; i < lpIr->nNumImages; ++i)
    {
        ICONDIRENTRY ide;

        // Convert internal format to ICONDIRENTRY
        ide.bWidth  = static_cast<BYTE>(lpIr->IconImages[i].Width);
        ide.bHeight = static_cast<BYTE>(lpIr->IconImages[i].Height);
        if (ide.bHeight == 0) // 256x256 icon, both width and height must be 0
            ide.bWidth = 0;
        ide.bReserved = 0;
        ide.wPlanes   = lpIr->IconImages[i].lpbi->bmiHeader.biPlanes;
        ide.wBitCount = lpIr->IconImages[i].lpbi->bmiHeader.biBitCount;

        if ((ide.wPlanes * ide.wBitCount) >= 8)
            ide.bColorCount = 0;
        else
            ide.bColorCount = 1 << (ide.wPlanes * ide.wBitCount);
        ide.dwBytesInRes  = lpIr->IconImages[i].dwNumBytes;
        ide.dwImageOffset = CalculateImageOffset(lpIr, i);

        // Write the ICONDIRENTRY to disk
        if (!WriteFile(hFile, &ide, sizeof(ICONDIRENTRY), &dwBytesWritten, nullptr))
            return GetLastError();

        if (dwBytesWritten != sizeof(ICONDIRENTRY))
            return GetLastError();
    }

    // Write the image bits for each image
    for (UINT i = 0; i < lpIr->nNumImages; ++i)
    {
        DWORD dwTemp = lpIr->IconImages[i].lpbi->bmiHeader.biSizeImage;
        bool  bError = false; // fix size even on error

        // Set the sizeimage member to zero, but not if the icon is PNG
        if (lpIr->IconImages[i].lpbi->bmiHeader.biCompression != 65536)
            lpIr->IconImages[i].lpbi->bmiHeader.biSizeImage = 0;
        if (!WriteFile(hFile, lpIr->IconImages[i].lpBits, lpIr->IconImages[i].dwNumBytes, &dwBytesWritten, nullptr))
            bError = true;

        if (dwBytesWritten != lpIr->IconImages[i].dwNumBytes)
            bError = true;

        // set it back
        lpIr->IconImages[i].lpbi->bmiHeader.biSizeImage = dwTemp;
        if (bError)
            return GetLastError();
    }
    return NO_ERROR;
}

DWORD CIconExtractor::CalculateImageOffset(LPICONRESOURCE lpIr, UINT nIndex)
{
    // Calculate the ICO header size
    DWORD dwSize = 3 * sizeof(WORD);
    // Add the ICONDIRENTRY's
    dwSize += lpIr->nNumImages * sizeof(ICONDIRENTRY);
    // Add the sizes of the previous images
    for (UINT i = 0; i < nIndex; ++i)
        dwSize += lpIr->IconImages[i].dwNumBytes;

    return dwSize;
}

DWORD CIconExtractor::WriteICOHeader(HANDLE hFile, UINT nNumEntries)
{
    WORD  output         = 0;
    DWORD dwBytesWritten = 0;

    // Write 'reserved' WORD
    if (!WriteFile(hFile, &output, sizeof(WORD), &dwBytesWritten, nullptr))
        return GetLastError();
    // Did we write a WORD?
    if (dwBytesWritten != sizeof(WORD))
        return GetLastError();
    // Write 'type' WORD (1)
    output = 1;
    if (!WriteFile(hFile, &output, sizeof(WORD), &dwBytesWritten, nullptr))
        return GetLastError();
    // Did we write a WORD?
    if (dwBytesWritten != sizeof(WORD))
        return GetLastError();
    // Write Number of Entries
    output = static_cast<WORD>(nNumEntries);
    if (!WriteFile(hFile, &output, sizeof(WORD), &dwBytesWritten, nullptr))
        return GetLastError();
    // Did we write a WORD?
    if (dwBytesWritten != sizeof(WORD))
        return GetLastError();

    return NO_ERROR;
}

BOOL CIconExtractor::AdjustIconImagePointers(LPICONIMAGE lpImage)
{
    if (lpImage == nullptr)
        return FALSE;

    // BITMAPINFO is at beginning of bits
    lpImage->lpbi = reinterpret_cast<LPBITMAPINFO>(lpImage->lpBits);
    // Width - simple enough
    lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
    // Icons are stored in funky format where height is doubled - account for it
    lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight) / 2;
    // How many colors?
    lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;
    // XOR bits follow the header and color table
    lpImage->lpXOR = reinterpret_cast<PBYTE>(FindDIBBits(reinterpret_cast<LPSTR>(lpImage->lpbi)));
    // AND bits follow the XOR bits
    lpImage->lpAND = lpImage->lpXOR + (lpImage->Height * BytesPerLine(reinterpret_cast<LPBITMAPINFOHEADER>(lpImage->lpbi)));

    return TRUE;
}

LPSTR CIconExtractor::FindDIBBits(LPSTR lpbi)
{
    return (lpbi + *reinterpret_cast<LPDWORD>(lpbi) + PaletteSize(lpbi));
}

WORD CIconExtractor::PaletteSize(LPSTR lpbi)
{
    return (DIBNumColors(lpbi) * sizeof(RGBQUAD));
}

DWORD CIconExtractor::BytesPerLine(LPBITMAPINFOHEADER lpBmih)
{
    return WIDTHBYTES(lpBmih->biWidth * lpBmih->biPlanes * lpBmih->biBitCount);
}

WORD CIconExtractor::DIBNumColors(LPSTR lpbi)
{
    DWORD dwClrUsed = reinterpret_cast<LPBITMAPINFOHEADER>(lpbi)->biClrUsed;

    if (dwClrUsed)
        return static_cast<WORD>(dwClrUsed);

    WORD wBitCount = reinterpret_cast<LPBITMAPINFOHEADER>(lpbi)->biBitCount;

    switch (wBitCount)
    {
        case 1:
            return 2;
        case 4:
            return 16;
        case 8:
            return 256;
        default:
            return 0;
    }
    //return 0;
}
