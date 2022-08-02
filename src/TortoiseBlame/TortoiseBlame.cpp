// TortoiseBlame - a Viewer for Subversion Blames

// Copyright (C) 2003-2022 - TortoiseSVN

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

#include "stdafx.h"
#include "CmdLineParser.h"
#include "TortoiseBlame.h"
#include "registry.h"
#include "LangDll.h"
#include "CreateProcessHelper.h"
#include "UnicodeUtils.h"
#include <ClipboardHelper.h>
#include "TaskbarUUID.h"
#include "BlameIndexColors.h"
#include "CrashReport.h"
#include "DebugOutput.h"
#include "DPIAware.h"
#include "LoadIconEx.h"
#include "Theme.h"
#include "DarkModeHelper.h"
#include "Monitor.h"
#include "resource.h"

#include <algorithm>
#include <regex>
#include <strsafe.h>
#include <dwmapi.h>

#pragma warning(push)
#pragma warning(disable : 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>
#pragma warning(pop)

#define MAX_LOADSTRING 1000

#define STYLE_MARK 11

#define COLORBYNONE    0
#define COLORBYAGE     1
#define COLORBYAGECONT 2
#define COLORBYAUTHOR  3

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Dwmapi.lib")

// Global Variables:
wchar_t      szTitle[MAX_LOADSTRING]       = {0}; // The title bar text
wchar_t      szWindowClass[MAX_LOADSTRING] = {0}; // the main window class name
std::wstring szViewtitle;
std::wstring szOrigPath;
std::wstring szPegRev;
wchar_t      searchStringNotFound[MAX_LOADSTRING] = {0};

#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant due to the following bools
bool showDate   = false;
bool showAuthor = true;
bool showLine   = true;
bool showPath   = false;

static TortoiseBlame app;
long                 TortoiseBlame::m_gotoLine = 0;
std::wstring         uuid;

COLORREF colorSet[MAX_BLAMECOLORS]{};

TortoiseBlame::TortoiseBlame()
    : hInstance(nullptr)
    , hResource(nullptr)
    , currentDialog(nullptr)
    , wMain(nullptr)
    , wEditor(nullptr)
    , wBlame(nullptr)
    , wHeader(nullptr)
    , wLocator(nullptr)
    , hwndTT(nullptr)
    , bIgnoreEOL(false)
    , bIgnoreSpaces(false)
    , bIgnoreAllSpaces(false)
    , m_themeCallbackId(0)
    , m_mouseRev(-2)
    , m_selectedRev(-1)
    , m_selectedOrigRev(-1)
    , m_lowestRev(LONG_MAX)
    , m_highestRev(0)
    , m_ttVisible(false)
    , m_font(nullptr)
    , m_italicFont(nullptr)
    , m_uiFont(nullptr)
    , m_blameWidth(0)
    , m_revWidth(0)
    , m_dateWidth(0)
    , m_authorWidth(0)
    , m_pathWidth(0)
    , m_lineWidth(0)
    , m_selectedLine(-1)
    , m_windowColor(CTheme::Instance().IsDarkTheme() ? CTheme::darkBkColor : GetSysColor(COLOR_WINDOW))
    , m_textColor(CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT))
    , m_textHighLightColor(GetSysColor(COLOR_HIGHLIGHTTEXT))
    , m_directFunction(0)
    , m_directPointer(0)
{
    m_szTip[0]      = 0;
    m_wszTip[0]     = 0;
    m_szFindWhat[0] = 0;
    colorSet[0]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor1", BLAMEINDEXCOLOR1);
    colorSet[1]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor2", BLAMEINDEXCOLOR2);
    colorSet[2]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor3", BLAMEINDEXCOLOR3);
    colorSet[3]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor4", BLAMEINDEXCOLOR4);
    colorSet[4]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor5", BLAMEINDEXCOLOR5);
    colorSet[5]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor6", BLAMEINDEXCOLOR6);
    colorSet[6]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor7", BLAMEINDEXCOLOR7);
    colorSet[7]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor8", BLAMEINDEXCOLOR8);
    colorSet[8]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor9", BLAMEINDEXCOLOR9);
    colorSet[9]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor10", BLAMEINDEXCOLOR10);
    colorSet[10]    = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor11", BLAMEINDEXCOLOR11);
    colorSet[11]    = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor12", BLAMEINDEXCOLOR12);
    m_regColorBy    = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameColorBy", COLORBYAGE);
    m_colorBy       = m_regColorBy;

    SetupColoring();
    SecureZeroMemory(&m_fr, sizeof(m_fr));
}

TortoiseBlame::~TortoiseBlame()
{
    if (m_font)
        DeleteObject(m_font);
    if (m_italicFont)
        DeleteObject(m_italicFont);
    if (m_uiFont)
        DeleteObject(m_uiFont);
}

std::wstring TortoiseBlame::GetAppDirectory()
{
    std::wstring path;
    DWORD        len       = 0;
    DWORD        bufferLen = MAX_PATH; // MAX_PATH is not the limit here!
    do
    {
        bufferLen += MAX_PATH; // MAX_PATH is not the limit here!
        auto pBuf = std::make_unique<wchar_t[]>(bufferLen);
        len       = GetModuleFileName(nullptr, pBuf.get(), bufferLen);
        path      = std::wstring(pBuf.get(), len);
    } while (len == bufferLen);
    path = path.substr(0, path.rfind('\\') + 1);

    return path;
}

// Return a color which is interpolated between c1 and c2.
// Slider controls the relative proportions as a percentage:
// Slider = 0   represents pure c1
// Slider = 50  represents equal mixture
// Slider = 100 represents pure c2
COLORREF TortoiseBlame::InterColor(COLORREF c1, COLORREF c2, int slider)
{
    int r, g, b;

    // Limit Slider to 0..100% range
    if (slider < 0)
        slider = 0;
    if (slider > 100)
        slider = 100;

    // The color components have to be treated individually.
    r = (GetRValue(c2) * slider + GetRValue(c1) * (100 - slider)) / 100;
    g = (GetGValue(c2) * slider + GetGValue(c1) * (100 - slider)) / 100;
    b = (GetBValue(c2) * slider + GetBValue(c1) * (100 - slider)) / 100;

    return RGB(r, g, b);
}

LRESULT TortoiseBlame::SendEditor(UINT msg, WPARAM wParam, LPARAM lParam) const
{
    if (m_directFunction)
    {
        return reinterpret_cast<SciFnDirect>(m_directFunction)(m_directPointer, msg, wParam, lParam);
    }
    return ::SendMessage(wEditor, msg, wParam, lParam);
}

