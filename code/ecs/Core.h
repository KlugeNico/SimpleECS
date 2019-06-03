/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */

#ifndef SIMPLE_ECS_CORE_H
#define SIMPLE_ECS_CORE_H

#define POW_2_32 4294967296

#include <unordered_map>
#include <vector>
#include <forward_list>
#include <algorithm>

namespace EcsCore {

    typedef std::uint32_t uint32;
    typedef std::uint64_t uint64;
    typedef uint64 Entity_Id;
    typedef uint32 Entity_Version;
    typedef uint32 Entity_Index;
    typedef uint32 Component_Id;
    typedef uint32 SetIterator_Id;
    typedef uint32 Intern_Index;
    typedef std::string Component_Key;

    static const uint32 INVALID = 0;


    namespace EcsCoreIntern {     // private

        struct Bitset {

        public:
            explicit Bitset(uint32 maxBit) : bitset(std::vector<uint64>((maxBit / 64) + 1)) {
            }

            inline void set(std::vector<uint32> *bits) {
                for (uint32 position : *bits)
                    set(position);
            }

            inline void set(uint32 bit) {
                bitset[bit / 64] |= uint64(1) << (bit % 64);
            }

            inline void unset(uint32 bit) {
                bitset[bit / 64] &= ~(uint64(1) << (bit % 64));
            }

            inline void reset() {
                for (uint64 &bitrow : bitset)
                    bitrow = 0;
            }

            inline bool isSet(uint32 bit) {
                return ((bitset[bit / 64] >> (bit % 64)) & uint64(1)) == 1;
            }

            inline bool contains(Bitset *other) {
                for (uint32 i = 0; i < bitset.size(); ++i)
                    if ((other->bitset[i] & ~bitset[i]) > 0)
                        return false;
                return true;
            }

        private:
            std::vector<uint64> bitset;

        };


        class Entity {

        public:
            explicit Entity(Entity_Version version, uint32 maxComponentAmount) : version_(version), componentMask(
                    Bitset(maxComponentAmount)) {}

            Entity_Version version() const { return version_; }

            Entity_Id id(Entity_Index index) const { return (((Entity_Id) version_) * POW_2_32 | index); }

            Bitset *getComponentMask() { return &componentMask; }

            void reset() {
                version_++;
                componentMask.reset();
            }

        private:
            Entity_Version version_;
            Bitset componentMask;

        };


        class EntitySet {

        public:
            EntitySet(std::vector<Component_Id> componentIds, uint32 maxComponentAmount, uint32 maxEntityAmount) :
                    mask(Bitset(maxComponentAmount)),
                    componentIds(componentIds) {
                std::sort(componentIds.begin(), componentIds.end());
                mask.set(&componentIds);
                entities.push_back(INVALID);
                entities.reserve(maxEntityAmount);
                internIndices.reserve(maxEntityAmount);
                freeInternIndices.reserve(maxEntityAmount);
            }

            inline void dumbAddIfMember(Entity_Id entityId, Bitset *bitset) {
                if (bitset->contains(&mask))
                    add(entityId);
            }

            void updateMembership(Entity_Index entityIndex, Bitset *previous, Bitset *recent) {
                if (previous->contains(&mask)) {
                    if (recent->contains(&mask)) // nothing changed
                        return;

                    freeInternIndices.push_back(internIndices[entityIndex]);
                    entities[internIndices[entityIndex]] = INVALID; // doesn't contain entity any more
                    internIndices[entityIndex] = INVALID;
                    return;
                }

                if (!recent->contains(&mask)) // nothing changed
                    return;

                add(entityIndex);      // add entityId, because it's not added yet, but should be
            }

            inline Intern_Index next(Intern_Index internIndex) {
                int entitiesSize = entities.size();
                do {
                    if (++internIndex >= entitiesSize) {
                        return INVALID; // End of Array
                    }
                } while (entities[internIndex] == INVALID);
                return internIndex;
            }

            inline Entity_Index getIndex(Intern_Index internIndex) {
                return entities[internIndex];
            }

