// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "svnpropertypage.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "auto_buffer.h"
#include "CreateProcessHelper.h"

#define MAX_STRING_LENGTH       4096            //should be big enough

// Nonmember function prototypes
BOOL CALLBACK PageProc (HWND, UINT, WPARAM, LPARAM);
UINT CALLBACK PropPageCallbackProc ( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

// CShellExt member functions (needed for IShellPropSheetExt)
STDMETHODIMP CShellExt::AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,
                                  LPARAM lParam)
{
    for (std::vector<tstring>::iterator I = files_.begin(); I != files_.end(); ++I)
    {
        SVNStatus svn;
        if (svn.GetStatus(CTSVNPath(I->c_str())) == (-2))
            return S_OK;            // file/directory not under version control

        if (svn.status->versioned == 0)
            return S_OK;
    }

    if (files_.size() == 0)
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
    psp.pszTitle = _T("Subversion");
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
        return sheetpage->PageProc(hwnd, uMessage, wParam, lParam);
    else
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
    :filenames(newFilenames)
{
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
        tstring svnCmd = _T(" /command:");
        svnCmd += _T("log /path:\"");
        svnCmd += filenames.front().c_str();
        svnCmd += _T("\"");
        RunCommand(svnCmd);
    }
    if (LOWORD(wParam) == IDC_EDITPROPERTIES)
    {
        DWORD pathlength = GetTempPath(0, NULL);
        auto_buffer<TCHAR> path(pathlength+1);
        auto_buffer<TCHAR> tempFile(pathlength + 100);
        GetTempPath (pathlength+1, path);
        GetTempFileName (path, _T("svn"), 0, tempFile);
        tstring retFilePath = tstring(tempFile);

        HANDLE file = ::CreateFile (tempFile,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            0,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_TEMPORARY,
            0);

        if (file != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            for (std::vector<tstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
            {
                ::WriteFile (file, I->c_str(), (DWORD)I->size()*sizeof(TCHAR), &written, 0);
                ::WriteFile (file, _T("\n"), 2, &written, 0);
            }
            ::CloseHandle(file);

            tstring svnCmd = _T(" /command:");
            svnCmd += _T("properties /pathfile:\"");
            svnCmd += retFilePath.c_str();
            svnCmd += _T("\"");
            svnCmd += _T(" /deletepathfile");
            RunCommand(svnCmd);
        }
    }
}

void CSVNPropertyPage::Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen)
{
    struct tm newtime;
    SYSTEMTIME systime;
    TCHAR timebuf[MAX_STRING_LENGTH];
    TCHAR datebuf[MAX_STRING_LENGTH];

    LCID locale = (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
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
        if (CRegStdDWORD(_T("Software\\TortoiseSVN\\LogDateFormat")) == 1)
            ret = GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
        else
            ret = GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
        if (ret == 0)
            datebuf[0] = '\0';
        ret = GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_STRING_LENGTH);
        if (ret == 0)
            timebuf[0] = '\0';
        *buf = '\0';
        _tcsncat_s(buf, buflen, timebuf, MAX_STRING_LENGTH-1);
        _tcsncat_s(buf, buflen, _T(", "), MAX_STRING_LENGTH-1);
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
            TCHAR buf[MAX_STRING_LENGTH];
            __time64_t  time;
            if (svn.status->versioned)
            {
                LoadLangDll();
                if (svn.status->node_status == svn_wc_status_added)
                {
                    // disable the "show log" button for added files
                    HWND showloghwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
                    if (GetFocus() == showloghwnd)
                        ::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
                    ::EnableWindow(showloghwnd, FALSE);
                }
                else
                {
                    _stprintf_s(buf, _T("%d"), svn.status->changed_rev);
                    SetDlgItemText(m_hwnd, IDC_REVISION, buf);
                }
                if (svn.status->repos_relpath)
                {
                    size_t len = strlen(svn.status->repos_relpath) + strlen(svn.status->repos_root_url);
                    len += 2;
                    auto_buffer<char> url(len*4);
                    strcpy_s(url, len*4, svn.status->repos_root_url);
                    strcat_s(url, len*4, "/");
                    strcat_s(url, len*4, svn.status->repos_relpath);

                    auto_buffer<char> unescapedurl(len);
                    strcpy_s(unescapedurl, len, url.get());
                    CStringA escapedurl = CPathUtils::PathEscape(url.get());
                    CPathUtils::Unescape(unescapedurl);
                    strcpy_s(url, len*4, escapedurl);
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
                if (svn.status->node_status != svn_wc_status_added)
                {
                    _stprintf_s(buf, _T("%d"), svn.status->changed_rev);
                    SetDlgItemText(m_hwnd, IDC_CREVISION, buf);
                    time = (__time64_t)svn.status->changed_date/1000000L;
                    Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                    SetDlgItemText(m_hwnd, IDC_CDATE, buf);
                }
                if (svn.status->changed_author)
                    SetDlgItemText(m_hwnd, IDC_AUTHOR, CUnicodeUtils::StdGetUnicode(svn.status->changed_author).c_str());
                SVNStatus::GetStatusString(g_hResInst, svn.status->node_status, buf, _countof(buf), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_NODESTATUS, buf);
                SVNStatus::GetStatusString(g_hResInst, svn.status->text_status, buf, _countof(buf), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
                SVNStatus::GetStatusString(g_hResInst, svn.status->prop_status, buf, _countof(buf), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_PROPSTATUS, buf);
                if (infodata)
                    time = (__time64_t)infodata->texttime;
                else
                    time = (__time64_t)svn.status->changed_date/1000000L;
                Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                SetDlgItemText(m_hwnd, IDC_TEXTDATE, buf);
                if (infodata)
                    time = (__time64_t)infodata->proptime;
                else
                    time = (__time64_t)svn.status->changed_date/1000000L;
                Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                SetDlgItemText(m_hwnd, IDC_PROPDATE, buf);

                if (svn.status->lock && svn.status->lock->owner)
                    SetDlgItemText(m_hwnd, IDC_LOCKOWNER, CUnicodeUtils::StdGetUnicode(svn.status->lock->owner).c_str());
                if (svn.status->lock)
                    time = (__time64_t)svn.status->lock->creation_date/1000000L;
                Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
                SetDlgItemText(m_hwnd, IDC_LOCKDATE, buf);
                if (infodata)
                    SetDlgItemText(m_hwnd, IDC_REPOUUID, (LPCTSTR)infodata->reposUUID);
                if (svn.status->changelist)
                    SetDlgItemText(m_hwnd, IDC_CHANGELIST, CUnicodeUtils::StdGetUnicode(svn.status->changelist).c_str());
                SVNStatus::GetDepthString(g_hResInst, infodata ? infodata->depth : svn_depth_unknown, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
                SetDlgItemText(m_hwnd, IDC_DEPTHEDIT, buf);

                if (infodata)
                    SetDlgItemText(m_hwnd, IDC_CHECKSUM, (LPCTSTR)infodata->checksum);

                if (svn.status->locked)
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
    else if (filenames.size() != 0)
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
                TCHAR tbuf[MAX_STRING_LENGTH];
                _tcsncpy_s(tbuf, CUnicodeUtils::StdGetUnicode(svn.status->repos_relpath).c_str(), _countof(tbuf)-1);
                TCHAR * ptr = _tcsrchr(tbuf, '/');
                if (ptr != 0)
                {
                    *ptr = 0;
                }
                SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
            }
            SetDlgItemText(m_hwnd, IDC_LOCKED, _T(""));
            SetDlgItemText(m_hwnd, IDC_COPIED, _T(""));
            SetDlgItemText(m_hwnd, IDC_SWITCHED, _T(""));
            SetDlgItemText(m_hwnd, IDC_FILEEXTERNAL, _T(""));
            SetDlgItemText(m_hwnd, IDC_TREECONFLICT, _T(""));

            SetDlgItemText(m_hwnd, IDC_DEPTHEDIT, _T(""));
            SetDlgItemText(m_hwnd, IDC_CHECKSUM, _T(""));
            SetDlgItemText(m_hwnd, IDC_REPOUUID, _T(""));

            ShowWindow(GetDlgItem(m_hwnd, IDC_ESCAPEDURLLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(m_hwnd, IDC_REPOURLUNESCAPED), SW_HIDE);
        }
    }
}

void CSVNPropertyPage::RunCommand(const tstring& command)
{
    tstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
    CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), const_cast<TCHAR*>(command.c_str()));
}
