// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2016, 2018, 2021 - TortoiseSVN

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
#include "SVNBase.h"
#include "TSVNPath.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include <Commctrl.h>

#pragma comment(lib, "Comctl32.lib")

#ifdef HAVE_APPUTILS
#    include "AppUtils.h"
#endif

#include "../TortoiseShell/resource.h"

#pragma warning(push)
#include "svn_client.h"
#pragma warning(pop)

#ifndef TSVN_STATICSHELL
// ReSharper disable once CppInconsistentNaming
extern "C" void TSVN_ClearLastUsedAuthCache();
#endif

SVNBase::SVNBase()
    : m_err(nullptr)
    , m_pCtx(nullptr)
{
}

SVNBase::~SVNBase()
{
}

#ifdef CSTRING_AVAILABLE
CString SVNBase::GetLastErrorMessage(int wrap /* = 80 */) const
{
    CString msg = GetErrorString(m_err, wrap);
    if (!m_postCommitErr.IsEmpty())
    {
#    ifdef _MFC_VER
        msg += L"\n" + CStringUtils::LinesWrap(m_postCommitErr, wrap);
#    else
        msg += L"\n" + CStringUtils::LinesWrap(m_postCommitErr, wrap);
#    endif
    }
    return msg;
}

CString SVNBase::GetErrorString(svn_error_t* err, int wrap /* = 80 */)
{
    if (err != nullptr)
    {
        CString      temp;
        CString      msg;
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
                const char*  errString = nullptr;
                svn_error_t* tempErr   = svn_utf_cstring_to_utf8(&errString, apr_strerror(errPtr->apr_err, errBuf, _countof(errBuf) - 1), errPtr->pool);
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
        msg = CStringUtils::LinesWrap(msg, wrap);
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
                    const char*  errString = nullptr;
                    svn_error_t* tempErr   = svn_utf_cstring_to_utf8(&errString, apr_strerror(errPtr->apr_err, errBuf, _countof(errBuf) - 1), errPtr->pool);
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
            temp = CStringUtils::LinesWrap(temp, wrap);
            msg += temp;
        }
        temp.Empty();
        if (svn_error_find_cause(err, SVN_ERR_WC_LOCKED) && (err->apr_err != SVN_ERR_WC_CLEANUP_REQUIRED))
            temp.LoadString(IDS_SVNERR_RUNCLEANUP);

#    ifdef IDS_SVNERR_CHECKPATHORURL
        // add some hint text for some of the error messages
        switch (err->apr_err)
        {
            case SVN_ERR_BAD_FILENAME:
            case SVN_ERR_BAD_URL:
                // please check the path or URL you've entered.
                temp.LoadString(IDS_SVNERR_CHECKPATHORURL);
                break;
            case SVN_ERR_WC_CLEANUP_REQUIRED:
                // do a "cleanup"
                temp.LoadString(IDS_SVNERR_RUNCLEANUP);
                break;
            case SVN_ERR_WC_NOT_UP_TO_DATE:
            case SVN_ERR_FS_TXN_OUT_OF_DATE:
                // do an update first
                temp.LoadString(IDS_SVNERR_UPDATEFIRST);
                break;
            case SVN_ERR_WC_CORRUPT:
            case SVN_ERR_WC_CORRUPT_TEXT_BASE:
                // do a "cleanup". If that doesn't work you need to do a fresh checkout.
                temp.LoadString(IDS_SVNERR_CLEANUPORFRESHCHECKOUT);
                break;
            case SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED:
                temp.LoadString(IDS_SVNERR_POSTCOMMITHOOKFAILED);
                break;
            case SVN_ERR_REPOS_POST_LOCK_HOOK_FAILED:
                temp.LoadString(IDS_SVNERR_POSTLOCKHOOKFAILED);
                break;
            case SVN_ERR_REPOS_POST_UNLOCK_HOOK_FAILED:
                temp.LoadString(IDS_SVNERR_POSTUNLOCKHOOKFAILED);
                break;
            case SVN_ERR_REPOS_HOOK_FAILURE:
                temp.LoadString(IDS_SVNERR_HOOKFAILED);
                break;
            case SVN_ERR_SQLITE_BUSY:
                temp.LoadString(IDS_SVNERR_SQLITEBUSY);
                break;
            default:
                break;
        }
        if ((err->apr_err == SVN_ERR_FS_PATH_NOT_LOCKED) ||
            (err->apr_err == SVN_ERR_FS_NO_SUCH_LOCK) ||
            (err->apr_err == SVN_ERR_RA_NOT_LOCKED))
        {
            // the lock has already been broken from another working copy
            temp.LoadString(IDS_SVNERR_UNLOCKFAILEDNOLOCK);
        }
        else if ((err->apr_err != SVN_ERR_REPOS_HOOK_FAILURE) &&
                 SVN_ERR_IS_UNLOCK_ERROR(err))
        {
            // if you want to break the lock, use the "check for modifications" dialog
            temp.LoadString(IDS_SVNERR_UNLOCKFAILED);
        }
        if (!temp.IsEmpty())
        {
            msg += L"\n" + temp;
        }
#    endif
        return msg;
    }
    return L"";
}