void TortoiseBlame::SetTitle() const
{
#define MAX_PATH_LENGTH 80
    ASSERT(dialogname.GetLength() < MAX_PATH_LENGTH);
    WCHAR pathBuf[MAX_PATH] = {0};
    if (szViewtitle.size() >= MAX_PATH)
    {
        try
        {
            std::wstring str = szViewtitle;
            std::wregex  rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
            std::wstring replacement = L"$1$2...$3";
            std::wstring str2        = std::regex_replace(str, rx, replacement);
            if (str2.size() >= MAX_PATH)
                str2 = str2.substr(0, MAX_PATH - 2);
            PathCompactPathEx(pathBuf, str2.c_str(), MAX_PATH_LENGTH - static_cast<UINT>(wcslen(szTitle)), 0);
        }
        catch (std::exception&)
        {
            PathCompactPathEx(pathBuf, szViewtitle.c_str(), MAX_PATH_LENGTH - static_cast<UINT>(wcslen(szTitle)), 0);
        }
    }
    else
        PathCompactPathEx(pathBuf, szViewtitle.c_str(), MAX_PATH_LENGTH - static_cast<UINT>(wcslen(szTitle)), 0);
    std::wstring title;
    switch (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\DialogTitles", 0)))
    {
        case 0: // url/path - dialogname - appname
            title = pathBuf;
            title += L" - ";
            title += szTitle;
            title += L" - TortoiseSVN";
            break;
        case 1: // dialogname - url/path - appname
            title = szTitle;
            title += L" - ";
            title += pathBuf;
            title += L" - TortoiseSVN";
            break;
    }
    SetWindowText(wMain, title.c_str());
}

BOOL TortoiseBlame::OpenFile(const wchar_t* fileName)
{
    SendEditor(SCI_SETREADONLY, FALSE);
    SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);
    SendEditor(SCI_CLEARALL);
    SetTitle();
    SendEditor(SCI_SETSAVEPOINT);
    SendEditor(SCI_CANCEL);
    SendEditor(SCI_SETUNDOCOLLECTION, 0);
    SendEditor(EM_EMPTYUNDOBUFFER);
    ::ShowWindow(wEditor, SW_HIDE);

    int  stringBufLen = 4096;
    auto stringBuf    = std::make_unique<char[]>(stringBufLen);
    int  wideBufLen   = 4096;
    auto wideBuf      = std::make_unique<wchar_t[]>(wideBufLen);
    int  utf8BufLen   = 4096;
    auto utf8Buf      = std::make_unique<char[]>(utf8BufLen);

    FILE* file       = nullptr;
    int   retrycount = 10;
    while (retrycount)
    {
        _wfopen_s(&file, fileName, L"rbS");
        if (file == nullptr)
        {
            Sleep(500);
            retrycount--;
        }
        else
            break;
    }
    if (file == nullptr)
        return FALSE;

    ProfileTimer profiler(L"Blame: OpenFile");

    //m_revs.reserve(100000);
    //m_mergedRevs.reserve(100000);
    //m_dates.reserve(100000);
    //m_mergedDates.reserve(100000);
    //m_authors.reserve(100000);
    //m_mergedAuthors.reserve(100000);
    //m_mergedPaths.reserve(100000);

    m_lowestRev                 = LONG_MAX;
    m_highestRev                = 0;
    LONG         lineNumber     = 0;
    svn_revnum_t rev            = 0;
    svn_revnum_t mergedRev      = 0;
    int          strLen         = 0;
    bool         bHasMergePaths = false;
    for (;;)
    {
        rev       = 0;
        mergedRev = 0;
        // line number
        size_t len = fread(&lineNumber, sizeof(LONG), 1, file);
        if (len == 0)
            break;
        // revision
        len = fread(&rev, sizeof(svn_revnum_t), 1, file);
        if (len == 0)
            break;
        m_revs.push_back(rev);
        m_revSet.insert(rev);
        // author
        len = fread(&strLen, sizeof(int), 1, file);
        if (len == 0)
            break;
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len = fread(stringBuf.get(), sizeof(char), strLen, file);
            if (len == 0)
                break;
            stringBuf[strLen] = 0;
            auto author       = CUnicodeUtils::StdGetUnicode(stringBuf.get());
            m_authors.push_back(author);
            m_authorSet.insert(author);
        }
        else
        {
            m_authors.push_back(L"");
            m_authorSet.insert(L"");
        }
        // date
        len = fread(&strLen, sizeof(int), 1, file);
        if (len == 0)
            break;
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len = fread(stringBuf.get(), sizeof(char), strLen, file);
            if (len == 0)
                break;
            stringBuf[strLen] = 0;
            m_dates.push_back(CUnicodeUtils::StdGetUnicode(stringBuf.get()));
        }
        else
            m_dates.push_back(L"");
        // merged revision
        len = fread(&mergedRev, sizeof(svn_revnum_t), 1, file);
        if (len == 0)
            break;
        m_mergedRevs.push_back(mergedRev);
        if ((mergedRev > 0) && (mergedRev < rev))
        {
            m_lowestRev  = min(m_lowestRev, mergedRev);
            m_highestRev = max(m_highestRev, mergedRev);
        }
        else
        {
            m_lowestRev  = min(m_lowestRev, rev);
            m_highestRev = max(m_highestRev, rev);
        }
        // merged author
        len = fread(&strLen, sizeof(int), 1, file);
        if (len == 0)
            break;
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len = fread(stringBuf.get(), sizeof(char), strLen, file);
            if (len == 0)
                break;
            stringBuf[strLen] = 0;
            m_mergedAuthors.push_back(CUnicodeUtils::StdGetUnicode(stringBuf.get()));
        }
        else
            m_mergedAuthors.push_back(L"");
        // merged date
        len = fread(&strLen, sizeof(int), 1, file);
        if (len == 0)
            break;
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len = fread(stringBuf.get(), sizeof(char), strLen, file);
            if (len == 0)
                break;
            stringBuf[strLen] = 0;
            m_mergedDates.push_back(CUnicodeUtils::StdGetUnicode(stringBuf.get()));
        }
        else
            m_mergedDates.push_back(L"");
        // merged path
        len = fread(&strLen, sizeof(int), 1, file);
        if (len == 0)
            break;
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len = fread(stringBuf.get(), sizeof(char), strLen, file);
            if (len == 0)
                break;
            stringBuf[strLen] = 0;
            m_mergedPaths.push_back(CUnicodeUtils::StdGetUnicode(stringBuf.get()));
            bHasMergePaths = true;
        }
        else
            m_mergedPaths.push_back(L"");
        // text line
        len = fread(&strLen, sizeof(int), 1, file);
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len = fread(stringBuf.get(), sizeof(char), strLen, file);
            if (len == 0)
                break;
            stringBuf.get()[strLen] = 0;
            char* linePtr           = stringBuf.get();
            // in case we find an UTF8 BOM at the beginning of the line, we remove it
            if ((static_cast<unsigned char>(linePtr[0]) == 0xEF) && (static_cast<unsigned char>(linePtr[1]) == 0xBB) && (static_cast<unsigned char>(linePtr[2]) == 0xBF))
            {
                linePtr += 3;
                strLen -= 3;
            }
            if ((static_cast<unsigned char>(linePtr[0]) == 0xBB) && (static_cast<unsigned char>(linePtr[1]) == 0xEF) && (static_cast<unsigned char>(linePtr[2]) == 0xBF))
            {
                linePtr += 3;
                strLen -= 3;
            }
            // check each line for illegal utf8 sequences. If one is found, we treat
            // the file as ASCII, otherwise we assume an UTF8 file.
            unsigned char* utf8CheckBuf = reinterpret_cast<unsigned char*>(linePtr);
            bool           bUTF8        = false;
            while (*utf8CheckBuf)
            {
                if ((*utf8CheckBuf == 0xC0) || (*utf8CheckBuf == 0xC1) || (*utf8CheckBuf >= 0xF5))
                {
                    bUTF8 = false;
                    break;
                }
                else if ((*utf8CheckBuf & 0xE0) == 0xC0)
                {
                    bUTF8 = true;
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0) != 0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                }
                else if ((*utf8CheckBuf & 0xF0) == 0xE0)
                {
                    bUTF8 = true;
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0) != 0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0) != 0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                }
                else if ((*utf8CheckBuf & 0xF8) == 0xF0)
                {
                    bUTF8 = true;
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0) != 0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0) != 0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0) != 0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                }
                else if (*utf8CheckBuf >= 0x80)
                {
                    bUTF8 = false;
                    break;
                }

                utf8CheckBuf++;
            }
            if (!bUTF8)
            {
                if (strLen >= wideBufLen)
                {
                    wideBufLen = strLen * 2 + 1024;
                    wideBuf    = std::make_unique<wchar_t[]>(wideBufLen + 1LL);
                }

                int mbLen = MultiByteToWideChar(CP_ACP, 0, linePtr, strLen, wideBuf.get(), wideBufLen);
                if (mbLen)
                {
                    if (mbLen >= utf8BufLen)
                    {
                        utf8BufLen = mbLen + 1024;
                        utf8Buf    = std::make_unique<char[]>(utf8BufLen + 1LL);
                    }
                    mbLen = WideCharToMultiByte(CP_UTF8, 0, wideBuf.get(), mbLen, utf8Buf.get(), utf8BufLen, nullptr, nullptr);
                    if (mbLen)
                        SendEditor(SCI_APPENDTEXT, mbLen, reinterpret_cast<LPARAM>(utf8Buf.get()));
                    else
                        SendEditor(SCI_APPENDTEXT, strlen(linePtr), reinterpret_cast<LPARAM>(static_cast<char*>(linePtr)));
                }
                else
                    SendEditor(SCI_APPENDTEXT, strlen(linePtr), reinterpret_cast<LPARAM>(static_cast<char*>(linePtr)));
            }
            else
                SendEditor(SCI_APPENDTEXT, strlen(linePtr), reinterpret_cast<LPARAM>(static_cast<char*>(linePtr)));
        }
        if (len == 0)
            break;
        len = fread(&strLen, sizeof(int), 1, file);
        if (len == 0)
            break;
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len               = fread(stringBuf.get(), sizeof(char), strLen, file);
            stringBuf[strLen] = 0;
            if (rev)
            {
                auto foundIt = m_logMessages.find(rev);
                if (foundIt == m_logMessages.end())
                {
                    auto msg = CUnicodeUtils::StdGetUnicode(stringBuf.get());
                    if (msg.size() > MAX_LOG_LENGTH)
                    {
                        msg = msg.substr(0, MAX_LOG_LENGTH - 5);
                        msg = msg + L"\n...";
                    }
                    m_logMessages[rev] = msg;
                }
            }
        }
        len = fread(&strLen, sizeof(int), 1, file);
        if (len == 0)
            break;
        if (strLen)
        {
            if (strLen >= stringBufLen)
            {
                stringBufLen = strLen + 1024;
                stringBuf    = std::make_unique<char[]>(stringBufLen + 1LL);
            }
            len               = fread(stringBuf.get(), sizeof(char), strLen, file);
            stringBuf[strLen] = 0;
            if (mergedRev)
            {
                auto foundIt = m_logMessages.find(mergedRev);
                if (foundIt == m_logMessages.end())
                {
                    auto msg = CUnicodeUtils::StdGetUnicode(stringBuf.get());
                    if (msg.size() > MAX_LOG_LENGTH)
                    {
                        msg = msg.substr(0, MAX_LOG_LENGTH - 5);
                        msg = msg + L"\n...";
                    }
                    m_logMessages[mergedRev] = msg;
                }
            }
        }

        SendEditor(SCI_APPENDTEXT, 1, reinterpret_cast<LPARAM>("\n"));
    };

    fclose(file);

    SendEditor(SCI_SETUNDOCOLLECTION, 1);
    ::SetFocus(wEditor);
    SendEditor(EM_EMPTYUNDOBUFFER);
    SendEditor(SCI_SETSAVEPOINT);
    SendEditor(SCI_GOTOPOS, 0);
    SendEditor(SCI_SETSCROLLWIDTHTRACKING, TRUE);
    int numDigits = 0;
    int lineCount = static_cast<int>(m_authors.size());
    while (lineCount)
    {
        lineCount /= 10;
        numDigits++;
    }
    numDigits = max(numDigits, 3);
    {
        int pixelWidth = static_cast<int>(8 + numDigits * SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, reinterpret_cast<LPARAM>("8")));
        if (showLine)
            SendEditor(SCI_SETMARGINWIDTHN, 0, pixelWidth);
        else
            SendEditor(SCI_SETMARGINWIDTHN, 0);
    }
    SendEditor(SCI_SETREADONLY, TRUE);

    //check which lexer to use, depending on the filetype
    SetupLexer(fileName);
    ::ShowWindow(wEditor, SW_SHOW);
    ::InvalidateRect(wMain, nullptr, TRUE);
    RECT rc;
    GetWindowRect(wMain, &rc);
    SetWindowPos(wMain, nullptr, rc.left, rc.top, rc.right - rc.left - 1, rc.bottom - rc.top, 0);

    m_blameWidth = 0;
    SetupColoring();
    InitSize();

    if (!bHasMergePaths)
    {
        HMENU hMenu = GetMenu(wMain);
        EnableMenuItem(hMenu, ID_VIEW_MERGEPATH, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
    }

    if ((m_revs.size() != m_mergedRevs.size()) ||
        (m_mergedRevs.size() != m_dates.size()) ||
        (m_dates.size() != m_mergedDates.size()) ||
        (m_mergedDates.size() != m_authors.size()) ||
        (m_authors.size() != m_mergedAuthors.size()) ||
        (m_mergedAuthors.size() != m_mergedPaths.size()))
    {
        m_revs.clear();
        m_mergedRevs.clear();
        m_dates.clear();
        m_mergedDates.clear();
        m_authors.clear();
        m_mergedAuthors.clear();
        m_mergedPaths.clear();
        return FALSE;
    }

    return TRUE;
}

