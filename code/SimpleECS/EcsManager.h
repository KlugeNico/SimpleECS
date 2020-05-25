/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#ifndef SIMPLEECS_ECSMANAGER_H
#define SIMPLEECS_ECSMANAGER_H

#include <memory>
#include "Core.h"
#include "Register.h"


namespace sEcs {

    typedef float DELTA_TYPE;
    typedef Id ObjectId;
    typedef Id SystemId;
    typedef Id PointerId;


    namespace ConceptType {
        enum Type {
            SYSTEM,
            COMPONENT,
            OBJECT,
            POINTER,
            EVENT,
            SIZE_T
        };
    }

    class System {

    public:
        virtual void update(DELTA_TYPE delta) = 0;

    };


    class EcsManager : public sEcs::Core {

    private:
        using sEcs::Core::registerComponent;
        using sEcs::Core::generateEvent;

    public:
        EcsManager(const EcsManager&) = delete;

        EcsManager();


        template<ConceptType::Type C_T>
        Id getIdByName (const Key& name) {
            return conceptRegisters[C_T].getId(name);
        }


        template<ConceptType::Type C_T>
        Id name (Id id, const Key& name) {
            if (getIdByName<C_T>(name) != 0)
                throw std::invalid_argument ("Name already used: " + name);
            else
                conceptRegisters[ConceptType::EVENT].set(name, id);

            return id;
        }


        EventId generateEvent(const std::string& eventName);


        ComponentId registerComponent(const std::string& componentName, ComponentHandle* ch);


        SystemId addSystem(const std::string& systemName, const std::shared_ptr<System>& system);

        inline std::shared_ptr<void> getSystem(SystemId systemId) {
            return systems[systemId];
        }


        SystemId addObject(const std::string& objectName, const std::shared_ptr<void>& object);

        inline std::shared_ptr<void> getObject(ObjectId objectId) {
            return objects[objectId];
        }


        PointerId addPointer(const std::string& pointerName, void* pointer);

        inline void* getPointer(PointerId pointerId) {
            return pointers[pointerId];
        }


        void update(DELTA_TYPE delta);


    private:
        std::vector<std::shared_ptr<System>> systems;
        std::vector<std::shared_ptr<void>> objects;
        std::vector<void*> pointers;
        sEcs::Register conceptRegisters[static_cast<int>(ConceptType::SIZE_T)];

    };


}




#endif //SIMPLEECS_ECSMANAGER_H
