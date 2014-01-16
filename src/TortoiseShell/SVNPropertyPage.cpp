// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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

#define MAX_STRING_LENGTH       4096            //should be big enough

// Nonmember function prototypes
BOOL CALLBACK PageProc (HWND, UINT, WPARAM, LPARAM);
UINT CALLBACK PropPageCallbackProc ( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

// CShellExt member functions (needed for IShellPropSheetExt)
STDMETHODIMP CShellExt::AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    __try
    {
        return AddPages_Wrap(lpfnAddPage, lParam);
    }
    __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return E_FAIL;
}

STDMETHODIMP CShellExt::AddPages_Wrap (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    for (std::vector<tstring>::iterator I = files_.begin(); I != files_.end(); ++I)
    {
        SVNStatus svn;
        if (svn.GetStatus(CTSVNPath(I->c_str())) == (-2))
            return S_OK;            // file/directory not under version control

        if (svn.status->versioned == 0)
            return S_OK;
    }

    if (files_.empty())
        return S_OK;

    LoadLangDll();
    PROPSHEETPAGE psp;
    SecureZeroMemory(&psp, sizeof(PROPSHEETPAGE));
    HPROPSHEETPAGE hPage;
    CSVNPropertyPage *sheetpage = NULL;

    sheetpage = new (std::nothrow) CSVNPropertyPage(files_);

    if (sheetpage == NULL)
        return E_OUTOFMEMORY;

    psp.dwSize = sizeof (psp);
    psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK;
    psp.hInstance = g_hResInst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE);
    psp.pszIcon = MAKEINTRESOURCE(IDI_APPSMALL);
    psp.pszTitle = L"Subversion";
    psp.pfnDlgProc = (DLGPROC) PageProc;
    psp.lParam = (LPARAM) sheetpage;
    psp.pfnCallback = PropPageCallbackProc;
    psp.pcRefParent = (UINT*)&g_cRefThisDll;

    hPage = CreatePropertySheetPage (&psp);

    if (hPage != NULL)
    {
        if (!lpfnAddPage (hPage, lParam))
        {
            delete sheetpage;
            DestroyPropertySheetPage (hPage);
        }
    }

    return S_OK;
}



STDMETHODIMP CShellExt::ReplacePage (UINT /*uPageID*/, LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/, LPARAM /*lParam*/)
{
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog procedures and other callback functions

BOOL CALLBACK PageProc (HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CSVNPropertyPage * sheetpage;

    if (uMessage == WM_INITDIALOG)
    {
        sheetpage = (CSVNPropertyPage*) ((LPPROPSHEETPAGE) lParam)->lParam;
        SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) sheetpage);
        sheetpage->SetHwnd(hwnd);
    }
    else
    {
        sheetpage = (CSVNPropertyPage*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
    }

    if (sheetpage != 0L)
    {
        __try
        {
            return sheetpage->PageProc(hwnd, uMessage, wParam, lParam);
        }
        __except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
        {
        }
    }
    return FALSE;
}

UINT CALLBACK PropPageCallbackProc ( HWND /*hwnd*/, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
    // Delete the page before closing.
    if (PSPCB_RELEASE == uMsg)
    {
        CSVNPropertyPage* sheetpage = (CSVNPropertyPage*) ppsp->lParam;
        delete sheetpage;
    }
    return 1;
}

// *********************** CSVNPropertyPage *************************

CSVNPropertyPage::CSVNPropertyPage(const std::vector<tstring> &newFilenames)
    : filenames(newFilenames)
    , m_hwnd(NULL)
{
    stringtablebuffer[0] = 0;
}

CSVNPropertyPage::~CSVNPropertyPage(void)
{
}

void CSVNPropertyPage::SetHwnd(HWND newHwnd)
{
    m_hwnd = newHwnd;
}

