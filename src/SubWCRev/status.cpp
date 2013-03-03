// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013 - TortoiseSVN

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

#pragma warning(push)
#include <apr_pools.h>
#include "svn_client.h"
#include "svn_wc.h"
#include "svn_dirent_uri.h"
#include "svn_utf.h"
#include "svn_props.h"
#pragma warning(pop)
#include "SubWCRev.h"
#include "registry.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include <algorithm>

#pragma warning(push)
#pragma warning(disable:4127)   //conditional expression is constant (cause of SVN_ERR)

CRegStdString regTagsPattern = CRegStdString(_T("Software\\TortoiseSVN\\RevisionGraph\\TagsPattern"), _T("tags"));

// Copy the URL from src to dest, unescaping on the fly.
void UnescapeCopy(const char * root, const char * src, char * dest, int buf_len)
{
    const char * pszSource = root;
    char * pszDest = dest;
    int len = 0;

    // under VS.NET2k5 strchr() wants this to be a non-const array :/

    static char szHex[] = "0123456789ABCDEF";

    // Unescape special characters. The number of characters
    // in the *pszDest is assumed to be <= the number of characters
    // in pszSource (they are both the same string anyway)

    bool bRoot = true;
    while ((*pszSource != '\0') && (++len < buf_len))
    {
        if (*pszSource == '%')
        {
            // The next two chars following '%' should be digits
            if ( *(pszSource + 1) == '\0' ||
                *(pszSource + 2) == '\0' )
            {
                // nothing left to do
                break;
            }

            char nValue = '?';
            char * pszLow = NULL;
            char * pszHigh = NULL;
            pszSource++;

            char up = (char) toupper(*pszSource);
            pszHigh = strchr(szHex, up);

            if (pszHigh != NULL)
            {
                pszSource++;
                up = (char) toupper(*pszSource);
                pszLow = strchr(szHex, up);

                if (pszLow != NULL)
                {
                    nValue = (char) (((pszHigh - szHex) << 4) +
                        (pszLow - szHex));
                }
            }
            *pszDest++ = nValue;
        }
        else
            *pszDest++ = *pszSource;

        pszSource++;
        if ((bRoot)&&(*pszSource == 0))
        {
            if (*pszDest != '/')
                *pszDest++ = '/';
            pszSource = src;
            bRoot = false;
        }
    }

    *pszDest = '\0';
}

tstring Tokenize(const _TCHAR* str, const _TCHAR* delim, tstring::size_type& iStart)
{
    const _TCHAR* pstr = str + iStart;
    const _TCHAR* r = _tcsstr(pstr, delim);
    tstring::size_type dlen = _tcslen(delim);

    while( r )
    {
        if( r > pstr )
        {
            iStart = tstring::size_type(r - str) + dlen;
            return tstring(pstr, tstring::size_type(r - pstr));
        }
        pstr = r + dlen;
        r = _tcsstr(pstr, delim);
    }

    if( _tcslen(pstr) > 0)
    {
        iStart = tstring::size_type(_tcslen(str));
        return tstring(pstr);
    }
    return tstring();
}

bool IsTaggedVersion(const char * url)
{
    bool isTag = false;

    if (!url)
    {
        return false;
    }

    tstring urllower = Utf8ToWide(url);
    std::transform(urllower.begin(), urllower.end(), urllower.begin(), tolower);

    // look for the tag pattern inside in the url
    tstring sTags = regTagsPattern;
    tstring::size_type pos = 0;
    tstring temp;
    while (!isTag)
    {
        temp = Tokenize(sTags.c_str(), _T(";"), pos);
        if (!temp.length())
            break;

        tstring::size_type urlpos = 0;
        tstring temp2;
        for(;;)
        {
            temp2 = Tokenize(urllower.c_str(), _T("/"), urlpos);
            if (!temp2.length())
                break;

            if (_tcswildcmp(temp.c_str(), temp2.c_str()))
            {
                isTag = true;
                break;
            }
        }
    }
    return isTag;
}

