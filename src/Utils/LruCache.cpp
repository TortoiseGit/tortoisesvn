// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseSVN

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

#include "stdafx.h"
#include "LruCache.h"

#if defined(_DEBUG)
static class LruCacheTest
{
public:
    LruCacheTest()
    {
        {
            LruCache<int, int> cache(2);
            cache.insert_or_assign(1, 100);
            cache.insert_or_assign(2, 200);
            // Emulate access to key '1'
            cache.try_get(1);

            // Add third entry. Key '2' should be evicted/
            cache.insert_or_assign(3, 300);

            ATLASSERT(cache.try_get(1) != nullptr);
            ATLASSERT(*cache.try_get(1) == 100);

            ATLASSERT(cache.try_get(2) == nullptr);

            ATLASSERT(cache.try_get(3) != nullptr);
            ATLASSERT(*cache.try_get(3) == 300);

            // Test LruCache.clear()
            cache.clear();
            ATLASSERT(cache.try_get(1) == nullptr);
            ATLASSERT(cache.try_get(2) == nullptr);
            ATLASSERT(cache.try_get(3) == nullptr);

            cache.insert_or_assign(1, 100);
            cache.insert_or_assign(3, 300);

            ATLASSERT(cache.try_get(1) != nullptr);
            ATLASSERT(*cache.try_get(1) == 100);
            ATLASSERT(cache.try_get(3) != nullptr);
            ATLASSERT(*cache.try_get(3) == 300);

            // Replace value associated with key '1'.
            cache.insert_or_assign(1, 101);
            ATLASSERT(cache.try_get(1) != nullptr);

            ATLASSERT(*cache.try_get(1) == 101);
            ATLASSERT(cache.try_get(3) != nullptr);
            ATLASSERT(*cache.try_get(3) == 300);
        }

        {   // Test empty cache behavior
            LruCache<int, int> cache(5);
            ATLASSERT(cache.try_get(1) == nullptr);
            ATLASSERT(cache.try_get(2) == nullptr);
            ATLASSERT(cache.try_get(3) == nullptr);
        }
    }
} LruCacheTest;

#endif
