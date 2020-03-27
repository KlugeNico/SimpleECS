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

namespace RtEcs {

    typedef float DELTA_TYPE;

    using EcsCore::EntityId;
    using EcsCore::EntityIndex;
    using EcsCore::Key;

    using EcsCore::INVALID;
    using EcsCore::NOT_AVAILABLE;
    using EcsCore::Storing;

#if CORE_ECS_EVENTS==1
    using EcsCore::ComponentAddedEvent;
    using EcsCore::ComponentDeletedEvent;
    using EcsCore::EntityCreatedEvent;
    using EcsCore::EntityErasedEvent;
#endif

    typedef SimpleEH::Key Event_Key;

    template<typename T>
    using Listener = SimpleEH::Listener<T>;

    class RtManager;

    class Entity {

    public:

        Entity() : manager(nullptr), entityId() {}

        Entity(EcsCore::Manager *manager, EcsCore::EntityId entityId)
            : manager(manager), entityId(entityId) {
        }

        template<typename T>
        T* addComponent(T&& component) {
            return manager->addComponent<T>(entityId, std::forward<T>(component));
        }

        template<typename ... Ts>
        bool addComponents(Ts&&... components) {
            return manager->addComponents<Ts...>(entityId, std::forward<Ts>(components)...);
        }

        inline bool erase() {
            return manager->eraseEntity(entityId);
        }

        inline bool isValid() {
            return manager->isValid(entityId);
        }

        inline EcsCore::EntityIndex index() {
            return manager->getIndex(entityId);
        }

        template<typename T>
        inline bool deleteComponent() {
            return manager->deleteComponent<T>(entityId);
        }

        template<typename T>
        inline T* getComponent() const {
            return manager->getComponent<T>(entityId);
        }

        inline EcsCore::EntityId id() const {
            return entityId;
        }

    private:
        EcsCore::EntityId entityId;
        EcsCore::Manager* manager;

    };


    class Handler {

    public:
        virtual void init(EcsCore::Manager* pManager, RtManager* pRtManager) {
            manager_ = pManager;
            rtManager_ = pRtManager;
        }

        Entity getEntity(EcsCore::EntityId entityId) {
            return {manager_, entityId};
        }

        Entity getEntityByIndex(EcsCore::EntityIndex entityIndex) {
            return {manager_, manager_->getIdFromIndex(entityIndex)};
        }

        Entity createEntity() {
            return {manager_, manager_->createEntity()};
        }

        template<typename T>
        T* getComponent(EcsCore::EntityId entityId) {
            return manager_->getComponent<T>(entityId);
        }

        // This method generates a new list with related entities, if not already existing.
        template<typename ... Ts>
        EcsCore::uint32 countEntities() {
            return manager_->getEntityAmount<Ts...>();
        }

        EcsCore::uint32 countEntities() {
            return manager_->getEntityAmount();
        }

        template<typename T>
        void emitEvent(const T& event) {
            manager_->emitEvent(event);
        }

        template<typename T>
        void subscribeEvent(SimpleEH::Listener<T>* listener) {
            manager_->subscribeEvent<T>(listener);
        }

        template<typename T>
        void unsubscribeEvent(SimpleEH::Listener<T>* listener) {
            manager_->unsubscribeEvent<T>(listener);
        }

        // Makes a variable available via access<TYPE>()
        template<typename T>
        void makeAvailable(T* object) {
            manager_->makeAvailable(object);
        }

        template<typename T>
        T* access() {
            return manager_->access<T>();
        }

        inline EcsCore::Manager* manager() {
            return manager_;
        }

        inline RtManager* rtManager() {
            return rtManager_;
        }

    private:
        EcsCore::Manager* manager_ = nullptr;
        RtManager* rtManager_ = nullptr;

    };


    class System : public Handler {

    public:
        virtual void update(DELTA_TYPE delta) = 0;

    };


    class RtManager : public System {

