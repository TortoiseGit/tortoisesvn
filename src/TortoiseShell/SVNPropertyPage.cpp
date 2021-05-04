// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014, 2016, 2018, 2021 - TortoiseSVN

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

#include "ShellExt.h"
#include "SVNPropertyPage.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "CreateProcessHelper.h"
#include "resource.h"

#define MAX_STRING_LENGTH 4096 //should be big enough

// Nonmember function prototypes
BOOL CALLBACK PageProc(HWND, UINT, WPARAM, LPARAM);
UINT CALLBACK PropPageCallbackProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

// CShellExt member functions (needed for IShellPropSheetExt)
HRESULT STDMETHODCALLTYPE CShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    for (auto I = m_files.begin(); I != m_files.end(); ++I)
    {
        SVNStatus svn;
        if (svn.GetStatus(CTSVNPath(I->c_str())) == (-2))
            return S_OK; // file/directory not under version control

        if (svn.status->versioned == 0)
            return S_OK;
    }

    if (m_files.empty())
        return S_OK;

    LoadLangDll();
    PROPSHEETPAGE psp = {0};

    CSVNPropertyPage* sheetPage = new (std::nothrow) CSVNPropertyPage(m_files);

    if (sheetPage == nullptr)
        return E_OUTOFMEMORY;

    UINT refParent  = static_cast<UINT>(g_cRefThisDll);
    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK;
    psp.hInstance   = g_hResInst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE);
    psp.pszIcon     = MAKEINTRESOURCE(IDI_APPSMALL);
    psp.pszTitle    = L"Subversion";
    psp.pfnDlgProc  = reinterpret_cast<DLGPROC>(PageProc);
    psp.lParam      = reinterpret_cast<LPARAM>(sheetPage);
    psp.pfnCallback = PropPageCallbackProc;
    psp.pcRefParent = &refParent;

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);

    if (hPage != nullptr)
    {
        if (!lpfnAddPage(hPage, lParam))
        {
            delete sheetPage;
            DestroyPropertySheetPage(hPage);
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellExt::ReplacePage(UINT /*uPageID*/, LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/, LPARAM /*lParam*/)
{
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog procedures and other callback functions

BOOL CALLBACK PageProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CSVNPropertyPage* sheetPage;

    if (uMessage == WM_INITDIALOG)
    {
        sheetPage = reinterpret_cast<CSVNPropertyPage*>(reinterpret_cast<LPPROPSHEETPAGEW>(lParam)->lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(sheetPage));
        sheetPage->SetHwnd(hwnd);
    }
    else
    {
        sheetPage = reinterpret_cast<CSVNPropertyPage*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (sheetPage != nullptr)
    {
        return sheetPage->PageProc(hwnd, uMessage, wParam, lParam);
    }
    return FALSE;
}

UINT CALLBACK PropPageCallbackProc(HWND /*hwnd*/, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    // Delete the page before closing.
    if (PSPCB_RELEASE == uMsg)
    {
        CSVNPropertyPage* sheetPage = reinterpret_cast<CSVNPropertyPage*>(ppsp->lParam);
        delete sheetPage;
    }
    return 1;
}

// *********************** CSVNPropertyPage *************************

CSVNPropertyPage::CSVNPropertyPage(const std::vector<std::wstring>& newFilenames)
    : m_hwnd(nullptr)
    , fileNames(newFilenames)
{
    stringTableBuffer[0] = 0;
}

CSVNPropertyPage::~CSVNPropertyPage()
{
}

void CSVNPropertyPage::SetHwnd(HWND newHwnd)
{
    m_hwnd = newHwnd;
}

BOOL CSVNPropertyPage::PageProc(HWND /*hwnd*/, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        case WM_INITDIALOG:
            InitWorkfileView();
            return TRUE;
        case WM_NOTIFY:
        {
            LPNMHDR point = reinterpret_cast<LPNMHDR>(lParam);
            int     code  = point->code;
            //
            // Respond to notifications.
            //
            if (code == PSN_APPLY)
            {
            }
            SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, FALSE);
            return TRUE;
        }
        case WM_DESTROY:
            return TRUE;
        case WM_COMMAND:
            PageProcOnCommand(wParam);
            break;
    } // switch (uMessage)
    return FALSE;
}

void CSVNPropertyPage::PageProcOnCommand(WPARAM wParam)
{
    if (HIWORD(wParam) != BN_CLICKED)
        return;

    if (LOWORD(wParam) == IDC_SHOWLOG)
    {
        std::wstring svnCmd = L" /command:";
        svnCmd += L"log /path:\"";
        svnCmd += fileNames.front().c_str();
        svnCmd += L"\"";
        RunCommand(svnCmd);
    }
    if (LOWORD(wParam) == IDC_EDITPROPERTIES)
    {
        DWORD pathLength = GetTempPath(0, nullptr);
        auto  path       = std::make_unique<wchar_t[]>(pathLength + 1);
        auto  tempFile   = std::make_unique<wchar_t[]>(pathLength + 100);
        GetTempPath(pathLength + 1, path.get());
        GetTempFileName(path.get(), L"svn", 0, tempFile.get());
        std::wstring retFilePath = std::wstring(tempFile.get());

        CAutoFile file = ::CreateFile(tempFile.get(),
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      nullptr,
                                      CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_TEMPORARY,
                                      nullptr);

        if (file)
        {
            DWORD written = 0;
            for (auto I = fileNames.begin(); I != fileNames.end(); ++I)
            {
                ::WriteFile(file, I->c_str(), static_cast<DWORD>(I->size()) * sizeof(wchar_t), &written, nullptr);
                ::WriteFile(file, L"\n", 2, &written, nullptr);
            }

            std::wstring svnCmd = L" /command:";
            svnCmd += L"properties /pathfile:\"";
            svnCmd += retFilePath.c_str();
            svnCmd += L"\"";
            svnCmd += L" /deletepathfile";
            RunCommand(svnCmd);
        }
    }
}

void CSVNPropertyPage::Time64ToTimeString(__time64_t time, wchar_t* buf, size_t buflen) const
{
    struct tm  newTime;
    SYSTEMTIME sysTime;

    bool bUseSystemLocale = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\UseSystemLocaleForDates", TRUE));
    LCID locale           = bUseSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
    locale                = MAKELCID(locale, SORT_DEFAULT);

    *buf = '\0';
    if (time)
    {
        _localtime64_s(&newTime, &time);

        sysTime.wDay                       = static_cast<WORD>(newTime.tm_mday);
        sysTime.wDayOfWeek                 = static_cast<WORD>(newTime.tm_wday);
        sysTime.wHour                      = static_cast<WORD>(newTime.tm_hour);
        sysTime.wMilliseconds              = 0;
        sysTime.wMinute                    = static_cast<WORD>(newTime.tm_min);
        sysTime.wMonth                     = static_cast<WORD>(newTime.tm_mon) + 1;
        sysTime.wSecond                    = static_cast<WORD>(newTime.tm_sec);
        sysTime.wYear                      = static_cast<WORD>(newTime.tm_year) + 1900;
        int     ret                        = 0;
        wchar_t dateBuf[MAX_STRING_LENGTH] = {0};
        if (CRegStdDWORD(L"Software\\TortoiseSVN\\LogDateFormat") == 1)
            ret = GetDateFormat(locale, DATE_SHORTDATE, &sysTime, nullptr, dateBuf, MAX_STRING_LENGTH);
        else
            ret = GetDateFormat(locale, DATE_LONGDATE, &sysTime, nullptr, dateBuf, MAX_STRING_LENGTH);
        if (ret == 0)
            dateBuf[0] = '\0';
        wchar_t timeBuf[MAX_STRING_LENGTH] = {0};
        ret                                = GetTimeFormat(locale, 0, &sysTime, nullptr, timeBuf, MAX_STRING_LENGTH);
        if (ret == 0)
            timeBuf[0] = '\0';
        *buf = '\0';
        wcsncat_s(buf, buflen, timeBuf, MAX_STRING_LENGTH - 1);
        wcsncat_s(buf, buflen, L", ", MAX_STRING_LENGTH - 1);
        wcsncat_s(buf, buflen, dateBuf, MAX_STRING_LENGTH - 1);
    }
}

void CSVNPropertyPage::InitWorkfileView()
{
    SVNStatus svn{};
    SVNInfo   info{};
    if (fileNames.size() == 1)
    {
        if (svn.GetStatus(CTSVNPath(fileNames.front().c_str())) > (-2))
        {
            const SVNInfoData* infoData = info.GetFirstFileInfo(CTSVNPath(fileNames.front().c_str()), SVNRev(), SVNRev());
            __time64_t         time;
            if (svn.status->versioned)
            {
                wchar_t buf[MAX_STRING_LENGTH] = {0};
                LoadLangDll();
                if (svn.status->node_status == svn_wc_status_added)
                {
                    // disable the "show log" button for added files
                    HWND showLogHwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
                    if (GetFocus() == showLogHwnd)
                        ::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
                    ::EnableWindow(showLogHwnd, FALSE);
                }
                if (svn.status->revision != SVN_INVALID_REVNUM)
                {
                    swprintf_s(buf, L"%ld", svn.status->revision);
                    SetDlgItemText(m_hwnd, IDC_REVISION, buf);
                }
                else
                    SetDlgItemText(m_hwnd, IDC_REVISION, L"");

                if (svn.status->repos_relpath)
                {
                    size_t len = strlen(svn.status->repos_relpath) + strlen(svn.status->repos_root_url);
                    len += 2;
                    auto url = std::make_unique<char[]>(len * 4);
                    strcpy_s(url.get(), len * 4, svn.status->repos_root_url);
                    strcat_s(url.get(), len * 4, "/");
                    strcat_s(url.get(), len * 4, svn.status->repos_relpath);

                    auto unescapedurl = std::make_unique<char[]>(len);
                    strcpy_s(unescapedurl.get(), len, url.get());
                    CStringA escapedurl = CPathUtils::PathEscape(url.get());
                    CPathUtils::Unescape(unescapedurl.get());
                    strcpy_s(url.get(), len * 4, escapedurl);
                    SetDlgItemText(m_hwnd, IDC_REPOURL, CUnicodeUtils::StdGetUnicode(unescapedurl.get()).c_str());
                    if (strcmp(unescapedurl.get(), url.get()))
                    {
                        ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_SHOW);
                        ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_SHOW);
                        SetDlgItemText(m_hwnd, IDC_REPOURLUNESCAPED, CUnicodeUtils::StdGetUnicode(url.get()).c_str());
                    }
                    else
                    {
                        ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_HIDE);
                        ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_HIDE);
                    }
                }
                else
                {
                    ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_HIDE);
                    ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_HIDE);
                }
                if (svn.status->changed_rev != SVN_INVALID_REVNUM)
                {
                    swprintf_s(buf, L"%ld", svn.status->changed_rev);
                    SetDlgItemText(m_hwnd, IDC_CREVISION, buf);
                }
                else
                    SetDlgItemText(m_hwnd, IDC_CREVISION, L"");

                if (svn.status->changed_date)
                {
                    time = static_cast<__time64_t>(svn.status->changed_date) / 1000000L;
                    Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                    SetDlgItemText(m_hwnd, IDC_CDATE, buf);
                }
                else
                    SetDlgItemText(m_hwnd, IDC_CDATE, L"");

                if (svn.status->changed_author)
                    SetDlgItemText(m_hwnd, IDC_AUTHOR, CUnicodeUtils::StdGetUnicode(svn.status->changed_author).c_str());
                SVNStatus::GetStatusString(g_hResInst, svn.status->node_status, buf, _countof(buf), static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))));
                SetDlgItemText(m_hwnd, IDC_NODESTATUS, buf);
                SVNStatus::GetStatusString(g_hResInst, svn.status->text_status, buf, _countof(buf), static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))));
                SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
                SVNStatus::GetStatusString(g_hResInst, svn.status->prop_status, buf, _countof(buf), static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))));
                SetDlgItemText(m_hwnd, IDC_PROPSTATUS, buf);
                if (infoData)
                    time = static_cast<__time64_t>(infoData->textTime);
                else
                    time = static_cast<__time64_t>(svn.status->changed_date) / 1000000L;
                Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                SetDlgItemText(m_hwnd, IDC_TEXTDATE, buf);

                if (svn.status->lock && svn.status->lock->owner)
                    SetDlgItemText(m_hwnd, IDC_LOCKOWNER, CUnicodeUtils::StdGetUnicode(svn.status->lock->owner).c_str());
                if (svn.status->lock)
                {
                    time = static_cast<__time64_t>(svn.status->lock->creation_date) / 1000000L;
                    Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                    SetDlgItemText(m_hwnd, IDC_LOCKDATE, buf);
                }
                if (infoData)
                    SetDlgItemText(m_hwnd, IDC_REPOUUID, static_cast<LPCWSTR>(infoData->reposUuid));
                if (svn.status->changelist)
                    SetDlgItemText(m_hwnd, IDC_CHANGELIST, CUnicodeUtils::StdGetUnicode(svn.status->changelist).c_str());
                SVNStatus::GetDepthString(g_hResInst, infoData ? infoData->depth : svn_depth_unknown, buf, sizeof(buf) / sizeof(wchar_t), static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))));
                SetDlgItemText(m_hwnd, IDC_DEPTHEDIT, buf);

                if (infoData)
                    SetDlgItemText(m_hwnd, IDC_CHECKSUM, static_cast<LPCWSTR>(infoData->checksum));

                if (svn.status->wc_is_locked)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_LOCKED, stringTableBuffer);

                if (svn.status->copied)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_COPIED, stringTableBuffer);

                if (svn.status->switched)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_SWITCHED, stringTableBuffer);

                if (svn.status->file_external)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_FILEEXTERNAL, stringTableBuffer);
            }
        }
    }
    else if (!fileNames.empty())
    {
        //deactivate the show log button
        HWND logWnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
        if (GetFocus() == logWnd)
            ::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
        ::EnableWindow(logWnd, FALSE);
        //get the handle of the list view
        if (svn.GetStatus(CTSVNPath(fileNames.front().c_str())) > (-2))
        {
            LoadLangDll();
            if (svn.status->repos_relpath)
            {
                CPathUtils::Unescape(const_cast<char*>(svn.status->repos_relpath));
                wchar_t tBuf[MAX_STRING_LENGTH] = {0};
                wcsncpy_s(tBuf, CUnicodeUtils::StdGetUnicode(svn.status->repos_relpath).c_str(), _countof(tBuf) - 1);
                wchar_t* ptr = wcsrchr(tBuf, '/');
                if (ptr != nullptr)
                {
                    *ptr = 0;
                }
                SetDlgItemText(m_hwnd, IDC_REPOURL, tBuf);
            }
            SetDlgItemText(m_hwnd, IDC_LOCKED, L"");
            SetDlgItemText(m_hwnd, IDC_COPIED, L"");
            SetDlgItemText(m_hwnd, IDC_SWITCHED, L"");
            SetDlgItemText(m_hwnd, IDC_FILEEXTERNAL, L"");
            SetDlgItemText(m_hwnd, IDC_TREECONFLICT, L"");

            SetDlgItemText(m_hwnd, IDC_DEPTHEDIT, L"");
            SetDlgItemText(m_hwnd, IDC_CHECKSUM, L"");
            SetDlgItemText(m_hwnd, IDC_REPOUUID, L"");

            ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_HIDE);
        }
    }
}

void CSVNPropertyPage::RunCommand(const std::wstring& command)
{
    std::wstring tortoiseProcPath = GetAppDirectory() + L"TortoiseProc.exe";
    CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), command.c_str());
}
