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
#include "Systems.h"

namespace sEcs {


    using sEcs::INVALID;
    using sEcs::NOT_AVAILABLE;

#if USE_ECS_EVENTS==1
    using sEcs::Events::EntityCreatedEvent;
    using sEcs::Events::EntityErasedEvent;
#endif

    enum Storing { POINTER, VALUE };


    static sEcs::EcsManager* ECS_MANAGER_INSTANCE = nullptr;


    namespace TypeWrapper_Intern {

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


        template<ConceptType ID_T, typename T>
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


        template<typename V>
        inline void recursiveCollectComponentIds(sEcs::ComponentId* list, uint32_t pos) {}

        template<typename V, typename T, typename... Ts>
        inline void recursiveCollectComponentIds(sEcs::ComponentId* list, uint32_t pos) {
            list[pos] = getSetId<COMPONENT, T>();
            recursiveCollectComponentIds<void, Ts...>(list, ++pos);
        }

        template<typename... Ts>
        void collectComponentIds(sEcs::ComponentId* list) {
            return recursiveCollectComponentIds<void, Ts...>(list, 0);
        }

    }


#if USE_ECS_EVENTS==1
    template<typename T>
    struct ComponentAddedEvent {
        explicit ComponentAddedEvent(const EntityId& entityId) : entityId(entityId) {}

        EntityId entityId;
    };

    template<typename T>
    struct ComponentDeletedEvent {
        explicit ComponentDeletedEvent(const EntityId& entityId) : entityId(entityId) {}

        EntityId entityId;
    };
#endif


    class Entity {

    public:

        Entity() : entityId() {}

        explicit Entity(sEcs::EntityId entityId)
            : entityId(entityId) {
        }

        template<typename T>
        T* addComponent(T&& component) {
            void* location = ECS_MANAGER_INSTANCE->addComponent(entityId, TypeWrapper_Intern::getSetId<COMPONENT, T>());
            new(location) T(std::forward<T>(component));
            return (T*) location;
        }

        template<typename ... Ts>
        bool addComponents(Ts&&... components) {
            auto ids = std::vector<sEcs::ComponentId>(sizeof...(Ts));
            TypeWrapper_Intern::collectComponentIds<Ts...>(&ids[0]);
            ECS_MANAGER_INSTANCE->activateComponents(entityId, &ids.front(), sizeof...(Ts));
            placeComponents(std::forward<Ts>(components)...);
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
            return ECS_MANAGER_INSTANCE->deleteComponent(entityId, TypeWrapper_Intern::getSetId<COMPONENT, T>());
        }

        template<typename T>
        inline T* getComponent() {
            return reinterpret_cast<T *>(
                    ECS_MANAGER_INSTANCE->getComponent(entityId, TypeWrapper_Intern::getSetId<COMPONENT, T>()));
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
            void* location = ECS_MANAGER_INSTANCE->getComponent(entityId, TypeWrapper_Intern::getSetId<COMPONENT, T>());
            new(location) T(std::forward<T>(component));
            recursivePlaceComponents<Ts...>(std::forward<Ts>(components)...);
        }

        template<typename T>
        inline void recursivePlaceComponents(T&& component) {
            void* location = ECS_MANAGER_INSTANCE->getComponent(entityId, TypeWrapper_Intern::getSetId<COMPONENT, T>());
            new(location) T(std::forward<T>(component));
        }

    };

    inline sEcs::EcsManager* manager() {
        return ECS_MANAGER_INSTANCE;
    }

    void initTypeManaging(sEcs::EcsManager& managerInstance) {
        ECS_MANAGER_INSTANCE = &managerInstance;

#if USE_ECS_EVENTS==1
        manager()->name<ConceptType::EVENT>(
                manager()->entityCreatedEventId(), TypeWrapper_Intern::className<EntityCreatedEvent>());
        manager()->name<ConceptType::EVENT>(
                manager()->entityErasedEventId(), TypeWrapper_Intern::className<EntityErasedEvent>());
#endif
    }

