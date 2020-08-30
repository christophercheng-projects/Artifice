#pragma once

#include <map>
#include <vector>

#include "Artifice/Core/Core.h"


template <class KEY, class VALUE, uint32 THRESHOLD>
class TemporaryCache
{
private:
    using Key = KEY;
    using ValueStack = std::vector<VALUE>;

    uint32 RingBackIndex = THRESHOLD - 1;

    std::map<Key, ValueStack> m_FreeRing[THRESHOLD];
    std::map<Key, ValueStack> m_ActiveList;

protected:
    virtual VALUE CreateValue(KEY key) = 0;
    virtual void DestroyValue(VALUE value) = 0;

public:

    void Reset()
    {
        ReleaseActive();
        m_ActiveList.clear();

        for (auto& ring : m_FreeRing)
        {
            for (auto& stack : ring)
            {
                for (auto& value : stack.second)
                {
                    DestroyValue(value);
                }
            }
            ring.clear();
        }
    }

    void Advance()
    {
        // Destroy all resources in back ring
        for (auto& pair : m_FreeRing[RingBackIndex])
        {
            for (auto& value : pair.second)
            {
                DestroyValue(value);
            }
        }
        // Rotate the rings
        for (int32 i = RingBackIndex; i > 0; i--)
        {
            m_FreeRing[i] = m_FreeRing[i - 1];
        }

        m_FreeRing[0].clear();
    }

    void ReleaseActive()
    {
        // Put all elements in the active list into the first free ring
        for (auto& pair : m_ActiveList)
        {
            const Key& key = pair.first;
            ValueStack& active_stack = pair.second;
            ValueStack& free_stack = m_FreeRing[0][key];

            free_stack.insert(free_stack.end(), active_stack.begin(), active_stack.end());

            active_stack.clear();
        }
    }

    VALUE Request(KEY key)
    {
        VALUE result;

        // Iterate from the last ring, which is up next to be deleted
        for (int32 i = RingBackIndex; i >= 0; i--)
        {
            ValueStack& stack = m_FreeRing[i][key];
            if (stack.empty())
            {
                // Check next ring item
                continue;
            }
            else
            {
                // Get from free list
                result = stack.back();
                // Remove from free list
                stack.pop_back();
                // add to active list
                m_ActiveList[key].push_back(result);
            }

            return result;
        }

        // If haven't returned by now, allocate new value
        result = CreateValue(key);
        m_ActiveList[key].push_back(result);

        return result;
    }

    uint64 GetAlive() const
    {
        uint64 count = 0;
        for (auto& ring : m_FreeRing)
        {
            for (auto& entry : ring)
            {
                count += entry.second.size();
            }
        }
        for (auto& pair : m_ActiveList)
        {
            count += pair.second.size();
        }

        return count;
    }
};

template <class KEY, class VALUE, uint32 THRESHOLD>
class TemporaryImmutableCache
{
private:
    using Key = KEY;
    using Value = VALUE;

    uint32 RingBackIndex = THRESHOLD - 1;

    std::map<Key, Value> m_Ring[THRESHOLD];

protected:
    virtual VALUE CreateValue(KEY key) = 0;
    virtual void DestroyValue(VALUE value) = 0;

public:

    void Reset()
    {
        for (auto& ring : m_Ring)
        {
            for (auto& entry : ring)
            {
                DestroyValue(entry.second);
            }
            ring.clear();
        }
    }

    void Advance()
    {
        uint32 destroy_count = 0;
        
        // Destroy all resources in back ring
        for (auto& entry : m_Ring[RingBackIndex])
        {
            DestroyValue(entry.second);
            destroy_count++;
        }

        // Rotate the rings
        for (uint32 i = RingBackIndex; i > 0; i--)
        {
            m_Ring[i] = m_Ring[i - 1];
        }

        m_Ring[0].clear();
    }

    VALUE Request(KEY key)
    {
        VALUE result;

        // Iterate from the last ring, which is up next to be deleted
        for (int32 i = RingBackIndex; i >= 0; i--)
        {
            if (m_Ring[i].count(key))
            {
                result = m_Ring[i][key];

                if (i != 0)
                {
                    // Move entry to first ring
                    m_Ring[i].erase(key);
                    m_Ring[0][key] = result;
                }

                return result;
            }
        }

        // If haven't returned by now, allocate new entry
        result = CreateValue(key);
        m_Ring[0][key] = result;

        return result;
    }

    uint64 GetAlive() const
    {
        uint64 count = 0;
        for (auto& ring : m_Ring)
        {
            count += ring.size();
        }

        return count;
    }
};