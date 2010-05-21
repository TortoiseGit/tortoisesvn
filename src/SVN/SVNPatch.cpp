// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2010 - TortoiseSVN

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
#include "StdAfx.h"
#include "Resource.h"
#include "SVNPatch.h"
#include "UnicodeUtils.h"
#include "ProgressDlg.h"
#include "DirFileEnum.h"
#include "SVNAdminDir.h"
#include "StringUtils.h"

#pragma warning(push)
#include "svn_dso.h"
#include "svn_utf.h"
#pragma warning(pop)


#define STRIP_LIMIT 10

int SVNPatch::abort_on_pool_failure (int /*retcode*/)
{
    abort ();
    return -1;
}

SVNPatch::SVNPatch() 
    : m_pool(NULL)
    , m_nStrip(0)
    , m_bInit(false)
{
    apr_initialize();
    svn_dso_initialize2();
    g_SVNAdminDir.Init();
    apr_pool_create_ex (&m_pool, NULL, abort_on_pool_failure, NULL);
}

SVNPatch::~SVNPatch()
{
    apr_pool_destroy (m_pool); 
    g_SVNAdminDir.Close();
    apr_terminate();
}


void SVNPatch::notify( void *baton, const svn_wc_notify_t *notify, apr_pool_t *pool )
{
    SVNPatch * pThis = (SVNPatch*)baton;
    if ((pThis)&&(pThis->m_bInit)&&(notify))
    {
        if (notify->action == svn_wc_notify_skip)
        {
            pThis->m_nRejected++;
        }
    }
    else if (pThis && notify)
    {
        if ((notify->action == svn_wc_notify_skip)||(notify->action == svn_wc_notify_patch_rejected_hunk))
        {
            pThis->m_nRejected++;
            //pThis->m_bSuccessfullyPatched = false;
        }
        if (notify->action != svn_wc_notify_add)
        {
            CString abspath = CUnicodeUtils::GetUnicode(notify->path);
            if (abspath.Left(pThis->m_targetpath.GetLength()).Compare(pThis->m_targetpath) == 0)
                pThis->m_testPath = abspath.Mid(pThis->m_targetpath.GetLength());
            else
                pThis->m_testPath = abspath;
        }
    }
}

svn_error_t * SVNPatch::patch_func( void *baton, const char * local_abspath, 
                                    const char *patch_abspath, 
                                    const char *reject_abspath, 
                                    apr_pool_t *scratch_pool )
{
    SVNPatch * pThis = (SVNPatch*)baton;
    if ((pThis)&&(pThis->m_bInit))
    {
        CString abspath = CUnicodeUtils::GetUnicode(local_abspath);
        if (abspath.Left(pThis->m_targetpath.GetLength()).Compare(pThis->m_targetpath) == 0)
            pThis->m_filePaths.push_back(abspath.Mid(pThis->m_targetpath.GetLength()));
        else
            pThis->m_filePaths.push_back(abspath);
        CString sFile = CUnicodeUtils::GetUnicode(patch_abspath);
        if (PathIsRelative(sFile))
            sFile = pThis->m_targetpath + _T("\\") + sFile;
        sFile.Replace('/', '\\');
        DeleteFile(sFile);
        sFile = CUnicodeUtils::GetUnicode(reject_abspath);
        if (PathIsRelative(sFile))
            sFile = pThis->m_targetpath + _T("\\") + sFile;
        sFile.Replace('/', '\\');
        DeleteFile(sFile);
    }
    else if (pThis)
    {
        pThis->m_patchedPath = CUnicodeUtils::GetUnicode(patch_abspath);
        if (pThis->m_bDryRun)
        {
            CString sFile = CUnicodeUtils::GetUnicode(patch_abspath);
            if (PathIsRelative(sFile))
                sFile = pThis->m_targetpath + _T("\\") + sFile;
            sFile.Replace('/', '\\');
            DeleteFile(sFile);
            sFile = CUnicodeUtils::GetUnicode(reject_abspath);
            if (PathIsRelative(sFile))
                sFile = pThis->m_targetpath + _T("\\") + sFile;
            sFile.Replace('/', '\\');
            DeleteFile(sFile);
        }
    }
    return NULL;
}

