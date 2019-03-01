//
// Created by kreischenderdepp on 26.01.19.
//

#ifndef SIMPLE_ECS_MANAGER_H
#define SIMPLE_ECS_MANAGER_H

#include <unordered_map>
#include <vector>
#include <algorithm>

typedef std::uint8_t uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;
typedef uint64 Entity_Id;
typedef uint32 Entity_Version;
typedef uint32 Entity_Index;
typedef uint32 Component_Id;
typedef uint32 System_Id;
typedef uint32 Setintern_Index;
typedef std::string Component_Key;

static const uint32 INVALID = 0;


struct Bitset {

public:
    explicit Bitset(uint32 maxBit) : bits(std::vector<uint64>((maxBit / 64) + 1)) {
    }

    inline void set(std::vector<uint32>* bits) {
        for (uint32 position : *bits)
            set(position);
    }

    inline void set(uint32 bit) {
        bits[bit / 64] |= uint64(1) << (bit % 64);
    }

    inline void unset(uint32 bit) {
        bits[bit / 64] &= ~(uint64(1) << (bit % 64));
    }

    inline void reset() {
        for (uint64 &bitrow : bits)
            bitrow = 0;
    }

    inline bool isSet(uint32 bit) {
        return ((bits[bit / 64] >> (bit % 64)) & 1) == 1;
    }

    inline bool contains(Bitset* other) {
        for (uint32 i = 0; i < bits.size(); ++i)
            if((other->bits[i] & ~bits[i]) > 0)
                return false;
        return true;
    }

private:
    std::vector<uint64> bits;

};


class Entity {

public:
    explicit Entity(Entity_Version version, uint32 maxComponentAmount) : version_(version), componentMask_(Bitset(maxComponentAmount)) {}

    Entity_Version version() const { return version_; }
    Bitset componentMask() const { return componentMask_; }

    bool isInvalid(Entity_Version version) const { return version_ != version; }

    void reset() {
        version_++;
        componentMask_.reset();
    }

private:
    Entity_Version version_;
    Bitset componentMask_;

};


class EntitySet {

public:
    EntitySet (std::vector<Component_Id> componentIds, uint32 maxComponentAmount) :
            mask(Bitset(maxComponentAmount)),
            componentIds(componentIds)
    {
        std::sort(componentIds.begin(), componentIds.end());
        mask.set(&componentIds);
        entities.push_back(INVALID);
    }

    void updateMembership(Entity_Id entityId, Bitset* originally, Bitset* recent) {
        if (originally->contains(&mask)) {
            if (recent->contains(&mask)) // nothing changed
                return;

            doomed.push_back(entityId); // doesn't contain entity any more
            return;
        }

        if (!recent->contains(&mask)) // nothing changed
            return;

        if (!doomed.empty()) {
            std::vector<Entity_Id>::iterator iterator1;
            for (iterator1 = doomed.begin(); iterator1 != doomed.end(); ++iterator1) {
                if (*iterator1 == entityId) {
                    doomed.erase(iterator1); // entity was prepared to be deleted. But now we want to keep it.
                    return;
                }
            }
        }

        entities.push_back(entityId); // add entityId, because it's not added yet
    }

    inline Setintern_Index next(Setintern_Index internIndex) {
        do {
            if (++internIndex >= entities.size()) {
                return INVALID; // End of Array
            }
        }
        while (entities[internIndex] == INVALID);
        return internIndex;
    }

    inline Entity_Id get(Setintern_Index internIndex) {
        return entities[internIndex];
    }

    bool concern(std::vector<Component_Id>* vector) {
        if (componentIds.size() != vector->size())
            return false;
        for (uint32 i = 0; i < componentIds.size(); ++i)
            if (componentIds[i] != (*vector)[i])
                return false;
        return true;
    }

private:
    Bitset mask;
    std::vector<Component_Id> componentIds;

    std::vector<Entity_Id> entities;
    std::vector<Entity_Id> doomed;

    std::vector<Setintern_Index> freeEntityIndices;

};


class System {

public:
    explicit System(EntitySet* entitySet) :
            entitySet(entitySet)
    {}

    inline Entity_Id next() {
        iterator = entitySet->next(iterator);
        return entitySet->get(iterator);
    }

private:
    EntitySet* entitySet;
    Setintern_Index iterator = 0;

};


class ComponentHandle {

public:
    explicit ComponentHandle(Component_Id id, void(*destroy)(void*), uint32 maxEntities) :
            id(id),
            destroy(destroy),
            components(std::vector<void*>(maxEntities + 1))
    {}

    ~ComponentHandle() {
        for (uint32 i = 0; i < components.size(); i++) deleteComponent(i);
    }

    Component_Id getComponentId() {
        return id;
    }

    void* getComponent(Entity_Index entityIndex) {
        return components[entityIndex];
    }

    void addComponent(Entity_Index entityIndex, void* component) {
        components[entityIndex] = component;
    }

    bool deleteComponent(Entity_Index entityIndex) {
        if (components[entityIndex] == nullptr)
            return false;
        destroy(components[entityIndex]);
        components[entityIndex] = nullptr;
        return true;
    }

private:
    std::vector<void*> components;
    void(*destroy)(void*);
    Component_Id id;

};


class Manager {

public:
    Manager(uint32 maxEntities, uint32 maxComponents) :
        maxEntities(maxEntities),
        maxComponents(maxComponents),
        componentVector(std::vector<ComponentHandle*>(maxComponents + 1)),
        entities(std::vector<Entity*>(maxEntities + 1))
    {}

    ~Manager(){
        for (ComponentHandle* ch : componentVector) delete ch;
        for (Entity* entity : entities) delete entity;
    }

