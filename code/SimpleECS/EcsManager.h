//
// Created by kreischenderdepp on 04.04.20.
//

#ifndef SIMPLEECS_ECSMANAGER_H
#define SIMPLEECS_ECSMANAGER_H

#include "Core.h"


namespace sEcs {

    typedef float DELTA_TYPE;
    typedef Id ObjectId;
    typedef Id SystemId;


    enum ConceptTypes {
        SYSTEM,
        COMPONENT,
        OBJECT,
        EVENT,
        SIZE_T
    };



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


        template<ConceptTypes C_T>
        ComponentId getIdByName (const Key& name) {
            return conceptRegisters[C_T].getId(name);
        }


        Events::EventId generateEvent(const std::string& eventName) {
            if (getIdByName<ConceptTypes::EVENT>(eventName) != 0)
                throw std::invalid_argument ("Event already existing: " + eventName);

            Events::EventId eventId = generateEvent();
            conceptRegisters[ConceptTypes::EVENT].set(eventName, eventId);

            return eventId;
        }


        ComponentId registerComponent(const std::string& componentName, ComponentHandle* ch) {
            if (getIdByName<ConceptTypes::COMPONENT>(componentName) != 0)
                throw std::invalid_argument ("Component already existing: " + componentName);

            ComponentId componentId = registerComponent(ch);
            conceptRegisters[ConceptTypes::COMPONENT].set(componentName, componentId);

            return componentId;
        }


        SystemId addSystem(const std::string& systemName, const std::shared_ptr<System>& system) {
            if (getIdByName<ConceptTypes::SYSTEM>(systemName) != 0)
                throw std::invalid_argument ("System already existing: " + systemName);

            systems.emplace_back(system);
            conceptRegisters[ConceptTypes::SYSTEM].set(systemName, systems.size() - 1);
            return systems.size() - 1;
        }

        inline std::shared_ptr<void> getSystem(SystemId systemId) {
            return systems[systemId];
        }


        SystemId addObject(const std::string& objectName, const std::shared_ptr<void>& object) {
            if (getIdByName<ConceptTypes::OBJECT>(objectName) != 0)
                throw std::invalid_argument ("Object already existing: " + objectName);

            objects.emplace_back(object);
            conceptRegisters[ConceptTypes::OBJECT].set(objectName, objects.size() - 1);
            return objects.size() - 1;
        }

        inline std::shared_ptr<void> getObject(ObjectId objectId) {
            return objects[objectId];
        }


        void update(DELTA_TYPE delta) {
            for (uint32 i = 1; i < systems.size(); i++) {
                systems[i]->update(delta);
            }
        }


    private:
        std::vector<std::shared_ptr<System>> systems;
        std::vector<std::shared_ptr<void>> objects;
        sEcs::Register conceptRegisters[ConceptTypes::SIZE_T];

    };


}




#endif //SIMPLEECS_ECSMANAGER_H