int SVNPatch::Init( const CString& patchfile, const CString& targetpath )
{
    svn_error_t *               err         = NULL;
    apr_pool_t *                scratchpool = NULL;
    svn_client_ctx_t *          ctx         = NULL;

    m_errorStr.Empty();
    m_patchfile = patchfile;
    m_targetpath = targetpath;
    m_testPath.Empty();

    m_patchfile.Replace('\\', '/');
    m_targetpath.Replace('\\', '/');

    apr_pool_create_ex(&scratchpool, m_pool, abort_on_pool_failure, NULL);
    svn_error_clear(svn_client_create_context(&ctx, scratchpool));
    ctx->notify_func2 = notify;
    ctx->notify_baton2 = this;

    m_bInit = true;
    m_filePaths.clear();
    m_nRejected = 0;
    m_nStrip = 0;
    err = svn_client_patch(CUnicodeUtils::GetUTF8(m_patchfile), CUnicodeUtils::GetUTF8(m_targetpath),
                           true, m_nStrip, false, NULL, NULL, true, true, patch_func, this, ctx,
                           m_pool, scratchpool);

    m_bInit = false;
    apr_pool_destroy(scratchpool);

    if (err)
    {
        m_errorStr = GetErrorMessage(err);
        m_filePaths.clear();
        svn_error_clear(err);
        return 0;
    }
    
    if ((m_nRejected > (m_filePaths.size() / 3)) && !m_testPath.IsEmpty())
    {
        m_nStrip++;
        bool found = false;
        for (m_nStrip = 0; m_nStrip < STRIP_LIMIT; ++m_nStrip)
        {
            for (std::vector<CString>::iterator it = m_filePaths.begin(); it != m_filePaths.end(); ++it)
            {
                if (Strip(*it).IsEmpty())
                {
                    found = true;
                    m_nStrip--;
                    break;
                }
            }
            if (found)
                break;
        }
    }

    if (m_nStrip == STRIP_LIMIT)
        m_filePaths.clear();
    else if (m_nStrip > 0)
    {
        apr_pool_create_ex(&scratchpool, m_pool, abort_on_pool_failure, NULL);
        svn_error_clear(svn_client_create_context(&ctx, scratchpool));
        ctx->notify_func2 = notify;
        ctx->notify_baton2 = this;

        m_bInit = true;
        m_bDryRun = true;
        m_filePaths.clear();
        m_nRejected = 0;
        m_nStrip = 0;
        err = svn_client_patch(CUnicodeUtils::GetUTF8(m_patchfile), CUnicodeUtils::GetUTF8(m_targetpath),
            true, m_nStrip, false, NULL, NULL, true, true, patch_func, this, ctx,
            m_pool, scratchpool);

        m_bInit = false;
        apr_pool_destroy(scratchpool);

        if (err)
        {
            m_errorStr = GetErrorMessage(err);
            m_filePaths.clear();
            svn_error_clear(err);
        }
    }

    return (int)m_filePaths.size();
}

bool SVNPatch::PatchFile(const CString& sPath, bool dryrun, CString& sSavePath)
{
    svn_error_t *               err         = NULL;
    apr_pool_t *                scratchpool = NULL;
    svn_client_ctx_t *          ctx         = NULL;

    m_errorStr.Empty();

    apr_pool_create_ex(&scratchpool, m_pool, abort_on_pool_failure, NULL);
    svn_error_clear(svn_client_create_context(&ctx, scratchpool));
    ctx->notify_func2 = notify;
    ctx->notify_baton2 = this;
    
    m_bInit = false;
    m_nRejected = 0;
    m_bSuccessfullyPatched = true;

    apr_array_header_t * arr = apr_array_make (scratchpool, 1, sizeof(const char *));
    const char * c = apr_pstrdup(scratchpool, (LPCSTR)CUnicodeUtils::GetUTF8(sPath));
    (*((const char **) apr_array_push(arr))) = c;

    m_bDryRun = dryrun;
    err = svn_client_patch(CUnicodeUtils::GetUTF8(m_patchfile), CUnicodeUtils::GetUTF8(m_targetpath),
                           true, m_nStrip, false, arr, NULL, true, true, patch_func, this, ctx,
                           m_pool, scratchpool);

    apr_pool_destroy(scratchpool);

    if (err)
    {
        m_errorStr = GetErrorMessage(err);
        svn_error_clear(err);
        return false;
    }
    if (m_bSuccessfullyPatched)
    {
        sSavePath = m_patchedPath;
    }
    return m_bSuccessfullyPatched;
}

CString SVNPatch::CheckPatchPath( const CString& path )
{
    // first check if the path already matches
    if (CountMatches(path) > (GetNumberOfFiles()/3))
        return path;

    CProgressDlg progress;
    CString tmp;
    progress.SetTitle(IDS_PATCH_SEARCHPATHTITLE);
    progress.SetShowProgressBar(false);
    tmp.LoadString(IDS_PATCH_SEARCHPATHLINE1);
    progress.SetLine(1, tmp);
    progress.ShowModeless(AfxGetMainWnd());

    // now go up the tree and try again
    CString upperpath = path;
    while (upperpath.ReverseFind('\\')>0)
    {
        upperpath = upperpath.Left(upperpath.ReverseFind('\\'));
        progress.SetLine(2, upperpath, true);
        if (progress.HasUserCancelled())
            return path;
        if (CountMatches(upperpath) > (GetNumberOfFiles()/3))
            return upperpath;
    }
    // still no match found. So try sub folders
    bool isDir = false;
    CString subpath;
    CDirFileEnum filefinder(path);
    while (filefinder.NextFile(subpath, &isDir))
    {
        if (progress.HasUserCancelled())
            return path;
        if (!isDir)
            continue;
        if (g_SVNAdminDir.IsAdminDirPath(subpath))
            continue;
        progress.SetLine(2, subpath, true);
        if (CountMatches(subpath) > (GetNumberOfFiles()/3))
            return subpath;
    }

    // if a patch file only contains newly added files
    // we can't really find the correct path.
    // But: we can compare paths strings without the filenames
    // and check if at least those match
    upperpath = path;
    while (upperpath.ReverseFind('\\')>0)
    {
        upperpath = upperpath.Left(upperpath.ReverseFind('\\'));
        progress.SetLine(2, upperpath, true);
        if (progress.HasUserCancelled())
            return path;
        if (CountDirMatches(upperpath) > (GetNumberOfFiles()/3))
            return upperpath;
    }

    return path;
}