    uint32 getMaxEntities() {
        return maxEntities;
    }

    uint32 getMaxComponents() {
        return maxEntities;
    }

    Entity_Id createEntity() {

        Entity_Index index;
        Entity_Version version;

        if (!freeEntityIndices.empty()) {
            index = freeEntityIndices.back();
            freeEntityIndices.pop_back();
            version = entities[index]->version();
        }
        else {
            index = ++lastEntityIndex;
            version = 1;
            entities[index] = new Entity(version, maxComponents);
        }

        return ((uint64)version << 32) & index;
    }

    bool deleteEntity(Entity_Id id) {

        Entity_Index index = getIndex(id);
        if (index == INVALID)
            return false;

        for (Component_Id i = 1; i <= lastComponentIndex; i++) {
            ComponentHandle* componentHandle = componentVector[i];
            componentHandle->deleteComponent(index);
        }
        entities[index]->reset();

        return true;
    }

    template <typename T>
    bool registerComponent(Component_Key componentKey) {
        getSetComponentHandle<T>(componentKey);
    }

    template <typename T>
    bool addComponent(Entity_Id entityId, T* component) {

        Entity_Index index = getIndex(entityId);
        if (index == INVALID)
            return false;

        if (!getComponentHandle<T>()->deleteComponent(index))
            entities[index]->componentMask().set(getComponentId<T>());

        getComponentHandle<T>()->addComponent(index, reinterpret_cast<void*>(component));

        return true;
    }

    template <typename T>
    T* getComponent(Entity_Id entityId) {

        Entity_Index index = getIndex(entityId);
        if (index == INVALID)
            return nullptr;

        return reinterpret_cast<T*>(getComponentHandle<T>()->getComponent(index));
    }

    template <typename T>
    bool deleteComponent(Entity_Id entityId) {

        Entity_Index index = getIndex(entityId);
        if (index == INVALID)
            return false;

        if(getComponentHandle<T>()->deleteComponent(index))
            entities[index]->componentMask().unset(getComponentHandle<T>()->getIndex());

        return true;
    }

    template <typename ... Ts>
    System_Id createSystem() {
        if (sizeof...(Ts) < 1)
            throw std::invalid_argument("Tried to create System without components!");
        std::vector<Component_Id> componentIds = std::vector<Component_Id>(sizeof...(Ts));
        insertComponentIds<Ts...>(&componentIds, 0);
        std::sort(componentIds.begin(), componentIds.end());
        EntitySet* entitySet = nullptr;
        for (EntitySet* set : entitySets) {
            if (set->concern(&componentIds)) {
                entitySet = set;
            }
        }
        if (entitySet == nullptr) {
            entitySet = new EntitySet(componentIds, maxComponents);
            entitySets.push_back(entitySet);
        }
        systems.push_back(new System(entitySet));
        return static_cast<System_Id>(systems.size() - 1);
    }

    Entity_Id nextEntity(System_Id systemId) {
        Entity_Id entityId;
        Entity_Index entityIndex;

        while (true) {
            entityId = systems[systemId]->next();
            if (entityId == INVALID)
                return INVALID; // End of list

            entityIndex = getIndex(entityId);
            if (entityIndex == INVALID || !entities[entityIndex]->hasComponents(systems[systemId]))
                systems[systemId]->remove();    // Remove Entity, because it doesn't exist anymore or lost Components.
            else
                return entityId;
        }
    }

private:
    uint32 maxEntities;
    Entity_Index lastEntityIndex = 0;
    std::vector<Entity*> entities;

    std::vector<Entity_Index> freeEntityIndices;

    uint32 maxComponents;
    Component_Id lastComponentIndex = 0;
    std::unordered_map<Component_Key, ComponentHandle*> componentMap;
    std::vector<ComponentHandle*> componentVector;

    std::vector<EntitySet*> entitySets;
    std::vector<System*> systems;

    template <typename T>
    Component_Id getComponentId() {
        static Component_Id id = getComponentHandle<T>()->getComponentId();
        return id;
    }

    template <typename T, typename ... Ts>
    void insertComponentIds(std::vector<Component_Id>* ids, uint32 pos) {
        ids[pos] = getComponentId<T>();
        if (sizeof...(Ts) > 0)
            insertComponentIds<Ts...>(ids, ++pos);
    }

    template <typename T>
    ComponentHandle* getComponentHandle() {
        static ComponentHandle* componentHandle = getSetComponentHandle<T>();
        return componentHandle;
    }

    template <typename T>
    ComponentHandle* linkComponentHandle(Component_Key componentKey) {
        if (componentKey.empty())
            throw std::invalid_argument( "Tried to get unregistered component!");;
        if (componentMap[componentKey] == nullptr) {
            ComponentHandle* chp = new ComponentHandle(
                    ++lastComponentIndex,
                    [](void* x) { delete reinterpret_cast<T*>(x); },
                    maxEntities );
            componentMap[componentKey] = chp;
            componentVector[chp->getComponentId()] = chp;
        }
        ComponentHandle* ch = componentMap[componentKey];
        return ch;
    }

    template <typename T>
    ComponentHandle* getSetComponentHandle(Component_Key componentKey = Component_Key()) {
        static ComponentHandle* componentHandle = linkComponentHandle<T>(componentKey);
        return componentHandle;
    }

    inline Entity_Index getIndex(Entity_Id entityId) {
        Entity_Index index = static_cast<Entity_Index>(entityId);
        if (index >= entities.size() || index < 1)
            return INVALID;
        Entity_Version version = static_cast<Entity_Version>(entityId << 32);
        if (version != entities[index]->version())
            return INVALID;
        return index;
    }

};


#endif //SIMPLE_ECS_MANAGER_H
