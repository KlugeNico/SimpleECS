/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#include "../Systems.h"


namespace sEcs {

    namespace Systems {

        IteratingSystem::IteratingSystem(Core* core) : _core(core) {}

        IteratingSystem::IteratingSystem(Core* core, std::vector<ComponentId> componentIds)
                : _core(core), componentIds(std::move(componentIds)) {
            setIteratorId = _core->createSetIterator(componentIds);
        }


        IterateAllSystem::IterateAllSystem(Core* core) : IteratingSystem(core) {}

        IterateAllSystem::IterateAllSystem(Core* core, std::vector<ComponentId> componentIds)
                    : IteratingSystem(core, std::move(componentIds)) {}

        void IterateAllSystem::update(DELTA_TYPE delta) {
            start(delta);
            sEcs::EntityId entityId = _core->nextEntity(setIteratorId);
            while (entityId.index != sEcs::INVALID) {
                update(entityId, delta);
                entityId = _core->nextEntity(setIteratorId);
            }
            end(delta);
        }


        IntervalSystem::IntervalSystem(Core* core, sEcs::uint32 intervals)
                : IteratingSystem(core),
                  intervals(intervals), overallDelta(0), leftIntervals(intervals) {
            if (intervals < 1)
                throw std::invalid_argument("Minimum 1 Interval!");
        }

        IntervalSystem::IntervalSystem(Core* core, std::vector<ComponentId> componentIds, sEcs::uint32 intervals)
                : IteratingSystem(core, std::move(componentIds)),
                  intervals(intervals), overallDelta(0), leftIntervals(intervals) {
            if (intervals < 1)
                throw std::invalid_argument("Minimum 1 Interval!");
        }

        void IntervalSystem::update(DELTA_TYPE delta) {

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

    }

}