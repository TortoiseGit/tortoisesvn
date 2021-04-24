// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2017, 2019, 2021 - TortoiseSVN

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
#include "apr_pools.h"
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
#include <tuple>
#include <fstream>

CRegStdString regTagsPattern = CRegStdString(L"Software\\TortoiseSVN\\RevisionGraph\\TagsPattern", L"tags");

void loadIgnorePatterns(const char* wc, SubWCRevT* subStat)
{
    std::string path       = wc;
    std::string ignorePath = path + "/.subwcrevignore";

    std::ifstream inFile;
    inFile.open(ignorePath);
    if (inFile.good())
    {
        std::string line;
        while (std::getline(inFile, line))
        {
            if (!line.empty())
                subStat->ignorePatterns.emplace(line, path.size());
        }
    }
}

// Copy the URL from src to dest, unescaping on the fly.
void UnescapeCopy(const char* root, const char* src, char* dest, int bufLen)
{
    const char* pszSource = root;
    char*       pszDest   = dest;
    int         len       = 0;

    // under VS.NET2k5 strchr() wants this to be a non-const array :/

    static char szHex[] = "0123456789ABCDEF";

    // Unescape special characters. The number of characters
    // in the *pszDest is assumed to be <= the number of characters
    // in pszSource (they are both the same string anyway)

    bool bRoot = true;
    while ((*pszSource != '\0') && (++len < bufLen))
    {
        if (*pszSource == '%')
        {
            // The next two chars following '%' should be digits
            if (*(pszSource + 1) == '\0' ||
                *(pszSource + 2) == '\0')
            {
                // nothing left to do
                break;
            }

            char  nValue  = '?';
            char* pszHigh = nullptr;
            pszSource++;

            char up = static_cast<char>(toupper(*pszSource));
            pszHigh = strchr(szHex, up);

            if (pszHigh != nullptr)
            {
                pszSource++;
                up           = static_cast<char>(toupper(*pszSource));
                char* pszLow = strchr(szHex, up);

                if (pszLow != nullptr)
                {
                    nValue = static_cast<char>(((pszHigh - szHex) << 4) +
                                               (pszLow - szHex));
                }
            }
            *pszDest++ = nValue;
        }
        else
            *pszDest++ = *pszSource;

        pszSource++;
        if ((bRoot) && (*pszSource == 0))
        {
            if (*pszDest != '/')
                *pszDest++ = '/';
            pszSource = src;
            bRoot     = false;
        }
    }

    *pszDest = '\0';
}

std::wstring Tokenize(const _TCHAR* str, const _TCHAR* delim, std::wstring::size_type& iStart)
{
    const _TCHAR*           pStr = str + iStart;
    const _TCHAR*           r    = wcsstr(pStr, delim);
    std::wstring::size_type dLen = wcslen(delim);

    while (r)
    {
        if (r > pStr)
        {
            iStart = static_cast<std::wstring::size_type>(r - str) + dLen;
            return std::wstring(pStr, static_cast<std::wstring::size_type>(r - pStr));
        }
        pStr = r + dLen;
        r    = wcsstr(pStr, delim);
    }

    if (pStr[0] != 0)
    {
        iStart = static_cast<std::wstring::size_type>(wcslen(str));
        return std::wstring(pStr);
    }
    return std::wstring();
}

bool IsTaggedVersion(const char* url)
{
    bool isTag = false;

    if (!url)
    {
        return false;
    }

    std::wstring urllower = Utf8ToWide(url);
    std::transform(urllower.begin(), urllower.end(), urllower.begin(), ::towlower);

    // look for the tag pattern inside in the url
    std::wstring            sTags = regTagsPattern;
    std::wstring::size_type pos   = 0;
    std::wstring            temp;
    while (!isTag)
    {
        temp = Tokenize(sTags.c_str(), L";", pos);
        if (!temp.length())
            break;

        std::wstring::size_type urlpos = 0;
        std::wstring            temp2;
        for (;;)
        {
            temp2 = Tokenize(urllower.c_str(), L"/", urlpos);
            if (!temp2.length())
                break;

            if (wcswildcmp(temp.c_str(), temp2.c_str()))
            {
                isTag = true;
                break;
            }
        }
    }
    return isTag;
}

