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

#include <unordered_map>
#include <vector>
#include <forward_list>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <typeinfo>

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
    typedef std::string Component_Key;

    static const uint32 INVALID = 0;
    static const std::nullptr_t NOT_AVAILABLE = nullptr;

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
                bitset[bit / BITSET_TYPE_SIZE] |= BITSET_TYPE(1) << (bit % BITSET_TYPE_SIZE);
            }

            inline void unset(uint32 bit) {
                bitset[bit / BITSET_TYPE_SIZE] &= ~(1u << (bit % BITSET_TYPE_SIZE));
            }

            inline void reset() {
                for (BITSET_TYPE &bitrow : bitset)
                    bitrow = 0;
            }

            inline bool isSet(uint32 bit) {
                return ((bitset[bit / BITSET_TYPE_SIZE] >> (bit % BITSET_TYPE_SIZE)) & BITSET_TYPE(1)) == 1;
            }

            inline bool contains(Bitset *other) {
                for (size_t i = 0; i < arraySize; ++i)
                    if ((other->bitset[i] & ~bitset[i]) > 0)
                        return false;
                return true;
            }

        private:
            size_t arraySize;
            BITSET_TYPE bitset[(size / BITSET_TYPE_SIZE) + 1]{};

        };


        template<size_t C_N>
        class Entity {

        public:
            explicit Entity() : version_(INVALID), componentMask(Bitset<C_N>()) {}

            explicit Entity(Entity_Version version) : version_(version) {}

            Entity_Version version() const { return version_; }

            Entity_Id id(Entity_Index index) const { return (((Entity_Id) version_) * POW_2_32 | index); }

            Bitset<C_N> *getComponentMask() { return &componentMask; }

            void reset() {
                version_++;
                componentMask.reset();
            }

        private:
            Entity_Version version_;
            Bitset<C_N> componentMask;

        };


        template<size_t C_N>
        class EntitySet {

        public:
            EntitySet(std::vector<Component_Id> componentIds, uint32 maxEntityAmount) :
                    componentIds(componentIds),
                    mask(Bitset<C_N>()) {
                std::sort(componentIds.begin(), componentIds.end());
                mask.set(&componentIds);
                entities.push_back(INVALID);
                entities.reserve(maxEntityAmount);
                internIndices.reserve(maxEntityAmount);
                freeInternIndices.reserve(maxEntityAmount);
            }

            inline void dumbAddIfMember(Entity_Id entityId, Bitset<C_N> *bitset) {
                if (bitset->contains(&mask))
                    add(entityId);
            }

            void updateMembership(Entity_Index entityIndex, Bitset<C_N> *previous, Bitset<C_N> *recent) {
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
            Bitset<C_N> mask;
            std::vector<Component_Id> componentIds;

            std::vector<Entity_Index> entities;

            std::vector<Intern_Index> internIndices;  // We need this List to avoid double insertions
            std::vector<Intern_Index> freeInternIndices;

        };


        template<size_t C_N>
        class SetIterator {

        public:
            explicit SetIterator(EntitySet<C_N> *entitySet) :
                    entitySet(entitySet) {}

            inline Entity_Index next() {
                iterator = entitySet->next(iterator);
                return entitySet->getIndex(iterator);
            }

            inline EntitySet<C_N>* getEntitySet() {
                return entitySet;
            }

        private:
            EntitySet<C_N> *entitySet;
            Intern_Index iterator = 0;

        };


        class ComponentHandle {

        public:
            virtual ~ComponentHandle() = default;

            explicit ComponentHandle(Component_Id id) :
                    id(id) {}

            Component_Id getComponentId() {
                return id;
            }

            // only defined behavior for valid requests (entity and component exists)
            virtual void* getComponent(Entity_Index entityIndex) = 0;

            // only defined behavior for valid requests (entity exists and component not)
            virtual void* createComponent(Entity_Index entityIndex) = 0;

            // only defined behavior for valid requests (entity and component exists)
            virtual void destroyComponent(Entity_Index entityIndex) = 0;

            Bitset<7> listeners;

        private:
            Component_Id id;

        };


        class PointingComponentHandle : public ComponentHandle {

        public:
            explicit PointingComponentHandle(Component_Id id, size_t typeSize, void(*deleteFunc)(void *), uint32 maxEntities) :
                    ComponentHandle(id),
                    typeSize(typeSize),
                    deleteFunc(deleteFunc),
                    components(std::vector<void *>(maxEntities + 1)) {}

            ~PointingComponentHandle() override {
                for (size_t i = 0; i < components.size(); i++) destroyComponent(i);
            }

            void* getComponent(Entity_Index entityIndex) override {
                return components[entityIndex];
            }

            void* createComponent(Entity_Index entityIndex) override {
                return components[entityIndex] = malloc(typeSize);
            }

            void destroyComponent(Entity_Index entityIndex) override {
                deleteFunc(getComponent(entityIndex));
                components[entityIndex] = nullptr;
            }

        private:
            void (*deleteFunc)(void *);
            size_t typeSize;
            std::vector<void *> components;

        };


        class ValuedComponentHandle : public ComponentHandle {

        public:
            explicit ValuedComponentHandle(Component_Id id, size_t typeSize, void(*destroy)(void *), uint32 maxEntities) :
                    ComponentHandle(id),
                    typeSize(typeSize),
                    destroy(destroy) {
                data = std::vector<char>((maxEntities + 1) * typeSize);
            }

            void* getComponent(Entity_Index entityIndex) override {
                return &data[entityIndex * typeSize];
            }

            void* createComponent(Entity_Index entityIndex) override {
                return &data[entityIndex * typeSize];
            }

            void destroyComponent(Entity_Index entityIndex) override {
                destroy(getComponent(entityIndex));
            }

        private:
            void (*destroy)(void *);
            size_t typeSize;
            std::vector<char> data;

        };


    }      // end private


    template<size_t C_N>
    class Manager {

    public:
        explicit Manager(uint32 maxEntities) :
                maxEntities(maxEntities),
                componentVector(std::vector<EcsCoreIntern::ComponentHandle *>(C_N + 1)),
                entities(std::vector<EcsCoreIntern::Entity<C_N>>(maxEntities + 1))  {
            entities[0] = EcsCoreIntern::Entity<C_N>();
        }

        ~Manager() {
            for (EcsCoreIntern::ComponentHandle *ch : componentVector) delete ch;
            for (EcsCoreIntern::EntitySet<C_N> *set : entitySets) delete set;
            for (EcsCoreIntern::SetIterator<C_N> *setIterator : setIterators) delete setIterator;
        }

        uint32 getMaxEntities() {
            return maxEntities;
        }

        uint32 getMaxComponents() {
            return C_N;
        }

        Entity_Id createEntity() {

            Entity_Index index;

            if (!freeEntityIndices.empty()) {
                index = freeEntityIndices.back();
                freeEntityIndices.pop_back();
            } else {
                index = ++lastEntityIndex;
                entities[index] = EcsCoreIntern::Entity<C_N>(Entity_Version(1));
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

            EcsCoreIntern::Bitset<C_N> originally = *entities[index].getComponentMask();

            for (Component_Id i = 1; i <= lastComponentIndex; i++) {
                EcsCoreIntern::ComponentHandle *ch = componentVector[i];
                if (originally.isSet(ch->getComponentId()))   // Only delete existing components
                    ch->destroyComponent(index);
            }

            entities[index].reset();
            updateAllMemberships(entityId, &originally, entities[index].getComponentMask());

            freeEntityIndices.push_back(index);

            return true;
        }

        template<typename T>
        void registerComponent(Component_Key componentKey, Storing storing = DEFAULT_STORING) {
            getSetComponentHandle<T>(componentKey, storing);
        }

        template <typename T>
        T* addComponent(Entity_Id entityId, T&& component) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return nullptr;

            EcsCoreIntern::ComponentHandle* ch = getComponentHandle<T>();
            EcsCoreIntern::Bitset<C_N> originally = *entities[index].getComponentMask();

            if (!originally.isSet(ch->getComponentId())) {   // Only update if component type is new for entity
                entities[index].getComponentMask()->set(getComponentId<T>());
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            } else {
                ch->destroyComponent(index);
            }

            addComponentsToComponentHandles<T>(index, component);

            return getComponent<T>(entityId);
        }

        template<typename ... Ts>
        bool addComponents(Entity_Id entityId, Ts&&... components) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return false;

            std::vector<EcsCoreIntern::ComponentHandle*> chs = std::vector<EcsCoreIntern::ComponentHandle*>(sizeof...(Ts));
            insertComponentHandles<void, Ts...>(&chs, 0);

            EcsCoreIntern::Bitset<C_N> originally = *entities[index].getComponentMask();

            bool modified = false;
            for (EcsCoreIntern::ComponentHandle* ch : chs)
                if (originally.isSet(ch->getComponentId()))
                    ch->destroyComponent(index);
                else
                    modified = true;

            if (modified) {   // Only update if component types are new for entity

                for (EcsCoreIntern::ComponentHandle* ch : chs)
                    entities[index].getComponentMask()->set(ch->getComponentId());

                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            }

            addComponentsToComponentHandles(index, std::forward<Ts>(components) ...);

            return true;
        }

        template<typename T>
        T *getComponent(Entity_Id entityId) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return nullptr;

            EcsCoreIntern::ComponentHandle* ch = getComponentHandle<T>();

            if (entities[index].getComponentMask()->isSet(ch->getComponentId()))
                return reinterpret_cast<T *>(ch->getComponent(index));

            return nullptr;
        }

        template<typename T>
        bool deleteComponent(Entity_Id entityId) {

            Entity_Index index = getIndex(entityId);
            if (index == INVALID)
                return false;

            EcsCoreIntern::ComponentHandle* ch = getComponentHandle<T>();
            EcsCoreIntern::Bitset<C_N> originally = *entities[index].getComponentMask();

            if (originally.isSet(ch->getComponentId())) {
                ch->destroyComponent(index);
                entities[index].getComponentMask()->unset(getComponentHandle<T>()->getComponentId());
                updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
                return true;
            }

            return false;

        }

        template<typename ... Ts>
        SetIterator_Id createSetIterator() {

            if (sizeof...(Ts) < 1)
                throw std::invalid_argument("SetIterator must subserve at least one Component!");

            std::vector<Component_Id> componentIds = std::vector<Component_Id>(sizeof...(Ts));
            insertComponentIds<Ts...>(&componentIds);
            std::sort(componentIds.begin(), componentIds.end());
            EcsCoreIntern::EntitySet<C_N> *entitySet = nullptr;

            for (EcsCoreIntern::EntitySet<C_N> *set : entitySets)
                if (set->concern(&componentIds))
                    entitySet = set;

            if (entitySet == nullptr) {
                entitySet = new EcsCoreIntern::EntitySet<C_N>(componentIds, maxEntities);
                entitySets.push_back(entitySet);

                // add all related Entities
                for (Entity_Index entityIndex = 1; entityIndex <= lastEntityIndex; entityIndex++)
                    entitySet->dumbAddIfMember(entities[entityIndex].id(entityIndex), entities[entityIndex].getComponentMask());
            }

            setIterators.push_back(new EcsCoreIntern::SetIterator<C_N>(entitySet));
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
        uint32 getEntityAmount() {
            static SetIterator_Id setIteratorId = createSetIterator<Ts...>();
            while (nextEntity(setIteratorId) != INVALID);
            return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
        }

        uint32 getEntityAmount() {
            return lastEntityIndex - freeEntityIndices.size();
        }

        template<typename T>
        void makeAvailable(T* object) {
            getSetSingleton(object);
        }

        template<typename T>
        T* access() {
            return getSetSingleton<T>();
        }


    private:
        uint32 maxEntities;
        Entity_Index lastEntityIndex = 0;
        std::vector<EcsCoreIntern::Entity<C_N>> entities;

        std::vector<Entity_Index> freeEntityIndices;

        Component_Id lastComponentIndex = 0;
        std::unordered_map<Component_Key, EcsCoreIntern::ComponentHandle *> componentMap;
        std::vector<EcsCoreIntern::ComponentHandle *> componentVector;

        std::vector<EcsCoreIntern::EntitySet<C_N> *> entitySets;
        std::vector<EcsCoreIntern::SetIterator<C_N> *> setIterators;

        template<typename T>
        Component_Id getComponentId() {
            static Component_Id id = getComponentHandle<T>()->getComponentId();
            return id;
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
            (*chs)[index] = getComponentHandle<T>();
            insertComponentHandles<V, Ts...>(chs, ++index);
        }

        template<typename T>
        void addComponentsToComponentHandles(Entity_Index index, T component) {
            void* p = getComponentHandle<T>()->createComponent(index);
            new(p) T(std::move(component));
        }

        template<typename T, typename... Ts>
        void addComponentsToComponentHandles(Entity_Index index, T&& component, Ts&&... components) {
            addComponentsToComponentHandles<T>(index, component);
            addComponentsToComponentHandles<Ts...>(index, std::forward<Ts>(components)...);
        }

        template<typename T>
        EcsCoreIntern::ComponentHandle *getComponentHandle() {
            static EcsCoreIntern::ComponentHandle *componentHandle = getSetComponentHandle<T>();
            return componentHandle;
        }

        template<typename T>
        EcsCoreIntern::ComponentHandle *linkComponentHandle(Component_Key *componentKey, Storing storing) {
            if (componentKey->empty()) {
                std::ostringstream oss;
                oss << "Tried to use unregistered component: " << typeid(T).name();
                throw std::invalid_argument(oss.str());
            }
            if (componentMap[*componentKey] == nullptr) {
                EcsCoreIntern::ComponentHandle* chp;
                switch (storing) {
                    case POINTER:
                        chp = new EcsCoreIntern::PointingComponentHandle(
                                ++lastComponentIndex,
                                sizeof(T),
                                [](void *x) { delete reinterpret_cast<T *>(x); },
                                maxEntities);
                        break;
                    default:
                        chp = new EcsCoreIntern::ValuedComponentHandle(
                                ++lastComponentIndex,
                                sizeof(T),
                                [](void *x) { reinterpret_cast<T *>(x)->~T(); },
                                maxEntities);
                        break;
                }
                componentMap[*componentKey] = chp;
                componentVector[chp->getComponentId()] = chp;
            }
            EcsCoreIntern::ComponentHandle *ch = componentMap[*componentKey];
            return ch;
        }

        template<typename T>
        EcsCoreIntern::ComponentHandle *getSetComponentHandle(Component_Key componentKey = Component_Key(), Storing storing = DEFAULT_STORING) {
            static EcsCoreIntern::ComponentHandle *componentHandle = linkComponentHandle<T>(&componentKey, storing);
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

        void updateAllMemberships(Entity_Id entityId, EcsCoreIntern::Bitset<C_N> *previous, EcsCoreIntern::Bitset<C_N> *recent) {
            for (EcsCoreIntern::EntitySet<C_N> *set : entitySets) {
                set->updateMembership(entityId, previous, recent);
            }
        }

        template<typename T>
        T* getSetSingleton(T* object = nullptr) {
            static T* singleton = object;
            return singleton;
        }

    };


}

#endif //SIMPLE_ECS_CORE_H
