// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2021 - TortoiseSVN
// Copyright (C) 2015-2020 - TortoiseGit

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
#include "resource.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include <string>
#include "registry.h"
#include "SciEdit.h"
#include "OnOutOfScope.h"
#include "LoadIconEx.h"
#include "Theme.h"
#include "DarkModeHelper.h"

void CSciEditContextMenuInterface::InsertMenuItems(CMenu&, int&) { return; }
bool CSciEditContextMenuInterface::HandleMenuItemClick(int, CSciEdit*) { return false; }
void CSciEditContextMenuInterface::HandleSnippet(int, const CString&, CSciEdit*) { return; }

#define STYLE_ISSUEBOLD       11
#define STYLE_ISSUEBOLDITALIC 12
#define STYLE_BOLD            14
#define STYLE_ITALIC          15
#define STYLE_UNDERLINED      16
#define STYLE_URL             17
#define INDIC_MISSPELLED      18

#define STYLE_MASK 0x1f

#define SCI_ADDWORD 2000

struct LocMap
{
    const char* cp;
    const char* defEnc;
};

struct LocMap enc2Locale[] = {
    {"28591", "ISO8859-1"},
    {"28592", "ISO8859-2"},
    {"28593", "ISO8859-3"},
    {"28594", "ISO8859-4"},
    {"28595", "ISO8859-5"},
    {"28596", "ISO8859-6"},
    {"28597", "ISO8859-7"},
    {"28598", "ISO8859-8"},
    {"28599", "ISO8859-9"},
    {"28605", "ISO8859-15"},
    {"20866", "KOI8-R"},
    {"21866", "KOI8-U"},
    {"1251", "microsoft-cp1251"},
    {"65001", "UTF-8"},
};

IMPLEMENT_DYNAMIC(CSciEdit, CWnd)

CSciEdit::CSciEdit()
    : m_directFunction(NULL)
    , m_directPointer(NULL)
    , m_spellCodepage(0)
    , m_separator(' ')
    , m_typeSeparator('?')
    , m_bDoStyle(false)
    , m_nAutoCompleteMinChars(3)
    , m_spellCheckerFactory(nullptr)
    , m_spellChecker(nullptr)
    , m_spellingCache(2000)
    , m_blockModifiedHandler(false)
    , m_bReadOnly(false)
    , m_themeCallbackId(0)
{
    m_hModule = ::LoadLibrary(L"SciLexer.DLL");
}

CSciEdit::~CSciEdit()
{
    CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);
    m_personalDict.Save();
}

static std::unique_ptr<UINT[]> Icon2Image(HICON hIcon)
{
    if (hIcon == nullptr)
        return nullptr;

    ICONINFO iconInfo;
    if (!GetIconInfo(hIcon, &iconInfo))
        return nullptr;

    BITMAP bm;
    if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bm))
        return nullptr;

    int        width            = bm.bmWidth;
    int        height           = bm.bmHeight;
    int        bytesPerScanLine = (width * 3 + 3) & 0xFFFFFFFC;
    int        size             = bytesPerScanLine * height;
    BITMAPINFO infoHeader;
    infoHeader.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    infoHeader.bmiHeader.biWidth       = width;
    infoHeader.bmiHeader.biHeight      = height;
    infoHeader.bmiHeader.biPlanes      = 1;
    infoHeader.bmiHeader.biBitCount    = 24;
    infoHeader.bmiHeader.biCompression = BI_RGB;
    infoHeader.bmiHeader.biSizeImage   = size;

    auto   ptrb          = std::make_unique<BYTE[]>(size * 2LL + height * width * 4LL);
    LPBYTE pixelsIconRGB = ptrb.get();
    LPBYTE alphaPixels   = pixelsIconRGB + size;
    HDC    hDC           = CreateCompatibleDC(nullptr);
    OnOutOfScope(DeleteDC(hDC));
    HBITMAP hBmpOld = static_cast<HBITMAP>(SelectObject(hDC, static_cast<HGDIOBJ>(iconInfo.hbmColor)));
    if (!GetDIBits(hDC, iconInfo.hbmColor, 0, height, static_cast<LPVOID>(pixelsIconRGB), &infoHeader, DIB_RGB_COLORS))
        return nullptr;

    SelectObject(hDC, hBmpOld);
    if (!GetDIBits(hDC, iconInfo.hbmMask, 0, height, static_cast<LPVOID>(alphaPixels), &infoHeader, DIB_RGB_COLORS))
        return nullptr;

    auto imagePixels = std::make_unique<UINT[]>(height * width);
    int  lsSrc       = width * 3;
    int  vsDest      = height - 1;
    for (int y = 0; y < height; y++)
    {
        int linePosSrc  = (vsDest - y) * lsSrc;
        int linePosDest = y * width;
        for (int x = 0; x < width; x++)
        {
            int currentDestPos          = linePosDest + x;
            int currentSrcPos           = linePosSrc + x * 3;
            imagePixels[currentDestPos] = (static_cast<UINT>((
                                                                 ((pixelsIconRGB[currentSrcPos + 2] /*Red*/) | (pixelsIconRGB[currentSrcPos + 1] << 8 /*Green*/)) | pixelsIconRGB[currentSrcPos] << 16 /*Blue*/
                                                                 ) |
                                                             ((alphaPixels[currentSrcPos] ? 0 : 0xff) << 24)) &
                                           0xffffffff);
        }
    }
    return imagePixels;
}

