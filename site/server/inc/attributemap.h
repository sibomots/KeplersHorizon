#ifndef __KH_ATTRIBUTE_MAP_H__
#define __KH_ATTRIBUTE_MAP_H__

#include <map>

enum class AttributeID : int
{
    POWER_DRIVE,
    BEAM,
    SCREEN,
    TUBE,
    MISSILE,
    SYSTEM_RACK,
    UNKNOWN,
};

// Attribute value type (all attributes are currently int)
typedef int AttributeValue;

// Map of attributes for commands
//typedef std::map<AttributeID, AttributeValue> AttributeMap;

typedef struct AttributeMap
{
    std::map<AttributeID, AttributeValue> data;
    AttributeMap()
    {
        data[AttributeID::POWER_DRIVE] =
            data[AttributeID::BEAM] =
            data[AttributeID::SCREEN] =
            data[AttributeID::TUBE] =
            data[AttributeID::MISSILE] =
            data[AttributeID::SYSTEM_RACK] = 0;
    }

    AttributeMap(int pd, int b, int s, int t, int sr = 0, int m = 0) {
        data[AttributeID::POWER_DRIVE] = pd;
        data[AttributeID::BEAM] = b;
        data[AttributeID::SCREEN] = s;
        data[AttributeID::TUBE] = t;
        data[AttributeID::MISSILE] = m;
        data[AttributeID::SYSTEM_RACK] = sr;
    }

    AttributeMap(const AttributeMap& rhs) = default;
    AttributeMap(AttributeMap&& rhs) noexcept = default;

    AttributeMap& operator=(const AttributeMap& rhs) = default;
    AttributeMap& operator=(AttributeMap&& rhs) noexcept = default;

    bool operator==(const AttributeMap& rhs) const {
        return data == rhs.data; // std::map comparison handles all keys automatically
    }

    const AttributeValue& operator[](AttributeID key) const {
        return data.at(key);
    }

    AttributeValue& operator[](AttributeID key) {
        // Simplified: The map handles the lookup. 
        // Use a single check for validity if you want to restrict keys.
        if (data.find(key) != data.end()) {
            return data[key];
        }
        return data[AttributeID::UNKNOWN];
    }

    void clear() {
       data.clear();
    } 

    std::pair<std::map<AttributeID, AttributeValue>::iterator, bool> 
    insert(const std::pair<AttributeID, AttributeValue>& value) {
        return data.insert(value);
    }

    template <typename V>
    auto insert_or_assign(AttributeID key, V&& value) {
        return data.insert_or_assign(key, std::forward<V>(value));
    }

    std::pair<std::map<AttributeID, AttributeValue>::iterator, bool> 
    insert(AttributeID key, AttributeValue val) {
        return data.insert({key, val});
    }

    using iterator = std::map<AttributeID, AttributeValue>::iterator;
    using const_iterator = std::map<AttributeID, AttributeValue>::const_iterator;

    // Non-const iterators (for modification)
    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }

    // Const iterators (for read-only access)
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }
    
    // Explicit const iterators
    const_iterator cbegin() const { return data.cbegin(); }
    const_iterator cend() const { return data.cend(); }

} AttributeMap;

#endif

