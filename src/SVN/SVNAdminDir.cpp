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
#include "UnicodeUtils.h"
#include "SVNAdminDir.h"

SVNAdminDir g_SVNAdminDir;

SVNAdminDir::SVNAdminDir() :
	m_nInit(0)
{
}

SVNAdminDir::~SVNAdminDir()
{
	if (m_nInit)
		svn_pool_destroy(m_pool);
}

bool SVNAdminDir::Init()
{
	if (m_nInit==0)
	{
		m_bVSNETHack = false;
		m_pool = svn_pool_create(NULL);
		size_t ret = 0;
		getenv_s(&ret, NULL, 0, "SVN_ASP_DOT_NET_HACK");
		if (ret)
		{
			svn_wc_set_adm_dir("_svn", m_pool);
			m_bVSNETHack = true;
		}
	}
	m_nInit++;
	return true;
}

bool SVNAdminDir::Close()
{
	m_nInit--;
	if (m_nInit>0)
		return false;
	svn_pool_destroy(m_pool);
	return true;
}

bool SVNAdminDir::IsAdminDirName(const CString& name)
{
	CStringA nameA = CUnicodeUtils::GetUTF8(name);
	return !!svn_wc_is_adm_dir(nameA, m_pool);
}

bool SVNAdminDir::HasAdminDir(const CString& path)
{
	return HasAdminDir(path, !!PathIsDirectory(path));
}

bool SVNAdminDir::HasAdminDir(const CString& path, bool bDir)
{
	if (path.IsEmpty())
		return false;
	bool bHasAdminDir = false;
	CString sDirName = path;
	if (!bDir)
	{
		sDirName = path.Left(path.ReverseFind('\\'));
	}
	bHasAdminDir = !!PathFileExists(sDirName + _T("\\.svn"));
	if (!bHasAdminDir && m_bVSNETHack)
		bHasAdminDir = !!PathFileExists(sDirName + _T("\\_svn"));
	return bHasAdminDir;
}

bool SVNAdminDir::IsAdminDirPath(const CString& path)
{
	if (path.IsEmpty())
		return false;
	bool bIsAdminDir = false;
	int ind = path.Find(_T(".svn"));
	if (ind >= 0)
	{
		if (ind == (path.GetLength() - 4))
		{
			if ((ind == 0)||(path.GetAt(ind-1) == '\\'))
				bIsAdminDir = true;
		}
		else if (path.Find(_T(".svn\\"))>=0)
		{
			if ((ind == 0)||(path.GetAt(ind-1) == '\\'))
				bIsAdminDir = true;
		}
	}
	if (!bIsAdminDir && m_bVSNETHack)
	{
		ind = path.Find(_T("_svn"));
		if (ind >= 0)
		{
			if (ind == (path.GetLength() - 4))
			{
				if ((ind == 0)||(path.GetAt(ind-1) == '\\'))
					bIsAdminDir = true;
			}
			else if (path.Find(_T("_svn\\"))>=0)
			{
				if ((ind == 0)||(path.GetAt(ind-1) == '\\'))
					bIsAdminDir = true;
			}
		}
	}
	return bIsAdminDir;
}



