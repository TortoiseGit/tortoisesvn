// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2015, 2019, 2021-2022 - TortoiseSVN

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
#include "SVNExternals.h"
#include "SVNHelpers.h"
#include "SVNError.h"
#include "SVN.h"
#include "SVNInfo.h"
#include "SVNRev.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"

#pragma warning(push)
#include <apr_uri.h>
#include "svn_wc.h"
#include "svn_client.h"
#include "svn_types.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_props.h"
#include "client.h"
#pragma warning(pop)

// copied from subversion\libsvn_client\externals.c (private functions there, that's why we copy it here)
static svn_error_t *
    uriScheme(const char **scheme, const char *uri, apr_pool_t *pool)
{
    apr_size_t i;

    for (i = 0; uri[i] && uri[i] != ':'; ++i)
        if (uri[i] == '/')
            goto error;

    if (i > 0 && uri[i] == ':' && uri[i + 1] == '/' && uri[i + 2] == '/')
    {
        *scheme = apr_pstrmemdup(pool, uri, i);
        return nullptr;
    }

error:
    return svn_error_createf(SVN_ERR_BAD_URL, nullptr,
                             "URL '%s' does not begin with a scheme",
                             uri);
}

static svn_error_t *
    resolveRelativeExternalURL(const char **                  resolvedURL,
                               const svn_wc_external_item2_t *item,
                               const char *                   reposRootURL,
                               const char *                   parentDirURL,
                               apr_pool_t *                   resultPool,
                               apr_pool_t *                   scratchPool)
{
    const char * url = item->url;
    apr_uri_t    parentDirUri{};
    apr_status_t status{};

    *resolvedURL = item->url;

    /* If the URL is already absolute, there is nothing to do. */
    if (svn_path_is_url(url))
    {
        /* "http://server/path" */
        const char *canonicalizedURL = nullptr;
        SVN_ERR(svn_uri_canonicalize_safe(&canonicalizedURL, nullptr, url, resultPool, scratchPool));
        *resolvedURL = canonicalizedURL;

        return nullptr;
    }

    if (url[0] == '/')
    {
        /* "/path", "//path", and "///path" */
        int numLeadingSlashes = 1;
        if (url[1] == '/')
        {
            numLeadingSlashes++;
            if (url[2] == '/')
                numLeadingSlashes++;
        }

        /* "//schema-relative" and in some cases "///schema-relative".
         This last format is supported on file:// schema relative. */
        url = apr_pstrcat(scratchPool,
                          apr_pstrndup(scratchPool, url, numLeadingSlashes),
                          svn_relpath_canonicalize(url + numLeadingSlashes,
                                                   scratchPool),
                          static_cast<char *>(nullptr));
    }
    else
    {
        /* "^/path" and "../path" */
        url = svn_relpath_canonicalize(url, scratchPool);
    }

    /* Parse the parent directory URL into its parts. */
    status = apr_uri_parse(scratchPool, parentDirURL, &parentDirUri);
    if (status)
        return svn_error_createf(SVN_ERR_BAD_URL, nullptr,
                                 "Illegal parent directory URL '%s'",
                                 parentDirURL);

    /* If the parent directory URL is at the server root, then the URL
     may have no / after the hostname so apr_uri_parse() will leave
     the URL's path as NULL. */
    if (!parentDirUri.path)
        parentDirUri.path = apr_pstrmemdup(scratchPool, "/", 1);
    parentDirUri.query    = nullptr;
    parentDirUri.fragment = nullptr;

    /* Handle URLs relative to the current directory or to the
     repository root.  The backpaths may only remove path elements,
     not the hostname.  This allows an external to refer to another
     repository in the same server relative to the location of this
     repository, say using SVNParentPath. */
    if ((0 == strncmp("../", url, 3)) ||
        (0 == strncmp("^/", url, 2)))
    {
        apr_array_header_t *baseComponents     = nullptr;
        apr_array_header_t *relativeComponents = nullptr;
        int                 i                  = 0;

        /* Decompose either the parent directory's URL path or the
         repository root's URL path into components.  */
        if (0 == strncmp("../", url, 3))
        {
            baseComponents     = svn_path_decompose(parentDirUri.path,
                                                scratchPool);
            relativeComponents = svn_path_decompose(url, scratchPool);
        }
        else
        {
            apr_uri_t reposRootUri;

            status = apr_uri_parse(scratchPool, reposRootURL,
                                   &reposRootUri);
            if (status)
                return svn_error_createf(SVN_ERR_BAD_URL, nullptr,
                                         "Illegal repository root URL '%s'",
                                         reposRootURL);

            /* If the repository root URL is at the server root, then
             the URL may have no / after the hostname so
             apr_uri_parse() will leave the URL's path as NULL. */
            if (!reposRootUri.path)
                reposRootUri.path = apr_pstrmemdup(scratchPool, "/", 1);

            baseComponents     = svn_path_decompose(reposRootUri.path,
                                                scratchPool);
            relativeComponents = svn_path_decompose(url + 2, scratchPool);
        }

        for (i = 0; i < relativeComponents->nelts; ++i)
        {
            const char *component = APR_ARRAY_IDX(relativeComponents,
                                                  i,
                                                  const char *);
            if (0 == strcmp("..", component))
            {
                /* Constructing the final absolute URL together with
                 apr_uri_unparse() requires that the path be absolute,
                 so only pop a component if the component being popped
                 is not the component for the root directory. */
                if (baseComponents->nelts > 1)
                    apr_array_pop(baseComponents);
            }
            else
                APR_ARRAY_PUSH(baseComponents, const char *) = component;
        }

        parentDirUri.path            = const_cast<char *>(svn_path_compose(baseComponents,
                                                                scratchPool));
        const char *canonicalizedURL = nullptr;
        SVN_ERR(svn_uri_canonicalize_safe(&canonicalizedURL, nullptr,
                                          apr_uri_unparse(scratchPool, &parentDirUri, 0),
                                          resultPool, scratchPool));
        *resolvedURL = canonicalizedURL;

        return nullptr;
    }

    /* The remaining URLs are relative to the either the scheme or
     server root and can only refer to locations inside that scope, so
     backpaths are not allowed. */
    if (svn_path_is_backpath_present(url + 2))
        return svn_error_createf(SVN_ERR_BAD_URL, nullptr,
                                 "The external relative URL '%s' cannot have backpaths, i.e. '..'",
                                 item->url);

    /* Relative to the scheme: Build a new URL from the parts we know.  */
    if (0 == strncmp("//", url, 2))
    {
        const char *scheme;

        SVN_ERR(uriScheme(&scheme, reposRootURL, scratchPool));
        const char *canonicalizedURL = nullptr;
        SVN_ERR(svn_uri_canonicalize_safe(&canonicalizedURL, nullptr,
                                          apr_pstrcat(scratchPool, scheme, ":", url, nullptr),
                                          resultPool, scratchPool));
        *resolvedURL = canonicalizedURL;
        return nullptr;
    }

    /* Relative to the server root: Just replace the path portion of the
     parent's URL.  */
    if (url[0] == '/')
    {
        parentDirUri.path = const_cast<char *>(url);

        const char *canonicalizedURL = nullptr;
        SVN_ERR(svn_uri_canonicalize_safe(&canonicalizedURL, nullptr,
                                          apr_uri_unparse(scratchPool, &parentDirUri, 0),
                                          resultPool, scratchPool));
        *resolvedURL = canonicalizedURL;
        return nullptr;
    }

    return svn_error_createf(SVN_ERR_BAD_URL, nullptr,
                             "Unrecognized format for the relative external URL '%s'",
                             item->url);
}