void TortoiseBlame::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char* face) const
{
    SendEditor(SCI_STYLESETFORE, style, CTheme::Instance().GetThemeColor(fore, true));
    SendEditor(SCI_STYLESETBACK, style, CTheme::Instance().GetThemeColor(back, true));
    if (size >= 1)
        SendEditor(SCI_STYLESETSIZE, style, size);
    if (face)
        SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

void TortoiseBlame::InitialiseEditor()
{
    m_directFunction = SendMessage(wEditor, SCI_GETDIRECTFUNCTION, 0, 0);
    m_directPointer  = SendMessage(wEditor, SCI_GETDIRECTPOINTER, 0, 0);
    CRegStdDWORD used2d(L"Software\\TortoiseSVN\\ScintillaDirect2D", TRUE);
    bool         enabled2d = false;
    if (static_cast<DWORD>(used2d))
        enabled2d = true;

    CreateFont(0);
    // Set up the global default style. These attributes are used wherever no explicit choices are made.
    std::wstring fontNameW = CRegStdString(L"Software\\TortoiseSVN\\BlameFontName", L"Consolas");
    std::string  fontName  = CUnicodeUtils::StdGetUTF8(fontNameW);
    SendEditor(SCI_STYLESETSIZE, STYLE_DEFAULT, static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\BlameFontSize", 10)));
    SendEditor(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>(fontName.c_str()));

    SendEditor(SCI_INDICSETSTYLE, STYLE_MARK, INDIC_ROUNDBOX);
    SendEditor(SCI_INDICSETFORE, STYLE_MARK, darkBlue);
    SendEditor(SCI_INDICSETUNDER, STYLE_MARK, TRUE);

    SendEditor(SCI_SETTABWIDTH, static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\BlameTabSize", 4)));
    SendEditor(SCI_SETREADONLY, TRUE);
    LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, reinterpret_cast<LPARAM>("_99999"));
    if (showLine)
        SendEditor(SCI_SETMARGINWIDTHN, 0, pix);
    else
        SendEditor(SCI_SETMARGINWIDTHN, 0);
    SendEditor(SCI_SETMARGINWIDTHN, 1);
    SendEditor(SCI_SETMARGINWIDTHN, 2);
    if (enabled2d)
    {
        SendEditor(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITERETAIN);
        SendEditor(SCI_SETBUFFEREDDRAW, 0);
    }
    m_regOldLinesColor        = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameOldColor", BLAMEOLDCOLOR);
    m_regNewLinesColor        = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameNewColor", BLAMENEWCOLOR);
    m_regLocatorOldLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameLocatorOldColor", BLAMEOLDCOLORBAR);
    m_regLocatorNewLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameLocatorNewColor", BLAMENEWCOLORBAR);

    m_regDarkOldLinesColor        = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameOldColorDark", DARKBLAMEOLDCOLOR);
    m_regDarkNewLinesColor        = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameNewColorDark", DARKBLAMENEWCOLOR);
    m_regDarkLocatorOldLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameLocatorOldColorDark", DARKBLAMEOLDCOLORBAR);
    m_regDarkLocatorNewLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameLocatorNewColorDark", DARKBLAMENEWCOLORBAR);
}

void TortoiseBlame::SelectLine(int yPos, bool bAlwaysSelect) const
{
    LONG line   = static_cast<LONG>(app.SendEditor(SCI_GETFIRSTVISIBLELINE));
    LONG height = static_cast<LONG>(app.SendEditor(SCI_TEXTHEIGHT));
    line        = line + (yPos / height);
    if (line < static_cast<LONG>(app.m_revs.size()))
    {
        app.SetSelectedLine(line);
        bool bUseMerged = ((m_mergedRevs[line] > 0) && (m_mergedRevs[line] < m_revs[line]));
        bool selChange  = bUseMerged ? app.m_mergedRevs[line] != app.m_selectedRev : app.m_revs[line] != app.m_selectedRev;
        if ((bAlwaysSelect) || (selChange))
        {
            app.m_selectedRev     = bUseMerged ? app.m_mergedRevs[line] : app.m_revs[line];
            app.m_selectedOrigRev = app.m_revs[line];
            app.m_selectedAuthor  = bUseMerged ? app.m_mergedAuthors[line] : app.m_authors[line];
            app.m_selectedDate    = bUseMerged ? app.m_mergedDates[line] : app.m_dates[line];
        }
        else
        {
            app.m_selectedAuthor.clear();
            app.m_selectedDate.clear();
            app.m_selectedRev     = -2;
            app.m_selectedOrigRev = -2;
        }
        ::InvalidateRect(app.wBlame, nullptr, FALSE);
    }
    else
    {
        app.SetSelectedLine(-1);
    }
}

void TortoiseBlame::StartSearchSel()
{
    int selTextLen = static_cast<int>(SendEditor(SCI_GETSELTEXT));
    if (selTextLen == 0)
        return;

    auto seltextbuffer = std::make_unique<char[]>(selTextLen + 1);
    SendEditor(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(static_cast<char*>(seltextbuffer.get())));
    if (seltextbuffer[0] == 0)
        return;
    wcsncpy_s(m_szFindWhat, _countof(m_szFindWhat), CUnicodeUtils::StdGetUnicode(seltextbuffer.get()).c_str(), _countof(m_szFindWhat) - 1);
    m_fr.Flags = FR_HIDEUPDOWN | FR_HIDEWHOLEWORD | FR_DOWN;
    DoSearch(m_szFindWhat, m_fr.Flags);
}

void TortoiseBlame::StartSearchSelReverse()
{
    int selTextLen = static_cast<int>(SendEditor(SCI_GETSELTEXT));
    if (selTextLen == 0)
        return;

    auto seltextbuffer = std::make_unique<char[]>(selTextLen + 1);
    SendEditor(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(static_cast<char*>(seltextbuffer.get())));
    if (seltextbuffer[0] == 0)
        return;
    wcsncpy_s(m_szFindWhat, _countof(m_szFindWhat), CUnicodeUtils::StdGetUnicode(seltextbuffer.get()).c_str(), _countof(m_szFindWhat) - 1);
    m_fr.Flags = FR_HIDEUPDOWN | FR_HIDEWHOLEWORD;
    DoSearch(m_szFindWhat, m_fr.Flags);
}

void TortoiseBlame::StartSearch()
{
    if (currentDialog)
        return;
    bool bCase = false;
    // Initialize FINDREPLACE
    if (m_fr.Flags & FR_MATCHCASE)
        bCase = true;
    SecureZeroMemory(&m_fr, sizeof(m_fr));
    m_fr.lStructSize   = sizeof(m_fr);
    m_fr.hwndOwner     = wMain;
    m_fr.lpstrFindWhat = m_szFindWhat;
    m_fr.wFindWhatLen  = _countof(m_szFindWhat);
    m_fr.Flags         = FR_HIDEWHOLEWORD | FR_DOWN;
    m_fr.Flags |= bCase ? FR_MATCHCASE : 0;

    currentDialog = FindText(&m_fr);
    CTheme::Instance().SetThemeForDialog(currentDialog, CTheme::Instance().IsDarkTheme());
}

void TortoiseBlame::DoSearchNext()
{
    m_fr.Flags |= FR_DOWN;
    if (m_szFindWhat[0] == 0)
        return StartSearchSel();
    DoSearch(m_szFindWhat, m_fr.Flags);
}

void TortoiseBlame::DoSearchPrev()
{
    m_fr.Flags &= ~FR_DOWN;
    if (m_szFindWhat[0] == 0)
        return StartSearchSelReverse();
    DoSearch(m_szFindWhat, m_fr.Flags);
}

bool TortoiseBlame::DoSearch(LPWSTR what, DWORD flags)
{
    wchar_t szWhat[80]     = {0};
    int     pos            = static_cast<int>(SendEditor(SCI_GETCURRENTPOS));
    int     line           = static_cast<int>(SendEditor(SCI_LINEFROMPOSITION, pos));
    bool    bFound         = false;
    bool    bCaseSensitive = !!(flags & FR_MATCHCASE);
    bool    bSearchDown    = !!(flags & FR_DOWN);
    wcscpy_s(szWhat, what);

    if (!bCaseSensitive)
    {
        MakeLower(szWhat, wcslen(szWhat));
    }

    std::wstring sWhat = std::wstring(szWhat);

    int     textSelStart = 0;
    int     textSelEnd   = 0;
    wchar_t buf[20]      = {0};
    int     i            = 0;
    int     lineBufSize  = 4096;
    auto    lineBuf      = std::make_unique<char[]>(lineBufSize + 1);
    if (line == m_revs.size())
        --line;
    for (i = line; (bSearchDown ? (i < static_cast<int>(m_authors.size())) : (i >= 0)) && (!bFound); bSearchDown ? ++i : --i)
    {
        const int bufsize = static_cast<int>(SendEditor(SCI_GETLINE, i));
        if (bufsize >= lineBufSize)
        {
            lineBufSize = bufsize + 1024;
            lineBuf     = std::make_unique<char[]>(lineBufSize + 1);
        }
        SecureZeroMemory(lineBuf.get(), bufsize + 1LL);
        SendEditor(SCI_GETLINE, i, reinterpret_cast<LPARAM>(lineBuf.get()));
        std::wstring sLine = CUnicodeUtils::StdGetUnicode(lineBuf.get());
        if (!bCaseSensitive)
        {
            std::transform(sLine.begin(), sLine.end(), sLine.begin(), ::towlower);
        }
        swprintf_s(buf, L"%ld", m_revs[i]);
        if (m_authors[i].compare(sWhat) == 0)
            bFound = true;
        else if ((!bCaseSensitive) && (_wcsicmp(m_authors[i].c_str(), szWhat) == 0))
            bFound = true;
        else if (wcscmp(buf, szWhat) == 0)
            bFound = true;
        else if (wcsstr(sLine.c_str(), szWhat))
        {
            textSelStart = static_cast<int>(SendEditor(SCI_POSITIONFROMLINE, i)) + static_cast<int>(wcsstr(sLine.c_str(), szWhat) - sLine.c_str());
            textSelEnd   = textSelStart + static_cast<int>(CUnicodeUtils::StdGetUTF8(szWhat).size());
            if ((line != i) || (textSelEnd != pos))
                bFound = true;
        }
    }
    if (!bFound)
    {
        for (bSearchDown ? i = 0 : i = static_cast<int>(m_authors.size()) - 1; (bSearchDown ? (i < line) : (i > line)) && (!bFound); bSearchDown ? ++i : --i)
        {
            const int bufsize = static_cast<int>(SendEditor(SCI_GETLINE, i));
            if (bufsize >= lineBufSize)
            {
                lineBufSize = bufsize + 1024;
                lineBuf     = std::make_unique<char[]>(lineBufSize + 1);
            }
            SecureZeroMemory(lineBuf.get(), bufsize + 1LL);
            SendEditor(SCI_GETLINE, i, reinterpret_cast<LPARAM>(lineBuf.get()));
            std::wstring sLine = CUnicodeUtils::StdGetUnicode(lineBuf.get());
            if (!bCaseSensitive)
            {
                std::transform(sLine.begin(), sLine.end(), sLine.begin(), ::towlower);
            }
            swprintf_s(buf, L"%ld", m_revs[i]);
            if (m_authors[i].compare(sWhat) == 0)
                bFound = true;
            else if ((!bCaseSensitive) && (_wcsicmp(m_authors[i].c_str(), szWhat) == 0))
                bFound = true;
            else if (wcscmp(buf, szWhat) == 0)
                bFound = true;
            else if (wcsstr(sLine.c_str(), szWhat))
            {
                bFound       = true;
                textSelStart = static_cast<int>(SendEditor(SCI_POSITIONFROMLINE, i)) + static_cast<int>(wcsstr(sLine.c_str(), szWhat) - sLine.c_str());
                textSelEnd   = textSelStart + static_cast<int>(CUnicodeUtils::StdGetUTF8(szWhat).size());
            }
        }
    }
    if (bFound)
    {
        GotoLine(i);
        if (textSelStart == textSelEnd)
        {
            int selstart = static_cast<int>(SendEditor(SCI_GETCURRENTPOS));
            int selend   = static_cast<int>(SendEditor(SCI_POSITIONFROMLINE, i));
            SendEditor(SCI_SETSELECTIONSTART, selstart);
            SendEditor(SCI_SETSELECTIONEND, selend);
        }
        else
        {
            SendEditor(SCI_SETSELECTIONSTART, textSelStart);
            SendEditor(SCI_SETSELECTIONEND, textSelEnd);
        }
        m_selectedLine = i - 1;
    }
    else
    {
        ::MessageBox(currentDialog ? currentDialog : wMain, searchStringNotFound, L"TortoiseBlame", MB_ICONINFORMATION);
    }
    return true;
}

bool TortoiseBlame::GotoLine(long line) const
{
    --line;
    if (line < 0)
        return false;
    if (static_cast<unsigned long>(line) >= m_authors.size())
    {
        line = static_cast<long>(m_authors.size()) - 1;
    }

    int nCurrentPos       = static_cast<int>(SendEditor(SCI_GETCURRENTPOS));
    int nCurrentLine      = static_cast<int>(SendEditor(SCI_LINEFROMPOSITION, nCurrentPos));
    int nFirstVisibleLine = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));
    int nLinesOnScreen    = static_cast<int>(SendEditor(SCI_LINESONSCREEN));

    if (line >= nFirstVisibleLine && line <= nFirstVisibleLine + nLinesOnScreen)
    {
        // no need to scroll
        SendEditor(SCI_GOTOLINE, line);
    }
    else
    {
        // Place the requested line one third from the top
        if (line > nCurrentLine)
        {
            SendEditor(SCI_GOTOLINE, static_cast<WPARAM>(line + static_cast<int>(nLinesOnScreen) * (2 / 3.0)));
        }
        else
        {
            SendEditor(SCI_GOTOLINE, static_cast<WPARAM>(line - static_cast<int>(nLinesOnScreen) * (1 / 3.0)));
        }
    }

    // Highlight the line
    int nPosStart = static_cast<int>(SendEditor(SCI_POSITIONFROMLINE, line));
    int nPosEnd   = static_cast<int>(SendEditor(SCI_GETLINEENDPOSITION, line));
    SendEditor(SCI_SETSEL, nPosEnd, nPosStart);

    return true;
}

bool TortoiseBlame::ScrollToLine(long line) const
{
    if (line < 0)
        return false;

    int nCurrentLine = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));

    int scrolldelta = line - nCurrentLine;
    SendEditor(SCI_LINESCROLL, 0, scrolldelta);

    return true;
}

void TortoiseBlame::CopySelectedLogToClipboard() const
{
    if (m_selectedRev <= 0)
        return;
    std::map<LONG, std::wstring>::iterator iter;
    if ((iter = app.m_logMessages.find(m_selectedRev)) != app.m_logMessages.end())
    {
        std::wstring msg;
        msg += m_selectedAuthor;
        msg += L"  ";
        msg += app.m_selectedDate;
        msg += L"\n";
        msg += iter->second;
        msg += L"\n";
        CClipboardHelper clipboardHelper;
        if (!clipboardHelper.Open(app.wBlame))
            return;

        EmptyClipboard();
        HGLOBAL  hClipboardData = CClipboardHelper::GlobalAlloc((msg.size() + 1) * sizeof(wchar_t));
        wchar_t* pchData        = static_cast<wchar_t*>(GlobalLock(hClipboardData));
        wcscpy_s(pchData, msg.size() + 1LL, msg.c_str());
        GlobalUnlock(hClipboardData);
        SetClipboardData(CF_UNICODETEXT, hClipboardData);
    }
}

void TortoiseBlame::CopySelectedRevToClipboard() const
{
    if (m_selectedRev <= 0)
        return;

    wchar_t bufRev[40] = {0};
    swprintf_s(bufRev, L"%ld", m_selectedRev);
    auto             len = wcslen(bufRev);
    CClipboardHelper clipboardHelper;
    if (!clipboardHelper.Open(app.wBlame))
        return;

    EmptyClipboard();
    HGLOBAL  hClipboardData = CClipboardHelper::GlobalAlloc((len + 1) * sizeof(wchar_t));
    wchar_t* pchData        = static_cast<wchar_t*>(GlobalLock(hClipboardData));
    wcscpy_s(pchData, len + 1LL, bufRev);
    GlobalUnlock(hClipboardData);
    SetClipboardData(CF_UNICODETEXT, hClipboardData);
}

