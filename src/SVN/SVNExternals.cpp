// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2013 - TortoiseSVN

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
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_types.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_props.h"
#include "svn_config.h"
#include "client.h"
#pragma warning(pop)

// copied from subversion\libsvn_client\externals.c (private functions there, that's why we copy it here)
static svn_error_t *
    uri_scheme(const char **scheme, const char *uri, apr_pool_t *pool)
{
    apr_size_t i;

    for (i = 0; uri[i] && uri[i] != ':'; ++i)
        if (uri[i] == '/')
            goto error;

    if (i > 0 && uri[i] == ':' && uri[i+1] == '/' && uri[i+2] == '/')
    {
        *scheme = apr_pstrmemdup(pool, uri, i);
        return SVN_NO_ERROR;
    }

error:
    return svn_error_createf(SVN_ERR_BAD_URL, 0,
        "URL '%s' does not begin with a scheme",
        uri);
}

static svn_error_t *
resolve_relative_external_url(const char **resolved_url,
                              const svn_wc_external_item2_t *item,
                              const char *repos_root_url,
                              const char *parent_dir_url,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool)
{
  const char *url = item->url;
  apr_uri_t parent_dir_uri;
  apr_status_t status;

  *resolved_url = item->url;

  /* If the URL is already absolute, there is nothing to do. */
  if (svn_path_is_url(url))
    {
      /* "http://server/path" */
      *resolved_url = svn_uri_canonicalize(url, result_pool);
      return SVN_NO_ERROR;
    }

  if (url[0] == '/')
    {
      /* "/path", "//path", and "///path" */
      int num_leading_slashes = 1;
      if (url[1] == '/')
        {
          num_leading_slashes++;
          if (url[2] == '/')
            num_leading_slashes++;
        }

      /* "//schema-relative" and in some cases "///schema-relative".
         This last format is supported on file:// schema relative. */
      url = apr_pstrcat(scratch_pool,
                        apr_pstrndup(scratch_pool, url, num_leading_slashes),
                        svn_relpath_canonicalize(url + num_leading_slashes,
                                                 scratch_pool),
                        (char*)NULL);
    }
  else
    {
      /* "^/path" and "../path" */
      url = svn_relpath_canonicalize(url, scratch_pool);
    }

  /* Parse the parent directory URL into its parts. */
  status = apr_uri_parse(scratch_pool, parent_dir_url, &parent_dir_uri);
  if (status)
    return svn_error_createf(SVN_ERR_BAD_URL, 0,
                             "Illegal parent directory URL '%s'",
                             parent_dir_url);

  /* If the parent directory URL is at the server root, then the URL
     may have no / after the hostname so apr_uri_parse() will leave
     the URL's path as NULL. */
  if (! parent_dir_uri.path)
    parent_dir_uri.path = apr_pstrmemdup(scratch_pool, "/", 1);
  parent_dir_uri.query = NULL;
  parent_dir_uri.fragment = NULL;

  /* Handle URLs relative to the current directory or to the
     repository root.  The backpaths may only remove path elements,
     not the hostname.  This allows an external to refer to another
     repository in the same server relative to the location of this
     repository, say using SVNParentPath. */
  if ((0 == strncmp("../", url, 3)) ||
      (0 == strncmp("^/", url, 2)))
    {
      apr_array_header_t *base_components;
      apr_array_header_t *relative_components;
      int i;

      /* Decompose either the parent directory's URL path or the
         repository root's URL path into components.  */
      if (0 == strncmp("../", url, 3))
        {
          base_components = svn_path_decompose(parent_dir_uri.path,
                                               scratch_pool);
          relative_components = svn_path_decompose(url, scratch_pool);
        }
      else
        {
          apr_uri_t repos_root_uri;

          status = apr_uri_parse(scratch_pool, repos_root_url,
                                 &repos_root_uri);
          if (status)
            return svn_error_createf(SVN_ERR_BAD_URL, 0,
                                     "Illegal repository root URL '%s'",
                                     repos_root_url);

          /* If the repository root URL is at the server root, then
             the URL may have no / after the hostname so
             apr_uri_parse() will leave the URL's path as NULL. */
          if (! repos_root_uri.path)
            repos_root_uri.path = apr_pstrmemdup(scratch_pool, "/", 1);

          base_components = svn_path_decompose(repos_root_uri.path,
                                               scratch_pool);
          relative_components = svn_path_decompose(url + 2, scratch_pool);
        }

      for (i = 0; i < relative_components->nelts; ++i)
        {
          const char *component = APR_ARRAY_IDX(relative_components,
                                                i,
                                                const char *);
          if (0 == strcmp("..", component))
            {
              /* Constructing the final absolute URL together with
                 apr_uri_unparse() requires that the path be absolute,
                 so only pop a component if the component being popped
                 is not the component for the root directory. */
              if (base_components->nelts > 1)
                apr_array_pop(base_components);
            }
          else
            APR_ARRAY_PUSH(base_components, const char *) = component;
        }

      parent_dir_uri.path = (char *)svn_path_compose(base_components,
                                                     scratch_pool);
      *resolved_url = svn_uri_canonicalize(apr_uri_unparse(scratch_pool,
                                                           &parent_dir_uri, 0),
                                       result_pool);
      return SVN_NO_ERROR;
    }

  /* The remaining URLs are relative to the either the scheme or
     server root and can only refer to locations inside that scope, so
     backpaths are not allowed. */
  if (svn_path_is_backpath_present(url + 2))
    return svn_error_createf(SVN_ERR_BAD_URL, 0,
                             "The external relative URL '%s' cannot have backpaths, i.e. '..'",
                             item->url);

  /* Relative to the scheme: Build a new URL from the parts we know.  */
  if (0 == strncmp("//", url, 2))
    {
      const char *scheme;

      SVN_ERR(uri_scheme(&scheme, repos_root_url, scratch_pool));
      *resolved_url = svn_uri_canonicalize(apr_pstrcat(scratch_pool, scheme,
                                                       ":", url, (char *)NULL),
                                           result_pool);
      return SVN_NO_ERROR;
    }

  /* Relative to the server root: Just replace the path portion of the
     parent's URL.  */
  if (url[0] == '/')
    {
      parent_dir_uri.path = (char *)url;
      *resolved_url = svn_uri_canonicalize(apr_uri_unparse(scratch_pool,
                                                           &parent_dir_uri, 0),
                                           result_pool);
      return SVN_NO_ERROR;
    }

  return svn_error_createf(SVN_ERR_BAD_URL, 0,
                           "Unrecognized format for the relative external URL '%s'",
                           item->url);
}


