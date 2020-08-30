#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Log.h"

class EntityHandle
{
    template<uint32 RING_SIZE>
    friend class EntityHandleAllocator;

private:
    uint32 m_ID = 0;

public:
    EntityHandle() = default;
    EntityHandle(const EntityHandle& other) : m_ID(other.m_ID) {}

    EntityHandle& operator=(const EntityHandle& other)
    {
        m_ID = other.m_ID;
        return *this;
    }

    bool operator==(const EntityHandle& other) const
    {
        return GetCookie() == other.GetCookie();
    }
    bool operator!=(const EntityHandle& other) const
    {
        return !(*this == other);
    }
    bool operator<(const EntityHandle& other) const
    {
        return GetCookie() < other.GetCookie();
    }

    uint32 GetCookie() const { return m_ID; }
    uint32 GetID() const { return m_ID; }

    bool IsNull() const { return GetCookie() == 0; }
private:
    void SetID(uint32 id) { m_ID = id; }
};

template <uint32 RING_SIZE>
class EntityHandleAllocator
{
    friend class EntityHandle;
private:
    const uint32 RingBackIndex = RING_SIZE - 1;

private:

private:
    std::unordered_set<uint32> m_ActiveIDs;
    std::vector<uint32> m_InvalidIDs[RING_SIZE];
    std::vector<uint32> m_FreeIDs;

    uint32 m_NextIDs = 0;
public:
    EntityHandleAllocator() = default;

    void Advance()
    {
        // TODO
        // Using an index is definitely better, so don't need to rotate rings.

        // Recycle invalid ids from back ring to free list
        m_FreeIDs.insert(m_FreeIDs.end(), m_InvalidIDs[RingBackIndex].begin(), m_InvalidIDs[RingBackIndex].end());

        // Rotate the rings
        for (uint32 i = RingBackIndex; i > 0; i--)
        {
            m_InvalidIDs[i] = m_InvalidIDs[i - 1];
        }

        // First ring now clear (old back ring)
        m_InvalidIDs[0].clear();
    }

    EntityHandle Allocate()
    {
        uint32 id;
        if (m_FreeIDs.size())
        {
            id = m_FreeIDs.back();
            m_FreeIDs.pop_back();
        }
        else
        {
            id = m_NextIDs++;
        }
        
        m_ActiveIDs.insert(id);

        EntityHandle handle;
        handle.SetID(id);

        return handle;
    }

    void Release(const EntityHandle& handle)
    {
        uint32 id = handle.GetID();

        AR_CORE_ASSERT(IsActive(id), "Tried releasing entity handle that is not active");

        m_ActiveIDs.erase(id);
        m_InvalidIDs[0].push_back(id);
    }
private:
    bool IsActive(uint32 id)
    {
        return m_ActiveIDs.find(id) != m_ActiveIDs.end();
    }
};

namespace std
{
template <>
struct hash<EntityHandle>
{
    size_t operator()(const EntityHandle& k) const
    {
        return k.GetCookie();
    }
};
} // namespace std

template <class T>
class TypeIdGenerator
{
private:
    static uint32 m_Count;

public:
    template <class U>
    static const uint32 GetNewID()
    {
        static const uint32 idCounter = m_Count++;
        return idCounter;
    }
};

template <class T>
uint32 TypeIdGenerator<T>::m_Count = 0;

class ComponentBase
{
};

template <class C>
class Component : public ComponentBase
{
public:
    static uint32 GetTypeID()
    {
        return TypeIdGenerator<ComponentBase>::GetNewID<C>();
    }
};

struct ArchetypeDescription
{
    std::set<uint32> ComponentTypes;
    bool Contains(uint32 id) const
    {
        return ComponentTypes.find(id) != ComponentTypes.end();
    }
    uint32 GetIndex(uint32 id) const
    {
        uint32 index = 0;
        for (auto& comp : ComponentTypes)
        {
            if (id == comp)
            {
                return index;
            }
            index++;
        }
        AR_CORE_FATAL();
    }
};

struct Archetype
{
    ArchetypeDescription Description;
    std::vector<std::vector<uint8>> ComponentData;
    std::vector<EntityHandle> EntityHandles;

