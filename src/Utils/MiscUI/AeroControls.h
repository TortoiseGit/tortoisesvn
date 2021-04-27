// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2014-2016, 2020-2021 - TortoiseSVN

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
#include "registry.h"
#include <map>
#pragma warning(push)
#pragma warning(disable : 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>
#pragma warning(pop)

class AeroControlBase
{
public:
    AeroControlBase();
    virtual ~AeroControlBase();

    bool SubclassControl(HWND hControl);
    bool SubclassControl(CWnd* parent, int controlId);
    void SubclassOkCancel(CWnd* parent);
    void SubclassOkCancelHelp(CWnd* parent);

    bool AeroDialogsEnabled();

private:
    static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uidSubclass, DWORD_PTR dwRefData);
    LRESULT                 StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                 ButtonWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                 ProgressbarWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static BOOL DetermineGlowSize(int* piSize, LPCWSTR pszClassIdList = NULL);
    static void DrawFocusRect(LPRECT prcFocus, HDC hdcPaint);
    static void DrawRect(LPRECT prc, HDC hdcPaint, Gdiplus::DashStyle dashStyle, Gdiplus::Color clr, Gdiplus::REAL width);
    static void FillRect(LPRECT prc, HDC hdcPaint, Gdiplus::Color clr);
    static int  GetStateFromBtnState(LONG_PTR dwStyle, BOOL bHot, BOOL bFocus, LRESULT dwCheckState, int iPartId, BOOL bHasMouseCapture);
    static void PaintControl(HWND hWnd, HDC hdc, RECT* prc, bool bDrawBorder);
    static void ScreenToClient(HWND hWnd, LPRECT lprc);
    static void DrawSolidWndRectOnParent(HWND hWnd, Gdiplus::Color clr);
    static void DrawEditBorder(HWND hWnd);
    static BOOL GetEditBorderColor(HWND hWnd, COLORREF* pClr);
    static void GetRoundRectPath(Gdiplus::GraphicsPath* pPath, const Gdiplus::Rect& r, int dia);

    CRegDWORD                m_regEnableDwmFrame;
    std::map<HWND, UINT_PTR> m_subclassedControls;
    ULONG_PTR                m_gdiplusToken;
};
