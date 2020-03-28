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

#define BITSET_TYPE std::uint8_t
#define BITSET_TYPE_SIZE 8u

#ifndef CORE_ECS_EVENTS
    #define CORE_ECS_EVENTS 1
#endif

#ifndef MAX_COMPONENT_AMOUNT
    #define MAX_COMPONENT_AMOUNT 63
#endif

#ifndef MAX_ENTITY_AMOUNT
    #define MAX_ENTITY_AMOUNT 100000
#endif

#include <unordered_map>
#include <vector>
#include <forward_list>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <cxxabi.h>
#include "EventHandler.h"

namespace EcsCore {

    enum Storing { POINTER, VALUE };
    static const Storing DEFAULT_STORING = Storing::VALUE;

    typedef std::uint32_t uint32;
    typedef std::uint64_t uint64;
    typedef uint32 EntityVersion;
    typedef uint32 EntityIndex;
    typedef uint32 ComponentId;
    typedef uint32 SetIteratorId;
    typedef uint32 InternIndex;
    typedef std::string Key;

    struct EntityId {
        uint32 version;
        uint32 index;

        EntityId() : version(0), index(0) {};

        explicit EntityId(uint64 asUint64) {
            index = (uint32)asUint64;
            version = asUint64 >> 32u;
        };
        EntityId(EntityVersion version, EntityIndex index) : version(version), index(index) {};
        bool operator == (const EntityId& other) const {
            return index == other.index && version == other.version;
        }
        bool operator != (const EntityId& other) const {
            return index != other.index || version != other.version;
        }
        explicit operator uint64() const {
            return (((uint64) version) * POW_2_32 | index);
        }
    };

    static const uint32 INVALID = 0;
    static const EntityId ENTITY_NULL;
    static const std::nullptr_t NOT_AVAILABLE = nullptr;

#if CORE_ECS_EVENTS==1
    template <typename T>
    struct ComponentAddedEvent {
        explicit ComponentAddedEvent (const EntityId& entityId) : entityId(entityId) {}
        EntityId entityId;
    };

    template <typename T>
    struct ComponentDeletedEvent {
        explicit ComponentDeletedEvent (const EntityId& entityId) : entityId(entityId) {}
        EntityId entityId;
    };

    struct EntityCreatedEvent {
        explicit EntityCreatedEvent (const EntityId& entityId) : entityId(entityId) {}
        EntityId entityId;
    };

    struct EntityErasedEvent {
        explicit EntityErasedEvent (const EntityId& entityId) : entityId(entityId) {}
        EntityId entityId;
    };
#endif

    namespace EcsCoreIntern {     // private

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


        struct ComponentInfo {
            ComponentId id;
            size_t typeSize;
#if CORE_ECS_EVENTS==1
            void (*emitDeleteEvent)(SimpleEH::SimpleEventHandler&, EntityId);
#endif
        };


        class ComponentHandle {

        public:
            virtual ~ComponentHandle() = default;

            explicit ComponentHandle(const ComponentInfo& componentInfo) :
                    componentInfo(componentInfo) {}

            ComponentId getComponentId() {
                return componentInfo.id;
            }

            void destroyComponent(EntityId entityId, EntityIndex entityIndex
#if CORE_ECS_EVENTS==1
                    , SimpleEH::SimpleEventHandler& eventHandler
#endif
                    ) {
                destroyComponentIntern(entityIndex);
#if CORE_ECS_EVENTS==1
                componentInfo.emitDeleteEvent(eventHandler, entityId);
#endif
            }

            // only defined behavior for valid requests (entity and component exists)
            virtual void* getComponent(EntityIndex entityIndex) = 0;

            // only defined behavior for valid requests (entity exists and component not)
            virtual void* createComponent(EntityIndex entityIndex) = 0;

        protected:
            ComponentInfo componentInfo;

        private:
            // only defined behavior for valid requests (entity and component exists)
            virtual void destroyComponentIntern(EntityIndex entityIndex) = 0;

        };