    template <class C>
    C* GetComponents()
    {
        // todo check to make sure has component
        uint32 comp_id = Component<C>::GetTypeID();
        uint32 index = Description.GetIndex(comp_id);
        return (C*)ComponentData[index].data();
    }
    std::vector<uint8>& GetComponentData(uint32 comp_id)
    {
        // todo check to make sure valid comp id
        uint32 index = Description.GetIndex(comp_id);
        return ComponentData[index];
    }
    bool Contains(uint32 comp_id) const
    {
        return Description.Contains(comp_id);
    }
    template <class C>
    bool Contains()
    {
        uint32 comp_id = Component<C>::GetTypeID();
        return Description.Contains(comp_id);
    }
};

struct EntityRegistry
{
    EntityHandleAllocator<8> m_HandleAllocator;
    
    struct EntityRecord
    {
        Archetype* Archetype;
        uint32 Index;
    };

    std::unordered_map<EntityHandle, EntityRecord> m_Entities;
    std::vector<Archetype*> m_Archetypes;
    std::unordered_map<uint32, uint32> m_ComponentSizes;

    void Advance()
    {
        m_HandleAllocator.Advance();
    }

    template<class C>
    void RegisterComponent()
    {
        uint32 id = Component<C>::GetTypeID();
        AR_CORE_ASSERT(m_ComponentSizes.find(id) == m_ComponentSizes.end(), "Tried registering component that was already registered");

        m_ComponentSizes[id] = sizeof(C);
    }

    EntityHandle CreateEntity()
    {
        EntityHandle result = m_HandleAllocator.Allocate();
        m_Entities.insert({result, {nullptr, 0}});
        return result;
    }

    void DestroyEntity(EntityHandle entity)
    {
        AR_CORE_ASSERT(m_Entities.find(entity) != m_Entities.end(), "Tried destroying invalid entity");
        
        EntityRecord record = m_Entities[entity];

        uint32 back_index = record.Archetype->EntityHandles.size() - 1;
        // Swap and pop
        EntityHandle back_entity = record.Archetype->EntityHandles.back();
        record.Archetype->EntityHandles[record.Index] = back_entity;
        record.Archetype->EntityHandles.pop_back();

        m_Entities[back_entity].Index = record.Index;

        for (auto& comp_id : record.Archetype->Description.ComponentTypes)
        {
            uint32 comp_size = m_ComponentSizes[comp_id];
            std::vector<uint8>& data = record.Archetype->GetComponentData(comp_id);

            memcpy(&data[record.Index * comp_size], &data[back_index * comp_size], comp_size);
            data.resize(back_index * comp_size);
        }

        m_Entities.erase(entity);
        m_HandleAllocator.Release(entity);
    }

    template <class C>
    void getid(std::set<uint32>& ids)
    {
        ids.insert(Component<C>::GetTypeID());
    }

    template <class First, class Second, class... Rest>
    void getid(std::set<uint32>& ids)
    {
        getid<First>(ids);
        getid<Second, Rest...>(ids);
    }

    template<class C, class... Cs>
    std::vector<EntityHandle> GetEntitiesWith()
    {
        std::set<uint32> ids;
        getid<C, Cs...>(ids);

        std::vector<EntityHandle> result;
        
        for (auto& archetype : m_Archetypes)
        {
            for (auto& id : ids)
            {
                if (!archetype->Description.Contains(id))
                {
                    break;
                }
            }

            result.insert(result.begin(), archetype->EntityHandles.begin(), archetype->EntityHandles.end());
        }

        return result;
    }
    template<class A, class... As>
    std::vector<Archetype*> GetArchetypesWith()
    {
        std::set<uint32> ids;
        getid<A, As...>(ids);

        std::vector<Archetype*> result;
        
        for (Archetype* archetype : m_Archetypes)
        {
            for (auto& id : ids)
            {
                if (!archetype->Description.Contains(id))
                {
                    break;
                }
            }

            result.push_back(archetype);
        }

        return result;
    }
    
    std::vector<EntityHandle> GetAllEntities()
    {
        std::vector<EntityHandle> result;

        for (auto& archetype : m_Archetypes)
        {
            result.insert(result.begin(), archetype->EntityHandles.begin(), archetype->EntityHandles.end());
        }

        return result;
    }

