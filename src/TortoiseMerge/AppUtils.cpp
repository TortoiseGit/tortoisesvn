// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2010, 2012-2014, 2018, 2021 - TortoiseSVN

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
#include "registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include "ProgressDlg.h"

#pragma warning(push)
#include "svn_pools.h"
#include "svn_io.h"
#include "svn_path.h"
#include "svn_diff.h"
#include "svn_string.h"
#include "svn_utf.h"
#pragma warning(pop)
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"

CAppUtils::CAppUtils()
{
}

CAppUtils::~CAppUtils()
{
}

BOOL CAppUtils::GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CProgressDlg* progDlg, HWND hWnd /*=nullptr*/)
{
    CString sScmPath = CRegString(L"Software\\TortoiseMerge\\SCMPath", L"");
    if (sScmPath.IsEmpty())
    {
        // no path set, so use TortoiseSVN as default
        sScmPath = CPathUtils::GetAppDirectory() + L"TortoiseProc.exe";
        sScmPath += L" /command:cat /path:\"%1\" /revision:%2 /savepath:\"%3\" /hwnd:%4";
    }
    CString sTemp;
    sTemp.Format(L"%p", static_cast<void*>(hWnd));
    sScmPath.Replace(L"%1", sPath);
    sScmPath.Replace(L"%2", sVersion);
    sScmPath.Replace(L"%3", sSavePath);
    sScmPath.Replace(L"%4", sTemp);
    // start the external SCM program to fetch the specific version of the file
    PROCESS_INFORMATION process;
    if (!CCreateProcessHelper::CreateProcess(nullptr, sScmPath, &process))
    {
        CFormatMessageWrapper errorDetails;
        MessageBox(nullptr, errorDetails, L"TortoiseMerge", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    DWORD ret = 0;
    do
    {
        ret = WaitForSingleObject(process.hProcess, 100);
    } while ((ret == WAIT_TIMEOUT) && (!progDlg->HasUserCancelled()));
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (progDlg->HasUserCancelled())
    {
        return FALSE;
    }
    if (!PathFileExists(sSavePath))
        return FALSE;
    return TRUE;
}

bool CAppUtils::CreateUnifiedDiff(const CString& orig, const CString& modified, const CString& output, int contextsize, bool ignoreEOL, bool bShowError)
{
    apr_file_t* outFile = nullptr;
    apr_pool_t* pool    = svn_pool_create(nullptr);

    svn_error_t* err = svn_io_file_open(&outFile, svn_dirent_internal_style(CUnicodeUtils::GetUTF8(output), pool),
                                        APR_WRITE | APR_CREATE | APR_BINARY | APR_TRUNCATE,
                                        APR_OS_DEFAULT, pool);
    if (!err)
    {
        svn_stream_t* stream = svn_stream_from_aprfile2(outFile, false, pool);
        if (stream)
        {
            svn_diff_t*              diff = nullptr;
            svn_diff_file_options_t* opts = svn_diff_file_options_create(pool);
            opts->ignore_eol_style        = ignoreEOL;
            opts->ignore_space            = svn_diff_file_ignore_space_none;
            err                           = svn_diff_file_diff_2(&diff, svn_dirent_internal_style(CUnicodeUtils::GetUTF8(orig), pool),
                                       svn_dirent_internal_style(CUnicodeUtils::GetUTF8(modified), pool), opts, pool);
            if (!err)
            {
                err = svn_diff_file_output_unified4(stream, diff, svn_dirent_internal_style(CUnicodeUtils::GetUTF8(orig), pool),
                                                    svn_dirent_internal_style(CUnicodeUtils::GetUTF8(modified), pool),
                                                    nullptr, nullptr, SVN_APR_LOCALE_CHARSET, nullptr, true, contextsize, nullptr, nullptr, pool);
                svn_stream_close(stream);
            }
        }
        apr_file_close(outFile);
    }
    if (err)
    {
        if (bShowError)
            AfxMessageBox(CAppUtils::GetErrorString(err), MB_ICONERROR);
        svn_error_clear(err);
        svn_pool_destroy(pool);
        return false;
    }
    svn_pool_destroy(pool);
    return true;
}

CString CAppUtils::GetErrorString(svn_error_t* err)
{
    CString msg;
    CString temp;

    if (err)
    {
        char         errBuf[256] = {0};
        svn_error_t* errPtr      = err;
        if (errPtr->message)
            msg = CUnicodeUtils::GetUnicode(errPtr->message);
        else
        {
            /* Is this a Subversion-specific error code? */
            if ((errPtr->apr_err > APR_OS_START_USEERR) && (errPtr->apr_err <= APR_OS_START_CANONERR))
                msg = svn_strerror(errPtr->apr_err, errBuf, _countof(errBuf));
            /* Otherwise, this must be an APR error code. */
            else
            {
                svn_error_t* tempErr   = nullptr;
                const char*  errString = nullptr;
                tempErr                = svn_utf_cstring_to_utf8(&errString, apr_strerror(errPtr->apr_err, errBuf, _countof(errBuf) - 1), errPtr->pool);
                if (tempErr)
                {
                    svn_error_clear(tempErr);
                    msg = L"Can't recode error string from APR";
                }
                else
                {
                    msg = CUnicodeUtils::GetUnicode(errString);
                }
            }
        }
        while (errPtr->child)
        {
            errPtr = errPtr->child;
            msg += L"\n";
            if (errPtr->message)
                temp = CUnicodeUtils::GetUnicode(errPtr->message);
            else
            {
                /* Is this a Subversion-specific error code? */
                if ((errPtr->apr_err > APR_OS_START_USEERR) && (errPtr->apr_err <= APR_OS_START_CANONERR))
                    temp = svn_strerror(errPtr->apr_err, errBuf, _countof(errBuf));
                /* Otherwise, this must be an APR error code. */
                else
                {
                    svn_error_t* tempErr   = nullptr;
                    const char*  errString = nullptr;
                    tempErr                = svn_utf_cstring_to_utf8(&errString, apr_strerror(errPtr->apr_err, errBuf, _countof(errBuf) - 1), errPtr->pool);
                    if (tempErr)
                    {
                        svn_error_clear(tempErr);
                        temp = L"Can't recode error string from APR";
                    }
                    else
                    {
                        temp = CUnicodeUtils::GetUnicode(errString);
                    }
                }
            }
            msg += temp;
        }
        return msg;
    }
    return L"";
}

bool CAppUtils::HasClipboardFormat(UINT format)
{
    if (OpenClipboard(nullptr))
    {
        UINT enumFormat = 0;
        do
        {
            if (enumFormat == format)
            {
                CloseClipboard();
                return true;
            }
        } while ((enumFormat = EnumClipboardFormats(enumFormat)) != 0);
        CloseClipboard();
    }
    return false;
}

COLORREF CAppUtils::IntenseColor(long scale, COLORREF col)
{
    // if the color is already dark (gray scale below 127),
    // then lighten the color by 'scale', otherwise darken it
    int Gray = (static_cast<int>(GetRValue(col)) + GetGValue(col) + GetBValue(col)) / 3;
    if (Gray > 127)
    {
        long red   = MulDiv(GetRValue(col), (255 - scale), 255);
        long green = MulDiv(GetGValue(col), (255 - scale), 255);
        long blue  = MulDiv(GetBValue(col), (255 - scale), 255);

        return RGB(red, green, blue);
    }
    long R = MulDiv(255 - GetRValue(col), scale, 255) + GetRValue(col);
    long G = MulDiv(255 - GetGValue(col), scale, 255) + GetGValue(col);
    long B = MulDiv(255 - GetBValue(col), scale, 255) + GetBValue(col);

    return RGB(R, G, B);
}
