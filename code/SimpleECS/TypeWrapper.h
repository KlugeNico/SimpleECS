/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */

#ifndef SIMPLE_ECS_ACCESS_H
#define SIMPLE_ECS_ACCESS_H

#include <type_traits>
#include <memory>

#include "Core.h"
#include "EventHandler.h"
#include "ComponentHandler.h"
#include "Register.h"
#include "EcsManager.h"

namespace sEcs {

    using sEcs::EntityId;
    using sEcs::EntityIndex;
    using sEcs::Key;

    using sEcs::INVALID;
    using sEcs::NOT_AVAILABLE;

#if CORE_ECS_EVENTS==1
    using sEcs::ComponentAddedEvent;
    using sEcs::ComponentDeletedEvent;
    using sEcs::EntityCreatedEvent;
    using sEcs::EntityErasedEvent;
#endif

    using Listener = sEcs::Listener;

    enum Storing { POINTER, VALUE };


    static sEcs::EcsManager* ECS_MANAGER_INSTANCE = nullptr;


    static std::string cleanClassName(const char* name) {
        char* strP = abi::__cxa_demangle(name,nullptr,nullptr,nullptr);
        std::string string(strP);
        free(strP);
        return std::move(string);
    }

    template<typename T>
    inline static std::string className() {
        return cleanClassName(typeid(T).name());
    }


    template<typename ID_T, typename T>
    sEcs::Id getSetId(sEcs::Id id = 0) {
        static sEcs::Id _id = id;

        if (id != 0)
            _id = id;

        if (_id == 0) {
            Key key = className<T>();
            _id = ECS_MANAGER_INSTANCE->getIdByName<ID_T>(key);

            if (_id == 0) {
                std::ostringstream oss;
                oss << "Tried to access unregistered Type: " << key;
                throw std::invalid_argument(oss.str());
            }
        }

        return _id;
    }


    template<typename T>
    sEcs::Id getGenerateEventId() {
        static sEcs::Id _id = ECS_MANAGER_INSTANCE->getIdByName<EVENT>(className<T>());

        if (_id == 0) {
            Key key = className<T>();
            _id = ECS_MANAGER_INSTANCE->generateEvent(key);
        }

        return _id;
    }


    template<typename T>
    inline void recursiveCollectComponentIds(sEcs::ComponentId* list, uint32_t pos) {
        list[pos] = getSetId<COMPONENT, T>();
    }

    template<typename T, typename... Ts>
    inline void recursiveCollectComponentIds(sEcs::ComponentId* list, uint32_t pos) {
        list[pos] = getSetId<COMPONENT, T>();
        recursiveCollectComponentIds<Ts...>(list, ++pos);
    }

    template<typename... Ts>
    void collectComponentIds(sEcs::ComponentId* list) {
        return recursiveCollectComponentIds<Ts...>(list, 0);
    }

    class Entity : EntityId {

    public:

        Entity() : entityId() {}

        explicit Entity(sEcs::EntityId entityId)
            : entityId(entityId) {
        }

        template<typename T>
        T* addComponent(T&& component) {
            void* location = ECS_MANAGER_INSTANCE->addComponent(entityId, getSetId<COMPONENT, T>());
            new(location) T(std::forward(component));
            return (T*) location;
        }

        template<typename ... Ts>
        bool addComponents(Ts&&... components) {
            auto ids = std::vector<sEcs::ComponentId>(sizeof...(Ts));
            collectComponentIds(&ids[0]);
            ECS_MANAGER_INSTANCE->activateComponents(entityId, &ids.front(), sizeof...(Ts));
            placeComponents(components...);
            return true;
        }

        inline bool erase() {
            return ECS_MANAGER_INSTANCE->eraseEntity(entityId);
        }

        inline bool isValid() {
            return ECS_MANAGER_INSTANCE->isValid(entityId);
        }

        inline sEcs::EntityIndex index() {
            return ECS_MANAGER_INSTANCE->getIndex(entityId);
        }

        template<typename T>
        inline bool deleteComponent() {
            return ECS_MANAGER_INSTANCE->deleteComponent(entityId, getSetId<COMPONENT, T>());
        }

