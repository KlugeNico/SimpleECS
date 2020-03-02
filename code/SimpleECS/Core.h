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
    typedef uint64 Entity_Id;
    typedef uint32 Entity_Version;
    typedef uint32 Entity_Index;
    typedef uint32 Component_Id;
    typedef uint32 SetIterator_Id;
    typedef uint32 Intern_Index;
    typedef std::string Key;

    static const uint32 INVALID = 0;
    static const std::nullptr_t NOT_AVAILABLE = nullptr;

#if CORE_ECS_EVENTS==1
    template <typename T>
    struct ComponentAddedEvent {
        explicit ComponentAddedEvent (const Entity_Id& entityId) : entityId(entityId) {}
        Entity_Id entityId;
    };

    template <typename T>
    struct ComponentDeletedEvent {
        explicit ComponentDeletedEvent (const Entity_Id& entityId) : entityId(entityId) {}
        Entity_Id entityId;
    };

    struct EntityCreatedEvent {
        explicit EntityCreatedEvent (const Entity_Id& entityId) : entityId(entityId) {}
        Entity_Id entityId;
    };

    struct EntityErasedEvent {
        explicit EntityErasedEvent (const Entity_Id& entityId) : entityId(entityId) {}
        Entity_Id entityId;
    };
#endif

    namespace EcsCoreIntern {     // private

        template<size_t size>
        struct Bitset {

        public:
            Bitset() {
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

            inline bool contains(Bitset *other) {
                for (size_t i = 0; i < arraySize; ++i)
                    if ((other->bitset[i] & ~bitset[i]) > 0u)
                        return false;
                return true;
            }

        private:
            size_t arraySize;
            BITSET_TYPE bitset[(size / BITSET_TYPE_SIZE) + 1]{};

        };

        typedef Bitset<MAX_COMPONENT_AMOUNT> ComponentBitset;

        class Entity {

        public:
            explicit Entity() : version_(INVALID), componentMask(ComponentBitset()) {}

            explicit Entity(Entity_Version version) : version_(version) {}

            Entity_Version version() const { return version_; }

            Entity_Id id(Entity_Index index) const { return (((Entity_Id) version_) * POW_2_32 | index); }

            ComponentBitset *getComponentMask() { return &componentMask; }

            void reset() {
                version_++;
                componentMask.reset();
            }

        private:
            Entity_Version version_;
            ComponentBitset componentMask;

        };


        class EntitySet {

        public:
            explicit EntitySet(std::vector<Component_Id> componentIds) :
                    componentIds(componentIds),
                    mask(ComponentBitset()) {
                std::sort(componentIds.begin(), componentIds.end());
                mask.set(&componentIds);
                entities.push_back(INVALID);
                entities.reserve(MAX_ENTITY_AMOUNT);
                internIndices.reserve(MAX_ENTITY_AMOUNT);
                freeInternIndices.reserve(MAX_ENTITY_AMOUNT);
            }

            inline void dumbAddIfMember(Entity_Id entityId, ComponentBitset *bitset) {
                if (bitset->contains(&mask))
                    add(entityId);
            }

            void updateMembership(Entity_Index entityIndex, ComponentBitset *previous, ComponentBitset *recent) {
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

            inline EntitySet* getEntitySet() {
                return entitySet;
            }

        private:
            EntitySet *entitySet;
            Intern_Index iterator = 0;

        };


        struct ComponentInfo {
            Component_Id id;
            size_t typeSize;
#if CORE_ECS_EVENTS==1
            void (*emitDeleteEvent)(SimpleEH::SimpleEventHandler&, Entity_Id);
#endif
        };


        class ComponentHandle {

        public:
            virtual ~ComponentHandle() = default;

            explicit ComponentHandle(const ComponentInfo& componentInfo) :
                    componentInfo(componentInfo) {}

            Component_Id getComponentId() {
                return componentInfo.id;
            }

            void destroyComponent(Entity_Id entityId, Entity_Index entityIndex
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
            virtual void* getComponent(Entity_Index entityIndex) = 0;

            // only defined behavior for valid requests (entity exists and component not)
            virtual void* createComponent(Entity_Index entityIndex) = 0;

        protected:
            ComponentInfo componentInfo;

        private:
            // only defined behavior for valid requests (entity and component exists)
            virtual void destroyComponentIntern(Entity_Index entityIndex) = 0;

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

            void* getComponent(Entity_Index entityIndex) override {
                return components[entityIndex];
            }

            void* createComponent(Entity_Index entityIndex) override {
                return components[entityIndex] = malloc(componentInfo.typeSize);
            }

            void destroyComponentIntern(Entity_Index entityIndex) override {
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

            void* getComponent(Entity_Index entityIndex) override {
                return &data[entityIndex * componentInfo.typeSize];
            }

            void* createComponent(Entity_Index entityIndex) override {
                return &data[entityIndex * componentInfo.typeSize];
            }

            void destroyComponentIntern(Entity_Index entityIndex) override {
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
                entities(std::vector<EcsCoreIntern::Entity>(MAX_ENTITY_AMOUNT + 1)) {
            entities[0] = EcsCoreIntern::Entity();
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

        Entity_Id createEntity() {

            Entity_Index index;

            if (!freeEntityIndices.empty()) {
                index = freeEntityIndices.back();
                freeEntityIndices.pop_back();
            } else {
                index = ++lastEntityIndex;
                entities[index] = EcsCoreIntern::Entity(Entity_Version(1));
            }

            Entity_Id entityId = entities[index].id(index);

#if CORE_ECS_EVENTS==1
            emitEvent(EntityCreatedEvent(entityId));
#endif

            return entityId;
        }

        bool isValid(Entity_Id entityId) {
            return getIndex(entityId) != INVALID;
        }

        bool eraseEntity(Entity_Id entityId) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return false;

#if CORE_ECS_EVENTS==1
            emitEvent(EntityErasedEvent(entityId));
#endif

            EcsCoreIntern::ComponentBitset originally = *entities[index].getComponentMask();

            for (Component_Id i = 1; i <= lastComponentIndex; i++) {
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
        T* addComponent(Entity_Id entityId, T&& component) {

            EcsCoreIntern::ComponentHandle* ch = getSetComponentHandle<T>(this, nullptr, VALUE);

            Entity_Index index = addComponentAdjustmentTypeless(entityId, ch);
            if (index == INVALID)
                return nullptr;

            addComponentsToComponentHandles<T>(index, component);

            return getComponent<T>(entityId);
        }

        template<typename ... Ts>
        bool addComponents(Entity_Id entityId, Ts&&... components) {

            std::vector<EcsCoreIntern::ComponentHandle*> chs = std::vector<EcsCoreIntern::ComponentHandle*>(sizeof...(Ts));
            insertComponentHandles<void, Ts...>(&chs, 0);

            Entity_Index index = addComponentsAdjustmentTypeless(entityId, chs);
            if (index == INVALID)
                return false;

            addComponentsToComponentHandles(index, std::forward<Ts>(components) ...);
            return true;
        }

        template<typename T>
        T *getComponent(Entity_Id entityId) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return nullptr;

            EcsCoreIntern::ComponentHandle* ch = getSetComponentHandle<T>(this, nullptr, VALUE);

            if (entities[index].getComponentMask()->isSet(ch->getComponentId()))
                return reinterpret_cast<T *>(ch->getComponent(index));

            return nullptr;
        }

        template<typename T>
        bool deleteComponent(Entity_Id entityId) {
            EcsCoreIntern::ComponentHandle* ch = getSetComponentHandle<T>(this, nullptr, VALUE);
            return deleteComponentTypeless(entityId, ch);
        }

        template<typename ... Ts>
        SetIterator_Id createSetIterator() {
            if (sizeof...(Ts) < 1)
                throw std::invalid_argument("SetIterator must subserve at least one Component!");

            std::vector<Component_Id> componentIds = std::vector<Component_Id>(sizeof...(Ts));
            insertComponentIds<Ts...>(&componentIds);

            return createSetIteratorTypeless(componentIds);
        }

        inline Entity_Id nextEntity(SetIterator_Id setIteratorId) {
            Entity_Index nextIndex = setIterators[setIteratorId]->next();
            return entities[nextIndex].id(nextIndex);
        }

        uint32 getEntityAmount(SetIterator_Id setIteratorId) {
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }

        template<typename ... Ts>
        uint32 getEntityAmount() {
            static SetIterator_Id setIteratorId = createSetIterator<Ts...>();
            while (nextEntity(setIteratorId) != INVALID);
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

        inline Entity_Index getIndex(Entity_Id entityId) {
            auto index = static_cast<Entity_Index>(entityId);
            if (index > lastEntityIndex)
                return INVALID;
            if (static_cast<Entity_Version>(entityId >> Entity_Id(32)) != entities[index].version())
                return INVALID;
            return index;
        }

        Entity_Id getIdFromIndex(Entity_Index index) {
            if (index > lastEntityIndex)
                return INVALID;
            return (((Entity_Id) entities[index].version()) * POW_2_32 | index);
        }

    private:
        Entity_Index lastEntityIndex = 0;
        std::vector<EcsCoreIntern::Entity> entities;

        std::vector<Entity_Index> freeEntityIndices;

        Component_Id lastComponentIndex = 0;
        std::unordered_map<Key, EcsCoreIntern::ComponentHandle *> componentMap;
        std::vector<EcsCoreIntern::ComponentHandle *> componentVector;

        std::vector<EcsCoreIntern::EntitySet *> entitySets;
        std::vector<EcsCoreIntern::SetIterator *> setIterators;

        std::vector<EcsCoreIntern::ComponentHandle* (*) (Manager*, const Key*, Storing)> getSetComponentHandleFuncList;

        std::unordered_map<Key, void*> singletons;


        Entity_Index addComponentAdjustmentTypeless(Entity_Id entityId, EcsCoreIntern::ComponentHandle* componentHandle) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return INVALID;

            Component_Id componentId = componentHandle->getComponentId();
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

        Entity_Index addComponentsAdjustmentTypeless(Entity_Id entityId, std::vector<EcsCoreIntern::ComponentHandle*>& chs) {
            Entity_Index index = getIndex(entityId);
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

        bool deleteComponentTypeless(Entity_Id entityId, EcsCoreIntern::ComponentHandle* ch) {
            Entity_Index index = getIndex(entityId);
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

        SetIterator_Id createSetIteratorTypeless(std::vector<Component_Id>& componentIds) {
            std::sort(componentIds.begin(), componentIds.end());
            EcsCoreIntern::EntitySet *entitySet = nullptr;

            for (EcsCoreIntern::EntitySet *set : entitySets)
                if (set->concern(&componentIds))
                    entitySet = set;

            if (entitySet == nullptr) {
                entitySet = new EcsCoreIntern::EntitySet(componentIds);
                entitySets.push_back(entitySet);

                // add all related Entities
                for (Entity_Index entityIndex = 1; entityIndex <= lastEntityIndex; entityIndex++)
                    entitySet->dumbAddIfMember(entities[entityIndex].id(entityIndex), entities[entityIndex].getComponentMask());
            }

            setIterators.push_back(new EcsCoreIntern::SetIterator(entitySet));
            return static_cast<SetIterator_Id>(setIterators.size() - 1);
        }

        template<typename ... Ts>
        void insertComponentIds(std::vector<Component_Id> *ids) {
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
        void addComponentsToComponentHandles(Entity_Index index, T component) {
            void* p = getSetComponentHandle<T>(this, nullptr, VALUE)->createComponent(index);
            new(p) T(std::move(component));
#if CORE_ECS_EVENTS==1
            emitEvent(ComponentAddedEvent<T>(entities[index].id(index)));
#endif
        }

        template<typename T, typename... Ts>
        void addComponentsToComponentHandles(Entity_Index index, T&& component, Ts&&... components) {
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

        void updateAllMemberships(Entity_Id entityId, EcsCoreIntern::ComponentBitset *previous, EcsCoreIntern::ComponentBitset *recent) {
            for (EcsCoreIntern::EntitySet *set : entitySets) {
                set->updateMembership(entityId, previous, recent);
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
        static void emitDeleteEvent(SimpleEH::SimpleEventHandler& eventHandler, Entity_Id entityId) {
            eventHandler.emitEvent(ComponentDeletedEvent<T>(entityId));
        }
#endif

    };

}

#endif //SIMPLE_ECS_CORE_H