void TortoiseBlame::BlamePreviousRevision() const
{
    LONG nRevisionTo = m_selectedOrigRev - 1;
    if (nRevisionTo < 1)
    {
        return;
    }

    // We now determine the smallest revision number in the blame file (but ignore "-1")
    // We do this for two reasons:
    // 1. we respect the "From revision" which the user entered
    // 2. we speed up the call of "svn blame" because previous smaller revision numbers don't have any effect on the result
    LONG nSmallestRevision = -1;
    for (LONG line = 0; line < static_cast<LONG>(app.m_revs.size()); line++)
    {
        const LONG nRevision = app.m_revs[line];
        if (nRevision > 0)
        {
            if (nSmallestRevision < 1)
            {
                nSmallestRevision = nRevision;
            }
            else
            {
                nSmallestRevision = min(nSmallestRevision, nRevision);
            }
        }
    }

    wchar_t bufStartRev[20] = {0};
    swprintf_s(bufStartRev, L"%ld", nSmallestRevision);

    wchar_t bufEndRev[20] = {0};
    swprintf_s(bufEndRev, L"%ld", nRevisionTo);

    wchar_t bufLine[20] = {0};
    swprintf_s(bufLine, L"%ld", m_selectedLine + 1); //using the current line is a good guess.

    std::wstring svnCmd = L" /command:blame ";
    svnCmd += L" /path:\"";
    svnCmd += szOrigPath;
    svnCmd += L"\"";
    svnCmd += L" /startrev:";
    svnCmd += bufStartRev;
    svnCmd += L" /endrev:";
    svnCmd += bufEndRev;
    svnCmd += szPegRev;
    svnCmd += L" /line:";
    svnCmd += bufLine;
    if (bIgnoreEOL)
        svnCmd += L" /ignoreeol";
    if (bIgnoreSpaces)
        svnCmd += L" /ignorespaces";
    if (bIgnoreAllSpaces)
        svnCmd += L" /ignoreallspaces";
    if (!uuid.empty())
    {
        svnCmd += L" /groupuuid:\"";
        svnCmd += uuid;
        svnCmd += L"\"";
    }
    RunCommand(svnCmd);
}

void TortoiseBlame::DiffPreviousRevision() const
{
    LONG nRevisionTo = m_selectedOrigRev;
    if (nRevisionTo < 1)
    {
        return;
    }

    LONG nRevisionFrom = nRevisionTo - 1;

    wchar_t bufStartRev[20] = {0};
    swprintf_s(bufStartRev, L"%ld", nRevisionFrom);

    wchar_t bufEndRev[20] = {0};
    swprintf_s(bufEndRev, L"%ld", nRevisionTo);

    std::wstring svnCmd = L" /command:diff ";
    svnCmd += L" /path:\"";
    svnCmd += szOrigPath;
    svnCmd += L"\"";
    svnCmd += L" /startrev:";
    svnCmd += bufStartRev;
    svnCmd += L" /endrev:";
    svnCmd += bufEndRev;
    svnCmd += szPegRev;
    if (!uuid.empty())
    {
        svnCmd += L" /groupuuid:\"";
        svnCmd += uuid;
        svnCmd += L"\"";
    }
    RunCommand(svnCmd);
}

void TortoiseBlame::ShowLog() const
{
    wchar_t bufRev[20] = {0};
    swprintf_s(bufRev, _countof(bufRev), L"%ld", m_selectedOrigRev);

    std::wstring svnCmd = L" /command:log ";
    svnCmd += L" /path:\"";
    svnCmd += szOrigPath;
    svnCmd += L"\"";
    svnCmd += L" /startrev:";
    svnCmd += bufRev;
    svnCmd += szPegRev;
    if (!uuid.empty())
    {
        svnCmd += L" /groupuuid:\"";
        svnCmd += uuid;
        svnCmd += L"\"";
    }

    if (m_selectedOrigRev != m_selectedRev)
    {
        svnCmd += L" /merge";
    }
    RunCommand(svnCmd);
}

void TortoiseBlame::Notify(SCNotification* notification)
{
    switch (notification->nmhdr.code)
    {
        case SCN_SAVEPOINTREACHED:
            break;

        case SCN_SAVEPOINTLEFT:
            break;
        case SCN_PAINTED:
            InvalidateRect(wBlame, nullptr, FALSE);
            InvalidateRect(wLocator, nullptr, FALSE);
            break;
        case SCN_GETBKCOLOR:
            if ((m_colorBy != COLORBYNONE) && (notification->line < static_cast<int>(m_revs.size())))
            {
                notification->lParam = GetLineColor(notification->line, false);
            }
            break;
        case SCN_ZOOM:
            if (showLine)
            {
                int numDigits = 0;
                int lineCount = static_cast<int>(m_authors.size());
                while (lineCount)
                {
                    lineCount /= 10;
                    numDigits++;
                }
                numDigits      = max(numDigits, 3);
                int pixelWidth = static_cast<int>(8 + numDigits * SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, reinterpret_cast<LPARAM>("8")));
                SendEditor(SCI_SETMARGINWIDTHN, 0, pixelWidth);
                CreateFont(static_cast<int>(SendEditor(SCI_TEXTHEIGHT)));
                m_blameWidth = 0;
                InitSize();
            }
            break;
        case SCN_UPDATEUI:
        {
            LRESULT firstLine     = SendEditor(SCI_GETFIRSTVISIBLELINE);
            LRESULT lastLine      = firstLine + SendEditor(SCI_LINESONSCREEN);
            long    startStylePos = static_cast<long>(SendEditor(SCI_POSITIONFROMLINE, firstLine));
            long    endStylePos   = static_cast<long>(SendEditor(SCI_POSITIONFROMLINE, lastLine)) + static_cast<long>(SendEditor(SCI_LINELENGTH, lastLine));
            if (endStylePos < 0)
                endStylePos = static_cast<long>(SendEditor(SCI_GETLENGTH)) - startStylePos;

            int len = endStylePos - startStylePos + 1;
            // reset indicators
            SendEditor(SCI_SETINDICATORCURRENT, STYLE_MARK);
            SendEditor(SCI_INDICATORCLEARRANGE, startStylePos, len);

            int selTextLen = static_cast<int>(SendEditor(SCI_GETSELTEXT));
            if (selTextLen == 0)
                break;

            auto seltextbuffer = std::make_unique<char[]>(selTextLen + 1);
            SendEditor(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(static_cast<char*>(seltextbuffer.get())));
            if (seltextbuffer[0] == 0)
                break;

            auto       textbuffer = std::make_unique<char[]>(len + 1);
            TEXTRANGEA textrange;
            textrange.lpstrText  = textbuffer.get();
            textrange.chrg.cpMin = startStylePos;
            textrange.chrg.cpMax = endStylePos;
            SendEditor(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textrange));

            char* startPos = strstr(textbuffer.get(), seltextbuffer.get());
            while (startPos)
            {
                SendEditor(SCI_INDICATORFILLRANGE, startStylePos + (static_cast<char*>(startPos) - static_cast<char*>(textbuffer.get())), selTextLen - 1LL);
                startPos = strstr(startPos + 1, seltextbuffer.get());
            }
        }
        break;
    }
}

void TortoiseBlame::Command(int id)
{
    switch (id)
    {
        case IDM_EXIT:
            ::PostQuitMessage(0);
            break;
        case ID_FILE_SETTINGS:
        {
            std::wstring svnCmd = L" /command:settings /page:19";
            RunCommand(svnCmd);
        }
        break;
        case ID_EDIT_FIND:
            StartSearch();
            break;
        case ID_EDIT_FINDSEL:
            StartSearchSel();
            break;
        case ID_EDIT_FINDSELREVERSE:
            StartSearchSelReverse();
            break;
        case ID_EDIT_FINDNEXT:
            DoSearchNext();
            break;
        case ID_EDIT_FINDPREV:
            DoSearchPrev();
            break;
        case ID_COPYTOCLIPBOARD:
            CopySelectedLogToClipboard();
            break;
        case ID_COPYTOCLIPBOARD_REV:
            CopySelectedRevToClipboard();
            break;
        case ID_BLAME_PREVIOUS_REVISION:
            BlamePreviousRevision();
            break;
        case ID_DIFF_PREVIOUS_REVISION:
            DiffPreviousRevision();
            break;
        case ID_SHOWLOG:
            ShowLog();
            break;
        case ID_EDIT_GOTOLINE:
            GotoLineDlg();
            break;
        case ID_VIEW_COLORBYAUTHOR:
        {
            if (m_colorBy == COLORBYAUTHOR)
                m_colorBy = COLORBYNONE;
            else
                m_colorBy = COLORBYAUTHOR;

            m_regColorBy = m_colorBy;
            SetupColoring();
            InitSize();
        }
        break;
        case ID_VIEW_COLORAGEOFLINES:
        {
            if (m_colorBy == COLORBYAGE)
                m_colorBy = COLORBYNONE;
            else
                m_colorBy = COLORBYAGE;

            m_regColorBy = m_colorBy;
            SetupColoring();
            InitSize();
        }
        break;
        case ID_VIEW_COLORBYAGE:
        {
            if (m_colorBy == COLORBYAGECONT)
                m_colorBy = COLORBYNONE;
            else
                m_colorBy = COLORBYAGECONT;

            m_regColorBy = m_colorBy;
            SetupColoring();
            InitSize();
        }
        break;
        case ID_VIEW_MERGEPATH:
        {
            bool bUseMerged = !m_mergedPaths.empty();
            showPath        = bUseMerged && !showPath;
            HMENU hMenu     = GetMenu(wMain);
            UINT  uCheck    = MF_BYCOMMAND;
            uCheck |= showPath ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_MERGEPATH, uCheck);
            m_blameWidth = 0;
            InitSize();
        }
        break;
        case ID_VIEW_DARKMODE:
        {
            CTheme::Instance().SetDarkTheme(!CTheme::Instance().IsDarkTheme());
        }
        break;
        default:
            break;
    };
}

void TortoiseBlame::GotoLineDlg() const
{
    if (DialogBox(hResource, MAKEINTRESOURCE(IDD_GOTODLG), wMain, GotoDlgProc) == IDOK)
    {
        GotoLine(m_gotoLine);
    }
}

INT_PTR CALLBACK TortoiseBlame::GotoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            CTheme::Instance().SetThemeForDialog(hwndDlg, CTheme::Instance().IsDarkTheme());
            HWND hwndOwner = GetParent(hwndDlg);
            RECT rc, rcOwner, rcDlg;
            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hwndDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);
            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
            SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + (rc.right / 2), rcOwner.top + (rc.bottom / 2), 0, 0, SWP_NOSIZE);
        }
        break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    HWND hEditCtrl = GetDlgItem(hwndDlg, IDC_LINENUMBER);
                    if (hEditCtrl)
                    {
                        wchar_t buf[MAX_PATH] = {0};
                        if (::GetWindowText(hEditCtrl, buf, _countof(buf)))
                        {
                            m_gotoLine = _wtol(buf);
                        }
                    }
                }
                    [[fallthrough]];
                case IDCANCEL:
                    CTheme::Instance().SetThemeForDialog(hwndDlg, false);
                    EndDialog(hwndDlg, wParam);
                    break;
            }
        }
        break;
    }
    return FALSE;
}