class Sb
{
public:
    Sb()
        : adjust(false)
    {
    }
    ~Sb() {}

    std::string extValue;
    CString     pathUrl;
    bool        adjust;
};

SVNExternals::SVNExternals()
{
}

SVNExternals::~SVNExternals()
{
}

bool SVNExternals::Add(const CTSVNPath &path, const std::string &extValue, bool fetchRev, svn_revnum_t headRev)
{
    SVN      svn;
    CString  pathUrl = svn.GetURLFromPath(path);
    CStringA dirUrl  = CUnicodeUtils::GetUTF8(pathUrl);
    CStringA root    = CUnicodeUtils::GetUTF8(svn.GetRepositoryRoot(path));

    SVNExternal ext;

    SVNPool             pool;
    apr_array_header_t *parsedExternals = nullptr;
    SVNError            svnError(svn_wc_parse_externals_description3(&parsedExternals, path.GetSVNApiPath(pool), extValue.c_str(), TRUE, pool));

    if (svnError.GetCode() == 0)
    {
        for (long i = 0; i < parsedExternals->nelts; ++i)
        {
            svn_wc_external_item2_t *e = APR_ARRAY_IDX(parsedExternals, i, svn_wc_external_item2_t *);

            if (e != nullptr)
            {
                ext.path        = path;
                ext.pathUrl     = pathUrl;
                ext.pegRevision = e->peg_revision;
                if ((e->peg_revision.kind != svn_opt_revision_unspecified) &&
                    (e->revision.kind == svn_opt_revision_unspecified))
                {
                    ext.revision     = e->peg_revision;
                    ext.origRevision = e->peg_revision;
                }
                else
                {
                    ext.revision     = e->revision;
                    ext.origRevision = e->revision;
                }
                if (headRev >= 0)
                {
                    ext.revision.kind         = svn_opt_revision_number;
                    ext.revision.value.number = headRev;
                }
                ext.url       = CUnicodeUtils::GetUnicode(e->url);
                ext.targetDir = CUnicodeUtils::GetUnicode(e->target_dir);
                if (fetchRev)
                {
                    CTSVNPath p = path;
                    p.AppendPathString(ext.targetDir);
                    if (p.IsDirectory())
                    {
                        bool         bSwitched, bModified, bSparse;
                        svn_revnum_t maxRev, minRev;
                        if (svn.GetWCRevisionStatus(p, false, minRev, maxRev, bSwitched, bModified, bSparse))
                        {
                            ext.revision.kind         = svn_opt_revision_number;
                            ext.revision.value.number = maxRev;
                        }
                    }
                    else
                    {
                        // GetWCRevisionStatus() does not work for file externals, that's
                        // why we use SVNInfo here to get the revision.
                        SVNInfo            svnInfo;
                        const SVNInfoData *info = svnInfo.GetFirstFileInfo(p, SVNRev::REV_WC, SVNRev::REV_WC);
                        if (info)
                        {
                            ext.revision.kind         = svn_opt_revision_number;
                            ext.revision.value.number = info->lastChangedRev;
                        }
                    }
                }

                const char * pFullUrl = nullptr;
                svn_error_t *error    = resolveRelativeExternalURL(&pFullUrl, e, root, dirUrl, pool, pool);
                if ((error == nullptr) && (pFullUrl))
                    ext.fullUrl = CUnicodeUtils::GetUnicode(pFullUrl);
                else
                    svn_error_clear(error);

                push_back(ext);
            }
        }
        return true;
    }

    return false;
}

