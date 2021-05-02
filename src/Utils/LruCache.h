// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016, 2021 - TortoiseSVN

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

#include <unordered_map>
#include <list>

template <typename KeyT, typename ValueT>
class LruCache
{
public:
    LruCache(size_t maxSize)
        : maxSize(maxSize)
    {
    }

    void insert_or_assign(const KeyT& key, const ValueT& val)
    {
        auto mapIt = itemsMap.find(key);
        if (mapIt == itemsMap.end())
        {
            evict(maxSize - 1);

            auto listIt = itemsList.insert(itemsList.cend(), ListItem(key, val));
            itemsMap.insert(std::make_pair(key, listIt));
        }
        else
        {
            mapIt->second->val = val;
        }
    }

    const ValueT* try_get(const KeyT& key)
    {
        auto it = itemsMap.find(key);
        if (it == itemsMap.end())
            return nullptr;

        // Move last recently accessed item to the end.
        if (it->second != itemsList.end())
        {
            itemsList.splice(itemsList.end(), itemsList, it->second);
        }

        return &it->second->val;
    }

    void reserve(size_t size)
    {
        itemsMap.reserve(min(maxSize, size));
    }

    void clear()
    {
        itemsMap.clear();
        itemsList.clear();
    }

protected:
    void evict(size_t itemsToKeep)
    {
        for (auto it = itemsList.begin(); itemsList.size() > itemsToKeep && it != itemsList.end();)
        {
            itemsMap.erase(it->key);
            it = itemsList.erase(it);
        }
    }

private:
    struct ListItem
    {
        ListItem(const KeyT& key, const ValueT& val)
            : key(key)
            , val(val)
        {
        }

        KeyT   key;
        ValueT val;
    };

    size_t                                                           maxSize;
    std::unordered_map<KeyT, typename std::list<ListItem>::iterator> itemsMap;
    std::list<ListItem>                                              itemsList;
};
