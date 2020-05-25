/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#include "../EcsManager.h"

namespace sEcs {

    EcsManager::EcsManager() {
        systems.emplace_back(nullptr);
        objects.emplace_back(nullptr);
        pointers.emplace_back(nullptr);
    }


    EventId EcsManager::generateEvent(const std::string& eventName) {
        if (getIdByName<ConceptType::EVENT>(eventName) != 0)
            throw std::invalid_argument ("Event already existing: " + eventName);

        EventId eventId = generateEvent();
        conceptRegisters[ConceptType::EVENT].set(eventName, eventId);

        return eventId;
    }


    ComponentId EcsManager::registerComponent(const std::string& componentName, ComponentHandle* ch) {
        if (getIdByName<ConceptType::COMPONENT>(componentName) != 0)
            throw std::invalid_argument ("Component already existing: " + componentName);

        ComponentId componentId = registerComponent(ch);
        conceptRegisters[ConceptType::COMPONENT].set(componentName, componentId);

        return componentId;
    }


    SystemId EcsManager::addSystem(const std::string& systemName, const std::shared_ptr<System>& system) {
        if (getIdByName<ConceptType::SYSTEM>(systemName) != 0)
            throw std::invalid_argument ("System already existing: " + systemName);

        systems.emplace_back(system);
        conceptRegisters[ConceptType::SYSTEM].set(systemName, systems.size() - 1);
        return systems.size() - 1;
    }


    SystemId EcsManager::addObject(const std::string& objectName, const std::shared_ptr<void>& object) {
        if (getIdByName<ConceptType::OBJECT>(objectName) != 0)
            throw std::invalid_argument ("Object already existing: " + objectName);

        objects.emplace_back(object);
        conceptRegisters[ConceptType::OBJECT].set(objectName, objects.size() - 1);
        return objects.size() - 1;
    }


    PointerId EcsManager::addPointer(const std::string& pointerName, void* pointer) {
        if (getIdByName<ConceptType::POINTER>(pointerName) != 0)
            throw std::invalid_argument ("Pointer already existing: " + pointerName);

        pointers.emplace_back(pointer);
        conceptRegisters[ConceptType::POINTER].set(pointerName, pointers.size() - 1);
        return pointers.size() - 1;
    }


    void EcsManager::update(DELTA_TYPE delta) {
        for (uint32 i = 1; i < systems.size(); i++) {
            systems[i]->update(delta);
        }
    }

}