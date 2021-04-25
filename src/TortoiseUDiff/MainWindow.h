// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009-2013, 2020-2021 - TortoiseSVN
// Copyright (C) 2020 - TortoiseGit

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
#include "Scintilla.h"
#include "FindBar.h"
#include <string>

/**
 * \ingroup TortoiseUDiff
 * Main window of TortoiseUDiff. Handles the child windows (Scintilla control,
 * CFindBar, ...).
 */
class CMainWindow : public CWindow
{
public:
    CMainWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = nullptr);
    ~CMainWindow() override;

    /**
    * Registers the window class and creates the window.
    */
    bool RegisterAndCreateWindow();

    LRESULT      SendEditor(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const;
    HWND         GetHWNDEdit() const { return m_hWndEdit; }
    bool         LoadFile(LPCTSTR filename);
    bool         LoadFile(HANDLE hFile) const;
    bool         SaveFile(LPCTSTR filename) const;
    void         SetTitle(LPCTSTR title);
    std::wstring GetAppDirectory() const;
    void         RunCommand(const std::wstring& command) const;

protected:
    /// the message handler for this window
    LRESULT CALLBACK WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    /// Handles all the WM_COMMAND window messages (e.g. menu commands)
    LRESULT DoCommand(int id);

    bool Initialize();
    void SetTheme(bool bDark) const;

private:
    void        SetAStyle(int style, COLORREF fore, COLORREF back = ::GetSysColor(COLOR_WINDOW), int size = -1, const char* face = nullptr) const;
    static bool IsUTF8(LPVOID pBuffer, size_t cb);
    void        InitEditor() const;
    void        SetupWindow(bool bUTF8) const;
    void        UpdateLineCount() const;

private:
    LRESULT m_directFunction;
    LRESULT m_directPointer;
    int     m_themeCallbackId;
    HWND    m_hWndEdit;

    CFindBar     m_findBar;
    bool         m_bShowFindBar;
    bool         m_bMatchCase;
    std::wstring m_findText;
    std::wstring m_fileName;

    void loadOrSaveFile(bool doLoad, const std::wstring& filename = L"");
};