            inline Bitset *getMask() {
                return &mask;
            }

            void add(Entity_Index entityIndex) {
                if (!freeInternIndices.empty()) {
                    internIndices[entityIndex] = freeInternIndices.back();
                    freeInternIndices.pop_back();
                    entities[internIndices[entityIndex]] = entityIndex;
                } else {
                    entities.push_back(entityIndex);
                    internIndices[entityIndex] = entities.size() - 1;
                }
            }

            bool concern(std::vector<Component_Id> *vector) {
                if (componentIds.size() != vector->size())
                    return false;
                for (uint32 i = 0; i < componentIds.size(); ++i)
                    if (componentIds[i] != (*vector)[i])
                        return false;
                return true;
            }

            uint32 getVagueAmount() {
                return entities.size() - freeInternIndices.size();
            }

        private:
            Bitset mask;
            std::vector<Component_Id> componentIds;

            std::vector<Entity_Index> entities;

            std::vector<Intern_Index> internIndices;  // We need this List to avoid double insertions
            std::vector<Intern_Index> freeInternIndices;

        };


        class SetIterator {

        public:
            explicit SetIterator(EntitySet *entitySet) :
                    entitySet(entitySet) {}

            inline Entity_Index next() {
                iterator = entitySet->next(iterator);
                return entitySet->getIndex(iterator);
            }

            inline bool subserve(Entity *entity) {
                return entity->getComponentMask()->contains(entitySet->getMask());
            }

            inline EntitySet* getEntitySet() {
                return entitySet;
            }

        private:
            EntitySet *entitySet;
            Intern_Index iterator = 0;

        };


        class ComponentHandle {

        public:
            explicit ComponentHandle(Component_Id id, void(*destroy)(void *), uint32 maxEntities) :
                    id(id),
                    destroy(destroy),
                    components(std::vector<void *>(maxEntities + 1)) {}

            ~ComponentHandle() {
                for (uint32 i = 0; i < components.size(); i++) deleteComponent(i);
            }

            Component_Id getComponentId() {
                return id;
            }

            void *getComponent(Entity_Index entityIndex) {
                return components[entityIndex];
            }

            void addComponent(Entity_Index entityIndex, void *component) {
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
            std::vector<void *> components;

            void (*destroy)(void *);

            Component_Id id;

        };

    }      // end private


    class Manager {

    public:
        Manager(uint32 maxEntities, uint32 maxComponents) :
                maxEntities(maxEntities),
                maxComponents(maxComponents),
                componentVector(std::vector<EcsCoreIntern::ComponentHandle *>(maxComponents + 1)) {
            entities.reserve(maxEntities);
            entities.emplace_back(0,0);
        }

        ~Manager() {
            for (EcsCoreIntern::ComponentHandle *ch : componentVector) delete ch;
            for (EcsCoreIntern::EntitySet *set : entitySets) delete set;
            for (EcsCoreIntern::SetIterator *setIterator : setIterators) delete setIterator;
        }

        uint32 getMaxEntities() {
            return maxEntities;
        }

        uint32 getMaxComponents() {
            return maxComponents;
        }

        Entity_Id createEntity() {

            Entity_Index index;

            if (!freeEntityIndices.empty()) {
                index = freeEntityIndices.back();
                freeEntityIndices.pop_back();
            } else {
                index = ++lastEntityIndex;
                entities[index] = EcsCoreIntern::Entity(Entity_Version(1), maxComponents);
            }

            return entities[index].id(index);
        }

        bool isValid(Entity_Id entityId) {
            return getIndex(entityId) != INVALID;
        }

        bool eraseEntity(Entity_Id entityId) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return false;

            for (Component_Id i = 1; i <= lastComponentIndex; i++) {
                EcsCoreIntern::ComponentHandle *componentHandle = componentVector[i];
                componentHandle->deleteComponent(index);
            }
            EcsCoreIntern::Bitset originally = *entities[index].getComponentMask();
            entities[index].reset();
            updateAllMemberships(entityId, &originally, entities[index].getComponentMask());