bool SVNExternals::TagExternals(bool bRemote, const CString &message, svn_revnum_t headRev, const CTSVNPath &origUrl, const CTSVNPath &tagUrl)
{
    // create a map of paths and external properties
    // so that we have all externals which belong to the same
    // property

    std::map<CTSVNPath, Sb> externals;
    for (std::vector<SVNExternal>::iterator it = begin(); it != end(); ++it)
    {
        SVNRev  rev     = it->revision;
        SVNRev  origRev = it->origRevision;
        SVNRev  pegRev  = it->pegRevision;
        CString peg;
        if (pegRev.IsValid() && !pegRev.IsHead())
            peg = L"@" + pegRev.ToString();
        else if (it->adjust)
            peg = L"@" + rev.ToString();
        else
            peg.Empty();

        CString targetDir = it->targetDir;
        if (targetDir.FindOneOf(L" \t") > 0)
            targetDir = L"\'" + targetDir + L"\'";

        CString temp;
        if (it->adjust && !rev.IsHead())
            temp.Format(L"-r %s %s%s %s", static_cast<LPCWSTR>(rev.ToString()), static_cast<LPCWSTR>(it->url), static_cast<LPCWSTR>(peg), static_cast<LPCWSTR>(targetDir));
        else if (origRev.IsValid() && !origRev.IsHead())
            temp.Format(L"-r %s %s%s %s", static_cast<LPCWSTR>(origRev.ToString()), static_cast<LPCWSTR>(it->url), static_cast<LPCWSTR>(peg), static_cast<LPCWSTR>(targetDir));
        else
            temp.Format(L"%s%s %s", static_cast<LPCWSTR>(it->url), static_cast<LPCWSTR>(peg), static_cast<LPCWSTR>(targetDir));

        Sb val = externals[it->path];
        if (!val.extValue.empty())
            val.extValue += "\n";
        val.extValue += CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(temp));
        val.adjust          = val.adjust || it->adjust;
        val.pathUrl         = it->pathUrl;
        externals[it->path] = val;
    }

    m_sError.Empty();
    SVN svn;
    // now set the new properties
    std::set<CTSVNPath> changedUrls;
    for (std::map<CTSVNPath, Sb>::iterator it = externals.begin(); it != externals.end(); ++it)
    {
        if (it->second.adjust)
        {
            if (bRemote)
            {
                // construct the url where to set the tagged svn:externals property
                // example paths:
                // origurl : http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk
                // tagurl  : http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7
                // pathurl : http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/ext

                CString sInsidePath = it->second.pathUrl.Mid(origUrl.GetSVNPathString().GetLength()); // 'ext'
                if (!origUrl.IsUrl())
                {
                    sInsidePath = it->second.pathUrl.Mid(svn.GetURLFromPath(origUrl).GetLength());
                }

                CTSVNPath targetUrl = tagUrl;           // http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7
                targetUrl.AppendRawString(sInsidePath); // http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7/ext

                if (changedUrls.find(targetUrl) == changedUrls.end())
                {
                    SVNProperties props(targetUrl, headRev, false, false);
                    if (!props.Add(SVN_PROP_EXTERNALS, it->second.extValue, false, svn_depth_empty, message))
                        m_sError = props.GetLastErrorMessage();
                    else
                    {
                        headRev = props.GetRevision();
                    }
                    changedUrls.insert(targetUrl);
                }
            }
            else
            {
                SVNProperties props(it->first, SVNRev::REV_WC, false, false);
                if (!props.Add(SVN_PROP_EXTERNALS, it->second.extValue))
                    m_sError = props.GetLastErrorMessage();
            }
        }
    }

    return m_sError.IsEmpty();
}

