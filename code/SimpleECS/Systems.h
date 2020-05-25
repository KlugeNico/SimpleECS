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
            explicit IteratingSystem(Core* core);
            explicit IteratingSystem(Core* core, std::vector<ComponentId> componentIds);

        protected:
            sEcs::SetIteratorId setIteratorId = 0;
            Core* _core = nullptr;
            std::vector<ComponentId> componentIds;

        };


        class IterateAllSystem : public IteratingSystem {

        public:
            explicit IterateAllSystem(Core* core);
            explicit IterateAllSystem(Core* core, std::vector<ComponentId> componentIds);

            virtual void start(DELTA_TYPE delta) {};
            virtual void update(EntityId entityId, DELTA_TYPE delta) = 0;
            virtual void end(DELTA_TYPE delta) {};

            void update(DELTA_TYPE delta) override;

        };


        class IntervalSystem : public IteratingSystem {

        public:
            virtual void start(DELTA_TYPE delta) {};
            virtual void update(EntityId entityId, DELTA_TYPE delta) = 0;
            virtual void end(DELTA_TYPE delta) {};

            explicit IntervalSystem(Core* core, sEcs::uint32 intervals = 1);

            explicit IntervalSystem(Core* core, std::vector<ComponentId> componentIds, sEcs::uint32 intervals = 1);

            void update(DELTA_TYPE delta) override;

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