            freeEntityIndices.push_back(index);

            return true;
        }

        template<typename T>
        void registerComponent(Component_Key componentKey) {
            getSetComponentHandle<T>(componentKey);
        }

        template <typename T>
        bool addComponent(Entity_Id entityId, T* component) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return false;

            if (!getComponentHandle<T>()->deleteComponent(index)) {   // Only update if component type is new for entity
                EcsCoreIntern::Bitset originally = *entities[index].getComponentMask();
                entities[index].getComponentMask()->set(getComponentId<T>());
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            }

            getComponentHandle<T>()->addComponent(index, reinterpret_cast<void*>(component));

            return true;
        }

        template<typename ... Ts>
        bool addComponents(Entity_Id entityId, Ts*... components) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return false;

            std::vector<EcsCoreIntern::ComponentHandle*> chs = std::vector<EcsCoreIntern::ComponentHandle*>(sizeof...(Ts));
            insertComponentHandles<Ts...>(&chs, 0, components...);

            bool modified = false;
            for (EcsCoreIntern::ComponentHandle* ch : chs)
                if (!ch->deleteComponent(index))
                    modified = true;

            if (modified) {   // Only update if component types are new for entity

                EcsCoreIntern::Bitset originally = *entities[index].getComponentMask();

                for (EcsCoreIntern::ComponentHandle* ch : chs)
                    entities[index].getComponentMask()->set(ch->getComponentId());

                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            }

            addComponentsToComponentHandles(index, components...);