    public:
        RtManager(const RtManager&) = delete;

        explicit RtManager() : managerObject() {
            Handler::init (&managerObject, this);
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of<System, T>::value>>
        T* addSystem(T* system) {
            system->init(manager(), this);
            systems.emplace_back(system);
            makeAvailable(system);
            return system;
        }

        template<typename T>
        T* addSingleton(T* singleton) {
            singletons.emplace_back(singleton);
            makeAvailable(singleton);
            return singleton;
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of<Handler, T>::value>>
        T* addHandler(T* handler) {
            handler->init(manager(), rtManager());
            return addSingleton(handler);
        }

        template<typename T>
        void registerComponent(EcsCore::Key key, EcsCore::Storing storing = EcsCore::DEFAULT_STORING) {
            manager()->registerComponent<T>(key, storing);
        }

        void update(DELTA_TYPE delta) override {
            for (const auto& system : systems) {
                system->update(delta);
            }
        }

    private:
        std::vector<std::shared_ptr<System>> systems;
        std::vector<std::shared_ptr<void>> singletons;
        EcsCore::Manager managerObject;

    };


    template<typename ... Ts>
    class IteratingSystem : public System {
        friend RtManager;

    protected:
        EcsCore::SetIteratorId SetIteratorId = 0;

        void init(EcsCore::Manager *pManager, RtManager *pRtManager) override {
            Handler::init(pManager, pRtManager);
            SetIteratorId = pManager->createSetIterator<Ts...>();
        }

    };


    template<typename ... Ts>
    class IterateAllSystem : public IteratingSystem<Ts...> {

    public:

        virtual void start(DELTA_TYPE delta){};
        virtual void update(Entity entity, DELTA_TYPE delta) = 0;
        virtual void end(DELTA_TYPE delta){};

        void update(DELTA_TYPE delta) override {
            start(delta);
            EcsCore::EntityId entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
            while (entityId.index != EcsCore::INVALID) {
                update(Entity(Handler::manager(), entityId), delta);
                entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
            }
            end(delta);
        }

    };


    template<typename ... Ts>
    class IntervalSystem : public IteratingSystem<Ts...> {

    public:
        virtual void start(DELTA_TYPE delta){};
        virtual void update(Entity entity, DELTA_TYPE delta) = 0;
        virtual void end(DELTA_TYPE delta){};

        explicit IntervalSystem(EcsCore::uint32 intervals = 1)
            : intervals(intervals), overallDelta(0), leftIntervals(intervals) {
                if (intervals < 1)
                    throw std::invalid_argument("Minimum 1 Interval!");
        }

        void update(DELTA_TYPE delta) override {

            if (leftIntervals == intervals)
                start(delta);

            EcsCore::EntityId entityId;
            if (leftIntervals == 1) {
                entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);

                while (entityId.index != EcsCore::INVALID) {
                    update(Entity(Handler::manager(), entityId), overallDelta);
                    entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
                }
            }
            else {
                EcsCore::uint32 amount = (Handler::manager()->getEntityAmount(IteratingSystem<Ts...>::SetIteratorId) - treated) / leftIntervals;

                for (EcsCore::uint32 i = 0; i < amount; i++) {
                    entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
                    if (entityId.index == EcsCore::INVALID)
                        break;
                    update(Entity(Handler::manager(), entityId), overallDelta);
                }

                treated += amount;
            }

            deltaSum += delta;

            if (leftIntervals <= 1) {
                treated = 0;
                leftIntervals = intervals;
                overallDelta = deltaSum;
                deltaSum = 0;
                end(delta);
            }
            else
                leftIntervals--;

        }

    private:
        EcsCore::uint32 intervals;
        EcsCore::uint32 leftIntervals;
        EcsCore::uint32 treated = 0;
        DELTA_TYPE deltaSum = 0;
        double overallDelta;

    };

}


#endif //SIMPLE_ECS_ACCESS_H
