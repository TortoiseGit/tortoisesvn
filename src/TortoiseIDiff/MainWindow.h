// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2007, 2009, 2011-2013, 2015-2016, 2020-2021 - TortoiseSVN

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
#include "BaseWindow.h"
#include "PicWindow.h"
#include "ResString.h"
#include <map>
#include <CommCtrl.h>

#define SPLITTER_BORDER 2

#define WINDOW_MINHEIGHT 200
#define WINDOW_MINWIDTH  200

enum FileType
{
    FileTypeMine   = 1,
    FileTypeTheirs = 2,
    FileTypeBase   = 3,
};

/**
 * \ingroup TortoiseIDiff
 * The main window of TortoiseIDiff.
 * Hosts the two image views, the menu, toolbar, slider, ...
 */
class CMainWindow : public CWindow
{
public:
    CMainWindow(HINSTANCE hInstance, const WNDCLASSEX* wcx = nullptr);
    ;

    /**
     * Registers the window class and creates the window.
     */
    bool RegisterAndCreateWindow();

    /**
     * Sets the image path and title for the left image view.
     */
    static void SetLeft(const std::wstring& leftpath, const std::wstring& lefttitle)
    {
        m_sLeftPicPath  = leftpath;
        m_sLeftPicTitle = lefttitle;
    }
    /**
     * Sets the image path and the title for the right image view.
     */
    static void SetRight(const std::wstring& rightpath, const std::wstring& righttitle)
    {
        m_sRightPicPath  = rightpath;
        m_sRightPicTitle = righttitle;
    }

    /**
     * Sets the image path and title for selection mode. In selection mode, the images
     * are shown side-by-side for the user to chose one of them. The chosen image is
     * saved at the path for \b FileTypeResult (if that path has been set) and the
     * process return value is the chosen FileType.
     */
    void SetSelectionImage(FileType ft, const std::wstring& path, const std::wstring& title);
    void SetSelectionResult(const std::wstring& path) { m_selectionResult = path; }

protected:
    /// the message handler for this window
    LRESULT CALLBACK WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    /// Handles all the WM_COMMAND window messages (e.g. menu commands)
    LRESULT DoCommand(int id, LPARAM lParam);

    /// Positions the child windows. Call this after the window sizes/positions have changed.
    void PositionChildren(RECT* clientrect = nullptr);
    /// Shows the "Open images" dialog where the user can select the images to diff
    bool                 OpenDialog() const;
    static BOOL CALLBACK OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static bool          AskForFile(HWND owner, wchar_t* path);

    // splitter methods
    static void DrawXorBar(HDC hdc, int x1, int y1, int width, int height);
    LRESULT     Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    LRESULT     Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    LRESULT     Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    void        Splitter_CaptureChanged();

    void SetTheme(bool bDark);
    int  m_themeCallbackId;
    // toolbar
    bool       CreateToolbar();
    HWND       m_hwndTb;
    HIMAGELIST m_hToolbarImgList;

    // command line params
    static std::wstring m_sLeftPicPath;
    static std::wstring m_sLeftPicTitle;

    static std::wstring m_sRightPicPath;
    static std::wstring m_sRightPicTitle;

    // image data
    CPicWindow m_picWindow1;
    CPicWindow m_picWindow2;
    CPicWindow m_picWindow3;
    bool       m_bShowInfo;
    COLORREF   m_transparentColor;

    // splitter data
    int  m_oldX;
    int  m_oldY;
    bool m_bMoved;
    bool m_bDragMode;
    bool m_bDrag2;
    int  m_nSplitterPos;
    int  m_nSplitterPos2;

    // one/two pane view
    bool                  m_bSelectionMode;
    bool                  m_bOverlap;
    bool                  m_bVertical;
    bool                  m_bLinkedPositions;
    bool                  m_bFitWidths;
    bool                  m_bFitHeights;
    CPicWindow::BlendType m_blendType;

    // selection mode data
    std::map<FileType, std::wstring> m_selectionPaths;
    std::map<FileType, std::wstring> m_selectionTitles;
    std::wstring                     m_selectionResult;
};
