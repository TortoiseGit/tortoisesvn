// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014-2015 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "SVNAdminDir.h"

SVNAdminDir g_SVNAdminDir;

SVNAdminDir::SVNAdminDir()
    : m_nInit(0)
    , m_bVSNETHack(false)
    , m_pool(NULL)
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
            svn_error_clear(svn_wc_set_adm_dir("_svn", m_pool));
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

bool SVNAdminDir::IsAdminDirName(const CString& name) const
{
    CStringA nameA = CUnicodeUtils::GetUTF8(name);
    try
    {
        nameA.MakeLower();
    }
    // catch everything: with MFC, an CInvalidArgException is thrown,
    // but with ATL only an CAtlException is thrown - and those
    // two exceptions are incompatible
    catch(...)
    {
        return false;
    }
    return !!svn_wc_is_adm_dir(nameA, m_pool);
}

bool SVNAdminDir::IsAdminDirPath(const CString& path) const
{
    if (path.IsEmpty())
        return false;
    bool bIsAdminDir = false;
    CString lowerpath = path;
    lowerpath.MakeLower();
    int ind = -1;
    int ind1 = 0;
    while ((ind1 = lowerpath.Find(L"\\.svn", ind1))>=0)
    {
        ind = ind1++;
        if (ind == (lowerpath.GetLength() - 5))
        {
            bIsAdminDir = true;
            break;
        }
        else if (lowerpath.Find(L"\\.svn\\", ind)>=0)
        {
            bIsAdminDir = true;
            break;
        }
    }
    if (!bIsAdminDir && m_bVSNETHack)
    {
        ind = -1;
        ind1 = 0;
        while ((ind1 = lowerpath.Find(L"\\_svn", ind1))>=0)
        {
            ind = ind1++;
            if (ind == (lowerpath.GetLength() - 5))
            {
                bIsAdminDir = true;
                break;
            }
            else if (lowerpath.Find(L"\\_svn\\", ind)>=0)
            {
                bIsAdminDir = true;
                break;
            }
        }
    }
    return bIsAdminDir;
}

bool SVNAdminDir::IsWCRoot(const CString& path) const
{
    if (PathIsUNCServer(path))
        return false;
    return IsWCRoot(path, !!PathIsDirectory(path));
}

bool SVNAdminDir::IsWCRoot(const CString& path, bool bDir) const
{
    // for now, the svn admin dir is always located at the wc root.
    // once that changes (maybe in svn 1.10?) we have to change
    // this function

    // Note: we don't use svn_wc_is_wc_root2() here because
    // this function needs to be *very* fast and must not
    // require that apr and svn is initialized or use memory pools

    if (path.IsEmpty())
        return false;
    if (PathIsUNCServer(path))
        return false;
    bool bIsWCRoot = false;
    CString sDirName = path;
    if (!bDir)
    {
        sDirName = path.Left(path.ReverseFind('\\'));
    }
    bIsWCRoot = !!PathFileExists(sDirName + L"\\.svn");
    if (!bIsWCRoot && m_bVSNETHack)
        bIsWCRoot = !!PathFileExists(sDirName + L"\\_svn");
    return bIsWCRoot;
}