BOOL CSVNPropertyPage::PageProc (HWND /*hwnd*/, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
    case WM_INITDIALOG:
        InitWorkfileView();
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR point = (LPNMHDR)lParam;
            int code = point->code;
            //
            // Respond to notifications.
            //
            if (code == PSN_APPLY)
            {
            }
            SetWindowLongPtr (m_hwnd, DWLP_MSGRESULT, FALSE);
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
    if(HIWORD(wParam) != BN_CLICKED)
        return;

    if (LOWORD(wParam) == IDC_SHOWLOG)
    {
        tstring svnCmd = L" /command:";
        svnCmd += L"log /path:\"";
        svnCmd += filenames.front().c_str();
        svnCmd += L"\"";
        RunCommand(svnCmd);
    }
    if (LOWORD(wParam) == IDC_EDITPROPERTIES)
    {
        DWORD pathlength = GetTempPath(0, NULL);
        std::unique_ptr<TCHAR[]> path(new TCHAR[pathlength+1]);
        std::unique_ptr<TCHAR[]> tempFile(new TCHAR[pathlength + 100]);
        GetTempPath (pathlength+1, path.get());
        GetTempFileName (path.get(), L"svn", 0, tempFile.get());
        tstring retFilePath = tstring(tempFile.get());

        CAutoFile file = ::CreateFile (tempFile.get(),
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ,
                                       0,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_TEMPORARY,
                                       0);

        if (file)
        {
            DWORD written = 0;
            for (std::vector<tstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
            {
                ::WriteFile (file, I->c_str(), (DWORD)I->size()*sizeof(TCHAR), &written, 0);
                ::WriteFile (file, L"\n", 2, &written, 0);
            }

            tstring svnCmd = L" /command:";
            svnCmd += L"properties /pathfile:\"";
            svnCmd += retFilePath.c_str();
            svnCmd += L"\"";
            svnCmd += L" /deletepathfile";
            RunCommand(svnCmd);
        }
    }
}

void CSVNPropertyPage::Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen)
{
    struct tm newtime;
    SYSTEMTIME systime;

    bool bUseSystemLocale = !!(DWORD)CRegStdDWORD(L"Software\\TortoiseSVN\\UseSystemLocaleForDates", TRUE);
    LCID locale = bUseSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
    locale = MAKELCID(locale, SORT_DEFAULT);

    *buf = '\0';
    if (time)
    {
        _localtime64_s(&newtime, &time);

        systime.wDay = (WORD)newtime.tm_mday;
        systime.wDayOfWeek = (WORD)newtime.tm_wday;
        systime.wHour = (WORD)newtime.tm_hour;
        systime.wMilliseconds = 0;
        systime.wMinute = (WORD)newtime.tm_min;
        systime.wMonth = (WORD)newtime.tm_mon+1;
        systime.wSecond = (WORD)newtime.tm_sec;
        systime.wYear = (WORD)newtime.tm_year+1900;
        int ret = 0;
        TCHAR datebuf[MAX_STRING_LENGTH] = { 0 };
        if (CRegStdDWORD(L"Software\\TortoiseSVN\\LogDateFormat") == 1)
            ret = GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
        else
            ret = GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
        if (ret == 0)
            datebuf[0] = '\0';
        TCHAR timebuf[MAX_STRING_LENGTH] = { 0 };
        ret = GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_STRING_LENGTH);
        if (ret == 0)
            timebuf[0] = '\0';
        *buf = '\0';
        _tcsncat_s(buf, buflen, timebuf, MAX_STRING_LENGTH-1);
        _tcsncat_s(buf, buflen, L", ", MAX_STRING_LENGTH-1);
        _tcsncat_s(buf, buflen, datebuf, MAX_STRING_LENGTH-1);
    }
}

