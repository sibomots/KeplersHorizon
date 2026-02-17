///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_ATTRIBUTE_MAP_H__
#define __KH_ATTRIBUTE_MAP_H__

#include <map>

enum class AttributeID : int
{
    POWER_DRIVE,
    PHASIC,
    SHIELD,
    LAUNCHER,
    TORPEDO,
    HANGAR,
    UNKNOWN,
};

// Attribute value type (all attributes are currently int)
typedef int AttributeValue;

typedef struct AttributeMap
{
    std::map<AttributeID, AttributeValue> data;
    AttributeMap()
    {
    }

    AttributeMap(int pd, int b, int s, int t, int hangar = 0, int m = 0)
    {
        data[AttributeID::POWER_DRIVE] = pd;
        data[AttributeID::PHASIC] = b;
        data[AttributeID::SHIELD] = s;
        data[AttributeID::LAUNCHER] = t;
        if (m > 0)
        {
            data[AttributeID::TORPEDO] = m;
        }
        if (hangar > 0)
        {
            data[AttributeID::HANGAR] = hangar;
        }
    }

    AttributeMap(const AttributeMap& rhs) = default;
    AttributeMap(AttributeMap&& rhs) noexcept = default;

    AttributeMap& operator=(const AttributeMap& rhs) = default;
    AttributeMap& operator=(AttributeMap&& rhs) noexcept = default;

    bool operator==(const AttributeMap& rhs) const
    {
        return data ==
               rhs.data; // std::map comparison handles all keys automatically
    }

    const AttributeValue& operator[](AttributeID key) const
    {
        return data.at(key);
    }

    AttributeValue& operator[](AttributeID key)
    {
        // Simplified: The map handles the lookup.
        // Use a single check for validity if you want to restrict keys.
        return data[key];
    }

    void clear()
    {
        data.clear();
    }

    std::pair<std::map<AttributeID, AttributeValue>::iterator, bool>
    insert(const std::pair<AttributeID, AttributeValue>& value)
    {
        return data.insert(value);
    }

    template <typename V> auto insert_or_assign(AttributeID key, V&& value)
    {
        return data.insert_or_assign(key, std::forward<V>(value));
    }

    std::pair<std::map<AttributeID, AttributeValue>::iterator, bool>
    insert(AttributeID key, AttributeValue val)
    {
        return data.insert({key, val});
    }

    using iterator = std::map<AttributeID, AttributeValue>::iterator;
    using const_iterator =
        std::map<AttributeID, AttributeValue>::const_iterator;

    // Non-const iterators (for modification)
    iterator begin()
    {
        return data.begin();
    }
    iterator end()
    {
        return data.end();
    }

    // Const iterators (for read-only access)
    const_iterator begin() const
    {
        return data.begin();
    }
    const_iterator end() const
    {
        return data.end();
    }

    // Explicit const iterators
    const_iterator cbegin() const
    {
        return data.cbegin();
    }
    const_iterator cend() const
    {
        return data.cend();
    }

} AttributeMap;

#endif
