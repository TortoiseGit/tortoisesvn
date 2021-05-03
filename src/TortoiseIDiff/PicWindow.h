// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2010, 2012-2016, 2020-2021 - TortoiseSVN

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
#include <CommCtrl.h>
#include "BaseWindow.h"
#include "Picture.h"
#include "NiceTrackbar.h"

#define HEADER_HEIGHT 30

#define ID_ANIMATIONTIMER   100
#define TIMER_ALPHASLIDER   101
#define ID_ALPHATOGGLETIMER 102

#define LEFTBUTTON_ID        101
#define RIGHTBUTTON_ID       102
#define PLAYBUTTON_ID        103
#define ALPHATOGGLEBUTTON_ID 104
#define BLENDALPHA_ID        105
#define BLENDXOR_ID          106
#define SELECTBUTTON_ID      107

#define TRACKBAR_ID   101
#define SLIDER_HEIGHT 30
#define SLIDER_WIDTH  30

#ifndef GET_X_LPARAM
#    define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#    define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

/**
 * \ingroup TortoiseIDiff
 * The image view window.
 * Shows an image and provides methods to scale the image or alpha blend it
 * over another image.
 */
class CPicWindow : public CWindow
{
private:
    CPicWindow() = delete;

public:
    CPicWindow(HINSTANCE hInstance, const WNDCLASSEX* wcx = nullptr)
        : CWindow(hInstance, wcx)
        , m_bValid(false)
        , m_picScale(100)
        , m_transparentColor(::GetSysColor(COLOR_WINDOW))
        , m_bFirstpaint(false)
        , m_pSecondPic(nullptr)
        , m_pTheOtherPic(nullptr)
        , m_bMainPic(false)
        , m_bLinkedPositions(true)
        , m_bFitWidths(false)
        , m_bFitHeights(false)
        , m_bOverlap(false)
        , m_bDragging(false)
        , m_blend(BLEND_ALPHA)
        , m_blendAlpha(0.5f)
        , m_bShowInfo(false)
        , m_hwndTT(nullptr)
        , m_hwndTrack(nullptr)
        , m_bSelectionMode(false)
        , m_themeCallbackId(0)
        , m_nVScrollPos(0)
        , m_nHScrollPos(0)
        , m_nVSecondScrollPos(0)
        , m_nHSecondScrollPos(0)
        , m_startVScrollPos(0)
        , m_startHScrollPos(0)
        , m_startVSecondScrollPos(0)
        , m_startHSecondScrollPos(0)
        , m_nDimensions(0)
        , m_nCurrentDimension(1)
        , m_nFrames(0)
        , m_nCurrentFrame(1)
        , m_hwndLeftBtn(nullptr)
        , m_hwndRightBtn(nullptr)
        , m_hwndPlayBtn(nullptr)
        , m_hwndSelectBtn(nullptr)
        , m_hwndAlphaToggleBtn(nullptr)
        , m_hLeft(nullptr)
        , m_hRight(nullptr)
        , m_hPlay(nullptr)
        , m_hStop(nullptr)
        , m_hAlphaToggle(nullptr)
        , m_bPlaying(false)
        , m_linkedWidth(0)
        , m_linkedHeight(0)
    {
        SetWindowTitle(L"Picture Window");
        m_lastTTPos.x     = 0;
        m_lastTTPos.y     = 0;
        m_wszTip[0]       = 0;
        m_szTip[0]        = 0;
        m_ptPanStart.x    = -1;
        m_ptPanStart.y    = -1;
        m_infoRect.left   = 0;
        m_infoRect.top    = 0;
        m_infoRect.right  = 0;
        m_infoRect.bottom = 0;
    };

    enum BlendType
    {
        BLEND_ALPHA,
        BLEND_XOR,
    };
    /// Registers the window class and creates the window
    bool RegisterAndCreateWindow(HWND hParent);

    /// Sets the image path and title to show
    void SetPic(const std::wstring& path, const std::wstring& title, bool bFirst);
    /// Returns the CPicture image object. Used to get an already loaded image
    /// object without having to load it again.
    CPicture* GetPic() { return &m_picture; }
    /// Sets the path and title of the second image which is alpha blended over the original
    void SetSecondPic(CPicture* pPicture = nullptr, const std::wstring& sectit = L"", const std::wstring& secpath = L"", int hpos = 0, int vpos = 0)
    {
        m_pSecondPic        = pPicture;
        m_picTitle2         = sectit;
        m_picPath2          = secpath;
        m_nVSecondScrollPos = vpos;
        m_nHSecondScrollPos = hpos;
    }

