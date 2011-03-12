// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2010-2011 - TortoiseSVN

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


void SVNPatch::notify( void *baton, const svn_wc_notify_t *notify, apr_pool_t * /*pool*/ )
{
    SVNPatch * pThis = (SVNPatch*)baton;
    if (pThis && notify)
    {
        PathRejects * pInfo = &pThis->m_filePaths[pThis->m_filePaths.size()-1];
        if ((notify->action == svn_wc_notify_skip)||(notify->action == svn_wc_notify_patch_rejected_hunk))
        {
            pInfo->rejects++;
        }
        if (notify->action != svn_wc_notify_add)
        {
            CString abspath = CUnicodeUtils::GetUnicode(notify->path);
            if (abspath.Left(pThis->m_targetpath.GetLength()).Compare(pThis->m_targetpath) == 0)
                pThis->m_testPath = abspath.Mid(pThis->m_targetpath.GetLength());
            else
                pThis->m_testPath = abspath;
        }
        pInfo->content = pInfo->content || (notify->content_state != svn_wc_notify_state_unknown);
        pInfo->props   = pInfo->props   || (notify->prop_state    != svn_wc_notify_state_unknown);

        if (((notify->action == svn_wc_notify_patch)||(notify->action == svn_wc_notify_add))&&(pThis->m_pProgDlg))
        {
            pThis->m_pProgDlg->FormatPathLine(2, IDS_PATCH_PATHINGFILE, (LPCTSTR)CUnicodeUtils::GetUnicode(notify->path));
        }
    }
}

svn_error_t * SVNPatch::patch_func( void *baton, svn_boolean_t * filtered, const char * canon_path_from_patchfile, 
                                    const char *patch_abspath, 
                                    const char *reject_abspath, 
                                    apr_pool_t * /*scratch_pool*/ )
{
    SVNPatch * pThis = (SVNPatch*)baton;
    if (pThis)
    {
        CString abspath = CUnicodeUtils::GetUnicode(canon_path_from_patchfile);
        PathRejects pr;
        pr.path = PathIsRelative(abspath) ? abspath : abspath.Mid(pThis->m_targetpath.GetLength());
        pr.rejects = 0;
        pr.resultPath = CUnicodeUtils::GetUnicode(patch_abspath);
        pr.resultPath.Replace('/', '\\');
        pr.rejectsPath = CUnicodeUtils::GetUnicode(reject_abspath);
        pr.rejectsPath.Replace('/', '\\');
        pr.content = false;
        pr.props = false;
        // only add this entry if it hasn't been added already
        bool bExists = false;
        for (auto it = pThis->m_filePaths.rbegin(); it != pThis->m_filePaths.rend(); ++it)
        {
            if (it->path.Compare(pr.path) == 0)
            {
                bExists = true;
                break;
            }
        }
        if (!bExists)
            pThis->m_filePaths.push_back(pr);
        pThis->m_nRejected = 0;
        *filtered = false;

        CTempFiles::Instance().AddFileToRemove(pr.resultPath);
        CTempFiles::Instance().AddFileToRemove(pr.rejectsPath);
    }
    return NULL;
}

svn_error_t * SVNPatch::patchfile_func( void *baton, svn_boolean_t * filtered, const char * canon_path_from_patchfile, 
                                        const char * /*patch_abspath*/, 
                                        const char * /*reject_abspath*/, 
                                        apr_pool_t * /*scratch_pool*/ )
{
    SVNPatch * pThis = (SVNPatch*)baton;
    if (pThis)
    {
        CString relpath = CUnicodeUtils::GetUnicode(canon_path_from_patchfile);
        relpath = PathIsRelative(relpath) ? relpath : relpath.Mid(pThis->m_targetpath.GetLength());
        *filtered = relpath.CompareNoCase(pThis->m_filetopatch) != 0;
    }
    return NULL;
}

