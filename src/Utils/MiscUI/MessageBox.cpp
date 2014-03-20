// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseSVN

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
#include "MessageBox.h"
#include "SmartHandle.h"
#include "XMessageBox.h"


UINT TSVNMessageBox( HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, UINT uType)
{
    return XMessageBox(hWnd, lpMessage, lpCaption, uType);
}

UINT TSVNMessageBox( HWND hWnd, UINT nMessage, UINT nCaption, UINT uType)
{
    return TSVNMessageBox(hWnd, (LPCTSTR)CString(MAKEINTRESOURCE(nMessage)), (LPCTSTR)CString(MAKEINTRESOURCE(nCaption)), uType);
}

UINT TSVNMessageBox( HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, int nDef, LPCTSTR lpButton1, LPCTSTR lpButton2, LPCTSTR lpButton3 /*= NULL*/ )
{
    CString sButtonTexts;
    if (lpButton1)
        sButtonTexts = lpButton1;
    if (lpButton2)
    {
        sButtonTexts += L"\n";
        sButtonTexts += lpButton2;
    }
    if (lpButton3)
    {
        sButtonTexts += L"\n";
        sButtonTexts += lpButton3;
    }
    XMSGBOXPARAMS xmsg;
    wcscpy_s(xmsg.szCustomButtons, (LPCTSTR)sButtonTexts);

    return XMessageBox(hWnd, lpMessage, lpCaption, nDef, &xmsg);
}

