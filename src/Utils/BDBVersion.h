// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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

// Will be initialized with BDB's
//	DB_VERSION_MAJOR,
//	DB_VERSION_MINOR and	
//	DB_VERSION_PATCH,	
// respectively.

// This construct circumvents a conflict
// between DBD\db.h and MFC\oledb.h

extern const int g_iDB_VERSION_MAJOR;
extern const int g_iDB_VERSION_MINOR;
extern const int g_iDB_VERSION_PATCH;
