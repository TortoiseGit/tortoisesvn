// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013, 2021 - TortoiseSVN

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
#include "SVNHelpers.h"
#include "TSVNPath.h"
#include "SVNConfig.h"
#include "../version.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "UnicodeUtils.h"
#pragma warning(push)
#include "svn_config.h"
#include "svn_pools.h"
#include "svn_client.h"
#pragma warning(pop)

SVNPool::SVNPool()
{
    m_pool = svn_pool_create(nullptr);
}

SVNPool::SVNPool(apr_pool_t* parentPool)
{
    m_pool = svn_pool_create(parentPool);
}

SVNPool::~SVNPool()
{
    svn_pool_destroy(m_pool);
}

SVNPool::operator apr_pool_t*() const
{
    return m_pool;
}

// The time is not yet right for this base class, but I'm thinking about it...

SVNHelper::SVNHelper()
    : m_ctx(nullptr)
    , m_bCancelled(false)
{
    m_pool = svn_pool_create(nullptr); // create the memory pool

    m_config = SVNConfig::Instance().GetConfig(m_pool);
    svn_error_clear(svn_client_create_context2(&m_ctx, m_config, m_pool));
    m_ctx->cancel_func  = cancelFunc;
    m_ctx->cancel_baton = this;
    m_ctx->client_name  = SVNHelper::GetUserAgentString(m_pool);
}

SVNHelper::~SVNHelper()
{
    svn_pool_destroy(m_pool);
}

svn_client_ctx_t* SVNHelper::ClientContext(apr_pool_t* pool) const
{
    if (pool == nullptr)
        return m_ctx;

    svn_client_ctx_t* ctx = nullptr;
    svn_error_clear(svn_client_create_context2(&ctx, SVNConfig::Instance().GetConfig(pool), pool));
    if (ctx)
    {
        ctx->cancel_func  = cancelFunc;
        ctx->cancel_baton = reinterpret_cast<void*>(const_cast<SVNHelper*>(this));
    }

    return ctx;
}

svn_error_t* SVNHelper::cancelFunc(void* cancelBaton)
{
    SVNHelper* helper = static_cast<SVNHelper*>(cancelBaton);
    if ((helper) && (helper->m_bCancelled))
    {
#ifdef IDS_SVN_USERCANCELLED
        CString temp;
        temp.LoadString(IDS_SVN_USERCANCELLED);
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, CUnicodeUtils::GetUTF8(temp));
#else
        return svn_error_create(SVN_ERR_CANCELLED, nullptr, "");
#endif
    }
    return nullptr;
}

bool SVNHelper::IsVersioned(const CTSVNPath& path, bool mustBeOk)
{
    if (!path.Exists())
        return false;
    SVNPool         pool;
    svn_node_kind_t kind         = svn_node_none;
    CTSVNPath       pathdir      = path.GetDirectory();
    const char*     localAbspath = pathdir.GetSVNApiPath(pool);
    if ((localAbspath == nullptr) || (localAbspath[0] == 0))
        return false;

    svn_wc_context_t* pCtx = nullptr;
    svn_error_t*      err  = svn_wc_context_create(&pCtx, nullptr, pool, pool);
    if (err)
    {
        svn_error_clear(err);
        return false;
    }
    err = svn_wc_read_kind2(&kind, pCtx, localAbspath, true, true, pool);
    if (err)
    {
        switch (err->apr_err)
        {
            case SVN_ERR_WC_NOT_WORKING_COPY:
            case SVN_ERR_WC_NOT_FILE:
            case SVN_ERR_WC_PATH_NOT_FOUND:
            case SVN_ERR_UNSUPPORTED_FEATURE: // junctions and hardlinks are not supported
            {
                svn_error_clear(err);
                svn_wc_context_destroy(pCtx);
                return false;
            }
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
                svn_wc_context_destroy(pCtx);
                if (mustBeOk)
                    return false;
                return true;
            }
            default:
                if (err->apr_err >= APR_OS_START_SYSERR)
                {
                    svn_error_clear(err);
                    svn_wc_context_destroy(pCtx);
                    return false; // assume unversioned in case we can't even access the path
                }
                svn_error_clear(err);
                svn_wc_context_destroy(pCtx);
                return true;
        }
    }

    svn_wc_context_destroy(pCtx);

    return kind != svn_node_none;
}

const char* SVNHelper::GetUserAgentString(apr_pool_t* pool)
{
    char namestring[MAX_PATH] = {0};
    sprintf_s(namestring, "TortoiseSVN-%d.%d.%d.%d", TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
    return apr_pstrdup(pool, namestring);
}