void CSciEdit::Init(LONG lLanguage)
{
    //Setup the direct access data
    m_directFunction = SendMessage(SCI_GETDIRECTFUNCTION, 0, 0);
    m_directPointer  = SendMessage(SCI_GETDIRECTPOINTER, 0, 0);
    Call(SCI_SETMARGINWIDTHN, 1, 0);
    Call(SCI_SETUSETABS, 0); //pressing TAB inserts spaces
    Call(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_END);
    Call(SCI_AUTOCSETIGNORECASE, 1);
    Call(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(nullptr));
    Call(SCI_SETCODEPAGE, SC_CP_UTF8);
    Call(SCI_AUTOCSETFILLUPS, 0, reinterpret_cast<LPARAM>("\t(["));
    Call(SCI_AUTOCSETMAXWIDTH, 0);
    //Set the default windows colors for edit controls
    Call(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
    Call(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
    Call(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));
    Call(SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);
    Call(SCI_INDICSETSTYLE, INDIC_MISSPELLED, INDIC_SQUIGGLE);
    Call(SCI_INDICSETFORE, INDIC_MISSPELLED, RGB(255, 0, 0));
    CStringA sWordChars;
    CStringA sWhiteSpace;
    for (int i = 0; i < 255; ++i)
    {
        if (i == '\r' || i == '\n')
            continue;
        else if (i < 0x20 || i == ' ')
            sWhiteSpace += static_cast<char>(i);
        else if (isalnum(i) || i == '\'' || i == '_' || i == '-')
            sWordChars += static_cast<char>(i);
    }
    Call(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sWordChars)));
    Call(SCI_SETWHITESPACECHARS, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sWhiteSpace)));
    m_bDoStyle              = static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\StyleCommitMessages", TRUE)) == TRUE;
    m_nAutoCompleteMinChars = static_cast<int>(static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\AutoCompleteMinChars", 3)));
    // look for dictionary files and use them if found
    long langId     = GetUserDefaultLCID();
    long origLangId = langId;
    if (lLanguage > 0)
    {
        // if a specific language is requested, then use that
        langId     = lLanguage;
        origLangId = lLanguage;
    }
    if (lLanguage >= 0)
    {
        if (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\Spellchecker", FALSE)) == FALSE)
        {
            // first try the Win8 spell checker
            BOOL    supported     = FALSE;
            HRESULT hr            = CoCreateInstance(__uuidof(SpellCheckerFactory), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_spellCheckerFactory));
            bool    bFallbackUsed = false;
            if (SUCCEEDED(hr))
            {
                wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {0};
                do
                {
                    LCIDToLocaleName(langId, localeName, _countof(localeName), 0);
                    supported = FALSE;
                    hr        = m_spellCheckerFactory->IsSupported(localeName, &supported);
                    if (supported)
                    {
                        hr = m_spellCheckerFactory->CreateSpellChecker(localeName, &m_spellChecker);
                        if (SUCCEEDED(hr))
                        {
                            m_personalDict.Init(langId);
                            break;
                        }
                    }
                    DWORD lid = SUBLANGID(langId);
                    lid--;
                    if (lid > 0)
                    {
                        langId = MAKELANGID(PRIMARYLANGID(langId), lid);
                    }
                    else if (langId == 1033)
                        langId = 0;
                    else
                    {
                        langId        = 1033;
                        bFallbackUsed = true;
                    }
                } while ((langId) && (!supported || FAILED(hr)));
            }
            if (FAILED(hr) || !supported || bFallbackUsed)
            {
                if (bFallbackUsed)
                    langId = origLangId;
                if ((lLanguage == 0) || (lLanguage && !LoadDictionaries(lLanguage)))
                {
                    do
                    {
                        LoadDictionaries(langId);
                        DWORD lid = SUBLANGID(langId);
                        lid--;
                        if (lid > 0)
                        {
                            langId = MAKELANGID(PRIMARYLANGID(langId), lid);
                        }
                        else if (langId == 1033)
                            langId = 0;
                        else
                        {
                            if (bFallbackUsed && supported)
                                langId = 0;
                            else
                                langId = 1033;
                        }
                    } while ((langId) && (!m_pChecker));
                }
                if (bFallbackUsed && m_pChecker)
                    m_spellChecker = nullptr;
            }
        }
    }
    Call(SCI_SETEDGEMODE, EDGE_NONE);
    Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
    Call(SCI_ASSIGNCMDKEY, SCK_END, SCI_LINEENDWRAP);
    Call(SCI_ASSIGNCMDKEY, SCK_END + (SCMOD_SHIFT << 16), SCI_LINEENDWRAPEXTEND);
    Call(SCI_ASSIGNCMDKEY, SCK_HOME, SCI_HOMEWRAP);
    Call(SCI_ASSIGNCMDKEY, SCK_HOME + (SCMOD_SHIFT << 16), SCI_HOMEWRAPEXTEND);

    CRegStdDWORD used2d(L"Software\\TortoiseSVN\\ScintillaDirect2D", TRUE);
    if (static_cast<DWORD>(used2d))
    {
        // set font quality for the popup window, since that window does not use D2D
        Call(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
        // now enable D2D
        Call(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITERETAIN);
        Call(SCI_SETBUFFEREDDRAW, 0);
        CRegStdDWORD useBiDi(L"Software\\TortoiseSVN\\ScintillaBidirectional", FALSE);
        if (static_cast<DWORD>(useBiDi))
        {
            // enable bidirectional mode
            // note: bidirectional mode requires d2d, and to make selections
            // work properly, the SCI_SETSELALPHA needs to be enabled.
            // See here for why:
            // https://groups.google.com/d/msg/scintilla-interest/0EnDpY9Tsbw/ohCgEZqqBwAJ
            // "For now, only translucent background selection works properly in bidirectional mode"
            //
            // note2: bidirectional mode is only required when editing.
            // Scintilla will show bidirectional text just fine even if bidirectional mode is not enabled.
            // And the cost of enabling bidirectional mode is ok since we're dealing here with
            // commit messages, not whole books.
            Call(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_WINDOWTEXT));
            Call(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
            Call(SCI_SETSELALPHA, 128);
            Call(SCI_SETBIDIRECTIONAL, SC_BIDIRECTIONAL_L2R);
        }
        else
        {
            Call(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
            Call(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
        }
    }
    else
    {
        Call(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
        Call(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
    }
    m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
        [this]() {
            OnSysColorChange();
        });
    OnSysColorChange();
}

void CSciEdit::Init(const ProjectProperties& props)
{
    Init(props.lProjectLanguage);
    m_sCommand = CStringA(CUnicodeUtils::GetUTF8(props.GetCheckRe()));
    m_sBugID   = CStringA(CUnicodeUtils::GetUTF8(props.GetBugIDRe()));
    m_sUrl     = CStringA(CUnicodeUtils::GetUTF8(props.sUrl));

    Call(SCI_SETMOUSEDWELLTIME, 333);

    if (props.nLogWidthMarker)
    {
        Call(SCI_SETWRAPMODE, SC_WRAP_NONE);
        Call(SCI_SETEDGEMODE, EDGE_LINE);
        Call(SCI_SETEDGECOLUMN, props.nLogWidthMarker);
        Call(SCI_SETSCROLLWIDTHTRACKING, TRUE);
        Call(SCI_SETSCROLLWIDTH, 1);
    }
    else
    {
        Call(SCI_SETEDGEMODE, EDGE_NONE);
        Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
    }
}

void CSciEdit::SetIcon(const std::map<int, UINT>& icons) const
{
    int iconWidth  = GetSystemMetrics(SM_CXSMICON);
    int iconHeight = GetSystemMetrics(SM_CYSMICON);
    Call(SCI_RGBAIMAGESETWIDTH, iconWidth);
    Call(SCI_RGBAIMAGESETHEIGHT, iconHeight);
    for (auto icon : icons)
    {
        CAutoIcon hIcon = LoadIconEx(AfxGetInstanceHandle(), MAKEINTRESOURCE(icon.second), iconWidth, iconHeight);
        auto      bytes = Icon2Image(hIcon);
        Call(SCI_REGISTERRGBAIMAGE, icon.first, reinterpret_cast<LPARAM>(bytes.get()));
    }
}

BOOL CSciEdit::LoadDictionaries(LONG lLanguageID)
{
    // Setup the spell checker
    wchar_t buf[6]         = {0};
    CString sFolderUp      = CPathUtils::GetAppParentDirectory();
    CString sFolderAppData = CPathUtils::GetAppDataDirectory();

    GetLocaleInfo(MAKELCID(lLanguageID, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf));
    CString sFile = buf;
    sFile += L"_";
    GetLocaleInfo(MAKELCID(lLanguageID, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf));
    sFile += buf;
    if (m_pChecker == nullptr)
    {
        if ((PathFileExists(sFolderAppData + L"dic\\" + sFile + L".aff")) &&
            (PathFileExists(sFolderAppData + L"dic\\" + sFile + L".dic")))
        {
            m_pChecker = std::make_unique<Hunspell>(CStringA(sFolderAppData + L"dic\\" + sFile + L".aff"), CStringA(sFolderAppData + L"dic\\" + sFile + L".dic"));
        }
        else if ((PathFileExists(sFolderUp + L"Languages\\" + sFile + L".aff")) &&
                 (PathFileExists(sFolderUp + L"Languages\\" + sFile + L".dic")))
        {
            m_pChecker = std::make_unique<Hunspell>(CStringA(sFolderUp + L"Languages\\" + sFile + L".aff"), CStringA(sFolderUp + L"Languages\\" + sFile + L".dic"));
        }
        if (m_pChecker)
        {
            const char* encoding = m_pChecker->get_dic_encoding();
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": %s\n", encoding);
            int n           = _countof(enc2Locale);
            m_spellCodepage = 0;
            for (int i = 0; i < n; i++)
            {
                if (strcmp(encoding, enc2Locale[i].defEnc) == 0)
                {
                    m_spellCodepage = atoi(enc2Locale[i].cp);
                }
            }
            m_personalDict.Init(lLanguageID);
        }
    }
    if (m_pChecker)
        return TRUE;
    return FALSE;
}

LRESULT CSciEdit::Call(UINT message, WPARAM wParam, LPARAM lParam) const
{
    ASSERT(::IsWindow(m_hWnd)); //Window must be valid
    ASSERT(m_directFunction);   //Direct function must be valid
    return reinterpret_cast<SciFnDirect>(m_directFunction)(m_directPointer, message, wParam, lParam);
}

CString CSciEdit::StringFromControl(const CStringA& text) const
{
    CString sText;
#ifdef UNICODE
    int codepage = static_cast<int>(Call(SCI_GETCODEPAGE));
    int resLen   = MultiByteToWideChar(codepage, 0, text, text.GetLength(), nullptr, 0);
    MultiByteToWideChar(codepage, 0, text, text.GetLength(), sText.GetBuffer(resLen + 1), resLen + 1);
    sText.ReleaseBuffer(resLen);
#else
    sText = text;
#endif
    return sText;
}

CStringA CSciEdit::StringForControl(const CString& text) const
{
    CStringA sTextA;
#ifdef UNICODE
    int codepage = static_cast<int>(Call(SCI_GETCODEPAGE));
    int resLen   = WideCharToMultiByte(codepage, 0, text, text.GetLength(), nullptr, 0, nullptr, nullptr);
    WideCharToMultiByte(codepage, 0, text, text.GetLength(), sTextA.GetBuffer(resLen), resLen, nullptr, nullptr);
    sTextA.ReleaseBuffer(resLen);
#else
    sTextA = text;
#endif
    ATLTRACE("string length %d\n", sTextA.GetLength());
    return sTextA;
}

void CSciEdit::SetText(const CString& sText)
{
    auto readonly = m_bReadOnly;
    OnOutOfScope(SetReadOnly(readonly));
    SetReadOnly(false);

    CStringA sTextA = StringForControl(sText);
    Call(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));

    if (Call(SCI_GETSCROLLWIDTHTRACKING) != 0)
        Call(SCI_SETSCROLLWIDTH, 1);

    // Scintilla seems to have problems with strings that
    // aren't terminated by a newline char. Once that char
    // is there, it can be removed without problems.
    // So we add here a newline, then remove it again.
    Call(SCI_DOCUMENTEND);
    Call(SCI_NEWLINE);
    Call(SCI_DELETEBACK);
}

void CSciEdit::InsertText(const CString& sText, bool bNewLine)
{
    auto readonly = m_bReadOnly;
    OnOutOfScope(SetReadOnly(readonly));
    SetReadOnly(false);

    CStringA sTextA = StringForControl(sText);
    Call(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));
    if (bNewLine)
        Call(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>("\n")));
}