    void StopTimer() const { KillTimer(*this, ID_ANIMATIONTIMER); }

    /// Returns the currently used alpha blending value (0.0-1.0)
    float GetBlendAlpha() const { return m_blendAlpha; }
    /// Sets the alpha blending value
    void SetBlendAlpha(BlendType type, float a)
    {
        m_blend      = type;
        m_blendAlpha = a;
        if (m_alphaSlider.IsValid())
            SendMessage(m_alphaSlider.GetWindow(), TBM_SETPOS, static_cast<WPARAM>(1), static_cast<LPARAM>(a * 16.0f));
        PositionTrackBar();
        InvalidateRect(*this, nullptr, FALSE);
    }
    /// Toggle the alpha blending value
    void ToggleAlpha()
    {
        if (0.0f != GetBlendAlpha())
            SetBlendAlpha(m_blend, 0.0f);
        else
            SetBlendAlpha(m_blend, 1.0f);
    }

    /// Set the color that this PicWindow will display behind transparent images.
    void SetTransparentColor(COLORREF back)
    {
        m_transparentColor = back;
        InvalidateRect(*this, nullptr, false);
    }

    /// Resizes the image to fit into the window. Small images are not enlarged.
    void FitImageInWindow();
    /// center the image in the view
    void CenterImage();
    /// forces the widths of the images to be the same
    void FitWidths(bool bFit);
    /// forces the heights of the images to be the same
    void FitHeights(bool bFit);
    /// Sets the zoom factor of the image
    void SetZoom(int zoom, bool centerMouse, bool inZoom = false);
    /// Returns the currently used zoom factor in which the image is shown.
    int GetZoom() const { return m_picScale; }
    /// Zooms in (true) or out (false) in nice steps
    void Zoom(bool in, bool centermouse);
    /// Sets the 'Other' pic window
    void SetOtherPicWindow(CPicWindow* pWnd) { m_pTheOtherPic = pWnd; }
    /// Links/Unlinks the two pic windows
    void LinkPositions(bool bLink) { m_bLinkedPositions = bLink; }
    /// Sets the overlay mode info
    void SetOverlapMode(bool b) { m_bOverlap = b; }

    void ShowInfo(bool bShow = true)
    {
        m_bShowInfo = bShow;
        InvalidateRect(*this, nullptr, false);
    }
    /// Sets up the scrollbars as needed
    void SetupScrollBars() const;

    bool HasMultipleImages() const;

    int  GetHPos() const { return m_nHScrollPos; }
    int  GetVPos() const { return m_nVScrollPos; }
    void SetZoomValue(int z)
    {
        m_picScale = z;
        InvalidateRect(*this, nullptr, FALSE);
    }

    void SetSelectionMode(bool bSelect = true) { m_bSelectionMode = bSelect; }
    /// Handles the mouse wheel
    void OnMouseWheel(short fwKeys, short zDelta);

protected:
    /// the message handler for this window
    LRESULT CALLBACK WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    /// Draws the view title bar
    void DrawViewTitle(HDC hDC, RECT* rect);
    /// Creates the image buttons
    bool CreateButtons();
    /// Handles vertical scrolling
    void OnVScroll(UINT nSbCode, UINT nPos);
    /// Handles horizontal scrolling
    void OnHScroll(UINT nSbCode, UINT nPos);
    /// Returns the client rectangle, without the scrollbars and the view title.
    /// Basically the rectangle the image can use.
    void GetClientRect(RECT* pRect) const;
    /// Returns the client rectangle, without the view title but with the scrollbars
    void GetClientRectWithScrollbars(RECT* pRect) const;
    /// the WM_PAINT function
    void Paint(HWND hwnd);
    /// Draw pic to hdc, with a border, scaled by scale.
    void ShowPicWithBorder(HDC hdc, const RECT& bounds, CPicture& pic, int scale);
    /// Positions the buttons
    void PositionChildren() const;
    /// advance to the next image in the file
    void NextImage();
    /// go back to the previous image in the file
    void PrevImage();
    /// starts/stops the animation
    void Animate(bool bStart) const;
    /// Creates the trackbar (the alpha blending slider control)
    void CreateTrackbar(HWND hwndParent);
    /// Moves the alpha slider trackbar to the correct position
    void PositionTrackBar() const;
    /// creates the info string used in the info box and the tooltips
    void BuildInfoString(wchar_t* buf, int size, bool bTooltip) const;
    /// adjusts the zoom to fit the specified width
    void SetZoomToWidth(long width);
    /// adjusts the zoom to fit the specified height
    void SetZoomToHeight(long height);
    /// sets the dark mode
    void SetTheme(bool bDark) const;
    /// returns the transparent color, adjusted for theme
    COLORREF GetTransparentThemedColor() const;