std::string SVNExternals::GetValue(const CTSVNPath &path) const
{
    std::string ret;
    for (auto it = cbegin(); it != cend(); ++it)
    {
        if (path.IsEquivalentToWithoutCase(it->path))
        {
            SVNRev  rev    = it->revision;
            SVNRev  pegrev = it->pegRevision;
            CString peg;
            if (pegrev.IsValid() && !pegrev.IsHead())
                peg = L"@" + pegrev.ToString();
            else
                peg.Empty();

            CString targetDir = it->targetDir;
            if (targetDir.Find(' ') >= 0)
                targetDir = L"'" + targetDir + L"'";
            CString temp;
            if (rev.IsValid() && !rev.IsHead() && (!rev.IsEqual(pegrev)))
                temp.Format(L"-r %s %s%s %s", static_cast<LPCWSTR>(rev.ToString()), static_cast<LPCWSTR>(it->url), static_cast<LPCWSTR>(peg), static_cast<LPCWSTR>(targetDir));
            else
                temp.Format(L"%s%s %s", static_cast<LPCWSTR>(it->url), static_cast<LPCWSTR>(peg), static_cast<LPCWSTR>(targetDir));
            if (ret.size())
                ret += "\n";
            ret += CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(temp));
        }
    }

    return ret;
}

CString SVNExternals::GetFullExternalUrl(const CString &extUrl, const CString &root, const CString &dirUrl)
{
    CStringA extUrlA = CUnicodeUtils::GetUTF8(extUrl);
    CStringA rootA   = CUnicodeUtils::GetUTF8(root);
    CStringA dirUrlA = CUnicodeUtils::GetUTF8(dirUrl);
    CString  url     = extUrl;

    SVNPool                 pool;
    const char *            pFullUrl = nullptr;
    svn_wc_external_item2_t e;
    e.url              = extUrlA;
    svn_error_t *error = resolveRelativeExternalURL(&pFullUrl, &e, rootA, dirUrlA, pool, pool);
    if ((error == nullptr) && (pFullUrl))
        url = CUnicodeUtils::GetUnicode(pFullUrl);
    else
        svn_error_clear(error);

    return url;
}

apr_hash_t *SVNExternals::GetHash(bool bLocal, apr_pool_t *pool) const
{
    apr_hash_t *externalsToPin = nullptr;
    if (!empty())
    {
        if (NeedsTagging())
        {
            externalsToPin = apr_hash_make(pool);

            for (const auto &ext : *this)
            {
                const char *key = nullptr;
                if (bLocal)
                    key = apr_pstrdup(pool, ext.path.GetSVNApiPath(pool));
                else
                    key = apr_pstrdup(pool, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(ext.pathUrl)));
                apr_array_header_t *extitemsarray = static_cast<apr_array_header_t *>(apr_hash_get(externalsToPin, key, APR_HASH_KEY_STRING));
                if (extitemsarray == nullptr)
                {
                    extitemsarray = apr_array_make(pool, 0, sizeof(svn_wc_external_item2_t *));
                }
                if (ext.adjust)
                {
                    svn_wc_external_item2_t *item = nullptr;
                    svn_wc_external_item2_create(&item, pool);
                    item->peg_revision                                       = ext.pegRevision;
                    item->revision                                           = ext.origRevision;
                    item->target_dir                                         = apr_pstrdup(pool, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(ext.targetDir)));
                    item->url                                                = apr_pstrdup(pool, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(ext.url)));
                    APR_ARRAY_PUSH(extitemsarray, svn_wc_external_item2_t *) = item;
                }
                apr_hash_set(externalsToPin, key, APR_HASH_KEY_STRING, static_cast<const void *>(extitemsarray));
            }
        }
    }

    return externalsToPin;
}

bool SVNExternals::NeedsTagging() const
{
    bool bHasExtsToPin = false;
    for (const auto &ext : *this)
    {
        if (ext.adjust)
        {
            bHasExtsToPin = true;
            break;
        }
    }
    return bHasExtsToPin;
}