CString CSciEdit::GetText() const
{
    LRESULT  len = Call(SCI_GETTEXT, 0, 0);
    CStringA sTextA;
    Call(SCI_GETTEXT, static_cast<WPARAM>(len + 1), reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA.GetBuffer(static_cast<int>(len) + 1))));
    sTextA.ReleaseBuffer();
    return StringFromControl(sTextA);
}

CString CSciEdit::GetWordUnderCursor(bool bSelectWord, bool allchars) const
{
    Sci_TextRange textRange;
    int           pos    = static_cast<int>(Call(SCI_GETCURRENTPOS));
    textRange.chrg.cpMin = static_cast<LONG>(Call(SCI_WORDSTARTPOSITION, pos, TRUE));
    if ((pos == textRange.chrg.cpMin) || (textRange.chrg.cpMin < 0))
        return CString();
    textRange.chrg.cpMax = static_cast<LONG>(Call(SCI_WORDENDPOSITION, textRange.chrg.cpMin, TRUE));

    auto textBuffer     = std::make_unique<char[]>(textRange.chrg.cpMax - textRange.chrg.cpMin + 1);
    textRange.lpstrText = textBuffer.get();
    Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
    CString sRet = StringFromControl(textBuffer.get());
    if (m_bDoStyle && !allchars)
    {
        for (const auto styleindicator : {'*', '_', '^'})
        {
            if (sRet.IsEmpty())
                break;
            if (sRet[sRet.GetLength() - 1] == styleindicator)
            {
                --textRange.chrg.cpMax;
                sRet.Truncate(sRet.GetLength() - 1);
            }
            if (sRet.IsEmpty())
                break;
            if (sRet[0] == styleindicator)
            {
                ++textRange.chrg.cpMin;
                sRet = sRet.Right(sRet.GetLength() - 1);
            }
        }
    }
    if (bSelectWord)
        Call(SCI_SETSEL, textRange.chrg.cpMin, textRange.chrg.cpMax);
    return sRet;
}

void CSciEdit::SetFont(CString sFontName, int iFontSizeInPoints) const
{
    CStringA fontName = CUnicodeUtils::GetUTF8(sFontName);
    Call(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(fontName)));
    Call(SCI_STYLESETSIZE, STYLE_DEFAULT, iFontSizeInPoints);
    Call(SCI_STYLECLEARALL);

    LPARAM color = static_cast<LPARAM>(GetSysColor(COLOR_HIGHLIGHT));
    // set the styles for the bug ID strings
    Call(SCI_STYLESETBOLD, STYLE_ISSUEBOLD, static_cast<LPARAM>(TRUE));
    Call(SCI_STYLESETFORE, STYLE_ISSUEBOLD, color);
    Call(SCI_STYLESETBOLD, STYLE_ISSUEBOLDITALIC, static_cast<LPARAM>(TRUE));
    Call(SCI_STYLESETITALIC, STYLE_ISSUEBOLDITALIC, static_cast<LPARAM>(TRUE));
    Call(SCI_STYLESETFORE, STYLE_ISSUEBOLDITALIC, color);
    Call(SCI_STYLESETHOTSPOT, STYLE_ISSUEBOLDITALIC, static_cast<LPARAM>(TRUE));

    // set the formatted text styles
    Call(SCI_STYLESETBOLD, STYLE_BOLD, static_cast<LPARAM>(TRUE));
    Call(SCI_STYLESETITALIC, STYLE_ITALIC, static_cast<LPARAM>(TRUE));
    Call(SCI_STYLESETUNDERLINE, STYLE_UNDERLINED, static_cast<LPARAM>(TRUE));

    // set the style for URLs
    Call(SCI_STYLESETFORE, STYLE_URL, color);
    Call(SCI_STYLESETHOTSPOT, STYLE_URL, static_cast<LPARAM>(TRUE));

    Call(SCI_SETHOTSPOTACTIVEUNDERLINE, static_cast<LPARAM>(TRUE));
}

void CSciEdit::SetAutoCompletionList(std::map<CString, int>&& list, wchar_t separator, wchar_t typeSeparator)
{
    //copy the auto completion list.

    m_autolist.clear();
    m_autolist      = std::move(list);
    m_separator     = separator;
    m_typeSeparator = typeSeparator;
}

// Helper for CSciEdit::IsMisspelled()
// Returns TRUE if sWord has spelling errors.
BOOL CSciEdit::CheckWordSpelling(const CString& sWord) const
{
    if (m_bReadOnly)
        return FALSE;
    if (m_spellChecker)
    {
        IEnumSpellingErrorPtr enumSpellingError = nullptr;
        if (SUCCEEDED(m_spellChecker->Check(sWord, &enumSpellingError)))
        {
            ISpellingErrorPtr spellingError = nullptr;
            if (enumSpellingError->Next(&spellingError) == S_OK)
            {
                CORRECTIVE_ACTION action = CORRECTIVE_ACTION_NONE;
                spellingError->get_CorrectiveAction(&action);
                if (action != CORRECTIVE_ACTION_NONE)
                {
                    return TRUE;
                }
            }
        }
    }
    else if (m_pChecker)
    {
        // convert the string from the control to the encoding of the spell checker module.
        auto sWordA = GetWordForSpellChecker(sWord);

        if (!m_pChecker->spell(sWordA))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CSciEdit::IsMisspelled(const CString& sWord)
{
    // words starting with a digit are treated as correctly spelled
    if (_istdigit(sWord.GetAt(0)))
        return FALSE;
    // words in the personal dictionary are correct too
    if (m_personalDict.FindWord(sWord))
        return FALSE;

    // Check spell checking cache first.
    const BOOL* cacheResult = m_spellingCache.try_get(std::wstring(sWord, sWord.GetLength()));
    if (cacheResult)
        return *cacheResult;

    // now we actually check the spelling...
    BOOL misspelled = CheckWordSpelling(sWord);

    if (misspelled)
    {
        // the word is marked as misspelled, we now check whether the word
        // is maybe a composite identifier
        // a composite identifier consists of multiple words, with each word
        // separated by a change in lower to uppercase letters
        misspelled = FALSE;
        if (sWord.GetLength() > 1)
        {
            int wordStart = 0;
            int wordEnd   = 1;
            while (wordEnd < sWord.GetLength())
            {
                while ((wordEnd < sWord.GetLength()) && (!_istupper(sWord[wordEnd])))
                    wordEnd++;
                if ((wordStart == 0) && (wordEnd == sWord.GetLength()))
                {
                    // words in the auto list are also assumed correctly spelled
                    if (m_autolist.find(sWord) != m_autolist.end())
                    {
                        misspelled = FALSE;
                        break;
                    }

                    misspelled = TRUE;
                    break;
                }

                CString token(sWord.Mid(wordStart, wordEnd - wordStart));
                if (token.GetLength() > 2 && CheckWordSpelling(token))
                {
                    misspelled = TRUE;
                    break;
                }

                wordStart = wordEnd;
                wordEnd++;
            }
        }
    }

    // Update cache.
    m_spellingCache.insert_or_assign(std::wstring(sWord, sWord.GetLength()), misspelled);

    return misspelled;
}

void CSciEdit::CheckSpelling(Sci_Position startpos, Sci_Position endpos)
{
    if ((m_pChecker == nullptr) && (m_spellChecker == nullptr))
        return;
    if (m_bReadOnly)
        return;

    Sci_TextRange textRange;

    textRange.chrg.cpMin = static_cast<Sci_PositionCR>(startpos);
    textRange.chrg.cpMax = textRange.chrg.cpMin;
    LRESULT lastPos      = endpos;
    if (lastPos < 0)
        lastPos = Call(SCI_GETLENGTH) - textRange.chrg.cpMin;
    Call(SCI_SETINDICATORCURRENT, INDIC_MISSPELLED);
    while (textRange.chrg.cpMax < lastPos)
    {
        textRange.chrg.cpMin = static_cast<LONG>(Call(SCI_WORDSTARTPOSITION, textRange.chrg.cpMax + 1LL, TRUE));
        if (textRange.chrg.cpMin < textRange.chrg.cpMax)
            break;
        textRange.chrg.cpMax = static_cast<LONG>(Call(SCI_WORDENDPOSITION, textRange.chrg.cpMin, TRUE));
        if (textRange.chrg.cpMin == textRange.chrg.cpMax)
        {
            textRange.chrg.cpMax++;
            // since Scintilla squiggles to the end of the text even if told to stop one char before it,
            // we have to clear here the squiggly lines to the end.
            if (textRange.chrg.cpMin)
                Call(SCI_INDICATORCLEARRANGE, textRange.chrg.cpMin - 1, textRange.chrg.cpMax - textRange.chrg.cpMin + 1);
            continue;
        }
        ATLASSERT(textRange.chrg.cpMax >= textRange.chrg.cpMin);
        auto textbuffer = std::make_unique<char[]>(textRange.chrg.cpMax - textRange.chrg.cpMin + 2);
        SecureZeroMemory(textbuffer.get(), textRange.chrg.cpMax - textRange.chrg.cpMin + 2);
        textRange.lpstrText = textbuffer.get();
        textRange.chrg.cpMax++;
        Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
        int len = static_cast<int>(strlen(textRange.lpstrText));
        if (len == 0)
        {
            textRange.chrg.cpMax--;
            Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
            len = static_cast<int>(strlen(textRange.lpstrText));
            textRange.chrg.cpMax++;
            len++;
        }
        if (len && textRange.lpstrText[len - 1] == '.')
        {
            // Try to ignore file names from the auto list.
            // Do do this, for each word ending with '.' we extract next word and check
            // whether the combined string is present in auto list.
            Sci_TextRange twoWords;
            twoWords.chrg.cpMin = textRange.chrg.cpMin;
            twoWords.chrg.cpMax = static_cast<LONG>(Call(SCI_WORDENDPOSITION, textRange.chrg.cpMax + 1, TRUE));
            auto twoWordsBuffer = std::make_unique<char[]>(twoWords.chrg.cpMax - twoWords.chrg.cpMin + 1LL);
            twoWords.lpstrText  = twoWordsBuffer.get();
            SecureZeroMemory(twoWords.lpstrText, twoWords.chrg.cpMax - twoWords.chrg.cpMin + 1LL);
            Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&twoWords));
            CString sWord = StringFromControl(twoWords.lpstrText);
            if (m_autolist.find(sWord) != m_autolist.end())
            {
                //mark word as correct (remove the squiggle line)
                Call(SCI_INDICATORCLEARRANGE, twoWords.chrg.cpMin, twoWords.chrg.cpMax - twoWords.chrg.cpMin);
                textRange.chrg.cpMax = twoWords.chrg.cpMax;
                continue;
            }
        }
        if (len)
            textRange.lpstrText[len - 1] = 0;
        textRange.chrg.cpMax--;
        if (textRange.lpstrText[0] != 0)
        {
            CString sWord = StringFromControl(textRange.lpstrText);
            if ((GetStyleAt(textRange.chrg.cpMin) != STYLE_URL) && IsMisspelled(sWord))
            {
                //mark word as misspelled
                Call(SCI_INDICATORFILLRANGE, textRange.chrg.cpMin, textRange.chrg.cpMax - textRange.chrg.cpMin);
            }
            else
            {
                //mark word as correct (remove the squiggle line)
                Call(SCI_INDICATORCLEARRANGE, textRange.chrg.cpMin, textRange.chrg.cpMax - textRange.chrg.cpMin);
                Call(SCI_INDICATORCLEARRANGE, textRange.chrg.cpMin, textRange.chrg.cpMax - textRange.chrg.cpMin + 1);
            }
        }
    }
}