svn_error_t* getfirststatus(void* baton, const char* path, const svn_client_status_t* status, apr_pool_t* pool)
{
    SubWCRevStatusBatonT* sb = static_cast<SubWCRevStatusBatonT*>(baton);
    if ((nullptr == status) || (nullptr == sb) || (nullptr == sb->subStat))
    {
        return nullptr;
    }
    if ((status->repos_relpath) && (sb->subStat->url[0] == 0))
    {
        UnescapeCopy(status->repos_root_url, status->repos_relpath, sb->subStat->url, URL_BUF);
        sb->subStat->bIsTagged = IsTaggedVersion(sb->subStat->url);
    }
    if (status->kind == svn_node_file)
    {
        const svn_string_t* value = nullptr;
        svn_error_t*        e     = svn_wc_prop_get2(&value, sb->wcCtx, path, "svn:needs-lock", pool, pool);
        if (e == nullptr)
            sb->subStat->lockData.needsLocks = (value != nullptr);
        else
        {
            sb->subStat->lockData.needsLocks = false;
            svn_error_clear(e);
        }
    }

    return nullptr;
}

svn_error_t* getallstatus(void* baton, const char* path, const svn_client_status_t* status, apr_pool_t* pool)
{
    SubWCRevStatusBatonT* sb = static_cast<SubWCRevStatusBatonT*>(baton);
    if ((nullptr == status) || (nullptr == sb) || (nullptr == sb->subStat))
    {
        return nullptr;
    }

    if (status->repos_relpath && !sb->subStat->ignorePatterns.empty())
    {
        for (const auto& pattern : sb->subStat->ignorePatterns)
        {
            if (strwildcmp(std::get<0>(pattern).c_str(), status->repos_relpath))
                return nullptr;
        }
    }
    if (status->local_abspath && !sb->subStat->ignorePatterns.empty())
    {
        auto laLen = strlen(status->local_abspath);
        for (const auto& pattern : sb->subStat->ignorePatterns)
        {
            auto offset = std::get<1>(pattern);
            if (laLen <= offset)
                continue;
            const char* relativepath = &status->local_abspath[offset];
            if (*relativepath == '/')
                ++relativepath;
            if (strwildcmp(std::get<0>(pattern).c_str(), relativepath))
                return nullptr;
        }
    }

    if (status->kind == svn_node_dir)
    {
        const svn_string_t* value = nullptr;
        svn_wc_prop_get2(&value, sb->wcCtx, path, "svn:externals", pool, pool);
        if (value)
        {
            apr_array_header_t* parsedExternals = nullptr;
            svn_error_t*        err             = svn_wc_parse_externals_description3(&parsedExternals, path, value->data, TRUE, pool);

            if (err == nullptr)
            {
                for (long i = 0; i < parsedExternals->nelts; ++i)
                {
                    svn_wc_external_item2_t* e = APR_ARRAY_IDX(parsedExternals, i, svn_wc_external_item2_t*);

                    if (e != nullptr)
                    {
                        if (e->revision.kind != svn_opt_revision_number)
                        {
                            sb->subStat->bIsExternalsNotFixed = TRUE;
                        }

                        if (((sb->subStat->bExternals) || (sb->subStat->bExternalsNoMixedRevision)) && (nullptr != sb->extArray))
                        {
                            SubWcExtDataT extData;
                            extData.path     = apr_pstrcat(sb->pool, path, "/", e->target_dir, NULL);
                            extData.revision = e->revision;
                            sb->extArray->push_back(extData);
                        }
                    }
                }
            }
        }
    }

    if (status->repos_root_url)
    {
        if (sb->subStat->rootUrl[0] == 0)
        {
            strncpy_s(sb->subStat->rootUrl, URL_BUF, status->repos_root_url, URL_BUF);
        }
        if (strncmp(sb->subStat->rootUrl, status->repos_root_url, URL_BUF) != 0)
            return nullptr;
    }
    if (status->changed_author)
    {
        if ((sb->subStat->author[0] == 0) && (status->repos_relpath) && (status->repos_root_url))
        {
            char entryUrl[URL_BUF] = {0};
            UnescapeCopy(status->repos_root_url, status->repos_relpath, entryUrl, URL_BUF);
            if (strncmp(sb->subStat->url, entryUrl, URL_BUF) == 0)
            {
                strncpy_s(sb->subStat->author, URL_BUF, status->changed_author, URL_BUF);
            }
        }
    }
    if ((status->kind == svn_node_file) || (sb->subStat->bFolders))
    {
        if (sb->subStat->cmtRev < status->changed_rev)
        {
            sb->subStat->cmtRev  = status->changed_rev;
            sb->subStat->cmtDate = status->changed_date;
        }
    }
    if (sb->subStat->maxRev < status->revision)
    {
        sb->subStat->maxRev = status->revision;
    }
    if ((status->revision > 0) && (sb->subStat->minRev > status->revision || sb->subStat->minRev == 0))
    {
        sb->subStat->minRev = status->revision;
    }

    sb->subStat->bIsSvnItem = false;
    switch (status->node_status)
    {
        case svn_wc_status_none:
        case svn_wc_status_unversioned:
            sb->subStat->hasUnversioned = TRUE;
            break;
        case svn_wc_status_ignored:
            break;
        case svn_wc_status_external:
        case svn_wc_status_incomplete:
        case svn_wc_status_normal:
            sb->subStat->bIsSvnItem = true;
            break;
        default:
            sb->subStat->bIsSvnItem = true;
            sb->subStat->hasMods    = TRUE;
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
            sb->subStat->bIsSvnItem = true;
            break;
        default:
            sb->subStat->bIsSvnItem = true;
            sb->subStat->hasMods    = TRUE;
            break;
    }

    // Assign the values for the lock information
    sb->subStat->lockData.isLocked = false;
    strcpy_s(sb->subStat->lockData.owner, OWNER_BUF, "");
    strcpy_s(sb->subStat->lockData.comment, COMMENT_BUF, "");
    sb->subStat->lockData.creationDate = 0;

    if ((status->lock) && (status->lock->token))
    {
        if ((status->lock->token[0] != 0))
        {
            sb->subStat->lockData.isLocked = true;
            if (nullptr != status->lock->owner)
                strncpy_s(sb->subStat->lockData.owner, OWNER_BUF, status->lock->owner, OWNER_BUF);
            if (nullptr != status->lock->comment)
                strncpy_s(sb->subStat->lockData.comment, COMMENT_BUF, status->lock->comment, COMMENT_BUF);
            sb->subStat->lockData.creationDate = status->lock->creation_date;
        }
    }
    return nullptr;
}