svn_error_t * getfirststatus(void * baton, const char * path, const svn_client_status_t * status, apr_pool_t * pool)
{
    SubWCRev_StatusBaton_t * sb = (SubWCRev_StatusBaton_t *) baton;
    if((NULL == status) || (NULL == sb) || (NULL == sb->SubStat))
    {
        return SVN_NO_ERROR;
    }
    if ((status->repos_relpath)&&(sb->SubStat->Url[0] == 0))
    {
        UnescapeCopy(status->repos_root_url, status->repos_relpath, sb->SubStat->Url, URL_BUF);
        sb->SubStat->bIsTagged = IsTaggedVersion(sb->SubStat->Url);
    }
    if (status->kind == svn_node_file)
    {
        const svn_string_t * value = NULL;
        svn_error_t * e = svn_wc_prop_get2(&value, sb->wc_ctx, path, "svn:needs-lock", pool, pool);
        if (e == nullptr)
            sb->SubStat->LockData.NeedsLocks = (value != 0);
        else
        {
            sb->SubStat->LockData.NeedsLocks = false;
            svn_error_clear(e);
        }
    }

    return SVN_NO_ERROR;
}

svn_error_t * getallstatus(void * baton, const char * path, const svn_client_status_t * status, apr_pool_t * pool)
{
    SubWCRev_StatusBaton_t * sb = (SubWCRev_StatusBaton_t *) baton;
    if((NULL == status) || (NULL == sb) || (NULL == sb->SubStat))
    {
        return SVN_NO_ERROR;
    }

    if (status->kind == svn_node_dir)
    {
        const svn_string_t * value = NULL;
        svn_wc_prop_get2(&value, sb->wc_ctx, path, "svn:externals", pool, pool);
        if (value)
        {
            apr_array_header_t* parsedExternals = NULL;
            svn_error_t * err = svn_wc_parse_externals_description3( &parsedExternals, path, value->data, TRUE, pool);

            if (err == nullptr)
            {
                for (long i=0; i < parsedExternals->nelts; ++i)
                {
                    svn_wc_external_item2_t * e = APR_ARRAY_IDX(parsedExternals, i, svn_wc_external_item2_t*);

                    if (e != NULL)
                    {
                        if (e->revision.kind != svn_opt_revision_number)
                        {
                            sb->SubStat->bIsExternalsNotFixed = TRUE;
                        }

                        if (((sb->SubStat->bExternals) || (sb->SubStat->bExternalsNoMixedRevision)) && (NULL != sb->extarray))
                        {
                            SubWcExtData_t extdata;
                            extdata.Path = apr_pstrcat(sb->pool, path, "/", e->target_dir, NULL);
                            extdata.Revision = e->revision;
                            sb->extarray->push_back(extdata);
                        }
                    }
                }
            }
        }
    }

    if (status->repos_root_url)
    {
        if (sb->SubStat->RootUrl[0] == 0)
        {
            strncpy_s(sb->SubStat->RootUrl, URL_BUF, status->repos_root_url, URL_BUF);
        }
        if (strncmp(sb->SubStat->RootUrl, status->repos_root_url, URL_BUF) != 0)
            return SVN_NO_ERROR;
    }
    if (status->changed_author)
    {
        if ((sb->SubStat->Author[0] == 0)&&(status->repos_relpath)&&(status->repos_root_url))
        {
            char EntryUrl[URL_BUF];
            UnescapeCopy(status->repos_root_url, status->repos_relpath,EntryUrl, URL_BUF);
            if (strncmp(sb->SubStat->Url, EntryUrl, URL_BUF) == 0)
            {
                strncpy_s(sb->SubStat->Author, URL_BUF, status->changed_author, URL_BUF);
            }
        }
    }
    if ((status->kind == svn_node_file)||(sb->SubStat->bFolders))
    {
        if (sb->SubStat->CmtRev < status->changed_rev)
        {
            sb->SubStat->CmtRev = status->changed_rev;
            sb->SubStat->CmtDate = status->changed_date;
        }
    }
    if (sb->SubStat->MaxRev < status->revision)
    {
        sb->SubStat->MaxRev = status->revision;
    }
    if ((status->revision > 0)&&(sb->SubStat->MinRev > status->revision || sb->SubStat->MinRev == 0))
    {
        sb->SubStat->MinRev = status->revision;
    }

    sb->SubStat->bIsSvnItem = false;
    switch (status->node_status)
    {
    case svn_wc_status_none:
    case svn_wc_status_unversioned:
        sb->SubStat->HasUnversioned = TRUE;
        break;
    case svn_wc_status_ignored:
        break;
    case svn_wc_status_external:
    case svn_wc_status_incomplete:
    case svn_wc_status_normal:
        sb->SubStat->bIsSvnItem = true;
        break;
    default:
        sb->SubStat->bIsSvnItem = true;
        sb->SubStat->HasMods = TRUE;
        break;
    }

    switch (status->prop_status)
    {
    case svn_wc_status_none:
    case svn_wc_status_unversioned:
    case svn_wc_status_ignored:
        break;
    case svn_wc_status_external:
    case svn_wc_status_incomplete:
    case svn_wc_status_normal:
        sb->SubStat->bIsSvnItem = true;
        break;
    default:
        sb->SubStat->bIsSvnItem = true;
        sb->SubStat->HasMods = TRUE;
        break;
    }

    // Assign the values for the lock information
    sb->SubStat->LockData.IsLocked = false;
    strcpy_s(sb->SubStat->LockData.Owner, OWNER_BUF, "");
    strcpy_s(sb->SubStat->LockData.Comment, COMMENT_BUF, "");
    sb->SubStat->LockData.CreationDate = 0;

    if ((status->lock)&&(status->lock->token))
    {
        if((status->lock->token[0] != 0))
        {
            sb->SubStat->LockData.IsLocked = true;
            if(NULL != status->lock->owner)
                strncpy_s(sb->SubStat->LockData.Owner, OWNER_BUF, status->lock->owner, OWNER_BUF);
            if(NULL != status->lock->comment)
                strncpy_s(sb->SubStat->LockData.Comment, COMMENT_BUF, status->lock->comment, COMMENT_BUF);
            sb->SubStat->LockData.CreationDate = status->lock->creation_date;
        }
    }
    return SVN_NO_ERROR;
}