void CSciEdit::SuggestSpellingAlternatives() const
{
    if ((m_pChecker == nullptr) && (m_spellChecker == nullptr))
        return;
    CString word = GetWordUnderCursor(true);
    Call(SCI_SETCURRENTPOS, Call(SCI_WORDSTARTPOSITION, Call(SCI_GETCURRENTPOS), TRUE));
    if (word.IsEmpty())
        return;

    CString suggestions;
    if (m_spellChecker)
    {
        IEnumStringPtr enumSpellingSuggestions = nullptr;
        if (SUCCEEDED(m_spellChecker->Suggest(word, &enumSpellingSuggestions)))
        {
            LPOLESTR string = nullptr;
            while (enumSpellingSuggestions->Next(1, &string, nullptr) == S_OK)
            {
                suggestions.AppendFormat(L"%s%c%d%c", static_cast<LPCWSTR>(CString(string)), m_typeSeparator, AUTOCOMPLETE_SPELLING, m_separator);
                CoTaskMemFree(string);
            }
        }
    }
    else if (m_pChecker)
    {
        auto wlst = m_pChecker->suggest(GetWordForSpellChecker(word));
        for (const auto& alternative : wlst)
            suggestions.AppendFormat(L"%s%c%d%c", static_cast<LPCWSTR>(GetWordFromSpellChecker(alternative)), m_typeSeparator, AUTOCOMPLETE_SPELLING, m_separator);
    }

    suggestions.TrimRight(m_separator);
    if (suggestions.IsEmpty())
        return;
    Call(SCI_AUTOCSETSEPARATOR, static_cast<WPARAM>(CStringA(m_separator).GetAt(0)));
    Call(SCI_AUTOCSETTYPESEPARATOR, static_cast<WPARAM>(m_typeSeparator));
    Call(SCI_AUTOCSETDROPRESTOFWORD, 1);
    Call(SCI_AUTOCSHOW, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(StringForControl(suggestions))));
    SetWindowStylesForAutocompletionPopup();
}

void CSciEdit::DoAutoCompletion(Sci_Position nMinPrefixLength)
{
    if (m_autolist.empty())
        return;
    int pos = static_cast<int>(Call(SCI_GETCURRENTPOS));
    if (pos != Call(SCI_WORDENDPOSITION, pos, TRUE))
        return; // don't auto complete if we're not at the end of a word
    CString word = GetWordUnderCursor();
    if (word.GetLength() < nMinPrefixLength)
    {
        word = GetWordUnderCursor(false, true);
        if (word.GetLength() < nMinPrefixLength)
            return; // don't auto complete yet, word is too short
    }
    CString sAutoCompleteList;

    for (int i = 0; i < 2; ++i)
    {
        std::vector<CString> words;

        pos = word.Find('-');

        CString wordLower = word;
        wordLower.MakeLower();
        CString wordHigher = word;
        wordHigher.MakeUpper();

        words.push_back(word);
        words.push_back(wordLower);
        words.push_back(wordHigher);

        if (pos >= 0)
        {
            CString s = wordLower.Left(pos);
            if (s.GetLength() >= nMinPrefixLength)
                words.push_back(s);
            s = wordLower.Mid(pos + 1);
            if (s.GetLength() >= nMinPrefixLength)
                words.push_back(s);
            s = wordHigher.Left(pos);
            if (s.GetLength() >= nMinPrefixLength)
                words.push_back(wordHigher.Left(pos));
            s = wordHigher.Mid(pos + 1);
            if (s.GetLength() >= nMinPrefixLength)
                words.push_back(wordHigher.Mid(pos + 1));
        }

        // note: the m_autolist is case-sensitive because
        // its contents are also used to mark words in it
        // as correctly spelled. If it would be case-insensitive,
        // case spelling mistakes would not show up as misspelled.
        std::map<CString, int> wordSet;
        for (const auto& w : words)
        {
            for (auto lowerIt = m_autolist.lower_bound(w);
                 lowerIt != m_autolist.end(); ++lowerIt)
            {
                int compare = w.CompareNoCase(lowerIt->first.Left(w.GetLength()));
                if (compare > 0)
                    continue;
                else if (compare == 0)
                {
                    wordSet.emplace(lowerIt->first, lowerIt->second);
                }
                else
                {
                    break;
                }
            }
        }

        for (const auto& w : wordSet)
            sAutoCompleteList.AppendFormat(L"%s%c%d%c", static_cast<LPCWSTR>(w.first), m_typeSeparator, w.second, m_separator);

        sAutoCompleteList.TrimRight(m_separator);

        if (i == 0)
        {
            if (sAutoCompleteList.IsEmpty())
            {
                // retry with all chars
                word = GetWordUnderCursor(false, true);
            }
            else
                break;
        }
        if (i == 1)
        {
            if (sAutoCompleteList.IsEmpty())
                return;
        }
    }

    Call(SCI_AUTOCSETSEPARATOR, static_cast<WPARAM>(CStringA(m_separator).GetAt(0)));
    Call(SCI_AUTOCSETTYPESEPARATOR, static_cast<WPARAM>(m_typeSeparator));
    auto sForControl = StringForControl(sAutoCompleteList);
    Call(SCI_AUTOCSHOW, StringForControl(word).GetLength(), reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sForControl)));
    SetWindowStylesForAutocompletionPopup();
}

