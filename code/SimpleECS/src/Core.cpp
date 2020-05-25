/*
 * Copyright (C) 2019 Nico Kluge <klugenico@mailbox.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 *
 * Author: Nico Kluge <klugenico@mailbox.org>
 */


#include <algorithm>
#include <stdexcept>
#include "../Core.h"

namespace sEcs {

    namespace Core_Intern {     // private

        EntityState::EntityState() : version(INVALID), componentMask(ComponentBitset()) {}

        EntityState::EntityState(EntityVersion version) : version(version) {}

        EntityId EntityState::id(EntityIndex index) const { return {version, index}; }

        ComponentBitset* EntityState::getComponentMask() { return &componentMask; }

        void EntityState::reset() {
            version++;
            componentMask.reset();
        }


        EntitySet::EntitySet(std::vector<ComponentId> componentIds) :
                componentIds(componentIds),
                mask(ComponentBitset()) {
            std::sort(componentIds.begin(), componentIds.end());
            mask.set(&componentIds);
            entities.push_back(INVALID);
            entities.reserve(MAX_ENTITY_AMOUNT);
            internIndices.reserve(MAX_ENTITY_AMOUNT);
            freeInternIndices.reserve(MAX_ENTITY_AMOUNT);
        }

        void EntitySet::updateMembership(EntityIndex entityIndex, ComponentBitset *previous, ComponentBitset *recent) {
            if (previous->contains(&mask)) {
                if (recent->contains(&mask)) // nothing changed
                    return;

                freeInternIndices.push_back(internIndices[entityIndex]);
                entities[internIndices[entityIndex]] = INVALID; // doesn't contain entity any more
                internIndices[entityIndex] = INVALID;
                return;
            }

            if (!recent->contains(&mask)) // nothing changed
                return;

            add(entityIndex);      // add entityId, because it's not added yet, but should be
        }

        void EntitySet::add(EntityIndex entityIndex) {
            if (!freeInternIndices.empty()) {
                internIndices[entityIndex] = freeInternIndices.back();
                freeInternIndices.pop_back();
                entities[internIndices[entityIndex]] = entityIndex;
            } else {
                entities.push_back(entityIndex);
                internIndices[entityIndex] = entities.size() - 1;
            }
        }

        bool EntitySet::concern(std::vector<ComponentId> *vector) {
            if (componentIds.size() != vector->size())
                return false;
            for (size_t i = 0; i < componentIds.size(); ++i)
                if (componentIds[i] != (*vector)[i])
                    return false;
            return true;
        }

        uint32 EntitySet::getVagueAmount() {
            return entities.size() - freeInternIndices.size();
        }

    }      // end private



    ////////////////////////////////////////
    //////////////    Core    //////////////
    ////////////////////////////////////////

    Core::Core() :
            entities(std::vector<Core_Intern::EntityState>(MAX_ENTITY_AMOUNT + 1)) {
        componentHandles.reserve(MAX_COMPONENT_AMOUNT + 1);
        componentHandles.push_back(nullptr);
        entities[0] = Core_Intern::EntityState();
#if USE_ECS_EVENTS==1
        entityCreatedEventId_ = generateEvent();
        entityErasedEventId_ = generateEvent();
#endif
    }


    Core::~Core() {
        for (ComponentHandle *ch : componentHandles) delete ch;
        for (Core_Intern::EntitySet *set : entitySets) delete set;
        for (Core_Intern::SetIterator *setIterator : setIterators) delete setIterator;
    }


    EntityId Core::createEntity() {

        EntityIndex index;

        if (!freeEntityIndices.empty()) {
            index = freeEntityIndices.back();
            freeEntityIndices.pop_back();
        } else {
            index = ++lastEntityIndex;
            entities[index] = Core_Intern::EntityState(EntityVersion(1));
        }

        EntityId entityId = entities[index].id(index);

#if USE_ECS_EVENTS==1
        auto event = Events::EntityCreatedEvent(entityId);
        emitEvent(entityCreatedEventId_, &event);
#endif

        return entityId;
    }


    bool Core::isValid(EntityId entityId) {
        return getIndex(entityId) != INVALID;
    }

    bool Core::eraseEntity(EntityId entityId) {

        EntityIndex index = getIndex(entityId);
        if (index == INVALID)
            return false;

#if USE_ECS_EVENTS==1
        auto eventErased = Events::EntityErasedEvent(entityId);
        emitEvent(entityErasedEventId_, &eventErased);
#endif

        Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

        for (ComponentId i = 1; i < componentHandles.size(); i++) {
            ComponentHandle *ch = componentHandles[i];
            if (originally.isSet(i)) {   // Only delete existing components
                ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
                auto event = Events::ComponentDeletedEvent(entityId);
                emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
            }
        }

        entities[index].reset();
        updateAllMemberships(entityId, &originally, entities[index].getComponentMask());

        freeEntityIndices.push_back(index);

        return true;
    }


    ComponentId Core::registerComponent(ComponentHandle* ch) {

        if (componentHandles.size() >= MAX_COMPONENT_AMOUNT) {
            throw std::length_error("To many component types registered! Define by MAX_COMPONENT_AMOUNT.");
        }

#if USE_ECS_EVENTS==1
        ComponentEventInfo& componentInfo = ch->getComponentEventInfo();
        componentInfo.addEventId = generateEvent();
        componentInfo.deleteEventId = generateEvent();
#endif
        componentHandles.push_back(ch);

        return componentHandles.size() - 1;
    }