void TortoiseBlame::SetTheme(bool bDark)
{
    std::wstring fontNameW = CRegStdString(L"Software\\TortoiseSVN\\BlameFontName", L"Consolas");
    std::string  fontName  = CUnicodeUtils::StdGetUTF8(fontNameW);
    if (bDark)
    {
        DarkModeHelper::Instance().AllowDarkModeForApp(TRUE);

        SetClassLongPtr(wMain, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));
        SetClassLongPtr(wHeader, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));

        auto controls = {wMain, wEditor, wBlame, wHeader, wLocator, hwndTT};
        for (auto& wnd : controls)
        {
            DarkModeHelper::Instance().AllowDarkModeForWindow(wnd, TRUE);
            if (FAILED(SetWindowTheme(wnd, L"DarkMode_Explorer", nullptr)))
                SetWindowTheme(wnd, L"Explorer", nullptr);
        }

        BOOL                                        darkFlag = TRUE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data     = {DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag)};
        DarkModeHelper::Instance().SetWindowCompositionAttribute(wMain, &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
        SetupColoring();
        SendEditor(SCI_SETSELFORE, TRUE, CTheme::Instance().GetThemeColor(RGB(0, 0, 0)));
        SendEditor(SCI_SETSELBACK, TRUE, CTheme::Instance().GetThemeColor(RGB(51, 153, 255)));
    }
    else
    {
        SetClassLongPtr(wMain, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
        SetClassLongPtr(wHeader, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));

        auto controls = {wMain, wEditor, wBlame, wHeader, wLocator, hwndTT};
        for (auto& wnd : controls)
        {
            DarkModeHelper::Instance().AllowDarkModeForWindow(wnd, FALSE);
            SetWindowTheme(wnd, L"Explorer", nullptr);
        }

        BOOL                                        darkFlag = FALSE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data     = {DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag)};
        DarkModeHelper::Instance().SetWindowCompositionAttribute(wMain, &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
        DarkModeHelper::Instance().AllowDarkModeForApp(FALSE);
        SetupColoring();
        SendEditor(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
        SendEditor(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
    }
    DarkModeHelper::Instance().RefreshTitleBarThemeColor(wMain, bDark);
    if (bDark || CTheme::Instance().IsHighContrastModeDark())
    {
        for (int c = 0; c <= STYLE_DEFAULT; ++c)
        {
            SendEditor(SCI_STYLESETFORE, c, BlameTextColorDark);
            SendEditor(SCI_STYLESETBACK, c, BlameBackColorDark);
        }
        SendEditor(SCI_SETCARETFORE, BlameTextColorDark);
        SendEditor(SCI_SETWHITESPACEFORE, true, RGB(180, 180, 180));
        SendEditor(SCI_STYLESETFORE, STYLE_BRACELIGHT, RGB(0, 150, 0));
        SendEditor(SCI_STYLESETBOLD, STYLE_BRACELIGHT, 1);
        SendEditor(SCI_STYLESETFORE, STYLE_BRACEBAD, RGB(255, 0, 0));
        SendEditor(SCI_STYLESETBOLD, STYLE_BRACEBAD, 1);
        SendEditor(SCI_SETFOLDMARGINCOLOUR, true, BlameTextColorDark);
        SendEditor(SCI_SETFOLDMARGINHICOLOUR, true, RGB(0, 0, 0));
        SendEditor(SCI_STYLESETFORE, STYLE_LINENUMBER, RGB(140, 140, 140));
        SendEditor(SCI_STYLESETBACK, STYLE_LINENUMBER, BlameBackColorDark);
    }
    else
    {
        for (int c = 0; c <= STYLE_DEFAULT; ++c)
        {
            SendEditor(SCI_STYLESETFORE, c, CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT));
            SendEditor(SCI_STYLESETBACK, c, CTheme::Instance().IsDarkTheme() ? CTheme::darkBkColor : GetSysColor(COLOR_WINDOW));
        }
        SendEditor(SCI_SETCARETFORE, CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT));
        SendEditor(SCI_SETWHITESPACEFORE, true, ::GetSysColor(COLOR_3DSHADOW));
        SendEditor(SCI_STYLESETFORE, STYLE_BRACELIGHT, RGB(0, 150, 0));
        SendEditor(SCI_STYLESETBOLD, STYLE_BRACELIGHT, 1);
        SendEditor(SCI_STYLESETFORE, STYLE_BRACEBAD, RGB(255, 0, 0));
        SendEditor(SCI_STYLESETBOLD, STYLE_BRACEBAD, 1);
        SendEditor(SCI_SETFOLDMARGINCOLOUR, true, RGB(240, 240, 240));
        SendEditor(SCI_SETFOLDMARGINHICOLOUR, true, RGB(255, 255, 255));
        SendEditor(SCI_STYLESETFORE, STYLE_LINENUMBER, RGB(109, 109, 109));
        SendEditor(SCI_STYLESETBACK, STYLE_LINENUMBER, RGB(230, 230, 230));
    }

    HMENU hMenu  = GetMenu(wMain);
    UINT  uCheck = MF_BYCOMMAND;
    uCheck |= CTheme::Instance().IsDarkTheme() ? MF_CHECKED : MF_UNCHECKED;
    CheckMenuItem(hMenu, ID_VIEW_DARKMODE, uCheck);
    UINT uEnabled = MF_BYCOMMAND;
    uEnabled |= CTheme::Instance().IsDarkModeAllowed() ? MF_ENABLED : MF_DISABLED;
    EnableMenuItem(hMenu, ID_VIEW_DARKMODE, uEnabled);

    ::RedrawWindow(wMain, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

LONG TortoiseBlame::GetBlameWidth()
{
    if (m_blameWidth)
        return m_blameWidth;
    LONG    blameWidth = 0;
    SIZE    width;
    HDC     hDC           = ::GetDC(wBlame);
    HFONT   oldFont       = static_cast<HFONT>(::SelectObject(hDC, m_font));
    wchar_t buf[MAX_PATH] = {0};
    swprintf_s(buf, L"*%8d ", 88888888);
    ::GetTextExtentPoint(hDC, buf, static_cast<int>(wcslen(buf)), &width);
    m_revWidth = width.cx + CDPIAware::Instance().Scale(wBlame, BLAMESPACE);
    blameWidth += m_revWidth;
    if (showDate)
    {
        swprintf_s(buf, L"%30s", L"31.08.2001 06:24:14");
        ::GetTextExtentPoint32(hDC, buf, static_cast<int>(wcslen(buf)), &width);
        m_dateWidth = width.cx + CDPIAware::Instance().Scale(wBlame, BLAMESPACE);
        blameWidth += m_dateWidth;
    }
    if (showAuthor)
    {
        SIZE maxWidth = {0};
        for (auto I = m_authors.begin(); I != m_authors.end(); ++I)
        {
            ::GetTextExtentPoint32(hDC, I->c_str(), static_cast<int>(I->size()), &width);
            if (width.cx > maxWidth.cx)
                maxWidth = width;
        }
        m_authorWidth = maxWidth.cx + CDPIAware::Instance().Scale(wBlame, BLAMESPACE);
        blameWidth += m_authorWidth;
    }
    if (showPath)
    {
        SIZE maxWidth = {0};
        for (auto I = m_mergedPaths.begin(); I != m_mergedPaths.end(); ++I)
        {
            ::GetTextExtentPoint32(hDC, I->c_str(), static_cast<int>(I->size()), &width);
            if (width.cx > maxWidth.cx)
                maxWidth = width;
        }
        m_pathWidth = maxWidth.cx + CDPIAware::Instance().Scale(wBlame, BLAMESPACE);
        blameWidth += m_pathWidth;
    }
    ::SelectObject(hDC, oldFont);
    POINT pt = {blameWidth, 0};
    LPtoDP(hDC, &pt, 1);
    m_blameWidth = pt.x;
    ReleaseDC(wBlame, hDC);
    return m_blameWidth;
}

void TortoiseBlame::CreateFont(int fontSize)
{
    if (m_font)
        DeleteObject(m_font);
    if (m_italicFont)
        DeleteObject(m_italicFont);
    if (m_uiFont)
        DeleteObject(m_uiFont);

    NONCLIENTMETRICS metrics = {0};
    metrics.cbSize           = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
    metrics.lfMessageFont.lfHeight = static_cast<LONG>(CDPIAware::Instance().ScaleFactorSystemToWindow(wBlame) * metrics.lfMessageFont.lfHeight);
    m_uiFont                       = CreateFontIndirect(&metrics.lfMessageFont);

    LOGFONT lf   = {0};
    lf.lfWeight  = FW_NORMAL;
    HDC hDC      = ::GetDC(wBlame);
    lf.lfCharSet = DEFAULT_CHARSET;
    if (fontSize == 0)
        lf.lfHeight = -CDPIAware::Instance().PointsToPixels(wBlame, static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\BlameFontSize", 10)));
    else
        lf.lfHeight = -fontSize;
    CRegStdString fontName = CRegStdString(L"Software\\TortoiseSVN\\BlameFontName", L"Consolas");
    wcscpy_s(lf.lfFaceName, static_cast<std::wstring>(fontName).c_str());
    m_font = ::CreateFontIndirect(&lf);

    lf.lfItalic  = TRUE;
    lf.lfWeight  = FW_BLACK;
    m_italicFont = ::CreateFontIndirect(&lf);

    ReleaseDC(wBlame, hDC);
}

void TortoiseBlame::DrawBlame(HDC hDC) const
{
    if (hDC == nullptr)
        return;
    if (m_font == nullptr)
        return;

    HFONT    oldFont       = nullptr;
    LONG_PTR line          = SendEditor(SCI_GETFIRSTVISIBLELINE);
    LONG_PTR linesOnScreen = SendEditor(SCI_LINESONSCREEN);
    LONG_PTR height        = SendEditor(SCI_TEXTHEIGHT);
    LONG_PTR Y             = 0;
    wchar_t  buf[MAX_PATH] = {0};
    RECT     rc;
    BOOL     sel = FALSE;
    GetClientRect(wBlame, &rc);
    ::SetBkColor(hDC, m_windowColor);
    ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);
    for (LRESULT i = line; i < (line + linesOnScreen); ++i)
    {
        sel = FALSE;
        if (i < static_cast<int>(m_revs.size()))
        {
            bool bUseMerged = ((m_mergedRevs[i] > 0) && (m_mergedRevs[i] < m_revs[i]));
            if (bUseMerged)
                oldFont = static_cast<HFONT>(::SelectObject(hDC, m_italicFont));
            else
                oldFont = static_cast<HFONT>(::SelectObject(hDC, m_font));
            ::SetBkColor(hDC, m_windowColor);
            ::SetTextColor(hDC, m_textColor);
            std::wstring author;
            if (i < static_cast<int>(m_authors.size()))
                author = bUseMerged ? m_mergedAuthors[i] : m_authors[i];
            if (!author.empty())
            {
                if (author.compare(m_mouseAuthor) == 0)
                    ::SetBkColor(hDC, m_mouseAuthorColor);
                if (author.compare(m_selectedAuthor) == 0)
                {
                    ::SetBkColor(hDC, m_selectedAuthorColor);
                    ::SetTextColor(hDC, m_textHighLightColor);
                    sel = TRUE;
                }
            }
            svn_revnum_t rev = bUseMerged ? m_mergedRevs[i] : m_revs[i];
            if ((rev == m_mouseRev) && (!sel))
                ::SetBkColor(hDC, m_mouseRevColor);
            if ((rev == m_selectedRev) && (rev >= 0))
            {
                ::SetBkColor(hDC, m_selectedRevColor);
                ::SetTextColor(hDC, m_textHighLightColor);
            }
            if (rev >= 0)
                swprintf_s(buf, L"%c%8ld       ", bUseMerged ? '*' : ' ', rev);
            else
                wcscpy_s(buf, L"     ----       ");
            rc.right    = rc.left + m_revWidth;
            RECT drawRC = rc;
            drawRC.left = 0;
            drawRC.top  = static_cast<LONG>(Y);
            ::DrawText(hDC, buf, static_cast<int>(wcslen(buf)), &drawRC, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE);
            int left = m_revWidth;
            if (showDate)
            {
                rc.right = rc.left + left + m_dateWidth;
                swprintf_s(buf, L"%30s            ", bUseMerged ? m_mergedDates[i].c_str() : m_dates[i].c_str());
                ::ExtTextOut(hDC, left, static_cast<int>(Y), ETO_CLIPPED, &rc, buf, static_cast<UINT>(wcslen(buf)), nullptr);
                left += m_dateWidth;
            }
            if (showAuthor)
            {
                rc.right = rc.left + left + m_authorWidth;
                swprintf_s(buf, L"%-30s            ", author.c_str());
                drawRC      = rc;
                drawRC.left = left;
                drawRC.top  = static_cast<LONG>(Y);
                ::DrawText(hDC, buf, static_cast<int>(wcslen(buf)), &drawRC, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE);
                left += m_authorWidth;
            }
            if (showPath && !m_mergedPaths.empty())
            {
                rc.right = rc.left + left + m_pathWidth;
                swprintf_s(buf, L"%-60s            ", m_mergedPaths[i].c_str());
                drawRC      = rc;
                drawRC.left = left;
                drawRC.top  = static_cast<LONG>(Y);
                ::DrawText(hDC, buf, static_cast<int>(wcslen(buf)), &drawRC, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE);
                left += m_authorWidth;
            }
            if ((i == m_selectedLine) && (currentDialog))
            {
                LOGBRUSH brush;
                brush.lbColor   = m_textColor;
                brush.lbHatch   = 0;
                brush.lbStyle   = BS_SOLID;
                HPEN    pen     = ExtCreatePen(PS_SOLID | PS_GEOMETRIC, 2, &brush, 0, nullptr);
                HGDIOBJ hPenOld = SelectObject(hDC, pen);
                RECT    rc2     = rc;
                rc2.top         = static_cast<LONG>(Y);
                rc2.bottom      = static_cast<LONG>(Y + height);
                ::MoveToEx(hDC, rc2.left, rc2.top, nullptr);
                ::LineTo(hDC, rc2.right, rc2.top);
                ::LineTo(hDC, rc2.right, rc2.bottom);
                ::LineTo(hDC, rc2.left, rc2.bottom);
                ::LineTo(hDC, rc2.left, rc2.top);
                SelectObject(hDC, hPenOld);
                DeleteObject(pen);
            }
            Y += height;
            ::SelectObject(hDC, oldFont);
        }
        else
        {
            break;
        }
    }
}

void TortoiseBlame::DrawHeader(HDC hDC) const
{
    if (hDC == nullptr)
        return;

    RECT  rc;
    HFONT oldFont = static_cast<HFONT>(::SelectObject(hDC, m_uiFont));
    GetClientRect(wHeader, &rc);
    ::SetBkColor(hDC, CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE)));
    ::SetTextColor(hDC, CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT));
    RECT edgeRc   = rc;
    edgeRc.bottom = edgeRc.top + CDPIAware::Instance().Scale(wHeader, HEADER_HEIGHT) / 2;
    if (!CTheme::Instance().IsDarkTheme())
        DrawEdge(hDC, &edgeRc, EDGE_BUMP, BF_FLAT | BF_RECT | BF_ADJUST);
    else
    {
        auto hPen     = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
        auto oldPen   = SelectObject(hDC, hPen);
        auto oldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));
        Rectangle(hDC, edgeRc.left, edgeRc.top, edgeRc.right, edgeRc.bottom);
        SelectObject(hDC, oldBrush);
        SelectObject(hDC, oldPen);
        DeleteObject(hPen);
    }

    // draw the path first
    WCHAR pathBuf[MAX_PATH] = {0};
    if (szViewtitle.size() >= MAX_PATH)
    {
        std::wstring str = szViewtitle;
        std::wregex  rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
        std::wstring replacement = L"$1$2...$3";
        std::wstring str2        = std::regex_replace(str, rx, replacement);
        if (str2.size() >= MAX_PATH)
            str2 = str2.substr(0, MAX_PATH - 2);
        wcscpy_s(pathBuf, str2.c_str());
        PathCompactPath(hDC, pathBuf, edgeRc.right - edgeRc.left - CDPIAware::Instance().Scale(wHeader, LOCATOR_WIDTH));
    }
    else
    {
        wcscpy_s(pathBuf, szViewtitle.c_str());
        PathCompactPath(hDC, pathBuf, edgeRc.right - edgeRc.left - CDPIAware::Instance().Scale(wHeader, LOCATOR_WIDTH));
    }
    DrawText(hDC, pathBuf, -1, &edgeRc, DT_SINGLELINE | DT_VCENTER);

    rc.top = rc.top + CDPIAware::Instance().Scale(wHeader, HEADER_HEIGHT) / 2;
    if (!CTheme::Instance().IsDarkTheme())
        DrawEdge(hDC, &rc, EDGE_BUMP, BF_FLAT | BF_RECT | BF_ADJUST);
    else
    {
        auto hPen     = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
        auto oldPen   = SelectObject(hDC, hPen);
        auto oldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));
        Rectangle(hDC, edgeRc.left, edgeRc.top, edgeRc.right, edgeRc.bottom);
        SelectObject(hDC, oldBrush);
        SelectObject(hDC, oldPen);
        DeleteObject(hPen);
    }

    RECT drawRc = rc;

    wchar_t szText[MAX_LOADSTRING] = {0};
    LoadString(app.hResource, IDS_HEADER_REVISION, szText, MAX_LOADSTRING);
    drawRc.left = CDPIAware::Instance().Scale(wHeader, LOCATOR_WIDTH);
    DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE | DT_VCENTER);
    int left = m_revWidth + CDPIAware::Instance().Scale(wHeader, LOCATOR_WIDTH);
    if (showDate)
    {
        LoadString(app.hResource, IDS_HEADER_DATE, szText, MAX_LOADSTRING);
        drawRc.left = left;
        DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE | DT_VCENTER);
        left += m_dateWidth;
    }
    if (showAuthor)
    {
        LoadString(app.hResource, IDS_HEADER_AUTHOR, szText, MAX_LOADSTRING);
        drawRc.left = left;
        DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE | DT_VCENTER);
        left += m_authorWidth;
    }
    if (showPath)
    {
        LoadString(app.hResource, IDS_HEADER_PATH, szText, MAX_LOADSTRING);
        drawRc.left = left;
        DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE | DT_VCENTER);
        left += m_pathWidth;
    }
    LoadString(app.hResource, IDS_HEADER_LINE, szText, MAX_LOADSTRING);
    drawRc.left = left;
    DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE | DT_VCENTER);

    ::SelectObject(hDC, oldFont);
}