class sb
{
public:
    sb() : adjust(false) {}
    ~sb() {}

    std::string extvalue;
    CString     pathurl;
    bool        adjust;
};


SVNExternals::SVNExternals()
{
}

SVNExternals::~SVNExternals()
{
}

bool SVNExternals::Add(const CTSVNPath& path, const std::string& extvalue, bool fetchrev, svn_revnum_t headrev)
{
    m_originals[path] = extvalue;
    SVN svn;
    CString pathurl = svn.GetURLFromPath(path);
    CStringA dirurl = CUnicodeUtils::GetUTF8(pathurl);
    CStringA root = CUnicodeUtils::GetUTF8(svn.GetRepositoryRoot(path));

    SVNExternal ext;

    SVNPool pool;
    apr_array_header_t* parsedExternals = NULL;
    SVNError svnError
        (svn_wc_parse_externals_description3
                            ( &parsedExternals
                            , path.GetSVNApiPath(pool)
                            , extvalue.c_str()
                            , TRUE
                            , pool));

    if (svnError.GetCode() == 0)
    {
        for (long i=0; i < parsedExternals->nelts; ++i)
        {
            svn_wc_external_item2_t * e = APR_ARRAY_IDX(parsedExternals, i, svn_wc_external_item2_t*);

            if (e != NULL)
            {
                ext.path = path;
                ext.pathurl = pathurl;
                ext.pegrevision = e->peg_revision;
                if ((e->peg_revision.kind != svn_opt_revision_unspecified) &&
                    (e->revision.kind == svn_opt_revision_unspecified))
                {
                    ext.revision = e->peg_revision;
                    ext.origrevision = e->peg_revision;
                }
                else
                {
                    ext.revision = e->revision;
                    ext.origrevision = e->revision;
                }
                if (headrev >= 0)
                {
                    ext.revision.kind = svn_opt_revision_number;
                    ext.revision.value.number = headrev;
                }
                ext.url = CUnicodeUtils::GetUnicode(e->url);
                ext.targetDir = CUnicodeUtils::GetUnicode(e->target_dir);
                if (fetchrev)
                {
                    svn_revnum_t maxrev, minrev;
                    CTSVNPath p = path;
                    p.AppendPathString(ext.targetDir);
                    if (p.IsDirectory())
                    {
                        bool bswitched, bmodified, bsparse;
                        if (svn.GetWCRevisionStatus(p, true, minrev, maxrev, bswitched, bmodified, bsparse))
                        {
                            ext.revision.kind = svn_opt_revision_number;
                            ext.revision.value.number = maxrev;
                        }
                    }
                    else
                    {
                        // GetWCRevisionStatus() does not work for file externals, that's
                        // why we use SVNInfo here to get the revision.
                        SVNInfo svninfo;
                        const SVNInfoData * info = svninfo.GetFirstFileInfo(p, SVNRev::REV_WC, SVNRev::REV_WC);
                        if (info)
                        {
                            ext.revision.kind = svn_opt_revision_number;
                            ext.revision.value.number = info->lastchangedrev;
                        }
                    }
                }

                const char * pFullUrl = NULL;
                svn_error_t * error = resolve_relative_external_url(&pFullUrl, e, root, dirurl, pool, pool);
                if ((error == NULL) && (pFullUrl))
                    ext.fullurl = CUnicodeUtils::GetUnicode(pFullUrl);
                else
                    svn_error_clear(error);

                push_back(ext);
            }
        }
        return true;
    }

    return false;
}