BOOL CSciEdit::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
    if (message != WM_NOTIFY)
        return CWnd::OnChildNotify(message, wParam, lParam, pLResult);

    LPNMHDR         lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
    SCNotification* lpScn   = reinterpret_cast<SCNotification*>(lParam);

    if (lpnmhdr->hwndFrom == m_hWnd)
    {
        switch (lpnmhdr->code)
        {
            case SCN_CHARADDED:
            {
                if ((lpScn->ch < 32) && (lpScn->ch != 13) && (lpScn->ch != 10))
                    Call(SCI_DELETEBACK);
                else
                {
                    DoAutoCompletion(m_nAutoCompleteMinChars);
                }
                return TRUE;
            }
            case SCN_AUTOCSELECTION:
            {
                CString text = StringFromControl(lpScn->text);
                if (m_autolist[text] == AUTOCOMPLETE_SNIPPET)
                {
                    Call(SCI_AUTOCCANCEL);
                    for (INT_PTR handlerIndex = 0; handlerIndex < m_arContextHandlers.GetCount(); ++handlerIndex)
                    {
                        CSciEditContextMenuInterface* pHandler = m_arContextHandlers.GetAt(handlerIndex);
                        pHandler->HandleSnippet(m_autolist[text], text, this);
                    }
                }
                return TRUE;
            }
            case SCN_STYLENEEDED:
            {
                Sci_Position startPos = static_cast<Sci_Position>(Call(SCI_GETENDSTYLED));
                Sci_Position endPos   = reinterpret_cast<SCNotification*>(lpnmhdr)->position;

                Sci_Position startWordPos = static_cast<Sci_Position>(Call(SCI_WORDSTARTPOSITION, startPos, true));
                Sci_Position endWordPos   = static_cast<Sci_Position>(Call(SCI_WORDENDPOSITION, endPos, true));

                MarkEnteredBugID(startWordPos, endWordPos);
                if (m_bDoStyle)
                    StyleEnteredText(startWordPos, endWordPos);

                StyleURLs(startWordPos, endWordPos);
                CheckSpelling(startWordPos, endWordPos);

                // Tell scintilla editor that we styled all requested range.
                Call(SCI_STARTSTYLING, endWordPos);
                Call(SCI_SETSTYLING, 0, 0);
            }
            break;
            case SCN_MODIFIED:
            {
                if (!m_blockModifiedHandler && (lpScn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)))
                {
                    LRESULT      firstLine = Call(SCI_GETFIRSTVISIBLELINE);
                    LRESULT      lastLine  = firstLine + Call(SCI_LINESONSCREEN);
                    Sci_Position firstPos  = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, firstLine));
                    Sci_Position lastPos   = static_cast<Sci_Position>(Call(SCI_GETLINEENDPOSITION, lastLine));
                    Sci_Position pos1      = lpScn->position;
                    Sci_Position pos2      = pos1 + lpScn->length;
                    // always use the bigger range
                    firstPos = min(firstPos, pos1);
                    lastPos  = max(lastPos, pos2);

                    WrapLines(firstPos, lastPos);
                }
            }
            break;
            case SCN_DWELLSTART:
            case SCN_HOTSPOTRELEASECLICK:
            {
                if (lpScn->position < 0)
                    break;

                Sci_TextRange textRange;
                textRange.chrg.cpMin = static_cast<Sci_PositionCR>(lpScn->position);
                textRange.chrg.cpMax = static_cast<Sci_PositionCR>(lpScn->position);
                DWORD style          = GetStyleAt(lpScn->position);
                if (style == 0)
                    break;
                while (GetStyleAt(textRange.chrg.cpMin - 1) == style)
                    --textRange.chrg.cpMin;
                while (GetStyleAt(textRange.chrg.cpMax + 1) == style)
                    ++textRange.chrg.cpMax;
                ++textRange.chrg.cpMax;
                auto textbuffer     = std::make_unique<char[]>(textRange.chrg.cpMax - textRange.chrg.cpMin + 1);
                textRange.lpstrText = textbuffer.get();
                Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
                CString url;
                if (style == STYLE_URL)
                {
                    url = StringFromControl(textbuffer.get());
                    if (url.Find('@') > 0 && !PathIsURL(url))
                        url = L"mailto:" + url;
                }
                else
                {
                    url = m_sUrl;
                    ProjectProperties::ReplaceBugIDPlaceholder(url, StringFromControl(textbuffer.get()));

                    // is the URL a relative one?
                    if (url.Left(2).Compare(L"^/") == 0)
                    {
                        // URL is relative to the repository root
                        CString url1                         = m_sRepositoryRoot + url.Mid(1);
                        wchar_t buf[INTERNET_MAX_URL_LENGTH] = {0};
                        DWORD   len                          = url.GetLength();
                        if (UrlCanonicalize(static_cast<LPCWSTR>(url1), buf, &len, 0) == S_OK)
                            url = CString(buf, len);
                        else
                            url = url1;
                    }
                    else if (url[0] == '/')
                    {
                        // URL is relative to the server's hostname
                        CString sHost;
                        // find the server's hostname
                        int schemePos = m_sRepositoryRoot.Find(L"//");
                        if (schemePos >= 0)
                        {
                            sHost                                = m_sRepositoryRoot.Left(m_sRepositoryRoot.Find('/', schemePos + 3));
                            CString url1                         = sHost + url;
                            wchar_t buf[INTERNET_MAX_URL_LENGTH] = {0};
                            DWORD   len                          = url.GetLength();
                            if (UrlCanonicalize(static_cast<LPCWSTR>(url), buf, &len, 0) == S_OK)
                                url = CString(buf, len);
                            else
                                url = url1;
                        }
                    }
                }
                if ((lpnmhdr->code == SCN_HOTSPOTRELEASECLICK) && (!url.IsEmpty()))
                    ShellExecute(GetParent()->GetSafeHwnd(), L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
                else
                {
                    CStringA sTextA = StringForControl(url);
                    Call(SCI_CALLTIPSHOW, lpScn->position + 3, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));
                }
            }
            break;
            case SCN_DWELLEND:
                Call(SCI_CALLTIPCANCEL);
                break;
        }
    }
    return CWnd::OnChildNotify(message, wParam, lParam, pLResult);
}

BEGIN_MESSAGE_MAP(CSciEdit, CWnd)
    ON_WM_KEYDOWN()
    ON_WM_CONTEXTMENU()
    ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

void CSciEdit::OnSysColorChange()
{
    __super::OnSysColorChange();
    Call(SCI_CLEARDOCUMENTSTYLE);
    if (CTheme::Instance().IsDarkTheme())
    {
        SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));
        Call(SCI_STYLESETFORE, STYLE_DEFAULT, CTheme::darkTextColor);
        Call(SCI_STYLESETBACK, STYLE_DEFAULT, CTheme::darkBkColor);
        Call(SCI_SETCARETFORE, CTheme::darkTextColor);

        Call(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST, RGB(187, 187, 187));
        Call(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_BACK, RGB(15, 15, 15));
        Call(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED, RGB(187, 187, 187));
        Call(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED_BACK, RGB(80, 80, 80));
    }
    else
    {
        SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
        Call(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
        Call(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
        Call(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));

        Call(SCI_RESETELEMENTCOLOUR, SC_ELEMENT_LIST);
        Call(SCI_RESETELEMENTCOLOUR, SC_ELEMENT_LIST_BACK);
        Call(SCI_RESETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED);
        Call(SCI_RESETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED_BACK);
    }
    Call(SCI_SETSELFORE, TRUE, CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_HIGHLIGHTTEXT)));
    Call(SCI_SETSELBACK, TRUE, CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_HIGHLIGHT)));
}

void CSciEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar)
    {
        case (VK_ESCAPE):
        {
            // escape key is already handled in PreTranslateMessage to prevent
            // it from getting to the main dialog. This code here is only
            // in case it does not automatically go to the parent, which
            // it sometimes does not.
            if ((Call(SCI_AUTOCACTIVE) == 0) && (Call(SCI_CALLTIPACTIVE) == 0))
                ::SendMessage(GetParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
        }
        break;
    }
    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CSciEdit::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case VK_ESCAPE:
            {
                // this shouldn't be necessary: Scintilla should
                // consume the escape key itself and prevent it
                // from being sent to the parent, but it sometimes
                // still sends it. So this is to make sure the escape
                // keypress is not ever sent to the parent dialog
                // if a tool- or calltip is active.
                if (Call(SCI_AUTOCACTIVE))
                {
                    Call(SCI_AUTOCCANCEL);
                    return TRUE;
                }
                if (Call(SCI_CALLTIPACTIVE))
                {
                    Call(SCI_CALLTIPCANCEL);
                    return TRUE;
                }
            }
            break;
            case VK_SPACE:
            {
                if ((GetKeyState(VK_CONTROL) & 0x8000) && ((GetKeyState(VK_MENU) & 0x8000) == 0))
                {
                    DoAutoCompletion(1);
                    return TRUE;
                }
            }
            break;
            case VK_TAB:
                // The TAB cannot be handled in OnKeyDown because it is too late by then.
                {
                    if ((GetKeyState(VK_CONTROL) & 0x8000) && ((GetKeyState(VK_MENU) & 0x8000) == 0))
                    {
                        //Ctrl-Tab was pressed, this means we should provide the user with
                        //a list of possible spell checking alternatives to the word under
                        //the cursor
                        SuggestSpellingAlternatives();
                        return TRUE;
                    }
                    else if (!Call(SCI_AUTOCACTIVE))
                    {
                        ::PostMessage(GetParent()->GetSafeHwnd(), WM_NEXTDLGCTL, GetKeyState(VK_SHIFT) & 0x8000, 0);
                        return TRUE;
                    }
                }
                break;
        }
    }
    return CWnd::PreTranslateMessage(pMsg);
}

ULONG CSciEdit::GetGestureStatus(CPoint /*ptTouch*/)
{
    return 0;
}

void CSciEdit::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    int anchor     = static_cast<int>(Call(SCI_GETANCHOR));
    int currentPos = static_cast<int>(Call(SCI_GETCURRENTPOS));
    int selStart   = static_cast<int>(Call(SCI_GETSELECTIONSTART));
    int selEnd     = static_cast<int>(Call(SCI_GETSELECTIONEND));
    int pointPos   = 0;
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        GetClientRect(&rect);
        ClientToScreen(&rect);
        point    = rect.CenterPoint();
        pointPos = static_cast<int>(Call(SCI_GETCURRENTPOS));
    }
    else
    {
        // change the cursor position to the point where the user
        // right-clicked.
        CPoint clientPoint = point;
        ScreenToClient(&clientPoint);
        pointPos = static_cast<int>(Call(SCI_POSITIONFROMPOINT, clientPoint.x, clientPoint.y));
    }
    CString sMenuItemText;
    CMenu   popup;
    bool    bRestoreCursor = true;
    if (popup.CreatePopupMenu())
    {
        bool bCanUndo      = !!Call(SCI_CANUNDO);
        bool bCanRedo      = !!Call(SCI_CANREDO);
        bool bHasSelection = (selEnd - selStart > 0);
        bool bCanPaste     = !!Call(SCI_CANPASTE);
        bool bIsReadOnly   = !!Call(SCI_GETREADONLY);
        UINT uEnabledMenu  = MF_STRING | MF_ENABLED;
        UINT uDisabledMenu = MF_STRING | MF_GRAYED;

        // find the word under the cursor
        CString sWord;
        if (pointPos)
        {
            // setting the cursor clears the selection
            Call(SCI_SETANCHOR, pointPos);
            Call(SCI_SETCURRENTPOS, pointPos);
            sWord = GetWordUnderCursor();
            // restore the selection
            Call(SCI_SETSELECTIONSTART, selStart);
            Call(SCI_SETSELECTIONEND, selEnd);
        }
        else
            sWord = GetWordUnderCursor();
        auto worda = GetWordForSpellChecker(sWord);

        int  nCorrections = 1;
        bool bSpellAdded  = false;
        // check if the word under the cursor is spelled wrong
        if (!bIsReadOnly && (m_pChecker || m_spellChecker) && (!worda.empty()))
        {
            if (m_spellChecker)
            {
                IEnumStringPtr enumSpellingSuggestions = nullptr;
                if (SUCCEEDED(m_spellChecker->Suggest(sWord, &enumSpellingSuggestions)))
                {
                    LPOLESTR string = nullptr;
                    while (enumSpellingSuggestions->Next(1, &string, nullptr) == S_OK)
                    {
                        bSpellAdded = true;
                        popup.InsertMenu(static_cast<UINT>(-1), 0, nCorrections++, string);
                        CoTaskMemFree(string);
                    }
                }
            }
            else if (m_pChecker)
            {
                // get the spell suggestions
                auto wlst = m_pChecker->suggest(worda);
                // add the suggestions to the context menu
                for (const auto& alternative : wlst)
                {
                    bSpellAdded = true;
                    CString sug = GetWordFromSpellChecker(alternative);
                    popup.InsertMenu(static_cast<UINT>(-1), 0, nCorrections++, sug);
                }
            }
        }
        // only add a separator if spelling correction suggestions were added
        if (bSpellAdded)
            popup.AppendMenu(MF_SEPARATOR);

        // also allow the user to add the word to the custom dictionary so
        // it won't show up as misspelled anymore
        if (!bIsReadOnly && (sWord.GetLength() < PDICT_MAX_WORD_LENGTH) && ((m_autolist.find(sWord) == m_autolist.end()) && (IsMisspelled(sWord))) &&
            (!_istdigit(sWord.GetAt(0))) && (!m_personalDict.FindWord(sWord)))
        {
            sMenuItemText.Format(IDS_SCIEDIT_ADDWORD, static_cast<LPCWSTR>(sWord));
            popup.AppendMenu(uEnabledMenu, SCI_ADDWORD, sMenuItemText);
            // another separator
            popup.AppendMenu(MF_SEPARATOR);
        }

        // add the 'default' entries
        sMenuItemText.LoadString(IDS_SCIEDIT_UNDO);
        popup.AppendMenu(bCanUndo ? uEnabledMenu : uDisabledMenu, SCI_UNDO, sMenuItemText);
        sMenuItemText.LoadString(IDS_SCIEDIT_REDO);
        popup.AppendMenu(bCanRedo ? uEnabledMenu : uDisabledMenu, SCI_REDO, sMenuItemText);

        popup.AppendMenu(MF_SEPARATOR);

        sMenuItemText.LoadString(IDS_SCIEDIT_CUT);
        popup.AppendMenu(!bIsReadOnly && bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_CUT, sMenuItemText);
        sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
        popup.AppendMenu(bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_COPY, sMenuItemText);
        sMenuItemText.LoadString(IDS_SCIEDIT_PASTE);
        popup.AppendMenu(bCanPaste ? uEnabledMenu : uDisabledMenu, SCI_PASTE, sMenuItemText);

        popup.AppendMenu(MF_SEPARATOR);

        sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
        popup.AppendMenu(uEnabledMenu, SCI_SELECTALL, sMenuItemText);

        popup.AppendMenu(MF_SEPARATOR);

        sMenuItemText.LoadString(IDS_SCIEDIT_SPLITLINES);
        popup.AppendMenu(bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_LINESSPLIT, sMenuItemText);

        if (m_arContextHandlers.GetCount() > 0)
            popup.AppendMenu(MF_SEPARATOR);

        int nCustoms = nCorrections;
        // now add any custom context menus
        for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
        {
            CSciEditContextMenuInterface* pHandler = m_arContextHandlers.GetAt(handlerindex);
            pHandler->InsertMenuItems(popup, nCustoms);
        }
        if (nCustoms > nCorrections)
        {
            // custom menu entries present, so add another separator
            popup.AppendMenu(MF_SEPARATOR);
        }

        int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, nullptr);
        switch (cmd)
        {
            case 0:
                break; // no command selected
            case SCI_SELECTALL:
                bRestoreCursor = false;
                [[fallthrough]];
            case SCI_UNDO:
            case SCI_REDO:
            case SCI_CUT:
            case SCI_COPY:
            case SCI_PASTE:
                Call(cmd);
                break;
            case SCI_ADDWORD:
                m_personalDict.AddWord(sWord);
                CheckSpelling(static_cast<int>(Call(SCI_POSITIONFROMLINE, Call(SCI_GETFIRSTVISIBLELINE))), static_cast<int>(Call(SCI_POSITIONFROMLINE, Call(SCI_GETFIRSTVISIBLELINE) + Call(SCI_LINESONSCREEN))));
                break;
            case SCI_LINESSPLIT:
            {
                int marker = static_cast<int>(Call(SCI_GETEDGECOLUMN) * Call(SCI_TEXTWIDTH, 0, reinterpret_cast<LPARAM>(" ")));
                if (marker)
                {
                    m_blockModifiedHandler = true;
                    OnOutOfScope(m_blockModifiedHandler = false);
                    Call(SCI_TARGETFROMSELECTION);
                    Call(SCI_LINESJOIN);
                    Call(SCI_LINESSPLIT, marker);
                }
            }
            break;
            default:
                if (cmd < nCorrections)
                {
                    Call(SCI_SETANCHOR, pointPos);
                    Call(SCI_SETCURRENTPOS, pointPos);
                    GetWordUnderCursor(true);
                    CString temp;
                    popup.GetMenuString(cmd, temp, 0);
                    // setting the cursor clears the selection
                    Call(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(StringForControl(temp))));
                }
                else if (cmd < (nCorrections + nCustoms))
                {
                    for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
                    {
                        CSciEditContextMenuInterface* pHandler = m_arContextHandlers.GetAt(handlerindex);
                        if (pHandler->HandleMenuItemClick(cmd, this))
                            break;
                    }
                }
        }
    }
    if (bRestoreCursor)
    {
        // restore the anchor and cursor position
        Call(SCI_SETCURRENTPOS, currentPos);
        Call(SCI_SETANCHOR, anchor);
    }
}

