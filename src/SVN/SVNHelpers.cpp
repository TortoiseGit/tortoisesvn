// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "StdAfx.h"
#include "SVNHelpers.h"
#include "TSVNPath.h"
#pragma warning(push)
#include "svn_config.h"
#include "svn_pools.h"
#include "svn_client.h"
#pragma warning(pop)

SVNPool::SVNPool()
{
    m_pool = svn_pool_create(NULL);
}

SVNPool::SVNPool(apr_pool_t* parentPool)
{
    m_pool = svn_pool_create(parentPool);
}

SVNPool::~SVNPool()
{
    svn_pool_destroy(m_pool);
}

SVNPool::operator apr_pool_t*()
{
    return m_pool;
}


// The time is not yet right for this base class, but I'm thinking about it...

SVNHelper::SVNHelper(void)
    : m_ctx(NULL)
    , m_bCancelled(false)
{
    m_pool = svn_pool_create (NULL);                // create the memory pool

    svn_error_clear(svn_client_create_context(&m_ctx, m_pool));
    m_ctx->cancel_func = cancelfunc;
    m_ctx->cancel_baton = this;
    svn_error_clear(svn_config_get_config(&(m_config), NULL, m_pool));
}

SVNHelper::~SVNHelper(void)
{
    svn_pool_destroy (m_pool);
}

void SVNHelper::ReloadConfig()
{
    svn_error_clear(svn_config_get_config(&(m_config), NULL, m_pool));
    m_ctx->config = m_config;
}

svn_client_ctx_t * SVNHelper::ClientContext(apr_pool_t * pool) const
{
    if (pool == NULL)
        return m_ctx;

    svn_client_ctx_t * ctx;
    svn_error_clear(svn_client_create_context(&ctx, pool));
    ctx->cancel_func = cancelfunc;
    ctx->cancel_baton = (void *)this;
    ctx->config = m_config;

    return ctx;
}

svn_error_t * SVNHelper::cancelfunc(void * cancelbaton)
{
    SVNHelper * helper = (SVNHelper*)cancelbaton;
    if ((helper)&&(helper->m_bCancelled))
    {
#ifdef IDS_SVN_USERCANCELLED
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
#else
        return svn_error_create(SVN_ERR_CANCELLED, NULL, "");
#endif
    }
    return NULL;
}

#ifndef SVN_NONET
bool SVNHelper::IsVersioned( const CTSVNPath& path )
{
    if (!path.Exists())
        return false;
    SVNPool pool;
    svn_wc_context_t * pctx = NULL;
    svn_error_t * err = svn_wc_context_create(&pctx, NULL, pool, pool);
    if (err)
    {
        svn_error_clear(err);
        return false;
    }
    int wcformat = 0;
    err = svn_wc_check_wc2(&wcformat, pctx, path.GetDirectory().GetSVNApiPath(pool), pool);
    if (err)
    {
        switch (err->apr_err)
        {
        case SVN_ERR_WC_NOT_WORKING_COPY:
        case SVN_ERR_WC_NOT_FILE:
        case SVN_ERR_WC_PATH_NOT_FOUND:
        case SVN_ERR_WC_CORRUPT:
        case SVN_ERR_WC_CORRUPT_TEXT_BASE:
        case SVN_ERR_WC_UNSUPPORTED_FORMAT:
        case SVN_ERR_WC_DB_ERROR:
        case SVN_ERR_WC_MISSING:
        case SVN_ERR_WC_PATH_UNEXPECTED_STATUS:
        case SVN_ERR_WC_UPGRADE_REQUIRED:
        case SVN_ERR_WC_CLEANUP_REQUIRED:
            {
                svn_error_clear(err);
                return false;
            }
            break;
        default:
            svn_error_clear(err);
            if (err->apr_err >= APR_OS_START_SYSERR)
                return false;   // assume unversioned in case we can't even access the path
            return true;
            break;
        }
    }

    return wcformat != 0;
}

#endif
