//
// Created by kreischenderdepp on 05.04.20.
//

#ifndef SIMPLEECS_SYSTEMS_H
#define SIMPLEECS_SYSTEMS_H


#include "EcsManager.h"


namespace sEcs {


    template<typename ... Ts>
    class IteratingSystem : public System {

        IteratingSystem(EcsManager* ecsManager) : manager(ecsManager) {}

    protected:
        sEcs::SetIteratorId SetIteratorId = 0;

        void init(sEcs::Core* pManager, RtManager* pRtManager) override {
            Handler::init(pManager, pRtManager);
            SetIteratorId = pManager->createSetIterator();
        }

    private:
        EcsManager* manager;

    };


    template<typename ... Ts>
    class IterateAllSystem : public IteratingSystem<Ts...> {

    public:

        virtual void start(DELTA_TYPE delta) {};

        virtual void update(Entity entity, DELTA_TYPE delta) = 0;

        virtual void end(DELTA_TYPE delta) {};

        void update(DELTA_TYPE delta) override {
            start(delta);
            sEcs::EntityId entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
            while (entityId.index != sEcs::INVALID) {
                update(Entity(Handler::manager(), entityId), delta);
                entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
            }
            end(delta);
        }

    };


    template<typename ... Ts>
    class IntervalSystem : public IteratingSystem<Ts...> {

    public:
        virtual void start(DELTA_TYPE delta) {};

        virtual void update(Entity entity, DELTA_TYPE delta) = 0;

        virtual void end(DELTA_TYPE delta) {};

        explicit IntervalSystem(sEcs::uint32 intervals = 1)
                : intervals(intervals), overallDelta(0), leftIntervals(intervals) {
            if (intervals < 1)
                throw std::invalid_argument("Minimum 1 Interval!");
        }

        void update(DELTA_TYPE delta) override {

            if (leftIntervals == intervals)
                start(delta);

            sEcs::EntityId entityId;
            if (leftIntervals == 1) {
                entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);

                while (entityId.index != sEcs::INVALID) {
                    update(Entity(Handler::manager(), entityId), overallDelta);
                    entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
                }
            } else {
                sEcs::uint32 amount =
                        (Handler::manager()->getEntityAmount(IteratingSystem<Ts...>::SetIteratorId) - treated) /
                        leftIntervals;

                for (sEcs::uint32 i = 0; i < amount; i++) {
                    entityId = Handler::manager()->nextEntity(IteratingSystem<Ts...>::SetIteratorId);
                    if (entityId.index == sEcs::INVALID)
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
            } else
                leftIntervals--;

        }

    private:
        sEcs::uint32 intervals;
        sEcs::uint32 leftIntervals;
        sEcs::uint32 treated = 0;
        DELTA_TYPE deltaSum = 0;
        double overallDelta;

    };


}


#endif //SIMPLEECS_SYSTEMS_H