bool CSciEdit::StyleEnteredText(Sci_Position startStylePos, Sci_Position endStylePos) const
{
    bool               bStyled       = false;
    const Sci_Position line          = static_cast<Sci_Position>(Call(SCI_LINEFROMPOSITION, startStylePos));
    const Sci_Position lineNumberEnd = static_cast<Sci_Position>(Call(SCI_LINEFROMPOSITION, endStylePos));
    for (auto lineNumber = line; lineNumber <= lineNumberEnd; ++lineNumber)
    {
        Sci_Position offset     = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, lineNumber));
        Sci_Position lineLen    = static_cast<Sci_Position>(Call(SCI_LINELENGTH, lineNumber));
        auto         lineBuffer = std::make_unique<char[]>(lineLen + 1);
        Call(SCI_GETLINE, lineNumber, reinterpret_cast<LPARAM>(lineBuffer.get()));
        lineBuffer[lineLen] = 0;
        Sci_Position start  = 0;
        Sci_Position end    = 0;
        while (FindStyleChars(lineBuffer.get(), '*', start, end))
        {
            Call(SCI_STARTSTYLING, start + offset, STYLE_BOLD);
            Call(SCI_SETSTYLING, end - start, STYLE_BOLD);
            bStyled = true;
            start   = end;
        }
        start = 0;
        end   = 0;
        while (FindStyleChars(lineBuffer.get(), '^', start, end))
        {
            Call(SCI_STARTSTYLING, start + offset, STYLE_ITALIC);
            Call(SCI_SETSTYLING, end - start, STYLE_ITALIC);
            bStyled = true;
            start   = end;
        }
        start = 0;
        end   = 0;
        while (FindStyleChars(lineBuffer.get(), '_', start, end))
        {
            Call(SCI_STARTSTYLING, start + offset, STYLE_UNDERLINED);
            Call(SCI_SETSTYLING, end - start, STYLE_UNDERLINED);
            bStyled = true;
            start   = end;
        }
    }
    return bStyled;
}

bool CSciEdit::WrapLines(Sci_Position startpos, Sci_Position endpos) const
{
    Sci_Position markerX = static_cast<Sci_Position>(Call(SCI_GETEDGECOLUMN) * Call(SCI_TEXTWIDTH, 0, reinterpret_cast<LPARAM>(" ")));
    if (markerX)
    {
        Call(SCI_SETTARGETSTART, startpos);
        Call(SCI_SETTARGETEND, endpos);
        Call(SCI_LINESSPLIT, markerX);
        return true;
    }
    return false;
}

void CSciEdit::AdvanceUTF8(const char* str, int& pos)
{
    if ((str[pos] & 0xE0) == 0xC0)
    {
        // utf8 2-byte sequence
        pos += 2;
    }
    else if ((str[pos] & 0xF0) == 0xE0)
    {
        // utf8 3-byte sequence
        pos += 3;
    }
    else if ((str[pos] & 0xF8) == 0xF0)
    {
        // utf8 4-byte sequence
        pos += 4;
    }
    else
        pos++;
}

bool CSciEdit::FindStyleChars(const char* line, char styler, Sci_Position& start, Sci_Position& end)
{
    int i = 0;
    int u = 0;
    while (i < start)
    {
        AdvanceUTF8(line, i);
        u++;
    }

    bool    bFoundMarker = false;
    CString sULine       = CUnicodeUtils::GetUnicode(line);
    // find a starting marker
    while (line[i] != 0)
    {
        if (line[i] == styler)
        {
            if ((line[i + 1] != 0) && (IsCharAlphaNumeric(sULine[u + 1])) &&
                (((u > 0) && (!IsCharAlphaNumeric(sULine[u - 1]))) || (u == 0)))
            {
                start = i + 1LL;
                AdvanceUTF8(line, i);
                u++;
                bFoundMarker = true;
                break;
            }
        }
        AdvanceUTF8(line, i);
        u++;
    }
    if (!bFoundMarker)
        return false;
    // find ending marker
    bFoundMarker = false;
    while (line[i] != 0)
    {
        if (line[i] == styler)
        {
            if ((IsCharAlphaNumeric(sULine[u - 1])) &&
                ((((u + 1) < sULine.GetLength()) && (!IsCharAlphaNumeric(sULine[u + 1]))) || ((u + 1) == sULine.GetLength())))
            {
                end = i;
                i++;
                bFoundMarker = true;
                break;
            }
        }
        AdvanceUTF8(line, i);
        u++;
    }
    return bFoundMarker;
}

BOOL CSciEdit::MarkEnteredBugID(Sci_Position startstylepos, Sci_Position endstylepos) const
{
    if (m_sCommand.IsEmpty())
        return FALSE;
    // get the text between the start and end position we have to style
    const Sci_Position lineNumber = static_cast<Sci_Position>(Call(SCI_LINEFROMPOSITION, startstylepos));
    Sci_Position       startPos   = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, static_cast<WPARAM>(lineNumber)));
    Sci_Position       endPos     = endstylepos;

    if (startPos == endPos)
        return FALSE;
    if (startPos > endPos)
    {
        auto switchtemp = startPos;
        startPos        = endPos;
        endPos          = switchtemp;
    }

    auto          textBuffer = std::make_unique<char[]>(endPos - startPos + 2);
    Sci_TextRange textRange;
    textRange.lpstrText  = textBuffer.get();
    textRange.chrg.cpMin = static_cast<Sci_PositionCR>(startPos);
    textRange.chrg.cpMax = static_cast<Sci_PositionCR>(endPos);
    Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
    CStringA msg = CStringA(textBuffer.get());

    Call(SCI_STARTSTYLING, startPos, STYLE_MASK);

    try
    {
        if (!m_sBugID.IsEmpty())
        {
            // match with two regex strings (without grouping!)
            const std::regex           regCheck(m_sCommand);
            const std::regex           regBugID(m_sBugID);
            const std::sregex_iterator end;
            std::string                s   = static_cast<LPCSTR>(msg);
            LONG                       pos = 0;
            // note:
            // if start_pos is 0, we're styling from the beginning and let the ^ char match the beginning of the line
            // that way, the ^ matches the very beginning of the log message and not the beginning of further lines.
            // problem is: this only works *while* entering log messages. If a log message is pasted in whole or
            // multiple lines are pasted, start_pos can be 0 and styling goes over multiple lines. In that case, those
            // additional line starts also match ^
            for (std::sregex_iterator it(s.begin(), s.end(), regCheck, startPos != 0 ? std::regex_constants::match_not_bol : std::regex_constants::match_default); it != end; ++it)
            {
                // clear the styles up to the match position
                Call(SCI_SETSTYLING, it->position(0) - pos, STYLE_DEFAULT);

                // (*it)[0] is the matched string
                std::string matchedString = (*it)[0];
                LONG        matchedPos    = 0;
                for (std::sregex_iterator it2(matchedString.begin(), matchedString.end(), regBugID); it2 != end; ++it2)
                {
                    ATLTRACE("matched id : %s\n", std::string((*it2)[0]).c_str());

                    // bold style up to the id match
                    ATLTRACE("position = %ld\n", it2->position(0));
                    if (it2->position(0))
                        Call(SCI_SETSTYLING, it2->position(0) - matchedPos, STYLE_ISSUEBOLD);
                    // bold and recursive style for the bug ID itself
                    if ((*it2)[0].str().size())
                        Call(SCI_SETSTYLING, (*it2)[0].str().size(), STYLE_ISSUEBOLDITALIC);
                    matchedPos = static_cast<LONG>(it2->position(0) + (*it2)[0].str().size());
                }
                if ((matchedPos) && (matchedPos < static_cast<LONG>(matchedString.size())))
                {
                    Call(SCI_SETSTYLING, matchedString.size() - matchedPos, STYLE_ISSUEBOLD);
                }
                pos = static_cast<LONG>(it->position(0) + matchedString.size());
            }
            // bold style for the rest of the string which isn't matched
            if (s.size() - pos)
                Call(SCI_SETSTYLING, s.size() - pos, STYLE_DEFAULT);
        }
        else
        {
            const std::regex           regCheck(m_sCommand);
            const std::sregex_iterator end;
            std::string                s   = static_cast<LPCSTR>(msg);
            LONG                       pos = 0;
            for (std::sregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
            {
                // clear the styles up to the match position
                if (it->position(0) - pos >= 0)
                    Call(SCI_SETSTYLING, it->position(0) - pos, STYLE_DEFAULT);
                pos = static_cast<LONG>(it->position(0));

                const std::smatch match = *it;
                // we define group 1 as the whole issue text and
                // group 2 as the bug ID
                if (match.size() >= 2)
                {
                    ATLTRACE("matched id : %s\n", std::string(match[1]).c_str());
                    if (match[1].first - s.begin() - pos >= 0)
                        Call(SCI_SETSTYLING, match[1].first - s.begin() - pos, STYLE_ISSUEBOLD);
                    Call(SCI_SETSTYLING, std::string(match[1]).size(), STYLE_ISSUEBOLDITALIC);
                    pos = static_cast<LONG>(match[1].second - s.begin());
                }
            }
        }
    }
    catch (std::exception&)
    {
    }

    return FALSE;
}