    template <class C>
    C* GetComponent(EntityHandle entity)
    {
        uint32 id = Component<C>::GetTypeID();
        AR_CORE_ASSERT(m_ComponentSizes.find(id) != m_ComponentSizes.end(), "Tried getting component that has not been registered");
        uint32 size = m_ComponentSizes[id];

        EntityRecord record = m_Entities[entity];
        return (C*)&record.Archetype->GetComponentData(id)[record.Index * size];
    }

    template <class C>
    void RemoveComponent(EntityHandle entity)
    {
        uint32 id = Component<C>::GetTypeID();
        AR_CORE_ASSERT(m_ComponentSizes.find(id) != m_ComponentSizes.end(), "Tried removing component that has not been registered");
        uint32 size = m_ComponentSizes[id];

        EntityRecord old_record = m_Entities[entity];
        AR_CORE_ASSERT(old_record.Archetype->Contains(id), "Tried getting component that has not been registered");

        ArchetypeDescription arch_desc = old_record.Archetype->Description;
        arch_desc.ComponentTypes.erase(id);

        EntityRecord new_record;
        new_record.Archetype = RequestArchetype(arch_desc);
        new_record.Index = new_record.Archetype->EntityHandles.size();
        new_record.Archetype->EntityHandles.push_back(entity);

        uint32 old_back_index = old_record.Archetype->EntityHandles.size() - 1;
        // Swap and pop
        EntityHandle old_back_entity = old_record.Archetype->EntityHandles.back();
        old_record.Archetype->EntityHandles[old_record.Index] = old_back_entity;
        old_record.Archetype->EntityHandles.pop_back();

        m_Entities[old_back_entity].Index = old_record.Index;

        for (auto& comp_id : old_record.Archetype->Description.ComponentTypes)
        {
            uint32 comp_size = m_ComponentSizes[comp_id];
            std::vector<uint8>& old_data = old_record.Archetype->GetComponentData(comp_id);

            if (comp_id != id)
            {
                std::vector<uint8>& new_data = new_record.Archetype->GetComponentData(comp_id);
                new_data.resize((new_record.Index + 1) * size);
                memcpy(&new_data[new_record.Index * comp_size], &old_data[old_record.Index * comp_size], comp_size);
            }

            memcpy(&old_data[old_record.Index * comp_size], &old_data[old_back_index * comp_size], comp_size);
            old_data.resize(old_back_index * comp_size);
        }

        m_Entities[entity] = new_record;
    }
    
    template <class C, typename... Args>
    void AddComponent(EntityHandle entity, Args&&... args)
    {
        uint32 id = Component<C>::GetTypeID();
        AR_CORE_ASSERT(m_ComponentSizes.find(id) != m_ComponentSizes.end(), "Tried adding component that has not been registered");
        uint32 size = m_ComponentSizes[id];

        EntityRecord old_record = m_Entities[entity];

        C* component = nullptr;

        if (old_record.Archetype)
        {
            ArchetypeDescription arch_desc = old_record.Archetype->Description;
            arch_desc.ComponentTypes.insert(id);

            EntityRecord new_record;
            new_record.Archetype = RequestArchetype(arch_desc);
            new_record.Index = new_record.Archetype->EntityHandles.size();
            new_record.Archetype->EntityHandles.push_back(entity);

            uint32 old_back_index = old_record.Archetype->EntityHandles.size() - 1;
            // Swap and pop
            EntityHandle old_back_entity = old_record.Archetype->EntityHandles.back();
            old_record.Archetype->EntityHandles[old_record.Index] = old_back_entity;
            old_record.Archetype->EntityHandles.pop_back();

            m_Entities[old_back_entity].Index = old_record.Index;

            for (auto& comp_id : old_record.Archetype->Description.ComponentTypes)
            {
                uint32 comp_size = m_ComponentSizes[comp_id];
                std::vector<uint8>& old_data = old_record.Archetype->GetComponentData(comp_id);
                std::vector<uint8>& new_data = new_record.Archetype->GetComponentData(comp_id);

                new_data.resize((new_record.Index + 1) * size);
                memcpy(&new_data[new_record.Index * comp_size], &old_data[old_record.Index * comp_size], comp_size);

                memcpy(&old_data[old_record.Index * comp_size], &old_data[old_back_index * comp_size], comp_size);
                old_data.resize(old_back_index * comp_size);
            }

            std::vector<uint8>& new_data = new_record.Archetype->GetComponentData(id);
            new_data.resize((new_record.Index + 1) * size);
            component = new (&new_data[new_record.Index * size]) C(std::forward<Args>(args)...);

            m_Entities[entity] = new_record;
        }
        else
        {
            ArchetypeDescription arch_desc;
            arch_desc.ComponentTypes.insert(id);

            EntityRecord new_record;
            new_record.Archetype = RequestArchetype(arch_desc);
            new_record.Index = new_record.Archetype->EntityHandles.size();
            new_record.Archetype->EntityHandles.push_back(entity);

            std::vector<uint8>& new_data = new_record.Archetype->GetComponentData(id);
            new_data.resize((new_record.Index + 1) * size);
            component = new (&new_data[new_record.Index * size]) C(std::forward<Args>(args)...);

            m_Entities[entity] = new_record;
        }
    }

