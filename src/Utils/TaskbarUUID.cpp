// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2011-2014, 2016-2017, 2019-2021 - TortoiseSVN

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
#include "TaskbarUUID.h"
#include "registry.h"
#include "CmdLineParser.h"

#include <Shobjidl.h>
#include "SmartHandle.h"
#include <atlbase.h>

#pragma warning(push)
#pragma warning(disable : 4458)
#include <GdiPlus.h>
#pragma warning(pop)
#pragma comment(lib, "gdiplus.lib")

#define APPID (L"TSVN.TSVN.1")

void setTaskIDPerUuid()
{
    using SetCurrentProcessExplicitAppUserModelIdfn                  = HRESULT STDAPICALLTYPE(PCWSTR appID);
    CAutoLibrary                                              hShell = AtlLoadSystemLibraryUsingFullPath(L"shell32.dll");
    if (hShell)
    {
        SetCurrentProcessExplicitAppUserModelIdfn *pfnSetCurrentProcessExplicitAppUserModelID = reinterpret_cast<SetCurrentProcessExplicitAppUserModelIdfn *>(GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID"));
        if (pfnSetCurrentProcessExplicitAppUserModelID)
        {
            std::wstring id = getTaskIDPerUuid();
            pfnSetCurrentProcessExplicitAppUserModelID(id.c_str());
        }
    }
}

std::wstring getTaskIDPerUuid(LPCWSTR uuid /*= NULL */)
{
    CRegStdDWORD r  = CRegStdDWORD(L"Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo", 3);
    std::wstring id = APPID;
    if ((r < 2) || (r == 3))
    {
        wchar_t buf[MAX_PATH] = {0};
        GetModuleFileName(nullptr, buf, MAX_PATH);
        std::wstring n = buf;
        n              = n.substr(n.find_last_of('\\') + 1);
        n              = n.substr(0, n.find_last_of('.'));
        n              = L"." + n;
        id += n;
    }

    if (r)
    {
        if (uuid)
        {
            id += uuid;
        }
        else
        {
            CCmdLineParser parser(GetCommandLine());
            if (parser.HasVal(L"groupuuid"))
            {
                id += parser.GetVal(L"groupuuid");
            }
        }
    }
    return id;
}

#ifdef _MFC_VER
extern CString g_sGroupingUuid;
#endif

void setUuidOverlayIcon(HWND hWnd)
{
    if (!CRegStdDWORD(L"Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo", 3))
        return;

    if (!CRegStdDWORD(L"Software\\TortoiseSVN\\GroupTaskbarIconsPerRepoOverlay", TRUE))
        return;

    std::wstring uuid;
#ifdef _MFC_VER
    uuid = g_sGroupingUuid;
#else
    CCmdLineParser parser(GetCommandLine());
    if (parser.HasVal(L"groupuuid"))
        uuid = parser.GetVal(L"groupuuid");
#endif
    if (uuid.empty())
        return;

    CComPtr<ITaskbarList3> pTaskbarInterface;
    if (FAILED(pTaskbarInterface.CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER)))
        return;

    int foundUuidIndex = 0;
    do
    {
        wchar_t buf[MAX_PATH] = {0};
        swprintf_s(buf, _countof(buf), L"%s%d", L"Software\\TortoiseSVN\\LastUsedUUIDsForGrouping\\", foundUuidIndex);
        CRegStdString r  = CRegStdString(buf);
        std::wstring  sr = r;
        if (sr.empty() || (sr.compare(uuid) == 0))
        {
            r = uuid;
            break;
        }
        foundUuidIndex++;
    } while (foundUuidIndex < 20);
    if (foundUuidIndex >= 20)
    {
        CRegStdString r = CRegStdString(L"Software\\TortoiseSVN\\LastUsedUUIDsForGrouping\\1");
        r.removeKey();
    }
    auto iconWidth  = GetSystemMetrics(SM_CXSMICON);
    auto iconHeight = GetSystemMetrics(SM_CYSMICON);

    DWORD colors[6] = {0x80FF0000, 0xB0FFFF00, 0x8000FF00, 0x800000FF, 0x80000000, 0xB000FFFF};

    ICONINFO iconInfo;
    auto     hScreenDC = GetDC(nullptr);
    auto     hdc       = CreateCompatibleDC(hScreenDC);

    iconInfo.hbmColor = CreateCompatibleBitmap(hScreenDC, iconWidth, iconHeight);
    iconInfo.hbmMask  = CreateCompatibleBitmap(hdc, iconWidth, iconHeight);
    iconInfo.fIcon    = TRUE;
    ReleaseDC(nullptr, hScreenDC);

    Gdiplus::SolidBrush black(Gdiplus::Color(0));
    // draw the icon mask, to get correct transparency
    auto hOldBm = SelectObject(hdc, iconInfo.hbmMask);
    PatBlt(hdc, 0, 0, iconWidth, iconHeight, WHITENESS);
    {
        Gdiplus::Graphics gmask(hdc);
        switch ((foundUuidIndex / 6) % 4)
        {
            case 0:
                gmask.FillEllipse(&black, 0, 0, iconWidth, iconHeight);
                break;
            case 1:
            {
                Gdiplus::Point pts[4];
                pts[0] = {iconWidth / 2, 0};
                pts[1] = {0, iconHeight / 2};
                pts[2] = {iconWidth / 2, iconHeight};
                pts[3] = {iconWidth, iconHeight / 2};
                gmask.FillPolygon(&black, pts, 4);
            }
            break;
            case 2:
            {
                Gdiplus::Point pts[3];
                pts[0] = {iconWidth / 2, 0};
                pts[1] = {0, iconHeight};
                pts[2] = {iconWidth, iconHeight};
                gmask.FillPolygon(&black, pts, 3);
            }
            break;
            case 3:
            {
                Gdiplus::Point pts[3];
                pts[0] = {0, 0};
                pts[1] = {iconWidth, 0};
                pts[2] = {iconWidth / 2, iconHeight};
                gmask.FillPolygon(&black, pts, 3);
            }
            break;
        }
    }

    // draw the icon itself
    SelectObject(hdc, iconInfo.hbmColor);
    {
        Gdiplus::Graphics g(hdc);
        g.FillRectangle(&black, 0, 0, iconWidth, iconHeight);
        Gdiplus::SolidBrush brush(Gdiplus::Color(static_cast<Gdiplus::ARGB>(colors[foundUuidIndex % 6])));
        switch ((foundUuidIndex / 6) % 4)
        {
            case 0:
                g.FillEllipse(&brush, 0, 0, iconWidth, iconHeight);
                break;
            case 1:
            {
                Gdiplus::Point pts[4];
                pts[0] = {iconWidth / 2, 0};
                pts[1] = {0, iconHeight / 2};
                pts[2] = {iconWidth / 2, iconHeight};
                pts[3] = {iconWidth, iconHeight / 2};
                g.FillPolygon(&brush, pts, 4);
            }
            break;
            case 2:
            {
                Gdiplus::Point pts[3];
                pts[0] = {iconWidth / 2, 0};
                pts[1] = {0, iconHeight};
                pts[2] = {iconWidth, iconHeight};
                g.FillPolygon(&brush, pts, 3);
            }
            break;
            case 3:
            {
                Gdiplus::Point pts[3];
                pts[0] = {0, 0};
                pts[1] = {iconWidth, 0};
                pts[2] = {iconWidth / 2, iconHeight};
                g.FillPolygon(&brush, pts, 3);
            }
            break;
        }
    }

    SelectObject(hdc, hOldBm);
    DeleteDC(hdc);

    CAutoIcon hIcon = CreateIconIndirect(&iconInfo);

    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);

    pTaskbarInterface->SetOverlayIcon(hWnd, hIcon, uuid.c_str());
}
