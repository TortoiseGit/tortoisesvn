// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
//
#pragma once
#include "svn_diff.h"
#include "diff.h"
#include "svn_pools.h"

class SVNLineDiff
{
public:
	SVNLineDiff();
	~SVNLineDiff();

	bool Diff(svn_diff_t** diff, LPCTSTR line1, int len1, LPCTSTR line2, int len2, bool bWordDiff);

public:
	static svn_error_t * datasource_open(void *baton, svn_diff_datasource_e datasource);
	static svn_error_t * datasource_close(void *baton, svn_diff_datasource_e datasource);
	static svn_error_t * next_token(apr_uint32_t * hash, void ** token, void * baton, svn_diff_datasource_e datasource);
	static svn_error_t * compare_token(void * baton, void * token1, void * token2, int * compare);
	static void discard_token(void * baton, void * token);
	static void discard_all_token(void *baton);

	apr_uint32_t Adler32(apr_uint32_t checksum, const WCHAR *data, apr_size_t len);
	std::vector<std::wstring>	m_line1tokens;
	std::vector<std::wstring>	m_line2tokens;
private:
	apr_pool_t *		m_pool;
	apr_pool_t *		m_subpool;
	LPCTSTR				m_line1;
	unsigned long		m_line1length;
	LPCTSTR				m_line2;
	unsigned long		m_line2length;
	TCHAR				m_token;
	unsigned long		m_line1pos;
	unsigned long		m_line2pos;

	bool				m_bWordDiff;


	static const svn_diff_fns_t SVNLineDiff_vtable;
};