        class PointingComponentHandle : public ComponentHandle {

        public:
            explicit PointingComponentHandle(const ComponentInfo& componentInfo, void(*deleteFunc)(void *)) :
                    deleteFunc(deleteFunc), ComponentHandle(componentInfo),
                    components(std::vector<void *>(MAX_ENTITY_AMOUNT + 1)) {}

            ~PointingComponentHandle() override {
                for (auto & component : components) {
                    deleteFunc(component);
                    component = nullptr;
                }
            }

            void* getComponent(EntityIndex entityIndex) override {
                return components[entityIndex];
            }

            void* createComponent(EntityIndex entityIndex) override {
                return components[entityIndex] = malloc(componentInfo.typeSize);
            }

            void destroyComponentIntern(EntityIndex entityIndex) override {
                deleteFunc(getComponent(entityIndex));
                components[entityIndex] = nullptr;
            }

        private:
            void (*deleteFunc)(void *);
            std::vector<void *> components;

        };


        class ValuedComponentHandle : public ComponentHandle {

        public:
            explicit ValuedComponentHandle(const ComponentInfo& componentInfo, void(*destroyFunc)(void *)) :
                    destroyFunc(destroyFunc), ComponentHandle(componentInfo) {
                data = std::vector<char>((MAX_ENTITY_AMOUNT + 1) * componentInfo.typeSize);
            }

            void* getComponent(EntityIndex entityIndex) override {
                return &data[entityIndex * componentInfo.typeSize];
            }

            void* createComponent(EntityIndex entityIndex) override {
                return &data[entityIndex * componentInfo.typeSize];
            }

            void destroyComponentIntern(EntityIndex entityIndex) override {
                destroyFunc(getComponent(entityIndex));
            }

        private:
            void (*destroyFunc)(void *);
            std::vector<char> data;

        };


    }      // end private


    class Manager : public SimpleEH::SimpleEventHandler {

    public:
        Manager(const Manager&) = delete;

        explicit Manager() :
                componentVector(std::vector<EcsCoreIntern::ComponentHandle *>(MAX_COMPONENT_AMOUNT + 1)),
                entities(std::vector<EcsCoreIntern::EntityState>(MAX_ENTITY_AMOUNT + 1)) {
            entities[0] = EcsCoreIntern::EntityState();
        }

        ~Manager() {
            for (EcsCoreIntern::ComponentHandle *ch : componentVector) delete ch;
            for (EcsCoreIntern::EntitySet *set : entitySets) delete set;
            for (EcsCoreIntern::SetIterator *setIterator : setIterators) delete setIterator;
            for (EcsCoreIntern::ComponentHandle* (*func) (Manager*, const Key*, Storing)
                    : getSetComponentHandleFuncList) {
                Key resetCode(ECS_RESET_CODE);
                func(this, &resetCode, DEFAULT_STORING);
            }
        }

        EntityId createEntity() {

            EntityIndex index;

            if (!freeEntityIndices.empty()) {
                index = freeEntityIndices.back();
                freeEntityIndices.pop_back();
            } else {
                index = ++lastEntityIndex;
                entities[index] = EcsCoreIntern::EntityState(EntityVersion(1));
            }

            EntityId entityId = entities[index].id(index);

#if CORE_ECS_EVENTS==1
            emitEvent(EntityCreatedEvent(entityId));
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

#if CORE_ECS_EVENTS==1
            emitEvent(EntityErasedEvent(entityId));
#endif

            EcsCoreIntern::ComponentBitset originally = *entities[index].getComponentMask();

            for (ComponentId i = 1; i <= lastComponentIndex; i++) {
                EcsCoreIntern::ComponentHandle *ch = componentVector[i];
                if (originally.isSet(ch->getComponentId())) {   // Only delete existing components
                    ch->destroyComponent(entityId, index
#if CORE_ECS_EVENTS==1
                    , *this
#endif
                    );
                }
            }

            entities[index].reset();
            updateAllMemberships(entityId, &originally, entities[index].getComponentMask());

            freeEntityIndices.push_back(index);

            return true;
        }