svn_error_t *
svn_status (    const char *path,
                void *status_baton,
                svn_boolean_t no_ignore,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool)
{
    SubWCRev_StatusBaton_t sb;
    std::vector<SubWcExtData_t> * extarray = new std::vector<SubWcExtData_t>;
    sb.SubStat = (SubWCRev_t *)status_baton;
    sb.extarray = extarray;
    sb.pool = pool;
    sb.wc_ctx = ctx->wc_ctx;

    svn_opt_revision_t wcrev;
    wcrev.kind = svn_opt_revision_working;

    SVN_ERR(svn_client_status5(NULL, ctx, path, &wcrev, svn_depth_empty, true, false, true, true, true, NULL, getfirststatus, &sb, pool));
    SVN_ERR(svn_client_status5(NULL, ctx, path, &wcrev, svn_depth_infinity, true, false, true, true, true, NULL, getallstatus, &sb, pool));


    // now crawl through all externals
    for (std::vector<SubWcExtData_t>::iterator I = extarray->begin(); I != extarray->end(); ++I)
    {
        SubWcExtData_t extdata = *I;
        svn_revnum_t minRev = -1;
        svn_revnum_t maxRev = -1;
        if (strcmp(path, extdata.Path))
        {
            if (sb.SubStat->bExternalsNoMixedRevision && (extdata.Revision.kind == svn_opt_revision_number))
            {
                minRev = sb.SubStat->MinRev;
                maxRev = sb.SubStat->MaxRev;
                sb.SubStat->MinRev = 0;
                sb.SubStat->MaxRev = 0;
            }

            svn_status (extdata.Path, sb.SubStat, no_ignore, ctx, pool);

            if (sb.SubStat->bExternalsNoMixedRevision && (extdata.Revision.kind == svn_opt_revision_number))
            {
                // Check if the used revsions are only same as the external explicit revision
                if ((extdata.Revision.value.number == sb.SubStat->MaxRev) && (extdata.Revision.value.number == sb.SubStat->MinRev))
                {
                    sb.SubStat->MaxRev = maxRev;
                    sb.SubStat->MinRev = minRev;
                }
                else
                {
                    if (sb.SubStat->MaxRev < maxRev)
                    {
                        sb.SubStat->MaxRev = maxRev;
                    }
                    if ((minRev > 0)&&(sb.SubStat->MinRev > minRev || sb.SubStat->MinRev == 0))
                    {
                        sb.SubStat->MinRev = minRev;
                    }
                    // Set an extra variable, because when an fixed external has been manually updated to head, no error occour.
                    sb.SubStat->bIsExternalMixed = TRUE;
                }
            }
        }
    }

    delete extarray;

    return SVN_NO_ERROR;
}
#pragma warning(pop)