            return true;
        }

        template<typename T>
        T *getComponent(Entity_Id entityId) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return nullptr;

            return reinterpret_cast<T *>(getComponentHandle<T>()->getComponent(index));
        }

        template<typename T>
        bool deleteComponent(Entity_Id entityId) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return false;

            if (getComponentHandle<T>()->deleteComponent(index)) {
                EcsCoreIntern::Bitset originally = *entities[index].getComponentMask();
                entities[index].getComponentMask()->unset(getComponentHandle<T>()->getComponentId());
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
                return true;
            }

            return false;
        }

        template<typename ... Ts>
        SetIterator_Id createSetIterator(Ts *... ts) {

            if (sizeof...(Ts) < 1)
                throw std::invalid_argument("SetIterator must subserve at least one Component!");

            std::vector<Component_Id> componentIds = std::vector<Component_Id>(sizeof...(Ts));
            insertComponentIds<Ts...>(&componentIds, ts...);
            std::sort(componentIds.begin(), componentIds.end());
            EcsCoreIntern::EntitySet *entitySet = nullptr;

            for (EcsCoreIntern::EntitySet *set : entitySets)
                if (set->concern(&componentIds))
                    entitySet = set;

            if (entitySet == nullptr) {
                entitySet = new EcsCoreIntern::EntitySet(componentIds, maxComponents, maxEntities);
                entitySets.push_back(entitySet);

                // add all related Entities
                for (Entity_Index entityIndex = 1; entityIndex <= lastEntityIndex; entityIndex++)
                    entitySet->dumbAddIfMember(entities[entityIndex].id(entityIndex), entities[entityIndex].getComponentMask());
            }

            setIterators.push_back(new EcsCoreIntern::SetIterator(entitySet));
            return static_cast<SetIterator_Id>(setIterators.size() - 1);
        }

        Entity_Id nextEntity(SetIterator_Id setIteratorId) {
            Entity_Index nextIndex = setIterators[setIteratorId]->next();
            return entities[nextIndex].id(nextIndex);
        }

        uint32 getEntityAmount(SetIterator_Id setIteratorId) {
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }

        template<typename ... Ts>
        uint32 getEntityAmount(Ts*... ts) {
            static SetIterator_Id setIteratorId = createSetIterator<Ts...>(ts...);
            while (nextEntity(setIteratorId) != INVALID);
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }

        uint32 getEntityAmount() {
            return lastEntityIndex - freeEntityIndices.size();
        }

    private:
        uint32 maxEntities;
        Entity_Index lastEntityIndex = 0;
        std::vector<EcsCoreIntern::Entity> entities;

        std::vector<Entity_Index> freeEntityIndices;

        uint32 maxComponents;
        Component_Id lastComponentIndex = 0;
        std::unordered_map<Component_Key, EcsCoreIntern::ComponentHandle *> componentMap;
        std::vector<EcsCoreIntern::ComponentHandle *> componentVector;

        std::vector<EcsCoreIntern::EntitySet *> entitySets;
        std::vector<EcsCoreIntern::SetIterator *> setIterators;

        template<typename T>
        Component_Id getComponentId() {
            static Component_Id id = getComponentHandle<T>()->getComponentId();
            return id;
        }

        template<typename ... Ts>
        void insertComponentIds(std::vector<Component_Id> *ids, Ts*... components) {
            std::vector<EcsCoreIntern::ComponentHandle*> chs = std::vector<EcsCoreIntern::ComponentHandle*>(sizeof...(Ts));
            insertComponentHandles<Ts...>(&chs, 0, components...);
            for (int i = 0; i < chs.size(); i++) {
                (*ids)[i] = chs[i]->getComponentId();
            }
        }

        template<typename T>
        void insertComponentHandles(std::vector<EcsCoreIntern::ComponentHandle *> *chs, int index, T* component) {
            (*chs)[index] = getComponentHandle<T>();
        }

        template<typename T, typename... Ts>
        void insertComponentHandles(std::vector<EcsCoreIntern::ComponentHandle *> *chs, int index, T* component, Ts*... components) {
            (*chs)[index] = getComponentHandle<T>();
            insertComponentHandles<Ts...>(chs, ++index, components...);
        }

        template<typename T>
        void addComponentsToComponentHandles(Entity_Index index, T* component) {
            getComponentHandle<T>()->addComponent(index, reinterpret_cast<void *>(component));
        }

        template<typename T, typename... Ts>
        void addComponentsToComponentHandles(Entity_Index index, T* component, Ts*... components) {
            getComponentHandle<T>()->addComponent(index, reinterpret_cast<void *>(component));
            addComponentsToComponentHandles<Ts...>(index, components...);
        }

        template<typename T>
        EcsCoreIntern::ComponentHandle *getComponentHandle() {
            static EcsCoreIntern::ComponentHandle *componentHandle = getSetComponentHandle<T>();
            return componentHandle;
        }

        template<typename T>
        EcsCoreIntern::ComponentHandle *linkComponentHandle(Component_Key *componentKey) {
            if (componentKey->empty())
                throw std::invalid_argument("Tried to get unregistered component!");
            if (componentMap[*componentKey] == nullptr) {
                auto *chp = new EcsCoreIntern::ComponentHandle(
                        ++lastComponentIndex,
                        [](void *x) { delete reinterpret_cast<T *>(x); },
                        maxEntities);
                componentMap[*componentKey] = chp;
                componentVector[chp->getComponentId()] = chp;
            }
            EcsCoreIntern::ComponentHandle *ch = componentMap[*componentKey];
            return ch;
        }

        template<typename T>
        EcsCoreIntern::ComponentHandle *getSetComponentHandle(Component_Key componentKey = Component_Key()) {
            static EcsCoreIntern::ComponentHandle *componentHandle = linkComponentHandle<T>(&componentKey);
            return componentHandle;
        }

        inline Entity_Index getIndex(Entity_Id entityId) {
            auto index = static_cast<Entity_Index>(entityId);
            if (index > lastEntityIndex)
                return INVALID;
            if (static_cast<Entity_Version>(entityId >> Entity_Id(32)) != entities[index].version())
                return INVALID;
            return index;
        }

        void updateAllMemberships(Entity_Id entityId, EcsCoreIntern::Bitset *previous, EcsCoreIntern::Bitset *recent) {
            for (EcsCoreIntern::EntitySet *set : entitySets) {
                set->updateMembership(entityId, previous, recent);
            }
        }

    };


}

#endif //SIMPLE_ECS_CORE_H
