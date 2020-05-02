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

#include "Core.h"


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

        EcsManager() {
            systems.emplace_back(nullptr);
            objects.emplace_back(nullptr);
        }


        template<ConceptType::Type C_T>
        Id getIdByName (const Key& name) {
            return conceptRegisters[C_T].getId(name);
        }


        template<ConceptType::Type C_T>
        Id name (Id id, const Key& name) {
            if (getIdByName<C_T>(name) != 0)
                throw std::invalid_argument ("Name already used: " + name);

            conceptRegisters[ConceptType::EVENT].set(name, id);

            return id;
        }


        EventId generateEvent(const std::string& eventName) {
            if (getIdByName<ConceptType::EVENT>(eventName) != 0)
                throw std::invalid_argument ("Event already existing: " + eventName);

            EventId eventId = generateEvent();
            conceptRegisters[ConceptType::EVENT].set(eventName, eventId);

            return eventId;
        }


        ComponentId registerComponent(const std::string& componentName, ComponentHandle* ch) {
            if (getIdByName<ConceptType::COMPONENT>(componentName) != 0)
                throw std::invalid_argument ("Component already existing: " + componentName);

            ComponentId componentId = registerComponent(ch);
            conceptRegisters[ConceptType::COMPONENT].set(componentName, componentId);

            return componentId;
        }


        SystemId addSystem(const std::string& systemName, const std::shared_ptr<System>& system) {
            if (getIdByName<ConceptType::SYSTEM>(systemName) != 0)
                throw std::invalid_argument ("System already existing: " + systemName);

            systems.emplace_back(system);
            conceptRegisters[ConceptType::SYSTEM].set(systemName, systems.size() - 1);
            return systems.size() - 1;
        }

        inline std::shared_ptr<void> getSystem(SystemId systemId) {
            return systems[systemId];
        }


        SystemId addObject(const std::string& objectName, const std::shared_ptr<void>& object) {
            if (getIdByName<ConceptType::OBJECT>(objectName) != 0)
                throw std::invalid_argument ("Object already existing: " + objectName);

            objects.emplace_back(object);
            conceptRegisters[ConceptType::OBJECT].set(objectName, objects.size() - 1);
            return objects.size() - 1;
        }

        inline std::shared_ptr<void> getObject(ObjectId objectId) {
            return objects[objectId];
        }


        PointerId addPointer(const std::string& pointerName, void* pointer) {
            if (getIdByName<ConceptType::POINTER>(pointerName) != 0)
                throw std::invalid_argument ("Pointer already existing: " + pointerName);

            pointers.emplace_back(pointer);
            conceptRegisters[ConceptType::POINTER].set(pointerName, pointers.size() - 1);
            return pointers.size() - 1;
        }

        inline void* getPointer(PointerId pointerId) {
            return pointers[pointerId];
        }


        void update(DELTA_TYPE delta) {
            for (uint32 i = 1; i < systems.size(); i++) {
                systems[i]->update(delta);
            }
        }


    private:
        std::vector<std::shared_ptr<System>> systems;
        std::vector<std::shared_ptr<void>> objects;
        std::vector<void*> pointers;
        sEcs::Register conceptRegisters[static_cast<int>(ConceptType::SIZE_T)];

    };


}




#endif //SIMPLEECS_ECSMANAGER_H