        template<typename T>
        void registerComponent(Key componentKey, Storing storing = DEFAULT_STORING) {
            getSetComponentHandle<T>(this, &componentKey, storing);
        }

        template <typename T>
        T* addComponent(EntityId entityId, T&& component) {

            EcsCoreIntern::ComponentHandle* ch = getSetComponentHandle<T>(this, nullptr, VALUE);

            EntityIndex index = addComponentAdjustmentTypeless(entityId, ch);
            if (index == INVALID)
                return nullptr;

            addComponentsToComponentHandles<T>(index, component);

            return getComponent<T>(entityId);
        }

        template<typename ... Ts>
        bool addComponents(EntityId entityId, Ts&&... components) {

            std::vector<EcsCoreIntern::ComponentHandle*> chs = std::vector<EcsCoreIntern::ComponentHandle*>(sizeof...(Ts));
            insertComponentHandles<void, Ts...>(&chs, 0);

            EntityIndex index = addComponentsAdjustmentTypeless(entityId, chs);
            if (index == INVALID)
                return false;

            addComponentsToComponentHandles(index, std::forward<Ts>(components) ...);
            return true;
        }

        template<typename T>
        T *getComponent(EntityId entityId) {

            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return nullptr;

            EcsCoreIntern::ComponentHandle* ch = getSetComponentHandle<T>(this, nullptr, VALUE);

            if (entities[index].getComponentMask()->isSet(ch->getComponentId()))
                return reinterpret_cast<T *>(ch->getComponent(index));

            return nullptr;
        }

        template<typename T>
        bool deleteComponent(EntityId entityId) {
            EcsCoreIntern::ComponentHandle* ch = getSetComponentHandle<T>(this, nullptr, VALUE);
            return deleteComponentTypeless(entityId, ch);
        }

        template<typename ... Ts>
        SetIteratorId createSetIterator() {
            if (sizeof...(Ts) < 1)
                throw std::invalid_argument("SetIterator must subserve at least one Component!");

            std::vector<ComponentId> componentIds = std::vector<ComponentId>(sizeof...(Ts));
            insertComponentIds<Ts...>(&componentIds);

            return createSetIteratorTypeless(componentIds);
        }

        inline EntityId nextEntity(SetIteratorId setIteratorId) {
            EntityIndex nextIndex = setIterators[setIteratorId]->next();
            return entities[nextIndex].id(nextIndex);
        }

        uint32 getEntityAmount(SetIteratorId setIteratorId) {
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }

        template<typename ... Ts>
        uint32 getEntityAmount() {
            static SetIteratorId setIteratorId = createSetIterator<Ts...>();
            while (nextEntity(setIteratorId).version != INVALID);
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }

        inline uint32 getEntityAmount() {
            return lastEntityIndex - freeEntityIndices.size();
        }

        template<typename S_T>
        inline void makeAvailable(S_T* object) {
            getSetSingleton(object);
        }