    void* Core::addComponent(EntityId entityId, ComponentId componentId) {

        EntityIndex index = getIndex(entityId);
        if (index == INVALID)
            return nullptr;

        ComponentHandle* ch = componentHandles[componentId];
        Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

        if (!originally.isSet( componentId )) {   // Only update if component type is new for entity
            entities[index].getComponentMask()->set( componentId );
            updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
        } else {
            ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
            auto event = Events::ComponentDeletedEvent(entityId);
            emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
        }

        void* comp = ch->createComponent(index);

#if USE_ECS_EVENTS==1
        auto event = Events::ComponentAddedEvent(entityId);
        emitEvent(ch->getComponentEventInfo().addEventId, &event);
#endif

        return comp;
    }


    // OptimizeAble
    bool Core::activateComponents(EntityId entityId, const ComponentId* ids, size_t idsAmount) {

        EntityIndex index = getIndex(entityId);
        if (index == INVALID)
            return false;

        Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

        bool modified = false;
        for (uint32 i = 0; i < idsAmount; i++) {
            ComponentHandle* ch = componentHandles[ids[i]];
            if (originally.isSet(ids[i])) {
                ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
                auto event = Events::ComponentDeletedEvent(entityId);
                emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
            }
            else
                modified = true;
        }

        if (modified) {   // Only update if component types are new for entity

            for (uint32 i = 0; i < idsAmount; i++)
                entities[index].getComponentMask()->set(ids[i]);

            updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
        }

#if USE_ECS_EVENTS==1
        for (uint32 i = 0; i < idsAmount; i++) {
            ComponentHandle* ch = componentHandles[ids[i]];
            auto event = Events::ComponentAddedEvent(entityId);
            emitEvent(ch->getComponentEventInfo().addEventId, &event);
        }
#endif

        return true;
    }


    void* Core::getComponent(EntityId entityId, ComponentId componentId) {

        EntityIndex index = getIndex(entityId);
        if (index == INVALID)
            return nullptr;

        if (!entities[index].getComponentMask()->isSet(componentId))
            return nullptr;

        return componentHandles[componentId]->getComponent(index);
    }


    bool Core::deleteComponent(EntityId entityId, ComponentId componentId) {
        EntityIndex index = getIndex(entityId);
        if (index == INVALID)
            return false;

        Core_Intern::ComponentBitset originally = *entities[index].getComponentMask();

        if (originally.isSet(componentId)) {
            auto* ch = componentHandles[componentId];
            ch->destroyComponent(entityId, index);
#if USE_ECS_EVENTS == 1
            auto event = Events::ComponentDeletedEvent(entityId);
            emitEvent(ch->getComponentEventInfo().deleteEventId, &event);
#endif
            entities[index].getComponentMask()->unset( componentId );
            updateAllMemberships(entityId, &originally, entities[index].getComponentMask());
            return true;
        }

        return false;
    }


#if USE_ECS_EVENTS == 1

    EventId Core::componentDeletedEventId(ComponentId componentId) {
        auto* ch = componentHandles[componentId];
        return ch->getComponentEventInfo().deleteEventId;
    }

    EventId Core::componentAddedEventId(ComponentId componentId) {
        auto* ch = componentHandles[componentId];
        return ch->getComponentEventInfo().addEventId;
    }

    EventId Core::entityCreatedEventId() {
        return entityCreatedEventId_;
    }

    EventId Core::entityErasedEventId() {
        return entityErasedEventId_;
    }

#endif


    SetIteratorId Core::createSetIterator(std::vector<ComponentId> componentIds) {

        std::sort(componentIds.begin(), componentIds.end());
        Core_Intern::EntitySet *entitySet = nullptr;

        for (Core_Intern::EntitySet *set : entitySets)
            if (set->concern(&componentIds))
                entitySet = set;

        if (entitySet == nullptr) {
            entitySet = new Core_Intern::EntitySet(componentIds);
            entitySets.push_back(entitySet);

            // add all related Entities
            for (EntityIndex entityIndex = 1; entityIndex <= lastEntityIndex; entityIndex++)
                entitySet->dumbAddIfMember(entities[entityIndex].id(entityIndex), entities[entityIndex].getComponentMask());
        }

        setIterators.push_back(new Core_Intern::SetIterator(entitySet));
        return static_cast<SetIteratorId>(setIterators.size() - 1);
    }

    uint32 Core::getEntityAmount(SetIteratorId setIteratorId) {
        return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
    }

    uint32 Core::getEntityAmount(std::vector<ComponentId>& componentIds) {
        static SetIteratorId setIteratorId = createSetIterator(componentIds);
        while (nextEntity(setIteratorId).version != INVALID);
        return setIterators[setIteratorId]->getEntitySet()->getVagueAmount();
    }

    EntityId Core::getIdFromIndex(EntityIndex index) {
        if (index > lastEntityIndex)
            return {};
        return {entities[index].version, index};
    }

    void Core::updateAllMemberships(EntityId entityId, Core_Intern::ComponentBitset *previous, Core_Intern::ComponentBitset *recent) {
        for (Core_Intern::EntitySet *set : entitySets) {
            set->updateMembership(entityId.index, previous, recent);
        }
    }

}