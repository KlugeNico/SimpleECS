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

namespace RtEcs {

    typedef float DELTA_TYPE;
    DELTA_TYPE INIT_DELTA = 0.1;

    class Entity {

    public:

        Entity(EcsCore::Manager *manager, EcsCore::Entity_Id entityId)
            : manager(manager), entityId(entityId) {
        }

        template<typename T>
        bool addComponent(T* component) {
            manager->addComponent(entityId, component);
        }

        template<typename ... Ts>
        bool addComponents(Ts*... components) {
            manager->addComponents(entityId, components...);
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
        T* getComponent() {
            return manager->getComponent<T>(entityId);
        }

        EcsCore::Entity_Id getId() {
            return entityId;
        }

    private:
        EcsCore::Entity_Id entityId;
        EcsCore::Manager* manager;

    };


    class System {

    public:
        virtual ~System() = default;

        virtual void update(DELTA_TYPE delta) = 0;

        virtual void init(EcsCore::Manager* pManager) {
            manager = pManager;
        }

        Entity getEntity(EcsCore::Entity_Id entityId) {
            return {manager, entityId};
        }

        Entity createEntity() {
            return {manager, manager->createEntity()};
        }

        template<typename T>
        T* getComponent(EcsCore::Entity_Id entityId) {
            return manager->getComponent<T>(entityId);
        }

        // This method generates a new list with related entities, if not already existing.
        template<typename ... Ts>
        EcsCore::uint32 countEntities(Ts*... ts) {
            return manager->getEntityAmount<Ts...>(ts...);
        }

        EcsCore::uint32 countEntities() {
            return manager->getEntityAmount();
        }

    protected:
        EcsCore::Manager* manager = nullptr;

    };


    class RtEcs {

    public:

        ~RtEcs() {
            for (System* system : systems)
                delete system;
        }

        RtEcs(EcsCore::uint32 maxEntities, EcsCore::uint32 maxComponents) :
                manager(maxEntities, maxComponents) {
        }

        void addSystem(System* system) {
            system->init(&manager);
            systems.push_back(system);
        }

        void update(DELTA_TYPE delta) {
            for (System* system : systems) {
                system->update(delta);
            }
        }

        EcsCore::Manager* getEcsCoreManager() {
            return &manager;
        }

        Entity createEntity() {
            return {&manager, manager.createEntity()};
        }

        template<typename T>
        void registerComponent(EcsCore::Component_Key key) {
            manager.registerComponent<T>(key);
        }

    private:
        EcsCore::Manager manager;
        std::vector<System*> systems;

    };


    template<typename ... Ts>
    class IteratingSystem : public System {

    protected:
        EcsCore::SetIterator_Id setIteratorId = 0;

        void init(EcsCore::Manager *pManager) override {
            System::init(pManager);
            setIteratorId = manager->createSetIterator(new Ts...);
        }

    };


    template<typename ... Ts>
    class IterateAllSystem : public IteratingSystem<Ts...> {

    public:

        virtual void update(Entity entity, double delta) = 0;

        void update(DELTA_TYPE delta) override {
            EcsCore::Entity_Id entityId = System::manager->nextEntity(IteratingSystem<Ts...>::setIteratorId);
            while (entityId != EcsCore::INVALID) {
                update(Entity(System::manager, entityId), delta);
                entityId = System::manager->nextEntity(IteratingSystem<Ts...>::setIteratorId);
            }
        }

    };


    template<typename ... Ts>
    class IntervalSystem : public IteratingSystem<Ts...> {

    public:
        virtual void start(DELTA_TYPE delta){};
        virtual void update(Entity entity, DELTA_TYPE delta) = 0;
        virtual void end(DELTA_TYPE delta){};

        explicit IntervalSystem(EcsCore::uint32 intervals, DELTA_TYPE initDelta = INIT_DELTA)
            : intervals(intervals), overallDelta(initDelta) {

                leftIntervals = intervals;
                if (intervals < 1)
                    throw std::invalid_argument("Minimum 1 Interval!");
        }

        void update(DELTA_TYPE delta) override {

            if (leftIntervals == intervals)
                start(delta);

            EcsCore::Entity_Id entityId;
            if (leftIntervals == 1) {
                entityId = System::manager->nextEntity(IteratingSystem<Ts...>::setIteratorId);

                while (entityId != EcsCore::INVALID) {
                    update(Entity(System::manager, entityId), overallDelta);
                    entityId = System::manager->nextEntity(IteratingSystem<Ts...>::setIteratorId);
                }
            }
            else {
                EcsCore::uint32 amount = (System::manager->getEntityAmount(IteratingSystem<Ts...>::setIteratorId) - treated) / leftIntervals;

                for (EcsCore::uint32 i = 0; i < amount; i++) {
                    entityId = System::manager->nextEntity(IteratingSystem<Ts...>::setIteratorId);
                    if (entityId == EcsCore::INVALID)
                        break;
                    update(Entity(System::manager, entityId), overallDelta);
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