bool SVNExternals::TagExternals(bool bRemote, const CString& message, svn_revnum_t headrev, const CTSVNPath& origurl, const CTSVNPath& tagurl)
{
    // create a map of paths and external properties
    // so that we have all externals which belong to the same
    // property

    std::map<CTSVNPath, sb> externals;
    for (std::vector<SVNExternal>::iterator it = begin(); it != end(); ++it)
    {
        SVNRev rev = it->revision;
        SVNRev origrev = it->origrevision;
        SVNRev pegrev = it->pegrevision;
        CString peg;
        if (pegrev.IsValid() && !pegrev.IsHead())
            peg = _T("@") + pegrev.ToString();
        else if (it->adjust)
            peg = _T("@") + rev.ToString();
        else
            peg.Empty();

        CString temp;
        if (it->adjust && !rev.IsHead())
            temp.Format(_T("-r %s %s%s %s"), rev.ToString(), it->url, (LPCTSTR)peg, it->targetDir);
        else if (origrev.IsValid() && !origrev.IsHead())
            temp.Format(_T("-r %s %s%s %s"), origrev.ToString(), it->url, (LPCTSTR)peg, it->targetDir);
        else
            temp.Format(_T("%s%s %s"), it->url, (LPCTSTR)peg, it->targetDir);

        sb val = externals[it->path];
        if (!val.extvalue.empty())
            val.extvalue += "\n";
        val.extvalue += CUnicodeUtils::StdGetUTF8((LPCTSTR)temp);
        val.adjust = val.adjust || it->adjust;
        val.pathurl = it->pathurl;
        externals[it->path] = val;
    }

    m_sError.Empty();
    SVN svn;
    // now set the new properties
    std::set<CTSVNPath> changedurls;
    for (std::map<CTSVNPath, sb>::iterator it = externals.begin(); it != externals.end(); ++it)
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

                CString sInsidePath = it->second.pathurl.Mid(origurl.GetSVNPathString().GetLength()); // 'ext'
                if (!origurl.IsUrl())
                {
                    sInsidePath = it->second.pathurl.Mid(svn.GetURLFromPath(origurl).GetLength());
                }

                CTSVNPath targeturl = tagurl; // http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7
                targeturl.AppendRawString(sInsidePath); // http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7/ext

                if (changedurls.find(targeturl) != changedurls.end())
                {
                    SVNProperties props(targeturl, headrev, false, false);
                    if (!props.Add(SVN_PROP_EXTERNALS, it->second.extvalue, false, svn_depth_empty, message))
                        m_sError = props.GetLastErrorMessage();
                    else
                    {
                        headrev = props.GetRevision();
                    }
                    changedurls.insert(targeturl);
                }
            }
            else
            {
                SVNProperties props(it->first, SVNRev::REV_WC, false, false);
                if (!props.Add(SVN_PROP_EXTERNALS, it->second.extvalue))
                    m_sError = props.GetLastErrorMessage();
            }
        }
    }

    return m_sError.IsEmpty();
}

std::string SVNExternals::GetValue(const CTSVNPath& path) const
{
    std::string ret;
    for (auto it = cbegin(); it != cend(); ++it)
    {
        if (path.IsEquivalentToWithoutCase(it->path))
        {
            SVNRev rev = it->revision;
            SVNRev pegrev = it->pegrevision;
            CString peg;
            if (pegrev.IsValid() && !pegrev.IsHead())
                peg = _T("@") + pegrev.ToString();
            else
                peg.Empty();

            CString targetDir = it->targetDir;
            if (targetDir.Find(' ')>=0)
                targetDir = L"'" + targetDir + L"'";
            CString temp;
            if (rev.IsValid() && !rev.IsHead() && (!rev.IsEqual(pegrev)))
                temp.Format(_T("-r %s %s%s %s"), rev.ToString(), it->url, (LPCTSTR)peg, targetDir);
            else
                temp.Format(_T("%s%s %s"), it->url, (LPCTSTR)peg, targetDir);
            if (ret.size())
                ret += "\n";
            ret += CUnicodeUtils::StdGetUTF8((LPCTSTR)temp);
        }
    }

    return ret;
}

bool SVNExternals::RestoreExternals()
{
    for (std::map<CTSVNPath, std::string>::iterator it = m_originals.begin(); it != m_originals.end(); ++it)
    {
        SVNProperties props(it->first, SVNRev::REV_WC, false, false);
        props.Add(SVN_PROP_EXTERNALS, it->second);
    }

    return true;
}

CString SVNExternals::GetFullExternalUrl( const CString& extUrl, const CString& root, const CString& dirUrl )
{
    CStringA extUrlA = CUnicodeUtils::GetUTF8(extUrl);
    CStringA rootA = CUnicodeUtils::GetUTF8(root);
    CStringA dirUrlA = CUnicodeUtils::GetUTF8(dirUrl);
    CString url = extUrl;

    SVNPool pool;
    const char * pFullUrl = NULL;
    svn_wc_external_item2_t e;
    e.url = extUrlA;
    svn_error_t * error = resolve_relative_external_url(&pFullUrl, &e, rootA, dirUrlA, pool, pool);
    if ((error == NULL) && (pFullUrl))
        url = CUnicodeUtils::GetUnicode(pFullUrl);
    else
        svn_error_clear(error);

    return url;
}