int SVNPatch::Init( const CString& patchfile, const CString& targetpath, CProgressDlg *pPprogDlg )
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

    if (pPprogDlg)
    {
        pPprogDlg->SetTitle(IDS_APPNAME);
        pPprogDlg->FormatNonPathLine(1, IDS_PATCH_PROGTITLE);
        pPprogDlg->SetShowProgressBar(false);
        pPprogDlg->ShowModeless(AfxGetMainWnd());
        m_pProgDlg = pPprogDlg;
    }

    m_filePaths.clear();
    m_nRejected = 0;
    m_nStrip = 0;
    err = svn_client_patch(CUnicodeUtils::GetUTF8(m_patchfile),     // patch_abspath
                           CUnicodeUtils::GetUTF8(m_targetpath),    // local_abspath
                           true,                                    // dry_run
                           m_nStrip,                                // strip_count
                           false,                                   // reverse
                           true,                                    // ignore_whitespace
                           false,                                   // remove_tempfiles
                           patch_func,                              // patch_func
                           this,                                    // patch_baton
                           ctx,                                     // client context
                           m_pool, scratchpool);

    m_pProgDlg = NULL;
    apr_pool_destroy(scratchpool);

    if (err)
    {
        m_errorStr = GetErrorMessage(err);
        m_filePaths.clear();
        svn_error_clear(err);
        return 0;
    }
    
    if ((m_nRejected > ((int)m_filePaths.size() / 3)) && !m_testPath.IsEmpty())
    {
        m_nStrip++;
        bool found = false;
        for (m_nStrip = 0; m_nStrip < STRIP_LIMIT; ++m_nStrip)
        {
            for (std::vector<PathRejects>::iterator it = m_filePaths.begin(); it != m_filePaths.end(); ++it)
            {
                if (Strip(it->path).IsEmpty())
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

        m_filePaths.clear();
        m_nRejected = 0;
        m_nStrip = 0;
        err = svn_client_patch(CUnicodeUtils::GetUTF8(m_patchfile),     // patch_abspath
                               CUnicodeUtils::GetUTF8(m_targetpath),    // local_abspath
                               true,                                    // dry_run
                               m_nStrip,                                // strip_count
                               false,                                   // reverse
                               true,                                    // ignore_whitespace
                               false,                                   // remove_tempfiles
                               patch_func,                              // patch_func
                               this,                                    // patch_baton
                               ctx,                                     // client context
                               m_pool, scratchpool);

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

bool SVNPatch::PatchPath( const CString& path )
{
    svn_error_t *               err         = NULL;
    apr_pool_t *                scratchpool = NULL;
    svn_client_ctx_t *          ctx         = NULL;

    m_errorStr.Empty();

    m_patchfile.Replace('\\', '/');
    m_targetpath.Replace('\\', '/');

    m_filetopatch = path.Mid(m_targetpath.GetLength()+1);
    m_filetopatch.Replace('\\', '/');

    apr_pool_create_ex(&scratchpool, m_pool, abort_on_pool_failure, NULL);
    svn_error_clear(svn_client_create_context(&ctx, scratchpool));

    m_nRejected = 0;
    m_nStrip = 0;
    err = svn_client_patch(CUnicodeUtils::GetUTF8(m_patchfile),     // patch_abspath
        CUnicodeUtils::GetUTF8(m_targetpath),    // local_abspath
        false,                                   // dry_run
        m_nStrip,                                // strip_count
        false,                                   // reverse
        true,                                    // ignore_whitespace
        true,                                    // remove_tempfiles
        patchfile_func,                          // patch_func
        this,                                    // patch_baton
        ctx,                                     // client context
        m_pool, scratchpool);

    apr_pool_destroy(scratchpool);

    if (err)
    {
        m_errorStr = GetErrorMessage(err);
        svn_error_clear(err);
        return false;
    }

    return true;
}


int SVNPatch::GetPatchResult(const CString& sPath, CString& sSavePath, CString& sRejectPath) const
{
    for (std::vector<PathRejects>::const_iterator it = m_filePaths.begin(); it != m_filePaths.end(); ++it)
    {
        if (it->path.CompareNoCase(sPath)==0)
        {
            sSavePath = it->resultPath;
            if (it->rejects > 0)
                sRejectPath = it->rejectsPath;
            else
                sRejectPath.Empty();
            return it->rejects;
        }
    }
    return -1;
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

    if (Err != NULL)
    {
        svn_error_t * ErrPtr = Err;
        msg = GetErrorMessageForNode(ErrPtr);
        while (ErrPtr->child)
        {
            ErrPtr = ErrPtr->child;
            msg += _T("\n");
            temp = GetErrorMessageForNode(ErrPtr);
            msg += temp;
        }
    }
    return msg;
}

CString	 SVNPatch::GetErrorMessageForNode(svn_error_t* Err) const
{
    CString msg;
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
            {
                msg = svn_strerror (ErrPtr->apr_err, errbuf, _countof (errbuf));
            }
            /* Otherwise, this must be an APR error code. */
            else
            {
                svn_error_t *temp_err = NULL;
                const char * err_string = NULL;
                temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, _countof (errbuf)-1), ErrPtr->pool);
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
    }
    return msg;
}