void CSVNPropertyPage::InitWorkfileView()
{
    SVNStatus svn;
    SVNInfo info;
    if (filenames.size() == 1)
    {
        if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
        {
            const SVNInfoData * infodata = info.GetFirstFileInfo(CTSVNPath(filenames.front().c_str()), SVNRev(), SVNRev());
            __time64_t  time;
            if (svn.status->versioned)
            {
                TCHAR buf[MAX_STRING_LENGTH] = { 0 };
                LoadLangDll();
                if (svn.status->node_status == svn_wc_status_added)
                {
                    // disable the "show log" button for added files
                    HWND showloghwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
                    if (GetFocus() == showloghwnd)
                        ::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
                    ::EnableWindow(showloghwnd, FALSE);
                }
                if (svn.status->revision != SVN_INVALID_REVNUM)
                {
                    _stprintf_s(buf, L"%d", svn.status->revision);
                    SetDlgItemText(m_hwnd, IDC_REVISION, buf);
                }
                else
                    SetDlgItemText(m_hwnd, IDC_REVISION, L"");

                if (svn.status->repos_relpath)
                {
                    size_t len = strlen(svn.status->repos_relpath) + strlen(svn.status->repos_root_url);
                    len += 2;
                    std::unique_ptr<char[]> url(new char[len*4]);
                    strcpy_s(url.get(), len*4, svn.status->repos_root_url);
                    strcat_s(url.get(), len*4, "/");
                    strcat_s(url.get(), len*4, svn.status->repos_relpath);

                    std::unique_ptr<char[]> unescapedurl(new char[len]);
                    strcpy_s(unescapedurl.get(), len, url.get());
                    CStringA escapedurl = CPathUtils::PathEscape(url.get());
                    CPathUtils::Unescape(unescapedurl.get());
                    strcpy_s(url.get(), len*4, escapedurl);
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
                    _stprintf_s(buf, L"%d", svn.status->changed_rev);
                    SetDlgItemText(m_hwnd, IDC_CREVISION, buf);
                }
                else
                    SetDlgItemText(m_hwnd, IDC_CREVISION, L"");

                if (svn.status->changed_date)
                {
                    time = (__time64_t)svn.status->changed_date/1000000L;
                    Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                    SetDlgItemText(m_hwnd, IDC_CDATE, buf);
                }
                else
                    SetDlgItemText(m_hwnd, IDC_CDATE, L"");

                if (svn.status->changed_author)
                    SetDlgItemText(m_hwnd, IDC_AUTHOR, CUnicodeUtils::StdGetUnicode(svn.status->changed_author).c_str());
                SVNStatus::GetStatusString(g_hResInst, svn.status->node_status, buf, _countof(buf), (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_NODESTATUS, buf);
                SVNStatus::GetStatusString(g_hResInst, svn.status->text_status, buf, _countof(buf), (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
                SVNStatus::GetStatusString(g_hResInst, svn.status->prop_status, buf, _countof(buf), (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_PROPSTATUS, buf);
                if (infodata)
                    time = (__time64_t)infodata->texttime;
                else
                    time = (__time64_t)svn.status->changed_date/1000000L;
                Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                SetDlgItemText(m_hwnd, IDC_TEXTDATE, buf);

                if (svn.status->lock && svn.status->lock->owner)
                    SetDlgItemText(m_hwnd, IDC_LOCKOWNER, CUnicodeUtils::StdGetUnicode(svn.status->lock->owner).c_str());
                if (svn.status->lock)
                {
                    time = (__time64_t)svn.status->lock->creation_date/1000000L;
                    Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                    SetDlgItemText(m_hwnd, IDC_LOCKDATE, buf);
                }
                if (infodata)
                    SetDlgItemText(m_hwnd, IDC_REPOUUID, (LPCTSTR)infodata->reposUUID);
                if (svn.status->changelist)
                    SetDlgItemText(m_hwnd, IDC_CHANGELIST, CUnicodeUtils::StdGetUnicode(svn.status->changelist).c_str());
                SVNStatus::GetDepthString(g_hResInst, infodata ? infodata->depth : svn_depth_unknown, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_DEPTHEDIT, buf);

                if (infodata)
                    SetDlgItemText(m_hwnd, IDC_CHECKSUM, (LPCTSTR)infodata->checksum);

                if (svn.status->wc_is_locked)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_LOCKED, stringtablebuffer);

                if (svn.status->copied)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_COPIED, stringtablebuffer);

                if (svn.status->switched)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_SWITCHED, stringtablebuffer);

                if (svn.status->file_external)
                    MAKESTRING(IDS_YES);
                else
                    MAKESTRING(IDS_NO);
                SetDlgItemText(m_hwnd, IDC_FILEEXTERNAL, stringtablebuffer);
            }
        }
    }
    else if (!filenames.empty())
    {
        //deactivate the show log button
        HWND logwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
        if (GetFocus() == logwnd)
            ::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
        ::EnableWindow(logwnd, FALSE);
        //get the handle of the list view
        if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
        {
            LoadLangDll();
            if (svn.status->repos_relpath)
            {
                CPathUtils::Unescape((char*)svn.status->repos_relpath);
                TCHAR tbuf[MAX_STRING_LENGTH] = { 0 };
                _tcsncpy_s(tbuf, CUnicodeUtils::StdGetUnicode(svn.status->repos_relpath).c_str(), _countof(tbuf)-1);
                TCHAR * ptr = _tcsrchr(tbuf, '/');
                if (ptr != 0)
                {
                    *ptr = 0;
                }
                SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
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

void CSVNPropertyPage::RunCommand(const tstring& command)
{
    tstring tortoiseProcPath = GetAppDirectory() + L"TortoiseProc.exe";
    CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), const_cast<TCHAR*>(command.c_str()));
}
