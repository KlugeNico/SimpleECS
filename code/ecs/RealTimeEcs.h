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

#include "Core.h"
#include "EventHandler.h"

namespace RtEcs {

    typedef float DELTA_TYPE;

    using EcsCore::Entity_Id;
    using EcsCore::Component_Key;

    using EcsCore::INVALID;
    using EcsCore::NOT_AVAILABLE;
    using EcsCore::Storing;

    using EcsCore::ComponentAddedEvent;
    using EcsCore::ComponentDeletedEvent;
    using EcsCore::EntityCreatedEvent;
    using EcsCore::EntityErasedEvent;

    typedef SimpleEH::Event_Key Event_Key;

    template<typename T>
    using Listener = SimpleEH::Listener<T>;

    class RtManager;

    class Entity {

    public:

        Entity(EcsCore::Manager *manager, EcsCore::Entity_Id entityId)
            : manager(manager), entityId(entityId) {
        }

        template<typename T>
        T* addComponent(T&& component) {
            return manager->addComponent<T>(entityId, std::forward<T>(component));
        }

        template<typename ... Ts>
        bool addComponents(Ts&&... components) {
            manager->addComponents<Ts...>(entityId, std::forward<Ts>(components)...);
        }

        bool erase() {
            return manager->eraseEntity(entityId);
        }

        bool isValid() {
            return manager->isValid(entityId);
        }

        template<typename T>
        bool deleteComponent() {
            return manager->deleteComponent<T>(entityId);
        }

        template<typename T>
        T* getComponent() const {
            return manager->getComponent<T>(entityId);
        }

        EcsCore::Entity_Id getId() const {
            return entityId;
        }

    private:
        EcsCore::Entity_Id entityId;
        EcsCore::Manager* const manager;

    };


    class System {

    public:
        virtual ~System() = default;

        virtual void update(DELTA_TYPE delta) = 0;

        virtual void init(EcsCore::Manager* pManager, RtManager* pRtManager) {
            manager_ = pManager;
            rtManager_ = pRtManager;
        }

        Entity getEntity(EcsCore::Entity_Id entityId) {
            return {manager_, entityId};
        }

        Entity createEntity() {
            return {manager_, manager_->createEntity()};
        }

        template<typename T>
        T* getComponent(EcsCore::Entity_Id entityId) {
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
        void emitEvent(T& event) {
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

        template<typename T>
        void registerEvent(Event_Key eventKey) {
            manager_->registerEvent<T>(eventKey);
        }

        // Makes a variable available via access<TYPE>()
        template<typename T>
        void makeAvailable(T* object) {
            manager_->makeAvailable(object);
        }

        template<typename T>
        T* access() {
            manager_->access<T>();
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


    class RtManager : public System {

    public:

        explicit RtManager() : managerObject() {
            init (&managerObject, this);
        }

        ~RtManager() override {
            for (System* system : systems)
                delete system;
        }

        void addSystem(System* system) {
            system->init(manager(), this);
            systems.push_back(system);
        }

        template<typename T>
        void registerComponent(EcsCore::Component_Key key, EcsCore::Storing storing = EcsCore::DEFAULT_STORING) {
            manager()->registerComponent<T>(key, storing);
        }

        void update(DELTA_TYPE delta) override {
            for (System* system : systems) {
                system->update(delta);
            }
        }

    private:
        std::vector<System*> systems;
        EcsCore::Manager managerObject;

    };


    template<typename ... Ts>
    class IteratingSystem : public System {

    protected:
        EcsCore::SetIterator_Id setIteratorId = 0;

        void init(EcsCore::Manager *pManager, RtManager *pRtManager) override {
            System::init(pManager, pRtManager);
            setIteratorId = pManager->createSetIterator<Ts...>();
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
            EcsCore::Entity_Id entityId = System::manager()->nextEntity(IteratingSystem<Ts...>::setIteratorId);
            while (entityId != EcsCore::INVALID) {
                update(Entity(System::manager(), entityId), delta);
                entityId = System::manager()->nextEntity(IteratingSystem<Ts...>::setIteratorId);
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

            EcsCore::Entity_Id entityId;
            if (leftIntervals == 1) {
                entityId = System::manager()->nextEntity(IteratingSystem<Ts...>::setIteratorId);

                while (entityId != EcsCore::INVALID) {
                    update(Entity(System::manager(), entityId), overallDelta);
                    entityId = System::manager()->nextEntity(IteratingSystem<Ts...>::setIteratorId);
                }
            }
            else {
                EcsCore::uint32 amount = (System::manager()->getEntityAmount(IteratingSystem<Ts...>::setIteratorId) - treated) / leftIntervals;

                for (EcsCore::uint32 i = 0; i < amount; i++) {
                    entityId = System::manager()->nextEntity(IteratingSystem<Ts...>::setIteratorId);
                    if (entityId == EcsCore::INVALID)
                        break;
                    update(Entity(System::manager(), entityId), overallDelta);
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