svn_error_t*
    svnStatus(const char*       path,
              void*             statusBaton,
              svn_boolean_t     noIgnore,
              svn_client_ctx_t* ctx,
              apr_pool_t*       pool)
{
    SubWCRevStatusBatonT sb{};
    auto                 extArray = std::make_unique<std::vector<SubWcExtDataT>>();
    sb.subStat                    = static_cast<SubWCRevT*>(statusBaton);
    sb.extArray                   = extArray.get();
    sb.pool                       = pool;
    sb.wcCtx                      = ctx->wc_ctx;

    svn_opt_revision_t wcrev;
    wcrev.kind = svn_opt_revision_working;

    SVN_ERR(svn_client_status6(NULL, ctx, path, &wcrev, svn_depth_empty, true, false, true, true, true, true, NULL, getfirststatus, &sb, pool));
    SVN_ERR(svn_client_status6(NULL, ctx, path, &wcrev, svn_depth_infinity, true, false, true, true, true, true, NULL, getallstatus, &sb, pool));

    // now crawl through all externals
    for (std::vector<SubWcExtDataT>::iterator I = extArray->begin(); I != extArray->end(); ++I)
    {
        SubWcExtDataT extData = *I;
        if (strcmp(path, extData.path))
        {
            svn_revnum_t minRev = -1;
            svn_revnum_t maxRev = -1;
            if (sb.subStat->bExternalsNoMixedRevision && (extData.revision.kind == svn_opt_revision_number))
            {
                minRev             = sb.subStat->minRev;
                maxRev             = sb.subStat->maxRev;
                sb.subStat->minRev = 0;
                sb.subStat->maxRev = 0;
            }

            svnStatus(extData.path, sb.subStat, noIgnore, ctx, pool);

            if (sb.subStat->bExternalsNoMixedRevision && (extData.revision.kind == svn_opt_revision_number))
            {
                // Check if the used revsions are only same as the external explicit revision
                if ((extData.revision.value.number == sb.subStat->maxRev) && (extData.revision.value.number == sb.subStat->minRev))
                {
                    sb.subStat->maxRev = maxRev;
                    sb.subStat->minRev = minRev;
                }
                else
                {
                    if (sb.subStat->maxRev < maxRev)
                    {
                        sb.subStat->maxRev = maxRev;
                    }
                    if ((minRev > 0) && (sb.subStat->minRev > minRev || sb.subStat->minRev == 0))
                    {
                        sb.subStat->minRev = minRev;
                    }
                    // Set an extra variable, because when a fixed external has been manually updated to head, no error occurs.
                    sb.subStat->bIsExternalMixed = TRUE;
                }
            }
        }
    }

    return nullptr;
}
