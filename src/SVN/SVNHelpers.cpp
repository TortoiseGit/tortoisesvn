// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#include "StdAfx.h"
#include "SVNHelpers.h"


SVNPool::SVNPool()
{
	m_pool = svn_pool_create(NULL);
	svn_utf_initialize(m_pool);
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
{
	m_bCancelled = false;
	m_pool = svn_pool_create (NULL);				// create the memory pool
	
	svn_utf_initialize(m_pool);

	// Why do we do this?
	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", m_pool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", m_pool);

	svn_client_create_context(&m_ctx, m_pool);
	m_ctx->cancel_func = cancelfunc;
	m_ctx->cancel_baton = this;

}

SVNHelper::~SVNHelper(void)
{
	svn_pool_destroy (m_pool);
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