    template <class C>
    void AddComponent(EntityHandle entity, C component)
    {
        uint32 id = Component<C>::GetTypeID();
        AR_CORE_ASSERT(m_ComponentSizes.find(id) != m_ComponentSizes.end(), "Tried adding component that has not been registered");
        uint32 size = m_ComponentSizes[id];

        EntityRecord old_record = m_Entities[entity];


        if (old_record.Archetype)
        {
            ArchetypeDescription arch_desc = old_record.Archetype->Description;
            arch_desc.ComponentTypes.insert(id);

            EntityRecord new_record;
            new_record.Archetype = RequestArchetype(arch_desc);
            new_record.Index = new_record.Archetype->EntityHandles.size();
            new_record.Archetype->EntityHandles.push_back(entity);

            uint32 old_back_index = old_record.Archetype->EntityHandles.size() - 1;
            // Swap and pop
            EntityHandle old_back_entity = old_record.Archetype->EntityHandles.back();
            old_record.Archetype->EntityHandles[old_record.Index] = old_back_entity;
            old_record.Archetype->EntityHandles.pop_back();

            m_Entities[old_back_entity].Index = old_record.Index;

            for (auto& comp_id : old_record.Archetype->Description.ComponentTypes)
            {
                uint32 comp_size = m_ComponentSizes[comp_id];
                std::vector<uint8>& old_data = old_record.Archetype->GetComponentData(comp_id);
                std::vector<uint8>& new_data = new_record.Archetype->GetComponentData(comp_id);

                new_data.resize((new_record.Index + 1) * size);
                memcpy(&new_data[new_record.Index * comp_size], &old_data[old_record.Index * comp_size], comp_size);

                memcpy(&old_data[old_record.Index * comp_size], &old_data[old_back_index * comp_size], comp_size);
                old_data.resize(old_back_index * comp_size);
            }

            std::vector<uint8>& new_data = new_record.Archetype->GetComponentData(id);
            new_data.resize((new_record.Index + 1) * size);
            C comp = component;
            memcpy(&new_data[new_record.Index * size], &comp, size);

            m_Entities[entity] = new_record;
        }
        else
        {
            ArchetypeDescription arch_desc;
            arch_desc.ComponentTypes.insert(id);

            EntityRecord new_record;
            new_record.Archetype = RequestArchetype(arch_desc);
            new_record.Index = new_record.Archetype->EntityHandles.size();
            new_record.Archetype->EntityHandles.push_back(entity);

            std::vector<uint8>& new_data = new_record.Archetype->GetComponentData(id);
            new_data.resize((new_record.Index + 1) * size);
            C comp = component;
            memcpy(&new_data[new_record.Index * size], &comp, size);

            m_Entities[entity] = new_record;
        }
    }

    Archetype* RequestArchetype(const ArchetypeDescription& desc)
    {
        for (Archetype* arch : m_Archetypes)
        {
            if (arch->Description.ComponentTypes == desc.ComponentTypes)
            {
                return arch;
            }
        }

        Archetype* archetype = new Archetype();
        archetype->Description = desc;
        m_Archetypes.push_back(archetype);
        archetype->ComponentData.resize(desc.ComponentTypes.size(), {});

        return archetype;
    }
};
