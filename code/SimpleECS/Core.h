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

#include <unordered_map>
#include <vector>
#include <forward_list>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <cxxabi.h>

#define POW_2_32 4294967296

#define BITSET_TYPE std::uint8_t
#define BITSET_TYPE_SIZE 8u

#ifndef USE_ECS_EVENTS
#define USE_ECS_EVENTS 1
#endif

#ifndef MAX_COMPONENT_AMOUNT
#define MAX_COMPONENT_AMOUNT 63
#endif

#ifndef MAX_ENTITY_AMOUNT
#define MAX_ENTITY_AMOUNT 100000
#endif

#include "Typedef.h"
#include "EventHandler.h"

namespace sEcs {

    static const uint32 INVALID = 0;
    static const EntityId ENTITY_NULL;
    static const std::nullptr_t NOT_AVAILABLE = nullptr;


#if USE_ECS_EVENTS==1

    namespace Events {

        struct ComponentAddedEvent {
            explicit ComponentAddedEvent(const EntityId& entityId) : entityId(entityId) {}

            EntityId entityId;
        };

        struct ComponentDeletedEvent {
            explicit ComponentDeletedEvent(const EntityId& entityId) : entityId(entityId) {}

            EntityId entityId;
        };

        struct EntityCreatedEvent {
            explicit EntityCreatedEvent(const EntityId& entityId) : entityId(entityId) {}

            EntityId entityId;
        };

        struct EntityErasedEvent {
            explicit EntityErasedEvent(const EntityId& entityId) : entityId(entityId) {}

            EntityId entityId;
        };

    }

#endif

    namespace Core_Intern {     // private

        template<size_t size>
        struct BitSet {

        public:
            BitSet() {
                arraySize = (size / BITSET_TYPE_SIZE) + 1;
            }

            inline void set(std::vector<uint32> *bits) {
                for (uint32 position : *bits)
                    set(position);
            }

            inline void set(uint32 bit) {
                bitset[bit / BITSET_TYPE_SIZE] |= BITSET_TYPE(1u) << (bit % BITSET_TYPE_SIZE);
            }

            inline void unset(uint32 bit) {
                bitset[bit / BITSET_TYPE_SIZE] &= ~(1u << (bit % BITSET_TYPE_SIZE));
            }

            inline void reset() {
                for (BITSET_TYPE &bitrow : bitset)
                    bitrow = 0;
            }

            inline bool isSet(uint32 bit) {
                return ((bitset[bit / BITSET_TYPE_SIZE] >> (bit % BITSET_TYPE_SIZE)) & BITSET_TYPE(1u)) == 1u;
            }

            inline bool contains(BitSet *other) {
                for (size_t i = 0; i < arraySize; ++i)
                    if ((other->bitset[i] & ~bitset[i]) > 0u)
                        return false;
                return true;
            }

        private:
            size_t arraySize;
            BITSET_TYPE bitset[(size / BITSET_TYPE_SIZE) + 1]{};

        };

        typedef BitSet<MAX_COMPONENT_AMOUNT> ComponentBitset;

        class EntityState {

        public:
            explicit EntityState() : version(INVALID), componentMask(ComponentBitset()) {}

            explicit EntityState(EntityVersion version) : version(version) {}

            EntityId id(EntityIndex index) const { return {version, index}; }

            ComponentBitset *getComponentMask() { return &componentMask; }

            void reset() {
                version++;
                componentMask.reset();
            }

            EntityVersion version;
            ComponentBitset componentMask;

        };


        class EntitySet {

        public:
            explicit EntitySet(std::vector<ComponentId> componentIds) :
                    componentIds(componentIds),
                    mask(ComponentBitset()) {
                std::sort(componentIds.begin(), componentIds.end());
                mask.set(&componentIds);
                entities.push_back(INVALID);
                entities.reserve(MAX_ENTITY_AMOUNT);
                internIndices.reserve(MAX_ENTITY_AMOUNT);
                freeInternIndices.reserve(MAX_ENTITY_AMOUNT);
            }

            inline void dumbAddIfMember(EntityId entityId, ComponentBitset *bitset) {
                if (bitset->contains(&mask))
                    add(entityId.index);
            }

            void updateMembership(EntityIndex entityIndex, ComponentBitset *previous, ComponentBitset *recent) {
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

            inline InternIndex next(InternIndex internIndex) {
                int entitiesSize = entities.size();
                do {
                    if (++internIndex >= entitiesSize) {
                        return INVALID; // End of Array
                    }
                } while (entities[internIndex] == INVALID);
                return internIndex;
            }

            inline EntityIndex getIndex(InternIndex internIndex) {
                return entities[internIndex];
            }

            void add(EntityIndex entityIndex) {
                if (!freeInternIndices.empty()) {
                    internIndices[entityIndex] = freeInternIndices.back();
                    freeInternIndices.pop_back();
                    entities[internIndices[entityIndex]] = entityIndex;
                } else {
                    entities.push_back(entityIndex);
                    internIndices[entityIndex] = entities.size() - 1;
                }
            }

            bool concern(std::vector<ComponentId> *vector) {
                if (componentIds.size() != vector->size())
                    return false;
                for (size_t i = 0; i < componentIds.size(); ++i)
                    if (componentIds[i] != (*vector)[i])
                        return false;
                return true;
            }

            uint32 getVagueAmount() {
                return entities.size() - freeInternIndices.size();
            }

        private:
            ComponentBitset mask;
            std::vector<ComponentId> componentIds;

            std::vector<EntityIndex> entities;

            std::vector<InternIndex> internIndices;  // We need this List to avoid double insertions
            std::vector<InternIndex> freeInternIndices;

        };