        template<typename T>
        inline T* getComponent() {
            return ECS_MANAGER_INSTANCE->getComponent(entityId, getSetId<COMPONENT, T>());
        }

        inline sEcs::EntityId id() {
            return entityId;
        }

    private:
        sEcs::EntityId entityId;

        template<typename... Ts>
        void placeComponents(Ts&&... components) {
            return recursivePlaceComponents(std::forward<Ts>(components)...);
        }

        template<typename T, typename... Ts>
        inline void recursivePlaceComponents(T&& component, Ts&&... components) {
            new(ECS_MANAGER_INSTANCE->getComponent(entityId, getSetId<COMPONENT, T>())) T(std::forward(component));
            recursivePlaceComponents<Ts...>(std::forward<Ts>(components)...);
        }

        template<typename T>
        inline void recursivePlaceComponents(T&& component) {
            new(ECS_MANAGER_INSTANCE->getComponent(entityId, getSetId<COMPONENT, T>())) T(std::forward(component));
        }

    };


    inline sEcs::EcsManager* manager() {
        return ECS_MANAGER_INSTANCE;
    }


    Entity getEntity(sEcs::EntityId entityId) {
        return Entity( entityId );
    }

    Entity getEntityByIndex(sEcs::EntityIndex entityIndex) {
        return Entity(ECS_MANAGER_INSTANCE->getIdFromIndex(entityIndex) );
    }

    Entity createEntity() {
        return { Entity(ECS_MANAGER_INSTANCE->createEntity() ) };
    }

    // This method generates a new list with related entities, if not already existing.
    template<typename ... Ts>
    sEcs::uint32 countEntities() {
        auto ids = std::vector<sEcs::ComponentId>(sizeof...(Ts));
        collectComponentIds<Ts...>(&ids[0]);
        return ECS_MANAGER_INSTANCE->getEntityAmount(ids);
    }

    sEcs::uint32 countEntities() {
        return ECS_MANAGER_INSTANCE->getEntityAmount();
    }


    template<typename T>
    void emitEvent(const T& event) {
        ECS_MANAGER_INSTANCE->emitEvent(event);
    }

    template<typename T>
    void subscribeEvent(sEcs::Listener* listener) {
        ECS_MANAGER_INSTANCE->subscribeEvent(getGenerateEventId<T>(), listener);
    }

    template<typename T>
    void unsubscribeEvent(sEcs::Listener* listener) {
        ECS_MANAGER_INSTANCE->unsubscribeEvent(getGenerateEventId<T>(), listener);
    }


    // Makes a variable available via access<TYPE>()
    template<typename T>
    T* addSingleton(T* singleton) {
        Key key = className<T>();
        ObjectId id = ECS_MANAGER_INSTANCE->addObject(key, singleton);
        getSetId<OBJECT, T>(id);
        return singleton;
    }

    template<typename T>
    T* accessSingleton() {
        return ECS_MANAGER_INSTANCE->getObject(getSetId<OBJECT, T>());
    }


    template <typename T, typename = std::enable_if_t<std::is_base_of<System, T>::value>>
    T* addSystem(T* system) {
        Key key = className<T>();
        ObjectId id = ECS_MANAGER_INSTANCE->addSystem(key, system);
        getSetId<SYSTEM, T>(id);
        return system;
    }

    template<typename T>
    T* accessSystem() {
        return ECS_MANAGER_INSTANCE->getObject(getSetId<SYSTEM, T>());
    }


    template<typename T>
    void registerComponent(Storing storing = Storing::VALUE) {
        Key key = className<T>();
        sEcs::ComponentId id = 0;
        switch (storing) {
            case POINTER:
                id = manager()->registerComponent(key, new PointingComponentHandle( sizeof(T),
                    [](void *p) { delete reinterpret_cast<T *>(p); }));
                break;
            case VALUE:
                id = manager()->registerComponent(key, new ValuedComponentHandle( sizeof(T),
                    [](void *p) { reinterpret_cast<T *>(p)->~T(); }));
                break;
        }
        getSetId<T>(id);
    }

}


#endif //SIMPLE_ECS_ACCESS_H