void TortoiseBlame::DrawLocatorBar(HDC hDC)
{
    if (hDC == nullptr)
        return;

    LONG_PTR line          = SendEditor(SCI_GETFIRSTVISIBLELINE);
    LONG_PTR linesOnScreen = SendEditor(SCI_LINESONSCREEN);
    LONG_PTR Y             = 0;
    COLORREF blackColor    = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);

    RECT rc;
    GetClientRect(wLocator, &rc);
    RECT lineRect    = rc;
    LONG height      = rc.bottom - rc.top;
    LONG currentLine = 0;

    // draw the colored bar
    for (auto it = m_revs.begin(); it != m_revs.end(); ++it)
    {
        // get the line color
        COLORREF cr = GetLineColor(currentLine, true);
        currentLine++;
        if ((currentLine > line) && (currentLine <= (line + linesOnScreen)))
        {
            cr = InterColor(cr, blackColor, 10);
        }
        SetBkColor(hDC, cr);
        lineRect.top    = static_cast<LONG>(Y);
        lineRect.bottom = static_cast<LONG>(currentLine * height / m_revs.size());
        ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, nullptr, 0, nullptr);
        Y = lineRect.bottom;
    }

    if (!m_revs.empty())
    {
        // now draw two lines indicating the scroll position of the source view
        SetBkColor(hDC, blackColor);
        lineRect.top    = static_cast<LONG>(line * height / m_revs.size());
        lineRect.bottom = lineRect.top + 1;
        ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, nullptr, 0, nullptr);
        lineRect.top    = static_cast<LONG>((line + linesOnScreen) * height / m_revs.size());
        lineRect.bottom = lineRect.top + 1;
        ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, nullptr, 0, nullptr);
    }
}

void TortoiseBlame::StringExpand(LPSTR str) const
{
    char* cPos = str;
    do
    {
        cPos = strchr(cPos, '\n');
        if (cPos)
        {
            memmove(cPos + 1, cPos, strlen(cPos));
            *cPos = '\r';
            cPos++;
            cPos++;
        }
    } while (cPos != nullptr);
}
void TortoiseBlame::StringExpand(LPWSTR str) const
{
    wchar_t* cPos = str;
    do
    {
        cPos = wcschr(cPos, '\n');
        if (cPos)
        {
            wmemmove(cPos + 1, cPos, wcslen(cPos));
            *cPos = '\r';
            cPos++;
            cPos++;
        }
    } while (cPos != nullptr);
}

void TortoiseBlame::MakeLower(wchar_t* buffer, size_t len)
{
    CharLowerBuff(buffer, static_cast<DWORD>(len));
}

void TortoiseBlame::RunCommand(const std::wstring& command)
{
    std::wstring tortoiseProcPath = GetAppDirectory() + L"TortoiseProc.exe";
    CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), command.c_str());
}

static void ProcessWindowsMessage(HWND window, HACCEL table, MSG& message)
{
    if (TranslateAccelerator(window, table, &message) == 0)
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

// Forward declarations of functions included in this code module:
ATOM             MyRegisterClass(HINSTANCE hResource);
ATOM             MyRegisterBlameClass(HINSTANCE hResource);
ATOM             MyRegisterHeaderClass(HINSTANCE hResource);
ATOM             MyRegisterLocatorClass(HINSTANCE hResource);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndBlameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndHeaderProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndLocatorProc(HWND, UINT, WPARAM, LPARAM);
UINT             uFindReplaceMsg;

//Get the message identifier
const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE /*hPrevInstance*/,
                      LPWSTR lpCmdLine,
                      int    nCmdShow)
{
    app.hInstance = hInstance;

    SetDllDirectory(L"");
    CCrashReportTSVN crasher(L"TortoiseBlame " TEXT(APP_X64_STRING));
    CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR                    gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    HMODULE hSciLexerDll = ::LoadLibrary(L"SciLexer.DLL");
    if (hSciLexerDll == nullptr)
        return FALSE;

    setTaskIDPerUuid();

    CRegStdDWORD loc    = CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033);
    long         langId = loc;

    CLangDll langDll;
    app.hResource = langDll.Init(L"TortoiseBlame", langId);
    if (app.hResource == nullptr)
        app.hResource = app.hInstance;

    // Initialize global strings
    LoadString(app.hResource, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(app.hResource, IDC_TORTOISEBLAME, szWindowClass, MAX_LOADSTRING);
    LoadString(app.hResource, IDS_SEARCHNOTFOUND, searchStringNotFound, MAX_LOADSTRING);
    MyRegisterClass(app.hResource);
    MyRegisterBlameClass(app.hResource);
    MyRegisterHeaderClass(app.hResource);
    MyRegisterLocatorClass(app.hResource);

    // Perform application initialization:
    if (!InitInstance(app.hResource, nCmdShow))
    {
        langDll.Close();
        FreeLibrary(hSciLexerDll);
        return FALSE;
    }

    wchar_t blameFile[MAX_PATH] = {0};

    CCmdLineParser parser(lpCmdLine);

    if (parser.HasVal(L"groupuuid"))
    {
        uuid = parser.GetVal(L"groupuuid");
    }

    if (__argc > 1)
    {
        wcscpy_s(blameFile, __wargv[1]);
    }
    if (__argc > 2)
    {
        if (parser.HasKey(L"path"))
            szViewtitle = parser.GetVal(L"path");
        else if (__wargv[3])
            szViewtitle = __wargv[3];
        if (parser.HasVal(L"revrange"))
        {
            szViewtitle += L" : ";
            szViewtitle += parser.GetVal(L"revrange");
        }
    }
    if ((blameFile[0] == 0) || parser.HasKey(L"?") || parser.HasKey(L"help"))
    {
        wchar_t szInfo[MAX_LOADSTRING] = {0};
        LoadString(app.hResource, IDS_COMMANDLINE_INFO, szInfo, MAX_LOADSTRING);
        MessageBox(nullptr, szInfo, L"TortoiseBlame", MB_ICONERROR);
        langDll.Close();
        FreeLibrary(hSciLexerDll);
        return 0;
    }

    if (parser.HasKey(L"path"))
    {
        szOrigPath = parser.GetVal(L"path");
    }

    if (parser.HasKey(L"pegrev"))
    {
        szPegRev = L" /pegrev:";
        szPegRev += parser.GetVal(L"pegrev");
    }

    app.bIgnoreEOL       = parser.HasKey(L"ignoreeol");
    app.bIgnoreSpaces    = parser.HasKey(L"ignorespaces");
    app.bIgnoreAllSpaces = parser.HasKey(L"ignoreallspaces");

    app.SendEditor(SCI_SETCODEPAGE, GetACP());
    app.OpenFile(blameFile);

    if (parser.HasKey(L"line"))
    {
        app.GotoLine(parser.GetLongVal(L"line"));
    }

    HACCEL hAccelTable = LoadAccelerators(app.hResource, reinterpret_cast<LPCWSTR>(IDC_TORTOISEBLAME));
    MSG    msg         = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        if (app.currentDialog == nullptr)
        {
            ProcessWindowsMessage(app.wMain, hAccelTable, msg);
            continue;
        }

        if (!IsDialogMessage(app.currentDialog, &msg))
        {
            ProcessWindowsMessage(msg.hwnd, hAccelTable, msg);
        }
    }
    langDll.Close();
    FreeLibrary(hSciLexerDll);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hResource)
{
    WNDCLASSEX wcEx;

    wcEx.cbSize = sizeof(WNDCLASSEX);

    wcEx.style         = CS_HREDRAW | CS_VREDRAW;
    wcEx.lpfnWndProc   = static_cast<WNDPROC>(WndProc);
    wcEx.cbClsExtra    = 0;
    wcEx.cbWndExtra    = 0;
    wcEx.hInstance     = hResource;
    wcEx.hIcon         = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEBLAME), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcEx.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcEx.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
    wcEx.lpszMenuName  = reinterpret_cast<LPCWSTR>(IDC_TORTOISEBLAME);
    wcEx.lpszClassName = szWindowClass;
    wcEx.hIconSm       = LoadIconEx(wcEx.hInstance, reinterpret_cast<LPCWSTR>(IDI_TORTOISEBLAME));

    return RegisterClassEx(&wcEx);
}