        class SetIterator {

        public:
            explicit SetIterator(EntitySet *entitySet) :
                    entitySet(entitySet) {}

            inline EntityIndex next() {
                iterator = entitySet->next(iterator);
                return entitySet->getIndex(iterator);
            }

            inline EntitySet* getEntitySet() {
                return entitySet;
            }

        private:
            EntitySet *entitySet;
            InternIndex iterator = 0;

        };

    }      // end private


#if USE_ECS_EVENTS == 1
    struct ComponentEventInfo {
        sEcs::uint32 addEventId = 0;
        sEcs::uint32 deleteEventId = 0;
    };
#endif


    class ComponentHandle {

    public:
        virtual ~ComponentHandle() = default;

        void destroyComponent(sEcs::EntityId entityId, sEcs::EntityIndex entityIndex) {
            destroyComponentIntern(entityIndex);
        }

        // only defined behavior for valid requests (entity and component exists)
        virtual void* getComponent(sEcs::EntityIndex entityIndex) = 0;

        // only defined behavior for valid requests (entity exists and component not)
        virtual void* createComponent(sEcs::EntityIndex entityIndex) = 0;

#if USE_ECS_EVENTS == 1

        ComponentEventInfo& getComponentEventInfo() {
            return componentInfo;
        }

    protected:
        ComponentEventInfo componentInfo;
#endif

    private:
        // only defined behavior for valid requests (entity and component exists)
        virtual void destroyComponentIntern(sEcs::EntityIndex entityIndex) = 0;

    };


    class Core : public sEcs::Events::EventHandler {

    public:

        Core(const Core&) = delete;

        explicit Core() :
                entities(std::vector<Core_Intern::EntityState>(MAX_ENTITY_AMOUNT + 1)) {
            componentHandles.reserve(MAX_COMPONENT_AMOUNT + 1);
            componentHandles.push_back(nullptr);
            entities[0] = Core_Intern::EntityState();
#if USE_ECS_EVENTS==1
            entityCreatedEventId = generateEvent();
            entityErasedEventId = generateEvent();
#endif
        }


        ~Core() {
            for (ComponentHandle *ch : componentHandles) delete ch;
            for (Core_Intern::EntitySet *set : entitySets) delete set;
            for (Core_Intern::SetIterator *setIterator : setIterators) delete setIterator;
        }


        EntityId createEntity() {

            EntityIndex index;

            if (!freeEntityIndices.empty()) {
                index = freeEntityIndices.back();
                freeEntityIndices.pop_back();
            } else {
                index = ++lastEntityIndex;
                entities[index] = Core_Intern::EntityState(EntityVersion(1));
            }

            EntityId entityId = entities[index].id(index);

#if USE_ECS_EVENTS==1
            auto event = Events::EntityCreatedEvent(entityId);
            emitEvent(entityCreatedEventId, &event);
#endif

            return entityId;
        }


        bool isValid(EntityId entityId) {
            return getIndex(entityId) != INVALID;
        }

        bool eraseEntity(EntityId entityId) {

            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return false;

#if USE_ECS_EVENTS==1
            auto eventErased = Events::EntityErasedEvent(entityId);
            emitEvent(entityErasedEventId, &eventErased);
#endif

            Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

            for (ComponentId i = 1; i < componentHandles.size(); i++) {
                ComponentHandle *ch = componentHandles[i];
                if (originally.isSet(i)) {   // Only delete existing components
                    ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
                    auto event = Events::ComponentDeletedEvent(entityId);
                    emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
                }
            }

            entities[index].reset();
            updateAllMemberships(entityId, &originally, entities[index].getComponentMask());

            freeEntityIndices.push_back(index);

            return true;
        }


        ComponentId registerComponent(ComponentHandle* ch) {

            if (componentHandles.size() >= MAX_COMPONENT_AMOUNT) {
                throw std::length_error("To many component types registered! Define by MAX_COMPONENT_AMOUNT.");
            }

#if USE_ECS_EVENTS==1
            ComponentEventInfo& componentInfo = ch->getComponentEventInfo();
            componentInfo.addEventId = generateEvent();
            componentInfo.deleteEventId = generateEvent();
#endif
            componentHandles.push_back(ch);

            return componentHandles.size() - 1;
        }