bool CSciEdit::IsValidURLChar(unsigned char ch)
{
    return isalnum(ch) ||
           ch == '_' || ch == '/' || ch == ';' || ch == '?' || ch == '&' || ch == '=' ||
           ch == '%' || ch == ':' || ch == '.' || ch == '#' || ch == '-' || ch == '+' ||
           ch == '|' || ch == '>' || ch == '<' || ch == '!' || ch == '@' || ch == '~';
}

void CSciEdit::StyleURLs(Sci_Position startStylePos, Sci_Position endStylePos) const
{
    const Sci_Position lineNumber = static_cast<Sci_Position>(Call(SCI_LINEFROMPOSITION, startStylePos));
    startStylePos                 = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, static_cast<WPARAM>(lineNumber)));

    auto          len        = endStylePos - startStylePos + 1;
    auto          textBuffer = std::make_unique<char[]>(len + 1);
    Sci_TextRange textRange;
    textRange.lpstrText  = textBuffer.get();
    textRange.chrg.cpMin = static_cast<Sci_PositionCR>(startStylePos);
    textRange.chrg.cpMax = static_cast<Sci_PositionCR>(endStylePos);
    Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
    // we're dealing with utf8 encoded text here, which means one glyph is
    // not necessarily one byte/wchar_t
    // that's why we use CStringA to still get a correct char index
    CStringA msg = textBuffer.get();

    int startUrl = -1;
    for (int i = 0; i <= msg.GetLength(); AdvanceUTF8(msg, i))
    {
        if ((i < len) && IsValidURLChar(msg[i]))
        {
            if (startUrl < 0)
                startUrl = i;
        }
        else
        {
            if (startUrl >= 0)
            {
                bool strip = true;
                if (msg[startUrl] == '<' && i < len) // try to detect and do not strip URLs put within <>
                {
                    while (startUrl <= i && msg[startUrl] == '<') // strip leading '<'
                        ++startUrl;
                    strip = false;
                    i     = startUrl;
                    while (i < len && msg[i] != '\r' && msg[i] != '\n' && msg[i] != '>') // find first '>' or new line after resetting i to start position
                        AdvanceUTF8(msg, i);
                }
                ASSERT(startStylePos + i <= endStylePos);
                int skipTrailing = 0;
                while (strip && i - skipTrailing - 1 > startUrl && (msg[i - skipTrailing - 1] == '.' || msg[i - skipTrailing - 1] == '-' || msg[i - skipTrailing - 1] == '?' || msg[i - skipTrailing - 1] == ';' || msg[i - skipTrailing - 1] == ':' || msg[i - skipTrailing - 1] == '>' || msg[i - skipTrailing - 1] == '<' || msg[i - skipTrailing - 1] == '!'))
                    ++skipTrailing;
                if (!IsUrlOrEmail(msg.Mid(startUrl, i - startUrl - skipTrailing)))
                {
                    startUrl = -1;
                    continue;
                }
                ASSERT(startStylePos + i - skipTrailing <= endStylePos);
                Call(SCI_STARTSTYLING, startStylePos + startUrl, STYLE_URL);
                Call(SCI_SETSTYLING, i - startUrl - skipTrailing, STYLE_URL);
            }
            startUrl = -1;
        }
    }
}

bool CSciEdit::IsUrlOrEmail(const CStringA& sText)
{
    if (!PathIsURLA(sText))
    {
        auto atPos = sText.Find('@');
        if (atPos <= 0)
            return false;
        if (sText.ReverseFind('.') > atPos)
            return true;
        return false;
    }
    if (sText.Find("://") >= 0)
        return true;
    return false;
}

std::string CSciEdit::GetWordForSpellChecker(const CString& sWord) const
{
    // convert the string from the control to the encoding of the spell checker module.
    std::string sWordA;
    if (m_spellCodepage)
    {
        int lengthIncTerminator = WideCharToMultiByte(m_spellCodepage, 0, sWord, -1, nullptr, 0, nullptr, nullptr);
        if (lengthIncTerminator <= 1)
            return ""; // converting to the codepage failed
        sWordA.resize(lengthIncTerminator - 1LL);
        WideCharToMultiByte(m_spellCodepage, 0, sWord, -1, sWordA.data(), lengthIncTerminator - 1, nullptr, nullptr);
    }
    else
        sWordA = std::string(reinterpret_cast<LPCSTR>(static_cast<LPCWSTR>(sWord)));

    sWordA.erase(sWordA.find_last_not_of("\'\".,") + 1);
    sWordA.erase(0, sWordA.find_first_not_of("\'\".,"));

    if (m_bDoStyle)
    {
        for (const auto styleIndicator : {'*', '_', '^'})
        {
            if (sWordA.empty())
                break;
            if (sWordA[sWordA.size() - 1] == styleIndicator)
                sWordA.resize(sWordA.size() - 1);
            if (sWordA.empty())
                break;
            if (sWordA[0] == styleIndicator)
                sWordA = sWordA.substr(sWordA.size() - 1);
        }
    }

    return sWordA;
}

CString CSciEdit::GetWordFromSpellChecker(const std::string& sWordA) const
{
    CString sWord;
    if (m_spellCodepage)
    {
        wchar_t* buf = sWord.GetBuffer(static_cast<int>(sWordA.size()) * 2);
        int      lengthIncTerminator =
            MultiByteToWideChar(m_spellCodepage, 0, sWordA.c_str(), -1, buf, static_cast<int>(sWordA.size()) * 2);
        if (lengthIncTerminator == 0)
            return L"";
        sWord.ReleaseBuffer(lengthIncTerminator - 1);
    }
    else
        sWord = CString(sWordA.c_str());

    sWord.Trim(L"\'\".,");

    return sWord;
}

void CSciEdit::RestyleBugIDs() const
{
    int endstylepos = static_cast<int>(Call(SCI_GETLENGTH));
    // clear all styles
    Call(SCI_STARTSTYLING, 0, STYLE_MASK);
    Call(SCI_SETSTYLING, endstylepos, STYLE_DEFAULT);
    // style the bug IDs
    MarkEnteredBugID(0, endstylepos);
}

void CSciEdit::SetReadOnly(bool bReadOnly)
{
    m_bReadOnly = bReadOnly;
    Call(SCI_SETREADONLY, m_bReadOnly);
}

void CSciEdit::SetWindowStylesForAutocompletionPopup()
{
    if (CTheme::Instance().IsDarkTheme())
    {
        EnumThreadWindows(GetCurrentThreadId(), AdjustThemeProc, 0);
    }
}

BOOL CSciEdit::AdjustThemeProc(HWND hwnd, LPARAM /*lParam*/)
{
    wchar_t szWndClassName[MAX_PATH] = {0};
    GetClassName(hwnd, szWndClassName, _countof(szWndClassName));
    if ((wcscmp(szWndClassName, L"ListBoxX") == 0) ||
        (wcscmp(szWndClassName, WC_LISTBOX) == 0))
    {
        // in dark mode, the resizing border is visible at the top
        // of the popup, and it's white and ugly.
        // this removes the border, but that also means that the
        // popup is not resizable anymore - which I think is not
        // really necessary anyway.
        auto dwCurStyle = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE));
        dwCurStyle &= ~WS_THICKFRAME;
        dwCurStyle |= WS_BORDER;
        SetWindowLongPtr(hwnd, GWL_STYLE, dwCurStyle);

        DarkModeHelper::Instance().AllowDarkModeForWindow(hwnd, TRUE);
        SetWindowTheme(hwnd, L"Explorer", nullptr);
        EnumChildWindows(hwnd, AdjustThemeProc, 0);
    }

    return TRUE;
}