ATOM MyRegisterBlameClass(HINSTANCE hResource)
{
    WNDCLASSEX wcEx;

    wcEx.cbSize = sizeof(WNDCLASSEX);

    wcEx.style         = CS_HREDRAW | CS_VREDRAW;
    wcEx.lpfnWndProc   = static_cast<WNDPROC>(WndBlameProc);
    wcEx.cbClsExtra    = 0;
    wcEx.cbWndExtra    = 0;
    wcEx.hInstance     = hResource;
    wcEx.hIcon         = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEBLAME), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcEx.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcEx.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
    wcEx.lpszMenuName  = nullptr;
    wcEx.lpszClassName = L"TortoiseBlameBlame";
    wcEx.hIconSm       = LoadIconEx(wcEx.hInstance, reinterpret_cast<LPCWSTR>(IDI_TORTOISEBLAME));

    return RegisterClassEx(&wcEx);
}

ATOM MyRegisterHeaderClass(HINSTANCE hResource)
{
    WNDCLASSEX wcEx;

    wcEx.cbSize = sizeof(WNDCLASSEX);

    wcEx.style         = CS_HREDRAW | CS_VREDRAW;
    wcEx.lpfnWndProc   = static_cast<WNDPROC>(WndHeaderProc);
    wcEx.cbClsExtra    = 0;
    wcEx.cbWndExtra    = 0;
    wcEx.hInstance     = hResource;
    wcEx.hIcon         = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEBLAME), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcEx.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcEx.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_BTNFACE + 1));
    wcEx.lpszMenuName  = nullptr;
    wcEx.lpszClassName = L"TortoiseBlameHeader";
    wcEx.hIconSm       = LoadIconEx(wcEx.hInstance, reinterpret_cast<LPCWSTR>(IDI_TORTOISEBLAME));

    return RegisterClassEx(&wcEx);
}

ATOM MyRegisterLocatorClass(HINSTANCE hResource)
{
    WNDCLASSEX wcEx;

    wcEx.cbSize = sizeof(WNDCLASSEX);

    wcEx.style         = CS_HREDRAW | CS_VREDRAW;
    wcEx.lpfnWndProc   = static_cast<WNDPROC>(WndLocatorProc);
    wcEx.cbClsExtra    = 0;
    wcEx.cbWndExtra    = 0;
    wcEx.hInstance     = hResource;
    wcEx.hIcon         = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEBLAME), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcEx.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcEx.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
    wcEx.lpszMenuName  = nullptr;
    wcEx.lpszClassName = L"TortoiseBlameLocator";
    wcEx.hIconSm       = LoadIconEx(wcEx.hInstance, reinterpret_cast<LPCWSTR>(IDI_TORTOISEBLAME));

    return RegisterClassEx(&wcEx);
}