int SVNPatch::CountMatches( const CString& path ) const
{
    int matches = 0;
    for (int i=0; i<GetNumberOfFiles(); ++i)
    {
        CString temp = GetStrippedPath(i);
        temp.Replace('/', '\\');
        if (PathIsRelative(temp))
            temp = path + _T("\\")+ temp;
        if (PathFileExists(temp))
            matches++;
    }
    return matches;
}

int SVNPatch::CountDirMatches( const CString& path ) const
{
    int matches = 0;
    for (int i=0; i<GetNumberOfFiles(); ++i)
    {
        CString temp = GetStrippedPath(i);
        temp.Replace('/', '\\');
        if (PathIsRelative(temp))
            temp = path + _T("\\")+ temp;
        // remove the filename
        temp = temp.Left(temp.ReverseFind('\\'));
        if (PathFileExists(temp))
            matches++;
    }
    return matches;
}

CString SVNPatch::GetStrippedPath( int nIndex ) const
{
    if (nIndex < 0)
        return _T("");
    if (nIndex < (int)m_filePaths.size())
    {
        CString filepath = Strip(GetFilePath(nIndex));
        return filepath;
    }

    return _T("");
}

CString SVNPatch::Strip( const CString& filename ) const
{
    CString s = filename;
    if ( m_nStrip>0 )
    {
        // Remove windows drive letter "c:"
        if ( s.GetLength()>2 && s[1]==':')
        {
            s = s.Mid(2);
        }

        for (int nStrip=1;nStrip<=m_nStrip;nStrip++)
        {
            // "/home/ts/my-working-copy/dir/file.txt"
            //  "home/ts/my-working-copy/dir/file.txt"
            //       "ts/my-working-copy/dir/file.txt"
            //          "my-working-copy/dir/file.txt"
            //                          "dir/file.txt"
            int p = s.FindOneOf(_T("/\\"));
            if (p < 0)
            {
                s.Empty();
                break;
            }
            s = s.Mid(p+1);
        }
    }
    return s;
}

CString SVNPatch::GetErrorMessage(svn_error_t * Err) const
{
    CString msg;
    CString temp;
    char errbuf[256];

    if (Err != NULL)
    {
        svn_error_t * ErrPtr = Err;
        if (ErrPtr->message)
            msg = CUnicodeUtils::GetUnicode(ErrPtr->message);
        else
        {
            /* Is this a Subversion-specific error code? */
            if ((ErrPtr->apr_err > APR_OS_START_USEERR)
                && (ErrPtr->apr_err <= APR_OS_START_CANONERR))
                msg = svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf));
            /* Otherwise, this must be an APR error code. */
            else
            {
                svn_error_t *temp_err = NULL;
                const char * err_string = NULL;
                temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
                if (temp_err)
                {
                    svn_error_clear (temp_err);
                    msg = _T("Can't recode error string from APR");
                }
                else
                {
                    msg = CUnicodeUtils::GetUnicode(err_string);
                }
            }
        }
        msg = CStringUtils::LinesWrap(msg, 80);
        while (ErrPtr->child)
        {
            ErrPtr = ErrPtr->child;
            msg += _T("\n");
            if (ErrPtr->message)
                temp = CUnicodeUtils::GetUnicode(ErrPtr->message);
            else
            {
                /* Is this a Subversion-specific error code? */
                if ((ErrPtr->apr_err > APR_OS_START_USEERR)
                    && (ErrPtr->apr_err <= APR_OS_START_CANONERR))
                    temp = svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf));
                /* Otherwise, this must be an APR error code. */
                else
                {
                    svn_error_t *temp_err = NULL;
                    const char * err_string = NULL;
                    temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
                    if (temp_err)
                    {
                        svn_error_clear (temp_err);
                        temp = _T("Can't recode error string from APR");
                    }
                    else
                    {
                        temp = CUnicodeUtils::GetUnicode(err_string);
                    }
                }
            }
            temp = CStringUtils::LinesWrap(temp, 80);
            msg += temp;
        }
        return msg;
    }
    return _T("");
}


