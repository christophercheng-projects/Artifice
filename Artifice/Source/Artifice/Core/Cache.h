#pragma once

#include <map>


template <class KEY, class VALUE>
class ImmutableCache
{
private:
    using Key = KEY;
    using Value = VALUE;

    std::map<KEY, VALUE> m_Storage;

protected:
    virtual VALUE CreateValue(KEY key) = 0;
    virtual void DestroyValue(VALUE value) = 0;

public:
    ImmutableCache() = default;

    void Reset()
    {
        for (auto& entry : m_Storage)
        {
            DestroyValue(entry.second);
        }
        m_Storage.clear();
    }

    VALUE Request(KEY key)
    {
        VALUE result;

        auto find = m_Storage.find(key);
        if (find != m_Storage.end())
        {
            return find->second;
        }

        // if did not find, allocate new entry
        result = CreateValue(key);
        m_Storage[key] = result;

        return result;
    }

    VALUE* RequestPointer(KEY key)
    {
        auto find = m_Storage.find(key);
        if (find != m_Storage.end())
        {
            return &find->second;
        }

        // if did not find, allocate new entry
        m_Storage[key] = CreateValue(key);

        return &m_Storage[key];
    }
};