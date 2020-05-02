/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#ifndef SIMPLEECS_SYSTEMS_H
#define SIMPLEECS_SYSTEMS_H


#include <utility>

#include "EcsManager.h"


namespace sEcs {

    namespace Systems {

        class IteratingSystem : public System {

        public:
            explicit IteratingSystem(Core* core) : _core(core) {}

            explicit IteratingSystem(Core* core, std::vector<ComponentId> componentIds)
                    : _core(core), componentIds(std::move(componentIds)) {
                setIteratorId = _core->createSetIterator(componentIds);
            }

        protected:
            sEcs::SetIteratorId setIteratorId = 0;
            Core* _core = nullptr;
            std::vector<ComponentId> componentIds;

        };


        class IterateAllSystem : public IteratingSystem {

        public:
            explicit IterateAllSystem(Core* core) : IteratingSystem(core) {}

            explicit IterateAllSystem(Core* core, std::vector<ComponentId> componentIds)
                    : IteratingSystem(core, std::move(componentIds)) {}

            virtual void start(DELTA_TYPE delta) {};
            virtual void update(EntityId entityId, DELTA_TYPE delta) = 0;
            virtual void end(DELTA_TYPE delta) {};

            void update(DELTA_TYPE delta) override {
                start(delta);
                sEcs::EntityId entityId = _core->nextEntity(setIteratorId);
                while (entityId.index != sEcs::INVALID) {
                    update(entityId, delta);
                    entityId = _core->nextEntity(setIteratorId);
                }
                end(delta);
            }

        };


        class IntervalSystem : public IteratingSystem {

        public:
            virtual void start(DELTA_TYPE delta) {};
            virtual void update(EntityId entityId, DELTA_TYPE delta) = 0;
            virtual void end(DELTA_TYPE delta) {};

            explicit IntervalSystem(Core* core, sEcs::uint32 intervals = 1)
                    : IteratingSystem(core),
                      intervals(intervals), overallDelta(0), leftIntervals(intervals) {
                if (intervals < 1)
                    throw std::invalid_argument("Minimum 1 Interval!");
            }

            explicit IntervalSystem(Core* core, std::vector<ComponentId> componentIds, sEcs::uint32 intervals = 1)
                    : IteratingSystem(core, std::move(componentIds)),
                      intervals(intervals), overallDelta(0), leftIntervals(intervals) {
                if (intervals < 1)
                    throw std::invalid_argument("Minimum 1 Interval!");
            }

            void update(DELTA_TYPE delta) override {

                if (leftIntervals == intervals)
                    start(delta);

                sEcs::EntityId entityId;
                if (leftIntervals == 1) {
                    entityId = _core->nextEntity(setIteratorId);

                    while (entityId.index != sEcs::INVALID) {
                        update(entityId, overallDelta);
                        entityId = _core->nextEntity(setIteratorId);
                    }
                } else {
                    sEcs::uint32 amount =
                            (_core->getEntityAmount(setIteratorId) - treated) /
                            leftIntervals;

                    for (sEcs::uint32 i = 0; i < amount; i++) {
                        entityId = _core->nextEntity(setIteratorId);
                        if (entityId.index == sEcs::INVALID)
                            break;
                        update(entityId, overallDelta);
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

}


#endif //SIMPLEECS_SYSTEMS_H