        template<typename S_T>
        inline S_T* access() {
            return getSetSingleton<S_T>();
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
        std::vector<EcsCoreIntern::EntityState> entities;

        std::vector<EntityIndex> freeEntityIndices;

        ComponentId lastComponentIndex = 0;
        std::unordered_map<Key, EcsCoreIntern::ComponentHandle *> componentMap;
        std::vector<EcsCoreIntern::ComponentHandle *> componentVector;

        std::vector<EcsCoreIntern::EntitySet *> entitySets;
        std::vector<EcsCoreIntern::SetIterator *> setIterators;

        std::vector<EcsCoreIntern::ComponentHandle* (*) (Manager*, const Key*, Storing)> getSetComponentHandleFuncList;

        std::unordered_map<Key, void*> singletons;


        EntityIndex addComponentAdjustmentTypeless(EntityId entityId, EcsCoreIntern::ComponentHandle* componentHandle) {

            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return INVALID;

            ComponentId componentId = componentHandle->getComponentId();
            EcsCoreIntern::ComponentBitset originally = *entities[index].getComponentMask();

            if (!originally.isSet( componentId )) {   // Only update if component type is new for entity
                entities[index].getComponentMask()->set( componentId );
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            } else {
                componentHandle->destroyComponent(entityId, index
#if CORE_ECS_EVENTS==1
                , *this
#endif
                );
            }
            return index;
        }

        EntityIndex addComponentsAdjustmentTypeless(EntityId entityId, std::vector<EcsCoreIntern::ComponentHandle*>& chs) {
            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return INVALID;

            EcsCoreIntern::ComponentBitset originally = *entities[index].getComponentMask();

            bool modified = false;
            for (EcsCoreIntern::ComponentHandle* ch : chs)
                if (originally.isSet(ch->getComponentId()))
                    ch->destroyComponent(entityId, index
#if CORE_ECS_EVENTS==1
            , *this
#endif
                    );
                else
                    modified = true;

            if (modified) {   // Only update if component types are new for entity

                for (EcsCoreIntern::ComponentHandle* ch : chs)
                    entities[index].getComponentMask()->set(ch->getComponentId());

                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            }
            return index;
        }

        bool deleteComponentTypeless(EntityId entityId, EcsCoreIntern::ComponentHandle* ch) {
            EntityIndex index = getIndex(entityId);
            if (index == INVALID)
                return false;

            EcsCoreIntern::ComponentBitset originally = *entities[index].getComponentMask();

            if (originally.isSet(ch->getComponentId())) {
                ch->destroyComponent(entityId, index
#if CORE_ECS_EVENTS==1
                , *this
#endif
                );
                entities[index].getComponentMask()->unset( ch->getComponentId() );
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
                return true;
            }

            return false;
        }

        SetIteratorId createSetIteratorTypeless(std::vector<ComponentId>& componentIds) {
            std::sort(componentIds.begin(), componentIds.end());
            EcsCoreIntern::EntitySet *entitySet = nullptr;

            for (EcsCoreIntern::EntitySet *set : entitySets)
                if (set->concern(&componentIds))
                    entitySet = set;

            if (entitySet == nullptr) {
                entitySet = new EcsCoreIntern::EntitySet(componentIds);
                entitySets.push_back(entitySet);

                // add all related Entities
                for (EntityIndex entityIndex = 1; entityIndex <= lastEntityIndex; entityIndex++)
                    entitySet->dumbAddIfMember(entities[entityIndex].id(entityIndex), entities[entityIndex].getComponentMask());
            }

            setIterators.push_back(new EcsCoreIntern::SetIterator(entitySet));
            return static_cast<SetIteratorId>(setIterators.size() - 1);
        }

        template<typename ... Ts>
        void insertComponentIds(std::vector<ComponentId> *ids) {
            std::vector<EcsCoreIntern::ComponentHandle*> chs = std::vector<EcsCoreIntern::ComponentHandle*>(sizeof...(Ts));
            insertComponentHandles<void, Ts...>(&chs, 0);
            for (size_t i = 0; i < chs.size(); i++) {
                (*ids)[i] = chs[i]->getComponentId();
            }
        }

        template<typename V>
        void insertComponentHandles(std::vector<EcsCoreIntern::ComponentHandle *> *chs, int index) {
        }

        template<typename V, typename T, typename... Ts>
        void insertComponentHandles(std::vector<EcsCoreIntern::ComponentHandle *> *chs, int index) {
            (*chs)[index] = getSetComponentHandle<T>(this, nullptr, VALUE);
            insertComponentHandles<V, Ts...>(chs, ++index);
        }

        template<typename T>
        void addComponentsToComponentHandles(EntityIndex index, T component) {
            void* p = getSetComponentHandle<T>(this, nullptr, VALUE)->createComponent(index);
            new(p) T(std::move(component));
#if CORE_ECS_EVENTS==1
            emitEvent(ComponentAddedEvent<T>(entities[index].id(index)));
#endif
        }

        template<typename T, typename... Ts>
        void addComponentsToComponentHandles(EntityIndex index, T&& component, Ts&&... components) {
            addComponentsToComponentHandles<T>(index, component);
            addComponentsToComponentHandles<Ts...>(index, std::forward<Ts>(components)...);
        }

        template<typename T>
        EcsCoreIntern::ComponentHandle *linkComponentHandle(const Key* componentKey, Storing storing,
                                                            EcsCoreIntern::ComponentHandle* (*getSetComponentHandleFunc) (Manager*, const Key*, Storing)) {
            if (!componentKey) {
                std::ostringstream oss;
                oss << "Tried to access unregistered component: " << typeid(T).name();
                throw std::invalid_argument(oss.str());
            }
            if (componentMap[*componentKey] == nullptr) {
                EcsCoreIntern::ComponentHandle* chp;
                EcsCoreIntern::ComponentInfo componentInfo = EcsCoreIntern::ComponentInfo();

                componentInfo.id = ++lastComponentIndex;
                componentInfo.typeSize = sizeof(T);
#if CORE_ECS_EVENTS==1
                componentInfo.emitDeleteEvent = &emitDeleteEvent<T>;
#endif

                switch (storing) {
                    case POINTER:
                        chp = new EcsCoreIntern::PointingComponentHandle(componentInfo,
                                [](void *p) { delete reinterpret_cast<T *>(p); }   );
                        break;
                    default:
                        chp = new EcsCoreIntern::ValuedComponentHandle(componentInfo,
                                [](void *p) { reinterpret_cast<T *>(p)->~T(); }    );
                        break;
                }
                componentMap[*componentKey] = chp;
                componentVector[chp->getComponentId()] = chp;
                getSetComponentHandleFuncList.emplace_back(getSetComponentHandleFunc);
            }
            EcsCoreIntern::ComponentHandle *ch = componentMap[*componentKey];
            return ch;
        }

        template<typename T>
        static EcsCoreIntern::ComponentHandle* getSetComponentHandle(
                Manager* instance, const Key* componentKey = nullptr, Storing storing = DEFAULT_STORING) {

            static EcsCoreIntern::ComponentHandle* componentHandle =
                    instance->linkComponentHandle<T>(componentKey, storing, &getSetComponentHandle<T>);

            if (componentKey)
                if (*componentKey == ECS_RESET_CODE)
                    componentHandle = nullptr;
                else
                    componentHandle = instance->linkComponentHandle<T>(componentKey, storing, &getSetComponentHandle<T>);

            else if (!componentHandle) {
                std::ostringstream oss;
                oss << "Tried to access unregistered component: " << typeid(T).name();
                throw std::invalid_argument(oss.str());
            }

            return componentHandle;
        }

        void updateAllMemberships(EntityId entityId, EcsCoreIntern::ComponentBitset *previous, EcsCoreIntern::ComponentBitset *recent) {
            for (EcsCoreIntern::EntitySet *set : entitySets) {
                set->updateMembership(entityId.index, previous, recent);
            }
        }

        template<typename T>
        T* getSetSingleton(T* object = nullptr) {
            static T* singleton = object;
            if (object || !singleton) {
                Key className = cleanClassName(typeid(T).name());
                if (object) {
                    singleton = object;
                    singletons[className] = singleton;
                }
                else {
                    if (singletons[className])
                        singleton = reinterpret_cast<T *>(singletons[className]);
                    else {
                        std::ostringstream oss;
                        oss << "Tried to access unregistered Type: " << typeid(T).name() << ". Use makeAvailable.";
                        throw std::invalid_argument(oss.str());
                    }
                }
            }
            return singleton;
        }

#if CORE_ECS_EVENTS==1
        template<typename T>
        static void emitDeleteEvent(SimpleEH::SimpleEventHandler& eventHandler, EntityId entityId) {
            eventHandler.emitEvent(ComponentDeletedEvent<T>(entityId));
        }
#endif

    };

}

#endif //SIMPLE_ECS_CORE_H
