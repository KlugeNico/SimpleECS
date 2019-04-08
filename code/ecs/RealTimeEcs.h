//
// Created by Nico Kluge on 01.04.19.
//

#ifndef SIMPLE_ECS_ACCESS_H
#define SIMPLE_ECS_ACCESS_H

#include "Core.h"

namespace RealTimeEcs {

    using namespace EcsCore;

    class System {

    public:
        virtual void run();

        Entity getEntity;

    private:


    };

}


#endif //SIMPLE_ECS_ACCESS_H