    std::wstring m_picPath;          ///< the path to the image we show
    std::wstring m_picTitle;         ///< the string to show in the image view as a title
    CPicture     m_picture;          ///< the picture object of the image
    bool         m_bValid;           ///< true if the picture object is valid, i.e. if the image could be loaded and can be shown
    int          m_picScale;         ///< the scale factor of the image in percent
    COLORREF     m_transparentColor; ///< the color to draw under the images
    bool         m_bFirstpaint;      ///< true if the image is painted the first time. Used to initialize some stuff when the window is valid for sure.
    CPicture*    m_pSecondPic;       ///< if set, this is the picture to draw transparently above the original
    CPicWindow*  m_pTheOtherPic;     ///< pointer to the other picture window. Used for "linking" the two windows when scrolling/zooming/...
    bool         m_bMainPic;         ///< if true, this is the first image
    bool         m_bLinkedPositions; ///< if true, the two image windows are linked together for scrolling/zooming/...
    bool         m_bFitWidths;       ///< if true, the two image windows are shown with the same width
    bool         m_bFitHeights;      ///< if true, the two image windows are shown with the same height
    bool         m_bOverlap;         ///< true if the overlay mode is active
    bool         m_bDragging;        ///< indicates an ongoing dragging operation
    BlendType    m_blend;            ///< type of blending to use
    std::wstring m_picTitle2;        ///< the title of the second picture
    std::wstring m_picPath2;         ///< the path of the second picture
    float        m_blendAlpha;       ///<the alpha value for transparency blending
    bool         m_bShowInfo;        ///< true if the info rectangle of the image should be shown
    wchar_t      m_wszTip[8192];
    char         m_szTip[8192];
    POINT        m_lastTTPos;
    HWND         m_hwndTT;
    HWND         m_hwndTrack;
    bool         m_bSelectionMode; ///< true if TortoiseIDiff is in selection mode, used to resolve conflicts
    int          m_themeCallbackId;
    // scrollbar info
    int   m_nVScrollPos;           ///< vertical scroll position
    int   m_nHScrollPos;           ///< horizontal scroll position
    int   m_nVSecondScrollPos;     ///< vertical scroll position of second pic at the moment of enabling overlap mode
    int   m_nHSecondScrollPos;     ///< horizontal scroll position of second pic at the moment of enabling overlap mode
    POINT m_ptPanStart;            ///< the point of the last mouse click
    int   m_startVScrollPos;       ///< the vertical scroll position when panning starts
    int   m_startHScrollPos;       ///< the horizontal scroll position when panning starts
    int   m_startVSecondScrollPos; ///< the vertical scroll position of the second pic when panning starts
    int   m_startHSecondScrollPos; ///< the horizontal scroll position of the second pic when panning starts
    // image frames/dimensions
    UINT m_nDimensions;
    UINT m_nCurrentDimension;
    UINT m_nFrames;
    UINT m_nCurrentFrame;

    // controls
    HWND          m_hwndLeftBtn;
    HWND          m_hwndRightBtn;
    HWND          m_hwndPlayBtn;
    HWND          m_hwndSelectBtn;
    CNiceTrackbar m_alphaSlider;
    HWND          m_hwndAlphaToggleBtn;
    HICON         m_hLeft;
    HICON         m_hRight;
    HICON         m_hPlay;
    HICON         m_hStop;
    HICON         m_hAlphaToggle;
    bool          m_bPlaying;
    RECT          m_infoRect;

    // linked image sizes/positions
    long m_linkedWidth;
    long m_linkedHeight;
};