int SVNBase::ShowErrorDialog(HWND hParent) const
{
    return ShowErrorDialog(hParent, CTSVNPath());
}

int SVNBase::ShowErrorDialog(HWND hParent, const CTSVNPath& wcPath, const CString& sErr) const
{
    UNREFERENCED_PARAMETER(wcPath);
    int ret = -1;

    CString sError = m_err ? GetErrorString(m_err) : m_postCommitErr;
    if (!sErr.IsEmpty())
        sError = sErr;

    CString sCleanup     = CString(MAKEINTRESOURCE(IDS_RUNCLEANUPNOW));
    CString sClose       = CString(MAKEINTRESOURCE(IDS_CLOSE));
    CString sInstruction = CString(MAKEINTRESOURCE(IDS_SVNREPORTEDANERROR));

    TASKDIALOGCONFIG tConfig   = {0};
    tConfig.cbSize             = sizeof(TASKDIALOGCONFIG);
    tConfig.hwndParent         = hParent;
    tConfig.dwFlags            = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT;
    tConfig.dwCommonButtons    = TDCBF_CLOSE_BUTTON;
    tConfig.pszWindowTitle     = L"TortoiseSVN";
    tConfig.pszMainIcon        = TD_ERROR_ICON;
    tConfig.pszMainInstruction = sInstruction;
    tConfig.pszContent         = static_cast<LPCWSTR>(sError);
#    ifdef HAVE_APPUTILS
    TASKDIALOG_BUTTON aCustomButtons[2];
    aCustomButtons[0].nButtonID     = 1000;
    aCustomButtons[0].pszButtonText = sCleanup;
    aCustomButtons[1].nButtonID     = IDOK;
    aCustomButtons[1].pszButtonText = sClose;
    if (m_err && (m_err->apr_err == SVN_ERR_WC_CLEANUP_REQUIRED) && (!wcPath.IsEmpty()))
    {
        tConfig.dwCommonButtons = 0;
        tConfig.dwFlags |= TDF_USE_COMMAND_LINKS;
        tConfig.pButtons = aCustomButtons;
        tConfig.cButtons = _countof(aCustomButtons);
    }
#    endif
    TaskDialogIndirect(&tConfig, &ret, nullptr, nullptr);
#    ifdef HAVE_APPUTILS
    if (ret == 1000)
    {
        // run cleanup
        CString sCmd;
        sCmd.Format(L"/command:cleanup /path:\"%s\" /cleanup /nodlg /hwnd:%p",
                    wcPath.GetDirectory().GetWinPath(), static_cast<void*>(hParent));
        CAppUtils::RunTortoiseProc(sCmd);
    }
#    endif
    return ret;
}

void SVNBase::ClearCAPIAuthCacheOnError() const
{
#    ifndef TSVN_STATICSHELL
    if (m_err != nullptr)
    {
        if ((SVN_ERROR_IN_CATEGORY(m_err->apr_err, SVN_ERR_AUTHN_CATEGORY_START)) ||
            (SVN_ERROR_IN_CATEGORY(m_err->apr_err, SVN_ERR_AUTHZ_CATEGORY_START)) ||
            (m_err->apr_err == SVN_ERR_RA_DAV_FORBIDDEN) ||
            (m_err->apr_err == SVN_ERR_RA_CANNOT_CREATE_SESSION))
            TSVN_ClearLastUsedAuthCache();
    }
#    endif
}

#endif