        void* addComponent(EntityId entityId, ComponentId componentId) {

            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return nullptr;

            ComponentHandle* ch = componentHandles[componentId];
            Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

            if (!originally.isSet( componentId )) {   // Only update if component type is new for entity
                entities[index].getComponentMask()->set( componentId );
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            } else {
                ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
                auto event = Events::ComponentDeletedEvent(entityId);
                emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
            }

            void* comp = ch->createComponent(index);

#if USE_ECS_EVENTS==1
            auto event = Events::ComponentAddedEvent(entityId);
            emitEvent(ch->getComponentEventInfo().addEventId, &event);
#endif

            return comp;
        }


        // OptimizeAble
        bool activateComponents(EntityId entityId, const ComponentId* ids, size_t idsAmount) {

            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return false;

            Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

            bool modified = false;
            for (uint32 i = 0; i < idsAmount; i++) {
                ComponentHandle* ch = componentHandles[ids[i]];
                if (originally.isSet(ids[i])) {
                    ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
                    auto event = Events::ComponentDeletedEvent(entityId);
                    emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
                }
                else
                    modified = true;
            }

            if (modified) {   // Only update if component types are new for entity

                for (uint32 i = 0; i < idsAmount; i++)
                    entities[index].getComponentMask()->set(ids[i]);

                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            }

#if USE_ECS_EVENTS==1
            for (uint32 i = 0; i < idsAmount; i++) {
                ComponentHandle* ch = componentHandles[ids[i]];
                auto event = Events::ComponentAddedEvent(entityId);
                emitEvent(ch->getComponentEventInfo().addEventId, &event);
            }
#endif

            return true;
        }


        void* getComponent(EntityId entityId, ComponentId componentId) {

            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return nullptr;

            if (!entities[index].getComponentMask()->isSet(componentId))
                return nullptr;

            return componentHandles[componentId]->getComponent(index);
        }


        bool deleteComponent(EntityId entityId, ComponentId componentId) {
            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return false;

            Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

            if (originally.isSet(componentId)) {
                auto* ch = componentHandles[componentId];
                ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
                auto event = Events::ComponentDeletedEvent(entityId);
                emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
                entities[index].getComponentMask()->unset( componentId );
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
                return true;
            }

            return false;
        }


        SetIteratorId createSetIterator(std::vector<ComponentId> componentIds) {

            std::sort(componentIds.begin(), componentIds.end());
            Core_Intern::EntitySet *entitySet = nullptr;

            for (Core_Intern::EntitySet *set : entitySets)
                if (set->concern(&componentIds))
                    entitySet = set;

            if (entitySet == nullptr) {
                entitySet = new Core_Intern::EntitySet(componentIds);
                entitySets.push_back(entitySet);

                // add all related Entities
                for (EntityIndex entityIndex = 1; entityIndex <= lastEntityIndex; entityIndex++)
                    entitySet->dumbAddIfMember(entities[entityIndex].id(entityIndex), entities[entityIndex].getComponentMask());
            }

            setIterators.push_back(new Core_Intern::SetIterator(entitySet));
            return static_cast<SetIteratorId>(setIterators.size() - 1);
        }


        inline EntityId nextEntity(SetIteratorId setIteratorId) {
            EntityIndex nextIndex = setIterators[setIteratorId]->next();
            return entities[nextIndex].id(nextIndex);
        }


        uint32 getEntityAmount(SetIteratorId setIteratorId) {
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }


        uint32 getEntityAmount(std::vector<ComponentId>& componentIds) {
            static SetIteratorId setIteratorId = createSetIterator(componentIds);
            while (nextEntity(setIteratorId).version != INVALID);
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }


        inline uint32 getEntityAmount() {
            return lastEntityIndex - freeEntityIndices.size();
        }


        inline EntityIndex getIndex(EntityId entityId) {
            if (entityId.index > lastEntityIndex)
                return INVALID;
            if (entityId.version != entities[entityId.index].version)
                return INVALID;
            return entityId.index;
        }


        EntityId getIdFromIndex(EntityIndex index) {
            if (index > lastEntityIndex)
                return ENTITY_NULL;
            return {entities[index].version, index};
        }


    private:
        EntityIndex lastEntityIndex = 0;

#if USE_ECS_EVENTS == 1
        uint32 entityCreatedEventId = generateEvent();
        uint32 entityErasedEventId = generateEvent();
#endif

        std::vector<Core_Intern::EntityState> entities;
        std::vector<EntityIndex> freeEntityIndices;

        std::vector<ComponentHandle *> componentHandles;

        std::vector<Core_Intern::EntitySet *> entitySets;
        std::vector<Core_Intern::SetIterator *> setIterators;

        void updateAllMemberships(EntityId entityId, Core_Intern::ComponentBitset *previous, Core_Intern::ComponentBitset *recent) {
            for (Core_Intern::EntitySet *set : entitySets) {
                set->updateMembership(entityId.index, previous, recent);
            }
        }

    };

}

#endif //SIMPLE_ECS_CORE_H
