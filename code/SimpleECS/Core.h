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

#include "Typedef.h"
#include "EventHandler.h"

namespace sEcs {

    static const uint32 INVALID = 0;
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
            explicit EntityState();

            explicit EntityState(EntityVersion version);

            EntityId id(EntityIndex index) const;

            ComponentBitset *getComponentMask();

            void reset();

            EntityVersion version;
            ComponentBitset componentMask;

        };


        class EntitySet {

        public:
            explicit EntitySet(std::vector<ComponentId> componentIds);

            inline void dumbAddIfMember(EntityId entityId, ComponentBitset *bitset) {
                if (bitset->contains(&mask))
                    add(entityId.index);
            }

            void updateMembership(EntityIndex entityIndex, ComponentBitset *previous, ComponentBitset *recent);

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

            void add(EntityIndex entityIndex);

            bool concern(std::vector<ComponentId> *vector);

            uint32 getVagueAmount();

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
        EventId addEventId = 0;
        EventId deleteEventId = 0;
    };
#endif


    class ComponentHandle {

    public:
        virtual ~ComponentHandle() = default;

        // only defined behavior for valid requests (entity and component exists)
        void destroyComponent(sEcs::EntityId entityId, sEcs::EntityIndex entityIndex) {
            destroyComponentIntern(entityIndex);
        }

        // only defined behavior for valid requests (entity and component exists)
        virtual void* getComponent(sEcs::EntityIndex entityIndex) = 0;

        // only defined behavior for valid requests (entity exists and component not)
        virtual void* createComponent(sEcs::EntityIndex entityIndex) = 0;

#if USE_ECS_EVENTS == 1

        ComponentEventInfo& getComponentEventInfo() {
            return componentEventInfo;
        }

    protected:
        ComponentEventInfo componentEventInfo;
#endif

    private:
        // only defined behavior for valid requests (entity and component exists)
        virtual void destroyComponentIntern(sEcs::EntityIndex entityIndex) = 0;

    };


    class Core : public sEcs::Events::EventHandler {

    public:

        Core(const Core&) = delete;

        explicit Core();

        ~Core();

        EntityId createEntity();

        bool isValid(EntityId entityId);

        bool eraseEntity(EntityId entityId);

        ComponentId registerComponent(ComponentHandle* ch);

        void* addComponent(EntityId entityId, ComponentId componentId);

        // OptimizeAble
        bool activateComponents(EntityId entityId, const ComponentId* ids, size_t idsAmount);

        void* getComponent(EntityId entityId, ComponentId componentId);

        bool deleteComponent(EntityId entityId, ComponentId componentId);

#if USE_ECS_EVENTS == 1

        EventId componentDeletedEventId(ComponentId componentId);

        EventId componentAddedEventId(ComponentId componentId);

        EventId entityCreatedEventId();

        EventId entityErasedEventId();

#endif

        SetIteratorId createSetIterator(std::vector<ComponentId> componentIds);

        inline EntityId nextEntity(SetIteratorId setIteratorId) {
            EntityIndex nextIndex = setIterators[setIteratorId]->next();
            return entities[nextIndex].id(nextIndex);
        }

        uint32 getEntityAmount(SetIteratorId setIteratorId);

        uint32 getEntityAmount(std::vector<ComponentId>& componentIds);

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

        EntityId getIdFromIndex(EntityIndex index);


    private:
        EntityIndex lastEntityIndex = 0;

#if USE_ECS_EVENTS == 1
        uint32 entityCreatedEventId_;
        uint32 entityErasedEventId_;
#endif

        std::vector<Core_Intern::EntityState> entities;
        std::vector<EntityIndex> freeEntityIndices;

        std::vector<ComponentHandle *> componentHandles;

        std::vector<Core_Intern::EntitySet *> entitySets;
        std::vector<Core_Intern::SetIterator *> setIterators;

        void updateAllMemberships(
                EntityId entityId, Core_Intern::ComponentBitset *previous, Core_Intern::ComponentBitset *recent);

    };

}

#endif //SIMPLE_ECS_CORE_H
