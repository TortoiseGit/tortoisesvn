// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
//
#pragma once


/**
 * defines global types and their associated "invalid" values.
 */
namespace LogCache
{
#ifdef HUGE_LOG_CACHE

	typedef size_t index_t;
	typedef index_t revision_t;

	enum
	{
		NO_INDEX = (size_t)0xffffffffffffffff,
		NO_REVISION = NO_INDEX,
	};

#else

	/// log caching index
	typedef DWORD index_t;
	/// revision number
	typedef index_t revision_t;

	enum
	{
		/// invalid/unknown index
		NO_INDEX = (revision_t)0xffffffff,
		/// invalid/unknown revision
		NO_REVISION = NO_INDEX,
	};

#endif
};