    void updateEcs(double delta) {
        ECS_MANAGER_INSTANCE->update(delta);
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

    void eraseEntity(sEcs::Entity entity) {
        ECS_MANAGER_INSTANCE->eraseEntity(entity.id());
    }


    // This method generates a new list with related entities, if not already existing.
    template<typename ... Ts>
    sEcs::uint32 countEntities() {
        auto ids = std::vector<sEcs::ComponentId>(sizeof...(Ts));
        TypeWrapper_Intern::collectComponentIds<Ts...>(&ids[0]);
        return ECS_MANAGER_INSTANCE->getEntityAmount(ids);
    }

    sEcs::uint32 countEntities() {
        return ECS_MANAGER_INSTANCE->getEntityAmount();
    }


    // Makes a variable available via access<TYPE>()
    template<typename T>
    T* addSingleton(T* singleton) {
        Key key = TypeWrapper_Intern::className<T>();
        ObjectId id = ECS_MANAGER_INSTANCE->addObject(key, singleton);
        TypeWrapper_Intern::getSetId<OBJECT, T>(id);
        return singleton;
    }

    template<typename T>
    T* accessSingleton() {
        return ECS_MANAGER_INSTANCE->getObject(TypeWrapper_Intern::getSetId<OBJECT, T>());
    }


    template <typename T, typename = std::enable_if_t<std::is_base_of<System, T>::value>>
    std::shared_ptr<T> addSystem(std::shared_ptr<T> system) {
        Key key = TypeWrapper_Intern::className<T>();
        SystemId id = ECS_MANAGER_INSTANCE->addSystem(key, system);
        TypeWrapper_Intern::getSetId<SYSTEM, T>(id);
        return system;
    }

    template<typename T>
    T* accessSystem() {
        return ECS_MANAGER_INSTANCE->getObject(TypeWrapper_Intern::getSetId<SYSTEM, T>());
    }


    template<typename T>
    void registerComponent(Storing storing = Storing::VALUE) {
        Key key = TypeWrapper_Intern::className<T>();
        sEcs::ComponentId compId = 0;
        switch (storing) {
            case POINTER:
                compId = manager()->registerComponent(key,
                        new PointingComponentHandle(sizeof(T), [](void *p) { delete reinterpret_cast<T *>(p); }));
                break;
            case VALUE:
                compId = manager()->registerComponent(key,
                        new ValuedComponentHandle(sizeof(T), [](void *p) { reinterpret_cast<T *>(p)->~T(); }));
                break;
        }
        TypeWrapper_Intern::getSetId<COMPONENT, T>(compId);

#if USE_ECS_EVENTS==1
        manager()->name<ConceptType::EVENT>(
                manager()->componentAddedEventId(compId), TypeWrapper_Intern::className<ComponentAddedEvent<T>>());
        manager()->name<ConceptType::EVENT>(
                manager()->componentDeletedEventId(compId), TypeWrapper_Intern::className<ComponentDeletedEvent<T>>());
#endif

    }


    template<typename T>
    class Listener : public Events::Listener {
    public:
        virtual void receive(const T& event) = 0;

        void receive(EventId eventId, const void* event) override {
            receive(*(reinterpret_cast <const T *>(event)));
        }
    };

    template<typename T>
    void emitEvent(const T& event) {
        ECS_MANAGER_INSTANCE->emitEvent(TypeWrapper_Intern::getGenerateEventId<T>(), &event);
    }

    template<typename T>
    void subscribeEvent(Listener<T>* listener) {
        ECS_MANAGER_INSTANCE->subscribeEvent(TypeWrapper_Intern::getGenerateEventId<T>(), listener);
    }

    template<typename T>
    void unsubscribeEvent(Listener<T>* listener) {
        ECS_MANAGER_INSTANCE->unsubscribeEvent(TypeWrapper_Intern::getGenerateEventId<T>(), listener);
    }


    template<typename ... Ts>
    class IteratingSystem : public Systems::IteratingSystem {

    public:
        IteratingSystem() : Systems::IteratingSystem(ECS_MANAGER_INSTANCE) {
            componentIds = std::vector<sEcs::ComponentId>(sizeof...(Ts));
            TypeWrapper_Intern::collectComponentIds<Ts...>(&componentIds[0]);
            setIteratorId = _core->createSetIterator(componentIds);
        }

        virtual void update(Entity entity, DELTA_TYPE delta) = 0;

    };


    template<typename ... Ts>
    class IterateAllSystem : public Systems::IterateAllSystem {

    public:
        IterateAllSystem() : Systems::IterateAllSystem(ECS_MANAGER_INSTANCE) {
            componentIds = std::vector<sEcs::ComponentId>(sizeof...(Ts));
            TypeWrapper_Intern::collectComponentIds<Ts...>(&componentIds[0]);
            setIteratorId = _core->createSetIterator(componentIds);
        }

        virtual void update(Entity entity, DELTA_TYPE delta) = 0;

        void update(EntityId entityId, DELTA_TYPE delta) override {
            update(Entity(entityId), delta);
        }

    };


    template<typename ... Ts>
    class IntervalSystem : public Systems::IntervalSystem {

    public:
        explicit IntervalSystem(sEcs::uint32 intervals = 1)
                : Systems::IntervalSystem(ECS_MANAGER_INSTANCE, intervals) {
            componentIds = std::vector<sEcs::ComponentId>(sizeof...(Ts));
            TypeWrapper_Intern::collectComponentIds<Ts...>(&componentIds[0]);
            setIteratorId = _core->createSetIterator(componentIds);
        }

        virtual void update(Entity entity, DELTA_TYPE delta) = 0;

        void update(EntityId entityId, DELTA_TYPE delta) override {
            update(Entity(entityId), delta);
        }

    };

}


#endif //SIMPLE_ECS_ACCESS_H
