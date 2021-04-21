// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2021, 2021 - TortoiseSVN

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

extern char* g_pConfigDir;

/**
 * \ingroup SVN
 * Global class to set the path to the directory where the m_config files are
 * stored. This is used to override the default path from the command line with
 * the /configdir:"path\to\m_config\dir" param (as the CL client has with the
 * --m_config-dir param.
 */
class SVNGlobal
{
public:
    SVNGlobal();
    ~SVNGlobal();
    static void SetConfigDir(CString sConfigDir);
};

// ReSharper disable once CppInconsistentNaming
extern SVNGlobal g_SVNGlobal;