BOOL InitInstance(HINSTANCE hResource, int nCmdShow)
{
    app.wMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hResource, NULL);

    if (!app.wMain)
    {
        return FALSE;
    }

    auto         monHash = GetMonitorSetupHash();
    CRegStdDWORD pos(L"Software\\TortoiseSVN\\TBlamePos" + monHash, 0);
    CRegStdDWORD width(L"Software\\TortoiseSVN\\TBlameSize" + monHash, 0);
    CRegStdDWORD state(L"Software\\TortoiseSVN\\TBlameState" + monHash, 0);
    if (static_cast<DWORD>(pos) && static_cast<DWORD>(width))
    {
        RECT rc;
        rc.left       = LOWORD(static_cast<DWORD>(pos));
        rc.top        = HIWORD(static_cast<DWORD>(pos));
        rc.right      = rc.left + LOWORD(static_cast<DWORD>(width));
        rc.bottom     = rc.top + HIWORD(static_cast<DWORD>(width));
        HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
        if (hMon)
        {
            // only restore the window position if the monitor is valid
            MoveWindow(app.wMain, LOWORD(static_cast<DWORD>(pos)), HIWORD(static_cast<DWORD>(pos)),
                       LOWORD(static_cast<DWORD>(width)), HIWORD(static_cast<DWORD>(width)), FALSE);
        }
    }
    if (static_cast<DWORD>(state) == SW_MAXIMIZE)
        ShowWindow(app.wMain, SW_MAXIMIZE);
    else
        ShowWindow(app.wMain, nCmdShow);
    UpdateWindow(app.wMain);

    //Create the tooltips

    INITCOMMONCONTROLSEX iccEx;
    TOOLINFO             ti;
    RECT                 rect; // for client area coordinates
    iccEx.dwICC  = ICC_WIN95_CLASSES;
    iccEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCommonControlsEx(&iccEx);

    /* CREATE A TOOLTIP WINDOW */
    app.hwndTT = CreateWindowEx(NULL,
                                TOOLTIPS_CLASS,
                                nullptr,
                                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                app.wBlame,
                                nullptr,
                                app.hResource,
                                nullptr);

    SetWindowPos(app.hwndTT,
                 HWND_TOP,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    /* GET COORDINATES OF THE MAIN CLIENT AREA */
    GetClientRect(app.wBlame, &rect);

    /* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_TRACK | TTF_ABSOLUTE; //TTF_SUBCLASS | TTF_PARSELINKS;
    ti.hwnd     = app.wBlame;
    ti.hinst    = app.hResource;
    ti.uId      = 0;
    ti.lpszText = LPSTR_TEXTCALLBACK;
    // ToolTip control will cover the whole window
    ti.rect.left   = rect.left;
    ti.rect.top    = rect.top;
    ti.rect.right  = rect.right;
    ti.rect.bottom = rect.bottom;

    /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
    SendMessage(app.hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(static_cast<LPTTTOOLINFOW>(&ti)));
    SendMessage(app.hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
    //SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELONG(50000, 0));
    //SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_RESHOW, MAKELONG(1000, 0));

    uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);

    app.m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
        [&]() {
            app.SetTheme(CTheme::Instance().IsDarkTheme());
        });
    app.SetTheme(CTheme::Instance().IsDarkTheme());

    return TRUE;
}

void TortoiseBlame::InitSize()
{
    RECT rc{};
    RECT blameRc{};
    RECT sourceRc{};
    ::GetClientRect(wMain, &rc);
    ::SetWindowPos(wHeader, nullptr, rc.left, rc.top, rc.right - rc.left, CDPIAware::Instance().Scale(wMain, HEADER_HEIGHT), 0);
    rc.top += CDPIAware::Instance().Scale(wMain, HEADER_HEIGHT);
    blameRc.left    = rc.left;
    blameRc.top     = rc.top;
    LONG w          = GetBlameWidth();
    blameRc.right   = w > abs(rc.right - rc.left) ? rc.right : w + rc.left;
    blameRc.bottom  = rc.bottom;
    sourceRc.left   = blameRc.right;
    sourceRc.top    = rc.top;
    sourceRc.bottom = rc.bottom;
    sourceRc.right  = rc.right;
    if (m_colorBy != COLORBYNONE)
    {
        ::OffsetRect(&blameRc, CDPIAware::Instance().Scale(wMain, LOCATOR_WIDTH), 0);
        ::OffsetRect(&sourceRc, CDPIAware::Instance().Scale(wMain, LOCATOR_WIDTH), 0);
        sourceRc.right -= CDPIAware::Instance().Scale(wMain, LOCATOR_WIDTH);
    }
    InvalidateRect(wMain, nullptr, FALSE);
    ::SetWindowPos(wEditor, nullptr, sourceRc.left, sourceRc.top, sourceRc.right - sourceRc.left, sourceRc.bottom - sourceRc.top, 0);
    ::SetWindowPos(wBlame, nullptr, blameRc.left, blameRc.top, blameRc.right - blameRc.left, blameRc.bottom - blameRc.top, 0);
    if (m_colorBy != COLORBYNONE)
        ::SetWindowPos(wLocator, nullptr, 0, blameRc.top, CDPIAware::Instance().Scale(wMain, LOCATOR_WIDTH), blameRc.bottom - blameRc.top, SWP_SHOWWINDOW);
    else
        ::ShowWindow(wLocator, SW_HIDE);
}

COLORREF TortoiseBlame::GetLineColor(Sci_Position line, bool bLocatorBar)
{
    switch (m_colorBy)
    {
        case COLORBYAGE:
            return m_revColorMap[m_revs[line]];
        case COLORBYAGECONT:
        {
            if (bLocatorBar)
            {
                COLORREF newCol = (CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark()) ? static_cast<DWORD>(m_regDarkLocatorNewLinesColor) : static_cast<DWORD>(m_regLocatorNewLinesColor);
                COLORREF oldCol = (CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark()) ? static_cast<DWORD>(m_regDarkLocatorOldLinesColor) : static_cast<DWORD>(m_regLocatorOldLinesColor);
                return InterColor(oldCol, newCol, (m_revs[line] - m_lowestRev) * 100 / ((m_highestRev - m_lowestRev) + 1));
            }
            else
            {
                COLORREF newCol = (CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark()) ? static_cast<DWORD>(m_regDarkNewLinesColor) : static_cast<DWORD>(m_regNewLinesColor);
                COLORREF oldCol = (CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark()) ? static_cast<DWORD>(m_regDarkOldLinesColor) : static_cast<DWORD>(m_regOldLinesColor);
                return InterColor(oldCol, newCol, (m_revs[line] - m_lowestRev) * 100 / ((m_highestRev - m_lowestRev) + 1));
            }
        }
        case COLORBYAUTHOR:
            return m_authorColorMap[m_authors[line]];
        case COLORBYNONE:
        default:
            return m_windowColor;
    }
}

void TortoiseBlame::SetupColoring()
{
    m_windowColor = CTheme::Instance().IsDarkTheme() ? CTheme::darkBkColor : GetSysColor(COLOR_WINDOW);
    m_textColor   = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
    if (CTheme::Instance().IsDarkTheme() || CTheme::Instance().IsHighContrastModeDark())
    {
        m_textHighLightColor  = m_textColor;
        m_selectedRevColor    = RGB(0, 30, 80);
        m_selectedAuthorColor = InterColor(m_selectedRevColor, m_textHighLightColor, 15);
    }
    else
    {
        m_textHighLightColor  = GetSysColor(COLOR_HIGHLIGHTTEXT);
        m_selectedRevColor    = GetSysColor(COLOR_HIGHLIGHT);
        m_selectedAuthorColor = InterColor(m_selectedRevColor, m_textHighLightColor, 35);
    }

    m_mouseRevColor    = InterColor(m_windowColor, m_textColor, 20);
    m_mouseAuthorColor = InterColor(m_windowColor, m_textColor, 10);

    HMENU hMenu = GetMenu(wMain);
    CheckMenuItem(hMenu, ID_VIEW_COLORBYAUTHOR, MF_BYCOMMAND | ((m_colorBy == COLORBYAUTHOR) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_VIEW_COLORAGEOFLINES, MF_BYCOMMAND | ((m_colorBy == COLORBYAGE) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_VIEW_COLORBYAGE, MF_BYCOMMAND | ((m_colorBy == COLORBYAGECONT) ? MF_CHECKED : MF_UNCHECKED));
    m_blameWidth = 0;
    m_authorColorMap.clear();
    int colIndex = 0;
    switch (m_colorBy)
    {
        case COLORBYAGE:
        {
            for (auto it = m_revSet.cbegin(); it != m_revSet.cend(); ++it)
            {
                m_revColorMap[*it] = CTheme::Instance().GetThemeColor(colorSet[colIndex++ % MAX_BLAMECOLORS], true);
            }
        }
        break;
        case COLORBYAGECONT:
        {
            // nothing to do
        }
        break;
        case COLORBYAUTHOR:
        {
            for (auto it = m_authorSet.cbegin(); it != m_authorSet.cend(); ++it)
            {
                m_authorColorMap[*it] = CTheme::Instance().GetThemeColor(colorSet[colIndex++ % MAX_BLAMECOLORS], true);
            }
        }
        break;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == uFindReplaceMsg)
    {
        LPFINDREPLACE lpfr = reinterpret_cast<LPFINDREPLACE>(lParam);
        // there are actually processes that send this message to other
        // processes, with lParam set to zero or even invalid data!
        // We can't do anything about invalid data, but we can check
        // for a null pointer.
        if (lpfr)
        {
            // If the FR_DIALOGTERM flag is set,
            // invalidate the handle identifying the dialog box.
            if (lpfr->Flags & FR_DIALOGTERM)
            {
                app.currentDialog = nullptr;
                return 0;
            }
            if (lpfr->Flags & FR_FINDNEXT)
            {
                app.DoSearch(lpfr->lpstrFindWhat, lpfr->Flags);
            }
        }
        return 0;
    }
    if (message == TaskBarButtonCreated)
    {
        setUuidOverlayIcon(hWnd);
    }
    auto optRet = CTheme::HandleMenuBar(hWnd, message, wParam, lParam);
    if (optRet.has_value())
        return optRet.value();
    switch (message)
    {
        case WM_CREATE:
            app.wEditor = ::CreateWindow(
                L"Scintilla",
                L"Source",
                WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
                0, 0,
                100, 100,
                hWnd,
                nullptr,
                app.hResource,
                nullptr);
            app.InitialiseEditor();
            ::ShowWindow(app.wEditor, SW_SHOW);
            ::SetFocus(app.wEditor);
            app.wBlame = ::CreateWindow(
                L"TortoiseBlameBlame",
                L"blame",
                WS_CHILD | WS_CLIPCHILDREN,
                CW_USEDEFAULT, 0,
                CW_USEDEFAULT, 0,
                hWnd,
                NULL,
                app.hResource,
                NULL);
            ::ShowWindow(app.wBlame, SW_SHOW);
            app.wHeader = ::CreateWindow(
                L"TortoiseBlameHeader",
                L"header",
                WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
                CW_USEDEFAULT, 0,
                CW_USEDEFAULT, 0,
                hWnd,
                NULL,
                app.hResource,
                NULL);
            ::ShowWindow(app.wHeader, SW_SHOW);
            app.wLocator = ::CreateWindow(
                L"TortoiseBlameLocator",
                L"locator",
                WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
                CW_USEDEFAULT, 0,
                CW_USEDEFAULT, 0,
                hWnd,
                NULL,
                app.hResource,
                NULL);
            ::ShowWindow(app.wLocator, SW_SHOW);
            return 0;

        case WM_SIZE:
            if (wParam != 1)
            {
                app.InitSize();
            }
            return 0;

        case WM_COMMAND:
            app.Command(LOWORD(wParam));
            break;
        case WM_NOTIFY:
            app.Notify(reinterpret_cast<SCNotification*>(lParam));
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_CLOSE:
        {
            auto         monHash = GetMonitorSetupHash();
            CRegStdDWORD pos(L"Software\\TortoiseSVN\\TBlamePos_" + monHash, 0);
            CRegStdDWORD width(L"Software\\TortoiseSVN\\TBlameSize_" + monHash, 0);
            CRegStdDWORD state(L"Software\\TortoiseSVN\\TBlameState_" + monHash, 0);
            RECT         rc;
            GetWindowRect(app.wMain, &rc);
            if ((rc.left >= 0) && (rc.top >= 0))
            {
                pos   = MAKELONG(rc.left, rc.top);
                width = MAKELONG(rc.right - rc.left, rc.bottom - rc.top);
            }
            WINDOWPLACEMENT wp = {0};
            wp.length          = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(app.wMain, &wp);
            state = wp.showCmd;
            CTheme::Instance().RemoveRegisteredCallback(app.m_themeCallbackId);
            ::DestroyWindow(app.wEditor);
            ::PostQuitMessage(0);
        }
            return 0;
        case WM_SETFOCUS:
            ::SetFocus(app.wBlame);
            break;
        case WM_SYSCOLORCHANGE:
        case WM_SETTINGCHANGE:
            SendMessage(app.wEditor, message, wParam, lParam);
            CTheme::Instance().OnSysColorChanged();
            CTheme::Instance().SetDarkTheme(CTheme::Instance().IsDarkTheme(), true);
            return 0;
        case WM_DPICHANGED:
        {
            SendMessage(app.wEditor, message, wParam, lParam);
            CDPIAware::Instance().Invalidate();
            const RECT* rect = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(hWnd, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            app.CreateFont(0);
            ::RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndBlameProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hDC = BeginPaint(app.wBlame, &ps);
            app.DrawBlame(hDC);
            EndPaint(app.wBlame, &ps);
        }
        break;
        case WM_COMMAND:
            app.Command(LOWORD(wParam));
            break;
        case WM_NOTIFY:
            switch (reinterpret_cast<LPNMHDR>(lParam)->code)
            {
                case TTN_GETDISPINFO:
                {
                    POINT point;
                    DWORD ptW = GetMessagePos();
                    point.x   = GET_X_LPARAM(ptW);
                    point.y   = GET_Y_LPARAM(ptW);
                    ::ScreenToClient(app.wBlame, &point);
                    LONG_PTR line   = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
                    LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
                    line            = line + (point.y / height);
                    if (line >= static_cast<LONG>(app.m_revs.size()))
                        break;
                    if (line < 0)
                        break;
                    bool bUseMerged = ((app.m_mergedRevs[line] > 0) && (app.m_mergedRevs[line] < app.m_revs[line]));
                    LONG rev        = bUseMerged ? app.m_mergedRevs[line] : app.m_revs[line];
                    LONG origrev    = app.m_revs[line];

                    SecureZeroMemory(app.m_szTip, sizeof(app.m_szTip));
                    SecureZeroMemory(app.m_wszTip, sizeof(app.m_wszTip));
                    std::map<LONG, std::wstring>::iterator iter;
                    std::wstring                           msg;
                    if (!showAuthor)
                    {
                        msg += app.m_authors[line];
                    }
                    if (!showDate)
                    {
                        if (!showAuthor)
                            msg += L"  ";
                        msg += app.m_dates[line];
                    }
                    if ((iter = app.m_logMessages.find(rev)) != app.m_logMessages.end())
                    {
                        if (!showAuthor || !showDate)
                            msg += '\n';
                        msg += iter->second;
                        if (rev != origrev)
                        {
                            // add the merged revision
                            std::map<LONG, std::wstring>::iterator iter2;
                            if ((iter2 = app.m_logMessages.find(origrev)) != app.m_logMessages.end())
                            {
                                if (!msg.empty())
                                    msg += L"\n------------------\n";
                                wchar_t revBuf[100] = {0};
                                swprintf_s(revBuf, L"merged in r%ld:\n----\n", origrev);
                                msg += revBuf;
                                msg += iter2->second;
                            }
                        }
                    }
                    if (msg.size() > MAX_LOG_LENGTH)
                    {
                        msg = msg.substr(0, MAX_LOG_LENGTH - 5);
                        msg = msg + L"\n...";
                    }

                    // an empty tooltip string will deactivate the tooltips,
                    // which means we must make sure that the tooltip won't
                    // be empty.
                    if (msg.empty())
                        msg = L" ";

                    LPNMHDR pNMHDR = reinterpret_cast<LPNMHDR>(lParam);
                    if (pNMHDR->code == TTN_NEEDTEXTA)
                    {
                        NMTTDISPINFOA* pTtta = reinterpret_cast<NMTTDISPINFOA*>(pNMHDR);
                        StringCchCopyA(app.m_szTip, _countof(app.m_szTip), CUnicodeUtils::StdGetUTF8(msg).c_str());
                        app.StringExpand(app.m_szTip);
                        pTtta->lpszText = app.m_szTip;
                    }
                    else
                    {
                        NMTTDISPINFOW* pTttw = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);
                        pTttw->lpszText      = app.m_wszTip;
                        StringCchCopy(app.m_wszTip, _countof(app.m_wszTip), msg.c_str());
                        app.StringExpand(app.m_wszTip);
                    }
                }
                break;
            }
            return 0;
        case WM_DESTROY:
            break;
        case WM_CLOSE:
            return 0;
        case WM_MOUSELEAVE:
            app.m_mouseRev = -2;
            app.m_mouseAuthor.clear();
            app.m_ttVisible = FALSE;
            SendMessage(app.hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
            ::InvalidateRect(app.wBlame, nullptr, FALSE);
            break;
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT mevt;
            mevt.cbSize      = sizeof(TRACKMOUSEEVENT);
            mevt.dwFlags     = TME_LEAVE;
            mevt.dwHoverTime = HOVER_DEFAULT;
            mevt.hwndTrack   = app.wBlame;
            ::TrackMouseEvent(&mevt);
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(app.wBlame, &pt);
            pt.x += 15;
            pt.y += 15;
            SendMessage(app.hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
            if (!app.m_ttVisible)
            {
                TOOLINFO ti;
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd   = app.wBlame;
                ti.uId    = 0;
                SendMessage(app.hwndTT, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));
            }
            int      y      = GET_Y_LPARAM(lParam);
            LONG_PTR line   = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
            LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
            line            = line + (y / height);
            app.m_ttVisible = (line < static_cast<LONG>(app.m_revs.size()));
            if (app.m_ttVisible)
            {
                if (app.m_authors[line].compare(app.m_mouseAuthor) != 0)
                {
                    app.m_mouseAuthor = app.m_authors[line];
                }
                if (app.m_revs[line] != app.m_mouseRev)
                {
                    app.m_mouseRev = app.m_revs[line];
                    ::InvalidateRect(app.wBlame, nullptr, FALSE);
                    SendMessage(app.hwndTT, TTM_UPDATE, 0, 0);
                }
            }
        }
        break;
        case WM_LBUTTONDOWN:
        {
            int y = GET_Y_LPARAM(lParam);
            app.SelectLine(y, false);
        }
        break;
        case WM_SETFOCUS:
            ::SetFocus(app.wBlame);
            app.SendEditor(SCI_GRABFOCUS);
            break;
        case WM_CONTEXTMENU:
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            if ((xPos == -1) || (yPos == -1))
            {
                // requested from keyboard, not mouse pointer
                // use the center of the window
                RECT rect;
                GetWindowRect(app.wBlame, &rect);
                xPos = rect.right - rect.left;
                yPos = rect.bottom - rect.top;
            }
            POINT yPt = {0, yPos};
            ScreenToClient(app.wBlame, &yPt);
            app.SelectLine(yPt.y, true);
            if (app.m_selectedRev <= 0)
                break;

            HMENU hMenu    = LoadMenu(app.hResource, MAKEINTRESOURCE(IDR_BLAMEPOPUP));
            HMENU hPopMenu = GetSubMenu(hMenu, 0);

            if (szOrigPath.empty())
            {
                // Without knowing the original path we cannot blame the previous revision
                // because we don't know which filename to pass to tortoiseproc.
                EnableMenuItem(hPopMenu, ID_BLAME_PREVIOUS_REVISION, MF_DISABLED | MF_GRAYED);
            }

            TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, xPos, yPos, 0, app.wBlame, nullptr);
            DestroyMenu(hMenu);
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndHeaderProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hDC = BeginPaint(app.wHeader, &ps);
            app.DrawHeader(hDC);
            EndPaint(app.wHeader, &ps);
        }
        break;
        case WM_COMMAND:
            break;
        case WM_DESTROY:
            break;
        case WM_CLOSE:
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndLocatorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hDC = BeginPaint(app.wLocator, &ps);
            app.DrawLocatorBar(hDC);
            EndPaint(app.wLocator, &ps);
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_MOUSEMOVE:
            if (wParam & MK_LBUTTON)
            {
                RECT rect;
                ::GetClientRect(hWnd, &rect);
                int nLine = static_cast<int>((HIWORD(lParam) * app.m_revs.size() / (rect.bottom - rect.top)));

                if (nLine < 0)
                    nLine = 0;
                app.ScrollToLine(nLine);
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#pragma warning(pop)
