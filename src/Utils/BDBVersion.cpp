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
//
#include "BDBVersion.h"

#ifdef _WIN64
#include "..\..\ext\berkeley-db\db4.4-x64\include\db.h"
#else
#include "..\..\ext\berkeley-db\db4.4-win32\include\db.h"
#endif

const int g_iDB_VERSION_MAJOR = DB_VERSION_MAJOR;
const int g_iDB_VERSION_MINOR = DB_VERSION_MINOR;
const int g_iDB_VERSION_PATCH = DB_VERSION_PATCH;

