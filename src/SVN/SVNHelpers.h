// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007,2010 - TortoiseSVN

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

#pragma once

/*
 * allow compilation even if the following structs have not been
 * defined yet (i.e. in case of missing includes).
 */

struct apr_pool_t;
struct svn_client_ctx_t;
struct svn_error_t;

/**
 * \ingroup SVN
 * This class encapsulates an apr_pool taking care of destroying it at end of scope
 * Use this class in preference to doing svn_pool_create and then trying to remember all
 * the svn_pool_destroys which might be needed.
 */
class SVNPool
{
public:
    SVNPool();
    explicit SVNPool(apr_pool_t* parentPool);
    ~SVNPool();
private:
    // Not implemented - we don't want any copying of these objects
    SVNPool(const SVNPool& rhs);
    SVNPool& operator=(SVNPool& rhs);

public:
    operator apr_pool_t*();

private:
    apr_pool_t* m_pool;
};


//////////////////////////////////////////////////////////////////////////



class SVNHelper
{
public:
    SVNHelper(void);
    ~SVNHelper(void);
private:
    SVNHelper(const SVNHelper&){}
    SVNHelper& operator=(SVNHelper&){};

public:
    apr_pool_t*         Pool() const { return m_pool; }
    svn_client_ctx_t*   ClientContext() const { return m_ctx; }
    void                Cancel(bool bCancelled = true) {m_bCancelled = bCancelled;}
    void                ReloadConfig();
protected:
    apr_pool_t *        m_pool;
    svn_client_ctx_t *  m_ctx;
    bool                m_bCancelled;

    static svn_error_t * cancelfunc(void * cancelbaton);
};

